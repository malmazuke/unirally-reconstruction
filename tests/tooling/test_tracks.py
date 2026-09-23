"""Tests of the track stream inventory (TRACK-BREADTH): header parsing, the shape table and
the tile-set producer on a synthetic ROM, and, when the supported ROM is present, the
tracked stream manifest. The ROM case reports a skip when the ROM is absent; it is never
counted as a pass."""

from __future__ import annotations

import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

from unirally_lab.content import provenance, tracks  # noqa: E402

PROJECT = ROOT / "tools" / "project.py"
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


def _rom_path() -> Path | None:
    location = ROOT / "local" / "rom-location.txt"
    if not location.is_file():
        return None
    path = Path(location.read_text(encoding="utf-8").strip()).expanduser()
    return path if path.is_file() else None


class TrackedManifestTests(unittest.TestCase):
    @unittest.skipIf(_rom_path() is None, "supported ROM absent (local/rom-location.txt): tracked manifest not checked")
    def test_inventory_equals_tracked_manifest(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            report = Path(tmp) / "report.json"
            run = subprocess.run([sys.executable, str(PROJECT), "content", "rnc-inventory", "--expect", str(MANIFEST),
                                  "--report", str(report)], capture_output=True, text=True, cwd=ROOT, timeout=120)
            self.assertEqual(run.returncode, 0, run.stdout + run.stderr)
            self.assertEqual(json.loads(report.read_text())["track_count"], 45)

    def test_manifest_carries_no_payload_bytes(self) -> None:
        manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
        self.assertEqual(manifest["track_count"], len(manifest["streams"]))
        for stream in manifest["streams"]:
            self.assertEqual(set(stream["derived"]), {"bg1_tiles", "tile_columns", "tile_flags"})
            self.assertEqual(len(stream["header"]["unnamed_bytes"]["0-2"]), 6)


if __name__ == "__main__":
    unittest.main()
