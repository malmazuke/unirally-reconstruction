"""``access`` subcommands (M1-02): capture the trace-derived memory access
record of a replay manifest in a fresh worker process, and query it.

Exit codes follow the repository convention: 0 success, 1 failed check
(digest mismatch, ring overflow, inconsistent totals, no match for a query),
2 missing prerequisite (ROM, core, access file), 3 invalid input, 4 timeout.
"""

from __future__ import annotations

import argparse
import json
import sys
from collections import Counter
from pathlib import Path
from typing import Any

from .. import EXIT_FAILURE, EXIT_INVALID_INPUT, EXIT_MISSING_PREREQUISITE, EXIT_OK
from .. import report as reportmod
from ..coverage import derive as covderive
from ..reference import commands as refcmd
from ..replay import commands as replaycmd
from . import derive, modes

ROOT = reportmod.repo_root()
DEFAULT_RING = 262144


def _finish(rep: reportmod.Report, args: argparse.Namespace, status: int) -> int:
    return refcmd._finish(rep, args, status)


def parse_address(text: str) -> int:
    """``$7E:0313``, ``$7E0313``, ``0x7E0313`` or decimal."""
    t = text.strip()
    if t.startswith("$"):
        return int(t[1:].replace(":", ""), 16)
    return int(t, 0)


def addr(a: int) -> str:
    return covderive.addr(a)


# ------------------------------------------------------------- capture


