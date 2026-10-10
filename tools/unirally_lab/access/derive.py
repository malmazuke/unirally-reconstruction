"""Streaming derivation of the access record from the per-frame trace drain.

For every frame the worker hands over the raw ring entries executed in that
frame (oldest first) and the work RAM at the end of the frame. Each entry is
decoded against the ROM (opcode and operand bytes at the pc, length under the
entry's M/X flags) and ``modes.decode`` gives its accesses from the
pre-instruction registers. Aggregation is per distinct
(pc, mode, addressing, kind, width, wrap, address) for work RAM and
register/other addresses; ROM reads are aggregated per (pc, mode, addressing,
kind) with their address range plus a bitmap of ROM offsets read.

Resolution from work RAM (D-0002): the pointer bytes of indirect forms and
the opcode/operand bytes of code executing from work RAM are taken from the
recorded stores of the same frame when the last recorded write to the byte
before the access has a known value (``recorded_store``), from the work RAM
at the start of the frame when the byte's recorded writes all come later
(``start_of_frame``), or from the work RAM at the end of the frame when no
recorded write touched the byte in the frame (``end_of_frame``). A byte whose
last preceding recorded write has an unknown value (read-modify-write, block
move, unresolved store) stays unresolved. Resolved accesses that write may
invalidate other resolutions of the same frame; the frame is re-resolved
until stable, and after a bounded number of rounds any resolution touched by
such a write is dropped. With ``resolve_rmw`` (``access capture --resolve-rmw``,
DATA-COVERAGE) a read-modify-write's result is computed from the byte's
resolved old value and the instruction's own registers (INC, DEC, ASL, LSR;
ROL and ROR with the carry in P; TSB and TRB with A), so a pointer advanced by
INC stays resolved; an RMW whose old value is unresolved stays unresolved. What the record cannot show at all is listed in
``residual``: writes through unresolved pointers (which could hit any byte,
so every resolution carries this caveat), DMA engine transfers (only their
parameter stores are recorded, see ``dma_log``) and VRAM/CGRAM/OAM contents.
"""

from __future__ import annotations

import hashlib
import struct
from bisect import bisect_left
from collections import Counter
from operator import itemgetter
from pathlib import Path
from typing import Any

from ..coverage.opcodes import TABLE, instruction_length
from . import modes
from .modes import (
    ADDRESSING_INDEX, BLOCK_READ, BLOCK_WRITE, PULL, PUSH, READ, RMW, WRAP_LINEAR, WRITE, KIND_NAMES, byte_addresses,
)

ACCESS_SCHEMA_VERSION = 1


class DrainOverflow(Exception):
    """A frame executed more instructions than the ring holds."""

TRACE_ENTRY_SIZE = 22
_ENTRY = struct.Struct("<IHHHHHBBBBHH")
assert _ENTRY.size == TRACE_ENTRY_SIZE
MAX_VALUES = 8            # distinct stored values kept per access key
MAX_RESOLVE_ROUNDS = 4
MAX_WATCH_PC_ENTRIES = 250_000
LABELS = ("recorded_store", "start_of_frame", "end_of_frame", "unresolved")
WRAM_SIZE = 0x20000

# Per-mode instruction lengths, indexed [mode][opcode].
_LENGTHS = tuple(tuple(instruction_length(op, mode) for op in range(256)) for mode in range(8))
_ADDRESSING = tuple(ADDRESSING_INDEX[TABLE[op][1]] for op in range(256))
# Opcodes that touch no memory beyond their own bytes: implied non-stack, immediate, relative, JMP abs, JML long.
_NO_ACCESS = tuple(
    (TABLE[op][1] in ("imm8", "immM", "immX", "rel8", "rel16")) or (TABLE[op][1] == "imp" and TABLE[op][0] not in (
        "PHA", "PHX", "PHY", "PHP", "PHB", "PHK", "PHD", "PLA", "PLX", "PLY", "PLP", "PLB", "PLD", "RTS", "RTL", "RTI"))
    or op in (0x4C, 0x5C) for op in range(256))
_WRITE_KINDS = frozenset({WRITE, RMW, PUSH, BLOCK_WRITE})

