"""ROM-free tests of the track stream inventory (TRACK-BREADTH): header parsing, the shape
table, the tile-set producer on a synthetic ROM, and the tracked manifest's shape. The
manifest's values need the ROM: `content rnc-inventory --expect
tests/manifests/content/track-streams.json` checks them, outside this suite."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

from unirally_lab.content import provenance, tracks  # noqa: E402

MANIFEST = ROOT / "tests" / "manifests" / "content" / "track-streams.json"


def _header(shape: int, list_offset: int) -> bytearray:
    header = bytearray(15)
    header[3:5] = (0x023F).to_bytes(2, "little")
    header[5:7] = (0x005D).to_bytes(2, "little")
    header[7:9] = (0x0100).to_bytes(2, "little")
    header[9:11] = (0x0002).to_bytes(2, "little")
    header[11:13] = list_offset.to_bytes(2, "little")
    header[13] = shape
    return header


def _put(rom: bytearray, bus: int, data: bytes) -> None:
    offset = provenance.rom_file_offset(bus, len(rom))
    rom[offset:offset + len(data)] = data


class ShapeTests(unittest.TestCase):
    def test_every_arm_covers_the_same_playfield(self) -> None:
        for byte, (_, columns, rows) in tracks.SHAPES.items():
            self.assertEqual(columns * rows, 16384)
            self.assertEqual((columns // 4) & 0xFF, byte)


class HeaderTests(unittest.TestCase):
    def test_fields_and_tile_list(self) -> None:
        decoded = bytes(_header(0x20, 15)) + bytes([1, 2, 0x16, 0xFF, 0xFF])
        header = tracks.parse_header(decoded)
        self.assertEqual(header["player_start"], [0x023F, 0x005D])
        self.assertEqual(header["opponent_start"], [0x0100, 0x0002])
        self.assertEqual(header["tile_set_ids"], [1, 2, 0x16])
        self.assertEqual(header["shape"], {"byte": 0x20, "arm": "$81:A406", "columns": 128, "rows": 128})

    def test_rejects_unknown_shape_and_unterminated_list(self) -> None:
        with self.assertRaises(tracks.TrackError):
            tracks.parse_header(bytes(_header(0x01, 15)) + b"\xff")
        with self.assertRaises(tracks.TrackError):
            tracks.parse_header(bytes(_header(0x40, 15)) + b"\x01\x02")
        with self.assertRaises(tracks.TrackError):
            tracks.parse_header(bytes(10))


class TileContentTests(unittest.TestCase):
    def test_transfer_order_columns_and_flags(self) -> None:
        rom = bytearray(0x200000)
        # Tile set 3: nine tiles (two transfer groups) at $16:8000; its table at $17:A0C4.
        tile_bytes = bytes((i * 7) & 0xFF for i in range(9 * 128))
        _put(rom, tracks.TILE_DIRECTORY_BUS + 5 * 3, bytes([0x16, 0x00, 0x80]) + (9 * 128).to_bytes(2, "little"))
        _put(rom, 0x168000, tile_bytes)
        _put(rom, tracks.TABLE_DIRECTORY_BUS + 4 * 3, bytes([0xC4, 0xA0, 0x17, 0x00]))
        columns = bytes(i & 0xFF for i in range(9 * 32))
        _put(rom, 0x17A0C4, columns)
        flags = bytes(range(40, 49))
        _put(rom, tracks.FLAGS_BASE_BUS + (0xA0C4 - 0xA0A4) // 32, flags)
        derived = tracks.tile_content(bytes(rom), [3])
        self.assertEqual(derived["tile_columns"], columns)
        self.assertEqual(derived["tile_flags"], flags)
        # First group of eight: tile k's top half at 64k, bottom half at 8*64 + 64k.
        self.assertEqual(derived["bg1_tiles"][:128], tile_bytes[0:64] + tile_bytes[512:576])
        # Second group holds one tile: top half then bottom half right after it.
        self.assertEqual(derived["bg1_tiles"][8 * 128:], tile_bytes[1024:1088] + tile_bytes[1088:1152])


class PackProfileTests(unittest.TestCase):
    """Profile v10's added entries (still in v11): the rules file and the loader's compiled table agree."""

    def test_rules_and_compiled_table_agree(self) -> None:
        import hashlib
        import re
        rules_path = ROOT / "tests" / "manifests" / "content" / "classic-crawler-tracks-pack.json"
        rules = json.loads(rules_path.read_text(encoding="utf-8"))
        added = [e for e in rules["entries"] if e["id"].startswith(("track.", "scenery.")) or e["id"] == "presentation.classic.track-names.v1"]
        source = (ROOT / "src" / "core" / "content_pack.cpp").read_text(encoding="utf-8")
        compiled = []
        for name in ("tracks_required{{", "locked_tracks_required{{"):  # v10's, then v12's
            table = source[source.index(name):]
            table = table[:table.index("}};")]
            compiled += [(i, int(n), d) for i, n, d in re.findall(r'\{"([^"]+)",\s*(\d+),\s*"([0-9a-f]{64})"\}', table)]
        self.assertEqual(compiled, [(e["id"], e["size"], e["sha256"]) for e in added])
        self.assertIn(hashlib.sha256(rules_path.read_bytes()).hexdigest(), source)
        self.assertEqual(rules["profile_id"], "classic.pal.crawler.tracks.v16")
        ids = {e["id"] for e in added}
        for index in tracks.NEW_RACE_TRACKS + tracks.LOCKED_RACE_TRACKS:
            for part in ("data", "tile-columns", "tile-flags", "bg1-tiles"):
                self.assertIn(f"track.{index:02d}.{part}", ids)
            for part in ("bg2-tiles", "bg2-map", "palette"):
                self.assertIn(f"scenery.{tracks.scenery(index):02d}.{part}", ids)


class SpecialTileEntryTests(unittest.TestCase):
    """Profile v11's added entry (R-0047): the corkscrew heights at $00:8088, both tables."""

    def test_rules_and_compiled_table_agree(self) -> None:
        import re
        rules = json.loads((ROOT / "tests" / "manifests" / "content" / "classic-crawler-tracks-pack.json").read_text(encoding="utf-8"))
        entry = next(e for e in rules["entries"] if e["id"] == "zoom.corkscrew-heights")
        self.assertEqual(entry["source"], {"kind": "raw", "pieces": [{"file_offset": 0x88, "length": 96}]})
        source = (ROOT / "src" / "core" / "content_pack.cpp").read_text(encoding="utf-8")
        table = source[source.index("special_tiles_required{{"):]
        table = table[:table.index("}};")]
        compiled = re.findall(r'\{"([^"]+)",\s*(\d+),\s*"([0-9a-f]{64})"\}', table)
        self.assertEqual(compiled, [(entry["id"], str(entry["size"]), entry["sha256"])])


