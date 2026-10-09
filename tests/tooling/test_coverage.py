"""ROM-free checks for the instruction-coverage tools (M1-01).

Nothing here loads the core or the ROM. The opcode-length table is checked
for every opcode under every M/X combination against hand-derived
expectations; the per-frame drain runs against a stub ring with the real
ring semantics (newest entries kept, oldest first on read) including the
overflow failure; the LoROM mapping, the edge classification and the map
derivation run on a synthetic ROM image and a synthetic trace; the CLI
error paths are exercised through ``project.py``.
"""

from __future__ import annotations

import json
import struct
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

from unirally_lab import EXIT_FAILURE, EXIT_INVALID_INPUT, EXIT_MISSING_PREREQUISITE, EXIT_OK  # noqa: E402
from unirally_lab.coverage import derive, drain, mapping, opcodes  # noqa: E402
from unirally_lab.reference import bsnes, worker  # noqa: E402

PROJECT = ROOT / "tools" / "project.py"
MAP = ROOT / "docs" / "map"


def run_cli(*args: str) -> subprocess.CompletedProcess:
    return subprocess.run([sys.executable, str(PROJECT), *args], capture_output=True, text=True, timeout=300)


# ------------------------------------------------------------------ opcodes


class OpcodeTableTests(unittest.TestCase):
    def test_every_opcode_has_a_length_in_every_mode(self) -> None:
        for opcode in range(256):
            for mode in range(8):
                with self.subTest(opcode=hex(opcode), mode=opcodes.mode_name(mode)):
                    self.assertIn(opcodes.instruction_length(opcode, mode), (1, 2, 3, 4))

    def test_accumulator_immediates_follow_m(self) -> None:
        # ADC AND BIT CMP EOR LDA ORA SBC #imm: 2 bytes with M=1, 3 with M=0.
        for opcode in (0x69, 0x29, 0x89, 0xC9, 0x49, 0xA9, 0x09, 0xE9):
            with self.subTest(opcode=hex(opcode)):
                self.assertEqual(opcodes.TABLE[opcode][1], "immM")
                self.assertEqual(opcodes.instruction_length(opcode, opcodes.MODE_M), 2)
                self.assertEqual(opcodes.instruction_length(opcode, opcodes.MODE_M | opcodes.MODE_X), 2)
                self.assertEqual(opcodes.instruction_length(opcode, 0), 3)
                self.assertEqual(opcodes.instruction_length(opcode, opcodes.MODE_X), 3)
                self.assertEqual(opcodes.instruction_length(opcode, opcodes.MODE_E | opcodes.MODE_M | opcodes.MODE_X), 2)

    def test_index_immediates_follow_x(self) -> None:
        # LDY LDX CPY CPX #imm: 2 bytes with X=1, 3 with X=0; M is irrelevant.
        for opcode in (0xA0, 0xA2, 0xC0, 0xE0):
            with self.subTest(opcode=hex(opcode)):
                self.assertEqual(opcodes.TABLE[opcode][1], "immX")
                self.assertEqual(opcodes.instruction_length(opcode, opcodes.MODE_X), 2)
                self.assertEqual(opcodes.instruction_length(opcode, opcodes.MODE_M), 3)
                self.assertEqual(opcodes.instruction_length(opcode, 0), 3)

    def test_fixed_immediates_and_signatures(self) -> None:
        for opcode, name in ((0xC2, "REP"), (0xE2, "SEP"), (0x42, "WDM"), (0x00, "BRK"), (0x02, "COP")):
            with self.subTest(name=name):
                self.assertEqual(opcodes.TABLE[opcode][0], name)
                for mode in range(8):
                    self.assertEqual(opcodes.instruction_length(opcode, mode), 2)

    def test_hand_derived_lengths(self) -> None:
        expected = {0xEA: 1, 0x60: 1, 0x6B: 1, 0x40: 1, 0xCB: 1, 0xFB: 1, 0x4C: 3, 0x6C: 3, 0x7C: 3, 0x5C: 4, 0xDC: 3, 0x20: 3,
                    0xFC: 3, 0x22: 4, 0x80: 2, 0x82: 3, 0x62: 3, 0xF4: 3, 0xD4: 2, 0x44: 3, 0x54: 3, 0x8F: 4, 0x9F: 4, 0xAF: 4,
                    0xBF: 4, 0x85: 2, 0x95: 2, 0x92: 2, 0x87: 2, 0x97: 2, 0x83: 2, 0x93: 2, 0x8D: 3, 0x9D: 3, 0x99: 3, 0x96: 2}
        for opcode, length in expected.items():
            with self.subTest(opcode=hex(opcode)):
                for mode in range(8):
                    self.assertEqual(opcodes.instruction_length(opcode, mode), length)

    def test_column_regularity(self) -> None:
        # The ALU rows (ORA AND EOR ADC STA LDA CMP SBC) share addressing modes by low nibble.
        pattern = {0x1: "idpx", 0x3: "sr", 0x5: "dp", 0x7: "ildp", 0xD: "abs", 0xF: "long", 0x11: "idpy", 0x12: "idp",
                   0x13: "isry", 0x15: "dpx", 0x17: "ildpy", 0x19: "absy", 0x1D: "absx", 0x1F: "longx"}
        for row in range(0, 0x100, 0x20):
            for low, addressing in pattern.items():
                with self.subTest(opcode=hex(row + low)):
                    self.assertEqual(opcodes.TABLE[row + low][1], addressing)
        for row in (0x00, 0x20, 0x40, 0x60, 0xA0, 0xC0, 0xE0):  # column 9 immediates (0x80 is BIT #)
            self.assertEqual(opcodes.TABLE[row + 0x09][1], "immM")
        self.assertEqual(opcodes.TABLE[0x89], ("BIT", "immM"))
        self.assertEqual(sum(1 for m, a in opcodes.TABLE if a == "immM"), 8)
        self.assertEqual(sum(1 for m, a in opcodes.TABLE if a == "immX"), 4)

    def test_mode_packing(self) -> None:
        self.assertEqual(opcodes.mode_from_flags(0x30, 0), opcodes.MODE_M | opcodes.MODE_X)
        self.assertEqual(opcodes.mode_from_flags(0x05, 0), 0)
        self.assertEqual(opcodes.mode_from_flags(0x34, 1), 7)
        self.assertEqual(opcodes.mode_name(7), "Emx")
        self.assertEqual(opcodes.mode_name(0), "NMX")
        self.assertEqual(opcodes.mode_name(opcodes.MODE_M), "NmX")

    def test_sequential_successor_wraps_within_bank(self) -> None:
        self.assertEqual(opcodes.sequential_successor(0x80FFFE, 3), 0x800001)
        self.assertEqual(opcodes.sequential_successor(0x008858, 3), 0x00885B)

    def test_static_references(self) -> None:
        self.assertEqual(opcodes.static_reference(0x20, b"\x34\x12", 0x808000, 0x7E), ("code", 0x801234))
        self.assertEqual(opcodes.static_reference(0x22, b"\x34\x12\x83", 0x808000, 0x7E), ("code", 0x831234))
        self.assertEqual(opcodes.static_reference(0xAD, b"\x00\x10", 0x808000, 0x7E), ("data", 0x7E1000))
        self.assertEqual(opcodes.static_reference(0xAF, b"\x00\x10\x83", 0x808000, 0x7E), ("data", 0x831000))
        self.assertIsNone(opcodes.static_reference(0xBD, b"\x00\x10", 0x808000, 0x7E))  # indexed
        self.assertIsNone(opcodes.static_reference(0x6C, b"\x00\x10", 0x808000, 0x7E))  # indirect
        self.assertIsNone(opcodes.static_reference(0xF4, b"\x00\x10", 0x808000, 0x7E))  # PEA value
        self.assertIsNone(opcodes.static_reference(0xA5, b"\x10", 0x808000, 0x7E))      # direct page


