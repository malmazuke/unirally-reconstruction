"""Per-frame drain of the core's instruction trace ring into coverage counts.

The ring (``unirally_trace_enable``) keeps the newest ``capacity`` executed
instructions; after every ``retro_run`` the drain reads ``trace_total``, takes
the newest ``delta`` entries of the ring and accumulates them. A frame whose
delta exceeds the capacity has lost entries and the capture fails (the caller
records the reason and exits 1). Enabling the ring cannot alter emulation:
the hook only copies registers.

Accumulated per distinct (pc, mode, data bank) *site*: execution count and the
first frame; per consecutive pair of sites: count. Consecutive pairs carry
across frames. Decoding against the ROM (lengths, edges, static references)
is left to ``derive`` so this module needs only the core's trace.

The coverage file (schema 1) is plain JSON with sorted keys so that two
captures of the same run are byte-identical.
"""

from __future__ import annotations

import struct
from collections import Counter
from typing import Any, Protocol

from .opcodes import mode_from_flags

COVERAGE_SCHEMA_VERSION = 1
TAIL_SITES = 64
TRACE_ENTRY_SIZE = 22
# pc:u32 a,x,y,s,d:u16 b,p,e,reserved:u8 v,h:u16 (22 bytes) -> we need pc, b, p, e.
_PC_B_P_E = struct.Struct("<I10xBBB5x")
assert _PC_B_P_E.size == TRACE_ENTRY_SIZE

# Site key: pc | mode << 24 | data bank << 27 (35 bits; Python ints).
_MODE_SHIFT, _BANK_SHIFT = 24, 27
_PC_MASK = 0xFFFFFF


class TraceSource(Protocol):
    def trace_total(self) -> int: ...
    def trace_read_raw(self, max_entries: int) -> bytes: ...


class DrainOverflow(Exception):
    """A frame executed more instructions than the ring holds."""


def site_key(pc: int, mode: int, bank: int) -> int:
    return pc | (mode << _MODE_SHIFT) | (bank << _BANK_SHIFT)


def split_key(key: int) -> tuple[int, int, int]:
    return key & _PC_MASK, (key >> _MODE_SHIFT) & 7, (key >> _BANK_SHIFT) & 0xFF


def keys_from_raw(raw: bytes) -> list[int]:
    """Site keys of every entry in a raw trace buffer, oldest first."""
    return [pc | (mode_from_flags(p, e) << _MODE_SHIFT) | (b << _BANK_SHIFT)
            for pc, b, p, e in _PC_B_P_E.iter_unpack(raw)]