class LoopEntryTests(unittest.TestCase):
    """Profile v13's added entry (R-0051): the loop's 17 signed x steps at $81:834C."""

    def test_rules_and_compiled_table_agree(self) -> None:
        import re
        rules = json.loads((ROOT / "tests" / "manifests" / "content" / "classic-crawler-tracks-pack.json").read_text(encoding="utf-8"))
        entry = next(e for e in rules["entries"] if e["id"] == "zoom.loop-offsets")
        self.assertEqual(entry["source"], {"kind": "raw", "pieces": [{"file_offset": 0x834C, "length": 34}]})
        source = (ROOT / "src" / "core" / "content_pack.cpp").read_text(encoding="utf-8")
        table = source[source.index("loop_required{{"):]
        table = table[:table.index("}};")]
        compiled = re.findall(r'\{"([^"]+)",\s*(\d+),\s*"([0-9a-f]{64})"\}', table)
        self.assertEqual(compiled, [(entry["id"], str(entry["size"]), entry["sha256"])])


class HunterEntryTests(unittest.TestCase):
    """Profile v14's added entries (R-0052): the HUNTER blink pattern and opponent palette."""

    def test_rules_and_compiled_table_agree(self) -> None:
        import re
        rules = json.loads((ROOT / "tests" / "manifests" / "content" / "classic-crawler-tracks-pack.json").read_text(encoding="utf-8"))
        by_id = {e["id"]: e for e in rules["entries"]}
        blink, palette = by_id["zoom.hunter-blink"], by_id["presentation.classic.hunter-opponent-palette.v1"]
        self.assertEqual(blink["id"], "zoom.hunter-blink")
        self.assertEqual(blink["source"], {"kind": "raw", "pieces": [{"file_offset": 0x1D3BC, "length": 64}]})
        self.assertEqual(palette["id"], "presentation.classic.hunter-opponent-palette.v1")
        self.assertEqual(palette["source"], {"kind": "raw", "pieces": [{"file_offset": 0x20400, "length": 32}]})
        source = (ROOT / "src" / "core" / "content_pack.cpp").read_text(encoding="utf-8")
        table = source[source.index("hunter_required{{"):]
        table = table[:table.index("}};")]
        compiled = re.findall(r'\{"([^"]+)",\s*(\d+),\s*"([0-9a-f]{64})"\}', table)
        self.assertEqual(compiled, [(e["id"], str(e["size"]), e["sha256"]) for e in (blink, palette)])