# ------------------------------------------------------------------ mapping


class MappingTests(unittest.TestCase):
    def test_lorom_offsets_and_mirrors(self) -> None:
        size = 0x200000
        self.assertEqual(mapping.rom_offset(0x008000, size), 0)
        self.assertEqual(mapping.rom_offset(0x00FFFF, size), 0x7FFF)
        self.assertEqual(mapping.rom_offset(0x018000, size), 0x8000)
        self.assertEqual(mapping.rom_offset(0x3FFFFF, size), 0x1FFFFF)
        self.assertEqual(mapping.rom_offset(0x808000, size), 0)           # fast mirror
        self.assertEqual(mapping.rom_offset(0x408000, size), 0)           # 2 MiB mirror at bank $40
        self.assertEqual(mapping.rom_offset(0x6FFFFF, size), 0x17FFFF)
        self.assertEqual(mapping.rom_offset(0xEF8000, size), 0x178000)
        self.assertEqual(mapping.canonical_rom_address(0x8000), 0x018000)
        self.assertEqual(mapping.canonical_rom_address(0x1FFFFF), 0x3FFFFF)

    def test_non_rom_regions(self) -> None:
        for a, region in ((0x7E0000, "wram"), (0x7FFFFF, "wram"), (0x000199, "wram"), (0x801FFF, "wram"), (0x002100, "other"),
                          (0x004200, "other"), (0x700000, "other"), (0x7D8000, "other"), (0xF08000, "other"), (0x400000, "other")):
            with self.subTest(address=hex(a)):
                self.assertEqual(mapping.classify(a), region)
                self.assertIsNone(mapping.rom_offset(a, 0x200000))
        self.assertEqual(mapping.wram_offset(0x7F0001), 0x10001)
        self.assertEqual(mapping.wram_offset(0x800199), 0x199)


