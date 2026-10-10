"""Derive the observed code/data map from a coverage document and the ROM.

Everything here is a pure function of the coverage document (``drain``) and
the ROM bytes, so the derivation is testable ROM-free on synthetic inputs and
reproducible: the same coverage file and ROM give the same map.

Rules (M1-01):

- An executed site (pc, mode, data bank) in ROM is decoded with the opcode
  byte at its LoROM offset and the length under its observed M/X flags. Its
  opcode byte and operand bytes are classified; a byte that is both an opcode
  and an operand of different executed instructions counts as an opcode and
  the overlap is reported. Every other ROM byte stays *unclassified*.
- A site outside ROM (work RAM or anything else) is listed, not decoded.
- A trace step from site a to site b whose target is not a's sequential
  successor is an *edge*; its kind is the predecessor's control-flow opcode
  (JSR/JSL/JMP/JML/RTS/RTL/RTI/BRK/COP/branch), or ``interrupt entry`` when
  the target is an NMI/IRQ vector target (the entry itself is not traced,
  R-0002 finding 7), or ``unknown`` otherwise (for example a predecessor
  outside ROM).
- Absolute and long operands of executed instructions are *static
  references* (data in the observed data bank, or code for JSR/JMP/JSL/JML);
  indexed, indirect and direct-page forms are not resolved.
"""

from __future__ import annotations

from collections import Counter, defaultdict
from typing import Any

from . import mapping
from .opcodes import (
    FLOW_KINDS, MODE_E, MODE_M, MODE_X, TABLE, instruction_length, mode_name, sequential_successor, static_reference,
)

MAP_SCHEMA_VERSION = 1
MAPPING_RULE = ("LoROM (bsnes board LOROM-RAM#A): banks $00-$6F and $80-$EF, offsets $8000-$FFFF; "
                "ROM offset = ((bank & $7F) << 15 | (address & $7FFF)) mod ROM size")
# Header vector slots: (name, header-relative offset). The header lies at ROM
# offset 0x7FC0 (LoROM), vectors from 0x7FE0; addresses are bank $00.
NATIVE_VECTORS = (("native_cop", 0x24), ("native_brk", 0x26), ("native_abort", 0x28), ("native_nmi", 0x2A),
                  ("native_reserved", 0x2C), ("native_irq", 0x2E))
EMULATION_VECTORS = (("emu_cop", 0x34), ("emu_reserved", 0x36), ("emu_abort", 0x38), ("emu_nmi", 0x3A),
                     ("emu_reset", 0x3C), ("emu_irq_brk", 0x3E))
INTERRUPT_VECTORS = frozenset({"native_nmi", "native_irq", "emu_nmi", "emu_irq_brk"})
EDGE_KINDS = ("JSR", "JSL", "JMP", "JML", "RTS", "RTL", "RTI", "BRK", "COP", "branch", "interrupt entry", "unknown")


def addr(a: int) -> str:
    return f"${a >> 16:02X}:{a & 0xFFFF:04X}"


def parse_addr(text: str) -> int:
    bank, offset = text.lstrip("$").split(":")
    return (int(bank, 16) << 16) | int(offset, 16)


def modes_list(mask: int) -> list[str]:
    return [mode_name(m) for m in range(8) if mask & (1 << m)]


def read_vectors(rom: bytes, header_offset: int = 0x7FC0) -> list[dict[str, Any]]:
    out = []
    for group, table in (("native", NATIVE_VECTORS), ("emulation", EMULATION_VECTORS)):
        for name, rel in table:
            off = header_offset + rel
            value = int.from_bytes(rom[off:off + 2], "little")
            out.append({"name": name, "group": group, "rom_offset": off, "value": value, "target": value})
    return out


