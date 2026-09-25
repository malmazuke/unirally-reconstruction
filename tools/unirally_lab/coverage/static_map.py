"""Static, annotated code map of the code banks $80-$83 (STATIC-CODE-MAP).

The analysis reads the ROM and the tracked observed maps (``docs/map/*.map.json``,
map schema 1) and assigns every byte of ROM offsets $00000-$1FFFF (banks
$80-$83; banks $00-$03 mirror them) exactly one class:

* ``observed``: executed in a tracked map. With the raw coverage behind the
  maps the instructions are the recorded sites; without it, the instructions
  that every tiling of a range shares (the range tiled under its recorded
  modes, without propagation, into exactly its recorded instruction count
  with every recorded entry point on a boundary). The tiling is also the
  independent cross-check of the sites.
* ``inferred``: decoded statically by transactional descent from the observed
  code, the vectors and jump tables. Gap-sweep decodings are listed as
  candidates and stay ``unknown``.
* ``data``: named by an operand (a recorded static reference, a long operand
  of inferred code) or a jump-table walk, and never decoded as code.
* ``unknown``: everything else.

Processor state is a set of candidate 3-bit modes (``opcodes.MODE_*``). REP and
SEP narrow it; PLP, RTI and XCE widen it to all native modes. An instruction
whose length differs between the candidates is not decoded: it is recorded as
a mode ambiguity and descent stops there. An address reached along several
paths keeps the modes of the first path that decoded it, so the listing's
mode column can understate the modes it is reached in. Two assumptions are named and
counted rather than hidden: a JSR/JSL returns to the next instruction, and it
returns in the mode it was called in.

The listing is a reading aid and a hypothesis generator (D-0008); it is not
gameplay evidence. Tracked outputs carry addresses, classes and counts only,
never ROM bytes, opcode bytes or mnemonics.
"""

from __future__ import annotations

import json
import re
from collections import Counter, deque
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Iterable

from . import mapping, native_symbols
from .opcodes import MODE_E, MODE_M, MODE_X, TABLE, instruction_length, mode_name

SPAN = 0x20000                     # ROM offsets of banks $80-$83
BANKS = (0x80, 0x81, 0x82, 0x83)
CLASSES = ("observed", "inferred", "data", "unknown")
NATIVE_MODES = frozenset({0, MODE_M, MODE_X, MODE_M | MODE_X})

BRANCHES = frozenset({0x10, 0x30, 0x50, 0x70, 0x90, 0xB0, 0xD0, 0xF0})
# Instructions after which execution does not fall through to the next address.
TERMINAL = frozenset({0x60, 0x6B, 0x40, 0x4C, 0x5C, 0x6C, 0x7C, 0xDC, 0x80, 0x82, 0xDB})
# Opcodes descent does not decode: software interrupts, stop, reserved WDM.
IMPLAUSIBLE = frozenset({0x00, 0x02, 0xDB, 0x42})
CALLS = frozenset({0x20, 0x22, 0xFC})
JUMP_TABLES = frozenset({0x7C, 0xFC})   # JMP (abs,X), JSR (abs,X): table in the program bank
MODE_RESET = frozenset({0x28, 0x40, 0xFB})  # PLP, RTI, XCE
MAX_TABLE = 128
GAP_MIN = 4                         # instructions a gap-sweep routine must hold
CITED = re.compile(r"\$(8[0-3]|0[0-3]):([0-9A-Fa-f]{4})")


# ------------------------------------------------------------- addressing


def offset_of(address: int) -> int | None:
    """ROM offset in the code span for a 24-bit address, else None."""
    off = mapping.rom_offset(address, 1 << 21)
    return off if off is not None and off < SPAN else None


def addr(offset: int) -> str:
    """Canonical $80-$83 name of a code-span offset."""
    return f"${0x80 + (offset >> 15):02X}:{0x8000 | (offset & 0x7FFF):04X}"


def parse(text: str) -> int:
    bank, rest = text.lstrip("$").split(":")
    return (int(bank, 16) << 16) | int(rest, 16)


def modes_of(names: Iterable[str]) -> frozenset[int]:
    out = set()
    for n in names:
        out.add((MODE_E if n[0] == "E" else 0) | (MODE_M if n[1] == "m" else 0) | (MODE_X if n[2] == "x" else 0))
    return frozenset(out)


def names(modes: Iterable[int]) -> list[str]:
    return sorted(mode_name(m) for m in modes)


def lengths(opcode: int, modes: frozenset[int]) -> set[int]:
    return {instruction_length(opcode, m) for m in modes}


def after(opcode: int, operand: int, modes: frozenset[int]) -> frozenset[int]:
    """Candidate modes after executing ``opcode`` in ``modes``."""
    if opcode in MODE_RESET:
        return NATIVE_MODES
    if opcode in (0xC2, 0xE2):  # REP, SEP
        bits = (MODE_M if operand & 0x20 else 0) | (MODE_X if operand & 0x10 else 0)
        return frozenset(m if m & MODE_E else (m & ~bits if opcode == 0xC2 else m | bits) for m in modes)
    return modes


# --------------------------------------------------------------- analysis