# Key layout (Python int): pc:24 | mode:3 | addressing:5 | kind:3 | width-1:2 | wrap:2 | address:24
_SHIFT_ADDR = 24
_SHIFT_WRAP = 24
_SHIFT_WIDTH = 26
_SHIFT_KIND = 28
_SHIFT_ADDRESSING = 31
_SHIFT_MODE = 36
_SHIFT_PC = 39


def encode_key(pc: int, mode: int, addressing: int, kind: int, width: int, wrap: int, address: int) -> int:
    return (pc << _SHIFT_PC) | (mode << _SHIFT_MODE) | (addressing << _SHIFT_ADDRESSING) | (kind << _SHIFT_KIND) \
        | ((width - 1) << _SHIFT_WIDTH) | (wrap << _SHIFT_WRAP) | address


def decode_key(key: int) -> tuple[int, int, int, int, int, int, int]:
    return (key >> _SHIFT_PC, (key >> _SHIFT_MODE) & 7, (key >> _SHIFT_ADDRESSING) & 31, (key >> _SHIFT_KIND) & 7,
            ((key >> _SHIFT_WIDTH) & 3) + 1, (key >> _SHIFT_WRAP) & 3, key & 0xFFFFFF)


def rom_offset(address: int, size: int) -> int | None:
    bank = address >> 16
    off = address & 0xFFFF
    if off >= 0x8000 and (bank <= 0x6F or 0x80 <= bank <= 0xEF):
        return (((bank & 0x7F) << 15) | (off & 0x7FFF)) % size
    return None


def wram_offset(address: int) -> int | None:
    bank = address >> 16
    off = address & 0xFFFF
    if bank in (0x7E, 0x7F):
        return ((bank - 0x7E) << 16) | off
    if off < 0x2000 and (bank <= 0x3F or 0x80 <= bank <= 0xBF):
        return off
    return None


def is_register(address: int) -> bool:
    """B-bus and CPU registers reachable from banks $00-$3F/$80-$BF at $2100-$5FFF."""
    bank = address >> 16
    off = address & 0xFFFF
    return 0x2000 <= off < 0x6000 and (bank <= 0x3F or 0x80 <= bank <= 0xBF)


class RmwOp:
    """One read-modify-write whose result follows from its old value (``resolve_rmw``)."""

    __slots__ = ("seq", "mnemonic", "width", "a", "carry", "offsets", "generation", "result", "label")

    def __init__(self, seq: int, mnemonic: str, width: int, a: int, carry: int, offsets: tuple[int, ...]) -> None:
        self.seq = seq
        self.mnemonic = mnemonic
        self.width = width
        self.a = a
        self.carry = carry
        self.offsets = offsets
        self.generation = -1
        self.result: int | None = None
        self.label = 3

    def apply(self, old: int) -> int:
        bits = 8 * self.width
        mask = (1 << bits) - 1
        m = self.mnemonic
        if m == "INC":
            return (old + 1) & mask
        if m == "DEC":
            return (old - 1) & mask
        if m == "ASL":
            return (old << 1) & mask
        if m == "LSR":
            return old >> 1
        if m == "ROL":
            return ((old << 1) | self.carry) & mask
        if m == "ROR":
            return (old >> 1) | (self.carry << (bits - 1))
        if m == "TSB":
            return old | (self.a & mask)
        if m == "TRB":
            return old & ~self.a & mask
        raise ValueError(f"not a read-modify-write: {m}")


class RmwByte:
    """Byte ``k`` of an RmwOp's result, as a write list's value."""

    __slots__ = ("op", "k")

    def __init__(self, op: RmwOp, k: int) -> None:
        self.op = op
        self.k = k


