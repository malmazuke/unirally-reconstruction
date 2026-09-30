"""ctypes driver for the pinned bsnes libretro core with laboratory exports.

The core is the upstream ``target-libretro`` build plus
``patches/bsnes-lab-exports.patch``, which adds ``unirally_*`` symbols for
work RAM, cartridge RAM, CPU registers, a deterministic reseed and an
instruction trace ring. Only this module knows the C ABI.
"""

from __future__ import annotations

import ctypes as C
import hashlib
import os
import struct
from dataclasses import dataclass
from pathlib import Path
from typing import Any

API_VERSION = 1

# libretro constants (libretro.h)
_ENV_GET_SYSTEM_DIRECTORY = 9
_ENV_SET_PIXEL_FORMAT = 10
_ENV_GET_VARIABLE = 15
_ENV_GET_VARIABLE_UPDATE = 17
_ENV_GET_LOG_INTERFACE = 27
_ENV_GET_SAVE_DIRECTORY = 31
_ENV_SET_GEOMETRY = 37
_ENV_GET_FASTFORWARDING = 49 | 0x10000
_PIXEL_FORMAT_XRGB8888 = 1
_REGION_NTSC, _REGION_PAL = 0, 1

BUTTONS = {"b": 0, "y": 1, "select": 2, "start": 3, "up": 4, "down": 5, "left": 6, "right": 7,
           "a": 8, "x": 9, "l": 10, "r": 11}

# Core options fixed for laboratory runs. ``bsnes_entropy=None`` gives an
# all-zero power-on RAM and a randomness source that always returns zero;
# everything else is the upstream default. Changing any of these changes the
# reference and must be recorded with the run.
DEFAULT_OPTIONS: dict[str, str] = {
    "bsnes_entropy": "None", "bsnes_hotfixes": "OFF", "bsnes_ppu_fast": "ON", "bsnes_dsp_fast": "ON",
    "bsnes_run_ahead_frames": "OFF", "bsnes_video_filter": "None", "bsnes_aspect_ratio": "Auto",
    "bsnes_blur_emulation": "OFF", "bsnes_cpu_overclock": "100", "bsnes_cpu_fastmath": "OFF",
    "bsnes_sa1_overclock": "100", "bsnes_sfx_overclock": "100", "bsnes_ppu_deinterlace": "ON",
    "bsnes_ppu_no_sprite_limit": "OFF", "bsnes_ppu_no_vram_blocking": "OFF", "bsnes_ppu_show_overscan": "OFF",
    "bsnes_mode7_scale": "1x", "bsnes_mode7_perspective": "ON", "bsnes_mode7_supersample": "OFF",
    "bsnes_mode7_mosaic": "ON", "bsnes_dsp_cubic": "OFF", "bsnes_dsp_echo_shadow": "OFF",
    "bsnes_coprocessor_delayed_sync": "ON", "bsnes_coprocessor_prefer_hle": "ON", "bsnes_sgb_bios": "SGB1.sfc",
}

_REGISTERS = struct.Struct("<IHHHHHBBBBHHBBBB")  # UnirallyLab::Registers
_TRACE_ENTRY = struct.Struct("<IHHHHHBBBBHH")   # UnirallyLab::TraceEntry


class CoreError(Exception):
    pass


class CoreMissingError(CoreError):
    """The core library cannot be loaded: a missing prerequisite."""


class _RetroVariable(C.Structure):
    _fields_ = [("key", C.c_char_p), ("value", C.c_char_p)]


class _RetroGameInfo(C.Structure):
    _fields_ = [("path", C.c_char_p), ("data", C.c_void_p), ("size", C.c_size_t), ("meta", C.c_char_p)]


@dataclass
class FrameOutput:
    video: tuple[int, int, str] | None  # width, height, sha256 of the XRGB8888 buffer
    audio_sha256: str
    audio_frames: int