@dataclass
class Instruction:
    length: int
    modes: frozenset[int]
    source: str               # observed | descent | table | gap
    via: str = ""             # the seed kind for inferred code


@dataclass
class Analysis:
    rom: bytes
    maps: list[dict[str, Any]]
    ins: dict[int, Instruction] = field(default_factory=dict)
    owner: dict[int, int] = field(default_factory=dict)          # byte offset -> instruction start
    data: dict[int, set[str]] = field(default_factory=dict)      # byte offset -> reasons
    ambiguities: dict[int, dict[str, Any]] = field(default_factory=dict)
    conflicts: list[dict[str, Any]] = field(default_factory=list)
    stops: Counter = field(default_factory=Counter)
    data_into_code: set[int] = field(default_factory=set)
    unresolved_abs: int = 0
    callers: dict[int, set[tuple[int, str]]] = field(default_factory=dict)
    entries: dict[int, Counter] = field(default_factory=dict)    # observed entry kinds, summed over maps
    entry_modes: dict[int, frozenset[int]] = field(default_factory=dict)
    vector_targets: dict[int, list[str]] = field(default_factory=dict)
    tables: list[dict[str, Any]] = field(default_factory=list)
    tiling: dict[str, Any] = field(default_factory=dict)
    sites: dict[int, dict[int, int]] | None = None               # offset -> {mode: length}, from raw coverage
    assumptions: Counter = field(default_factory=Counter)
    rejected: list[dict[str, Any]] = field(default_factory=list)
    candidates: dict[int, Instruction] = field(default_factory=dict)   # gap-sweep decodings; their bytes stay unknown
    candidate_owner: dict[int, int] = field(default_factory=dict)

    # ---------------------------------------------------------- helpers

    def byte(self, off: int) -> int:
        return self.rom[off]

    def operand(self, off: int, length: int) -> int:
        return int.from_bytes(self.rom[off + 1:off + length], "little")

    def bank_end(self, off: int) -> int:
        return (off | 0x7FFF) + 1

    def fits(self, off: int, length: int) -> str | None:
        """Why an instruction of ``length`` at ``off`` cannot be placed, or None."""
        if off + length > self.bank_end(off):
            return "crosses a bank end"
        for b in range(off, off + length):
            if b in self.owner:
                return "overlaps decoded code"
        return None

    def place(self, off: int, inst: Instruction) -> None:
        self.ins[off] = inst
        for b in range(off, off + inst.length):
            self.owner[b] = off

    # --------------------------------------------------- observed tiling

    def load_sites(self, coverages: list[dict[str, Any]]) -> None:
        """Observed instruction starts and modes from the raw coverage files behind the tracked maps."""
        self.sites = {}
        for cov in coverages:
            for pc, mode, _bank, _count, _first in cov["sites"]:
                off = offset_of(pc) if mapping.classify(pc) == mapping.REGION_ROM else None
                if off is not None:
                    self.sites.setdefault(off, {})[mode] = instruction_length(self.byte(off), mode)

    def tile_observed(self) -> None:
        """Place the observed instructions, and check them against every tracked range's tiling.

        With raw sites, each site is placed at its recorded length under its recorded mode and
        the tiling is the agreement check: every instruction that all tilings of a range share
        must be a site of that length. Without them, the shared instructions are what is placed.
        """
        per_map = []
        placed: dict[int, tuple[int, set[int]]] = {}
        for doc in self.maps:
            entry = {}
            for e in doc["entry_points"]:
                off = offset_of(parse(e["address"]))
                if off is not None and e.get("region") == "rom":
                    entry[off] = modes_of(e["modes"])
            multi = {}
            for m in doc["multi_mode_addresses"]:
                off = offset_of(parse(m["address"]))
                if off is not None:
                    multi[off] = set(m["lengths"])
            unique = ambiguous = failed = unresolved = checked = disagree = 0
            for r in doc["ranges"]:
                start = offset_of(parse(r["start"]))
                if start is None:
                    continue
                end = offset_of(parse(r["end"])) + 1
                result = self._tile(start, end, r["instructions"], modes_of(r["modes"]), entry, multi)
                if result is None:
                    failed += 1
                    self.conflicts.append({"address": addr(start), "map": doc["scenario_id"], "reason": "observed range has no tiling"})
                    continue
                certain, tilings = result
                if tilings > 1:
                    ambiguous += 1
                    unresolved += r["instructions"] - len(certain)
                else:
                    unique += 1
                for off, length, sub in certain:
                    if self.sites is not None:
                        checked += 1
                        if length not in self.sites.get(off, {}).values():
                            disagree += 1
                            self.conflicts.append({"address": addr(off), "map": doc["scenario_id"],
                                                   "reason": "every tiling of the range holds an instruction here that no site matches"})
                        continue
                    prev = placed.get(off)
                    if prev is not None and prev[0] != length:
                        self.conflicts.append({"address": addr(off), "map": doc["scenario_id"], "reason": "maps disagree on length"})
                    placed.setdefault(off, (length, set()))[1].update(sub)
            per_map.append({"scenario_id": doc["scenario_id"], "ranges": len(doc["ranges"]), "one_tiling": unique,
                            "several_tilings": ambiguous, "instructions_not_fixed_by_tiling": unresolved, "no_tiling": failed,
                            "shared_instructions_checked_against_sites": checked, "disagreements": disagree})
        if self.sites is not None:
            placed = {}
            for off, by_mode in self.sites.items():
                for length in set(by_mode.values()):
                    if off in placed:
                        self.conflicts.append({"address": addr(off), "reason": "one site has two lengths under different modes"})
                        continue
                    placed[off] = (length, {m for m, l in by_mode.items() if l == length})
        for off in sorted(placed):
            length, ms = placed[off]
            if self.fits(off, length) is None:
                self.place(off, Instruction(length, frozenset(ms), "observed"))
            else:
                self.conflicts.append({"address": addr(off), "reason": "observed instructions overlap"})
        self.tiling = {"source": "raw coverage sites" if self.sites is not None else "range tiling", "maps": per_map}
        for doc in self.maps:
            for e in doc["entry_points"]:
                off = offset_of(parse(e["address"]))
                if off is None or e.get("region") != "rom":
                    continue
                self.entries.setdefault(off, Counter()).update(e["kinds"])
                self.entry_modes[off] = self.entry_modes.get(off, frozenset()) | modes_of(e["modes"])

    def _tile(self, start: int, end: int, count: int, range_modes: frozenset[int], entry: dict[int, frozenset[int]],
              multi: dict[int, set[int]]):
        """The instructions every tiling of one range shares, or None if the range cannot be tiled.

        A tiling covers ``start``..``end`` with exactly ``count`` instructions whose lengths are
        possible under the range's recorded modes (the recorded lengths at a multi-mode address),
        with every recorded entry point on a boundary. Modes are not propagated here: REP/SEP
        propagation can exclude the true tiling when a range joins code entered in several
        modes, so an instruction is kept only when every tiling contains it.
        """
        entries = {e for e in entry if start < e < end}

        def choices(pos: int) -> list[tuple[int, frozenset[int]]]:
            op = self.byte(pos)
            out = []
            for length in sorted(multi.get(pos) or lengths(op, range_modes)):
                if pos + length > end or any(e in entries for e in range(pos + 1, pos + length)):
                    continue
                sub = frozenset(m for m in range_modes if instruction_length(op, m) == length) or range_modes
                out.append((length, sub))
            return out

        fwd: dict[tuple[int, int], int] = {(start, 0): 1}
        for pos in range(start, end):
            for n in range(count):
                c = fwd.get((pos, n))
                if c:
                    for length, _ in choices(pos):
                        key = (pos + length, n + 1)
                        fwd[key] = fwd.get(key, 0) + c
        total = fwd.get((end, count), 0)
        if total == 0:
            return None
        back: dict[tuple[int, int], int] = {(end, count): 1}
        for pos in range(end - 1, start - 1, -1):
            for n in range(count - 1, -1, -1):
                c = sum(back.get((pos + length, n + 1), 0) for length, _ in choices(pos))
                if c:
                    back[(pos, n)] = c
        certain = []
        for pos in range(start, end):
            for length, sub in choices(pos):
                through = sum(fwd.get((pos, n), 0) * back.get((pos + length, n + 1), 0) for n in range(count))
                if through == total:
                    certain.append((pos, length, sub))
        return certain, total

    # --------------------------------------------------------- descent

    def flow(self, off: int, length: int, modes: frozenset[int]) -> tuple[list[tuple[int, frozenset[int]]], list[tuple[int, frozenset[int], str]]]:
        """Successors of an instruction: (routine-internal targets, other routines' entries with their kind).

        Fall-through, branches and JMP stay in the routine; JSR/JSL/JML targets are other routines and
        are tried separately. Targets outside the code span are dropped.
        """
        op = self.byte(off)
        operand = self.operand(off, length)
        out_modes = after(op, operand, modes)
        bank = 0x800000 | ((off >> 15) << 16)
        local: list[tuple[int, frozenset[int]]] = []
        calls: list[tuple[int, frozenset[int], str]] = []
        if op not in TERMINAL and off + length < self.bank_end(off):   # the PC wraps within its bank, out of ROM
            local.append((off + length, out_modes))
        here = 0x8000 | (off & 0x7FFF)
        target = None
        if op in BRANCHES or op == 0x80:
            target = bank | (here + length + (operand - 256 if operand & 0x80 else operand)) & 0xFFFF
        elif op == 0x82:
            target = bank | (here + length + (operand - 65536 if operand & 0x8000 else operand)) & 0xFFFF
        elif op == 0x4C:
            target = bank | operand
        if target is not None:
            t = offset_of(target)
            if t is not None:
                local.append((t, out_modes))
        kind = {0x20: "JSR", 0x22: "JSL", 0x5C: "JML"}.get(op)
        if kind:
            t = offset_of(bank | operand if op == 0x20 else operand)
            if t is not None:
                calls.append((t, out_modes, kind))
        return local, calls

    def implausible(self, off: int, length: int, allow_rti: bool) -> str | None:
        op = self.byte(off)
        if op in IMPLAUSIBLE or (op == 0x40 and not allow_rti):
            return "implausible opcode"
        if op in (0x20, 0x4C, 0xFC, 0x7C) and self.operand(off, length) < 0x8000:
            return "JSR/JMP to a non-ROM address in the program bank"
        return None

    def trial(self, seed: int, modes: frozenset[int], allow_rti: bool = False):
        """Decode one routine's internal flow from ``seed`` without committing it.

        Returns (instructions, calls, ambiguities, rejection). A trial is rejected whole when any
        instruction it reaches is implausible, overlaps decoded code, or is entered mid-instruction;
        an instruction whose length depends on an unknown M/X bit ends that path as an ambiguity.
        """
        local: dict[int, Instruction] = {}
        owner: dict[int, int] = {}
        calls: list[tuple[int, frozenset[int], str, int]] = []
        ambiguous: dict[int, frozenset[int]] = {}
        work = deque([(seed, modes)])
        while work:
            off, ms = work.popleft()
            if off in self.ins or off in local:
                continue
            if off in self.owner or off in owner:
                return local, calls, ambiguous, "enters an instruction mid-way"
            if off in self.data:
                return local, calls, ambiguous, "runs into data"
            op = self.byte(off)
            ls = lengths(op, ms)
            if len(ls) > 1:
                ambiguous[off] = ms
                continue
            length = ls.pop()
            why = self.implausible(off, length, allow_rti)
            if why is None and off + length > self.bank_end(off):
                why = "crosses a bank end"
            if why is None and any(b in self.owner or b in owner for b in range(off, off + length)):
                why = "overlaps decoded code"
            if why:
                return local, calls, ambiguous, why
            local[off] = Instruction(length, ms, "")
            for b in range(off, off + length):
                owner[b] = off
            internal, out = self.flow(off, length, ms)
            work.extend(internal)
            calls.extend((t, m, k, off) for t, m, k in out)
        return local, calls, ambiguous, None

    def commit(self, found: dict[int, Instruction], ambiguous: dict[int, frozenset[int]], source: str, via: str) -> None:
        for off in sorted(found):
            inst = found[off]
            self.place(off, Instruction(inst.length, inst.modes, source, via))
        for off, ms in ambiguous.items():
            if off not in self.owner:
                self.ambiguities.setdefault(off, {"address": addr(off), "modes": names(ms), "reason": f"{via}: length depends on an unknown M/X bit"})

    def record_calls(self, off: int) -> list[tuple[int, frozenset[int], str]]:
        inst = self.ins[off]
        internal, calls = self.flow(off, inst.length, inst.modes)
        op = self.byte(off)
        if op == 0x4C:
            for t, _ in internal[-1:]:
                self.callers.setdefault(t, set()).add((off, "JMP"))
        for t, _, kind in calls:
            self.callers.setdefault(t, set()).add((off, kind))
        if op in CALLS and op != 0xFC:
            self.assumptions["call returns in the calling mode"] += 1
        return calls

    def descend(self, seeds: Iterable[tuple[int, frozenset[int], str]], source: str) -> None:
        work = deque(seeds)
        while work:
            off, modes, via = work.popleft()
            if off in self.ins:
                continue
            found, calls, ambiguous, why = self.trial(off, modes, allow_rti=via == "vector")
            if why:
                self.stops[why] += 1
                self.rejected.append({"address": addr(off), "seed": via, "reason": why})
                continue
            self.commit(found, ambiguous, source, via)
            for o in sorted(found):
                self.record_calls(o)
            work.extend((t, m, k) for t, m, k, _ in calls)

    def observed_seeds(self) -> list[tuple[int, frozenset[int], str]]:
        seeds = []
        for off in sorted(self.ins):
            inst = self.ins[off]
            internal, _ = self.flow(off, inst.length, inst.modes)
            seeds.extend((t, m, "unexecuted path") for t, m in internal if t not in self.ins)
            seeds.extend((t, m, k) for t, m, k in self.record_calls(off) if t not in self.ins)
        return seeds

    def vector_seeds(self) -> list[tuple[int, frozenset[int], str]]:
        seeds = []
        for v in self.maps[0]["vectors"]:
            t = offset_of(parse(v["target"]))
            if t is None or v["value"] in (0x0000, 0xFFFF):
                continue
            self.vector_targets.setdefault(t, []).append(v["name"])
            modes = frozenset({MODE_E | MODE_M | MODE_X}) if v["group"] == "emulation" else NATIVE_MODES
            seeds.append((t, modes, "vector"))
        return seeds

    def walk_tables(self) -> list[tuple[int, frozenset[int], str]]:
        seeds = []
        for off in sorted(self.ins):
            op = self.byte(off)
            if op not in JUMP_TABLES:
                continue
            base = self.operand(off, 3)
            if base < 0x8000:
                continue
            table = (off & ~0x7FFF) | (base & 0x7FFF)
            if any(t["table"] == addr(table) for t in self.tables):
                continue
            n = 0
            while n < MAX_TABLE:
                e = table + 2 * n
                if e + 2 > self.bank_end(table) or e in self.owner or e + 1 in self.owner:
                    break
                target = int.from_bytes(self.rom[e:e + 2], "little")
                if target == 0x0000 and n == 0:   # a placeholder index 0 (as at $81:82F5): table bytes, no target
                    for b in (e, e + 1):
                        self.data.setdefault(b, set()).add("jump table")
                    n += 1
                    continue
                if target < 0x8000:
                    break
                t = (off & ~0x7FFF) | (target & 0x7FFF)
                if t in self.owner and t not in self.ins or t in self.data or self.byte(t) in IMPLAUSIBLE:
                    break
                for b in (e, e + 1):
                    self.data.setdefault(b, set()).add("jump table")
                seeds.append((t, self.ins[off].modes, "jump table"))
                self.callers.setdefault(t, set()).add((off, "table"))
                n += 1
            self.tables.append({"table": addr(table), "dispatch": addr(off), "entries": n})
        return seeds

    # -------------------------------------------------------- gap sweep

    def gaps(self) -> list[int]:
        """Start of every maximal run of unclassified bytes (a run does not cross a bank end)."""
        out = []
        prev_free = False
        for off in range(SPAN):
            free = off not in self.owner and off not in self.data and off not in self.candidate_owner
            if free and (not prev_free or off & 0x7FFF == 0):
                out.append(off)
            prev_free = free
        return out

    def sweep(self) -> int:
        """List gap starts that every native mode decoding them plausibly decodes as the same routine.

        The trial must reach at least ``GAP_MIN`` instructions including one that ends the flow.
        These are candidates only: their bytes stay ``unknown``, because the sweep also accepts
        pointer and value tables (R-0045), and they seed nothing. Modes that decode a gap start
        to different routines are recorded as an ambiguity.
        """
        added = 0
        for start in self.gaps():
            if start in self.candidate_owner:
                continue
            shapes = {}
            for m in sorted(NATIVE_MODES):
                found, _calls, ambiguous, why = self.trial(start, frozenset({m}))
                if why or ambiguous or len(found) < GAP_MIN or not any(self.byte(o) in TERMINAL for o in found):
                    continue
                if any(b in self.candidate_owner for o, i in found.items() for b in range(o, o + i.length)):
                    continue
                shapes[m] = found
            if not shapes:
                continue
            if len({tuple((o, f[o].length) for o in sorted(f)) for f in shapes.values()}) > 1:
                self.ambiguities.setdefault(start, {"address": addr(start), "modes": names(shapes),
                                                    "reason": "gap sweep: decodes differently under different modes"})
                continue
            found = next(iter(shapes.values()))
            for o, i in found.items():
                self.candidates[o] = Instruction(i.length, frozenset(m for f in shapes.values() for m in f[o].modes), "gap", "gap sweep")
                for b in range(o, o + i.length):
                    self.candidate_owner[b] = o
            added += 1
        return added

    # ------------------------------------------------------------- data

    def mark_references(self) -> None:
        for doc in self.maps:
            for ref in doc["static_references"]:
                if "data" not in ref["kinds"] or ref.get("region") != "rom":
                    continue
                off = offset_of(parse(ref["address"]))
                if off is not None:
                    self.mark_data(off, "observed reference")
        for off in sorted(self.ins):
            if self.ins[off].source == "observed":
                continue
            op = self.byte(off)
            mode = TABLE[op][1]
            if mode in ("long", "longx") and op not in (0x22, 0x5C):
                t = offset_of(self.operand(off, 4))
                if t is not None:
                    self.mark_data(t, "long operand")
            elif mode in ("abs", "absx", "absy") and op not in (0x20, 0x4C, 0xF4):
                self.unresolved_abs += 1

    def mark_data(self, off: int, reason: str) -> None:
        if off in self.owner:
            self.data_into_code.add(off)
            return
        self.data.setdefault(off, set()).add(reason)

    # ----------------------------------------------------------- driver

    def run(self) -> "Analysis":
        self.tile_observed()
        self.descend(self.observed_seeds(), "descent")
        self.descend(self.vector_seeds(), "descent")
        while True:
            seeds = self.walk_tables()
            if not seeds:
                break
            self.descend(seeds, "table")
        self.mark_references()
        self.sweep()
        return self

    # ---------------------------------------------------------- results

    def classes(self) -> list[str]:
        out = []
        for off in range(SPAN):
            start = self.owner.get(off)
            if start is not None:
                out.append("observed" if self.ins[start].source == "observed" else "inferred")
            elif off in self.data:
                out.append("data")
            else:
                out.append("unknown")
        return out


