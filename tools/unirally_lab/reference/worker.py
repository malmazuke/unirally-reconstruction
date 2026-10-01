#!/usr/bin/env python3
"""Fresh-process reference run: execute one script on the core and write samples.

Usage: worker.py --core LIB --rom ROM --script S --samples-out OUT
                 [--system-dir D] [--state-in P] [--save-after N --state-out P]
                 [--fields F] [--stop-after-frame N] [--wram-dump-out P]
                 [--coverage-out P [--coverage-ring N]] [--frame-image N ...] [--frame-image-dir D]
                 [--access-out P [--access-ring N] [--access-from-frame N] [--access-to-frame N]
                  [--access-watch-address A ...] [--access-watch-pc P ...]
                  [--wram-series-out P --wram-series-range START LENGTH [--wram-series-every N]]]

The M1-02 options (``--access-*``, ``--wram-series-*``) are additive in the
same way: the trace ring is drained per frame into the access derivation
(``access.derive``) over the chosen frame window, the work RAM at the end of
each drained frame is read, and the optional work RAM series records its
range at the end of every executed frame on the stride; nothing else changes.

The M1-01 options are additive: without ``--coverage-out`` and
``--frame-image`` every output, exit code and digest is as before. With
``--coverage-out`` the trace ring is enlarged to ``--coverage-ring`` entries
and drained after every frame (``coverage.drain``); the samples' 64-entry
trace window is then the newest entries of that ring, as before.

Exit codes follow the repository convention: 0 success, 1 run failure,
2 missing core/ROM, 3 invalid script or arguments. Timeouts are enforced by
the parent (``project.py reference ...``), which kills this process.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import sys
import tempfile
import time
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from unirally_lab import EXIT_FAILURE, EXIT_INVALID_INPUT, EXIT_MISSING_PREREQUISITE, EXIT_OK  # noqa: E402
from unirally_lab.access import derive as access_derive  # noqa: E402
from unirally_lab.coverage import drain as coverage_drain  # noqa: E402
from unirally_lab.reference import audio as audio_capture
from unirally_lab.reference import bsnes  # noqa: E402

SCRIPT_SCHEMA_VERSION = 1
# Samples schema 2 (M0-04) adds per-frame ``fields`` (declared work RAM
# ranges as hex), the top-level ``fields`` declaration, ``process`` identity,
# ``stop_after_frame``/``wram_dump`` records and reads the trace window
# before the final serialize. The sample digest is unchanged from schema 1.
SAMPLES_SCHEMA_VERSION = 2
WRAM_SIZE = 0x20000  # SNES work RAM $7E0000-$7FFFFF; checked against the core at run time
MAX_FIELDS = 64


class ScriptError(ValueError):
    pass


def load_script(path: Path) -> dict[str, Any]:
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, ValueError) as exc:
        raise ScriptError(f"cannot read script {path}: {exc}") from exc
    return validate_script(data)


def validate_script(data: Any) -> dict[str, Any]:
    if not isinstance(data, dict) or data.get("schema_version") != SCRIPT_SCHEMA_VERSION:
        raise ScriptError(f"script schema_version must be {SCRIPT_SCHEMA_VERSION}")
    if data.get("core", "bsnes") != "bsnes":
        raise ScriptError(f"unsupported core {data.get('core')!r}")
    frames = data.get("frames")
    if not isinstance(frames, int) or isinstance(frames, bool) or frames <= 0:
        raise ScriptError("frames must be a positive integer")
    every = data.get("sample_every", 1)
    if not isinstance(every, int) or isinstance(every, bool) or every <= 0:
        raise ScriptError("sample_every must be a positive integer")
    trace = data.get("trace_entries", 64)
    if not isinstance(trace, int) or isinstance(trace, bool) or trace < 0 or trace > 1_000_000:
        raise ScriptError("trace_entries must be an integer in 0..1000000")
    inputs = data.get("inputs", [])
    if not isinstance(inputs, list):
        raise ScriptError("inputs must be a list")
    for i, entry in enumerate(inputs):
        if not isinstance(entry, dict):
            raise ScriptError(f"inputs[{i}] must be an object")
        lo, hi, port = entry.get("from"), entry.get("to"), entry.get("port", 0)
        buttons = entry.get("buttons")
        if not all(isinstance(v, int) and not isinstance(v, bool) for v in (lo, hi, port)):
            raise ScriptError(f"inputs[{i}]: from, to and port must be integers")
        if lo < 0 or hi < lo or port not in (0, 1):
            raise ScriptError(f"inputs[{i}]: need 0 <= from <= to and port in (0, 1)")
        if not isinstance(buttons, list) or not buttons or any(b not in bsnes.BUTTONS for b in buttons):
            raise ScriptError(f"inputs[{i}]: buttons must be a non-empty list from {sorted(bsnes.BUTTONS)}")
    writes = data.get("cartridge_ram_writes", [])
    if not isinstance(writes, list):
        raise ScriptError("cartridge_ram_writes must be a list")
    for i, entry in enumerate(writes):
        if not isinstance(entry, dict) or set(entry) != {"after_frame", "offset", "byte"}:
            raise ScriptError(f"cartridge_ram_writes[{i}] must be an object with after_frame, offset and byte")
        if not all(isinstance(v, int) and not isinstance(v, bool) for v in entry.values()):
            raise ScriptError(f"cartridge_ram_writes[{i}]: after_frame, offset and byte must be integers")
        if not (0 <= entry["after_frame"] < frames and entry["offset"] >= 0 and 0 <= entry["byte"] <= 0xFF):
            raise ScriptError(f"cartridge_ram_writes[{i}]: need 0 <= after_frame < frames, offset >= 0, 0 <= byte <= 255")
        if any((v["after_frame"], v["offset"]) == (entry["after_frame"], entry["offset"]) for v in writes[:i]):
            raise ScriptError(f"cartridge_ram_writes[{i}]: a second write to the same byte after the same frame")
    options = data.get("core_options", {})
    if not isinstance(options, dict) or any(not isinstance(k, str) or not isinstance(v, str) for k, v in options.items()):
        raise ScriptError("core_options must map strings to strings")
    if set(options) - set(bsnes.DEFAULT_OPTIONS):
        raise ScriptError(f"unknown core_options {sorted(set(options) - set(bsnes.DEFAULT_OPTIONS))}")
    return data


def validate_fields(data: Any, wram_size: int = WRAM_SIZE) -> list[dict[str, Any]]:
    """Declared capture fields: named, non-overlapping-name work RAM ranges.

    Each entry is ``{"name": str, "start": int, "length": int}`` with the
    range inside work RAM. The worker records every sampled frame's bytes of
    each range as lowercase hex under ``frames[].fields[name]``.
    """
    if not isinstance(data, list):
        raise ScriptError("fields must be a list")
    if len(data) > MAX_FIELDS:
        raise ScriptError(f"at most {MAX_FIELDS} fields may be captured")
    names: set[str] = set()
    for i, entry in enumerate(data):
        if not isinstance(entry, dict):
            raise ScriptError(f"fields[{i}] must be an object")
        name, start, length = entry.get("name"), entry.get("start"), entry.get("length")
        if not isinstance(name, str) or not name or name in names:
            raise ScriptError(f"fields[{i}]: name must be a unique non-empty string")
        if not all(isinstance(v, int) and not isinstance(v, bool) for v in (start, length)):
            raise ScriptError(f"fields[{i}] {name!r}: start and length must be integers")
        if start < 0 or length <= 0 or start + length > wram_size:
            raise ScriptError(f"fields[{i}] {name!r}: range {start}+{length} lies outside work RAM of {wram_size} bytes")
        names.add(name)
    return [{"name": e["name"], "start": e["start"], "length": e["length"]} for e in data]


def capture_fields(wram: bytes, fields: list[dict[str, Any]]) -> dict[str, str]:
    return {f["name"]: wram[f["start"]:f["start"] + f["length"]].hex() for f in fields}


STATE_IDENTITY_KEYS = ("sha256", "core_sha256", "rom_sha256", "script_sha256", "serialization_method")


def validate_state_sidecar(meta: Any, actual: dict[str, Any], script_frames: int) -> tuple[int, dict[str, Any]]:
    """Check a state's sidecar against the current run; returns (resume frame, post-serialize sample).

    A state is only meaningful for the exact core build, ROM, script and
    synchronization method it was taken with, and must leave frames to run.
    """
    if not isinstance(meta, dict):
        raise ScriptError("state sidecar must be a JSON object")
    try:
        after = meta["after_frame"]
        expected = {k: meta[k] for k in STATE_IDENTITY_KEYS}
        post = meta["post_serialize"]
        post_ok = isinstance(post, dict) and isinstance(post.get("wram_sha256"), str) and isinstance(post.get("registers"), dict)
    except (KeyError, TypeError) as exc:
        raise ScriptError(f"state sidecar incomplete: {exc}") from exc
    if not post_ok:
        raise ScriptError("state sidecar lacks a post_serialize sample")
    mismatched = [k for k in STATE_IDENTITY_KEYS if expected[k] != actual.get(k)]
    if mismatched:
        raise ScriptError(f"state does not belong to this run: {', '.join(mismatched)} differ from the sidecar")
    if not isinstance(after, int) or isinstance(after, bool) or not (0 <= after < script_frames - 1):
        raise ScriptError(f"state after frame {after!r} leaves no frames to run in a {script_frames}-frame script")
    return after + 1, post


def forced_sample_frames(script_frames: int, save_after: int | None, start: int) -> set[int]:
    """Frames sampled regardless of ``sample_every``: the last frame, the save
    frame and the one after it, and the resume frame after a restore, so that a
    divergence right after a save or restore cannot fall between samples.
    ``script_frames`` is the number of frames the run will execute (the
    script's count, or ``--stop-after-frame`` + 1 when the run is cut short)."""
    forced = {script_frames - 1}
    if save_after is not None:
        forced |= {save_after, save_after + 1}
    if start > 0:
        forced.add(start)
    return {f for f in forced if 0 <= f < script_frames}


def should_sample(frame: int, sample_every: int, forced: set[int], dense_from: int | None = None) -> bool:
    """Sample on the grid, at forced frames, and every frame from ``dense_from`` on."""
    if dense_from is not None and frame >= dense_from:
        return True
    return frame % sample_every == 0 or frame in forced


def inputs_for_frame(script: dict[str, Any], frame: int) -> dict[int, set[str]]:
    result: dict[int, set[str]] = {0: set(), 1: set()}
    for entry in script.get("inputs", []):
        if entry["from"] <= frame <= entry["to"]:
            result[entry.get("port", 0)].update(entry["buttons"])
    return result


def cartridge_ram_writes_after(script: dict[str, Any], frame: int) -> list[tuple[int, int]]:
    """The (offset, byte) writes into cartridge RAM made once the frame has run and been sampled."""
    return [(w["offset"], w["byte"]) for w in script.get("cartridge_ram_writes", []) if w["after_frame"] == frame]


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as fh:
        for block in iter(lambda: fh.read(1 << 20), b""):
            h.update(block)
    return h.hexdigest()


def run(args: argparse.Namespace) -> int:
    if getattr(args, 'audio_out', None) and (args.state_in or args.save_after is not None):
        print('raw audio capture currently requires uninterrupted cold runs', file=sys.stderr)
        return EXIT_INVALID_INPUT
    script_path = Path(args.script)
    try:
        script = load_script(script_path)
    except ScriptError as exc:
        print(f"invalid script: {exc}", file=sys.stderr)
        return EXIT_INVALID_INPUT
    if (args.save_after is None) != (args.state_out is None):
        print("--save-after and --state-out must be given together", file=sys.stderr)
        return EXIT_INVALID_INPUT
    if args.sample_from_frame is not None and args.sample_from_frame < 0:
        print("--sample-from-frame must not be negative", file=sys.stderr)
        return EXIT_INVALID_INPUT
    if args.save_after is not None and not (0 <= args.save_after < script["frames"]):
        print("--save-after must lie inside the script's frame range", file=sys.stderr)
        return EXIT_INVALID_INPUT
    if args.stop_after_frame is not None and not (0 <= args.stop_after_frame < script["frames"]):
        print("--stop-after-frame must lie inside the script's frame range", file=sys.stderr)
        return EXIT_INVALID_INPUT
    if args.coverage_out and args.coverage_ring <= 0:
        print("--coverage-ring must be positive", file=sys.stderr)
        return EXIT_INVALID_INPUT
    if any(a < 0 or a > 0xFFFFFF for a in args.coverage_watch or []):
        print("--coverage-watch addresses must be 24-bit", file=sys.stderr)
        return EXIT_INVALID_INPUT
    if args.access_out and args.access_ring <= 0:
        print("--access-ring must be positive", file=sys.stderr)
        return EXIT_INVALID_INPUT
    if any(a < 0 or a > 0xFFFFFF for a in (args.access_watch_address or []) + (args.access_watch_pc or [])):
        print("--access-watch-address and --access-watch-pc must be 24-bit", file=sys.stderr)
        return EXIT_INVALID_INPUT
    if args.access_from_frame is not None and not (0 <= args.access_from_frame < script["frames"]):
        print("--access-from-frame must lie inside the script's frame range", file=sys.stderr)
        return EXIT_INVALID_INPUT
    if args.access_to_frame is not None and not (0 <= args.access_to_frame < script["frames"]):
        print("--access-to-frame must lie inside the script's frame range", file=sys.stderr)
        return EXIT_INVALID_INPUT
    if args.access_from_frame is not None and args.access_to_frame is not None and args.access_to_frame < args.access_from_frame:
        print("--access-to-frame must not precede --access-from-frame", file=sys.stderr)
        return EXIT_INVALID_INPUT
    if (args.wram_series_out is None) != (args.wram_series_range is None):
        print("--wram-series-out and --wram-series-range must be given together", file=sys.stderr)
        return EXIT_INVALID_INPUT
    if args.wram_series_range is not None:
        ws, wl = args.wram_series_range
        if ws < 0 or wl <= 0 or ws + wl > WRAM_SIZE or args.wram_series_every <= 0:
            print("--wram-series-range must lie inside work RAM and --wram-series-every must be positive", file=sys.stderr)
            return EXIT_INVALID_INPUT
        if not args.access_out:
            print("--wram-series-out requires --access-out", file=sys.stderr)
            return EXIT_INVALID_INPUT
    frame_images = set(args.frame_image or [])
    if any(f < 0 or f >= script["frames"] for f in frame_images):
        print("--frame-image frames must lie inside the script's frame range", file=sys.stderr)
        return EXIT_INVALID_INPUT
    fields: list[dict[str, Any]] = []
    if args.fields:
        try:
            fields = validate_fields(json.loads(Path(args.fields).read_text(encoding="utf-8")))
        except (OSError, ValueError) as exc:  # ScriptError is a ValueError
            print(f"invalid fields file {args.fields}: {exc}", file=sys.stderr)
            return EXIT_INVALID_INPUT

    rom = Path(args.rom)
    if not rom.is_file():
        print(f"ROM not found: {rom}", file=sys.stderr)
        return EXIT_MISSING_PREREQUISITE
    # The core reads and writes cartridge RAM (save.ram) in its system/save
    # directory. Every run gets a private, empty directory so that a previous
    # run's save file cannot become this run's initial SRAM (M0-03 attempt 8).
    base = Path(args.system_dir) if args.system_dir else Path(args.samples_out).parent / "core-system"
    base.mkdir(parents=True, exist_ok=True)
    system_dir = Path(tempfile.mkdtemp(prefix="run-", dir=base))
    try:
        core = bsnes.BsnesCore(Path(args.core), system_dir, script.get("core_options"))
    except bsnes.CoreMissingError as exc:
        print(str(exc), file=sys.stderr)
        return EXIT_MISSING_PREREQUISITE
    except bsnes.CoreError as exc:
        print(str(exc), file=sys.stderr)
        return EXIT_FAILURE

    started = time.monotonic()
    out: dict[str, Any] = {
        "schema_version": SAMPLES_SCHEMA_VERSION,
        "core": {"library": str(core.library), "sha256": sha256_file(core.library), "api_version": core.api_version,
                 "options": dict(core.options)},
        "rom": {"path": str(rom), "sha256": sha256_file(rom), "size": rom.stat().st_size},
        "script": {"path": str(script_path), "sha256": sha256_file(script_path), "frames": script["frames"],
                   "sample_every": script.get("sample_every", 1), "sample_from_frame": args.sample_from_frame},
        "stop_after_frame": args.stop_after_frame,
        "fields": fields,
        "process": {"pid": os.getpid(), "parent_pid": os.getppid(), "argv": sys.argv,
                    "started_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())},
        "frames": [],
    }
    try:
        core.load(rom)
    except bsnes.CoreMissingError as exc:
        print(str(exc), file=sys.stderr)
        return EXIT_MISSING_PREREQUISITE
    except bsnes.CoreError as exc:
        print(str(exc), file=sys.stderr)
        return EXIT_FAILURE
    try:
        core.set_serialization_method(args.serialization_method)
    except bsnes.CoreError as exc:
        print(str(exc), file=sys.stderr)
        return EXIT_INVALID_INPUT
    out["core"]["serialization_method"] = core.serialization_method
    out["rom"]["region"] = core.region
    out["wram_size"] = len(core.wram())
    out["cartridge_ram_size"] = len(core.cartridge_ram())
    if fields and any(f["start"] + f["length"] > out["wram_size"] for f in fields):
        print(f"declared fields exceed the core's work RAM of {out['wram_size']} bytes", file=sys.stderr)
        return EXIT_INVALID_INPUT
    if any(w["offset"] >= out["cartridge_ram_size"] for w in script.get("cartridge_ram_writes", [])):
        print(f"a cartridge RAM write lies outside the core's {out['cartridge_ram_size']} bytes", file=sys.stderr)
        return EXIT_INVALID_INPUT
    out["initial"] = {"wram_sha256": hashlib.sha256(core.wram()).hexdigest(),
                      "cartridge_ram_sha256": hashlib.sha256(core.cartridge_ram()).hexdigest(),
                      "registers": core.registers(), "save_files_present": sorted(p.name for p in system_dir.iterdir())}
    trace_entries = script.get("trace_entries", 64)
    coverage: coverage_drain.FrameDrain | None = None
    access: access_derive.AccessDrain | None = None
    ring_capacity = trace_entries
    if args.coverage_out or args.access_out:
        ring_capacity = max(trace_entries, args.coverage_ring if args.coverage_out else 0, args.access_ring if args.access_out else 0)
        core.trace_enable(ring_capacity)
        if args.coverage_out:
            coverage = coverage_drain.FrameDrain(core, ring_capacity, args.coverage_watch)
        if args.access_out:
            series = None
            if args.wram_series_out:
                series = (args.wram_series_range[0], args.wram_series_range[1], args.wram_series_every, Path(args.wram_series_out))
            access = access_derive.AccessDrain(rom.read_bytes(), ring_capacity, args.access_watch_address, args.access_watch_pc, series)
    elif trace_entries:
        core.trace_enable(trace_entries)
    access_seen_total = core.trace_total() if access is not None else 0
    access_window: tuple[int, int] | None = None
    if frame_images:
        core.keep_frame = True
        image_dir = Path(args.frame_image_dir) if args.frame_image_dir else Path(args.samples_out).parent / "frames"
        image_dir.mkdir(parents=True, exist_ok=True)
        out["frame_images"] = []

    start = 0
    if args.state_in:
        state_in = Path(args.state_in)
        if not state_in.is_file():
            print(f"state file not found: {state_in}", file=sys.stderr)
            return EXIT_MISSING_PREREQUISITE
        blob = state_in.read_bytes()
        actual = {"sha256": hashlib.sha256(blob).hexdigest(), "core_sha256": out["core"]["sha256"], "rom_sha256": out["rom"]["sha256"],
                  "script_sha256": out["script"]["sha256"], "serialization_method": core.serialization_method}
        try:
            meta = json.loads((state_in.with_suffix(state_in.suffix + ".json")).read_text(encoding="utf-8"))
            start, post = validate_state_sidecar(meta, actual, script["frames"])
        except (OSError, ValueError) as exc:  # ScriptError is a ValueError
            print(f"state sidecar rejected: {exc}", file=sys.stderr)
            return EXIT_INVALID_INPUT
        if not core.unserialize(blob):
            print("core rejected the state", file=sys.stderr)
            return EXIT_FAILURE
        regs = core.registers()
        wram_sha = hashlib.sha256(core.wram()).hexdigest()
        out["state_in"] = {"path": str(state_in), **actual, "after_frame": start - 1, "resumed_at_frame": start,
                           "wram_sha256": wram_sha, "registers": regs,
                           "matches_post_serialize": wram_sha == post["wram_sha256"] and regs == post["registers"]}

    sample_every = script.get("sample_every", 1)
    end = script["frames"] if args.stop_after_frame is None else args.stop_after_frame + 1
    if end <= start:
        print(f"--stop-after-frame {args.stop_after_frame} lies before the first frame to run ({start})", file=sys.stderr)
        return EXIT_INVALID_INPUT
    forced = forced_sample_frames(end, args.save_after, start)
    if access is not None:
        access_window = (start if args.access_from_frame is None else max(start, args.access_from_frame),
                         end - 1 if args.access_to_frame is None else min(end - 1, args.access_to_frame))
        if access_window[1] < access_window[0]:
            print(f"access window {access_window} is empty for frames {start}..{end - 1}", file=sys.stderr)
            return EXIT_INVALID_INPUT
        if access_window[0] == start:
            access.set_previous_wram(core.wram())
    state_digest = hashlib.sha256()
    state_digest.update(b"initial" + bytes.fromhex(out["initial"]["wram_sha256"]) + bytes.fromhex(out["initial"]["cartridge_ram_sha256"]))
    av_digest = hashlib.sha256()
    audio = None
    if getattr(args, 'audio_out', None):
        try:
            audio = audio_capture.AudioCapture(core, Path(args.audio_out), args.audio_capacity,
                args.audio_instructions, set(args.audio_ram_frame or []),
                {key: out[key] for key in ('core', 'rom', 'script')}, args.audio_cpu_watch, args.audio_smp_watch, args.audio_cpu_instructions)
        except (bsnes.CoreError, OSError) as exc:
            print(f'audio capture setup failed: {exc}', file=sys.stderr)
            core.unload(); shutil.rmtree(system_dir, ignore_errors=True)
            return EXIT_FAILURE
    for frame in range(start, end):
        for port, buttons in inputs_for_frame(script, frame).items():
            core.set_inputs(port, buttons)
        output = core.run_frame()
        if audio is not None:
            try:
                audio.drain(frame)
            except bsnes.CoreError as exc:
                audio.finish('failed', str(exc))
                print(f'audio capture failed: {exc}', file=sys.stderr)
                core.unload(); shutil.rmtree(system_dir, ignore_errors=True)
                return EXIT_FAILURE
        if coverage is not None:
            try:
                coverage.drain_frame(frame)
            except coverage_drain.DrainOverflow as exc:
                print(f"coverage capture failed: {exc}", file=sys.stderr)
                _write_coverage(Path(args.coverage_out), coverage, out, (start, frame), failure=str(exc))
                core.unload()
                shutil.rmtree(system_dir, ignore_errors=True)
                return EXIT_FAILURE
        if access is not None:
            total_now = core.trace_total()
            delta = total_now - access_seen_total
            access_seen_total = total_now
            if access.series is not None:
                access.series_frame(frame, core.wram())
            if access_window[0] <= frame <= access_window[1]:
                try:
                    if delta > ring_capacity:
                        raise access_derive.DrainOverflow(f"frame {frame}: {delta} instructions exceed the ring capacity {ring_capacity}; entries were lost")
                    raw = core.trace_read_raw(ring_capacity) if delta else b""
                    available = len(raw) // access_derive.TRACE_ENTRY_SIZE
                    if available < delta:
                        raise access_derive.DrainOverflow(f"frame {frame}: ring returned {available} entries for a delta of {delta}")
                    access.drain_frame(frame, raw[(available - delta) * access_derive.TRACE_ENTRY_SIZE:], core.wram())
                except access_derive.DrainOverflow as exc:
                    print(f"access capture failed: {exc}", file=sys.stderr)
                    _write_access(Path(args.access_out), access, out, (access_window[0], frame), failure=str(exc))
                    core.unload()
                    shutil.rmtree(system_dir, ignore_errors=True)
                    return EXIT_FAILURE
            elif frame == access_window[0] - 1:
                access.set_previous_wram(core.wram())
        if frame in frame_images:
            if core.frame_raw is None:
                out["frame_images"].append({"frame": frame, "path": None, "note": "no video output this frame"})
            else:
                width, height, pitch, raw = core.frame_raw
                image = image_dir / f"frame-{frame:05d}.png"
                image.write_bytes(bsnes.frame_png(width, height, pitch, raw))
                out["frame_images"].append({"frame": frame, "path": str(image), "width": width, "height": height,
                                            "sha256": hashlib.sha256(image.read_bytes()).hexdigest()})
        if should_sample(frame, sample_every, forced, args.sample_from_frame):
            wram = core.wram()
            wram_sha = hashlib.sha256(wram).hexdigest()
            regs_raw = core.registers_raw()
            state_digest.update(frame.to_bytes(4, "little") + bytes.fromhex(wram_sha) + regs_raw)
            av_digest.update(frame.to_bytes(4, "little") + (output.video[2] if output.video else "").encode() + output.audio_sha256.encode())
            out["frames"].append({"frame": frame, "wram_sha256": wram_sha, "registers": bsnes.registers_to_dict(regs_raw),
                                  "fields": capture_fields(wram, fields),
                                  "video": output.video, "audio_sha256": output.audio_sha256, "audio_frames": output.audio_frames})
        # A script's cartridge RAM writes (FIFTH-WIN-COMPLETION): made after the frame's sample and
        # before a state saved after it, so the next frame and a resumed run both see them.
        for offset, byte in cartridge_ram_writes_after(script, frame):
            try:
                core.write_cartridge_ram(offset, bytes([byte]))
            except bsnes.CoreError as exc:
                print(f"cartridge RAM write failed: {exc}", file=sys.stderr)
                core.unload()
                shutil.rmtree(system_dir, ignore_errors=True)
                return EXIT_INVALID_INPUT
        if args.save_after is not None and frame == args.save_after:
            blob = core.serialize()
            state_out = Path(args.state_out)
            state_out.parent.mkdir(parents=True, exist_ok=True)
            state_out.write_bytes(blob)
            # Serializing first runs the cores to a synchronization point, so the
            # state corresponds to this post-serialize sample, not to the
            # frame-end sample above (M0-03 review finding).
            meta = {"after_frame": frame, "sha256": hashlib.sha256(blob).hexdigest(), "size": len(blob),
                    "core_sha256": out["core"]["sha256"], "rom_sha256": out["rom"]["sha256"], "script_sha256": out["script"]["sha256"],
                    "serialization_method": core.serialization_method,
                    "post_serialize": {"wram_sha256": hashlib.sha256(core.wram()).hexdigest(), "registers": core.registers()}}
            state_out.with_suffix(state_out.suffix + ".json").write_text(json.dumps(meta, indent=2, sort_keys=True) + "\n", encoding="utf-8")
            out["state_out"] = {"path": str(state_out), **meta}

    out["start_frame"], out["end_frame"] = start, end - 1
    out["sample_digest"] = state_digest.hexdigest()
    out["av_digest"] = av_digest.hexdigest()
    if args.wram_dump_out:
        # Raw work RAM at the end of the last executed frame, before the final
        # serialize below moves the cores to a synchronization point.
        dump = Path(args.wram_dump_out)
        dump.parent.mkdir(parents=True, exist_ok=True)
        wram = core.wram()
        dump.write_bytes(wram)
        out["wram_dump"] = {"after_frame": end - 1, "path": str(dump), "sha256": hashlib.sha256(wram).hexdigest(), "size": len(wram)}
    if trace_entries:
        # Read at the end of the last frame: the final serialize below executes
        # a few more instructions under Strict synchronization (R-0002 finding 7).
        window = core.trace_read_newest(trace_entries, ring_capacity) if coverage is not None else core.trace_read(trace_entries)
        out["trace"] = {"instructions_executed": core.trace_total(), "window": window}
    if coverage is not None:
        out["coverage"] = _write_coverage(Path(args.coverage_out), coverage, out, (start, end - 1))
    if access is not None:
        out["access"] = _write_access(Path(args.access_out), access, out, access_window)
    if audio is not None:
        out['audio_capture'] = audio.finish()
    final_state = core.serialize()
    out["final"] = {"wram_sha256": hashlib.sha256(core.wram()).hexdigest(),
                    "cartridge_ram_sha256": hashlib.sha256(core.cartridge_ram()).hexdigest(),
                    "state_sha256": hashlib.sha256(final_state).hexdigest(), "state_size": len(final_state),
                    "registers": core.registers()}
    if trace_entries:
        out["trace"]["instructions_executed_after_final_serialize"] = core.trace_total()
    out["input_polls"] = core.input_polls
    out["unknown_environment_commands"] = sorted(core.unknown_env)
    out["elapsed_seconds"] = round(time.monotonic() - started, 3)
    core.unload()
    out["final"]["save_files_written"] = sorted(p.name for p in system_dir.iterdir())
    shutil.rmtree(system_dir, ignore_errors=True)
    samples = Path(args.samples_out)
    samples.parent.mkdir(parents=True, exist_ok=True)
    samples.write_text(json.dumps(out, indent=1, sort_keys=True) + "\n", encoding="utf-8")
    return EXIT_OK


def _write_coverage(path: Path, coverage: coverage_drain.FrameDrain, out: dict[str, Any], frames: tuple[int, int],
                    failure: str | None = None) -> dict[str, Any]:
    """Write the coverage document (M1-01); returns its summary for the samples file."""
    # Identity without host paths so that two captures of one run are byte-identical.
    identity = {"rom": {k: v for k, v in out["rom"].items() if k != "path"},
                "core": {k: v for k, v in out["core"].items() if k != "library"},
                "script": {k: v for k, v in out["script"].items() if k != "path"},
                "status": "failed" if failure else "complete", "failure": failure}
    doc = coverage.document(identity, frames)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(doc, sort_keys=True, separators=(",", ":")) + "\n", encoding="utf-8")
    return {"path": str(path), "sha256": sha256_file(path), "instructions": coverage.total, "max_frame_delta": coverage.max_delta,
            "ring_capacity": coverage.capacity, "sites": len(coverage.sites), "pairs": len(coverage.pairs), "status": identity["status"]}


def _write_access(path: Path, access: access_derive.AccessDrain, out: dict[str, Any], frames: tuple[int, int],
                  failure: str | None = None) -> dict[str, Any]:
    """Write the access record (M1-02); returns its summary for the samples file."""
    identity = {"rom": {k: v for k, v in out["rom"].items() if k != "path"},
                "core": {k: v for k, v in out["core"].items() if k not in ("library", "sha256")},
                "script": {k: v for k, v in out["script"].items() if k != "path"},
                "trace_total_at_end": out.get("trace", {}).get("instructions_executed"),
                "status": "failed" if failure else "complete", "failure": failure}
    doc = access.document(identity, frames)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(doc, sort_keys=True, separators=(",", ":")) + "\n", encoding="utf-8")
    return {"path": str(path), "sha256": sha256_file(path), "instructions": access.total, "accesses": access.total_accesses,
            "max_frame_delta": access.max_delta, "ring_capacity": access.capacity, "frames": list(frames), "status": identity["status"],
            "library_sha256": out["core"]["sha256"]}


