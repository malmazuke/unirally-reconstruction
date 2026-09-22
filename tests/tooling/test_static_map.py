"""ROM-free checks for the static code map (STATIC-CODE-MAP).

Every test builds a synthetic 128 KiB image of banks $80-$83 and a minimal
map-schema-1 document; nothing here reads the game ROM. The last class checks
the tracked outputs under ``docs/map/static/`` for the no-ROM-bytes rule.
"""

from __future__ import annotations

import json
import re
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

from unirally_lab.coverage import static_map as sm  # noqa: E402
from unirally_lab.coverage.opcodes import MODE_M, MODE_X, TABLE, mode_from_flags  # noqa: E402

STATIC = ROOT / "docs" / "map" / "static"
NMX8 = mode_from_flags(0x30, 0)          # native, 8-bit A and index


def image(patches: dict[int, bytes]) -> bytes:
    rom = bytearray(sm.SPAN)             # BRK everywhere: no decoding runs on into filler
    for off, data in patches.items():
        rom[off:off + len(data)] = data
    return bytes(rom)


def doc(ranges: list[tuple[int, int, int, list[str]]], entries: list[tuple[int, list[str], dict[str, int]]] = (),
        refs: list[int] = ()) -> dict:
    return {
        "scenario_id": "synthetic", "rom": {"sha256": "0" * 64, "size": sm.SPAN},
        "coverage": {"sha256": "1" * 64},
        "vectors": [],
        "ranges": [{"start": sm.addr(s), "end": sm.addr(e), "instructions": n, "modes": modes} for s, e, n, modes in ranges],
        "entry_points": [{"address": sm.addr(o), "region": "rom", "modes": m, "kinds": k} for o, m, k in entries],
        "multi_mode_addresses": [],
        "static_references": [{"address": sm.addr(o), "kinds": ["data"], "region": "rom"} for o in refs],
    }


def analyse(rom: bytes, d: dict, sites: list[tuple[int, int]] | None = None) -> sm.Analysis:
    a = sm.Analysis(rom, [d])
    if sites is not None:
        a.load_sites([{"sites": [[0x800000 | ((o >> 15) << 16) | 0x8000 | (o & 0x7FFF), m, 0x80, 1, 0] for o, m in sites]}])
    return a.run()


class DescentTests(unittest.TestCase):
    def test_descent_follows_a_call_from_observed_code(self) -> None:
        # $80:8000 JSR $8100 ; RTS (observed) -> $80:8100 LDA #$12 ; RTS (inferred, 8-bit A)
        rom = image({0x0000: bytes([0x20, 0x00, 0x81, 0x60]), 0x0100: bytes([0xA9, 0x12, 0x60])})
        a = analyse(rom, doc([(0x0000, 0x0003, 2, ["Nmx"])]), sites=[(0x0000, NMX8), (0x0003, NMX8)])
        cls = a.classes()
        self.assertEqual(cls[0x0000:0x0004], ["observed"] * 4)
        self.assertEqual(a.ins[0x0100].length, 2)
        self.assertEqual(a.ins[0x0100].source, "descent")
        self.assertEqual(cls[0x0100:0x0103], ["inferred"] * 3)
        self.assertIn((0x0000, "JSR"), a.callers[0x0100])

    def test_rep_and_sep_propagate_the_accumulator_width(self) -> None:
        # REP #$20 ; LDA #$1234 ; SEP #$20 ; LDA #$12 ; RTS, called in 8-bit mode
        body = bytes([0xC2, 0x20, 0xA9, 0x34, 0x12, 0xE2, 0x20, 0xA9, 0x12, 0x60])
        rom = image({0x0000: bytes([0x20, 0x00, 0x81, 0x60]), 0x0100: body})
        a = analyse(rom, doc([(0x0000, 0x0003, 2, ["Nmx"])]), sites=[(0x0000, NMX8), (0x0003, NMX8)])
        self.assertEqual([a.ins[o].length for o in (0x0100, 0x0102, 0x0105, 0x0107, 0x0109)], [2, 3, 2, 2, 1])
        self.assertFalse(a.ins[0x0102].modes & {MODE_M | MODE_X, MODE_M})

    def test_an_unknown_width_is_marked_not_guessed(self) -> None:
        # Observed CLC ; JMP $8100, the JMP recorded under both accumulator widths: LDA # there is ambiguous.
        rom = image({0x0000: bytes([0x18, 0x4C, 0x00, 0x81]), 0x0100: bytes([0xA9, 0x12, 0x34, 0x60])})
        wide = mode_from_flags(0x10, 0)
        a = analyse(rom, doc([(0x0000, 0x0003, 2, ["NMx", "Nmx"])]), sites=[(0x0000, NMX8), (0x0001, NMX8), (0x0001, wide)])
        self.assertNotIn(0x0100, a.ins)
        self.assertIn(0x0100, a.ambiguities)
        self.assertEqual(a.classes()[0x0100], "unknown")

    def test_a_routine_reaching_an_implausible_opcode_is_rejected_whole(self) -> None:
        # JSR $8100 where $8100 is LDA #$12 ; BRK: nothing of it is decoded.
        rom = image({0x0000: bytes([0x20, 0x00, 0x81, 0x60]), 0x0100: bytes([0xA9, 0x12, 0x00, 0x00])})
        a = analyse(rom, doc([(0x0000, 0x0003, 2, ["Nmx"])]), sites=[(0x0000, NMX8), (0x0003, NMX8)])
        self.assertNotIn(0x0100, a.ins)
        self.assertEqual(a.stops["implausible opcode"], 1)