class Derivation:
    def __init__(self, coverage: dict[str, Any], rom: bytes, header_offset: int = 0x7FC0) -> None:
        self.cov = coverage
        self.rom = rom
        self.size = len(rom)
        self.header_offset = header_offset
        self.vectors = read_vectors(rom, header_offset)
        self.vector_targets = {v["target"]: v["name"] for v in self.vectors if v["value"] not in (0x0000, 0xFFFF)}
        self.interrupt_targets = {v["target"] for v in self.vectors if v["name"] in INTERRUPT_VECTORS}
        # Per 24-bit byte address (instruction bytes as executed).
        self.byte_kind: dict[int, str] = {}          # "opcode" or "operand"
        self.byte_overlap: set[int] = set()
        self.byte_count: Counter[int] = Counter()    # executions of the instruction covering the byte
        self.byte_modes: dict[int, int] = defaultdict(int)
        self.byte_first: dict[int, int] = {}
        # Per instruction start address.
        self.instr_modes: dict[int, int] = defaultdict(int)
        self.instr_lengths: dict[int, set[int]] = defaultdict(set)
        self.instr_count: Counter[int] = Counter()
        self.instr_first: dict[int, int] = {}
        # Per ROM offset (mirrors folded).
        self.offset_kind: dict[int, str] = {}
        self.offset_overlap: set[int] = set()
        self.non_rom_sites: list[dict[str, Any]] = []
        self.site_decode: dict[tuple[int, int], tuple[int, int]] = {}  # (pc, mode) -> (opcode, length) for ROM sites
        self.static_refs: dict[int, dict[str, Any]] = {}
        self.instructions_by_mode: Counter[int] = Counter()
        self.sites_by_mode: Counter[int] = Counter()
        self.edges: dict[tuple[int, str, int], int] = Counter()  # (target, kind, target mode) -> count
        self.edge_sources: dict[tuple[int, str, int], set[int]] = defaultdict(set)
        self.unknown_edges: list[dict[str, Any]] = []
        self.sequential_steps = 0
        self.edge_steps = 0
        self._decode_sites()
        self._classify_pairs()

    # -------------------------------------------------------------- sites

    def _decode_sites(self) -> None:
        for pc, mode, bank, count, first in self.cov["sites"]:
            self.instructions_by_mode[mode] += count
            self.sites_by_mode[mode] += 1
            region = mapping.classify(pc)
            if region != mapping.REGION_ROM:
                self.non_rom_sites.append({"address": addr(pc), "region": region, "mode": mode_name(mode), "data_bank": bank,
                                           "count": count, "first_frame": first,
                                           "wram_offset": mapping.wram_offset(pc)})
                continue
            offset = mapping.rom_offset(pc, self.size)
            opcode = self.rom[offset]
            length = instruction_length(opcode, mode)
            self.site_decode[(pc, mode)] = (opcode, length)
            self.instr_modes[pc] |= 1 << mode
            self.instr_lengths[pc].add(length)
            self.instr_count[pc] += count
            self.instr_first[pc] = min(first, self.instr_first.get(pc, first))
            operand = bytearray()
            for i in range(length):
                a = sequential_successor(pc, i)
                o = mapping.rom_offset(a, self.size)
                kind = "opcode" if i == 0 else "operand"
                if i:
                    operand.append(self.rom[o])
                for table, overlap, key in ((self.byte_kind, self.byte_overlap, a), (self.offset_kind, self.offset_overlap, o)):
                    prev = table.get(key)
                    if prev is None:
                        table[key] = kind
                    elif prev != kind:
                        overlap.add(key)
                        table[key] = "opcode"
                self.byte_count[a] += count
                self.byte_modes[a] |= 1 << mode
                self.byte_first[a] = min(first, self.byte_first.get(a, first))
            ref = static_reference(opcode, bytes(operand), pc, bank)
            if ref is not None:
                kind, target = ref
                entry = self.static_refs.setdefault(target, {"address": addr(target), "region": mapping.classify(target),
                                                             "rom_offset": mapping.rom_offset(target, self.size),
                                                             "kinds": set(), "references": 0, "from": set()})
                entry["kinds"].add(kind)
                entry["references"] += 1
                entry["from"].add(pc)

    # -------------------------------------------------------------- edges

    def _classify_pairs(self) -> None:
        for pc, mode, bank, npc, nmode, nbank, count in self.cov["pairs"]:
            decoded = self.site_decode.get((pc, mode))
            if decoded is not None:
                opcode, length = decoded
                if npc == sequential_successor(pc, length):
                    self.sequential_steps += count
                    continue
                if npc in self.interrupt_targets and opcode not in (0x00, 0x02):
                    kind = "interrupt entry"
                else:
                    kind = FLOW_KINDS.get(opcode, "unknown")
            else:
                opcode = None
                kind = "interrupt entry" if npc in self.interrupt_targets else "unknown"
            self.edge_steps += count
            key = (npc, kind, nmode)
            self.edges[key] += count
            self.edge_sources[key].add(pc)
            if kind == "unknown":
                self.unknown_edges.append({"from": addr(pc), "from_region": mapping.classify(pc), "from_mode": mode_name(mode),
                                           "from_decoded": opcode is not None, "to": addr(npc),
                                           "to_mode": mode_name(nmode), "count": count})

    # ------------------------------------------------------------- outputs

    def ranges(self) -> list[dict[str, Any]]:
        """Maximal runs of consecutive executed byte addresses (24-bit space)."""
        out: list[dict[str, Any]] = []
        run: list[int] = []

        def flush() -> None:
            if not run:
                return
            start, end = run[0], run[-1]
            starts = [a for a in run if a in self.instr_modes]
            modes = 0
            for a in run:
                modes |= self.byte_modes[a]
            out.append({
                "start": addr(start), "end": addr(end), "bytes": len(run),
                "rom_offset": mapping.rom_offset(start, self.size),
                "opcode_bytes": sum(1 for a in run if self.byte_kind[a] == "opcode"),
                "operand_bytes": sum(1 for a in run if self.byte_kind[a] == "operand"),
                "instructions": len(starts),
                "executions": sum(self.instr_count[a] for a in starts),
                "modes": modes_list(modes),
                "first_frame": min(self.byte_first[a] for a in run),
            })
            run.clear()

        for a in sorted(self.byte_kind):
            if run and a != run[-1] + 1:
                flush()
            run.append(a)
        flush()
        return out

    def multi_mode(self) -> list[dict[str, Any]]:
        return [{"address": addr(a), "modes": modes_list(m), "lengths": sorted(self.instr_lengths[a]), "count": self.instr_count[a]}
                for a, m in sorted(self.instr_modes.items()) if bin(m).count("1") > 1]

    def entry_points(self) -> list[dict[str, Any]]:
        by_target: dict[int, dict[str, Any]] = {}
        for (target, kind, mode), count in self.edges.items():
            e = by_target.setdefault(target, {"address": addr(target), "vector": self.vector_targets.get(target),
                                              "kinds": {}, "modes": 0, "count": 0, "sources": 0})
            if mapping.classify(target) != mapping.REGION_ROM:
                e["region"] = mapping.classify(target)
            e["kinds"][kind] = e["kinds"].get(kind, 0) + count
            e["modes"] |= 1 << mode
            e["count"] += count
        for target, e in by_target.items():
            e["kinds"] = dict(sorted(e["kinds"].items()))
            e["modes"] = modes_list(e["modes"])
            e["sources"] = len(set().union(*(self.edge_sources[k] for k in self.edge_sources if k[0] == target)))
            e["first_frame"] = self.instr_first.get(target)
            if e["vector"] is None:
                del e["vector"]
        return [by_target[t] for t in sorted(by_target)]

    def vector_table(self) -> list[dict[str, Any]]:
        out = []
        watch = self.cov.get("watch", {}).get("per_frame", {})
        start = self.cov["frames"]["start"]
        for v in self.vectors:
            target = v["target"]
            executed = target in self.instr_count and v["value"] not in (0x0000, 0xFFFF)
            entry = {**v, "target": addr(target), "executed": executed,
                     "count": self.instr_count.get(target, 0) if executed else 0,
                     "first_frame": self.instr_first.get(target) if executed else None,
                     "modes": modes_list(self.instr_modes.get(target, 0)) if executed else []}
            series = watch.get(str(target))
            if series is not None and v["value"] not in (0x0000, 0xFFFF):
                entry["per_frame"] = per_frame_summary(series, start)
            out.append(entry)
        return out

    def banks(self) -> list[dict[str, Any]]:
        per: dict[int, dict[str, Any]] = {}
        for a, kind in self.byte_kind.items():
            b = per.setdefault(a >> 16, {"bank": f"${a >> 16:02X}", "bytes": 0, "opcode_bytes": 0, "operand_bytes": 0, "instructions": 0, "executions": 0})
            b["bytes"] += 1
            b[f"{kind}_bytes"] += 1
        for a, n in self.instr_count.items():
            per[a >> 16]["instructions"] += 1
            per[a >> 16]["executions"] += n
        return [per[k] for k in sorted(per)]

    def totals(self) -> dict[str, Any]:
        opcode = sum(1 for k in self.offset_kind.values() if k == "opcode")
        operand = sum(1 for k in self.offset_kind.values() if k == "operand")
        edge_kinds = Counter()
        for (_, kind, _), n in self.edges.items():
            edge_kinds[kind] += n
        return {
            "rom_size": self.size,
            "executed_opcode_bytes": opcode,
            "executed_operand_bytes": operand,
            "unclassified_bytes": self.size - opcode - operand,
            "overlapping_bytes": len(self.offset_overlap),
            "executed_addresses_24bit": len(self.byte_kind),
            "instruction_addresses": len(self.instr_count),
            "sites": len(self.cov["sites"]),
            "non_rom_sites": len(self.non_rom_sites),
            "instructions_executed": self.cov["instructions"]["total"],
            "instructions_by_mode": {mode_name(m): n for m, n in sorted(self.instructions_by_mode.items())},
            "sites_by_mode": {mode_name(m): n for m, n in sorted(self.sites_by_mode.items())},
            "multi_mode_addresses": len(self.multi_mode()),
            "entry_points": len(self.entry_points()),
            "edge_steps": self.edge_steps,
            "sequential_steps": self.sequential_steps,
            "edges_by_kind": {k: edge_kinds.get(k, 0) for k in EDGE_KINDS if edge_kinds.get(k)},
            "unknown_edges": len(self.unknown_edges),
            "static_references": len(self.static_refs),
        }

    def static_references(self) -> list[dict[str, Any]]:
        out = []
        for target in sorted(self.static_refs):
            e = self.static_refs[target]
            out.append({"address": e["address"], "region": e["region"], "kinds": sorted(e["kinds"]),
                        "references": e["references"], "referencing_instructions": len(e["from"]),
                        "executed": target in self.instr_count})
        return out

    def detail(self) -> list[dict[str, Any]]:
        """Per-address detail for the ignored artifact (carries opcode bytes)."""
        out = []
        for a in sorted(self.byte_kind):
            entry = {"address": addr(a), "rom_offset": mapping.rom_offset(a, self.size), "kind": self.byte_kind[a],
                     "count": self.byte_count[a], "modes": modes_list(self.byte_modes[a]), "first_frame": self.byte_first[a]}
            if a in self.instr_modes:
                entry["opcode"] = f"0x{self.rom[mapping.rom_offset(a, self.size)]:02x}"
                entry["mnemonic"] = TABLE[self.rom[mapping.rom_offset(a, self.size)]][0]
                entry["lengths"] = sorted(self.instr_lengths[a])
                entry["executions"] = self.instr_count[a]
            if a in self.byte_overlap:
                entry["overlap"] = True
            out.append(entry)
        return out


