"""Validated-pack preparation and bounded SDL frontend launch.

The pack a launch uses is chosen by the profile recorded inside it, never by
its file name or by the track: `--pack PATH` names one pack, which must carry
the supported profile; without it, the newest pack under `local/` that
validates is used, or one is extracted from `--rom` to the profile's default
path. An existing pack is never replaced unless `--replace-pack` says so.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import subprocess
import sys
import time
from dataclasses import dataclass
from pathlib import Path

from .. import EXIT_FAILURE, EXIT_INVALID_INPUT, EXIT_MISSING_PREREQUISITE, EXIT_OK, EXIT_TIMEOUT
from .. import report as reportmod
from ..content import pack as packmod
from ..procs import run_bounded

ROOT = reportmod.repo_root()


@dataclass(frozen=True)
class FrontendPaths:
    pack: Path | None  # None: select by profile under local/, or extract there.
    rom: Path | None
    rules: Path
    executable: Path
    report: Path | None


def _canonical_path(value: str | Path) -> Path:
    # Quoted '~' remains a literal path component. An unquoted tilde is
    # expanded by the caller's shell before it reaches this command.
    return Path(value).resolve(strict=False)


def _default_executable(preset: str) -> Path:
    app_directory = ROOT / "build" / preset / "src" / "app"
    if sys.platform == "darwin":
        return app_directory / "unirally.app" / "Contents" / "MacOS" / "unirally"
    return app_directory / "unirally"


def _frontend_paths(args: argparse.Namespace) -> FrontendPaths:
    executable = (Path(args.executable) if args.executable else
                  _default_executable(args.preset))
    rom = (None if args.rom is None or not args.rom.strip()
           else _canonical_path(args.rom))
    return FrontendPaths(
        pack=None if args.pack is None else _canonical_path(args.pack),
        rom=rom,
        rules=_canonical_path(args.rules),
        executable=_canonical_path(executable),
        report=None if not args.report else _canonical_path(args.report),
    )


def _paths_alias(first: Path, second: Path) -> bool:
    if first == second:
        return True
    try:
        return first.samefile(second)
    except (FileNotFoundError, OSError):
        return False


def _report_collision(paths: FrontendPaths) -> tuple[str, Path] | None:
    if paths.report is None:
        return None
    candidates = [
        ("--rules", paths.rules),
        ("frontend executable", paths.executable),
    ]
    if paths.pack is not None:
        candidates.insert(0, ("--pack", paths.pack))
    if paths.rom is not None:
        candidates.append(("--rom", paths.rom))
    for label, input_path in candidates:
        if _paths_alias(paths.report, input_path):
            return label, input_path
    return None


def _finish(rep: reportmod.Report, report: Path | None, status: int) -> int:
    rep.finish("passed" if status == EXIT_OK else "failed")
    rep.write(report)
    reportmod.print_summary(rep)
    return status


def _track_choice(value: str) -> str:
    if value in ("dragster", "zoom-zoo") or (value.isdigit() and 0 <= int(value) <= 44):
        return value
    raise argparse.ArgumentTypeError("use dragster, zoom-zoo or a track number 0-44")


def default_pack_path(rules: dict) -> Path:
    """Where a pack of the rules' profile is extracted when no pack is named.

    The name is the profile itself, so a profile bump extracts to a fresh
    file instead of finding an older pack in its way.
    """
    return _canonical_path(ROOT / "local" / (str(rules["profile_id"]).replace(".", "-") + ".pack"))


def pack_profile(data: bytes) -> str | None:
    """The profile a pack records in its header, or None if the header is unreadable."""
    try:
        if data[:8] != packmod.MAGIC:
            return None
        offset = 8 + 4 + 32 + 32
        profile, _ = packmod._text(data, offset, "profile identity")
        return profile
    except ValueError:
        return None


def _select_local_pack(rep: reportmod.Report, rules: dict, rules_sha: str) -> tuple[Path, dict] | None:
    """The newest pack under local/ that validates against the rules."""
    candidates = sorted((p for p in (ROOT / "local").glob("*.pack") if p.is_file()),
                        key=lambda p: p.stat().st_mtime, reverse=True)
    rejected = []
    for candidate in candidates:
        candidate = _canonical_path(candidate)
        data = candidate.read_bytes()
        try:
            inspected = packmod.validate_pack(data, rules, rules_sha)
        except (ValueError, OSError):
            rejected.append(f"{candidate.name} ({pack_profile(data) or 'unreadable'})")
            continue
        detail = f"selected {candidate} by its profile {inspected['profile_id']}; ROM was not opened"
        if rejected:
            detail += "; not selected: " + ", ".join(rejected)
        rep.add_check("classic_pack", "passed", detail=detail)
        return candidate, inspected
    if rejected:
        rep.data["packs_not_selected"] = rejected
    return None


def _supported_profile_check(rep: reportmod.Report, executable: Path, profile: str, preset: str,
                             timeout: float) -> int | None:
    """Compare the built app's supported profiles with the rules' profile.

    The supported profile is compiled into the binary, so a build from before a
    profile bump rejects a correct new pack as unsupported. Asking the binary
    first turns that into a rebuild instruction. An executable that reports no
    profiles (older builds, test stand-ins) is recorded, not failed.
    """
    probe = run_bounded([str(executable), "--supported-profiles"], timeout=min(timeout, 30.0), cwd=ROOT)
    reported = [line.strip() for line in probe.stdout.splitlines() if line.strip()]
    if probe.outcome != "passed" or not reported:
        rep.add_check("supported_profile", "skipped",
                      detail="the frontend executable did not report its supported pack profiles")
        return None
    if profile in reported:
        rep.add_check("supported_profile", "passed", detail=f"the built app supports {profile}")
        return None
    rep.add_check("supported_profile", "missing",
                  detail=f"the built app supports {', '.join(reported)} but the pack rules declare {profile}; "
                         f"the build is stale: run `python3 tools/project.py build --preset {preset}`")
    return EXIT_MISSING_PREREQUISITE


def cmd_run(args: argparse.Namespace) -> int:
    try:
        paths = _frontend_paths(args)
    except (OSError, RuntimeError) as exc:
        print(f"cannot resolve frontend path: {exc}", file=sys.stderr)
        return EXIT_INVALID_INPUT
    collision = _report_collision(paths)
    if collision is not None:
        label, input_path = collision
        print(f"refusing to write --report onto {label} path {input_path}",
              file=sys.stderr)
        return EXIT_INVALID_INPUT
    rep = reportmod.Report(sys.argv, task_id=args.task)
    if (
        not math.isfinite(args.timeout)
        or args.timeout <= 0
        or (args.updates is not None and args.updates <= 0)
    ):
        rep.add_check("arguments", "failed", detail="--timeout must be finite and positive; --updates must be positive")
        return _finish(rep, paths.report, EXIT_INVALID_INPUT)
    rules_path = paths.rules
    executable = paths.executable
    try:
        rules, rules_sha = packmod.load_rules(rules_path)
        rep.add_input("extraction_rules", rules_path, rules_sha)
    except (FileNotFoundError, ValueError, json.JSONDecodeError, OSError) as exc:
        rep.add_check("extraction_rules", "failed", detail=str(exc))
        return _finish(rep, paths.report, EXIT_INVALID_INPUT)
    profile = str(rules["profile_id"])

    inspected = None
    replace_stale = None
    replace_reason = ""
    if paths.pack is None:
        selected = _select_local_pack(rep, rules, rules_sha)
        if selected is not None:
            pack_path, inspected = selected
        else:
            pack_path = default_pack_path(rules)
    else:
        pack_path = paths.pack

    if inspected is None and pack_path.exists():
        data = pack_path.read_bytes()
        try:
            inspected = packmod.validate_pack(data, rules, rules_sha)
        except (ValueError, OSError) as exc:
            recorded = pack_profile(data)
            identity = (f"; it records profile {recorded} and this launch needs {profile}"
                        if recorded and recorded != profile else "")
            if paths.rom is not None and getattr(args, "replace_pack", False):
                # The old pack is moved aside only once the new one has been
                # extracted, so a failed extraction leaves it where it was; the
                # check is recorded when the move has happened.
                replace_stale = pack_path.with_name(f"{pack_path.name}.stale-{time.strftime('%Y%m%dT%H%M%SZ', time.gmtime())}")
                replace_reason = f"{exc}{identity}"
            else:
                remedy = ("pass --replace-pack to rebuild it" if paths.rom is not None
                          else "pass --rom PATH with --replace-pack to rebuild it")
                rep.add_check("classic_pack", "failed",
                              detail=f"existing pack is invalid and was not replaced: {exc}{identity}; "
                                     f"move {pack_path} aside, or {remedy}")
                return _finish(rep, paths.report, EXIT_INVALID_INPUT)
        else:
            rep.add_check("classic_pack", "passed", detail=f"validated existing pack {pack_path}; ROM was not opened")

    if inspected is not None:
        rep.add_input("classic_pack", pack_path, inspected["pack_sha256"], size=inspected["pack_size"])
        rep.data["first_launch_extraction"] = False
    else:
        if paths.rom is None:
            rep.add_check("supported_rom", "missing",
                          detail=f"no pack with profile {profile} at {pack_path}; select the supported PAL ROM with --rom PATH to create it")
            return _finish(rep, paths.report, EXIT_MISSING_PREREQUISITE)
        rom_path = paths.rom
        if not rom_path.is_file():
            rep.add_check("supported_rom", "missing", detail=f"ROM does not exist: {rom_path}")
            return _finish(rep, paths.report, EXIT_MISSING_PREREQUISITE)
        try:
            rom = rom_path.read_bytes()
            rep.add_input("rom", rom_path, hashlib.sha256(rom).hexdigest(), size=len(rom))
            payload, rows = packmod.build_pack(rom, rules, rules_sha)
            rep.add_check("exact_rom_identity", "passed", detail=rules["source_rom"]["sha256"])
        except (ValueError, subprocess.CalledProcessError) as exc:
            if replace_stale is not None:
                rep.add_check("classic_pack", "failed",
                              detail=f"existing pack is invalid ({replace_reason}) and was not replaced: extraction failed")
            rep.add_check("supported_rom", "failed", detail=f"extraction failed, nothing was replaced: {exc}")
            return _finish(rep, paths.report, EXIT_INVALID_INPUT)
        except OSError as exc:
            rep.add_check("pack_creation", "failed", detail=str(exc))
            return _finish(rep, paths.report, EXIT_FAILURE)
        try:
            if replace_stale is not None:
                pack_path.rename(replace_stale)
                rep.add_check("classic_pack", "passed",
                              detail=f"existing pack was invalid ({replace_reason}); --replace-pack moved it to {replace_stale}")
                rep.add_artifact("replaced_pack", replace_stale)
            packmod.write_atomic(pack_path, payload)
            inspected = packmod.validate_pack(pack_path.read_bytes(), rules, rules_sha)
        except ValueError as exc:
            rep.add_check("supported_rom", "failed", detail=f"the extracted pack failed validation: {exc}")
            return _finish(rep, paths.report, EXIT_INVALID_INPUT)
        except OSError as exc:
            rep.add_check("pack_creation", "failed", detail=str(exc))
            return _finish(rep, paths.report, EXIT_FAILURE)
        rep.add_check("atomic_pack_creation", "passed", detail=f"{len(rows)} entries at {pack_path}")
        rep.add_input("classic_pack", pack_path, inspected["pack_sha256"], size=inspected["pack_size"])
        rep.add_artifact("classic_pack", pack_path)
        rep.data["first_launch_extraction"] = True

    if not executable.is_file():
        rep.add_check("frontend_executable", "missing", detail=f"{executable} not found; run `project.py build --preset {args.preset}`")
        return _finish(rep, paths.report, EXIT_MISSING_PREREQUISITE)
    stale_build = _supported_profile_check(rep, executable, profile, args.preset, args.timeout)
    if stale_build is not None:
        return _finish(rep, paths.report, stale_build)
    command = [str(executable), "--content-pack", str(pack_path)]
    track = getattr(args, "track", "dragster")
    if track != "dragster":
        command.extend(["--track", track])
    if args.hidden:
        command.append("--hidden")
    if args.updates is not None:
        command.extend(["--updates", str(args.updates)])
    if args.fixed_controller_mask is not None:
        command.extend(["--fixed-controller-mask", str(args.fixed_controller_mask)])
    launched = run_bounded(command, timeout=args.timeout, cwd=ROOT)
    detail = launched.tail(2000)
    if launched.outcome == "timeout":
        rep.add_check("frontend_launch", "timeout", detail=detail)
        return _finish(rep, paths.report, EXIT_TIMEOUT)
    if launched.missing:
        rep.add_check("frontend_launch", "missing", detail=detail)
        return _finish(rep, paths.report, EXIT_MISSING_PREREQUISITE)
    if launched.outcome != "passed":
        rep.add_check("frontend_launch", "failed", detail=detail)
        return _finish(rep, paths.report, EXIT_FAILURE)
    rep.add_check("frontend_launch", "passed", detail=detail or "frontend exited successfully")
    rep.data["audio"] = "intentionally omitted in M3"
    return _finish(rep, paths.report, EXIT_OK)


def register(sub: argparse._SubParsersAction) -> None:
    frontend = sub.add_parser("frontend", help="prepare and launch the minimal SDL3 frontend")
    actions = frontend.add_subparsers(dest="frontend_command", required=True)
    run = actions.add_parser("run", help="validate/create the Classic pack and run the desktop app",
                             description="Validate an existing Classic pack, or exact-gate --rom and create it atomically before launch. "
                                         "Without --pack, the newest pack under local/ that carries the supported profile is used. "
                                         "Audio is intentionally not implemented in M3.")
    run.add_argument("--pack", default=None,
                     help="one pack to validate and launch; omit it to select a pack by profile under local/")
    run.add_argument("--track", type=_track_choice, default="dragster",
                     help="dragster, zoom-zoo, or the number of a race track with a recovered scenario (TRACK-BREADTH)")
    run.add_argument("--rom", help="supported PAL ROM for first launch only; omission means selection was cancelled")
    run.add_argument("--replace-pack", action="store_true",
                     help="with --rom, move an incompatible existing pack aside and extract a new one in its place")
    run.add_argument("--preset", default="app-debug")
    run.add_argument("--executable", help=argparse.SUPPRESS)
    run.add_argument("--rules", default=str(ROOT / packmod.TWO_TRACK_RULES_PATH), help=argparse.SUPPRESS)
    run.add_argument("--updates", type=int, help="exit after this many updates (smoke-test aid)")
    run.add_argument("--fixed-controller-mask", type=int, choices=range(0, 65536), help=argparse.SUPPRESS)
    run.add_argument("--hidden", action="store_true", help="create a hidden window (smoke-test aid)")
    run.add_argument("--timeout", type=float, default=86400)
    run.add_argument("--report")
    run.add_argument("--task")
    run.set_defaults(func=cmd_run)
