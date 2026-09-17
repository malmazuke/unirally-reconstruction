"""Validated-pack preparation and bounded SDL frontend launch."""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import sys
from dataclasses import dataclass
from pathlib import Path

from .. import EXIT_FAILURE, EXIT_INVALID_INPUT, EXIT_MISSING_PREREQUISITE, EXIT_OK, EXIT_TIMEOUT
from .. import report as reportmod
from ..content import pack as packmod
from ..procs import run_bounded

ROOT = reportmod.repo_root()


@dataclass(frozen=True)
class FrontendPaths:
    pack: Path
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
    if getattr(args, "track", "dragster") == "zoom-zoo":
        if args.rules == str(ROOT / packmod.RULES_PATH):args.rules=str(ROOT / packmod.TWO_TRACK_RULES_PATH)
        if args.pack == str(ROOT / "local" / "classic-crawler-dragster.pack"):
            args.pack=str(ROOT / "local" / "classic-crawler-two-tracks.pack")
    executable = (Path(args.executable) if args.executable else
                  _default_executable(args.preset))
    rom = (None if args.rom is None or not args.rom.strip()
           else _canonical_path(args.rom))
    return FrontendPaths(
        pack=_canonical_path(args.pack),
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
        ("--pack", paths.pack),
        ("--rules", paths.rules),
        ("frontend executable", paths.executable),
    ]
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
    pack_path = paths.pack
    rules_path = paths.rules
    executable = paths.executable
    try:
        rules, rules_sha = packmod.load_rules(rules_path)
        rep.add_input("extraction_rules", rules_path, rules_sha)
    except (FileNotFoundError, ValueError, json.JSONDecodeError, OSError) as exc:
        rep.add_check("extraction_rules", "failed", detail=str(exc))
        return _finish(rep, paths.report, EXIT_INVALID_INPUT)

    if pack_path.exists():
        try:
            inspected = packmod.validate_pack(pack_path.read_bytes(), rules, rules_sha)
        except (ValueError, OSError) as exc:
            rep.add_check("classic_pack", "failed", detail=f"existing pack is invalid and was not replaced: {exc}")
            return _finish(rep, paths.report, EXIT_INVALID_INPUT)
        rep.add_check("classic_pack", "passed", detail=f"validated existing pack {pack_path}; ROM was not opened")
        rep.add_input("classic_pack", pack_path, inspected["pack_sha256"], size=inspected["pack_size"])
        rep.data["first_launch_extraction"] = False
    else:
        if paths.rom is None:
            rep.add_check("supported_rom", "missing", detail="pack is absent; select the supported PAL ROM with --rom PATH")
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
            packmod.write_atomic(pack_path, payload)
            inspected = packmod.validate_pack(pack_path.read_bytes(), rules, rules_sha)
        except ValueError as exc:
            rep.add_check("supported_rom", "failed", detail=str(exc))
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
    command = [str(executable), "--content-pack", str(pack_path)]
    if getattr(args,"track","dragster")=="zoom-zoo":command.extend(["--track","zoom-zoo"])
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
                             description="Validate an existing Classic pack, or exact-gate --rom and create it atomically before launch. Audio is intentionally not implemented in M3.")
    run.add_argument("--pack", default=str(ROOT / "local" / "classic-crawler-dragster.pack"))
    run.add_argument("--track", choices=["dragster","zoom-zoo"], default="dragster")
    run.add_argument("--rom", help="supported PAL ROM for first launch only; omission means selection was cancelled")
    run.add_argument("--preset", default="app-debug")
    run.add_argument("--executable", help=argparse.SUPPRESS)
    run.add_argument("--rules", default=str(ROOT / packmod.RULES_PATH), help=argparse.SUPPRESS)
    run.add_argument("--updates", type=int, help="exit after this many updates (smoke-test aid)")
    run.add_argument("--fixed-controller-mask", type=int, choices=range(0, 65536), help=argparse.SUPPRESS)
    run.add_argument("--hidden", action="store_true", help="create a hidden window (smoke-test aid)")
    run.add_argument("--timeout", type=float, default=86400)
    run.add_argument("--report")
    run.add_argument("--task")
    run.set_defaults(func=cmd_run)