def probe(args: argparse.Namespace) -> int:
    """Load the core without a ROM and print its laboratory API version."""
    try:
        core = bsnes.BsnesCore(Path(args.core), Path(args.system_dir or Path(args.core).parent / "core-system"))
    except bsnes.CoreMissingError as exc:
        print(json.dumps({"error": str(exc)}))
        return EXIT_MISSING_PREREQUISITE
    except bsnes.CoreError as exc:
        print(json.dumps({"error": str(exc)}))
        return EXIT_FAILURE
    print(json.dumps({"api_version": core.api_version, "library_sha256": sha256_file(core.library)}))
    return EXIT_OK


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--core", required=True)
    parser.add_argument("--system-dir")
    parser.add_argument("--probe", action="store_true", help="only load the core and report its API version")
    parser.add_argument("--rom")
    parser.add_argument("--script")
    parser.add_argument("--samples-out")
    parser.add_argument('--audio-out', help='new directory for raw DSP events/PCM/APU observation')
    parser.add_argument('--audio-capacity', type=int, default=262144)
    parser.add_argument('--audio-cpu-watch', type=lambda v: int(v, 0), action='append',
                        help='CPU PC with timed register observations (repeatable, maximum 64)')
    parser.add_argument('--audio-smp-watch', type=lambda v: int(v, 0), action='append',
                        help='SPC700 PC with timed register observations (repeatable, maximum 64)')
    parser.add_argument('--audio-cpu-instructions', action='store_true', help='include every CPU instruction boundary (observation ABI 5)')
    parser.add_argument('--audio-instructions', action='store_true', help='include SPC700 instruction boundaries')
    parser.add_argument('--audio-ram-frame', type=int, action='append', help='end-frame APU RAM snapshot (repeatable)')
    parser.add_argument("--state-in")
    parser.add_argument("--save-after", type=int)
    parser.add_argument("--state-out")
    parser.add_argument("--sample-from-frame", type=int,
                        help="sample every frame from this one on, regardless of sample_every (restore checks use it)")
    parser.add_argument("--fields", help="JSON list of named work RAM ranges to record at every sampled frame")
    parser.add_argument("--stop-after-frame", type=int,
                        help="run the script only up to and including this frame (the script and its digest are unchanged)")
    parser.add_argument("--wram-dump-out", help="write the raw work RAM after the last executed frame here")
    parser.add_argument("--serialization-method", default="Strict", choices=("Fast", "Strict"),
                        help="bsnes save-state synchronization method (default Strict; see R-0002)")
    parser.add_argument("--coverage-out", help="M1-01: drain the trace ring every frame and write the coverage document here")
    parser.add_argument("--coverage-ring", type=int, default=262144,
                        help="M1-01: ring capacity used with --coverage-out (default 262144; a frame executing more fails the run)")
    parser.add_argument("--coverage-watch", type=lambda v: int(v, 0), action="append",
                        help="M1-01: 24-bit address whose executions are counted per frame in the coverage document (repeatable)")
    parser.add_argument("--frame-image", type=int, action="append", help="M1-01: write this frame's video output as PNG (repeatable)")
    parser.add_argument("--frame-image-dir", help="M1-01: directory for --frame-image files (default <samples dir>/frames)")
    parser.add_argument("--access-out", help="M1-02: derive the memory access record from the trace ring every frame and write it here")
    parser.add_argument("--access-ring", type=int, default=262144, help="M1-02: ring capacity used with --access-out (default 262144)")
    parser.add_argument("--access-from-frame", type=int, help="M1-02: first frame of the access derivation window (default: the first frame run)")
    parser.add_argument("--access-to-frame", type=int, help="M1-02: last frame of the access derivation window (default: the last frame run)")
    parser.add_argument("--access-watch-address", type=lambda v: int(v, 0), action="append",
                        help="M1-02: 24-bit address whose accesses are logged per frame with values (repeatable)")
    parser.add_argument("--access-watch-pc", type=lambda v: int(v, 0), action="append",
                        help="M1-02: 24-bit pc whose registers are logged at every execution (repeatable)")
    parser.add_argument("--wram-series-out", help="M1-02: binary file receiving the work RAM range of --wram-series-range every --wram-series-every frames")
    parser.add_argument("--wram-series-range", type=lambda v: int(v, 0), nargs=2, metavar=("START", "LENGTH"),
                        help="M1-02: work RAM offset and length of the series (with --wram-series-out)")
    parser.add_argument("--wram-series-every", type=int, default=1, help="M1-02: frame stride of the series (default 1)")
    try:
        args = parser.parse_args(argv)
    except SystemExit as exc:
        return EXIT_INVALID_INPUT if exc.code not in (0, None) else EXIT_OK
    if args.probe:
        return probe(args)
    if not (args.rom and args.script and args.samples_out):
        print("--rom, --script and --samples-out are required", file=sys.stderr)
        return EXIT_INVALID_INPUT
    return run(args)


if __name__ == "__main__":
    sys.exit(main())