def load_maps(paths: Iterable[Path]) -> list[dict[str, Any]]:
    return [json.loads(p.read_text(encoding="utf-8")) for p in sorted(paths)]


def check_sites(a: Analysis, coverage: dict[str, Any]) -> dict[str, Any]:
    """Every ROM site of a raw coverage file must be a decoded instruction start of the same length under its mode."""
    checked = 0
    disagreements = []
    outside = 0
    for pc, mode, _bank, _count, _first in coverage["sites"]:
        if mapping.classify(pc) != mapping.REGION_ROM:
            continue
        off = offset_of(pc)
        if off is None:
            outside += 1
            continue
        checked += 1
        inst = a.ins.get(off)
        expect = instruction_length(a.byte(off), mode)
        if inst is None or inst.source != "observed" or inst.length != expect:
            disagreements.append({"address": addr(off), "mode": mode_name(mode), "expected_length": expect,
                                  "decoded_length": inst.length if inst else None,
                                  "decoded_source": inst.source if inst else ("inside instruction" if off in a.owner else "not decoded")})
    return {"sites_checked": checked, "sites_outside_code_span": outside, "disagreements": len(disagreements),
            "first_disagreements": disagreements[:50]}


# ------------------------------------------------------------ routines


def routines(a: Analysis, cls: list[str]) -> list[dict[str, Any]]:
    starts = set(a.callers) | set(a.vector_targets)
    starts |= {o for o, k in a.entries.items() if k.keys() & {"JSR", "JSL", "interrupt entry"}}
    starts = sorted(s for s in starts if s in a.ins)
    out = []
    for i, s in enumerate(starts):
        limit = starts[i + 1] if i + 1 < len(starts) else SPAN
        limit = min(limit, a.bank_end(s))
        end = s
        while end < limit and end in a.owner:
            end += 1
        callers = sorted(a.callers.get(s, ()))
        kinds = Counter(k for _, k in callers)
        out.append({
            "start": addr(s), "end": addr(end - 1), "bytes": end - s, "class": cls[s],
            "mode_at_entry": names(a.entry_modes.get(s) or a.ins[s].modes),
            "static_entries": dict(sorted(kinds.items())),
            "observed_entries": dict(sorted(a.entries.get(s, Counter()).items())),
            "vectors": sorted(a.vector_targets.get(s, [])),
            "callers": [addr(o) for o, _ in callers],
        })
    return out