def per_frame_summary(series: list[int], start: int) -> dict[str, Any]:
    """Summary of a watched address's executions per frame (frames numbered from ``start``).

    ``once_per_frame_from`` is the first frame of the tail in which every frame, to the end, holds
    exactly one entry; it is reported only when no frame of the whole series holds more than one.
    A frame without an entry is legitimate (the race scenario disables NMIs while it loads, R-0006
    finding 11) and is listed in ``gaps_after_first``; a frame with two or more entries of an
    interrupt vector target is not, and voids the once-per-frame claim wherever it occurs."""
    first = next((i for i, n in enumerate(series) if n), None)
    once_from = None
    if all(n <= 1 for n in series):
        for i in range(len(series) - 1, -1, -1):
            if series[i] != 1:
                break
            once_from = i
    return {
        "frames_with_zero": sum(1 for n in series if n == 0),
        "frames_with_one": sum(1 for n in series if n == 1),
        "frames_with_more": sum(1 for n in series if n > 1),
        "first_frame": None if first is None else start + first,
        "once_per_frame_from": None if once_from is None else start + once_from,
        "gaps_after_first": [] if first is None else ranges_of_frames([start + i for i in range(first, len(series)) if series[i] == 0]),
    }


def ranges_of_frames(frames: list[int]) -> list[list[int]]:
    out: list[list[int]] = []
    for f in frames:
        if out and f == out[-1][1] + 1:
            out[-1][1] = f
        else:
            out.append([f, f])
    return out


