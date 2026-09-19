#!/usr/bin/env python3
"""Stable repository command entry point.

Usage: python3 tools/project.py <subcommand> [options]

Exit codes: 0 success, 1 check failure, 2 missing prerequisite,
3 invalid input, 4 timeout. Every subcommand accepts --report <path>
to write a JSON report; see tools/unirally_lab/report.py for the schema.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from unirally_lab import (  # noqa: E402
    EXIT_FAILURE,
    EXIT_INVALID_INPUT,
    EXIT_MISSING_PREREQUISITE,
    EXIT_OK,
    __version__,
    lab_commands,
    report as reportmod,
    rom as rommod,
)
from unirally_lab.access import commands as access_commands  # noqa: E402
from unirally_lab.content import commands as content_commands  # noqa: E402
from unirally_lab.coverage import commands as coverage_commands  # noqa: E402
from unirally_lab.reference import commands as reference_commands  # noqa: E402
from unirally_lab.replay import commands as replay_commands  # noqa: E402
from unirally_lab.native import commands as native_commands  # noqa: E402
from unirally_lab.frontend import commands as frontend_commands  # noqa: E402
from unirally_lab import judge_commands  # noqa: E402

ROOT = reportmod.repo_root()
DEFAULT_ROM_LOCATION = ROOT / "local" / "rom-location.txt"


def _default_rom_path() -> Path | None:
    if DEFAULT_ROM_LOCATION.is_file():
        text = DEFAULT_ROM_LOCATION.read_text(encoding="utf-8").strip()
        if text:
            return Path(text).expanduser()
    return None


def _same_file(a: Path | None, b: str | None) -> bool:
    if a is None or not b:
        return False
    try:
        return a.resolve() == Path(b).expanduser().resolve()
    except OSError:
        return False


def cmd_rom_inspect(args: argparse.Namespace) -> int:
    rep = reportmod.Report(sys.argv, task_id=args.task)
    if args.path is not None and not args.path.strip():
        print("--path must not be empty; omit it to use local/rom-location.txt", file=sys.stderr)
        return EXIT_INVALID_INPUT
    path = Path(args.path).expanduser() if args.path is not None else _default_rom_path()
    status = EXIT_OK
    manifest = None

    for label, out in (("--manifest-out", args.manifest_out), ("--report", args.report)):
        if _same_file(path, out):
            print(f"refusing to write {label} onto the input file {path}", file=sys.stderr)
            return EXIT_INVALID_INPUT

    if path is None:
        rep.add_check(
            "rom_available", "missing",
            detail="no --path and no local/rom-location.txt; supply the ROM path",
        )
        status = EXIT_MISSING_PREREQUISITE
    else:
        try:
            manifest = rommod.inspect_rom(path)
            rep.add_input("rom", path, manifest["file"]["sha256"], size=manifest["file"]["size"])
            rep.add_check("rom_available", "passed", detail=str(path))
        except rommod.RomMissingError as exc:
            rep.add_check("rom_available", "missing", detail=str(exc))
            status = EXIT_MISSING_PREREQUISITE
        except rommod.RomError as exc:
            rep.add_check("rom_available", "failed", detail=str(exc))
            status = EXIT_INVALID_INPUT

    if manifest is not None:
        hdr = manifest["header"]
        rep.add_check(
            "header_complement", "passed" if hdr["complement_matches"] else "failed",
            detail=f"{manifest['header_location']} @0x{hdr['offset']:X}",
        )
        chk = manifest["checksum"]
        rep.add_check(
            "internal_checksum", "passed" if chk["matches"] else "failed",
            detail=f"header 0x{chk['header']:04x} computed 0x{chk['computed']:04x}",
        )
        if not (hdr["complement_matches"] and chk["matches"]):
            status = EXIT_FAILURE

        if args.expect:
            try:
                expected = rommod.load_manifest(Path(args.expect))
            except rommod.RomMissingError as exc:
                rep.add_check("expected_manifest", "missing", detail=str(exc))
                status = EXIT_MISSING_PREREQUISITE
            except rommod.RomError as exc:
                rep.add_check("expected_manifest", "failed", detail=str(exc))
                status = EXIT_INVALID_INPUT
            else:
                fields = rommod.compare_identity(manifest, expected)
                mismatches = [f for f in fields if not f["matches"]]
                rep.add_check(
                    "identity_matches_expected", "passed" if not mismatches else "failed",
                    detail=f"{len(fields) - len(mismatches)}/{len(fields)} identity fields match",
                    fields=fields,
                )
                if mismatches:
                    status = EXIT_FAILURE
                    for f in mismatches:
                        print(f"mismatch {f['field']}: expected {f['expected']!r}, observed {f['observed']!r}", file=sys.stderr)

        if args.manifest_out:
            out = Path(args.manifest_out)
            rommod.write_manifest(manifest, out)
            rep.add_artifact("rom_manifest", out)
        if args.print:
            import json
            print(json.dumps(manifest, indent=2, sort_keys=True))

    rep.finish("passed" if status == EXIT_OK else "failed")
    rep.write(Path(args.report) if args.report else None)
    reportmod.print_summary(rep)
    return status


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="project.py", description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--version", action="version", version=f"unirally_lab {__version__}")
    sub = parser.add_subparsers(dest="command", required=True)

    rom = sub.add_parser("rom", help="original ROM identification")
    romsub = rom.add_subparsers(dest="rom_command", required=True)
    insp = romsub.add_parser("inspect", help="hash and identify a ROM image without modifying it")
    insp.add_argument("--path", help="ROM file; defaults to the path in local/rom-location.txt")
    insp.add_argument("--expect", help="tracked manifest to compare identity fields against")
    insp.add_argument("--manifest-out", help="write the observed manifest JSON here")
    insp.add_argument("--print", action="store_true", help="print the manifest to stdout")
    insp.add_argument("--report", help="write the JSON run report here")
    insp.add_argument("--task", help="task ID to record in the report")
    insp.set_defaults(func=cmd_rom_inspect)

    lab_commands.register(sub)
    reference_commands.register(sub)
    replay_commands.register(sub)
    coverage_commands.register(sub)
    access_commands.register(sub)
    content_commands.register(sub)
    native_commands.register(sub)
    frontend_commands.register(sub)
    judge_commands.register(sub)
    return parser


def _normalize_frontend_timeout(argv: list[str]) -> list[str]:
    """Keep argparse from treating a separated negative infinity as an option."""
    if argv[:2] != ["frontend", "run"]:
        return argv
    normalized = argv.copy()
    for index in range(2, len(normalized) - 1):
        if (normalized[index] == "--timeout" and
                normalized[index + 1].casefold() in {"-inf", "-infinity"}):
            normalized[index:index + 2] = [
                f"--timeout={normalized[index + 1]}"
            ]
            break
    return normalized


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    try:
        raw_argv = list(sys.argv[1:] if argv is None else argv)
        args = parser.parse_args(_normalize_frontend_timeout(raw_argv))
    except SystemExit as exc:
        # argparse exits 2 on usage errors; map that to the invalid-input code.
        return EXIT_INVALID_INPUT if exc.code not in (0, None) else EXIT_OK
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