def cmd_capture(args: argparse.Namespace) -> int:
    rep = reportmod.Report(sys.argv, task_id=args.task)
    root = Path(args.root).resolve()
    if args.timeout <= 0 or args.ring <= 0:
        rep.add_check("arguments", "failed", detail="--timeout and --ring must be positive")
        return _finish(rep, args, EXIT_INVALID_INPUT)
    if (args.wram_series_range is None) != (args.wram_series_every is None) and args.wram_series_range is None:
        rep.add_check("arguments", "failed", detail="--wram-series-every needs --wram-series-range")
        return _finish(rep, args, EXIT_INVALID_INPUT)
    try:
        watch_addresses = [parse_address(a) for a in args.watch_address or []]
        watch_pcs = [parse_address(a) for a in args.watch_pc or []]
    except ValueError as exc:
        rep.add_check("arguments", "failed", detail=f"unparseable address: {exc}")
        return _finish(rep, args, EXIT_INVALID_INPUT)
    manifest = replaycmd._load_manifest(rep, Path(args.manifest), "replay")
    if manifest is None:
        return _finish(rep, args, EXIT_INVALID_INPUT)
    frames = manifest["run"]["frames"]
    for label, value in (("--from-frame", args.from_frame), ("--to-frame", args.to_frame)):
        if value is not None and not (0 <= value < frames):
            rep.add_check("arguments", "failed", detail=f"{label} {value} lies outside the manifest's {frames} frames")
            return _finish(rep, args, EXIT_INVALID_INPUT)
    if args.from_frame is not None and args.to_frame is not None and args.to_frame < args.from_frame:
        rep.add_check("arguments", "failed", detail="--to-frame precedes --from-frame")
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
    access_out = out_dir / "access.json"
    cmd = refcmd._worker_command(p, side.script, samples_out, state_in=side.state_in) + ["--fields", str(side.fields)]
    cmd += ["--access-out", str(access_out), "--access-ring", str(args.ring)]
    if args.from_frame is not None:
        cmd += ["--access-from-frame", str(args.from_frame)]
    if args.to_frame is not None:
        cmd += ["--access-to-frame", str(args.to_frame)]
    for a in watch_addresses:
        cmd += ["--access-watch-address", f"0x{a:06X}"]
    for a in watch_pcs:
        cmd += ["--access-watch-pc", f"0x{a:06X}"]
    if args.resolve_rmw:
        cmd += ["--access-resolve-rmw"]
    series_out = None
    if args.wram_series_range:
        try:
            ws, wl = (parse_address(v) for v in args.wram_series_range)
        except ValueError as exc:
            rep.add_check("arguments", "failed", detail=f"--wram-series-range: {exc}")
            return _finish(rep, args, EXIT_INVALID_INPUT)
        series_out = out_dir / "wram-series.bin"
        cmd += ["--wram-series-out", str(series_out), "--wram-series-range", str(ws), str(wl),
                "--wram-series-every", str(args.wram_series_every or 1)]
    for frame in args.frame_image or []:
        cmd += ["--frame-image", str(frame)]
    if args.frame_image:
        cmd += ["--frame-image-dir", str(out_dir / "frames")]
    samples, status = refcmd._run_worker(rep, "capture_run", p, cmd, samples_out, args.timeout, out_dir)
    if samples is None:
        if access_out.is_file():
            try:
                failed = json.loads(access_out.read_text(encoding="utf-8"))
                rep.add_check("ring_not_overflowed", "failed", detail=str(failed.get("failure")))
            except ValueError:
                pass
        return _finish(rep, args, status)
    refcmd._check_identity_and_region(rep, p, samples, argparse.Namespace(expect_region=args.expect_region))
    replaycmd._check_expected(rep, "capture", manifest, samples)

    try:
        doc = derive.validate_document(json.loads(access_out.read_text(encoding="utf-8")))
    except (OSError, ValueError) as exc:
        rep.add_check("access_written", "failed", detail=f"{access_out}: {exc}")
        return _finish(rep, args, EXIT_FAILURE)
    doc["scenario_id"] = manifest["scenario_id"]
    doc["manifest"] = {"path": str(Path(args.manifest)), "sha256": refcmd.sha256_file(Path(args.manifest))}
    doc["core"].update({"name": manifest["core"]["name"], "commit": p.core["commit"], "patch_sha256": p.core["patch_sha256"]})
    doc["regeneration_command"] = regeneration_command(manifest["scenario_id"], args, watch_addresses, watch_pcs)
    access_out.write_text(json.dumps(doc, sort_keys=True, separators=(",", ":")) + "\n", encoding="utf-8")
    sha = refcmd.sha256_file(access_out)
    rep.add_artifact("access", access_out)
    if series_out is not None and series_out.is_file():
        rep.add_artifact("wram_series", series_out)
    ins = doc["instructions"]
    rep.add_check("access_written", "passed",
                  detail=f"{access_out} sha256 {sha[:16]}; frames {doc['frames']['start']}-{doc['frames']['end']}; {ins['total']} instructions, "
                         f"{doc['accesses_total']} accesses ({len(doc['accesses'])} distinct work RAM/register keys, {len(doc['rom_reads'])} ROM-read sites, "
                         f"{doc['rom_bytes_read']} ROM bytes read); library sha256 {samples['core']['sha256'][:16]} kept outside the record")
    ring_ok = doc.get("status") == "complete" and ins["max_frame_delta"] <= doc["ring_capacity"]
    rep.add_check("ring_not_overflowed", "passed" if ring_ok else "failed",
                  detail=f"largest frame delta {ins['max_frame_delta']} of ring {doc['ring_capacity']}; status {doc.get('status')}")
    traced = samples.get("trace", {}).get("instructions_executed")
    whole = doc["frames"]["start"] == samples["start_frame"] and doc["frames"]["end"] == samples["end_frame"]
    if whole:
        totals_ok = traced is not None and traced == ins["total"] == sum(ins["per_frame"])
        rep.add_check("instruction_total_consistent", "passed" if totals_ok else "failed",
                      detail=f"samples trace {traced}; access total {ins['total']} = sum of {len(ins['per_frame'])} per-frame deltas {sum(ins['per_frame'])}")
    else:
        totals_ok = traced is not None and ins["total"] == sum(ins["per_frame"]) and ins["total"] <= traced
        rep.add_check("instruction_total_consistent", "passed" if totals_ok else "failed",
                      detail=f"window {doc['frames']['start']}-{doc['frames']['end']} of run {samples['start_frame']}-{samples['end_frame']}: "
                             f"access total {ins['total']} = sum of per-frame deltas {sum(ins['per_frame'])} <= samples trace {traced}")
    res = doc["residual"]
    rep.add_check("residual_reported", "passed", required=False,
                  detail=f"unresolved accesses {res['unresolved_total']} (stores {res['unresolved_stores']}); "
                         f"{len(res['non_rom_pcs'])} pcs outside ROM; {res['dma_triggers']} DMA triggers, {res['hdma_enables']} HDMA enables; "
                         f"resolution conflicts {doc['resolution_conflict_bytes']} bytes, dropped {doc['resolutions_dropped']}")
    rep.data["access"] = {"path": str(access_out), "sha256": sha, "scenario_id": manifest["scenario_id"], "frames": doc["frames"],
                          "instructions": ins["total"], "accesses": doc["accesses_total"], "distinct_keys": len(doc["accesses"]),
                          "rom_read_sites": len(doc["rom_reads"]), "rom_bytes_read": doc["rom_bytes_read"], "residual": {k: v for k, v in res.items() if k != "notes"},
                          "wram_series": doc.get("wram_series"), "frame_images": samples.get("frame_images", [])}
    rep.data["samples"] = replaycmd._samples_summary(samples)
    return _finish(rep, args, replaycmd._status_from_checks(rep))