def address_set_from_ranges(ranges: list[dict[str, Any]]) -> set[int]:
    out: set[int] = set()
    for r in ranges:
        start, end = parse_addr(r["start"]), parse_addr(r["end"])
        out.update(range(start, end + 1))
    return out


def ranges_from_addresses(addresses: set[int]) -> list[dict[str, str | int]]:
    out: list[dict[str, str | int]] = []
    run: list[int] = []
    for a in sorted(addresses):
        if run and a != run[-1] + 1:
            out.append({"start": addr(run[0]), "end": addr(run[-1]), "bytes": len(run)})
            run = []
        run.append(a)
    if run:
        out.append({"start": addr(run[0]), "end": addr(run[-1]), "bytes": len(run)})
    return out


def compare_maps(current: dict[str, Any], baseline: dict[str, Any]) -> dict[str, Any]:
    """What ``current`` executes that ``baseline`` does not (addresses, entry points)."""
    cur = address_set_from_ranges(current["ranges"])
    base = address_set_from_ranges(baseline["ranges"])
    new_addresses = cur - base
    cur_targets = {e["address"] for e in current["entry_points"]}
    base_targets = {e["address"] for e in baseline["entry_points"]}
    new_entries = [e for e in current["entry_points"] if e["address"] not in base_targets]
    return {
        "baseline_scenario": baseline["scenario_id"],
        "baseline_coverage_sha256": baseline["coverage"]["sha256"],
        "bytes_only_here": len(new_addresses),
        "bytes_only_in_baseline": len(base - cur),
        "bytes_in_both": len(cur & base),
        "entry_points_only_here": len(new_entries),
        "entry_points_only_in_baseline": len(base_targets - cur_targets),
        "new_ranges": ranges_from_addresses(new_addresses),
        "new_entry_points": new_entries,
    }