class FrameDrain:
    def __init__(self, source: TraceSource, capacity: int, watch: list[int] | None = None) -> None:
        if capacity <= 0:
            raise ValueError("ring capacity must be positive")
        self.source = source
        self.capacity = capacity
        # Watched 24-bit addresses (vector targets): executions per frame, so
        # that "once per frame" claims are checked per frame, not from totals.
        self.watch: list[int] = sorted(set(watch or []))
        self.watch_per_frame: dict[int, list[int]] = {a: [] for a in self.watch}
        self.sites: Counter[int] = Counter()
        self.first_frame: dict[int, int] = {}
        self.pairs: Counter[tuple[int, int]] = Counter()
        self.per_frame: list[int] = []
        self.total = 0
        self.max_delta = 0
        self.first_key: int | None = None
        self.last_key: int | None = None
        self.tail: list[int] = []  # the newest TAIL_SITES keys, for comparison with the samples' trace window
        self._seen_total = source.trace_total()
        self.initial_total = self._seen_total

    def drain_frame(self, frame: int) -> int:
        """Accumulate the instructions executed since the previous drain. Returns the delta."""
        total = self.source.trace_total()
        delta = total - self._seen_total
        if delta < 0:
            raise DrainOverflow(f"frame {frame}: trace total went backwards ({self._seen_total} -> {total})")
        if delta > self.capacity:
            raise DrainOverflow(f"frame {frame}: {delta} instructions exceed the ring capacity {self.capacity}; entries were lost")
        self._seen_total = total
        self.per_frame.append(delta)
        self.total += delta
        if delta > self.max_delta:
            self.max_delta = delta
        for a in self.watch:
            self.watch_per_frame[a].append(0)
        if delta == 0:
            return 0
        raw = self.source.trace_read_raw(self.capacity)
        available = len(raw) // TRACE_ENTRY_SIZE
        if available < delta:
            raise DrainOverflow(f"frame {frame}: ring returned {available} entries for a delta of {delta}")
        keys = keys_from_raw(raw[(available - delta) * TRACE_ENTRY_SIZE:])
        frame_counts = Counter(keys)
        self.sites.update(frame_counts)
        for key in frame_counts:
            self.first_frame.setdefault(key, frame)
        if self.watch:
            watched = set(self.watch)
            for key, n in frame_counts.items():
                pc = key & _PC_MASK
                if pc in watched:
                    self.watch_per_frame[pc][-1] += n
        if self.first_key is None:
            self.first_key = keys[0]
        if self.last_key is not None:
            self.pairs[(self.last_key, keys[0])] += 1
        self.pairs.update(zip(keys, keys[1:]))
        self.last_key = keys[-1]
        self.tail = (self.tail + keys[-TAIL_SITES:])[-TAIL_SITES:]
        return delta

    def document(self, identity: dict[str, Any], frames: tuple[int, int]) -> dict[str, Any]:
        """The coverage document (schema 1)."""
        sites = [[*split_key(k), self.sites[k], self.first_frame[k]] for k in sorted(self.sites)]
        pairs = [[*split_key(a), *split_key(b), n] for (a, b), n in sorted(self.pairs.items())]
        return {
            "schema_version": COVERAGE_SCHEMA_VERSION,
            "kind": "instruction_coverage",
            **identity,
            "ring_capacity": self.capacity,
            "frames": {"start": frames[0], "end": frames[1], "count": len(self.per_frame)},
            "instructions": {"initial_total": self.initial_total, "total": self.total, "max_frame_delta": self.max_delta,
                             "per_frame": list(self.per_frame)},
            "first_site": None if self.first_key is None else list(split_key(self.first_key)),
            "last_site": None if self.last_key is None else list(split_key(self.last_key)),
            "tail_sites": [list(split_key(k)) for k in self.tail],
            "watch": {"addresses": list(self.watch), "per_frame": {str(a): list(self.watch_per_frame[a]) for a in self.watch}},
            "site_fields": ["pc", "mode", "data_bank", "count", "first_frame"],
            "sites": sites,
            "pair_fields": ["pc", "mode", "data_bank", "next_pc", "next_mode", "next_data_bank", "count"],
            "pairs": pairs,
        }


def validate_document(data: Any) -> dict[str, Any]:
    """Structural check of a coverage document; raises ValueError."""
    if not isinstance(data, dict) or data.get("schema_version") != COVERAGE_SCHEMA_VERSION or data.get("kind") != "instruction_coverage":
        raise ValueError(f"coverage schema_version must be {COVERAGE_SCHEMA_VERSION} with kind instruction_coverage")
    for key in ("rom", "core", "frames", "instructions", "sites", "pairs", "ring_capacity"):
        if key not in data:
            raise ValueError(f"coverage document lacks {key!r}")
    if not isinstance(data["sites"], list) or not isinstance(data["pairs"], list):
        raise ValueError("sites and pairs must be lists")
    for i, s in enumerate(data["sites"]):
        if not (isinstance(s, list) and len(s) == 5 and all(isinstance(v, int) and not isinstance(v, bool) for v in s)):
            raise ValueError(f"sites[{i}] must be five integers")
        if not (0 <= s[0] <= 0xFFFFFF and 0 <= s[1] <= 7 and 0 <= s[2] <= 0xFF and s[3] > 0 and s[4] >= 0):
            raise ValueError(f"sites[{i}] out of range: {s}")
    for i, p in enumerate(data["pairs"]):
        if not (isinstance(p, list) and len(p) == 7 and all(isinstance(v, int) and not isinstance(v, bool) for v in p)):
            raise ValueError(f"pairs[{i}] must be seven integers")
    ins = data["instructions"]
    if not isinstance(ins, dict) or not isinstance(ins.get("per_frame"), list) or sum(ins["per_frame"]) != ins.get("total"):
        raise ValueError("instructions.total must equal the sum of instructions.per_frame")
    if sum(s[3] for s in data["sites"]) != ins["total"]:
        raise ValueError("site counts must sum to instructions.total")
    return data