class Resolver:
    """Byte values of work RAM at a point within one frame (see the module docstring)."""

    def __init__(self, writes: dict[int, list[tuple[int, Any]]], start: bytes, end: bytes) -> None:
        self.writes = writes
        self.start = start
        self.end = end
        self.generation = 0
        self.rmw_read: set[int] = set()   # bytes whose values computed RMW results depend on

    def invalidate(self) -> None:
        """Forget computed RMW results after ``writes`` changed."""
        self.generation += 1
        self.rmw_read = set()

    def byte(self, offset: int, seq: int) -> tuple[int | None, int]:
        w = self.writes.get(offset)
        if w is None:
            return self.end[offset], 2
        j = bisect_left(w, seq, key=itemgetter(0))
        if j == 0:
            return self.start[offset], 1
        value = w[j - 1][1]
        if value is None:
            return None, 3
        if isinstance(value, RmwByte):
            op = value.op
            if op.generation != self.generation:
                self._evaluate(op)
            if op.result is None:
                return None, 3
            return (op.result >> (8 * value.k)) & 0xFF, op.label
        return value, 0

    def _previous_op(self, offset: int, seq: int) -> RmwOp | None:
        w = self.writes.get(offset)
        if not w:
            return None
        j = bisect_left(w, seq, key=itemgetter(0))
        if j and isinstance(w[j - 1][1], RmwByte):
            op = w[j - 1][1].op
            if op.generation != self.generation:
                return op
        return None

    def _evaluate(self, op: RmwOp) -> None:
        """Compute ``op`` and the uncomputed RMWs its old value depends on, oldest first, without recursion."""
        stack = [op]
        while stack:
            cur = stack[-1]
            if cur.generation == self.generation:
                stack.pop()
                continue
            deps = [d for d in (self._previous_op(o, cur.seq) for o in cur.offsets) if d is not None]
            if deps:
                stack.extend(deps)
                continue
            self.rmw_read.update(cur.offsets)
            old, label = self.value(list(cur.offsets), cur.seq)
            cur.result = None if old is None else cur.apply(old)
            cur.label = label
            cur.generation = self.generation
            stack.pop()

    def value(self, offsets: list[int], seq: int) -> tuple[int | None, int]:
        """Little-endian value of the bytes at ``offsets`` and the weakest label."""
        result = 0
        label = 0
        for k, off in enumerate(offsets):
            v, lab = self.byte(off, seq)
            if v is None:
                return None, 3
            result |= v << (8 * k)
            if lab > label:
                label = lab
        return result, label