def build_map(coverage: dict[str, Any], rom: bytes, scenario_id: str, core: dict[str, Any], coverage_sha256: str,
              regeneration_command: str, header_offset: int = 0x7FC0) -> tuple[dict[str, Any], list[dict[str, Any]]]:
    """The tracked map document and the per-address detail list."""
    d = Derivation(coverage, rom, header_offset)
    doc = {
        "schema_version": MAP_SCHEMA_VERSION,
        "kind": "code_map",
        "scenario_id": scenario_id,
        "rom": {"sha256": coverage["rom"]["sha256"], "size": coverage["rom"]["size"]},
        "core": core,
        "coverage": {"sha256": coverage_sha256, "frames": coverage["frames"], "instructions": coverage["instructions"]["total"],
                     "max_frame_delta": coverage["instructions"]["max_frame_delta"], "ring_capacity": coverage["ring_capacity"],
                     "first_site": {"address": addr(coverage["first_site"][0]), "mode": mode_name(coverage["first_site"][1])} if coverage.get("first_site") else None,
                     **({"merged_runs": len(coverage["merged_from"])} if "merged_from" in coverage else {})},
        "regeneration_command": regeneration_command,
        "mapping_rule": MAPPING_RULE,
        "mode_notation": "E/N = emulation/native; m/M = 8/16-bit accumulator; x/X = 8/16-bit index (from the pre-instruction P and E)",
        "totals": d.totals(),
        "vectors": d.vector_table(),
        "banks": d.banks(),
        "ranges": d.ranges(),
        "multi_mode_addresses": d.multi_mode(),
        "entry_points": d.entry_points(),
        "non_rom_sites": d.non_rom_sites,
        "unknown_edges": d.unknown_edges,
        "static_references": d.static_references(),
    }
    return doc, d.detail()


def dump_map(doc: dict[str, Any]) -> str:
    """Deterministic JSON: one line per list element so the file diffs and stays readable."""
    import json

    lines = ["{"]
    keys = list(doc)
    for i, key in enumerate(keys):
        value = doc[key]
        comma = "," if i < len(keys) - 1 else ""
        if isinstance(value, list) and value and isinstance(value[0], dict):
            lines.append(f'  {json.dumps(key)}: [')
            for j, item in enumerate(value):
                lines.append("    " + json.dumps(item, sort_keys=True, separators=(",", ":")) + ("," if j < len(value) - 1 else ""))
            lines.append(f"  ]{comma}")
        else:
            lines.append(f'  {json.dumps(key)}: {json.dumps(value, sort_keys=True, separators=(",", ":"))}{comma}')
    lines.append("}")
    return "\n".join(lines) + "\n"