def registers_to_dict(raw: bytes) -> dict[str, int]:
    pc, a, x, y, s, d, b, p, e, mdr, v, h, field, wai, stp, _ = _REGISTERS.unpack(raw)
    return {"pc": pc, "a": a, "x": x, "y": y, "s": s, "d": d, "b": b, "p": p, "e": e, "mdr": mdr,
            "vcounter": v, "hcounter": h, "field": field, "wai": wai, "stp": stp}


class BsnesCore:
    """One loaded core instance. A process should create at most one."""

    def __init__(self, library: Path, system_dir: Path, options: dict[str, str] | None = None) -> None:
        self.library = Path(library)
        self.options = {**DEFAULT_OPTIONS, **(options or {})}
        unknown = set(self.options) - set(DEFAULT_OPTIONS)
        if unknown:
            raise CoreError(f"unknown core options {sorted(unknown)}")
        if not self.library.is_file():
            raise CoreMissingError(f"core library not found: {self.library}")
        try:
            self._lib = C.CDLL(str(self.library))
        except OSError as exc:
            raise CoreMissingError(f"cannot load {self.library}: {exc}") from exc
        try:
            self._lib.unirally_api_version.restype = C.c_uint32
            api = self._lib.unirally_api_version()
        except AttributeError as exc:
            raise CoreError(f"{self.library} lacks the laboratory exports (unpatched core?)") from exc
        if api != API_VERSION:
            raise CoreError(f"core laboratory API {api} != expected {API_VERSION}")
        self.api_version = api
        self._system_dir = Path(system_dir)
        self._system_dir.mkdir(parents=True, exist_ok=True)
        # ctypes copies a c_char_p's pointer, not the buffer: the encoded bytes
        # objects themselves must stay referenced for as long as the core may
        # read them, so they live in these attributes for the core's lifetime.
        self._system_dir_bytes = str(self._system_dir).encode()
        self._option_bytes = {key.encode(): value.encode() for key, value in self.options.items()}
        self._keep: list[Any] = []
        self.serialization_method: str | None = None
        self._inputs: dict[int, set[int]] = {0: set(), 1: set()}
        self._frame_video: tuple[int, int, str] | None = None
        # M1-01 (additive): when ``keep_frame`` is set the XRGB8888 frame buffer
        # of the last run_frame is retained as (width, height, pitch, bytes).
        self.keep_frame = False
        self.frame_raw: tuple[int, int, int, bytes] | None = None
        self._frame_audio = hashlib.sha256()
        self._frame_audio_frames = 0
        self.input_polls = 0
        self.unknown_env: set[int] = set()
        self.loaded = False
        self._bind()

    # ------------------------------------------------------------ binding

    def _bind(self) -> None:
        lib = self._lib

        @C.CFUNCTYPE(C.c_bool, C.c_uint, C.c_void_p)
        def environ_cb(cmd, data):
            if cmd == _ENV_SET_PIXEL_FORMAT:
                return C.cast(data, C.POINTER(C.c_uint))[0] == _PIXEL_FORMAT_XRGB8888
            if cmd == _ENV_GET_VARIABLE:
                var = C.cast(data, C.POINTER(_RetroVariable))
                value = self._option_bytes.get(var[0].key) if var[0].key else None
                if value is None:
                    return False
                var[0].value = value  # points into the bytes object held by self._option_bytes
                return True
            if cmd == _ENV_GET_VARIABLE_UPDATE:
                C.cast(data, C.POINTER(C.c_bool))[0] = False
                return True
            if cmd in (_ENV_GET_SYSTEM_DIRECTORY, _ENV_GET_SAVE_DIRECTORY):
                C.cast(data, C.POINTER(C.c_char_p))[0] = self._system_dir_bytes
                return True
            if cmd == _ENV_GET_FASTFORWARDING:
                C.cast(data, C.POINTER(C.c_bool))[0] = False
                return True
            if cmd == _ENV_SET_GEOMETRY:
                return True
            if cmd != _ENV_GET_LOG_INTERFACE:
                self.unknown_env.add(cmd)
            return False

        @C.CFUNCTYPE(None, C.c_void_p, C.c_uint, C.c_uint, C.c_size_t)
        def video_cb(data, width, height, pitch):
            if data:
                raw = C.string_at(data, pitch * height)
                self._frame_video = (width, height, hashlib.sha256(raw).hexdigest())
                if self.keep_frame:
                    self.frame_raw = (width, height, pitch, raw)

        @C.CFUNCTYPE(None, C.c_int16, C.c_int16)
        def audio_cb(left, right):
            self._frame_audio.update(struct.pack("<hh", left, right))
            self._frame_audio_frames += 1

        @C.CFUNCTYPE(C.c_size_t, C.POINTER(C.c_int16), C.c_size_t)
        def audio_batch_cb(data, frames):
            self._frame_audio.update(C.string_at(data, frames * 4))
            self._frame_audio_frames += frames
            return frames

        @C.CFUNCTYPE(None)
        def input_poll_cb():
            self.input_polls += 1

        @C.CFUNCTYPE(C.c_int16, C.c_uint, C.c_uint, C.c_uint, C.c_uint)
        def input_state_cb(port, device, index, id_):
            return 1 if id_ in self._inputs.get(port, ()) else 0

        self._keep += [environ_cb, video_cb, audio_cb, audio_batch_cb, input_poll_cb, input_state_cb]
        lib.retro_set_environment(environ_cb)
        lib.retro_set_video_refresh(video_cb)
        lib.retro_set_audio_sample(audio_cb)
        lib.retro_set_audio_sample_batch(audio_batch_cb)
        lib.retro_set_input_poll(input_poll_cb)
        lib.retro_set_input_state(input_state_cb)
        lib.retro_load_game.argtypes = [C.POINTER(_RetroGameInfo)]
        lib.retro_load_game.restype = C.c_bool
        lib.retro_get_region.restype = C.c_uint
        lib.retro_serialize_size.restype = C.c_size_t
        lib.retro_serialize.argtypes = [C.c_void_p, C.c_size_t]
        lib.retro_serialize.restype = C.c_bool
        lib.retro_unserialize.argtypes = [C.c_void_p, C.c_size_t]
        lib.retro_unserialize.restype = C.c_bool
        lib.unirally_memory.argtypes = [C.c_uint, C.POINTER(C.c_size_t)]
        lib.unirally_memory.restype = C.c_void_p
        lib.unirally_cpu_registers.argtypes = [C.c_void_p, C.c_size_t]
        lib.unirally_cpu_registers.restype = C.c_size_t
        lib.unirally_reseed.argtypes = [C.c_uint32, C.c_uint32]
        lib.unirally_trace_enable.argtypes = [C.c_size_t]
        lib.unirally_trace_enable.restype = C.c_bool
        lib.unirally_trace_read.argtypes = [C.c_void_p, C.c_size_t]
        lib.unirally_trace_read.restype = C.c_size_t
        lib.unirally_trace_total.restype = C.c_uint64
        lib.unirally_set_serialization_method.argtypes = [C.c_char_p]
        lib.unirally_set_serialization_method.restype = C.c_bool
        lib.unirally_trace_entry_size.restype = C.c_size_t
        lib.retro_init()

    # ---------------------------------------------------------- lifecycle

    def load(self, rom: Path) -> None:
        rom = Path(rom)
        if not rom.is_file():
            raise CoreMissingError(f"ROM not found: {rom}")
        info = _RetroGameInfo(str(rom).encode(), None, 0, None)
        self._keep.append(info)
        if not self._lib.retro_load_game(C.byref(info)):
            raise CoreError(f"core refused to load {rom}")
        self.loaded = True
        # Serialized states must not carry a clock-derived seed (see task M0-03, attempt 2).
        self._lib.unirally_reseed(0, 0)
        entry = self._lib.unirally_trace_entry_size()
        if entry != _TRACE_ENTRY.size:
            raise CoreError(f"trace entry size {entry} != {_TRACE_ENTRY.size}")

    def set_serialization_method(self, method: str) -> None:
        """"Fast" (bsnes default) or "Strict": how the cores are synchronized before serializing."""
        if not self._lib.unirally_set_serialization_method(method.encode()):
            raise CoreError(f"unknown serialization method {method!r}")
        self.serialization_method = method

    def unload(self) -> None:
        if self.loaded:
            self._lib.retro_unload_game()
            self.loaded = False
        self._lib.retro_deinit()

    @property
    def region(self) -> str:
        return "PAL" if self._lib.retro_get_region() == _REGION_PAL else "NTSC"

    # ----------------------------------------------------------- running

    def set_inputs(self, port: int, buttons: set[str]) -> None:
        unknown = set(buttons) - set(BUTTONS)
        if unknown:
            raise CoreError(f"unknown buttons {sorted(unknown)}")
        self._inputs[port] = {BUTTONS[b] for b in buttons}

    def run_frame(self) -> FrameOutput:
        self._frame_video = None
        self.frame_raw = None
        self._frame_audio = hashlib.sha256()
        self._frame_audio_frames = 0
        self._lib.retro_run()
        return FrameOutput(self._frame_video, self._frame_audio.hexdigest(), self._frame_audio_frames)

    # ----------------------------------------------------------- capture

    def _memory(self, id_: int) -> bytes:
        size = C.c_size_t()
        ptr = self._lib.unirally_memory(id_, C.byref(size))
        if not ptr or size.value == 0:
            return b""
        return C.string_at(ptr, size.value)

    def wram(self) -> bytes:
        return self._memory(0)

    def cartridge_ram(self) -> bytes:
        return self._memory(1)

    def write_cartridge_ram(self, offset: int, data: bytes) -> None:
        """Write bytes into the core's live cartridge RAM (the save RAM the game reads and writes)."""
        size = C.c_size_t()
        ptr = self._lib.unirally_memory(1, C.byref(size))
        if not ptr or offset < 0 or offset + len(data) > size.value:
            raise CoreError(f"cartridge RAM write at {offset:#x} ({len(data)} bytes) is outside its {size.value} bytes")
        C.memmove(ptr + offset, data, len(data))

    def registers_raw(self) -> bytes:
        buf = C.create_string_buffer(_REGISTERS.size)
        n = self._lib.unirally_cpu_registers(buf, _REGISTERS.size)
        if n != _REGISTERS.size:
            raise CoreError(f"register block size {n} != {_REGISTERS.size}")
        return buf.raw

    def registers(self) -> dict[str, int]:
        return registers_to_dict(self.registers_raw())

    def serialize(self) -> bytes:
        size = self._lib.retro_serialize_size()
        buf = C.create_string_buffer(size)
        if not self._lib.retro_serialize(buf, size):
            raise CoreError("retro_serialize failed")
        return buf.raw

    def unserialize(self, blob: bytes) -> bool:
        return bool(self._lib.retro_unserialize(blob, len(blob)))

    def trace_enable(self, entries: int) -> None:
        if not self._lib.unirally_trace_enable(entries):
            raise CoreError(f"cannot allocate a trace ring of {entries} entries")

    def trace_total(self) -> int:
        return int(self._lib.unirally_trace_total())

    def trace_read_raw(self, max_entries: int) -> bytes:
        """Up to ``max_entries`` of the ring as raw entries, oldest first (M1-01, additive)."""
        buf = C.create_string_buffer(_TRACE_ENTRY.size * max_entries)
        n = self._lib.unirally_trace_read(buf, max_entries)
        return buf.raw[:n * _TRACE_ENTRY.size]

    def trace_read(self, max_entries: int) -> list[dict[str, int]]:
        return trace_entries_from_raw(self.trace_read_raw(max_entries))

    def trace_read_newest(self, entries: int, capacity: int) -> list[dict[str, int]]:
        """The newest ``entries`` of a ring of ``capacity`` (the ring copies oldest first)."""
        raw = self.trace_read_raw(capacity)
        return trace_entries_from_raw(raw[max(0, len(raw) - entries * _TRACE_ENTRY.size):])