# --------------------------------------------------------- cited addresses


def cited_addresses(root: Path) -> dict[int, list[str]]:
    """ROM code-span offset -> records citing it (the task record's inventory rule)."""
    globs = ("docs/research/*.md", "tasks/*.md", "docs/inventory/*.md", "docs/content/*.md")
    out: dict[int, set[str]] = {}
    for g in globs:
        for path in sorted(root.glob(g)):
            text = path.read_text(encoding="utf-8")
            for m in CITED.finditer(text):
                address = (int(m.group(1), 16) << 16) | int(m.group(2), 16)
                off = offset_of(address)
                if off is not None:
                    out.setdefault(off, set()).add(path.relative_to(root).as_posix())
    return {o: sorted(r) for o, r in sorted(out.items())}


def labels(a: Analysis, cls: list[str], cited: dict[int, list[str]], routine_starts: set[int]) -> list[dict[str, Any]]:
    out = []
    for off, records in cited.items():
        if off in routine_starts:
            kind = "sub"
        elif off in a.ins:
            kind = "loc"
        elif off in a.owner:
            kind = "mid"
        elif off in a.data:
            kind = "dat"
        else:
            kind = "unk"
        where = {"sub": "routine start", "loc": "instruction start", "mid": f"inside the instruction at {addr(a.owner.get(off, off))}",
                 "dat": "data", "unk": "unknown"}[kind]
        out.append({"address": addr(off), "label": f"{kind}_{addr(off)[1:].replace(':', '')}", "class": cls[off],
                    "comment": where, "source_records": records})
    return out


