"""Replay manifests: versioned descriptions of one reference scenario.

A manifest (schema 1) fixes everything a fresh host needs to reproduce a
reference run and compare it: scenario identity, ROM and core identity, the
initial procedure (cold start or a validated state sidecar), both
controllers' inputs with their timing unit and injection point, the sampled
field schema and the regeneration command. Fields are raw diagnostic work
RAM ranges plus the whole-WRAM digest and the CPU register block; no field
carries a gameplay interpretation (that is M1).

Validation is purely structural and ROM-free. Whether the named ROM, core
build or state is actually present is a run-time prerequisite check.
"""

from __future__ import annotations

import json
import re
from pathlib import Path
from typing import Any

from ..reference import bsnes
from ..reference.worker import WRAM_SIZE, ScriptError, validate_fields, validate_script

MANIFEST_SCHEMA_VERSION = 1
ORIGIN_KINDS = ("cold_start", "state")
FIELD_KINDS = ("wram_sha256", "registers", "wram_range")
BUILTIN_FIELDS = {"wram_sha256", "registers"}
TIMING_UNITS = ("frame",)
PORTS = (0, 1)
_SHA256 = re.compile(r"^[0-9a-f]{64}$")
_SCENARIO_ID = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._-]{0,63}$")
# Required checks a stored state must have passed to be a reference origin (D-0001).
RESTORE_CHECKS = ("save_does_not_perturb", "restore_matches_saved_state", "restore_continuation_identical")


class ManifestError(ValueError):
    pass


def _is_int(value: Any) -> bool:
    return isinstance(value, int) and not isinstance(value, bool)


def _require(data: dict[str, Any], key: str, where: str) -> Any:
    if key not in data:
        raise ManifestError(f"{where} lacks {key!r}")
    return data[key]


def _sha(value: Any, where: str) -> str:
    if not isinstance(value, str) or not _SHA256.match(value):
        raise ManifestError(f"{where} must be a lowercase hex SHA-256")
    return value


def _relpath(value: Any, where: str) -> str:
    if not isinstance(value, str) or not value or Path(value).is_absolute() or ".." in Path(value).parts:
        raise ManifestError(f"{where} must be a relative path inside the repository")
    return value


def load_manifest(path: Path) -> dict[str, Any]:
    try:
        data = json.loads(Path(path).read_text(encoding="utf-8"))
    except (OSError, ValueError) as exc:
        raise ManifestError(f"cannot read manifest {path}: {exc}") from exc
    return validate_manifest(data)


def validate_manifest(data: Any) -> dict[str, Any]:
    """Structural validation; every defect is reported as ManifestError (exit 3), never a traceback."""
    try:
        return _validate(data)
    except ManifestError:
        raise
    except ScriptError as exc:  # from the shared field/script validators
        raise ManifestError(str(exc)) from exc
    except (KeyError, TypeError, AttributeError, ValueError) as exc:  # malformed nesting of an untrusted document
        raise ManifestError(f"malformed manifest: {exc!r}") from exc


