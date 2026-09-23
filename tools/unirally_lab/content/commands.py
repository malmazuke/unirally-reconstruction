"""``content`` subcommands (M1-03): provenance inventory from an access record,
manifest-driven decode of ROM content, and the runtime rendering comparison
against a frame image.

Exit codes follow the repository convention: 0 success, 1 failed check
(digest or work RAM mismatch, missing pairing), 2 missing prerequisite
(ROM, access record, dump, image), 3 invalid input.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
from pathlib import Path
from typing import Any

from .. import EXIT_FAILURE, EXIT_INVALID_INPUT, EXIT_MISSING_PREREQUISITE, EXIT_OK
from .. import report as reportmod
from ..access import commands as accesscmd
from ..access import derive as accderive
from ..reference import commands as refcmd
from . import pack as packmod
from . import ppu, provenance, rnc, tracks, zoom_zoo_contract

ROOT = reportmod.repo_root()
MANIFEST_SCHEMA_VERSION = 1


def _finish(rep: reportmod.Report, args: argparse.Namespace, status: int) -> int:
    return refcmd._finish(rep, args, status)


def _status(rep: reportmod.Report) -> int:
    return refcmd._status_from_checks(rep)


def _rom_path(args: argparse.Namespace) -> Path | None:
    if getattr(args, "rom", None):
        return Path(args.rom).expanduser()
    loc = ROOT / "local" / "rom-location.txt"
    if loc.is_file():
        text = loc.read_text(encoding="utf-8").strip()
        if text:
            return Path(text).expanduser()
    return None


def _load_rom(rep: reportmod.Report, args: argparse.Namespace, manifest: dict[str, Any] | None) -> bytes | None:
    path = _rom_path(args)
    if path is None or not path.is_file():
        rep.add_check("rom_available", "missing", detail="no --rom and no local/rom-location.txt (or the file is absent)")
        return None
    data = path.read_bytes()
    sha = hashlib.sha256(data).hexdigest()
    rep.add_input("rom", path, sha, size=len(data))
    if manifest is not None and manifest["rom"]["sha256"] != sha:
        rep.add_check("rom_available", "failed", detail=f"ROM sha256 {sha[:16]} differs from the manifest's {manifest['rom']['sha256'][:16]}")
        return None
    rep.add_check("rom_available", "passed", detail=f"{path} sha256 {sha[:16]}")
    return data


# --------------------------------------------------------------- manifest


def validate_manifest(data: Any) -> dict[str, Any]:
    if not isinstance(data, dict) or data.get("schema_version") != MANIFEST_SCHEMA_VERSION or data.get("kind") != "content_manifest":
        raise ValueError(f"content manifest schema_version must be {MANIFEST_SCHEMA_VERSION} with kind content_manifest")
    for key in ("id", "scenario_id", "rom", "items"):
        if key not in data:
            raise ValueError(f"content manifest lacks {key!r}")
    if not isinstance(data["rom"], dict) or "sha256" not in data["rom"]:
        raise ValueError("rom must carry sha256")
    ids = set()
    for i, item in enumerate(data["items"]):
        for key in ("id", "kind", "expected"):
            if key not in item:
                raise ValueError(f"items[{i}] lacks {key!r}")
        if item["id"] in ids:
            raise ValueError(f"duplicate item id {item['id']!r}")
        ids.add(item["id"])
        if item["kind"] == "raw":
            if not item.get("pieces"):
                raise ValueError(f"items[{i}] raw item needs pieces")
            for p in item["pieces"]:
                if not (isinstance(p.get("file_offset"), int) and isinstance(p.get("length"), int) and p["length"] > 0):
                    raise ValueError(f"items[{i}] piece needs file_offset and a positive length")
        elif item["kind"] == "rnc":
            src = item.get("source", {})
            if not (isinstance(src.get("bank"), int) and isinstance(src.get("address"), int)):
                raise ValueError(f"items[{i}] rnc item needs source.bank and source.address")
        else:
            raise ValueError(f"items[{i}] unknown kind {item['kind']!r}")
        if not isinstance(item["expected"].get("sha256"), str) or not isinstance(item["expected"].get("length"), int):
            raise ValueError(f"items[{i}] expected needs sha256 and length")
    return data


def load_manifest(path: Path) -> dict[str, Any]:
    return validate_manifest(json.loads(path.read_text(encoding="utf-8")))


def decode_item(rom: bytes, item: dict[str, Any]) -> tuple[bytes, dict[str, Any]]:
    """Decoded bytes of one item and details of the transformation."""
    if item["kind"] == "raw":
        parts = [rom[p["file_offset"]:p["file_offset"] + p["length"]] for p in item["pieces"]]
        return b"".join(parts), {"transformation": "raw copy", "pieces": len(parts)}
    src = item["source"]
    read = rnc.lorom_reader(rom)
    header = rnc.parse_header(read, src["bank"], src["address"])
    out, end = rnc.decompress(read, src["bank"], src["address"])
    return out, {"transformation": "rnc method 1 (port of $81:B8E2)", "header": header,
                 "source_end": {"bank": end[0], "address": end[1]}, "source_file_offset": provenance.rom_file_offset((src["bank"] << 16) | src["address"], len(rom))}


# --------------------------------------------------------------- provenance


def _load_access(rep: reportmod.Report, path: Path, name: str = "access") -> dict[str, Any] | None:
    if not path.is_file():
        rep.add_check(f"{name}_available", "missing", detail=f"{path} not found; run `access capture` first")
        return None
    try:
        doc = accderive.validate_document(json.loads(path.read_text(encoding="utf-8")))
    except (OSError, ValueError) as exc:
        rep.add_check(f"{name}_available", "failed", detail=f"{path}: {exc}")
        return None
    rep.add_input(name, path, refcmd.sha256_file(path), scenario_id=doc.get("scenario_id"))
    rep.add_check(f"{name}_available", "passed", detail=f"{path} ({doc.get('scenario_id')}, frames {doc['frames']['start']}-{doc['frames']['end']})")
    return doc


def cmd_provenance(args: argparse.Namespace) -> int:
    rep = reportmod.Report(sys.argv, task_id=args.task)
    doc = _load_access(rep, Path(args.access))
    if doc is None:
        return _finish(rep, args, EXIT_MISSING_PREREQUISITE if not Path(args.access).is_file() else EXIT_INVALID_INPUT)
    rows = provenance.dma_inventory(doc, args.from_frame, args.to_frame)
    moves = provenance.block_moves(doc)
    images = provenance.port_images(doc, args.from_frame, args.to_frame)
    summ = provenance.summary(doc, rows, moves)
    out_dir = Path(args.out)
    out_dir.mkdir(parents=True, exist_ok=True)
    inventory = {
        "schema_version": 1, "kind": "content_provenance", "scenario_id": doc.get("scenario_id"),
        "access": {"path": str(args.access), "sha256": refcmd.sha256_file(Path(args.access))},
        "frames": {"from": args.from_frame, "to": args.to_frame},
        "summary": summ, "dma": rows, "block_moves": moves,
        "port_written_vram_ranges": images["vram_ranges"], "port_written_cgram_ranges": images["cgram_ranges"],
        "pairing_rule": provenance.__doc__.strip().splitlines()[-6:],
    }
    inv_path = out_dir / "provenance.json"
    inv_path.write_text(json.dumps(inventory, indent=1, sort_keys=True) + "\n", encoding="utf-8")
    (out_dir / "port-vram.bin").write_bytes(images["vram"])
    (out_dir / "port-cgram.bin").write_bytes(images["cgram"])
    rep.add_artifact("provenance", inv_path)
    whole = args.from_frame is None and args.to_frame is None
    rep.add_check("dma_triggers_reconcile", "passed" if (not whole or summ["mdmaen_stores"] == summ["record_dma_triggers"]) else "failed",
                  detail=f"{summ['mdmaen_stores']} MDMAEN stores in dma_log, record residual.dma_triggers {summ['record_dma_triggers']}; "
                         f"{summ['channel_transfers']} channel transfers listed")
    rep.add_check("destinations_paired", "passed" if summ["destinations_paired"] == summ["destinations_pairable"] else "failed", required=False,
                  detail=f"{summ['destinations_paired']} of {summ['destinations_pairable']} VRAM/CGRAM/OAM transfers have a watched address-port write in their frame")
    rep.add_check("block_moves_listed", "passed", required=False,
                  detail=f"{summ['block_moves']} transfers, {summ['block_move_bytes']} bytes, {summ['block_move_executions_logged']} logged executions of $00:0199"
                         + ("" if summ["block_moves"] else " (the record did not watch the pc, or none executed)"))
    rep.data["provenance"] = summ
    print(json.dumps(summ, indent=1, sort_keys=True))
    return _finish(rep, args, _status(rep))


# --------------------------------------------------------------- decode


def cmd_decode(args: argparse.Namespace) -> int:
    rep = reportmod.Report(sys.argv, task_id=args.task)
    mpath = Path(args.manifest)
    if not mpath.is_file():
        rep.add_check("manifest_available", "missing", detail=f"{mpath} not found")
        return _finish(rep, args, EXIT_MISSING_PREREQUISITE)
    try:
        manifest = load_manifest(mpath)
    except (OSError, ValueError) as exc:
        rep.add_check("manifest_available", "failed", detail=f"{mpath}: {exc}")
        return _finish(rep, args, EXIT_INVALID_INPUT)
    rep.add_input("manifest", mpath, refcmd.sha256_file(mpath), id=manifest["id"])
    rep.add_check("manifest_available", "passed", detail=f"{mpath} ({manifest['id']}, {len(manifest['items'])} items)")
    rom = _load_rom(rep, args, manifest)
    if rom is None:
        return _finish(rep, args, EXIT_MISSING_PREREQUISITE if _rom_path(args) is None or not _rom_path(args).is_file() else EXIT_FAILURE)
    dump = None
    if args.wram_dump:
        dp = Path(args.wram_dump)
        if not dp.is_file():
            rep.add_check("wram_dump_available", "missing", detail=f"{dp} not found")
            return _finish(rep, args, EXIT_MISSING_PREREQUISITE)
        dump = dp.read_bytes()
        rep.add_input("wram_dump", dp, hashlib.sha256(dump).hexdigest(), size=len(dump))
    out_dir = Path(args.out)
    out_dir.mkdir(parents=True, exist_ok=True)
    results = []
    for item in manifest["items"]:
        data, detail = decode_item(rom, item)
        sha = hashlib.sha256(data).hexdigest()
        ok = sha == item["expected"]["sha256"] and len(data) == item["expected"]["length"]
        (out_dir / f"{item['id']}.bin").write_bytes(data)
        rep.add_check(f"decode_{item['id']}", "passed" if ok else "failed",
                      detail=f"{len(data)} bytes sha256 {sha[:16]} (expected {item['expected']['length']} bytes {item['expected']['sha256'][:16]}); {detail['transformation']}")
        entry = {"id": item["id"], "kind": item["kind"], "length": len(data), "sha256": sha, "matches_expected": ok, **detail}
        rt = item.get("runtime", {})
        if dump is not None and isinstance(rt.get("work_ram_offset"), int):
            off = rt["work_ram_offset"]
            ref = dump[off:off + len(data)]
            known = set(rt.get("known_runtime_writes", []))
            diffs = [i for i in range(len(data)) if i >= len(ref) or data[i] != ref[i]]
            unexplained = [i for i in diffs if i not in known]
            rep.add_check(f"work_ram_{item['id']}", "passed" if not unexplained else "failed",
                          detail=f"{len(data) - len(diffs)} of {len(data)} bytes equal the dump at 0x{off:X}; differing offsets {[hex(i) for i in diffs[:8]]}"
                                 f"{'…' if len(diffs) > 8 else ''}; known runtime writes {[hex(i) for i in sorted(known)]}")
            entry["work_ram"] = {"offset": off, "equal_bytes": len(data) - len(diffs), "differing_offsets": diffs[:64], "unexplained": len(unexplained)}
        results.append(entry)
    res_path = out_dir / "decode.json"
    res_path.write_text(json.dumps({"schema_version": 1, "kind": "content_decode", "manifest": manifest["id"], "items": results}, indent=1, sort_keys=True) + "\n", encoding="utf-8")
    rep.add_artifact("decode", res_path)
    rep.data["decode"] = {"items": len(results), "all_match": all(r["matches_expected"] for r in results)}
    return _finish(rep, args, _status(rep))


def cmd_zoom_zoo_contract(args: argparse.Namespace) -> int:
    """Validate the bounded reference contract directly from the supported ROM."""
    rep = reportmod.Report(sys.argv, task_id=args.task)
    path = Path(args.contract)
    if not path.is_file():
        rep.add_check("contract_available", "missing", detail=f"{path} not found")
        return _finish(rep, args, EXIT_MISSING_PREREQUISITE)
    try:
        contract = json.loads(path.read_text(encoding="utf-8"))
        zoom_zoo_contract.validate_manifest(contract)
    except (OSError, json.JSONDecodeError, ValueError) as exc:
        rep.add_check("contract_available", "failed", detail=f"{path}: {exc}")
        return _finish(rep, args, EXIT_INVALID_INPUT)
    rep.add_input("contract", path, refcmd.sha256_file(path), id=contract["id"])
    rep.add_check("contract_available", "passed", detail=f"{path} ({contract['id']})")
    rom = _load_rom(rep, args, {"rom": {"sha256": contract["identity"]["rom_sha256"]}})
    if rom is None:
        missing = _rom_path(args) is None or not _rom_path(args).is_file()
        return _finish(rep, args, EXIT_MISSING_PREREQUISITE if missing else EXIT_FAILURE)
    try:
        result = zoom_zoo_contract.validate(contract, rom)
    except zoom_zoo_contract.ContractError as exc:
        rep.add_check("reference_contract", "failed", detail=str(exc))
        return _finish(rep, args, EXIT_FAILURE)
    rep.add_check("reference_contract", "passed",
                  detail=(f"{result['decoded_bytes']} decoded bytes; {result['selected_entries']} ordered entries, "
                          f"{result['transfers']} transfers; {result['gather_bytes']} gather bytes and "
                          f"{result['collision_words']} collision words reconstructed"))
    rep.data["zoom_zoo_contract"] = result
    return _finish(rep, args, EXIT_OK)


# --------------------------------------------------------------- Classic pack


def _pack_rules(args: argparse.Namespace) -> tuple[Path, dict[str, Any], str]:
    path = Path(args.rules) if args.rules else ROOT / packmod.RULES_PATH
    rules, digest = packmod.load_rules(path)
    return path, rules, digest


def cmd_pack(args: argparse.Namespace) -> int:
    rep = reportmod.Report(sys.argv, task_id=args.task)
    status = EXIT_OK
    output = Path(args.out)
    try:
        rules_path, rules, rules_digest = _pack_rules(args)
        rep.add_input("extraction_rules", rules_path, rules_digest)
        rom_path = _rom_path(args)
        if rom_path is None or not rom_path.is_file():
            rep.add_check("rom_available", "missing", detail="supported ROM path is absent")
            return _finish(rep, args, EXIT_MISSING_PREREQUISITE)
        rom = rom_path.read_bytes()
        rep.add_input("rom", rom_path, hashlib.sha256(rom).hexdigest(), size=len(rom))
        payload, rows = packmod.build_pack(rom, rules, rules_digest)
        rep.add_check("exact_rom_identity", "passed", detail=rules["source_rom"]["sha256"])
        packmod.write_atomic(output, payload, interrupt_before_commit=args.simulate_interruption_before_commit)
        inspected = packmod.validate_pack(output.read_bytes(), rules, rules_digest)
        rep.add_check("atomic_pack_commit", "passed", detail=str(output))
        rep.add_check("entry_inventory", "passed", detail=f"{len(rows)} logical entries")
        rep.add_artifact("classic_pack", output)
        rep.data["pack"] = inspected
    except InterruptedError as exc:
        rep.add_check("atomic_pack_commit", "failed", detail=str(exc)); status = EXIT_FAILURE
        rep.data["output_exists"] = output.exists()
    except FileNotFoundError as exc:
        rep.add_check("prerequisite", "missing", detail=str(exc)); status = EXIT_MISSING_PREREQUISITE
    except (ValueError, json.JSONDecodeError, OSError) as exc:
        rep.add_check("pack_input", "failed", detail=str(exc)); status = EXIT_INVALID_INPUT
    return _finish(rep, args, status)


def cmd_pack_inspect(args: argparse.Namespace) -> int:
    rep = reportmod.Report(sys.argv, task_id=args.task)
    path = Path(args.pack)
    try:
        rules_path, rules, rules_digest = _pack_rules(args)
        rep.add_input("extraction_rules", rules_path, rules_digest)
        if not path.is_file():
            rep.add_check("pack_available", "missing", detail=f"{path} not found")
            return _finish(rep, args, EXIT_MISSING_PREREQUISITE)
        data = path.read_bytes()
        rep.add_input("classic_pack", path, hashlib.sha256(data).hexdigest(), size=len(data))
        inspected = packmod.validate_pack(data, rules, rules_digest)
        rep.add_check("pack_structure", "passed", detail="schema, identities and canonical layout")
        rep.add_check("entry_inventory", "passed", detail=f"{len(inspected['entries'])} logical entries")
        rep.add_check("entry_hashes", "passed", detail="all payload SHA-256 values match")
        rep.data["pack"] = inspected
        if args.print:
            print(json.dumps(inspected, indent=2, sort_keys=True))
        return _finish(rep, args, EXIT_OK)
    except FileNotFoundError as exc:
        rep.add_check("prerequisite", "missing", detail=str(exc)); return _finish(rep, args, EXIT_MISSING_PREREQUISITE)
    except (ValueError, json.JSONDecodeError, OSError) as exc:
        rep.add_check("pack_validation", "failed", detail=str(exc)); return _finish(rep, args, EXIT_INVALID_INPUT)


# --------------------------------------------------------------- compare


def _merge_watches(docs: list[dict[str, Any]]) -> dict[str, Any]:
    """Watch logs of several records over the same run; later records override per (address, frame)."""
    merged: dict[str, dict[str, Any]] = {}
    for d in docs:
        for addr, frames in d.get("watch_addresses", {}).items():
            merged.setdefault(addr, {}).update(frames)
    return {"watch_addresses": merged}


def _port_events(doc: dict[str, Any], offsets: set[int], upto: int) -> list[tuple[int, int, int, int, int]]:
    """(frame, seq, register offset, width, value) of watched CPU writes to ``offsets`` up to frame ``upto``, in order."""
    ev = []
    for key, frames in doc.get("watch_addresses", {}).items():
        a = int(key)
        off = a & 0xFFFF
        bank = a >> 16
        if off not in offsets or not (bank <= 0x3F or 0x80 <= bank <= 0xBF):
            continue
        for f, per in frames.items():
            if int(f) > upto:
                continue
            for seq, _pc, _kind, address, width, value in per.get("w", []):
                if value is not None:
                    ev.append((int(f), seq, address & 0xFFFF, width, value))
    ev.sort()
    return ev


def build_scene(manifest: dict[str, Any], rom: bytes, ports: dict[str, Any], watches: dict[str, Any], series: bytes | None,
                series_meta: dict[str, Any] | None, frame: int, oam_shadow: bytes, first_frame: int) -> dict[str, Any]:
    """VRAM, CGRAM and OAM as the original had them for ``frame``, from the decoded items plus the record."""
    scene = manifest["scene"]
    vram = bytearray(0x10000)
    cgram = bytearray(512)
    notes: list[str] = []
    placed = []
    for item in manifest["items"]:
        data, _ = decode_item(rom, item)
        pos = 0
        for piece in item.get("pieces", [{"length": len(data)}]):
            chunk = data[pos:pos + piece["length"]]
            pos += piece["length"]
            if "vram_word" in piece:
                vram[piece["vram_word"] * 2:piece["vram_word"] * 2 + len(chunk)] = chunk
            elif "cgram_byte" in piece:
                cgram[piece["cgram_byte"]:piece["cgram_byte"] + len(chunk)] = chunk
            elif "vram_byte" in item:
                vram[item["vram_byte"] + pos - len(chunk):item["vram_byte"] + pos] = chunk
        placed.append(item["id"])
    # DMA uploads recorded up to the frame: ROM sources (sprite tiles) and the tilemap staging buffers (series).
    rows = provenance.dma_inventory(ports, first_frame, frame)
    rom_dma = 0
    staged = 0
    unpaired = 0
    stage_sources = set(scene.get("tilemap_staging", {}).values())
    for r in rows:
        if r["b_bus_name"] != "VMDATAL" or r["direction"] != "A->B":
            continue
        if r.get("vmadd") is None:
            unpaired += 1
            continue
        src: bytes | None = None
        if r["a_bus_region"] == "rom":
            src = rom[r["rom_file_offset"]:r["rom_file_offset"] + r["size"]]
            rom_dma += 1
        elif r["a_bus_region"] == "wram" and series is not None and series_meta is not None and (r["a_bus"] & 0xFFFF) in stage_sources:
            start = series_meta["start"]
            length = series_meta["length"]
            # The DMA runs in the vblank at the start of its frame; the staging buffer it copies is the
            # one the previous frame's game loop left, i.e. the series entry of frame - 1 (calibrated, R-0008).
            src_frame = r["frame"] - 1
            idx = series_meta["frames"].index(src_frame) if src_frame in series_meta["frames"] else None
            if idx is not None:
                chunk = series[idx * length:(idx + 1) * length]
                off = (r["a_bus"] & 0xFFFF) - start
                src = chunk[off:off + r["size"]]
                staged += 1
        if src is None:
            continue
        vmadd = r["vmadd"]
        step = (1, 32, 128, 128)[(r["vmain"] or 0x80) & 3]
        for k in range(0, len(src) - 1, 2):
            i = ((vmadd * 2) & 0xFFFF)
            vram[i] = src[k]
            vram[(i + 1) & 0xFFFF] = src[k + 1]
            vmadd = (vmadd + step) & 0xFFFF
    # CGRAM and OAM port writes up to the frame.
    cgadd = 0
    latch = False
    for _f, _seq, off, width, value in _port_events(watches, {0x2121, 0x2122}, frame):
        if off == 0x2121:
            cgadd = value & 0xFF
            latch = False
            if width == 2:
                cgram[(cgadd * 2) & 511] = (value >> 8) & 0xFF
                latch = True
        else:
            cgram[((cgadd * 2) + (1 if latch else 0)) & 511] = value & 0xFF
            latch = not latch
            if not latch:
                cgadd = (cgadd + 1) & 0xFF
    oam = bytearray(oam_shadow)
    oamaddr = 0
    for _f, _seq, off, width, value in _port_events(watches, {0x2102, 0x2103, 0x2104}, frame):
        if off == 0x2102:
            oamaddr = ((value & 0x1FF) * 2) if width == 2 else ((oamaddr & 0x200) | ((value & 0xFF) * 2))
        elif off == 0x2103:
            oamaddr = (oamaddr & 0x1FF) | ((value & 1) << 9)
        else:
            if oamaddr < 544:
                oam[oamaddr] = value & 0xFF
            oamaddr += 1
    notes.append(f"{rom_dma} ROM->VRAM DMAs and {staged} staging-buffer DMAs replayed from frames {first_frame}-{frame}; {unpaired} VRAM DMAs had no paired destination")
    return {"vram": bytes(vram), "cgram": bytes(cgram), "oam": bytes(oam), "notes": notes, "placed": placed}


def _hdma_scroll(dump: bytes, table: int) -> tuple[int, int]:
    """First entry of a direct mode-3 HDMA table (count, lo, hi, lo, hi) as (first register, second register)."""
    return dump[table + 1] | (dump[table + 2] << 8), dump[table + 3] | (dump[table + 4] << 8)


def render_scene(manifest: dict[str, Any], scene: dict[str, Any], scroll: dict[str, tuple[int, int]], order: str) -> bytes:
    sc = manifest["scene"]
    palette = ppu.palette_from_cgram(scene["cgram"], order)
    bg1 = sc["bg1"]
    bg2 = sc["bg2"]
    l1 = ppu.render_bg(scene["vram"], bg1["map_byte"], bg1["wide"], bg1["tall"], bg1["tile_byte"], bg1["bpp"], bg1["tiles16"], *scroll["bg1"])
    l2 = ppu.render_bg(scene["vram"], bg2["map_byte"], bg2["wide"], bg2["tall"], bg2["tile_byte"], bg2["bpp"], bg2["tiles16"], *scroll["bg2"])
    obj = ppu.render_sprites(scene["vram"], scene["oam"], sc["obsel"])
    layers = [(l2, 0), (l1, 0), (obj, 0), (l2, 1), (l1, 1), (obj, 1), (obj, 2), (obj, 3)]
    return ppu.compose(palette, sc.get("backdrop", 0), layers)


def cmd_compare(args: argparse.Namespace) -> int:
    rep = reportmod.Report(sys.argv, task_id=args.task)
    mpath = Path(args.manifest)
    if not mpath.is_file():
        rep.add_check("manifest_available", "missing", detail=f"{mpath} not found")
        return _finish(rep, args, EXIT_MISSING_PREREQUISITE)
    try:
        manifest = load_manifest(mpath)
        if "scene" not in manifest:
            raise ValueError("the manifest carries no scene section")
    except (OSError, ValueError) as exc:
        rep.add_check("manifest_available", "failed", detail=f"{mpath}: {exc}")
        return _finish(rep, args, EXIT_INVALID_INPUT)
    rep.add_input("manifest", mpath, refcmd.sha256_file(mpath), id=manifest["id"])
    rep.add_check("manifest_available", "passed", detail=f"{mpath} ({manifest['id']})")
    rom = _load_rom(rep, args, manifest)
    if rom is None:
        return _finish(rep, args, EXIT_MISSING_PREREQUISITE)
    docs = []
    for i, p in enumerate(args.access):
        d = _load_access(rep, Path(p), "access" if i == 0 else f"access_{i}")
        if d is None:
            return _finish(rep, args, EXIT_MISSING_PREREQUISITE)
        docs.append(d)
    ports = docs[0]
    series = None
    meta = ports.get("wram_series")
    if meta:
        sp = Path(args.access[0]).parent / meta["path"]
        if sp.is_file():
            series = sp.read_bytes()
            rep.add_input("wram_series", sp, hashlib.sha256(series).hexdigest(), frames=len(meta["frames"]))
    if series is None:
        rep.add_check("series_available", "missing", detail="the first access record carries no work RAM series; tilemap uploads cannot be replayed")
        return _finish(rep, args, EXIT_MISSING_PREREQUISITE)
    for name, p in (("oam_dump", args.oam_dump), ("scroll_dump", args.scroll_dump), ("frame_image", args.frame_image)):
        if not Path(p).is_file():
            rep.add_check(f"{name}_available", "missing", detail=f"{p} not found")
            return _finish(rep, args, EXIT_MISSING_PREREQUISITE)
    oam_dump = Path(args.oam_dump).read_bytes()
    scroll_dump = Path(args.scroll_dump).read_bytes()
    rep.add_input("oam_dump", Path(args.oam_dump), hashlib.sha256(oam_dump).hexdigest())
    rep.add_input("scroll_dump", Path(args.scroll_dump), hashlib.sha256(scroll_dump).hexdigest())
    png = Path(args.frame_image).read_bytes()
    rep.add_input("frame_image", Path(args.frame_image), hashlib.sha256(png).hexdigest())
    try:
        width, height, image = ppu.read_png(png)
    except ValueError as exc:
        rep.add_check("frame_image_readable", "failed", detail=str(exc))
        return _finish(rep, args, EXIT_INVALID_INPUT)
    if (width, height) != (ppu.SCREEN_WIDTH, ppu.SCREEN_HEIGHT):
        rep.add_check("frame_image_readable", "failed", detail=f"{width}x{height}, expected 256x224")
        return _finish(rep, args, EXIT_INVALID_INPUT)
    sc = manifest["scene"]
    shadow = oam_dump[sc["oam_shadow"]:sc["oam_shadow"] + 544]
    upload_frame = args.frame if args.upload_frame is None else args.upload_frame
    scene = build_scene(manifest, rom, ports, _merge_watches(docs), series, meta, upload_frame, shadow, sc.get("first_race_frame", 0))
    scroll = {"bg1": _hdma_scroll(scroll_dump, sc["bg1"]["hdma_scroll_table"]), "bg2": _hdma_scroll(scroll_dump, sc["bg2"]["hdma_scroll_table"])}
    if args.bg1_scroll:
        scroll["bg1"] = (int(args.bg1_scroll[0], 0), int(args.bg1_scroll[1], 0))
    if args.bg2_scroll:
        scroll["bg2"] = (int(args.bg2_scroll[0], 0), int(args.bg2_scroll[1], 0))
    rendered = render_scene(manifest, scene, scroll, args.colour_order)
    rect = tuple(int(v, 0) for v in args.rect) if args.rect else (0, 16, ppu.SCREEN_WIDTH, ppu.SCREEN_HEIGHT - 16)
    if not (0 <= rect[0] and 0 <= rect[1] and rect[0] + rect[2] <= width and rect[1] + rect[3] <= height and rect[2] > 0 and rect[3] > 0):
        rep.add_check("rect_valid", "failed", detail=f"rectangle {rect} lies outside the 256x224 frame")
        return _finish(rep, args, EXIT_INVALID_INPUT)
    mism, total, diff = ppu.compare(rendered, image, width, rect)
    out_dir = Path(args.out)
    out_dir.mkdir(parents=True, exist_ok=True)
    (out_dir / f"render-{args.frame:05d}.png").write_bytes(ppu.write_png(width, height, rendered))
    (out_dir / f"diff-{args.frame:05d}.png").write_bytes(ppu.write_png(rect[2], rect[3], diff))
    (out_dir / f"side-by-side-{args.frame:05d}.png").write_bytes(ppu.write_png(width * 2, height, ppu.side_by_side([rendered, image], width, height)))
    for kind in ("render", "diff", "side-by-side"):
        rep.add_artifact(kind, out_dir / f"{kind}-{args.frame:05d}.png")
    result = {
        "frame": args.frame, "upload_frame": upload_frame, "rect": list(rect), "mismatches": mism, "pixels": total, "mismatch_fraction": mism / total,
        "scroll": {k: list(v) for k, v in scroll.items()}, "colour_order": args.colour_order, "omitted": list(ppu.OMITTED),
        "scene_notes": scene["notes"], "items_placed": scene["placed"],
    }
    (out_dir / f"compare-{args.frame:05d}.json").write_text(json.dumps(result, indent=1, sort_keys=True) + "\n", encoding="utf-8")
    rep.add_check("frame_rendered", "passed", detail=f"frame {args.frame}: BG1 scroll {scroll['bg1']}, BG2 scroll {scroll['bg2']}; {scene['notes'][0]}")
    threshold = args.max_mismatch_fraction
    rep.add_check("pixels_match", "passed" if mism / total <= threshold else "failed", required=threshold < 1.0,
                  detail=f"{mism} of {total} pixels differ over rectangle x={rect[0]} y={rect[1]} w={rect[2]} h={rect[3]} ({100.0 * mism / total:.2f} %); "
                         f"renderer omits: {'; '.join(ppu.OMITTED)}")
    rep.data["compare"] = result
    return _finish(rep, args, _status(rep))


# --------------------------------------------------------------- parser


# --------------------------------------------------------------- tracks (TRACK-BREADTH)


def cmd_rnc_inventory(args: argparse.Namespace) -> int:
    rep = reportmod.Report(sys.argv, task_id=args.task)
    rom = _load_rom(rep, args, None)
    if rom is None:
        return _finish(rep, args, EXIT_MISSING_PREREQUISITE)
    try:
        inv = tracks.inventory(rom)
    except (tracks.TrackError, ValueError, IndexError) as exc:
        rep.add_check("track_streams", "failed", detail=str(exc))
        return _finish(rep, args, EXIT_FAILURE)
    streams = inv["streams"]
    scanned = [i for i in range(len(rom) - 3) if rom[i:i + 4] == b"RNC\x01"]
    rep.add_check("directory_matches_scan", "passed" if scanned == [s["file_offset"] for s in streams] else "failed",
                  detail=f"{len(streams)} compressed assets from {inv['first_track_asset']}; {len(scanned)} RNC method-1 headers in the ROM")
    short = [s["index"] for s in streams if s["consumed_length"] != s["directory_length"]]
    rep.add_check("consumed_equals_directory_length", "passed" if not short else "failed",
                  detail="every stream consumes exactly its directory length" if not short else f"tracks {short} differ")
    text = json.dumps(inv, indent=1, sort_keys=True) + "\n"
    if args.out:
        out = Path(args.out)
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text(text, encoding="utf-8")
        rep.add_artifact("track_streams", out)
    if args.expect:
        expected = Path(args.expect).read_text(encoding="utf-8")
        rep.add_check("matches_tracked_manifest", "passed" if expected == text else "failed",
                      detail=f"{args.expect} {'identical' if expected == text else 'differs'}")
    rep.data["track_count"] = inv["track_count"]
    rep.data["inventory_sha256"] = hashlib.sha256(text.encode()).hexdigest()
    return _finish(rep, args, _status(rep))


def write_track_override(rom: bytes, index: int, out: Path) -> dict[str, Any]:
    """The three files `zoom_zoo_runner --track-override` reads, for track ``index``."""
    decoded, _ = tracks.decode_track(rom, index)
    header = tracks.parse_header(decoded)
    derived = tracks.tile_content(rom, header["tile_set_ids"])
    out.mkdir(parents=True, exist_ok=True)
    (out / "track-data.bin").write_bytes(decoded)
    (out / "tile-tables.bin").write_bytes(derived["tile_columns"])
    (out / "tile-flags.bin").write_bytes(derived["tile_flags"])
    return header


def cmd_track_idle_matrix(args: argparse.Namespace) -> int:
    """Every track from native race start with a released controller for a declared number of updates."""
    rep = reportmod.Report(sys.argv, task_id=args.task)
    out = Path(args.out)
    if out.exists():
        rep.add_check("fresh_output", "failed", detail=f"{out} exists; the matrix requires a fresh directory")
        return _finish(rep, args, EXIT_INVALID_INPUT)
    runner, pack_path = Path(args.runner), Path(args.pack)
    for label, path in (("runner", runner), ("pack", pack_path)):
        if not path.is_file():
            rep.add_check(f"{label}_available", "missing", detail=f"{path} not found")
            return _finish(rep, args, EXIT_MISSING_PREREQUISITE)
        rep.add_input(label, path, refcmd.sha256_file(path))
    rom = _load_rom(rep, args, None)
    if rom is None:
        return _finish(rep, args, EXIT_MISSING_PREREQUISITE)
    out.mkdir(parents=True)
    base = [str(runner), "--start", args.scenario, "--content-pack", str(pack_path)]
    empty = out / "no-inputs.txt"
    empty.write_text("")
    first = subprocess.run(base + ["--inputs", str(empty)], capture_output=True, text=True, timeout=60)
    if first.returncode != 0 or not first.stdout:
        rep.add_check("scenario_start", "failed", detail=first.stderr.strip()[:200])
        return _finish(rep, args, EXIT_FAILURE)
    start_frame = int(first.stdout.split()[0])
    inputs = out / "idle-inputs.txt"
    inputs.write_text("".join(f"{start_frame + k} 0 0\n" for k in range(1, args.updates + 1)))
    only = args.track or list(range(tracks.track_count(rom)))
    rows = []
    for index in only:
        directory = out / f"{index:02d}"
        header = write_track_override(rom, index, directory / "content")
        run = subprocess.run(base + ["--track-override", str(directory / "content"), "--inputs", str(inputs)],
                             capture_output=True, text=True, timeout=600)
        lines = run.stdout.splitlines()
        (directory / "states.txt").write_text(run.stdout)
        row = {"track": index, "shape": f"0x{header['shape']['byte']:02X}", "exit": run.returncode,
               "updates": max(len(lines) - 1, 0), "completed": run.returncode == 0 and len(lines) - 1 == args.updates,
               "fault": run.stderr.strip()[:200], "states_sha256": hashlib.sha256(run.stdout.encode()).hexdigest()}
        (directory / "run.json").write_text(json.dumps(row, indent=1, sort_keys=True) + "\n")
        rows.append(row)
    matrix = {"schema_version": 1, "kind": "track_idle_matrix", "scenario": args.scenario, "start_frame": start_frame,
              "updates": args.updates, "rows": rows}
    (out / "matrix.json").write_text(json.dumps(matrix, indent=1, sort_keys=True) + "\n")
    rep.add_artifact("matrix", out / "matrix.json")
    completed = sum(r["completed"] for r in rows)
    rep.add_check("matrix_written", "passed", detail=f"{completed} of {len(rows)} tracks completed {args.updates} updates")
    rep.data["completed"] = completed
    rep.data["faults"] = {r["track"]: r["fault"] for r in rows if not r["completed"]}
    return _finish(rep, args, _status(rep))


def register(sub: argparse._SubParsersAction) -> None:
    content = sub.add_parser("content", help="content provenance, decode and runtime comparison (M1-03)")
    csub = content.add_subparsers(dest="content_command", required=True)

    prov = csub.add_parser("provenance", help="DMA, block-move and port-write inventory from an access record")
    prov.add_argument("--access", required=True, help="access.json written by `access capture` (PPU address ports watched for destinations)")
    prov.add_argument("--out", required=True, help="directory for provenance.json and the port-written VRAM/CGRAM images")
    prov.add_argument("--from-frame", type=int)
    prov.add_argument("--to-frame", type=int)
    prov.add_argument("--report", help="write the JSON run report here")
    prov.add_argument("--task", help="task ID to record in the report")
    prov.set_defaults(func=cmd_provenance)

    dec = csub.add_parser("decode", help="decode the items of a content manifest from the ROM and verify their digests")
    dec.add_argument("--manifest", required=True, help="content manifest JSON (see tests/manifests/content/)")
    dec.add_argument("--out", required=True, help="directory for the decoded bytes (ignored artifacts) and decode.json")
    dec.add_argument("--rom", help="ROM file; defaults to the path in local/rom-location.txt")
    dec.add_argument("--wram-dump", help="whole work RAM dump to compare items that carry runtime.work_ram_offset against")
    dec.add_argument("--report", help="write the JSON run report here")
    dec.add_argument("--task", help="task ID to record in the report")
    dec.set_defaults(func=cmd_decode)

    zz = csub.add_parser("zoom-zoo-contract", help="validate the bounded M4-03 reference-only ZOOM ZOO contract")
    zz.add_argument("--contract", required=True, help="tracked ZOOM ZOO reference contract JSON")
    zz.add_argument("--rom", help="ROM file; defaults to local/rom-location.txt")
    zz.add_argument("--report", help="write the JSON run report here")
    zz.add_argument("--task", default="M4-03")
    zz.set_defaults(func=cmd_zoom_zoo_contract)

    pack = csub.add_parser("pack", help="extract the exact PAL ROM into an atomic Classic content pack")
    pack.add_argument("--rom", help="ROM file; defaults to local/rom-location.txt")
    pack.add_argument("--out", required=True, help="new ignored Classic pack path")
    pack.add_argument("--rules", help=argparse.SUPPRESS)
    pack.add_argument("--simulate-interruption-before-commit", action="store_true", help=argparse.SUPPRESS)
    pack.add_argument("--report")
    pack.add_argument("--task", default="M3-02A")
    pack.set_defaults(func=cmd_pack)

    inspect = csub.add_parser("pack-inspect", help="validate a Classic pack without reading a ROM")
    inspect.add_argument("--pack", required=True)
    inspect.add_argument("--rules", help=argparse.SUPPRESS)
    inspect.add_argument("--print", action="store_true")
    inspect.add_argument("--report")
    inspect.add_argument("--task", default="M3-02A")
    inspect.set_defaults(func=cmd_pack_inspect)

    inv = csub.add_parser("rnc-inventory", help="locate, unpack and digest every track stream through the asset directory (TRACK-BREADTH)")
    inv.add_argument("--rom", help="ROM file; defaults to local/rom-location.txt")
    inv.add_argument("--out", help="write the stream manifest (locations, sizes, header fields, digests; no bytes) here")
    inv.add_argument("--expect", help="tracked manifest the inventory must equal byte for byte")
    inv.add_argument("--report")
    inv.add_argument("--task", default="TRACK-BREADTH")
    inv.set_defaults(func=cmd_rnc_inventory)

    idle = csub.add_parser("track-idle-matrix", help="run every track headless from native race start with a released controller (TRACK-BREADTH)")
    idle.add_argument("--out", required=True, help="fresh directory for per-track content, states and matrix.json")
    idle.add_argument("--updates", required=True, type=int, help="updates per track, declared before the run")
    idle.add_argument("--track", type=int, action="append", help="track index (repeatable; default all)")
    idle.add_argument("--scenario", default="classic.crawler.zoom-zoo", help="scenario the runner's --start names")
    idle.add_argument("--runner", default="build/lab-debug/src/core/zoom_zoo_runner")
    idle.add_argument("--pack", default="local/classic-pal-crawler-two-tracks-v9.pack")
    idle.add_argument("--rom")
    idle.add_argument("--report")
    idle.add_argument("--task", default="TRACK-BREADTH")
    idle.set_defaults(func=cmd_track_idle_matrix)

    cmp_ = csub.add_parser("compare", help="render the decoded content as the original had it at a frame and compare with the frame image")
    cmp_.add_argument("--manifest", required=True)
    cmp_.add_argument("--access", required=True, action="append", help="access records: the first with the work RAM series and address-port watches; later ones add watch logs (repeatable)")
    cmp_.add_argument("--frame", required=True, type=int)
    cmp_.add_argument("--frame-image", required=True, help="PNG of the frame written by `access capture --frame-image`")
    cmp_.add_argument("--oam-dump", required=True, help="work RAM dump holding the OAM image the original uploaded (scene.oam_shadow)")
    cmp_.add_argument("--scroll-dump", required=True, help="work RAM dump holding the HDMA scroll tables in effect for the frame")
    cmp_.add_argument("--out", required=True)
    cmp_.add_argument("--rect", nargs=4, metavar=("X", "Y", "W", "H"), help="comparison rectangle (default 0 16 256 208)")
    cmp_.add_argument("--rom")
    cmp_.add_argument("--colour-order", default="bgr", choices=("bgr", "rgb"))
    cmp_.add_argument("--upload-frame", type=int, help="last frame whose DMAs and port writes are applied (default: the compared frame)")
    cmp_.add_argument("--bg1-scroll", nargs=2, metavar=("H", "V"))
    cmp_.add_argument("--bg2-scroll", nargs=2, metavar=("H", "V"))
    cmp_.add_argument("--max-mismatch-fraction", type=float, default=1.0, help="fail when more than this fraction of pixels differ (default: report only)")
    cmp_.add_argument("--report")
    cmp_.add_argument("--task")
    cmp_.set_defaults(func=cmd_compare)