# ------------------------------------------------------------- listing


def operand_text(a: Analysis, off: int, length: int) -> str:
    op = a.byte(off)
    mode = TABLE[op][1]
    v = a.operand(off, length)
    w = 2 * (length - 1)
    bank = 0x80 + (off >> 15)
    if mode == "imp":
        return ""
    if mode in ("imm8", "immM", "immX"):
        return f"#${v:0{w}X}"
    if mode == "rel8":
        t = ((0x8000 | (off & 0x7FFF)) + 2 + (v - 256 if v & 0x80 else v)) & 0xFFFF
        return f"${bank:02X}:{t:04X}"
    if mode == "rel16":
        t = ((0x8000 | (off & 0x7FFF)) + 3 + (v - 65536 if v & 0x8000 else v)) & 0xFFFF
        return f"${bank:02X}:{t:04X}"
    if mode == "blk":
        return f"${v & 0xFF:02X},${v >> 8:02X}"
    forms = {"dp": "${}", "dpx": "${},X", "dpy": "${},Y", "idp": "(${})", "idpx": "(${},X)", "idpy": "(${}),Y",
             "ildp": "[${}]", "ildpy": "[${}],Y", "sr": "${},S", "isry": "(${},S),Y", "abs": "${}", "absx": "${},X",
             "absy": "${},Y", "iabs": "(${})", "iabsx": "(${},X)", "ilabs": "[${}]", "long": "${}", "longx": "${},X"}
    return forms[mode].format(f"{v:0{w}X}")