def _validate(data: Any) -> dict[str, Any]:
    if not isinstance(data, dict) or data.get("schema_version") != MANIFEST_SCHEMA_VERSION:
        raise ManifestError(f"manifest schema_version must be {MANIFEST_SCHEMA_VERSION}")
    scenario = _require(data, "scenario_id", "manifest")
    if not isinstance(scenario, str) or not _SCENARIO_ID.match(scenario):
        raise ManifestError("scenario_id must match [A-Za-z0-9][A-Za-z0-9._-]{0,63}")
    for key in ("description", "tested_behavior", "regeneration_command"):
        value = _require(data, key, "manifest")
        if not isinstance(value, str) or not value.strip():
            raise ManifestError(f"{key} must be a non-empty string")

    rom = _require(data, "rom", "manifest")
    if not isinstance(rom, dict):
        raise ManifestError("rom must be an object")
    _relpath(_require(rom, "manifest", "rom"), "rom.manifest")
    _sha(_require(rom, "sha256", "rom"), "rom.sha256")

    core = _require(data, "core", "manifest")
    if not isinstance(core, dict):
        raise ManifestError("core must be an object")
    if _require(core, "name", "core") != "bsnes":
        raise ManifestError(f"unsupported core {core.get('name')!r}; only bsnes has an adapter")
    _relpath(_require(core, "lock", "core"), "core.lock")
    commit = _require(core, "commit", "core")
    if not isinstance(commit, str) or not re.match(r"^[0-9a-f]{40}$", commit):
        raise ManifestError("core.commit must be a full 40-hex-digit commit")
    _sha(_require(core, "patch_sha256", "core"), "core.patch_sha256")
    if _require(core, "serialization_method", "core") not in ("Fast", "Strict"):
        raise ManifestError("core.serialization_method must be Fast or Strict")
    options = core.get("options", {})
    if not isinstance(options, dict) or any(not isinstance(k, str) or not isinstance(v, str) for k, v in options.items()):
        raise ManifestError("core.options must map strings to strings")
    if set(options) - set(bsnes.DEFAULT_OPTIONS):
        raise ManifestError(f"unknown core.options {sorted(set(options) - set(bsnes.DEFAULT_OPTIONS))}")

    run = _require(data, "run", "manifest")
    if not isinstance(run, dict):
        raise ManifestError("run must be an object")
    frames = _require(run, "frames", "run")
    every = run.get("sample_every", 1)
    trace = run.get("trace_entries", 64)
    if not _is_int(frames) or frames <= 0:
        raise ManifestError("run.frames must be a positive integer")
    if not _is_int(every) or every <= 0:
        raise ManifestError("run.sample_every must be a positive integer")
    if not _is_int(trace) or trace < 0 or trace > 1_000_000:
        raise ManifestError("run.trace_entries must be an integer in 0..1000000")

    inputs = _require(data, "inputs", "manifest")
    if not isinstance(inputs, dict):
        raise ManifestError("inputs must be an object")
    if _require(inputs, "timing_unit", "inputs") not in TIMING_UNITS:
        raise ManifestError(f"inputs.timing_unit must be one of {TIMING_UNITS}")
    injection = _require(inputs, "injection_point", "inputs")
    if not isinstance(injection, str) or not injection.strip():
        raise ManifestError("inputs.injection_point must describe when inputs are applied")
    controllers = _require(inputs, "controllers", "inputs")
    if not isinstance(controllers, list):
        raise ManifestError("inputs.controllers must be a list")
    ports = [c.get("port") if isinstance(c, dict) else None for c in controllers]
    if sorted(ports, key=lambda p: (p is None, p)) != list(PORTS):
        raise ManifestError(f"inputs.controllers must list ports {list(PORTS)} exactly once each")
    for c in controllers:
        events = _require(c, "events", f"controller {c['port']}")
        if not isinstance(events, list):
            raise ManifestError(f"controller {c['port']}: events must be a list")
        for i, e in enumerate(events):
            if not isinstance(e, dict):
                raise ManifestError(f"controller {c['port']} events[{i}] must be an object")
            lo, hi, buttons = e.get("from"), e.get("to"), e.get("buttons")
            if not (_is_int(lo) and _is_int(hi)) or lo < 0 or hi < lo or hi >= frames:
                raise ManifestError(f"controller {c['port']} events[{i}]: need 0 <= from <= to < run.frames")
            if not isinstance(buttons, list) or not buttons or any(b not in bsnes.BUTTONS for b in buttons):
                raise ManifestError(f"controller {c['port']} events[{i}]: buttons must be a non-empty list from {sorted(bsnes.BUTTONS)}")
            if "port" in e:
                raise ManifestError(f"controller {c['port']} events[{i}]: the port is fixed by the controller entry")

    fields = _require(data, "fields", "manifest")
    if not isinstance(fields, list) or not fields:
        raise ManifestError("fields must be a non-empty list")
    names: set[str] = set()
    ranges: list[dict[str, Any]] = []
    for i, f in enumerate(fields):
        if not isinstance(f, dict):
            raise ManifestError(f"fields[{i}] must be an object")
        name, kind = f.get("name"), f.get("kind")
        if not isinstance(name, str) or not name or name in names:
            raise ManifestError(f"fields[{i}]: name must be a unique non-empty string")
        if kind not in FIELD_KINDS:
            raise ManifestError(f"fields[{i}] {name!r}: kind must be one of {FIELD_KINDS}")
        if kind in BUILTIN_FIELDS and name != kind:
            raise ManifestError(f"fields[{i}]: a {kind} field must be named {kind!r}")
        if kind == "wram_range":
            if name in BUILTIN_FIELDS:
                raise ManifestError(f"fields[{i}]: {name!r} is reserved for the built-in field")
            ranges.append({"name": name, "start": f.get("start"), "length": f.get("length")})
        names.add(name)
    validate_fields(ranges, WRAM_SIZE)

    origin = _require(data, "origin", "manifest")
    if not isinstance(origin, dict) or origin.get("kind") not in ORIGIN_KINDS:
        raise ManifestError(f"origin.kind must be one of {ORIGIN_KINDS}")
    if origin["kind"] == "state":
        _relpath(_require(origin, "path", "origin"), "origin.path")
        _sha(_require(origin, "sha256", "origin"), "origin.sha256")
        _relpath(_require(origin, "script", "origin"), "origin.script")
        _sha(_require(origin, "script_sha256", "origin"), "origin.script_sha256")
        after = _require(origin, "after_frame", "origin")
        if not _is_int(after) or not (0 <= after < frames - 1):
            raise ManifestError("origin.after_frame must leave frames to run")
        rc = _require(origin, "restore_check", "origin")
        if not isinstance(rc, dict):
            raise ManifestError("origin.restore_check must be an object naming the restore-check report")
        _relpath(_require(rc, "report", "origin.restore_check"), "origin.restore_check.report")
        if rc.get("save_after") != after:
            raise ManifestError("origin.restore_check.save_after must equal origin.after_frame")

    writes = data.get("cartridge_ram_writes", [])
    if not isinstance(writes, list):
        raise ManifestError("cartridge_ram_writes must be a list")
    for i, w in enumerate(writes):
        if not isinstance(w, dict) or set(w) != {"after_frame", "offset", "byte"} or not all(_is_int(v) for v in w.values()):
            raise ManifestError(f"cartridge_ram_writes[{i}] must be an object of integers after_frame, offset and byte")
        if not (0 <= w["after_frame"] < frames - 1 and 0 <= w["offset"] < 0x2000 and 0 <= w["byte"] <= 0xFF):
            raise ManifestError(f"cartridge_ram_writes[{i}]: need 0 <= after_frame < run.frames - 1, "
                                "0 <= offset < 0x2000 and 0 <= byte <= 255")
        if any((v["after_frame"], v["offset"]) == (w["after_frame"], w["offset"]) for v in writes[:i]):
            raise ManifestError(f"cartridge_ram_writes[{i}]: a second write to the same byte after the same frame")
        if origin["kind"] == "state" and w["after_frame"] <= origin["after_frame"]:
            raise ManifestError(f"cartridge_ram_writes[{i}]: a state origin's run starts after after_frame")

    expected = data.get("expected", {})
    if not isinstance(expected, dict):
        raise ManifestError("expected must be an object")
    for key in ("sample_digest", "final_state_sha256"):
        if key in expected:
            _sha(expected[key], f"expected.{key}")
    if set(expected) - {"sample_digest", "final_state_sha256"}:
        raise ManifestError(f"unknown expected keys {sorted(set(expected) - {'sample_digest', 'final_state_sha256'})}")
    return data