class AccessDrain:
    def __init__(self, rom: bytes, capacity: int, watch_addresses: list[int] | None = None, watch_pcs: list[int] | None = None,
                 series: tuple[int, int, int, Path] | None = None, resolve_rmw: bool = False) -> None:
        if capacity <= 0:
            raise ValueError("ring capacity must be positive")
        self.rom = rom
        self.size = len(rom)
        self.capacity = capacity
        self.resolve_rmw = resolve_rmw
        self.watch_addresses = sorted({wram_offset(a) if wram_offset(a) is not None else a for a in (watch_addresses or [])})
        self._watch_set = set(self.watch_addresses)
        self.watch_pcs = sorted(set(watch_pcs or []))
        self._watch_pc_set = set(self.watch_pcs)
        self.watch_log: dict[int, dict[int, dict[str, list]]] = {a: {} for a in self.watch_addresses}
        self.pc_log: dict[int, list[list[int]]] = {pc: [] for pc in self.watch_pcs}
        self.pc_log_truncated = False
        self.series = series
        self._series_fh = None
        self._series_frames: list[int] = []
        self._series_sha = hashlib.sha256()
        self.counts: Counter[int] = Counter()
        self.first: dict[int, int] = {}
        self.last: dict[int, int] = {}
        self.values: dict[int, list[int]] = {}
        self.values_truncated: set[int] = set()
        self.labels: dict[int, int] = {}       # key -> weakest resolution label seen (resolved accesses only)
        self.rom_reads: dict[int, list[int]] = {}   # (pc,mode,addressing,kind) key -> [count, min, max, first, last]
        self.rom_bitmap = bytearray(self.size)
        self.resolutions: Counter[tuple[int, int, int]] = Counter()   # (pc, addressing, label) -> count
        self.unresolved_stores = 0
        self.unresolved_store_pcs: Counter[int] = Counter()
        self.wram_code_unresolved: Counter[int] = Counter()
        self.non_rom_pcs: Counter[int] = Counter()
        self.dma_log: list[list[Any]] = []
        self._regs = bytearray(0x4400)
        self.per_frame: list[int] = []
        self.per_frame_accesses: list[int] = []
        self.per_frame_unresolved: list[int] = []
        self.total = 0
        self.total_accesses = 0
        self.max_delta = 0
        self.frames: list[int] = []
        self.prev_wram: bytes | None = None
        self.decode_failures: Counter[int] = Counter()
        self.resolution_conflicts = 0
        self.resolution_dropped = 0

    # ------------------------------------------------------------------ frames

    def set_previous_wram(self, wram: bytes) -> None:
        self.prev_wram = bytes(wram)

    def drain_frame(self, frame: int, raw: bytes, wram_end: bytes) -> int:
        if self.prev_wram is None:
            raise ValueError("the work RAM before the first drained frame must be set first")
        if len(wram_end) != WRAM_SIZE or len(self.prev_wram) != WRAM_SIZE:
            raise ValueError("work RAM must be 131072 bytes")
        entries = list(_ENTRY.iter_unpack(raw))
        n = len(entries)
        self.frames.append(frame)
        self.per_frame.append(n)
        self.total += n
        if n > self.max_delta:
            self.max_delta = n
        keys: list[int] = []
        writes: dict[int, list[tuple[int, int | None]]] = {}
        deferred: list[tuple] = []     # (seq, pc, mode, addressing, dbr, item)
        wram_code: list[tuple] = []    # (seq, index)
        unresolved = 0
        rom = self.rom
        size = self.size
        lengths = _LENGTHS
        no_access = _NO_ACCESS
        addressing_of = _ADDRESSING
        decode = modes.decode
        record = self._record

        for i in range(n):
            ent = entries[i]
            pc, a, x, y, s, d, b, p, e = ent[0], ent[1], ent[2], ent[3], ent[4], ent[5], ent[6], ent[7], ent[8]
            mode = (4 if e else 0) | (2 if p & 0x20 else 0) | (1 if p & 0x10 else 0)
            if pc in self._watch_pc_set:
                self._log_pc(pc, frame, a, x, y, s, d, b, p, e)
            bank = pc >> 16
            off = pc & 0xFFFF
            if off >= 0x8000 and (bank <= 0x6F or 0x80 <= bank <= 0xEF):
                romoff = (((bank & 0x7F) << 15) | (off & 0x7FFF)) % size
            else:
                self.non_rom_pcs[pc] += 1
                if wram_offset(pc) is not None:
                    wram_code.append((i, i))
                continue
            opcode = rom[romoff]
            if no_access[opcode]:
                continue
            length = lengths[mode][opcode]
            if off + length <= 0x10000:
                operand = rom[romoff + 1:romoff + length]
            else:
                operand = bytes(rom[o] for o in (rom_offset((pc & 0xFF0000) | ((pc + k) & 0xFFFF), size) for k in range(1, length)) if o is not None)
            nxt = entries[i + 1] if i + 1 < n else None
            accesses, items = decode(opcode, operand, pc, a, x, y, s, d, b, p, e, nxt)
            addressing = addressing_of[opcode]
            for acc in accesses:
                record(keys, writes, frame, i, pc, mode, addressing, acc,
                       rmw=(TABLE[opcode][0], a, p & 1) if acc[0] == RMW and self.resolve_rmw else None)
            for item in items:
                deferred.append((i, pc, mode, addressing, b, item))

        # ---- resolution rounds
        resolver = Resolver(writes, self.prev_wram, wram_end)
        rounds = 0
        while True:
            rounds += 1
            rnd = self._resolve_round(entries, n, frame, resolver, wram_code, deferred)
            conflict = (rnd["touched"] | resolver.rmw_read) & set(rnd["writes"])
            if conflict and rounds < MAX_RESOLVE_ROUNDS:
                # A resolved access wrote a byte that some resolution read from the frame's
                # work RAM images: merge the round's writes and resolve again from scratch.
                for off, lst in rnd["writes"].items():
                    merged = writes.get(off, []) + lst
                    merged.sort(key=itemgetter(0))
                    writes[off] = merged
                resolver.invalidate()
                continue
            if conflict:
                self.resolution_conflicts += len(conflict)
                self._drop_conflicting(rnd, conflict)
            break
        for pc, addressing, lab in rnd["labels"]:
            self.resolutions[(pc, addressing, lab)] += 1
        for pc, m in rnd["unresolved_store_pcs"].items():
            self.unresolved_store_pcs[pc] += m
            self.unresolved_stores += m
        unresolved = rnd["unresolved"]
        keys.extend(rnd["keys"])

        # ---- aggregation
        self.counts.update(keys)
        for k in set(keys):
            if k not in self.first:
                self.first[k] = frame
            self.last[k] = frame
        self.total_accesses += len(keys)
        self.per_frame_accesses.append(len(keys))
        self.per_frame_unresolved.append(unresolved)
        self.prev_wram = bytes(wram_end)
        return n

    def _resolve_round(self, entries: list[tuple], n: int, frame: int, resolver: "Resolver", wram_code: list[tuple],
                       deferred: list[tuple]) -> dict[str, Any]:
        """One resolution pass over the frame's work RAM code entries and deferred indirect items.

        Returns the produced keys, the writes those accesses make, the (pc, addressing,
        label) records, the bytes every resolution read, and the unresolved counts.
        Each produced key is tagged with the resolution it came from so that a conflict
        can drop exactly the affected accesses."""
        keys: list[int] = []
        round_writes: dict[int, list[tuple[int, int | None]]] = {}
        labels: list[tuple[int, int, int]] = []
        touched: set[int] = set()
        origin: list[tuple[int, frozenset[int], int]] = []   # (first key index, offsets read, label index)
        unresolved = 0
        unresolved_store_pcs: Counter[int] = Counter()
        lengths = _LENGTHS
        addressing_of = _ADDRESSING
        pending = list(deferred)
        for seq, index in wram_code:
            ent = entries[index]
            pc = ent[0]
            mode = (4 if ent[8] else 0) | (2 if ent[7] & 0x20 else 0) | (1 if ent[7] & 0x10 else 0)
            woff = wram_offset(pc)
            opcode, worst = resolver.byte(woff, seq)
            if opcode is None:
                unresolved += 1
                labels.append((pc, -1, 3))
                self.wram_code_unresolved[pc] += 1
                continue
            length = lengths[mode][opcode]
            offsets = [(woff + k) & (WRAM_SIZE - 1) for k in range(length)]
            touched.update(offsets)
            operand_bytes: list[int] = []
            for o in offsets[1:]:
                v, l2 = resolver.byte(o, seq)
                if v is None:
                    worst = 3
                    break
                operand_bytes.append(v)
                if l2 > worst:
                    worst = l2
            if worst == 3:
                unresolved += 1
                labels.append((pc, -1, 3))
                self.wram_code_unresolved[pc] += 1
                continue
            nxt = entries[index + 1] if index + 1 < n else None
            accesses, items = modes.decode(opcode, bytes(operand_bytes), pc, ent[1], ent[2], ent[3], ent[4], ent[5], ent[6], ent[7], ent[8], nxt)
            addressing = addressing_of[opcode]
            origin.append((len(keys), frozenset(offsets), len(labels)))
            labels.append((pc, addressing, worst))
            for acc in accesses:
                self._record(keys, round_writes, frame, seq, pc, mode, addressing, acc, label=worst,
                             rmw=(TABLE[opcode][0], ent[1], ent[7] & 1) if acc[0] == RMW and self.resolve_rmw else None)
            for item in items:
                pending.append((seq, pc, mode, addressing, ent[6], item))
        for seq, pc, mode, addressing, dbr, item in pending:
            paddr, pwidth, pwrap, _index, _use_dbr, kind, width, value = item
            offsets = [wram_offset(ba) for ba in byte_addresses(paddr, pwidth, pwrap)]
            pointer = None
            lab = 3
            if all(o is not None for o in offsets):
                touched.update(offsets)
                pointer, lab = resolver.value(offsets, seq)
            if pointer is None:
                unresolved += 1
                labels.append((pc, addressing, 3))
                if kind in _WRITE_KINDS:
                    unresolved_store_pcs[pc] += 1
                continue
            acc = modes.resolve_with_bank(item, pointer, dbr)
            origin.append((len(keys), frozenset(offsets), len(labels)))
            labels.append((pc, addressing, lab))
            self._record(keys, round_writes, frame, seq, pc, mode, addressing, acc, label=lab)
        return {"keys": keys, "writes": round_writes, "labels": labels, "touched": touched, "origin": origin,
                "unresolved": unresolved, "unresolved_store_pcs": unresolved_store_pcs}

    def _drop_conflicting(self, rnd: dict[str, Any], conflict: set[int]) -> None:
        """Remove the accesses of every resolution that read a byte the round itself wrote."""
        keys = rnd["keys"]
        origin = rnd["origin"]
        bounds = [o[0] for o in origin] + [len(keys)]
        keep: list[int] = []
        dropped = 0
        for j, (start, offsets, label_index) in enumerate(origin):
            segment = keys[start:bounds[j + 1]]
            if offsets & conflict:
                dropped += 1
                pc, addressing, _lab = rnd["labels"][label_index]
                rnd["labels"][label_index] = (pc, addressing, 3)
                continue
            keep.extend(segment)
        rnd["keys"] = keep
        rnd["unresolved"] += dropped
        self.resolution_dropped += dropped

    # ------------------------------------------------------------------ record

    def _record(self, keys: list[int], writes: dict[int, list[tuple[int, int | None]]], frame: int, seq: int, pc: int, mode: int,
                addressing: int, acc: tuple, label: int | None = None, rmw: tuple[str, int, int] | None = None) -> None:
        kind, address, width, value, wrap = acc
        bank = address >> 16
        off = address & 0xFFFF
        if off >= 0x8000 and (bank <= 0x6F or 0x80 <= bank <= 0xEF):
            # ROM: range per (pc, mode, addressing, kind), plus the offset bitmap.
            rk = (pc << 11) | (mode << 8) | (addressing << 3) | kind
            entry = self.rom_reads.get(rk)
            if entry is None:
                self.rom_reads[rk] = [1, address, address, frame, frame]
            else:
                entry[0] += 1
                if address < entry[1]:
                    entry[1] = address
                if address > entry[2]:
                    entry[2] = address
                entry[4] = frame
            bm = self.rom_bitmap
            size = self.size
            if width == 1 and wrap == WRAP_LINEAR:
                bm[(((bank & 0x7F) << 15) | (off & 0x7FFF)) % size] = 1
            else:
                for ba in byte_addresses(address, width, wrap):
                    o = rom_offset(ba, size)
                    if o is not None:
                        bm[o] = 1
            if label is not None:
                self.labels[rk] = max(self.labels.get(rk, 0), label)
            return
        key = encode_key(pc, mode, addressing, kind, width, wrap, address)
        keys.append(key)
        if label is not None:
            self.labels[key] = max(self.labels.get(key, 0), label)
        if value is not None and kind != READ and kind != PULL and kind != BLOCK_READ:
            lst = self.values.get(key)
            if lst is None:
                self.values[key] = [value]
            elif value not in lst:
                if len(lst) < MAX_VALUES:
                    lst.append(value)
                else:
                    self.values_truncated.add(key)
        if kind in _WRITE_KINDS:
            woff = wram_offset(address)
            if woff is not None and rmw is not None:
                offs = tuple(wram_offset(ba) for ba in byte_addresses(address, width, wrap))
                if all(o is not None for o in offs):
                    op = RmwOp(seq, rmw[0], width, rmw[1], rmw[2], offs)   # type: ignore[arg-type]
                    for k, wo in enumerate(offs):
                        writes.setdefault(wo, []).append((seq, RmwByte(op, k)))
                    woff = None
                    rmw = None
            if woff is not None:
                if width == 1:
                    lst = writes.get(woff)
                    if lst is None:
                        writes[woff] = [(seq, value)]
                    else:
                        lst.append((seq, value))
                else:
                    for k, ba in enumerate(byte_addresses(address, width, wrap)):
                        wo = wram_offset(ba)
                        if wo is None:
                            continue
                        bv = None if value is None else (value >> (8 * k)) & 0xFF
                        lst = writes.get(wo)
                        if lst is None:
                            writes[wo] = [(seq, bv)]
                        else:
                            lst.append((seq, bv))
            elif value is not None and 0x2000 <= off < 0x4400 and (bank <= 0x3F or 0x80 <= bank <= 0xBF):
                self._register_write(frame, seq, pc, off, width, value)
        if self._watch_set:
            watch_hit = None
            if width == 1:
                woff = wram_offset(address)
                target = woff if woff is not None else address
                if target in self._watch_set:
                    watch_hit = target
            else:
                for ba in byte_addresses(address, width, wrap):
                    woff = wram_offset(ba)
                    target = woff if woff is not None else ba
                    if target in self._watch_set:
                        watch_hit = target
                        break
            if watch_hit is not None:
                per_frame = self.watch_log[watch_hit].setdefault(frame, {"w": [], "r": []})
                (per_frame["w"] if kind in _WRITE_KINDS else per_frame["r"]).append([seq, pc, KIND_NAMES[kind], address, width, value])

    def _register_write(self, frame: int, seq: int, pc: int, off: int, width: int, value: int) -> None:
        regs = self._regs
        for k in range(width):
            o = off + k
            if o < len(regs):
                regs[o] = (value >> (8 * k)) & 0xFF
        if off <= 0x420B < off + width:
            v = (value >> (8 * (0x420B - off))) & 0xFF
            if v:
                self.dma_log.append([frame, seq, pc, "MDMAEN", v, self._channels(v)])
        if off <= 0x420C < off + width:
            v = (value >> (8 * (0x420C - off))) & 0xFF
            if v:
                self.dma_log.append([frame, seq, pc, "HDMAEN", v, self._channels(v)])

    def _channels(self, mask: int) -> dict[str, dict[str, int]]:
        out = {}
        regs = self._regs
        for c in range(8):
            if mask & (1 << c):
                base = 0x4300 + (c << 4)
                out[str(c)] = {"DMAP": regs[base], "BBAD": regs[base + 1],
                               "A1T": regs[base + 2] | (regs[base + 3] << 8) | (regs[base + 4] << 16),
                               "DAS": regs[base + 5] | (regs[base + 6] << 8), "DASB": regs[base + 7],
                               "A2A": regs[base + 8] | (regs[base + 9] << 8), "NTRL": regs[base + 10]}
        return out

    def _log_pc(self, pc: int, frame: int, a: int, x: int, y: int, s: int, d: int, b: int, p: int, e: int) -> None:
        log = self.pc_log[pc]
        if sum(len(v) for v in self.pc_log.values()) >= MAX_WATCH_PC_ENTRIES:
            self.pc_log_truncated = True
            return
        log.append([frame, a, x, y, s, d, b, p, e])

    def series_frame(self, frame: int, wram: bytes) -> None:
        """Append the declared work RAM range for ``frame`` when it lies on the stride (frames counted from 0).

        The worker calls this for every executed frame, inside or outside the derivation window."""
        if self.series is None:
            return
        start, length, every, path = self.series
        if frame % every:
            return
        if self._series_fh is None:
            path.parent.mkdir(parents=True, exist_ok=True)
            self._series_fh = open(path, "wb")
        chunk = wram[start:start + length]
        self._series_fh.write(chunk)
        self._series_sha.update(chunk)
        self._series_frames.append(frame)

    # ------------------------------------------------------------------ output

    def close(self) -> None:
        if self._series_fh is not None:
            self._series_fh.close()
            self._series_fh = None

    def rom_ranges(self) -> list[list[int]]:
        out: list[list[int]] = []
        bm = self.rom_bitmap
        i = 0
        n = len(bm)
        while i < n:
            if bm[i]:
                j = i
                while j < n and bm[j]:
                    j += 1
                out.append([i, j - i])
                i = j
            else:
                i += 1
        return out

    def document(self, identity: dict[str, Any], frames: tuple[int, int]) -> dict[str, Any]:
        self.close()
        accesses = []
        for key in sorted(self.counts):
            pc, mode, addressing, kind, width, wrap, address = decode_key(key)
            vals = self.values.get(key)
            accesses.append([pc, mode, addressing, kind, width, wrap, address, self.counts[key], self.first[key], self.last[key],
                             vals, key in self.values_truncated, self.labels.get(key)])
        rom_reads = []
        for rk in sorted(self.rom_reads):
            count, lo, hi, first, last = self.rom_reads[rk]
            rom_reads.append([rk >> 11, (rk >> 8) & 7, (rk >> 3) & 31, rk & 7, count, lo, hi, first, last, self.labels.get(rk)])
        ranges = self.rom_ranges()
        resolutions = [[pc, addressing, LABELS[lab], n] for (pc, addressing, lab), n in sorted(self.resolutions.items())]
        series = None
        if self.series is not None:
            start, length, every, path = self.series
            series = {"path": str(path.name), "start": start, "length": length, "every": every, "frames": list(self._series_frames),
                      "sha256": self._series_sha.hexdigest(), "bytes": length * len(self._series_frames)}
        return {
            "schema_version": ACCESS_SCHEMA_VERSION,
            "kind": "access_record",
            **identity,
            "ring_capacity": self.capacity,
            "resolve_rmw": self.resolve_rmw,
            "frames": {"start": frames[0], "end": frames[1], "count": len(self.per_frame)},
            "instructions": {"total": self.total, "max_frame_delta": self.max_delta, "per_frame": list(self.per_frame)},
            "accesses_total": self.total_accesses,
            "per_frame_accesses": list(self.per_frame_accesses),
            "per_frame_unresolved": list(self.per_frame_unresolved),
            "addressing_names": list(modes.ADDRESSING),
            "kind_names": list(KIND_NAMES),
            "wrap_names": ["linear", "bank", "page"],
            "label_names": list(LABELS),
            "access_fields": ["pc", "mode", "addressing", "kind", "width", "wrap", "address", "count", "first_frame", "last_frame",
                              "values", "values_truncated", "label"],
            "accesses": accesses,
            "rom_read_fields": ["pc", "mode", "addressing", "kind", "count", "min_address", "max_address", "first_frame", "last_frame", "label"],
            "rom_reads": rom_reads,
            "rom_bytes_read": int(sum(self.rom_bitmap)),
            "rom_read_ranges": ranges,
            "resolution_fields": ["pc", "addressing", "label", "count"],
            "resolutions": resolutions,
            "resolution_conflict_bytes": self.resolution_conflicts,
            "resolutions_dropped": self.resolution_dropped,
            "residual": {
                "unresolved_total": sum(self.per_frame_unresolved),
                "unresolved_stores": self.unresolved_stores,
                "unresolved_store_pcs": [[pc, n] for pc, n in sorted(self.unresolved_store_pcs.items())],
                "non_rom_pcs": [[pc, n] for pc, n in sorted(self.non_rom_pcs.items())],
                "dma_triggers": sum(1 for d in self.dma_log if d[3] == "MDMAEN"),
                "hdma_enables": sum(1 for d in self.dma_log if d[3] == "HDMAEN"),
                "notes": [
                    "Indirect accesses whose pointer bytes could not be resolved are counted per (pc, addressing) under label unresolved.",
                    "Every resolution assumes no unrecorded write hit the resolved bytes in the frame: writes through unresolved pointers "
                    "(unresolved_stores) and DMA engine writes to work RAM are not recorded.",
                    "DMA and HDMA engine transfers are not instructions; dma_log records each MDMAEN/HDMAEN store with the channel "
                    "parameters shadowed from the recorded register stores.",
                    "VRAM, CGRAM and OAM contents are not exported by the core; only the register stores that feed them are recorded.",
                ],
            },
            "dma_log_fields": ["frame", "seq", "pc", "register", "value", "channels"],
            "dma_log": self.dma_log,
            "watch_addresses": {str(a): {str(f): v for f, v in sorted(self.watch_log[a].items())} for a in self.watch_addresses},
            "watch_pcs": {str(pc): self.pc_log[pc] for pc in self.watch_pcs},
            "watch_pcs_truncated": self.pc_log_truncated,
            "wram_series": series,
        }


def validate_document(data: Any) -> dict[str, Any]:
    if not isinstance(data, dict) or data.get("schema_version") != ACCESS_SCHEMA_VERSION or data.get("kind") != "access_record":
        raise ValueError(f"access record schema_version must be {ACCESS_SCHEMA_VERSION} with kind access_record")
    for key in ("rom", "core", "frames", "instructions", "accesses", "rom_reads", "resolutions", "residual"):
        if key not in data:
            raise ValueError(f"access record lacks {key!r}")
    if not isinstance(data["accesses"], list):
        raise ValueError("accesses must be a list")
    for i, a in enumerate(data["accesses"]):
        if not (isinstance(a, list) and len(a) == 13 and all(isinstance(v, int) and not isinstance(v, bool) for v in a[:10])):
            raise ValueError(f"accesses[{i}] malformed")
    ins = data["instructions"]
    if not isinstance(ins, dict) or not isinstance(ins.get("per_frame"), list) or sum(ins["per_frame"]) != ins.get("total"):
        raise ValueError("instructions.total must equal the sum of instructions.per_frame")
    return data