def merge_documents(docs: list[dict[str, Any]], scenario_id: str, sources: list[dict[str, Any]]) -> dict[str, Any]:
    """One coverage document for several complete captures of the same ROM and core (DATA-COVERAGE).

    The runs are laid end to end on one frame axis in the given order: per-frame counts and watch
    series are concatenated, and a site's first frame is its first in the earliest run that
    executes it, offset by the frames before that run. Site and pair counts are summed; no pair
    joins the last site of one run to the first of the next. ``first_site`` is the first run's,
    ``tail_sites`` and ``last_site`` the last run's. ``sources`` (path and SHA-256 of each input,
    in order) is recorded as ``merged_from``. Raises ValueError when the inputs disagree."""
    if not docs or len(docs) != len(sources):
        raise ValueError("merge needs one source entry per coverage document, and at least one")
    first = docs[0]
    rom = first["rom"]
    core_key = ("name", "commit", "patch_sha256", "sha256")
    core = {k: first["core"].get(k) for k in core_key}
    watch_addresses = first.get("watch", {}).get("addresses", [])
    for i, d in enumerate(docs):
        if d.get("status") != "complete":
            raise ValueError(f"input {i} is not complete: {d.get('status')!r}")
        if d["rom"] != rom:
            raise ValueError(f"input {i} is of another ROM")
        if {k: d["core"].get(k) for k in core_key} != core:
            raise ValueError(f"input {i} is of another core")
        if d.get("watch", {}).get("addresses", []) != watch_addresses:
            raise ValueError(f"input {i} watches other addresses")
    sites: dict[tuple[int, int, int], list[int]] = {}
    pairs: Counter[tuple[int, ...]] = Counter()
    per_frame: list[int] = []
    watch: dict[str, list[int]] = {str(a): [] for a in watch_addresses}
    offset = 0
    for d in docs:
        for pc, mode, bank, count, first_frame in d["sites"]:
            key = (pc, mode, bank)
            cur = sites.get(key)
            if cur is None:
                sites[key] = [count, offset + first_frame]
            else:
                cur[0] += count
        for p in d["pairs"]:
            pairs[tuple(p[:6])] += p[6]
        per_frame.extend(d["instructions"]["per_frame"])
        for a in watch:
            watch[a].extend(d["watch"]["per_frame"][a])
        offset += d["frames"]["count"]
    last = docs[-1]
    return {
        "schema_version": COVERAGE_SCHEMA_VERSION,
        "kind": "instruction_coverage",
        "scenario_id": scenario_id,
        "status": "complete",
        "failure": None,
        "rom": rom,
        "core": first["core"],
        "ring_capacity": max(d["ring_capacity"] for d in docs),
        "frames": {"start": 0, "end": offset - 1, "count": offset},
        "instructions": {"initial_total": first["instructions"]["initial_total"], "total": sum(per_frame),
                         "max_frame_delta": max(d["instructions"]["max_frame_delta"] for d in docs), "per_frame": per_frame},
        "site_fields": first["site_fields"],
        "sites": [[pc, mode, bank, v[0], v[1]] for (pc, mode, bank), v in sorted(sites.items())],
        "pair_fields": first["pair_fields"],
        "pairs": [[*k, n] for k, n in sorted(pairs.items())],
        "first_site": first.get("first_site"),
        "last_site": last.get("last_site"),
        "tail_sites": last.get("tail_sites", []),
        "watch": {"addresses": list(watch_addresses), "per_frame": watch},
        "merged_from": sources,
    }