# -------------------------------------------------------------------- drain


class StubRing:
    """A trace ring with the core's semantics: keeps the newest ``capacity`` entries."""

    def __init__(self, capacity: int) -> None:
        self.capacity = capacity
        self.entries: list[bytes] = []
        self.total = 0

    def execute(self, pc: int, p: int = 0x30, e: int = 0, b: int = 0) -> None:
        self.entries.append(struct.pack("<IHHHHHBBBBHH", pc, 0, 0, 0, 0x1FF, 0, b, p, e, 0, 0, 0))
        self.entries = self.entries[-self.capacity:]
        self.total += 1

    def trace_total(self) -> int:
        return self.total

    def trace_read_raw(self, max_entries: int) -> bytes:
        return b"".join(self.entries[-max_entries:] if max_entries < len(self.entries) else self.entries)


class DrainTests(unittest.TestCase):
    def test_accumulates_newest_entries_per_frame_across_frames(self) -> None:
        ring = StubRing(8)
        d = drain.FrameDrain(ring, 8)
        for pc in (0x8000, 0x8003, 0x8005):
            ring.execute(pc)
        self.assertEqual(d.drain_frame(0), 3)
        for pc in (0x8000, 0x8003, 0x8005, 0x8000, 0x8003, 0x8005, 0x8100, 0x8102):  # exactly the capacity
            ring.execute(pc)
        self.assertEqual(d.drain_frame(1), 8)
        self.assertEqual(d.drain_frame(2), 0)
        doc = d.document({"rom": {"sha256": "a" * 64, "size": 1}, "core": {}, "script": {}}, (0, 2))
        drain.validate_document(doc)
        sites = {(s[0], s[1], s[2]): (s[3], s[4]) for s in doc["sites"]}
        self.assertEqual(sites[(0x8000, 3, 0)], (3, 0))
        self.assertEqual(sites[(0x8100, 3, 0)], (1, 1))
        pairs = {tuple(p[:6]): p[6] for p in doc["pairs"]}
        self.assertEqual(pairs[(0x8005, 3, 0, 0x8000, 3, 0)], 2)  # one within frame 1, one across the frame boundary
        self.assertEqual(pairs[(0x8100, 3, 0, 0x8102, 3, 0)], 1)
        self.assertEqual(doc["instructions"], {"initial_total": 0, "total": 11, "max_frame_delta": 8, "per_frame": [3, 8, 0]})
        self.assertEqual(doc["first_site"], [0x8000, 3, 0])
        self.assertEqual(doc["last_site"], [0x8102, 3, 0])
        self.assertEqual(doc["tail_sites"][-2:], [[0x8100, 3, 0], [0x8102, 3, 0]])

    def test_modes_and_data_bank_distinguish_sites(self) -> None:
        ring = StubRing(16)
        d = drain.FrameDrain(ring, 16)
        ring.execute(0x8000, p=0x30, e=1)
        ring.execute(0x8000, p=0x00, e=0)
        ring.execute(0x8000, p=0x00, e=0, b=0x7E)
        d.drain_frame(0)
        self.assertEqual(sorted(drain.split_key(k) for k in d.sites), [(0x8000, 0, 0), (0x8000, 0, 0x7E), (0x8000, 7, 0)])

    def test_overflow_fails_the_frame(self) -> None:
        ring = StubRing(4)
        d = drain.FrameDrain(ring, 4)
        for pc in range(5):
            ring.execute(0x8000 + pc)
        with self.assertRaises(drain.DrainOverflow):
            d.drain_frame(0)

    def test_short_ring_read_fails(self) -> None:
        ring = StubRing(4)
        d = drain.FrameDrain(ring, 8)  # the drain believes the ring is larger than it is
        for pc in range(6):
            ring.execute(0x8000 + pc)
        with self.assertRaises(drain.DrainOverflow):
            d.drain_frame(0)

    def test_document_validation(self) -> None:
        ring = StubRing(4)
        d = drain.FrameDrain(ring, 4)
        ring.execute(0x8000)
        d.drain_frame(0)
        doc = d.document({"rom": {}, "core": {}, "script": {}}, (0, 0))
        drain.validate_document(doc)
        for label, mutate in {
            "schema": lambda x: x.update(schema_version=2),
            "kind": lambda x: x.update(kind="samples"),
            "missing": lambda x: x.pop("pairs"),
            "site shape": lambda x: x.update(sites=[[1, 2, 3]]),
            "site range": lambda x: x.update(sites=[[0x1000000, 0, 0, 1, 0]]),
            "total": lambda x: x["instructions"].update(total=5),
        }.items():
            with self.subTest(case=label):
                bad = json.loads(json.dumps(doc))
                mutate(bad)
                with self.assertRaises(ValueError):
                    drain.validate_document(bad)

    def test_worker_rejects_bad_coverage_arguments(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            script = Path(tmp) / "s.json"
            script.write_text(json.dumps({"schema_version": 1, "frames": 10}))
            common = ["--core", str(Path(tmp) / "missing.dylib"), "--rom", str(Path(tmp) / "missing.sfc"), "--script", str(script),
                      "--samples-out", str(Path(tmp) / "out.json")]
            self.assertEqual(worker.main(common + ["--coverage-out", str(Path(tmp) / "c.json"), "--coverage-ring", "0"]), EXIT_INVALID_INPUT)
            self.assertEqual(worker.main(common + ["--frame-image", "10"]), EXIT_INVALID_INPUT)
            # With valid options the missing ROM is the first prerequisite reported.
            self.assertEqual(worker.main(common + ["--coverage-out", str(Path(tmp) / "c.json"), "--frame-image", "0"]), EXIT_MISSING_PREREQUISITE)


def drained(frames: list[list[int]], watch: list[int] | None = None, core: dict | None = None) -> dict:
    ring = StubRing(16)
    d = drain.FrameDrain(ring, 16, watch=watch)
    for i, pcs in enumerate(frames):
        for pc in pcs:
            ring.execute(pc)
        d.drain_frame(i)
    doc = d.document({"rom": {"sha256": "a" * 64, "size": 1}, "core": core or {"name": "bsnes", "commit": "c"}, "script": {}},
                     (0, len(frames) - 1))
    doc["status"] = "complete"
    return drain.validate_document(doc)


class MergeTests(unittest.TestCase):
    def test_runs_are_laid_end_to_end(self) -> None:
        a = drained([[0x8000, 0x8003], [0x8005]], watch=[0x8000])
        b = drained([[0x8100], [0x8000, 0x8003], [0x8000]], watch=[0x8000])
        sources = [{"path": "a", "sha256": "1"}, {"path": "b", "sha256": "2"}]
        m = drain.validate_document(drain.merge_documents([a, b], "corpus", sources))
        self.assertEqual(m["frames"], {"start": 0, "end": 4, "count": 5})
        self.assertEqual(m["instructions"]["per_frame"], [2, 1, 1, 2, 1])
        sites = {s[0]: (s[3], s[4]) for s in m["sites"]}
        self.assertEqual(sites[0x8000], (3, 0))       # counts summed; first frame from the earliest run
        self.assertEqual(sites[0x8100], (1, 2))       # b's frame 0 is frame 2 on the merged axis
        pairs = {(p[0], p[3]): p[6] for p in m["pairs"]}
        self.assertEqual(pairs[(0x8000, 0x8003)], 2)
        self.assertNotIn((0x8005, 0x8100), pairs)     # no pair across the runs
        self.assertEqual(m["watch"]["per_frame"]["32768"], [1, 0, 0, 1, 1])
        self.assertEqual((m["first_site"], m["last_site"]), (a["first_site"], b["last_site"]))
        self.assertEqual((m["scenario_id"], m["merged_from"]), ("corpus", sources))

    def test_inputs_must_agree(self) -> None:
        a = drained([[0x8000]])
        src = [{"path": "a"}, {"path": "b"}]
        for label, other in {
            "core": drained([[0x8000]], core={"name": "bsnes", "commit": "d"}),
            "watch": drained([[0x8000]], watch=[0x8000]),
            "status": dict(drained([[0x8000]]), status="overflow"),
            "rom": dict(drained([[0x8000]]), rom={"sha256": "b" * 64, "size": 1}),
        }.items():
            with self.subTest(case=label), self.assertRaises(ValueError):
                drain.merge_documents([a, other], "corpus", src)
        with self.assertRaises(ValueError):
            drain.merge_documents([a], "corpus", src)


class FramePngTests(unittest.TestCase):
    def test_png_structure_and_pixels(self) -> None:
        import zlib
        width, height, pitch = 2, 2, 12  # 4 bytes of row padding
        raw = bytes([0x11, 0x22, 0x33, 0, 0x44, 0x55, 0x66, 0, 0, 0, 0, 0,
                     0x77, 0x88, 0x99, 0, 0xAA, 0xBB, 0xCC, 0, 0, 0, 0, 0])
        png = bsnes.frame_png(width, height, pitch, raw)
        self.assertTrue(png.startswith(b"\x89PNG\r\n\x1a\n"))
        self.assertEqual(struct.unpack(">II", png[16:24]), (2, 2))
        idat_len = struct.unpack(">I", png[33:37])[0]
        self.assertEqual(png[37:41], b"IDAT")
        rows = zlib.decompress(png[41:41 + idat_len])
        self.assertEqual(rows, bytes([0, 0x33, 0x22, 0x11, 0x66, 0x55, 0x44, 0, 0x99, 0x88, 0x77, 0xCC, 0xBB, 0xAA]))
        self.assertTrue(png.endswith(b"IEND\xaeB`\x82"))


# ------------------------------------------------------------------- derive


def synthetic_rom() -> bytes:
    """A 64 KiB LoROM image: a program at $00:8000, a subroutine at $01:8000, vectors at 0x7FE0."""
    rom = bytearray(0x10000)
    prog = bytes([
        0x18,              # 8000 CLC
        0xFB,              # 8001 XCE          (E -> 0)
        0xC2, 0x30,        # 8002 REP #$30     (M=X=0)
        0xA9, 0x34, 0x12,  # 8004 LDA #$1234   (3 bytes, M=0)
        0x8D, 0x00, 0x10,  # 8007 STA $1000    (abs data reference, DBR)
        0x20, 0x20, 0x80,  # 800A JSR $8020
        0x22, 0x00, 0x80, 0x01,  # 800D JSL $018000
        0xE2, 0x20,        # 8011 SEP #$20     (M=1)
        0xA9, 0x56,        # 8013 LDA #$56     (2 bytes, M=1)
        0xD0, 0xFA,        # 8015 BNE $8011   (relative -6 from $8017)
        0xCB,              # 8017 WAI
        0x80, 0xFE,        # 8018 BRA $8018
    ])
    rom[0:len(prog)] = prog
    rom[0x20:0x22] = bytes([0xEA, 0x60])            # 8020 NOP; RTS
    rom[0x8000:0x8002] = bytes([0xEA, 0x6B])        # $01:8000 NOP; RTL
    rom[0x30:0x32] = bytes([0xEA, 0x40])            # 8030 NMI handler: NOP; RTI
    rom[0x7FC0:0x7FD5] = b"SYNTHETIC LAB ROM    "
    rom[0x7FEA:0x7FEC] = (0x8030).to_bytes(2, "little")  # native NMI
    rom[0x7FEE:0x7FF0] = (0x8040).to_bytes(2, "little")  # native IRQ (never executed)
    rom[0x7FFC:0x7FFE] = (0x8000).to_bytes(2, "little")  # emulation reset
    return bytes(rom)


class DeriveTests(unittest.TestCase):
    def setUp(self) -> None:
        self.rom = synthetic_rom()
        # Two frames of the synthetic program; frame 1 begins with the NMI waking WAI.
        ring2 = StubRing(64)
        d = drain.FrameDrain(ring2, 64)
        E, N16, NM8 = (0x34, 1), (0x00, 0), (0x20, 0)
        frame0 = [(0x8000, E), (0x8001, E), (0x8002, (0x34, 0)), (0x8004, N16), (0x8007, N16), (0x800A, N16), (0x8020, N16),
                  (0x8021, N16), (0x800D, N16), (0x018000, N16), (0x018001, N16), (0x8011, N16), (0x8013, NM8), (0x8015, NM8),
                  (0x8011, NM8), (0x8013, NM8), (0x8015, NM8), (0x8017, NM8)]
        frame1 = [(0x8030, NM8), (0x8031, NM8), (0x8018, NM8), (0x8018, NM8), (0x8018, NM8)]
        for pc, (p, e) in frame0:
            ring2.execute(pc, p=p, e=e, b=0x7E)
        d.drain_frame(0)
        for pc, (p, e) in frame1:
            ring2.execute(pc, p=p, e=e, b=0x7E)
        d.drain_frame(1)
        self.cov = drain.validate_document(d.document({"rom": {"sha256": "a" * 64, "size": len(self.rom)}, "core": {"name": "stub"},
                                                       "script": {"frames": 2, "path": None}, "status": "complete"}, (0, 1)))
        self.doc, self.detail = derive.build_map(self.cov, self.rom, "synthetic", {"name": "stub"}, "b" * 64, "cmd")

    def test_byte_classes_sum_to_rom_size(self) -> None:
        t = self.doc["totals"]
        self.assertEqual(t["executed_opcode_bytes"] + t["executed_operand_bytes"] + t["unclassified_bytes"], len(self.rom))
        # 8000-8019 is fully executed (26 bytes: 12 opcodes, 14 operands) plus 8020-8021, 8030-8031 and $01:8000-8001.
        self.assertEqual(t["executed_opcode_bytes"], 12 + 2 + 2 + 2)
        self.assertEqual(t["executed_operand_bytes"], 14)
        self.assertEqual(t["overlapping_bytes"], 0)

    def test_lengths_follow_observed_modes(self) -> None:
        by_addr = {e["address"]: e for e in self.detail}
        self.assertEqual(by_addr["$00:8004"]["lengths"], [3])   # LDA #imm with M=0
        self.assertEqual(by_addr["$00:8013"]["lengths"], [2])   # LDA #imm with M=1
        self.assertEqual(by_addr["$00:8005"]["kind"], "operand")
        self.assertEqual(by_addr["$00:8000"]["modes"], ["Emx"])
        self.assertEqual(by_addr["$00:8011"]["modes"], ["NMX", "NmX"])
        self.assertEqual([m["address"] for m in self.doc["multi_mode_addresses"]], ["$00:8011"])  # SEP executed with M=0 and M=1

    def test_edges_and_entry_points(self) -> None:
        entries = {e["address"]: e for e in self.doc["entry_points"]}
        self.assertEqual(entries["$00:8020"]["kinds"], {"JSR": 1})
        self.assertEqual(entries["$01:8000"]["kinds"], {"JSL": 1})
        self.assertEqual(entries["$00:800D"]["kinds"], {"RTS": 1})
        self.assertEqual(entries["$00:8011"]["kinds"], {"RTL": 1, "branch": 1})  # BNE taken once, then falls through
        self.assertEqual(entries["$00:8030"]["kinds"], {"interrupt entry": 1})
        self.assertEqual(entries["$00:8030"]["vector"], "native_nmi")
        self.assertEqual(entries["$00:8018"]["kinds"], {"RTI": 1, "branch": 2})
        self.assertNotIn("$00:8001", entries)  # sequential steps are not entry points
        t = self.doc["totals"]
        self.assertEqual(t["edges_by_kind"], {"JSR": 1, "JSL": 1, "RTS": 1, "RTL": 1, "RTI": 1, "branch": 3, "interrupt entry": 1})
        self.assertEqual(t["unknown_edges"], 0)
        self.assertEqual(t["sequential_steps"] + t["edge_steps"], 23 - 1)  # 23 instructions, 22 steps

    def test_vectors(self) -> None:
        vectors = {v["name"]: v for v in self.doc["vectors"]}
        self.assertEqual(vectors["emu_reset"]["target"], "$00:8000")
        self.assertTrue(vectors["emu_reset"]["executed"])
        self.assertEqual(vectors["emu_reset"]["count"], 1)
        self.assertEqual(vectors["native_nmi"], {**vectors["native_nmi"], "executed": True, "count": 1, "first_frame": 1, "modes": ["NmX"]})
        self.assertFalse(vectors["native_irq"]["executed"])
        self.assertEqual(vectors["native_irq"]["count"], 0)
        self.assertEqual(self.doc["coverage"]["first_site"], {"address": "$00:8000", "mode": "Emx"})

    def test_static_references_and_ranges(self) -> None:
        refs = {r["address"]: r for r in self.doc["static_references"]}
        self.assertEqual(refs["$7E:1000"]["kinds"], ["data"])
        self.assertEqual(refs["$7E:1000"]["region"], "wram")
        self.assertEqual(refs["$00:8020"]["kinds"], ["code"])
        self.assertTrue(refs["$00:8020"]["executed"])
        self.assertEqual(refs["$01:8000"]["kinds"], ["code"])
        starts = [r["start"] for r in self.doc["ranges"]]
        self.assertEqual(starts, ["$00:8000", "$00:8020", "$00:8030", "$01:8000"])
        first = self.doc["ranges"][0]
        self.assertEqual((first["end"], first["bytes"], first["instructions"], first["rom_offset"]), ("$00:8019", 26, 12, 0))
        self.assertEqual(self.doc["banks"][0]["bank"], "$00")

    def test_unknown_edge_from_non_rom_site(self) -> None:
        cov = json.loads(json.dumps(self.cov))
        cov["sites"].append([0x7E0100, 0, 0x7E, 1, 1])
        cov["pairs"].append([0x7E0100, 0, 0x7E, 0x8000, 0, 0x7E, 1])
        cov["instructions"]["total"] += 1
        cov["instructions"]["per_frame"][1] += 1
        doc, _ = derive.build_map(cov, self.rom, "synthetic", {}, "b" * 64, "cmd")
        self.assertEqual(len(doc["non_rom_sites"]), 1)
        self.assertEqual(doc["non_rom_sites"][0]["region"], "wram")
        self.assertEqual(doc["unknown_edges"], [{"from": "$7E:0100", "from_region": "wram", "from_mode": "NMX", "from_decoded": False,
                                                 "to": "$00:8000", "to_mode": "NMX", "count": 1}])

    def test_unknown_edge_from_a_rom_site(self) -> None:
        """A step from a decoded ROM instruction that no control-flow opcode or vector explains is an
        unknown edge from a ROM site: the case the tracked maps never show (R-0006 finding 7)."""
        cov = json.loads(json.dumps(self.cov))
        cov["pairs"].append([0x8004, 0, 0x7E, 0x8020, 0, 0x7E, 1])   # LDA #imm (3 bytes) stepping to $8020, not to $8007
        doc, _ = derive.build_map(cov, self.rom, "synthetic", {}, "b" * 64, "cmd")
        self.assertEqual(doc["totals"]["unknown_edges"], 1)
        self.assertEqual(doc["totals"]["edges_by_kind"]["unknown"], 1)
        self.assertEqual(doc["unknown_edges"], [{"from": "$00:8004", "from_region": "rom", "from_mode": "NMX", "from_decoded": True,
                                                 "to": "$00:8020", "to_mode": "NMX", "count": 1}])
        entries = {e["address"]: e for e in doc["entry_points"]}
        self.assertEqual(entries["$00:8020"]["kinds"], {"JSR": 1, "unknown": 1})
        self.assertEqual(doc["non_rom_sites"], [])
        self.assertIn("| $00:8004 | rom | yes | $00:8020 | 1 |", derive.summary_markdown(doc, None))

    def test_per_frame_summary(self) -> None:
        """The per-frame vector summary behind ``nmi_vector_once_per_frame`` (M1-01 review 1)."""
        s = derive.per_frame_summary([0, 0, 1, 1, 1], 10)
        self.assertEqual(s, {"frames_with_zero": 2, "frames_with_one": 3, "frames_with_more": 0,
                             "first_frame": 12, "once_per_frame_from": 12, "gaps_after_first": []})
        # The race pattern: NMIs on, an NMI-off gap, then exactly one per frame to the end.
        s = derive.per_frame_summary([0, 1, 1, 0, 0, 1, 1], 0)
        self.assertEqual((s["first_frame"], s["once_per_frame_from"], s["gaps_after_first"]), (1, 5, [[3, 4]]))
        s = derive.per_frame_summary([1, 1, 0, 1, 0, 1], 0)
        self.assertEqual((s["once_per_frame_from"], s["gaps_after_first"]), (5, [[2, 2], [4, 4]]))
        # No tail of ones: a silent last frame, or a silent series.
        self.assertIsNone(derive.per_frame_summary([1, 1, 0], 0)["once_per_frame_from"])
        self.assertEqual(derive.per_frame_summary([0, 0], 0), {"frames_with_zero": 2, "frames_with_one": 0, "frames_with_more": 0,
                                                                 "first_frame": None, "once_per_frame_from": None, "gaps_after_first": []})
        # Tightened: a frame with more than one entry anywhere voids the once-per-frame claim, wherever it is.
        for series in ([0, 2, 1, 1], [1, 1, 2], [2, 1, 1, 1], [0, 1, 3, 0, 1]):
            s = derive.per_frame_summary(series, 0)
            self.assertEqual(s["frames_with_more"], 1, series)
            self.assertIsNone(s["once_per_frame_from"], series)
        self.assertEqual(derive.per_frame_summary([0, 2, 1, 1], 0)["first_frame"], 1)

    def test_interrupt_entry_takes_precedence_over_control_flow_opcode(self) -> None:
        cov = json.loads(json.dumps(self.cov))
        # An NMI arriving right after the JSR at $800A: the next traced pc is the handler, not $8020.
        cov["pairs"].append([0x800A, 0, 0x7E, 0x8030, 0, 0x7E, 1])
        doc, _ = derive.build_map(cov, self.rom, "synthetic", {}, "b" * 64, "cmd")
        entries = {e["address"]: e for e in doc["entry_points"]}
        self.assertEqual(entries["$00:8030"]["kinds"], {"interrupt entry": 2})

    def test_dump_and_compare(self) -> None:
        text = derive.dump_map(self.doc)
        self.assertEqual(json.loads(text), self.doc)
        self.assertEqual(text, derive.dump_map(json.loads(text)))
        cov = json.loads(json.dumps(self.cov))
        cov["sites"].append([0x8040, 0, 0x7E, 1, 1])  # the IRQ target, executed once, reached by an unknown edge
        cov["pairs"].append([0x8018, 2, 0x7E, 0x8040, 0, 0x7E, 1])
        cov["instructions"]["total"] += 1
        cov["instructions"]["per_frame"][1] += 1
        bigger, _ = derive.build_map(cov, self.rom, "synthetic-2", {}, "c" * 64, "cmd")
        cmp = derive.compare_maps(bigger, self.doc)
        self.assertEqual(cmp["bytes_only_here"], 2)  # the byte at $8040 is 0x00: a two-byte BRK
        self.assertEqual(cmp["new_ranges"], [{"start": "$00:8040", "end": "$00:8041", "bytes": 2}])
        self.assertEqual([e["address"] for e in cmp["new_entry_points"]], ["$00:8040"])
        self.assertEqual(cmp["bytes_only_in_baseline"], 0)
        md = derive.summary_markdown(bigger, cmp)
        self.assertIn("## Compared to `synthetic`", md)
        self.assertIn("2,097,152" if len(self.rom) == 0x200000 else "65,536", md)


# ---------------------------------------------------------------------- CLI


class CliTests(unittest.TestCase):
    def test_map_without_coverage_is_missing(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            r = run_cli("coverage", "map", "--coverage", str(Path(tmp) / "none.json"), "--out", str(Path(tmp) / "m.json"),
                        "--report", str(Path(tmp) / "r.json"))
            self.assertEqual(r.returncode, EXIT_MISSING_PREREQUISITE, r.stderr)
            rep = json.loads((Path(tmp) / "r.json").read_text())
            self.assertEqual([c["outcome"] for c in rep["checks"]], ["missing"])

    def test_map_with_invalid_coverage_is_invalid(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            bad = Path(tmp) / "bad.json"
            bad.write_text(json.dumps({"schema_version": 1, "kind": "instruction_coverage", "sites": [[1, 2, 3]]}))
            r = run_cli("coverage", "map", "--coverage", str(bad), "--out", str(Path(tmp) / "m.json"))
            self.assertEqual(r.returncode, EXIT_INVALID_INPUT, r.stderr)
            failed = Path(tmp) / "failed.json"
            failed.write_text(json.dumps({"schema_version": 1, "kind": "instruction_coverage", "status": "failed", "failure": "overflow",
                                          "rom": {}, "core": {}, "frames": {}, "ring_capacity": 1, "sites": [], "pairs": [],
                                          "instructions": {"total": 0, "per_frame": []}}))
            r = run_cli("coverage", "map", "--coverage", str(failed), "--out", str(Path(tmp) / "m.json"))
            self.assertEqual(r.returncode, EXIT_INVALID_INPUT, r.stderr)

    def test_capture_with_unreachable_manifest(self) -> None:
        r = run_cli("coverage", "capture", "--manifest", str(ROOT / "tests" / "manifests" / "replay" / "unreachable.json"),
                    "--out", str(ROOT / "artifacts" / "test-coverage-unreachable"), "--ring", "0")
        self.assertEqual(r.returncode, EXIT_INVALID_INPUT, r.stderr)
        r = run_cli("coverage", "capture", "--manifest", str(ROOT / "tests" / "manifests" / "replay" / "does-not-exist.json"),
                    "--out", str(ROOT / "artifacts" / "test-coverage-unreachable"))
        self.assertEqual(r.returncode, EXIT_INVALID_INPUT, r.stderr)


class TrackedMapTests(unittest.TestCase):
    def test_tracked_maps_are_consistent_and_carry_no_rom_bytes(self) -> None:
        paths = sorted(MAP.glob("*.map.json"))
        self.assertTrue(paths)
        for path in paths:
            with self.subTest(map=path.name):
                doc = json.loads(path.read_text())
                self.assertEqual(doc["kind"], "code_map")
                self.assertEqual(doc["scenario_id"], path.name[:-len(".map.json")])
                t = doc["totals"]
                self.assertEqual(t["executed_opcode_bytes"] + t["executed_operand_bytes"] + t["unclassified_bytes"], t["rom_size"])
                self.assertEqual(sum(r["bytes"] for r in doc["ranges"]), t["executed_addresses_24bit"])
                self.assertEqual(sum(r["opcode_bytes"] for r in doc["ranges"]), sum(b["opcode_bytes"] for b in doc["banks"]))
                self.assertEqual(len(doc["entry_points"]), t["entry_points"])
                self.assertEqual(len(doc["vectors"]), 12)
                self.assertLessEqual(path.stat().st_size, 1 << 20)
                self.assertEqual(derive.dump_map(doc), path.read_text())
                for key in ("opcode", "mnemonic", "bytes_hex"):
                    self.assertNotIn(f'"{key}"', path.read_text())
                summary = path.with_name(path.name[:-len(".map.json")] + ".md")
                self.assertTrue(summary.is_file(), summary)
                self.assertIn(doc["coverage"]["sha256"][:16], summary.read_text())


if __name__ == "__main__":
    unittest.main()