def trace_entries_from_raw(raw: bytes) -> list[dict[str, int]]:
    out = []
    for pc, a, x, y, s, d, b, p, e, _, v, h in _TRACE_ENTRY.iter_unpack(raw):
        out.append({"pc": pc, "a": a, "x": x, "y": y, "s": s, "d": d, "b": b, "p": p, "e": e, "vcounter": v, "hcounter": h})
    return out


def frame_png(width: int, height: int, pitch: int, raw: bytes) -> bytes:
    """Encode an XRGB8888 frame buffer as an 8-bit RGB PNG (zlib only, no dependency)."""
    import struct as _struct
    import zlib

    def chunk(kind: bytes, body: bytes) -> bytes:
        return _struct.pack(">I", len(body)) + kind + body + _struct.pack(">I", zlib.crc32(kind + body) & 0xFFFFFFFF)

    rows = bytearray()
    for y in range(height):
        row = raw[y * pitch:y * pitch + width * 4]
        rows.append(0)  # filter: none
        # little-endian XRGB8888 stores B, G, R, X per pixel
        rows += bytes(v for px in range(width) for v in (row[px * 4 + 2], row[px * 4 + 1], row[px * 4]))
    header = _struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)
    return b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", header) + chunk(b"IDAT", zlib.compress(bytes(rows), 9)) + chunk(b"IEND", b"")