def regeneration_command(scenario_id: str, args: argparse.Namespace, watch_addresses: list[int], watch_pcs: list[int]) -> str:
    """The capture command in canonical form (no host paths) so two captures of one run are byte-identical."""
    parts = [f"python3 tools/project.py access capture --manifest tests/manifests/replay/{scenario_id}.json --out artifacts/access/{scenario_id}"]
    if args.from_frame is not None:
        parts.append(f"--from-frame {args.from_frame}")
    if args.to_frame is not None:
        parts.append(f"--to-frame {args.to_frame}")
    parts += [f"--watch-address 0x{a:06X}" for a in watch_addresses]
    parts += [f"--watch-pc 0x{a:06X}" for a in watch_pcs]
    if args.wram_series_range:
        ws, wl = (parse_address(v) for v in args.wram_series_range)
        parts.append(f"--wram-series-range 0x{ws:X} 0x{wl:X} --wram-series-every {args.wram_series_every or 1}")
    if args.ring != DEFAULT_RING:
        parts.append(f"--ring {args.ring}")
    return " ".join(parts)


# --------------------------------------------------------------- query


def load_access(path: Path) -> dict[str, Any]:
    return derive.validate_document(json.loads(path.read_text(encoding="utf-8")))


def access_rows(doc: dict[str, Any]) -> list[dict[str, Any]]:
    names = doc["addressing_names"]
    kinds = doc["kind_names"]
    wraps = doc["wrap_names"]
    labels = doc["label_names"]
    rows = []
    for pc, mode, addressing, kind, width, wrap, address, count, first, last, values, truncated, label in doc["accesses"]:
        rows.append({"pc": pc, "mode": covderive.mode_name(mode), "addressing": names[addressing], "kind": kinds[kind], "width": width,
                     "wrap": wraps[wrap], "address": address, "count": count, "first_frame": first, "last_frame": last,
                     "values": values, "values_truncated": truncated, "label": None if label is None else labels[label]})
    return rows


def _covers(row: dict[str, Any], target: int) -> bool:
    """Whether the access's byte span covers ``target`` (work RAM mirrors folded)."""
    wt = derive.wram_offset(target)
    for ba in modes.byte_addresses(row["address"], row["width"], ["linear", "bank", "page"].index(row["wrap"])):
        if ba == target:
            return True
        if wt is not None and derive.wram_offset(ba) == wt:
            return True
    return False


