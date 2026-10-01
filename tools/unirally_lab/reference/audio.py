"""Laboratory-only raw DSP observation; never an executable content source.

Events are packed little-endian <QQQQIHHBB: sequence, elapsed SMP divider ticks,
completed DSP clocks, CPU stepOnce ticks, next PC, address, value, kind, reserved.
Sequence is authoritative coroutine execution order. CPU/SMP counters are separate
clock domains; do not sort by converted time. PCM events carry L/R bit patterns
in address/value and mark wrapper delivery, not the DSP phase of generation.
"""
from __future__ import annotations

import ctypes as C
import hashlib
import json
import struct
from pathlib import Path
from typing import Any

from . import bsnes

KINDS = {1: 'dsp_run', 2: 'pcm_pair', 3: 'dsp_write', 4: 'smp_ram_write',
         5: 'cpu_port_write', 6: 'cpu_port_read', 7: 'smp_port_write',
         8: 'smp_port_read', 9: 'smp_port_clear_pair', 10: 'smp_instruction', 11: 'smp_registers_xsp',
         12: 'cpu_registers_ax',
         13: 'cpu_registers_ys', 14: 'frame_boundary_hv', 15: 'cpu_registers_bep'}


class AudioCapture:
    """Preallocated core buffer, drained once per frame; loss invalidates capture."""
    def __init__(self, core: bsnes.BsnesCore, directory: Path, capacity: int,
                 instructions: bool, ram_frames: set[int], identity: dict[str, Any], cpu_watch: list[int] | None = None) -> None:
        if not 1 <= capacity <= 1_000_000:
            raise bsnes.CoreError('audio capacity must be in 1..1000000')
        if core.options['bsnes_run_ahead_frames'] != 'OFF':
            raise bsnes.CoreError('audio capture requires run-ahead OFF')
        if len(cpu_watch or []) > 64 or any(not 0 <= pc <= 0xffffff for pc in cpu_watch or []):
            raise bsnes.CoreError('audio CPU watches must be at most 64 24-bit PCs')
        bsnes.bind_audio(core)
        if cpu_watch and core._lib.unirally_audio_api_version() < 3:
            raise bsnes.CoreError('audio CPU watches require observation ABI 3')
        directory.mkdir(parents=True, exist_ok=False)
        self.core, self.directory, self.capacity = core, directory, capacity
        self.ram_frames = ram_frames
        self.event_file = (directory / 'events.bin').open('wb')
        self.pcm_file = (directory / 'pcm.s16le').open('wb')
        self.event_hash, self.pcm_hash = hashlib.sha256(), hashlib.sha256()
        self.sequence, self.pairs = 0, 0
        self.kind_counts = {name: 0 for name in KINDS.values()}
        lib = core._lib
        self.data = {'schema_version': 1, 'status': 'incomplete', 'identity': identity,
                     'observation_abi': lib.unirally_audio_api_version(),
                     'cpu_watch': cpu_watch or [], 'all_smp_instructions': instructions,
                     'event_layout': '<QQQQIHHBB', 'event_size': bsnes._AUDIO_EVENT.size,
                     'kinds': KINDS, 'pcm_format': 'signed 16-bit little-endian interleaved L/R',
                     'pcm_position': 'wrapper delivery after DSP run, before float/resampling',
                     'clock_order': 'global sequence; independent integer SMP/CPU and completed DSP clocks',
                     'apu_frequency_hz': lib.unirally_audio_apu_frequency(),
                     'smp_frequency_hz': lib.unirally_audio_smp_frequency(),
                     'cpu_frequency_hz': lib.unirally_audio_cpu_frequency(),
                     'frames': [], 'ram_snapshots': []}
        self.snapshot(-1)
        self._write_metadata()
        if not lib.unirally_audio_enable(capacity, instructions):
            self.finish('failed', 'audio buffer allocation failed')
            raise bsnes.CoreError('audio buffer allocation failed')
        if lib.unirally_audio_api_version() >= 3:
            watched = (C.c_uint32 * len(cpu_watch or []))(*(cpu_watch or []))
            if not lib.unirally_audio_watch_cpu(watched, len(watched)):
                self.finish('failed', 'CPU watch setup failed')
                raise bsnes.CoreError('CPU watch setup failed')

    def snapshot(self, frame: int) -> None:
        raw = self.core._memory(2)
        if len(raw) != 65536:
            raise bsnes.CoreError('audio capture needs exactly 64 KiB APU RAM')
        name = 'apu-initial.bin' if frame < 0 else f'apu-{frame:05d}.bin'
        (self.directory / name).write_bytes(raw)
        self.data['ram_snapshots'].append({'frame': frame, 'path': name,
                                         'sha256': hashlib.sha256(raw).hexdigest()})

    def drain(self, frame: int) -> None:
        lib = self.core._lib
        if lib.unirally_audio_api_version() >= 3:
            lib.unirally_audio_frame_boundary()
        raw = bsnes.read_audio_raw(self.core, self.capacity)
        dropped, total = lib.unirally_audio_dropped(), lib.unirally_audio_total()
        if dropped or total != self.sequence + len(raw) // bsnes._AUDIO_EVENT.size:
            raise bsnes.CoreError(f'frame {frame}: lost audio events, total={total}, dropped={dropped}')
        if 2 * lib.unirally_audio_dsp_clocks() - lib.unirally_audio_smp_ticks() != lib.unirally_audio_dsp_balance():
            raise bsnes.CoreError(f'frame {frame}: audio clock accounting invariant failed')
        pcm = bytearray()
        first_pair, first_sequence = self.pairs, self.sequence
        for seq, smp, dsp, cpu, pc, address, value, kind, reserved in bsnes._AUDIO_EVENT.iter_unpack(raw):
            if seq != self.sequence or kind not in KINDS or reserved:
                raise bsnes.CoreError(f'frame {frame}: invalid audio event sequence/layout')
            self.sequence += 1
            self.kind_counts[KINDS[kind]] += 1
            if kind == 2:
                pcm.extend(struct.pack('<HH', address, value))
                self.pairs += 1
        self.event_file.write(raw); self.event_hash.update(raw)
        self.pcm_file.write(pcm); self.pcm_hash.update(pcm)
        self.data['frames'].append({'frame': frame, 'first_sequence': first_sequence,
                                   'events': self.sequence - first_sequence, 'first_pair': first_pair,
                                   'pairs': self.pairs - first_pair,
                                   'smp_ticks': lib.unirally_audio_smp_ticks(),
                                   'dsp_clocks': lib.unirally_audio_dsp_clocks(),
                                   'pcm_sha256': hashlib.sha256(pcm).hexdigest()})
        if lib.unirally_audio_api_version() >= 3:
            self.data['frames'][-1].update(cpu_ticks=lib.unirally_audio_cpu_ticks(),
                                          smp_balance=lib.unirally_audio_smp_balance(),
                                          dsp_balance=lib.unirally_audio_dsp_balance())
        if frame in self.ram_frames:
            self.snapshot(frame)

    def _write_metadata(self) -> None:
        (self.directory / 'audio.json').write_text(json.dumps(self.data, indent=2) + '\n')

    def finish(self, status: str = 'complete', failure: str | None = None) -> dict[str, Any]:
        lib = self.core._lib
        self.data.update(status=status, failure=failure, events=self.sequence, pairs=self.pairs,
                         kinds_count=self.kind_counts, dropped=lib.unirally_audio_dropped(),
                         events_sha256=self.event_hash.hexdigest(), pcm_sha256=self.pcm_hash.hexdigest())
        self.event_file.close(); self.pcm_file.close()
        lib.unirally_audio_enable(0, False)
        self._write_metadata()
        return {'path': str(self.directory / 'audio.json'), 'status': status,
                'events': self.sequence, 'pairs': self.pairs, 'failure': failure}
