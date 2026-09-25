"""Front-end content (FRONT-END-MAIN-MENU, R-0054): what the boot screens, the title and the
main menu load. Every entry is raw ROM content: an asset of the directory at `$82:B332`
(loaded through `$82:B1DB`/`$82:B183`), or a table the front-end code reads in place.
"""
from __future__ import annotations

from typing import Any

from . import provenance
from .tracks import _asset_piece, _raw

# Assets the boot, the title and the main menu load, by id (R-0054's load table).
FRONT_END_ASSETS = (1, 2, 5, 27, 28, 31, 68, 69, 70, 72, 74, 77, 78, 80, 88, 89, 91)

# Tables read in place: (entry id, bus address, length; None = up to and including 0xFF).
# (`$80:A877` also DMAs 1,920 bytes from `$84:AAF8` to VRAM word 0x3D80: the first 1,920 bytes of
# asset 69, which loads at the same address, so no entry of its own.)
FRONT_END_TABLES = (
    # The palette `$80:A8A8` DMAs to CGRAM 0-107 (`$80:A8D0`).
    ("front-end.base-palette", 0x80A8D4, 216),
    # The text printer's character table (`$80:C414`, R-0053).
    ("front-end.character-table", 0x80C709, 256),
    # The main menu's text stream, printed by `$80:C3BC` from `$80:ACF1`.
    ("front-end.main-menu-text", 0x80AD1F, None),
    # The arrow's spin frames: tile by `$C6 >> 1` (`$80:FB08`).
    ("front-end.arrow-frames", 0x80FBC5, 16),
    # The arrow's column by menu entry, in units of 8 pixels (`$80:88BE`, read through `$AE`).
    ("front-end.menu-arrow-columns", 0x8088BE, 5),
    # The four colours the palette cycle rotates through CGRAM 108-111 (`$80:FA82`).
    ("front-end.cycle-colours", 0x80FACD, 16),
)


def table_entry(rom: bytes, entry_id: str, bus: int, length: int | None) -> dict[str, Any]:
    start = provenance.rom_file_offset(bus, len(rom))
    if length is None:
        length = rom.index(b"\xff", start) + 1 - start
    return _raw(entry_id, rom, [(start, length)])


def v15_new_entries(rom: bytes) -> list[dict[str, Any]]:
    """The entries profile v15 adds to v14 (FRONT-END-MAIN-MENU), in pack order."""
    entries = [_raw(f"front-end.asset.{asset:03d}", rom, [_asset_piece(rom, asset)]) for asset in FRONT_END_ASSETS]
    return entries + [table_entry(rom, *table) for table in FRONT_END_TABLES]