def query(doc: dict[str, Any], address: int | None = None, pc: int | None = None, kind: str | None = None) -> dict[str, Any]:
    rows = access_rows(doc)
    if pc is not None:
        rows = [r for r in rows if r["pc"] == pc]
    if address is not None:
        rows = [r for r in rows if _covers(r, address)]
    if kind is not None:
        rows = [r for r in rows if r["kind"] == kind]
    writers: Counter[int] = Counter()
    readers: Counter[int] = Counter()
    for r in rows:
        (writers if r["kind"] in ("write", "rmw", "push", "block_write") else readers)[r["pc"]] += r["count"]
    rom_rows = []
    if pc is not None:
        names = doc["addressing_names"]
        kinds = doc["kind_names"]
        for rpc, mode, addressing, rkind, count, lo, hi, first, last, label in doc["rom_reads"]:
            if rpc == pc and (kind is None or kinds[rkind] == kind):
                rom_rows.append({"pc": rpc, "mode": covderive.mode_name(mode), "addressing": names[addressing], "kind": kinds[rkind],
                                 "count": count, "min_address": lo, "max_address": hi, "first_frame": first, "last_frame": last})
    return {"rows": rows, "rom_reads": rom_rows,
            "writers": [{"pc": p_, "count": n} for p_, n in sorted(writers.items())],
            "readers": [{"pc": p_, "count": n} for p_, n in sorted(readers.items())]}


def format_rows(result: dict[str, Any]) -> str:
    lines = ["pc         mode addressing kind        width address    count   first  last   label            values"]
    for r in result["rows"]:
        vals = "" if r["values"] is None else " ".join(f"0x{v:X}" for v in r["values"]) + ("…" if r["values_truncated"] else "")
        lines.append(f"{addr(r['pc'])} {r['mode']}  {r['addressing']:<10} {r['kind']:<11} {r['width']:<5} {addr(r['address'])} {r['count']:<7} "
                     f"{r['first_frame']:<6} {r['last_frame']:<6} {str(r['label'] or ''):<16} {vals}")
    for r in result["rom_reads"]:
        lines.append(f"{addr(r['pc'])} {r['mode']}  {r['addressing']:<10} {r['kind']:<11} rom   {addr(r['min_address'])}-{addr(r['max_address'])} {r['count']:<7} "
                     f"{r['first_frame']:<6} {r['last_frame']:<6}")
    if result["writers"] or result["readers"]:
        lines.append("writers: " + ", ".join(f"{addr(w['pc'])}×{w['count']}" for w in result["writers"]))
        lines.append("readers: " + ", ".join(f"{addr(w['pc'])}×{w['count']}" for w in result["readers"]))
    return "\n".join(lines)


def cmd_query(args: argparse.Namespace) -> int:
    rep = reportmod.Report(sys.argv, task_id=args.task)
    path = Path(args.access)
    if not path.is_file():
        rep.add_check("access_available", "missing", detail=f"{path} not found; run `access capture` first")
        return _finish(rep, args, EXIT_MISSING_PREREQUISITE)
    if args.address is None and args.pc is None:
        rep.add_check("arguments", "failed", detail="pass --address and/or --pc")
        return _finish(rep, args, EXIT_INVALID_INPUT)
    if args.kind is not None and args.kind not in modes.KIND_NAMES:
        rep.add_check("arguments", "failed", detail=f"--kind must be one of {modes.KIND_NAMES}")
        return _finish(rep, args, EXIT_INVALID_INPUT)
    try:
        address = parse_address(args.address) if args.address is not None else None
        pc = parse_address(args.pc) if args.pc is not None else None
    except ValueError as exc:
        rep.add_check("arguments", "failed", detail=f"unparseable address: {exc}")
        return _finish(rep, args, EXIT_INVALID_INPUT)
    try:
        doc = load_access(path)
    except (OSError, ValueError) as exc:
        rep.add_check("access_available", "failed", detail=f"{path}: {exc}")
        return _finish(rep, args, EXIT_INVALID_INPUT)
    rep.add_input("access", path, refcmd.sha256_file(path), scenario_id=doc.get("scenario_id"))
    rep.add_check("access_available", "passed", detail=f"{path} ({doc.get('scenario_id')}, frames {doc['frames']['start']}-{doc['frames']['end']})")
    result = query(doc, address, pc, args.kind)
    n = len(result["rows"]) + len(result["rom_reads"])
    rep.add_check("matches_found", "passed" if n else "failed", detail=f"{n} matching records for address={args.address} pc={args.pc} kind={args.kind}")
    rep.data["query"] = {"address": address, "pc": pc, "kind": args.kind, "matches": n, "writers": result["writers"], "readers": result["readers"]}
    if args.json:
        print(json.dumps(result, indent=1, sort_keys=True))
    else:
        print(format_rows(result))
    return _finish(rep, args, replaycmd._status_from_checks(rep))