def library_name() -> str:
    return {"darwin": "bsnes_libretro.dylib", "linux": "bsnes_libretro.so"}.get(os.uname().sysname.lower(), "bsnes_libretro.so")

# AUDIO-TITLE-MENU observation ABI is additive; existing API-1 cores remain usable.
_AUDIO_EVENT = struct.Struct('<QQQQIHHBB')


def bind_audio(core: BsnesCore) -> None:
    """Bind only on explicit audio capture, so old references remain loadable."""
    lib = core._lib
    try:
        lib.unirally_audio_api_version.restype = C.c_uint32
        if lib.unirally_audio_api_version() != 1:
            raise CoreError('unsupported audio observation ABI')
        lib.unirally_audio_byte_order.restype = C.c_uint32
        if lib.unirally_audio_byte_order() != 1:
            raise CoreError('audio observation ABI requires a little-endian host')
        lib.unirally_audio_dsp_balance.restype = C.c_int64
        lib.unirally_audio_event_size.restype = C.c_size_t
        if lib.unirally_audio_event_size() != _AUDIO_EVENT.size:
            raise CoreError('audio event ABI size mismatch')
        lib.unirally_audio_enable.argtypes = [C.c_size_t, C.c_bool]
        lib.unirally_audio_enable.restype = C.c_bool
        lib.unirally_audio_read.argtypes = [C.c_void_p, C.c_size_t]
        lib.unirally_audio_read.restype = C.c_size_t
        for name in ('total', 'dropped', 'smp_ticks', 'dsp_clocks', 'smp_frequency', 'cpu_frequency'):
            getattr(lib, 'unirally_audio_' + name).restype = C.c_uint64
        lib.unirally_audio_apu_frequency.restype = C.c_double
    except AttributeError as exc:
        raise CoreMissingError('core lacks the additive audio observation exports') from exc


def read_audio_raw(core: BsnesCore, capacity: int) -> bytes:
    buf = C.create_string_buffer(_AUDIO_EVENT.size * capacity)
    count = core._lib.unirally_audio_read(buf, capacity)
    if count > capacity:
        raise CoreError('audio observation returned more than its capacity')
    return buf.raw[:count * _AUDIO_EVENT.size]