# ------------------------------------------------------------ derivation


def derive_script(manifest: dict[str, Any]) -> dict[str, Any]:
    """The reference script (schema 1) that executes this manifest from a cold start."""
    run = manifest["run"]
    inputs = []
    for c in sorted(manifest["inputs"]["controllers"], key=lambda c: c["port"]):
        for e in c["events"]:
            inputs.append({"from": e["from"], "to": e["to"], "port": c["port"], "buttons": list(e["buttons"])})
    script = {
        "schema_version": 1,
        "core": manifest["core"]["name"],
        "description": f"Derived from replay manifest {manifest['scenario_id']}; do not edit, regenerate with `{manifest['regeneration_command']}`.",
        "frames": run["frames"],
        "sample_every": run.get("sample_every", 1),
        "trace_entries": run.get("trace_entries", 64),
        "inputs": inputs,
        "core_options": dict(manifest["core"].get("options", {})),
    }
    if manifest.get("cartridge_ram_writes"):
        script["cartridge_ram_writes"] = [dict(w) for w in manifest["cartridge_ram_writes"]]
    return validate_script(script)


def script_equivalent(script: dict[str, Any], derived: dict[str, Any]) -> list[str]:
    """Keys on which a stored script differs from what the manifest describes."""
    diffs = []
    for key in ("frames", "sample_every", "trace_entries", "core_options"):
        if script.get(key, derived[key]) != derived[key]:
            diffs.append(key)
    norm = lambda entries: sorted((e["from"], e["to"], e.get("port", 0), tuple(sorted(e["buttons"]))) for e in entries)  # noqa: E731
    if norm(script.get("inputs", [])) != norm(derived["inputs"]):
        diffs.append("inputs")
    writes = lambda s: sorted((w["after_frame"], w["offset"], w["byte"]) for w in s.get("cartridge_ram_writes", []))  # noqa: E731
    if writes(script) != writes(derived):
        diffs.append("cartridge_ram_writes")
    return diffs


def range_fields(manifest: dict[str, Any]) -> list[dict[str, Any]]:
    return [{"name": f["name"], "start": f["start"], "length": f["length"]} for f in manifest["fields"] if f["kind"] == "wram_range"]


def field_names(manifest: dict[str, Any]) -> list[str]:
    return [f["name"] for f in manifest["fields"]]


def inputs_at(manifest: dict[str, Any], frame: int) -> dict[str, list[str]]:
    """Buttons held on each port during ``frame`` (sorted, per port)."""
    held: dict[str, list[str]] = {}
    for c in sorted(manifest["inputs"]["controllers"], key=lambda c: c["port"]):
        buttons: set[str] = set()
        for e in c["events"]:
            if e["from"] <= frame <= e["to"]:
                buttons.update(e["buttons"])
        held[str(c["port"])] = sorted(buttons)
    return held


def write_json(path: Path, data: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf-8")