def summary_markdown(doc: dict[str, Any], comparison: dict[str, Any] | None = None) -> str:
    t = doc["totals"]
    size = t["rom_size"]
    lines = [
        f"# Observed code map: `{doc['scenario_id']}`",
        "",
        f"Derived from an instruction-coverage capture of the reference core ({doc['core'].get('name', 'bsnes')} "
        f"`{doc['core'].get('commit', '')[:12]}`, patch `{doc['core'].get('patch_sha256', '')[:12]}`) on ROM SHA-256 "
        f"`{doc['rom']['sha256'][:16]}…` ({size} bytes), frames {doc['coverage']['frames']['start']}–{doc['coverage']['frames']['end']}, "
        f"{doc['coverage']['instructions']:,} executed instructions (largest frame {doc['coverage']['max_frame_delta']:,}, ring "
        f"{doc['coverage']['ring_capacity']:,}). Coverage file SHA-256 `{doc['coverage']['sha256'][:16]}…`.",
        "",
        f"Regenerate: `{doc['regeneration_command']}`",
        "",
        "Only executed instructions and the header vectors classify bytes; nothing is inferred statically. "
        "Reads and writes are not tracked (no data-access map). Indexed and indirect operands are not resolved.",
        "",
        "## Totals",
        "",
        "| Quantity | Value |", "| --- | --- |",
        f"| Executed opcode bytes | {t['executed_opcode_bytes']:,} |",
        f"| Executed operand bytes | {t['executed_operand_bytes']:,} |",
        f"| Unclassified bytes | {t['unclassified_bytes']:,} |",
        f"| Sum (must equal the ROM size) | {t['executed_opcode_bytes'] + t['executed_operand_bytes'] + t['unclassified_bytes']:,} of {size:,} |",
        f"| Executed share of the ROM | {100 * (t['executed_opcode_bytes'] + t['executed_operand_bytes']) / size:.3f} % |",
        f"| Bytes both opcode and operand (counted as opcode) | {t['overlapping_bytes']} |",
        f"| Distinct instruction addresses (24-bit) | {t['instruction_addresses']:,} |",
        f"| Distinct sites (pc, mode, data bank) | {t['sites']:,} |",
        f"| Sites outside ROM | {t['non_rom_sites']} |",
        f"| Addresses executed in more than one mode | {t['multi_mode_addresses']} |",
        f"| Entry points (non-sequential targets) | {t['entry_points']:,} |",
        f"| Non-sequential steps / sequential steps | {t['edge_steps']:,} / {t['sequential_steps']:,} |",
        f"| Unknown edges | {t['unknown_edges']} |",
        f"| Statically referenced addresses | {t['static_references']:,} |",
        "",
        "## Mode mix",
        "",
        "| Mode | Instructions executed | Distinct sites |", "| --- | --- | --- |",
    ]
    for mode, n in t["instructions_by_mode"].items():
        lines.append(f"| {mode} | {n:,} | {t['sites_by_mode'].get(mode, 0):,} |")
    lines += ["", "## Executed bytes per bank", "", "| Bank | Executed bytes | Opcode | Operand | Instruction addresses | Executions |", "| --- | --- | --- | --- | --- | --- |"]
    for b in doc["banks"]:
        lines.append(f"| {b['bank']} | {b['bytes']:,} | {b['opcode_bytes']:,} | {b['operand_bytes']:,} | {b['instructions']:,} | {b['executions']:,} |")
    lines += ["", "## Edges by kind", "", "| Kind | Steps |", "| --- | --- |"]
    for k, n in t["edges_by_kind"].items():
        lines.append(f"| {k} | {n:,} |")
    lines += ["", "## Vectors", "", "| Vector | Value | Executed | Count | First frame | Modes | Per frame |", "| --- | --- | --- | --- | --- | --- | --- |"]
    for v in doc["vectors"]:
        pf = v.get("per_frame")
        per = "-" if pf is None else (f"0/1/2+ in {pf['frames_with_zero']}/{pf['frames_with_one']}/{pf['frames_with_more']} frames; exactly one per frame from {pf['once_per_frame_from']}"
                                       + (f"; no entry in frames {pf['gaps_after_first']}" if pf["gaps_after_first"] else ""))
        lines.append(f"| {v['name']} | {v['target']} | {'yes' if v['executed'] else 'no'} | {v['count']} | {v['first_frame'] if v['first_frame'] is not None else '-'} | {', '.join(v['modes']) or '-'} | {per} |")
    lines += ["", f"## Executed ranges ({len(doc['ranges'])})", "",
              "Maximal runs of consecutive executed byte addresses; `modes` is the union over instructions starting in the run.", "",
              "| Start | End | Bytes | ROM offset | Instructions | Executions | Modes | First frame |", "| --- | --- | --- | --- | --- | --- | --- | --- |"]
    for r in doc["ranges"]:
        lines.append(f"| {r['start']} | {r['end']} | {r['bytes']} | 0x{r['rom_offset']:06X} | {r['instructions']} | {r['executions']:,} | {', '.join(r['modes'])} | {r['first_frame']} |")
    if doc["multi_mode_addresses"]:
        lines += ["", "## Addresses executed in more than one mode", "", "| Address | Modes | Lengths | Count |", "| --- | --- | --- | --- |"]
        for m in doc["multi_mode_addresses"]:
            lines.append(f"| {m['address']} | {', '.join(m['modes'])} | {m['lengths']} | {m['count']:,} |")
    if doc["non_rom_sites"]:
        lines += ["", "## Sites outside ROM", "", "| Address | Region | Mode | Count | First frame |", "| --- | --- | --- | --- | --- |"]
        for s in doc["non_rom_sites"]:
            lines.append(f"| {s['address']} | {s['region']} | {s['mode']} | {s['count']:,} | {s['first_frame']} |")
    if doc["unknown_edges"]:
        lines += ["", f"## Unknown edges ({len(doc['unknown_edges'])})", "",
                  "Steps whose predecessor could not be decoded (outside ROM) or is not a control-flow instruction and whose target is not a vector target.", "",
                  "| From | Region | Decoded | To | Count |", "| --- | --- | --- | --- | --- |"]
        for e in doc["unknown_edges"][:64]:
            lines.append(f"| {e['from']} | {e['from_region']} | {'yes' if e['from_decoded'] else 'no'} | {e['to']} | {e['count']} |")
    if comparison is not None:
        lines += ["", f"## Compared to `{comparison['baseline_scenario']}`", "",
                  f"Bytes executed only here: {comparison['bytes_only_here']:,}; only in the baseline: {comparison['bytes_only_in_baseline']:,}; "
                  f"in both: {comparison['bytes_in_both']:,}. Entry points only here: {comparison['entry_points_only_here']:,}; "
                  f"only in the baseline: {comparison['entry_points_only_in_baseline']:,}.", "",
                  f"### Ranges executed only here ({len(comparison['new_ranges'])})", "", "| Start | End | Bytes |", "| --- | --- | --- |"]
        for r in comparison["new_ranges"]:
            lines.append(f"| {r['start']} | {r['end']} | {r['bytes']} |")
        new = comparison["new_entry_points"]
        called = [e for e in new if set(e["kinds"]) - {"branch"}]
        lines += ["", f"### Entry points only here ({len(new)}; {len(called)} reached by a call, jump, return or interrupt, {len(new) - len(called)} only by branches)", "",
                  "Branch-only targets lie inside the ranges above and are listed in the JSON map only.", "",
                  "| Address | Kinds | Modes | Count | First frame |", "| --- | --- | --- | --- | --- |"]
        for e in called:
            lines.append(f"| {e['address']} | {e['kinds']} | {', '.join(e['modes'])} | {e['count']:,} | {e['first_frame']} |")
    lines += ["", "## Limits", "",
              "- Coverage is of this scenario's frames only; an unexecuted byte is unclassified, not data.",
              "- Opcode and operand classification uses the observed M/X flags per site; a byte executed under two decodings is listed under multi-mode addresses.",
              "- Interrupt entries are not traced instructions (R-0002 finding 7); they are recognised by the target being an NMI/IRQ vector target.",
              "- Static references cover absolute (with the observed data bank) and long operands only.",
              "- Per-address detail (with opcode bytes) is an ignored artifact regenerated by the command above; tracked files carry no ROM bytes.",
              ""]
    return "\n".join(lines)