class GapAndDataTests(unittest.TestCase):
    def test_gap_sweep_lists_candidates_without_classifying_them(self) -> None:
        # Unreferenced after an observed RTS: CLC ; LDA $10 ; STA $12 ; RTS decodes alike in every mode.
        rom = image({0x0000: bytes([0x60]), 0x0001: bytes([0x18, 0xA5, 0x10, 0x85, 0x12, 0x60]), 0x0007: bytes([0x00] * 16)})
        a = analyse(rom, doc([(0x0000, 0x0000, 1, ["Nmx"])]), sites=[(0x0000, NMX8)])
        self.assertIn(0x0001, a.candidates)
        self.assertEqual(a.classes()[0x0001], "unknown")

    def test_gap_sweep_marks_mode_dependent_decodings(self) -> None:
        # LDA #imm decodes to different routines under 8- and 16-bit A.
        rom = image({0x0000: bytes([0x60]), 0x0001: bytes([0xA9, 0x60, 0x18, 0x18, 0x18, 0x60]), 0x0007: bytes([0x00] * 16)})
        a = analyse(rom, doc([(0x0000, 0x0000, 1, ["Nmx"])]), sites=[(0x0000, NMX8)])
        self.assertNotIn(0x0001, a.candidates)
        self.assertIn(0x0001, a.ambiguities)

    def test_operands_classify_data(self) -> None:
        # Observed reference to $80:8200; inferred LDA $808300 (long) names $80:8300.
        rom = image({0x0000: bytes([0x20, 0x00, 0x81, 0x60]), 0x0100: bytes([0xAF, 0x00, 0x83, 0x80, 0x60]),
                     0x0200: bytes([0x00] * 4), 0x0300: bytes([0x00] * 4)})
        a = analyse(rom, doc([(0x0000, 0x0003, 2, ["Nmx"])], refs=[0x0200]), sites=[(0x0000, NMX8), (0x0003, NMX8)])
        cls = a.classes()
        self.assertEqual((cls[0x0200], cls[0x0300]), ("data", "data"))
        self.assertEqual(sum(cls.count(c) for c in sm.CLASSES), sm.SPAN)


class AgreementTests(unittest.TestCase):
    def test_range_tiling_fixes_boundaries_the_sites_confirm(self) -> None:
        # LDA #$12 ; RTS: one tiling of 2 instructions under 8-bit A.
        rom = image({0x0000: bytes([0xA9, 0x12, 0x60])})
        a = analyse(rom, doc([(0x0000, 0x0002, 2, ["Nmx"])]), sites=[(0x0000, NMX8), (0x0002, NMX8)])
        m = a.tiling["maps"][0]
        self.assertEqual((m["one_tiling"], m["shared_instructions_checked_against_sites"], m["disagreements"]), (1, 2, 0))
        self.assertEqual(sm.check_sites(a, {"sites": [[0x808000, NMX8, 0x80, 1, 0]]})["disagreements"], 0)

    def test_a_site_of_another_length_is_a_disagreement(self) -> None:
        rom = image({0x0000: bytes([0xA9, 0x12, 0x60])})
        a = analyse(rom, doc([(0x0000, 0x0002, 2, ["Nmx"])]), sites=[(0x0000, NMX8), (0x0002, NMX8)])
        wide = mode_from_flags(0x10, 0)   # 16-bit A: LDA # would be three bytes
        self.assertEqual(sm.check_sites(a, {"sites": [[0x808000, wide, 0x80, 1, 0]]})["disagreements"], 1)

    def test_without_sites_the_shared_tiling_is_placed(self) -> None:
        rom = image({0x0000: bytes([0xA9, 0x12, 0x60])})
        a = analyse(rom, doc([(0x0000, 0x0002, 2, ["Nmx", "NMx"])]))
        self.assertEqual(a.tiling["source"], "range tiling")
        self.assertEqual((a.ins[0x0000].length, a.ins[0x0002].length), (2, 1))


class TrackedStaticMapTests(unittest.TestCase):
    # Edge kinds (JSR, JMP, RTS ...) are map-schema vocabulary shared with docs/map/*.map.json, not listing text.
    KINDS = {"JSR", "JSL", "JMP", "JML", "RTS", "RTL", "RTI", "BRK", "COP"}
    MNEMONICS = re.compile(r'"(?:' + "|".join(sorted({m for m, _ in TABLE} - KINDS)) + r')(?: [^"]*)?"')

    def test_tracked_static_outputs_carry_no_rom_bytes_or_mnemonics(self) -> None:
        paths = sorted(STATIC.glob("*.json"))
        self.assertEqual([p.name for p in paths], ["code-banks.map.json", "labels.json"])
        for path in paths:
            with self.subTest(path=path.name):
                text = path.read_text()
                for key in ("opcode", "mnemonic", "bytes_hex", "operand", "instructions_listing"):
                    self.assertNotIn(f'"{key}"', text)
                self.assertIsNone(self.MNEMONICS.search(text))
                self.assertLessEqual(path.stat().st_size, 1 << 20)
                self.assertEqual(sm.dump(json.loads(text)), text)
        doc = json.loads((STATIC / "code-banks.map.json").read_text())
        self.assertEqual(sum(b[c] for b in doc["banks"] for c in sm.CLASSES), sm.SPAN)
        self.assertEqual(doc["totals"]["bytes"], sm.SPAN)
        self.assertIn(f"{doc['totals']['unknown_share']:.1%}", (STATIC / "code-banks.md").read_text())


if __name__ == "__main__":
    unittest.main()