class FrontEndEntryTests(unittest.TestCase):
    """Profile v15's added entries (R-0054): the boot screens', the title's and the main menu's
    assets and tables, last in the rules and compiled in the same order."""

    def test_rules_and_compiled_table_agree(self) -> None:
        import re
        from tools.unirally_lab.content import front_end
        rules = json.loads((ROOT / "tests" / "manifests" / "content" / "classic-crawler-tracks-pack.json").read_text(encoding="utf-8"))
        expected = [f"front-end.asset.{asset:03d}" for asset in front_end.FRONT_END_ASSETS]
        expected += [table[0] for table in front_end.FRONT_END_TABLES]
        by_id = {e["id"]: e for e in rules["entries"]}
        added = [by_id[entry_id] for entry_id in expected]
        rider_menu = len(front_end.RIDER_MENU_ASSETS) + len(front_end.RIDER_MENU_TABLES)
        self.assertEqual(rules["entries"][-len(added) - rider_menu:-rider_menu], added)
        source = (ROOT / "src" / "core" / "content_pack.cpp").read_text(encoding="utf-8")
        table = source[source.index("front_end_required{{"):]
        table = table[:table.index("}};")]
        compiled = re.findall(r'\{"([^"]+)",\s*(\d+),\s*"([0-9a-f]{64})"\}', table)
        self.assertEqual(compiled, [(e["id"], str(e["size"]), e["sha256"]) for e in added])
        for entry in added:
            self.assertEqual(entry["source"]["kind"], "raw")


class RiderMenuEntryTests(unittest.TestCase):
    """Profile v16's added entries (R-0055): the rider menu's palettes, names, title and the
    decoration animator's tables, last in the rules and compiled in the same order."""

    def test_rules_and_compiled_table_agree(self) -> None:
        import re
        from tools.unirally_lab.content import front_end
        rules = json.loads((ROOT / "tests" / "manifests" / "content" / "classic-crawler-tracks-pack.json").read_text(encoding="utf-8"))
        expected = [f"front-end.asset.{asset:03d}" for asset in front_end.RIDER_MENU_ASSETS]
        expected += [table[0] for table in front_end.RIDER_MENU_TABLES]
        added = rules["entries"][-len(expected):]
        self.assertEqual([e["id"] for e in added], expected)
        source = (ROOT / "src" / "core" / "content_pack.cpp").read_text(encoding="utf-8")
        table = source[source.index("rider_menu_required{{"):]
        table = table[:table.index("}};")]
        compiled = re.findall(r'\{"([^"]+)",\s*(\d+),\s*"([0-9a-f]{64})"\}', table)
        self.assertEqual(compiled, [(e["id"], str(e["size"]), e["sha256"]) for e in added])
        for entry in added:
            self.assertEqual(entry["source"]["kind"], "raw")


class TrackedManifestTests(unittest.TestCase):
    def test_manifest_carries_no_payload_bytes(self) -> None:
        manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
        self.assertEqual(manifest["track_count"], len(manifest["streams"]))
        for stream in manifest["streams"]:
            self.assertEqual(set(stream["derived"]), {"bg1_tiles", "tile_columns", "tile_flags"})
            self.assertEqual(len(stream["header"]["unnamed_bytes"]["0-2"]), 6)


if __name__ == "__main__":
    unittest.main()
