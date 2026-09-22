"""``coverage`` subcommands (M1-01): capture instruction coverage of a replay
manifest in a fresh worker process, and derive the tracked code map; and
(STATIC-CODE-MAP) the static listing and map of the code banks.

Exit codes follow the repository convention: 0 success, 1 failed check
(digest mismatch, ring overflow, inconsistent totals), 2 missing
prerequisite (ROM, core, coverage file), 3 invalid input, 4 timeout.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any

from .. import EXIT_FAILURE, EXIT_INVALID_INPUT, EXIT_MISSING_PREREQUISITE, EXIT_OK
from .. import report as reportmod
from .. import rom as rommod
from ..reference import commands as refcmd
from ..replay import commands as replaycmd
from ..replay import manifest as mf
from . import derive, drain, static_map

ROOT = reportmod.repo_root()
DEFAULT_RING = 262144
DEFAULT_EXPECT = ROOT / "tests" / "manifests" / "rom" / "unirally-pal.json"


def _finish(rep: reportmod.Report, args: argparse.Namespace, status: int) -> int:
    return refcmd._finish(rep, args, status)


def _write_coverage(path: Path, doc: dict[str, Any]) -> str:
    path.write_text(json.dumps(doc, sort_keys=True, separators=(",", ":")) + "\n", encoding="utf-8")
    return refcmd.sha256_file(path)


# ------------------------------------------------------------- capture


def cmd_capture(args: argparse.Namespace) -> int:
    rep = reportmod.Report(sys.argv, task_id=args.task)
    root = Path(args.root).resolve()
    if args.timeout <= 0 or args.ring <= 0:
        rep.add_check("arguments", "failed", detail="--timeout and --ring must be positive")
        return _finish(rep, args, EXIT_INVALID_INPUT)
    manifest = replaycmd._load_manifest(rep, Path(args.manifest), "replay")
    if manifest is None:
        return _finish(rep, args, EXIT_INVALID_INPUT)
    status = replaycmd._lock_checks(rep, root, manifest, "replay")
    if status != EXIT_OK:
        return _finish(rep, args, status)
    p = replaycmd._prepare(rep, args, manifest)
    if p.status != EXIT_OK:
        return _finish(rep, args, p.status)
    out_dir = Path(args.out)
    out_dir.mkdir(parents=True, exist_ok=True)
    side = replaycmd._resolve_side(rep, root, out_dir, replaycmd.Side("capture", manifest))
    if side.status != EXIT_OK:
        return _finish(rep, args, side.status)

    samples_out = out_dir / "samples.json"
    coverage_out = out_dir / "coverage.json"
    cmd = refcmd._worker_command(p, side.script, samples_out, state_in=side.state_in) + ["--fields", str(side.fields)]
    cmd += ["--coverage-out", str(coverage_out), "--coverage-ring", str(args.ring)]
    vectors: dict[str, int] = {}
    if args.expect:
        try:
            vectors = dict(rommod.load_manifest(Path(args.expect))["header"]["vectors"])
        except (rommod.RomError, KeyError, TypeError):
            vectors = {}
    for name in ("native_nmi", "native_irq", "emu_nmi", "emu_irq_brk"):
        value = vectors.get(name)
        if isinstance(value, int) and value not in (0x0000, 0xFFFF):
            cmd += ["--coverage-watch", f"0x{value:06X}"]  # vector targets are bank $00 addresses
    for frame in args.frame_image or []:
        cmd += ["--frame-image", str(frame)]
    if args.frame_image:
        cmd += ["--frame-image-dir", str(out_dir / "frames")]
    samples, status = refcmd._run_worker(rep, "capture_run", p, cmd, samples_out, args.timeout, out_dir)
    if samples is None:
        if coverage_out.is_file():
            try:
                failed = json.loads(coverage_out.read_text(encoding="utf-8"))
                rep.add_check("ring_not_overflowed", "failed", detail=str(failed.get("failure")))
            except ValueError:
                pass
        return _finish(rep, args, status)
    refcmd._check_identity_and_region(rep, p, samples, argparse.Namespace(expect_region=args.expect_region))
    replaycmd._check_expected(rep, "capture", manifest, samples)

    try:
        cov = drain.validate_document(json.loads(coverage_out.read_text(encoding="utf-8")))
    except (OSError, ValueError) as exc:
        rep.add_check("coverage_written", "failed", detail=f"{coverage_out}: {exc}")
        return _finish(rep, args, EXIT_FAILURE)
    # Identity the worker cannot know: scenario, manifest and the pinned core commit/patch.
    cov["scenario_id"] = manifest["scenario_id"]
    cov["manifest"] = {"path": str(Path(args.manifest)), "sha256": refcmd.sha256_file(Path(args.manifest))}
    cov["core"].update({"name": manifest["core"]["name"], "commit": p.core["commit"], "patch_sha256": p.core["patch_sha256"]})
    sha = _write_coverage(coverage_out, cov)
    rep.add_artifact("coverage", coverage_out)
    rep.add_check("coverage_written", "passed", detail=f"{coverage_out} sha256 {sha[:16]}; {len(cov['sites'])} sites, {len(cov['pairs'])} pairs")

    ins = cov["instructions"]
    ring_ok = cov.get("status") == "complete" and ins["max_frame_delta"] <= cov["ring_capacity"]
    rep.add_check("ring_not_overflowed", "passed" if ring_ok else "failed",
                  detail=f"largest frame delta {ins['max_frame_delta']} of ring {cov['ring_capacity']}; status {cov.get('status')}")
    traced = samples.get("trace", {}).get("instructions_executed")
    totals_ok = traced is not None and traced == ins["total"] == sum(ins["per_frame"]) + 0 and ins["initial_total"] == 0
    rep.add_check("instruction_total_consistent", "passed" if totals_ok else "failed",
                  detail=f"samples trace {traced}; coverage total {ins['total']} = sum of {len(ins['per_frame'])} per-frame deltas "
                         f"{sum(ins['per_frame'])}; ring total before frame {cov['frames']['start']}: {ins['initial_total']}; "
                         f"after final serialize {samples.get('trace', {}).get('instructions_executed_after_final_serialize')}")
    window = samples.get("trace", {}).get("window", [])
    tail = cov.get("tail_sites", [])
    window_sites = [[w["pc"], drain.mode_from_flags(w["p"], w["e"]), w["b"]] for w in window]
    suffix_ok = bool(window) and window_sites == tail[-len(window_sites):]
    rep.add_check("trace_window_is_suffix_of_capture", "passed" if suffix_ok else "failed",
                  detail=f"{len(window)}-entry samples window vs the capture's newest {len(tail)} sites")
    reset = vectors.get("emu_reset")
    first = cov.get("first_site")
    if reset is not None and side.state_in is None:
        ok = first is not None and first[0] == reset and first[1] == 7  # bank $00, emulation mode with M=X=1
        rep.add_check("first_instruction_is_reset_vector", "passed" if ok else "failed",
                      detail=f"first site {derive.addr(first[0]) if first else None} mode {derive.mode_name(first[1]) if first else None}; "
                             f"header emulation reset vector ${reset:04X}")
    else:
        rep.add_check("first_instruction_is_reset_vector", "skipped", required=False,
                      detail="not a cold start or no ROM manifest to read the vector from")
    rep.data["coverage"] = {"path": str(coverage_out), "sha256": sha, "scenario_id": manifest["scenario_id"], "frames": cov["frames"],
                            "instructions": ins["total"], "max_frame_delta": ins["max_frame_delta"], "ring_capacity": cov["ring_capacity"],
                            "sites": len(cov["sites"]), "pairs": len(cov["pairs"]), "frame_images": samples.get("frame_images", [])}
    rep.data["samples"] = replaycmd._samples_summary(samples)
    return _finish(rep, args, replaycmd._status_from_checks(rep))


# ----------------------------------------------------------------- map


def regeneration_command(scenario_id: str) -> str:
    return (f"python3 tools/project.py coverage capture --manifest tests/manifests/replay/{scenario_id}.json --out artifacts/coverage/{scenario_id} "
            f"&& python3 tools/project.py coverage map --coverage artifacts/coverage/{scenario_id}/coverage.json "
            f"--out docs/map/{scenario_id}.map.json --summary docs/map/{scenario_id}.md")


def cmd_map(args: argparse.Namespace) -> int:
    rep = reportmod.Report(sys.argv, task_id=args.task)
    coverage_path = Path(args.coverage)
    if not coverage_path.is_file():
        rep.add_check("coverage_available", "missing", detail=f"{coverage_path} not found; run `coverage capture` first")
        return _finish(rep, args, EXIT_MISSING_PREREQUISITE)
    try:
        cov = drain.validate_document(json.loads(coverage_path.read_text(encoding="utf-8")))
    except (OSError, ValueError) as exc:
        rep.add_check("coverage_available", "failed", detail=f"{coverage_path}: {exc}")
        return _finish(rep, args, EXIT_INVALID_INPUT)
    if cov.get("status") != "complete":
        rep.add_check("coverage_available", "failed", detail=f"coverage status {cov.get('status')!r}: {cov.get('failure')}")
        return _finish(rep, args, EXIT_INVALID_INPUT)
    coverage_sha = refcmd.sha256_file(coverage_path)
    scenario = args.scenario or cov.get("scenario_id")
    if not scenario:
        rep.add_check("coverage_available", "failed", detail="coverage lacks a scenario_id; pass --scenario")
        return _finish(rep, args, EXIT_INVALID_INPUT)
    rep.add_input("coverage", coverage_path, coverage_sha, scenario_id=scenario)
    rep.add_check("coverage_available", "passed", detail=f"{coverage_path} ({scenario}, {len(cov['sites'])} sites)")

    rom = Path(args.rom).expanduser() if args.rom else refcmd._default_rom_path()
    if rom is None or not rom.is_file():
        rep.add_check("rom_available", "missing", detail=f"{rom or 'no --rom and no local/rom-location.txt'}")
        return _finish(rep, args, EXIT_MISSING_PREREQUISITE)
    try:
        observed = rommod.inspect_rom(rom)
    except rommod.RomMissingError as exc:
        rep.add_check("rom_available", "missing", detail=str(exc))
        return _finish(rep, args, EXIT_MISSING_PREREQUISITE)
    except rommod.RomError as exc:
        rep.add_check("rom_available", "failed", detail=str(exc))
        return _finish(rep, args, EXIT_INVALID_INPUT)
    rep.add_input("rom", rom, observed["file"]["sha256"], size=observed["file"]["size"])
    rep.add_check("rom_available", "passed", detail=str(rom))
    same = observed["file"]["sha256"] == cov["rom"]["sha256"] and not observed["copier_header"]["present"]
    rep.add_check("rom_matches_coverage", "passed" if same else "failed",
                  detail=f"ROM {observed['file']['sha256'][:16]}…, coverage {cov['rom']['sha256'][:16]}…; copier header {observed['copier_header']['present']}")
    if not same:
        return _finish(rep, args, EXIT_FAILURE)
    if observed["header_location"] != "lorom":
        rep.add_check("mapping_rule_applies", "failed", detail=f"header location {observed['header_location']}; only LoROM is implemented")
        return _finish(rep, args, EXIT_FAILURE)
    rep.add_check("mapping_rule_applies", "passed", detail=f"LoROM header at 0x{observed['header']['offset']:X}; {derive.MAPPING_RULE}")

    rom_bytes = rom.read_bytes()
    core = {k: cov["core"].get(k) for k in ("name", "commit", "patch_sha256", "sha256", "serialization_method")}
    core["library_sha256"] = core.pop("sha256")
    doc, detail = derive.build_map(cov, rom_bytes, scenario, core, coverage_sha, regeneration_command(scenario), observed["header"]["offset"])

    baseline = None
    if args.baseline:
        bpath = Path(args.baseline)
        if not bpath.is_file():
            rep.add_check("baseline_available", "missing", detail=f"{bpath} not found")
            return _finish(rep, args, EXIT_MISSING_PREREQUISITE)
        try:
            baseline = json.loads(bpath.read_text(encoding="utf-8"))
            if baseline.get("kind") != "code_map" or baseline.get("rom", {}).get("sha256") != cov["rom"]["sha256"]:
                raise ValueError("not a code map of the same ROM")
        except (OSError, ValueError) as exc:
            rep.add_check("baseline_available", "failed", detail=f"{bpath}: {exc}")
            return _finish(rep, args, EXIT_INVALID_INPUT)
        rep.add_input("baseline_map", bpath, refcmd.sha256_file(bpath), scenario_id=baseline.get("scenario_id"))
        rep.add_check("baseline_available", "passed", detail=f"{bpath} ({baseline.get('scenario_id')})")
        doc["compared_to"] = derive.compare_maps(doc, baseline)

    t = doc["totals"]
    total_ok = t["executed_opcode_bytes"] + t["executed_operand_bytes"] + t["unclassified_bytes"] == t["rom_size"] == len(rom_bytes)
    rep.add_check("byte_classes_sum_to_rom_size", "passed" if total_ok else "failed",
                  detail=f"{t['executed_opcode_bytes']} + {t['executed_operand_bytes']} + {t['unclassified_bytes']} = {t['rom_size']}")
    reset = next(v for v in doc["vectors"] if v["name"] == "emu_reset")
    first = doc["coverage"]["first_site"]
    cold = cov["frames"]["start"] == 0 and cov["instructions"]["initial_total"] == 0
    if cold:
        ok = first is not None and first["address"] == reset["target"] and reset["executed"]
        rep.add_check("reset_vector_is_first_instruction", "passed" if ok else "failed",
                      detail=f"first executed {first}; emulation reset vector {reset['target']} executed {reset['count']} time(s)")
    else:
        rep.add_check("reset_vector_is_first_instruction", "skipped", required=False, detail="capture did not start at frame 0")
    nmi = next(v for v in doc["vectors"] if v["name"] == "native_nmi")
    pf = nmi.get("per_frame")
    if pf is None:
        rep.add_check("nmi_vector_once_per_frame", "skipped", required=False, detail="the capture did not watch the NMI vector target per frame")
    else:
        nmi_ok = nmi["executed"] and pf["once_per_frame_from"] is not None
        rep.add_check("nmi_vector_once_per_frame", "passed" if nmi_ok else "failed", required=False,
                      detail=f"native NMI target {nmi['target']}: {nmi['count']} executions over frames {doc['coverage']['frames']['start']}-{doc['coverage']['frames']['end']}; "
                             f"frames with 0/1/2+ entries {pf['frames_with_zero']}/{pf['frames_with_one']}/{pf['frames_with_more']}; first frame with one {pf['first_frame']}; "
                             f"exactly one per frame from frame {pf['once_per_frame_from']} to the end; frames without an NMI after that first frame: {pf['gaps_after_first']}")
    unknown = t["unknown_edges"]
    rep.add_check("edges_classified", "passed", required=False,
                  detail=f"{t['edge_steps']} non-sequential steps, {t['sequential_steps']} sequential; {unknown} unknown edge(s) "
                         f"(from sites outside ROM: {sum(1 for e in doc['unknown_edges'] if e['from_region'] != 'rom')})")

    out = Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(derive.dump_map(doc), encoding="utf-8")
    rep.add_artifact("map", out)
    if args.summary:
        summary = Path(args.summary)
        summary.parent.mkdir(parents=True, exist_ok=True)
        summary.write_text(derive.summary_markdown(doc, doc.get("compared_to")), encoding="utf-8")
        rep.add_artifact("summary", summary)
    if args.detail:
        detail_path = Path(args.detail)
        detail_path.parent.mkdir(parents=True, exist_ok=True)
        detail_path.write_text(json.dumps({"scenario_id": scenario, "coverage_sha256": coverage_sha, "rom_sha256": cov["rom"]["sha256"],
                                           "note": "ignored artifact: carries opcode bytes; regenerate with the map's regeneration_command plus --detail",
                                           "addresses": detail}, indent=None, separators=(",", ":")) + "\n", encoding="utf-8")
        rep.add_artifact("detail", detail_path)
    size_ok = out.stat().st_size <= (1 << 20)
    rep.add_check("map_written", "passed" if size_ok else "failed",
                  detail=f"{out} ({out.stat().st_size} bytes, limit 1 MiB); {len(doc['ranges'])} ranges, {len(doc['entry_points'])} entry points, "
                         f"{len(doc['static_references'])} static references")
    rep.data["totals"] = t
    rep.data["vectors"] = doc["vectors"]
    if baseline is not None:
        c = doc["compared_to"]
        rep.data["compared_to"] = {k: c[k] for k in c if k not in ("new_ranges", "new_entry_points")}
    return _finish(rep, args, replaycmd._status_from_checks(rep))


# ---------------------------------------------------------- static map


def _static_analysis(rep: reportmod.Report, args: argparse.Namespace):
    """Load the ROM, the tracked maps and any raw coverage files, and run the static analysis."""
    map_paths = sorted(Path(args.maps).glob("*.map.json"))
    if not map_paths:
        rep.add_check("maps_available", "missing", detail=f"no *.map.json under {args.maps}")
        return EXIT_MISSING_PREREQUISITE, None
    maps = static_map.load_maps(map_paths)
    rep.add_check("maps_available", "passed", detail=", ".join(m["scenario_id"] for m in maps))
    rom_path = Path(args.rom).expanduser() if args.rom else refcmd._default_rom_path()
    if rom_path is None or not rom_path.is_file():
        rep.add_check("rom_available", "missing", detail=f"{rom_path or 'no --rom and no local/rom-location.txt'}")
        return EXIT_MISSING_PREREQUISITE, None
    try:
        observed = rommod.inspect_rom(rom_path)
    except rommod.RomError as exc:
        rep.add_check("rom_available", "failed", detail=str(exc))
        return EXIT_INVALID_INPUT, None
    rep.add_input("rom", rom_path, observed["file"]["sha256"], size=observed["file"]["size"])
    want = {m["rom"]["sha256"] for m in maps}
    if want != {observed["file"]["sha256"]} or observed["copier_header"]["present"]:
        rep.add_check("rom_matches_maps", "failed", detail=f"ROM {observed['file']['sha256']} (header {observed['copier_header']['present']}); maps want {sorted(want)}")
        return EXIT_INVALID_INPUT, None
    rep.add_check("rom_matches_maps", "passed", detail=observed["file"]["sha256"])
    rom = rom_path.read_bytes()
    analysis = static_map.Analysis(rom, maps)
    digests = []
    if args.coverage:
        wanted = {m["coverage"]["sha256"]: m["scenario_id"] for m in maps}
        coverages = {}
        for path in map(Path, args.coverage):
            if not path.is_file():
                rep.add_check("coverage_available", "missing", detail=f"{path} not found")
                return EXIT_MISSING_PREREQUISITE, None
            sha = refcmd.sha256_file(path)
            if sha not in wanted:
                rep.add_check("coverage_available", "failed", detail=f"{path} ({sha}) is not the coverage of any tracked map")
                return EXIT_INVALID_INPUT, None
            rep.add_input("coverage", path, sha, scenario_id=wanted[sha])
            coverages[sha] = json.loads(path.read_text(encoding="utf-8"))
        missing = sorted(wanted[s] for s in set(wanted) - set(coverages))
        if missing:
            rep.add_check("coverage_available", "failed", detail=f"no raw coverage for {', '.join(missing)}; pass one for every tracked map or none")
            return EXIT_INVALID_INPUT, None
        rep.add_check("coverage_available", "passed", detail=f"{len(coverages)} raw coverage file(s), one per tracked map")
        analysis.load_sites([coverages[s] for s in sorted(coverages)])
        digests = sorted(coverages)
    else:
        rep.add_check("coverage_available", "skipped", required=False,
                      detail="no --coverage: observed boundaries come from range tiling alone, and the per-site agreement check is not run")
    analysis.run()
    cls = analysis.classes()
    return EXIT_OK, (analysis, cls, map_paths, digests, [coverages[s] for s in digests] if digests else [])


def _agreement_checks(rep: reportmod.Report, analysis: static_map.Analysis, coverages: list[dict[str, Any]]) -> dict[str, Any]:
    tiled = analysis.tiling["maps"]
    no_tiling = sum(m["no_tiling"] for m in tiled)
    against = sum(m["disagreements"] for m in tiled)
    per_site = [static_map.check_sites(analysis, c) for c in coverages]
    checked = sum(p["sites_checked"] for p in per_site)
    bad = sum(p["disagreements"] for p in per_site)
    rep.add_check("observed_ranges_tile", "passed" if no_tiling == 0 and against == 0 else "failed",
                  detail=f"{sum(m['ranges'] for m in tiled)} ranges; {no_tiling} without a tiling; "
                         f"{sum(m['shared_instructions_checked_against_sites'] for m in tiled)} shared instructions checked against sites, {against} disagreements")
    if coverages:
        rep.add_check("every_site_decodes", "passed" if bad == 0 else "failed",
                      detail=f"{checked} sites over {len(per_site)} captures decode at the same address and length under their mode; {bad} disagreements")
    else:
        rep.add_check("every_site_decodes", "skipped", required=False, detail="needs --coverage")
    return {"tiling": analysis.tiling, "sites": [{"scenario_id": c.get("scenario_id"), **p} for c, p in zip(coverages, per_site)],
            "sites_checked": checked, "disagreements": bad + against + no_tiling}


def cmd_disassemble(args: argparse.Namespace) -> int:
    rep = reportmod.Report(sys.argv, task_id=args.task)
    status, result = _static_analysis(rep, args)
    if status != EXIT_OK:
        return _finish(rep, args, status)
    analysis, cls, map_paths, digests, coverages = result
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    agreement = _agreement_checks(rep, analysis, coverages)
    (out / "agreement.json").write_text(static_map.dump(agreement), encoding="utf-8")
    rep.add_artifact("agreement", out / "agreement.json")
    doc, labs = static_map.build(analysis, cls, ROOT, map_paths, digests)
    names_by_offset = {static_map.offset_of(static_map.parse(l["address"])): l["label"] for l in labs}
    for r in doc["routines"]:
        names_by_offset.setdefault(static_map.offset_of(static_map.parse(r["start"])), "sub_" + r["start"][1:].replace(":", ""))
    xrefs = {}
    for target, callers in analysis.callers.items():
        xrefs[target] = [f"{static_map.addr(o)} {k}" for o, k in sorted(callers)]
    for b in range(4):
        path = out / f"bank-{0x80 + b:02X}.lst"
        path.write_text(static_map.listing(analysis, cls, b, names_by_offset, xrefs), encoding="utf-8")
        rep.add_artifact(f"listing_{0x80 + b:02X}", path)
    detail = {"note": "ignored artifact: carries ROM bytes and mnemonics", "map": doc,
              "instructions": [{"address": static_map.addr(o), "length": i.length, "modes": static_map.names(i.modes), "source": i.source,
                                "mnemonic": static_map.TABLE[analysis.byte(o)][0], "operand": static_map.operand_text(analysis, o, i.length)}
                               for o, i in sorted(analysis.ins.items())],
              "data": [{"address": static_map.addr(o), "reasons": sorted(r)} for o, r in sorted(analysis.data.items())]}
    (out / "static-map.json").write_text(static_map.dump(detail), encoding="utf-8")
    rep.add_artifact("detail", out / "static-map.json")
    _partition_check(rep, cls, doc)
    return _finish(rep, args, replaycmd._status_from_checks(rep))


def _partition_check(rep: reportmod.Report, cls: list[str], doc: dict[str, Any]) -> None:
    t = doc["totals"]
    ok = len(cls) == static_map.SPAN and sum(t[k] for k in static_map.CLASSES) == static_map.SPAN
    rep.add_check("classes_partition_the_banks", "passed" if ok else "failed",
                  detail=", ".join(f"{k} {t[k]}" for k in static_map.CLASSES) + f"; unknown share {t['unknown_share']:.1%}")
    rep.data["totals"] = t


def cmd_static_map(args: argparse.Namespace) -> int:
    rep = reportmod.Report(sys.argv, task_id=args.task)
    status, result = _static_analysis(rep, args)
    if status != EXIT_OK:
        return _finish(rep, args, status)
    analysis, cls, map_paths, digests, coverages = result
    _agreement_checks(rep, analysis, coverages)
    rel = [p.resolve().relative_to(ROOT) if p.resolve().is_relative_to(ROOT) else p for p in map_paths]
    doc, labs = static_map.build(analysis, cls, ROOT, rel, digests)
    _partition_check(rep, cls, doc)
    for path, text in ((Path(args.out), static_map.dump(doc)), (Path(args.labels), static_map.dump(labs)),
                       (Path(args.summary), static_map.summary_markdown(doc, labs))):
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")
        rep.add_artifact(path.name, path)
    size_ok = Path(args.out).stat().st_size <= (1 << 20)
    rep.add_check("map_written", "passed" if size_ok else "failed", detail=f"{args.out} ({Path(args.out).stat().st_size} bytes, limit 1 MiB)")
    return _finish(rep, args, replaycmd._status_from_checks(rep))


# --------------------------------------------------------------- parser


def register(sub: argparse._SubParsersAction) -> None:
    cov = sub.add_parser("coverage", help="instruction coverage capture and the observed code map (M1-01)")
    csub = cov.add_subparsers(dest="coverage_command", required=True)

    capture = csub.add_parser("capture", help="run a replay manifest in a fresh worker with the per-frame trace drain")
    capture.add_argument("--manifest", required=True, help="replay manifest JSON (see tests/manifests/replay/)")
    capture.add_argument("--out", required=True, help="directory for coverage.json, samples, scripts, logs and frame images")
    capture.add_argument("--ring", type=int, default=DEFAULT_RING, help=f"trace ring capacity (default {DEFAULT_RING}); a frame executing more instructions fails the run")
    capture.add_argument("--frame-image", type=int, action="append", help="write this frame's video output as PNG under <out>/frames (repeatable)")
    capture.add_argument("--rom", help="ROM file; defaults to the path in local/rom-location.txt")
    capture.add_argument("--expect", default=str(DEFAULT_EXPECT), help="ROM identity manifest whose reset vector the first instruction must match; '' disables")
    capture.add_argument("--expect-region", default="PAL", help="region the core must report (default PAL)")
    capture.add_argument("--root", default=str(ROOT), help=argparse.SUPPRESS)
    capture.add_argument("--timeout", type=float, default=900, help="seconds for the worker process (default 900)")
    capture.add_argument("--report", help="write the JSON run report here")
    capture.add_argument("--task", help="task ID to record in the report")
    capture.set_defaults(func=cmd_capture)

    mp = csub.add_parser("map", help="derive the tracked code map from a coverage file and the ROM")
    mp.add_argument("--coverage", required=True, help="coverage.json written by `coverage capture`")
    mp.add_argument("--out", required=True, help="tracked map JSON to write (docs/map/<scenario>.map.json)")
    mp.add_argument("--summary", help="Markdown summary to write (docs/map/<scenario>.md)")
    mp.add_argument("--detail", help="per-address detail JSON (ignored artifact; carries opcode bytes)")
    mp.add_argument("--baseline", help="another scenario's map: list what this scenario executes that it does not")
    mp.add_argument("--scenario", help="scenario id (default: the coverage file's)")
    mp.add_argument("--rom", help="ROM file; defaults to the path in local/rom-location.txt")
    mp.add_argument("--report", help="write the JSON run report here")
    mp.add_argument("--task", help="task ID to record in the report")
    mp.set_defaults(func=cmd_map)

    maps_default = str(ROOT / "docs" / "map")
    dis = csub.add_parser("disassemble", help="static listing of banks $80-$83 from the ROM and the tracked maps (ignored output)")
    dis.add_argument("--out", required=True, help="directory for bank-80.lst..bank-83.lst, static-map.json and agreement.json")
    static = csub.add_parser("static-map", help="derive the tracked static code map, summary and labels of banks $80-$83")
    static.add_argument("--out", required=True, help="tracked map JSON (docs/map/static/code-banks.map.json)")
    static.add_argument("--summary", required=True, help="Markdown summary (docs/map/static/code-banks.md)")
    static.add_argument("--labels", required=True, help="labels JSON (docs/map/static/labels.json)")
    for p in (dis, static):
        p.add_argument("--maps", default=maps_default, help="directory of tracked *.map.json (default docs/map)")
        p.add_argument("--coverage", action="append", help="raw coverage.json behind a tracked map (repeatable; one per map or none)")
        p.add_argument("--rom", help="ROM file; defaults to the path in local/rom-location.txt")
        p.add_argument("--report", help="write the JSON run report here")
        p.add_argument("--task", help="task ID to record in the report")
    dis.set_defaults(func=cmd_disassemble)
    static.set_defaults(func=cmd_static_map)