def listing(a: Analysis, cls: list[str], bank: int, names_by_offset: dict[int, str], xrefs: dict[int, list[str]],
            native: dict[int, list[str]] | None = None) -> str:
    """The ignored per-bank listing: address, bytes, mnemonic, operand, mode, class, label, cross-references."""
    lines = [f"; bank ${0x80 + bank:02X} (ROM ${bank << 15:05X}-${(bank << 15) | 0x7FFF:05X}); ignored artifact, carries ROM bytes",
             "; address  bytes        instruction              modes          class/source    ; label, references"]
    off = bank << 15
    end = off + 0x8000
    while off < end:
        if off in a.ins or off in a.candidates:
            inst = a.ins.get(off) or a.candidates[off]
            raw = " ".join(f"{b:02X}" for b in a.rom[off:off + inst.length])
            text = f"{TABLE[a.byte(off)][0]} {operand_text(a, off, inst.length)}".rstrip()
            source = {"observed": "observed", "gap": "unknown/gap-cand"}.get(inst.source, f"inferred/{inst.source}")
            note = []
            if off in names_by_offset:
                note.append(names_by_offset[off] + ":")
            if native and off in native:
                note.append("native " + ", ".join(n.removeprefix("src/core/") for n in native[off]) + ";")
            if off in xrefs:
                note.append("from " + ", ".join(xrefs[off][:8]) + (f" (+{len(xrefs[off]) - 8})" if len(xrefs[off]) > 8 else ""))
            if off in a.ambiguities:
                note.append("ambiguity: " + a.ambiguities[off]["reason"])
            lines.append(f"{addr(off)}  {raw:<12} {text:<24} {','.join(names(inst.modes)):<14} {source:<15} ; {' '.join(note)}".rstrip(" ;"))
            off += inst.length
            continue
        run = off
        kind = cls[off]
        while run < end and run not in a.ins and run not in a.candidates and cls[run] == kind and run - off < 16:
            run += 1
        note = []
        if off in names_by_offset:
            note.append(names_by_offset[off] + ":")
        if off in a.ambiguities:
            note.append("ambiguity: " + a.ambiguities[off]["reason"])
        if kind == "data":
            note.append("; ".join(sorted(a.data[off])))
        raw = " ".join(f"{b:02X}" for b in a.rom[off:run])
        lines.append(f"{addr(off)}  .db {raw:<48} {kind} ; {' '.join(note)}".rstrip(" ;"))
        off = run
    return "\n".join(lines) + "\n"