# --------------------------------------------------------------- parser


def register(sub: argparse._SubParsersAction) -> None:
    acc = sub.add_parser("access", help="trace-derived memory access record: capture and query (M1-02)")
    asub = acc.add_subparsers(dest="access_command", required=True)

    capture = asub.add_parser("capture", help="run a replay manifest in a fresh worker deriving every instruction's memory accesses")
    capture.add_argument("--manifest", required=True, help="replay manifest JSON (see tests/manifests/replay/)")
    capture.add_argument("--out", required=True, help="directory for access.json, samples, scripts, logs, series and frame images")
    capture.add_argument("--from-frame", type=int, help="first frame to derive (default: the first frame of the run)")
    capture.add_argument("--to-frame", type=int, help="last frame to derive (default: the last frame of the run)")
    capture.add_argument("--watch-address", action="append", help="address ($7E:0313, 0x7E0313) whose accesses are logged per frame (repeatable)")
    capture.add_argument("--watch-pc", action="append", help="pc whose registers are logged at every execution (repeatable)")
    capture.add_argument("--wram-series-range", nargs=2, metavar=("START", "LENGTH"), help="work RAM offset and length written as a binary series")
    capture.add_argument("--wram-series-every", type=int, help="frame stride of the series (default 1)")
    capture.add_argument("--resolve-rmw", action="store_true",
                         help="compute read-modify-write results (INC, DEC, shifts, rotates with P's carry, TSB/TRB with A) from "
                              "their resolved old values, so pointers advanced by INC stay resolved (default: an RMW's result is unknown)")
    capture.add_argument("--ring", type=int, default=DEFAULT_RING, help=f"trace ring capacity (default {DEFAULT_RING})")
    capture.add_argument("--frame-image", type=int, action="append", help="write this frame's video output as PNG under <out>/frames (repeatable)")
    capture.add_argument("--rom", help="ROM file; defaults to the path in local/rom-location.txt")
    capture.add_argument("--expect-region", default="PAL", help="region the core must report (default PAL)")
    capture.add_argument("--root", default=str(ROOT), help=argparse.SUPPRESS)
    capture.add_argument("--timeout", type=float, default=1800, help="seconds for the worker process (default 1800)")
    capture.add_argument("--report", help="write the JSON run report here")
    capture.add_argument("--task", help="task ID to record in the report")
    capture.set_defaults(func=cmd_capture)

    q = asub.add_parser("query", help="list the access records matching an address and/or a pc")
    q.add_argument("--access", required=True, help="access.json written by `access capture`")
    q.add_argument("--address", help="address whose byte is covered by the listed accesses (work RAM mirrors match)")
    q.add_argument("--pc", help="instruction address")
    q.add_argument("--kind", help=f"one of {', '.join(modes.KIND_NAMES)}")
    q.add_argument("--json", action="store_true", help="print the result as JSON instead of a table")
    q.add_argument("--report", help="write the JSON run report here")
    q.add_argument("--task", help="task ID to record in the report")
    q.set_defaults(func=cmd_query)
