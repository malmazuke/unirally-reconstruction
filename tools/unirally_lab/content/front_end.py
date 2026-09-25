"""Front-end content (FRONT-END-MAIN-MENU, R-0054; FRONT-END-1P-SETUP, R-0055): what the boot
screens, the title, the main menu and the one-player setup screens load. Every entry is raw ROM content: an asset of the directory at `$82:B332`
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
    # The four colours the palette cycle rotates through CGRAM 108-111 (`$80:FA82`): seven words,
    # read from word `$00C9` (0-3) on; the code follows them.
    ("front-end.cycle-colours", 0x80FACD, 14),
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


# Profile v16 (FRONT-END-1P-SETUP, R-0055): the rider menu. The riders' sprite palettes, assets 6-21
# (rider r is asset 6 + r): `$80:A705` DMAs riders 0-7 from `$80:A72B` every frame, the HDMA table
# `$80:CD84` carries riders 8-15, and `$83:91F7` loads the chosen rider's asset at colour 0xF0.
RIDER_MENU_ASSETS = tuple(range(6, 22))
RIDER_MENU_TABLES = (
    # The riders' name records: 22 of 16 bytes, which `$83:92CC` copies to SRAM `$77:000C` on a
    # cold start; `$80:9B2F` prints them (text code F8).
    ("front-end.rider-names", 0x83800C, 0x160),
    # The rider menu's title, printed by `$80:C3BC` from `$80:CB1C` ("PICK YOUR UNI").
    ("front-end.pick-rider-title", 0x80BCAF, None),
    # The main menu's decoration animator `$83:9A1E`: tiles by step (`$83:9AF9`, `$83:9B01`,
    # `$83:9B27`, `$83:9B31`) and x offsets (`$83:9B09`, `$83:9B13`).
    ("front-end.decoration-frames", 0x839AF9, 0x4C),
)
# Where the rider menu reads the palettes in place: they must equal the assets.
RIDER_PALETTES_DMA = 0x80A72B  # riders 0-7, 255 bytes (the last colour's high byte is not sent)
RIDER_PALETTES_HDMA = 0x80CD84  # 40 00 00, then 90 + 16 colours for riders 8-15, then 00


def check_rider_palettes(rom: bytes) -> None:
    """The in-place palette tables are the riders' assets (R-0055), so native reads the assets."""
    assets = {}
    for asset in RIDER_MENU_ASSETS:
        offset, length = _asset_piece(rom, asset)
        assets[asset] = rom[offset:offset + length]
    dma = provenance.rom_file_offset(RIDER_PALETTES_DMA, len(rom))
    if rom[dma:dma + 255] != b"".join(assets[6 + r] for r in range(8))[:255]:
        raise ValueError("$80:A72B is not riders 0-7's palettes")
    hdma = provenance.rom_file_offset(RIDER_PALETTES_HDMA, len(rom))
    table = rom[hdma:hdma + 3 + 8 * 33 + 1]
    if table[:3] != b"\x40\x00\x00" or table[-1] != 0:
        raise ValueError("$80:CD84 does not start with colour 0 and end the table")
    for r in range(8):
        block = table[3 + r * 33:3 + (r + 1) * 33]
        if block[0] != 0x90 or block[1:] != assets[14 + r]:
            raise ValueError(f"$80:CD84 block {r} is not rider {8 + r}'s palette")


def v16_new_entries(rom: bytes) -> list[dict[str, Any]]:
    """The entries profile v16 adds to v15 (FRONT-END-1P-SETUP), in pack order."""
    check_rider_palettes(rom)
    entries = [_raw(f"front-end.asset.{asset:03d}", rom, [_asset_piece(rom, asset)]) for asset in RIDER_MENU_ASSETS]
    return entries + [table_entry(rom, *table) for table in RIDER_MENU_TABLES]