# ------------------------------------------------------------ tracked map


def bank_totals(cls: list[str]) -> list[dict[str, Any]]:
    out = []
    for b in range(4):
        c = Counter(cls[b << 15:(b + 1) << 15])
        out.append({"bank": f"${0x80 + b:02X}", **{k: c.get(k, 0) for k in CLASSES}})
    return out


def build(a: Analysis, cls: list[str], root: Path, map_paths: list[Path], coverage_digests: list[str]) -> tuple[dict[str, Any], list[dict[str, Any]]]:
    rts = routines(a, cls)
    starts = {offset_of(parse(r["start"])) for r in rts}
    cited = cited_addresses(root)
    labs = labels(a, cls, cited, starts)
    # NATIVE-READABILITY: name the native symbols citing each routine and label.
    native_path = root / native_symbols.OUT
    span = native_symbols.native_by_rom_span(json.loads(native_path.read_text(encoding="utf-8"))) if native_path.exists() else []
    for r in rts:
        nat = native_symbols.symbols_in(span, native_symbols.parse_key(r["start"]), native_symbols.parse_key(r["end"]))
        if nat:
            r["native"] = nat
    for lab in labs:
        nat = native_symbols.symbols_in(span, native_symbols.parse_key(lab["address"]), native_symbols.parse_key(lab["address"]))
        if nat:
            lab["native"] = nat
    banks = bank_totals(cls)
    totals = {k: sum(b[k] for b in banks) for k in CLASSES}
    source = Counter(i.source for i in a.ins.values())
    doc = {
        "schema_version": 1,
        "kind": "static_code_map",
        "rom": {"sha256": a.maps[0]["rom"]["sha256"], "size": a.maps[0]["rom"]["size"]},
        "scope": "ROM offsets $00000-$1FFFF: banks $80-$83 (mirrored at $00-$03)",
        "inputs": {"maps": [{"scenario_id": m["scenario_id"], "path": p.as_posix(), "coverage_sha256": m["coverage"]["sha256"]}
                            for m, p in zip(a.maps, map_paths)],
                   "observed_boundaries_from": a.tiling["source"], "raw_coverage_sha256": sorted(coverage_digests),
                   "native_symbols": native_symbols.OUT if native_path.exists() else None},
        "regeneration_command": "python3 tools/project.py coverage static-map --out docs/map/static/code-banks.map.json "
                                "--summary docs/map/static/code-banks.md --labels docs/map/static/labels.json "
                                "--coverage <each tracked map's raw coverage.json>",
        "classes": {"observed": "executed in a tracked map",
                    "inferred": "decoded statically by descent from observed code, the vectors or a jump table",
                    "data": "named by an operand or a jump-table walk, never decoded as code", "unknown": "none of these (gap-sweep candidates stay unknown)"},
        "totals": {"bytes": SPAN, **totals, "unknown_share": round(totals["unknown"] / SPAN, 4),
                   "instructions": dict(sorted(source.items())), "routines": len(rts),
                   "gap_sweep_candidate_instructions": len(a.candidates), "gap_sweep_candidate_bytes": len(a.candidate_owner),
                   "mode_ambiguities": len(a.ambiguities), "conflicts": len(a.conflicts), "jump_tables": len(a.tables),
                   "data_references_into_code": len(a.data_into_code), "absolute_operands_with_unknown_data_bank": a.unresolved_abs,
                   "descent_stops": dict(sorted(a.stops.items())), "assumptions_used": dict(sorted(a.assumptions.items())),
                   "cited_addresses": len(labs),
                   "routines_cited_by_native_code": sum(1 for r in rts if "native" in r),
                   "routine_bytes_cited_by_native_code": sum(r["bytes"] for r in rts if "native" in r), "cited_by_class": dict(sorted(Counter(l["class"] for l in labs).items())),
                   "cited_by_position": dict(sorted(Counter(l["label"][:3] for l in labs).items()))},
        "observed_agreement": a.tiling,
        "banks": banks,
        "routines": rts,
        "jump_tables": a.tables,
        "mode_ambiguities": [a.ambiguities[o] for o in sorted(a.ambiguities)],
        "conflicts": sorted(a.conflicts, key=lambda c: (c["address"], c["reason"])),
        "data_references_into_code": [addr(o) for o in sorted(a.data_into_code)],
    }
    return doc, labs


