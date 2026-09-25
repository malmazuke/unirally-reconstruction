"""The address-to-native-symbol index (NATIVE-READABILITY).

The scanner tests use authored C++ snippets. The last class checks the tracked index
against ``src/core`` and holds the ROM citations without a record to a falling limit.
"""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

from unirally_lab.coverage import native_symbols as ns  # noqa: E402

# ROM addresses src/core cites that no research, task, inventory or content record
# cites (directly or inside a cited range). NATIVE-READABILITY part 2 brings this to
# zero; lower it as citations gain records, never raise it.
ROM_WITHOUT_RECORD_LIMIT = 8


def symbols(text: str) -> dict[str, list[str]]:
    """Address -> sorted symbols for one authored file."""
    fs, lines = ns.scan("x.cpp", text)
    out: dict[str, set[str]] = {}
    for _, symbol, note in ns.attach(fs, lines):
        for c in ns.cites(note):
            out.setdefault(c.address, set()).add(symbol)
    return {k: sorted(v) for k, v in out.items()}


class CitationTests(unittest.TestCase):
    def test_forms_and_ranges(self) -> None:
        got = ns.cites("$81:C238 and $81C241, $82:A5FA-A61E, $83:F09E-$83:F0B9, $00:8088")
        self.assertEqual([(c.region, c.address, c.end) for c in got], [
            ("rom", "$81:C238", None), ("rom", "$81:C241", None), ("rom", "$82:A5FA", "$82:A61E"),
            ("rom", "$83:F09E", "$83:F0B9"), ("rom", "$80:8088", None)])

    def test_wram_sram_and_values(self) -> None:
        got = ns.cites("$0C73, $7E2102, $7E:0C73, $770825, #$48, $31, $FFFF, $17:C7D6")
        self.assertEqual([(c.region, c.address) for c in got], [
            ("wram", "$7E:2102"), ("wram", "$0C73"), ("sram", "$77:0825"), ("rom", "$17:C7D6"), ("wram", "$0C73")])

    def test_colonless_long_form_only_for_code_and_wram_banks(self) -> None:
        self.assertEqual(ns.cites("$17C7D6"), [])


class ScannerTests(unittest.TestCase):
    def test_function_body_comment(self) -> None:
        src = "namespace unirally {\nvoid update() {\n    // $81:C238: the reward path\n    int x = 0;\n}\n}\n"
        self.assertEqual(symbols(src), {"$81:C238": ["update"]})

    def test_leading_block_with_an_empty_line_attaches_to_the_next_function(self) -> None:
        src = "// $81:C219 consumes the queue.\n//\n// More prose.\nvoid f(int a) {\n}\n"
        self.assertEqual(symbols(src), {"$81:C219": ["f"]})

    def test_blank_line_detaches_a_block(self) -> None:
        src = "// $81:C219 about the file.\n\nvoid f() {\n}\n"
        self.assertEqual(symbols(src), {"$81:C219": ["(file)"]})

    def test_member_and_constant(self) -> None:
        src = ("struct Rider {\n    // $0BCB/$0BCD: mud cooldown.\n    std::uint16_t mud{};\n"
               "    std::uint16_t a{}, b{}; // $0D57\n};\n"
               "constexpr unsigned shift = 6; // $81:8A3F-8A44\n")
        self.assertEqual(symbols(src), {"$0BCB": ["Rider::mud"], "$0BCD": ["Rider::mud"],
                                        "$0D57": ["Rider::a, Rider::b"], "$81:8A3F": ["shift"]})

    def test_braced_default_argument_is_not_a_body(self) -> None:
        src = "void f(int a,\n       const T& t = {}) {\n    // $82:A35B\n}\n"
        self.assertEqual(symbols(src), {"$82:A35B": ["f"]})

    def test_constructor_initialiser_braces_and_access_specifiers(self) -> None:
        src = ("class Reader {\npublic:\n    explicit Reader(S s) : s_{s} {\n        // $81:8000\n    }\n"
               "private:\n    // $0300\n    S s_;\n};\n")
        self.assertEqual(symbols(src), {"$81:8000": ["Reader::Reader"], "$0300": ["Reader::s_"]})

    def test_data_table_and_operator(self) -> None:
        src = ("constexpr std::array<int, 2> table{\n    1, // $83:9FFA\n    2,\n};\n"
               "struct S {\n    // $1275\n    bool operator==(const S&) const = default;\n};\n")
        self.assertEqual(symbols(src), {"$83:9FFA": ["table"], "$1275": ["S::operator=="]})

    def test_strings_do_not_open_scopes(self) -> None:
        src = 'void f() {\n    const char* s = "{ $81:9000 }";\n    // $81:9001\n}\n'
        self.assertEqual(symbols(src), {"$81:9001": ["f"]})

    def test_lookup_finds_a_range_holding_the_address(self) -> None:
        index = {"addresses": [{"address": "$81:C219", "region": "rom", "native": ["a.cpp:f"], "range_ends": ["$81:C2C9"]}],
                 "symbols": {"a.cpp:f": {"rom": ["$81:C219"]}}}
        got = ns.lookup(index, ROOT, "$81C23C")
        self.assertEqual([r["address"] for r in got["addresses"]], ["$81:C219"])


class TrackedIndexTests(unittest.TestCase):
    def setUp(self) -> None:
        self.index = ns.build(ROOT)

    def test_tracked_index_matches_src_core(self) -> None:
        tracked = (ROOT / ns.OUT).read_text(encoding="utf-8")
        self.assertEqual(tracked, ns.dump(self.index), f"{ns.OUT} is stale: run `{ns.REGENERATE}`")

    def test_every_citation_names_a_symbol(self) -> None:
        for row in self.index["addresses"]:
            self.assertTrue(row["native"], row["address"])
            for s in row["native"]:
                self.assertRegex(s, r"^src/core/[\w.]+:\S")

    def test_rom_citations_without_a_record_only_fall(self) -> None:
        missing = ns.rom_without_record(self.index, ROOT)
        self.assertLessEqual(len(missing), ROM_WITHOUT_RECORD_LIMIT,
                             f"ROM addresses cited in src/core by no record: {missing}; cite the record in "
                             "the comment's evidence line, or add the address to the record that recovered it")

    def test_static_map_names_native_symbols(self) -> None:
        doc = json.loads((ROOT / "docs/map/static/code-banks.map.json").read_text(encoding="utf-8"))
        self.assertEqual(doc["inputs"]["native_symbols"], ns.OUT)
        self.assertGreater(doc["totals"]["routines_cited_by_native_code"], 0)


if __name__ == "__main__":
    unittest.main()