def dump(doc: Any) -> str:
    return json.dumps(doc, indent=1, sort_keys=True) + "\n"


def summary_markdown(doc: dict[str, Any], labs: list[dict[str, Any]]) -> str:
    t = doc["totals"]
    lines = [
        "# Static code map of banks $80-$83",
        "",
        "Generated by `coverage static-map` (STATIC-CODE-MAP, R-0045); do not edit by hand. A reading aid and a",
        "hypothesis generator under D-0008, not gameplay evidence: `inferred` code is a static decoding that no",
        "capture has executed. Observed boundaries come from " + doc["inputs"]["observed_boundaries_from"] + ".",
        "Each routine and label cited by native code names its native symbols (`native`), read from",
        "`native-symbols.json` (`coverage native-symbols`; regenerate it first).",
        "",
        "| Bank | Observed | Inferred | Data | Unknown |",
        "| --- | ---: | ---: | ---: | ---: |",
    ]
    for b in doc["banks"]:
        lines.append(f"| {b['bank']} | {b['observed']:,} | {b['inferred']:,} | {b['data']:,} | {b['unknown']:,} |")
    lines.append(f"| All | {t['observed']:,} | {t['inferred']:,} | {t['data']:,} | {t['unknown']:,} |")
    lines += [
        "",
        f"The four classes partition all {t['bytes']:,} bytes. **Unknown share: {t['unknown_share']:.1%}**"
        + (" - above D-0008's one-quarter revisit trigger, so no further heuristics are added; the unknown regions are targets for dynamic capture."
           if t["unknown_share"] > 0.25 else "."),
        "",
        "| Measure | Value |",
        "| --- | ---: |",
    ]
    for k, v in t["instructions"].items():
        lines.append(f"| Instructions from {k} | {v:,} |")
    for k in ("routines", "routines_cited_by_native_code", "routine_bytes_cited_by_native_code", "gap_sweep_candidate_instructions", "gap_sweep_candidate_bytes", "jump_tables", "mode_ambiguities", "conflicts", "data_references_into_code",
              "absolute_operands_with_unknown_data_bank"):
        lines.append(f"| {k.replace('_', ' ').capitalize()} | {t[k]:,} |")
    for k, v in t["descent_stops"].items():
        lines.append(f"| Descent stopped: {k} | {v:,} |")
    for k, v in t["assumptions_used"].items():
        lines.append(f"| Assumption used: {k} | {v:,} |")
    lines += ["", "## Agreement with the observations", "",
              "| Map | Ranges | One tiling | Several | Not fixed by tiling | Checked against sites | Disagreements |",
              "| --- | ---: | ---: | ---: | ---: | ---: | ---: |"]
    for m in doc["observed_agreement"]["maps"]:
        lines.append(f"| {m['scenario_id']} | {m['ranges']} | {m['one_tiling']} | {m['several_tilings']} | "
                     f"{m['instructions_not_fixed_by_tiling']} | {m['shared_instructions_checked_against_sites']} | {m['disagreements']} |")
    lines += ["", "## Cited addresses", "",
              f"{t['cited_addresses']} distinct code-bank ROM addresses cited by the research, task, inventory and content records "
              "(`labels.json`). By position: " + ", ".join(f"{k} {v}" for k, v in t["cited_by_position"].items())
              + " (sub routine start, loc instruction start, mid inside an instruction, dat data, unk unknown). "
              "By class: " + ", ".join(f"{k} {v}" for k, v in t["cited_by_class"].items()) + ".", ""]
    unknown = [l for l in labs if l["class"] == "unknown"]
    if unknown:
        lines += ["Cited addresses in `unknown` bytes:", "", "| Address | Records |", "| --- | --- |"]
        for l in unknown:
            lines.append(f"| {l['address']} | {', '.join(l['source_records'])} |")
        lines.append("")
    if doc["conflicts"]:
        lines += ["## Conflicts", "", "| Address | Reason |", "| --- | --- |"]
        for c in doc["conflicts"]:
            lines.append(f"| {c['address']} | {c['reason']} |")
        lines.append("")
    return "\n".join(lines)
