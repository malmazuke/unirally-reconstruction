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


# Profile v17 (FRONT-END-1P-SETUP part 2, R-0056): PICK TOUR, PICK TRACK and NOW PLAYING.
# Palettes: the medals' (assets 32-34 at colours 0x80-0xA0, `$80:9764`), the base palette's halves
# (35 at 0, 36 at 0x40, `$80:A858`), the done-track markers' (37 at 0xB0, `$80:E888`), and the
# computer riders' and SOMEONE's (22-26: rider 16-20's palette is asset 6 + rider, `$80:B3FC`).
# `$80:A82B` DMAs `$84:A378`, asset 68's last 1,920 bytes, so it needs no entry of its own.
TOUR_MENU_ASSETS = (22, 23, 24, 25, 26, 32, 33, 34, 35, 36, 37)
TOUR_MENU_TABLES = (
    # The medal object tiles `$83:94D0` DMAs to VRAM word 0x7A00, and PICK TRACK's set
    # (`$83:94FF`).
    ("front-end.medal-tiles", 0x87D5D8, 0xC00),
    ("front-end.track-menu-tiles", 0x87C9D8, 0xC00),
    # The four text streams `$80:E730` prints: the title and the left column (`$80:E7C4`), then
    # jumper and bounder, runner and sprinter, hunter.
    ("front-end.tour-menu-text", 0x80E7C4, 0x6A),
    # The badges' places in the text map (byte offsets, `$80:E82E`, hunter's at `$80:E83E`).
    ("front-end.tour-badge-places", 0x80E82E, 0x12),
    # The level each tour needs (`$80:E842`, as `$83:8E12`).
    ("front-end.tour-levels", 0x80E842, 10),
    # The arrow's targets by cursor, x and y words (`$80:E708`).
    ("front-end.tour-arrow-targets", 0x80E708, 40),
    # `$83:8D8F`'s badge pictures: palette by picture (`$83:8E1C`), then tile base words (`$83:8E26`).
    ("front-end.tour-badge-pictures", 0x838E1C, 30),
    # The medal objects' places, x, y and two unused bytes by tour (`$80:97DD`), then the attribute
    # by medal (`$80:9801`).
    ("front-end.medal-places", 0x8097DD, 36),
    ("front-end.medal-attributes", 0x809801, 4),
    # The tours' names (`$83:A1C8`, the strings `$83:A1B4` points to).
    ("front-end.tour-names", 0x83A1C8, 80),
    # PICK TRACK: the items' arrow columns (`$80:EA35`), the markers' rows (`$80:EA3B`) and the
    # pictures' places (`$80:EA40`); its text (`$80:EA58`); the medal words GOLD, SILVER, BRONZE
    # (`$80:EB06`); the markers' tiles (`$80:EB61`).
    ("front-end.track-menu-layout", 0x80EA35, 0x23),
    ("front-end.track-menu-text", 0x80EA58, None),
    ("front-end.medal-words", 0x80EB06, 0x1D),
    ("front-end.marker-tiles", 0x80EB61, 21),
    # NOW PLAYING: the race kind's words (`$80:B205`) and its streams and picture place
    # (`$80:B4F6-B57D`); a time's words `_quit___`, `_no_time` (`$80:FC5C`).
    ("front-end.race-kind-words", 0x80B205, 40),
    ("front-end.now-playing-text", 0x80B4F6, 0x88),
    ("front-end.time-words", 0x80FC5C, 18),
    # The race's laps by track (`$83:A254`) and the stunt events' qualifying scores by tour and
    # medal (`$83:A218`).
    ("front-end.laps", 0x83A254, 50),
    ("front-end.qualifying-scores", 0x83A218, 60),
)
TOUR_BADGE_TILES = 0x84A378  # `$80:A82B`'s source: asset 68's tail


def check_tour_badge_tiles(rom: bytes) -> None:
    """`$80:A82B` sends the end of asset 68 again (R-0056), so native reads the asset."""
    offset, length = _asset_piece(rom, 68)
    source = provenance.rom_file_offset(TOUR_BADGE_TILES, len(rom))
    if rom[source:source + 0x780] != rom[offset + length - 0x780:offset + length]:
        raise ValueError("$84:A378 is not asset 68's last 1,920 bytes")


def v17_new_entries(rom: bytes) -> list[dict[str, Any]]:
    """The entries profile v17 adds to v16 (FRONT-END-1P-SETUP part 2), in pack order."""
    check_tour_badge_tiles(rom)
    entries = [_raw(f"front-end.asset.{asset:03d}", rom, [_asset_piece(rom, asset)]) for asset in TOUR_MENU_ASSETS]
    return entries + [table_entry(rom, *table) for table in TOUR_MENU_TABLES]


# Profile v18 (FRONT-END-1P-CONTINUATION, R-0057): the one-run result screen's text (`$80:D187`:
# the track's name, COMPLETE, the headings and four rows) and its three trophies (`$80:D17B`,
# entries 112-114: x, y, tile, attribute).
RESULT_TABLES = (
    ("front-end.result-text", 0x80D187, None),
    ("front-end.result-icons", 0x80D17B, 12),
)


def v18_new_entries(rom: bytes) -> list[dict[str, Any]]:
    """The entries profile v18 adds to v17 (FRONT-END-1P-CONTINUATION), in pack order."""
    return [table_entry(rom, *table) for table in RESULT_TABLES]


# Profile v19 (FRONT-END-LAP-RESULT, R-0058): the lap result screen's text. `$80:910F` prints the
# headings, the graph's axes and TIME, its times, and "laps on" the track; `$80:918F` the track's
# record line; `$80:91A1` and `$80:91B9` the player's and the opponent's rows.
LAP_RESULT_TABLES = (
    ("front-end.lap-result-text", 0x80910F, None),
    ("front-end.lap-result-record", 0x80918F, None),
    ("front-end.lap-result-player", 0x8091A1, None),
    ("front-end.lap-result-opponent", 0x8091B9, None),
)


def v19_new_entries(rom: bytes) -> list[dict[str, Any]]:
    """The entries profile v19 adds to v18 (FRONT-END-LAP-RESULT), in pack order. Run once with the
    ROM to append them to the rules; the tests check the rules against the compiled table."""
    return [table_entry(rom, *table) for table in LAP_RESULT_TABLES]


# Profile v20 (FRONT-END-TOUR-END, R-0059): the medal award screen `$83:AEF6`. Its assets: the
# background's map and tiles (0x53, 0x54), the podium's map and tiles (0x65, 0x64) and colours
# (0x3B), the background's colours by medal (0x55 bronze, 0x56 silver), and the medal's object
# tiles (0x5C). The medal's second art, which `$83:B096-B111` DMAs from `$09:A658` and `$09:B658`
# (two 4 KiB halves, one span), and the award's tables at `$83:B120`: the two objects' first
# entries (8 bytes), the bounce (`$83:B129`, 25), the rider's poses by step (`$83:B142`, 58
# words) and the medal's tiles by step (`$83:B1B6`, 53).
AWARD_ASSETS = (0x3B, 0x53, 0x54, 0x55, 0x56, 0x5C, 0x64, 0x65)
AWARD_TABLES = (
    ("front-end.award-medal-art", 0x89A658, 0x2000),
    ("front-end.award-tables", 0x83B120, 0xCB),
)


def v20_new_entries(rom: bytes) -> list[dict[str, Any]]:
    """The entries profile v20 adds to v19 (FRONT-END-TOUR-END), in pack order. Run once with the
    ROM to append them to the rules; the tests check the rules against the compiled table."""
    entries = [_raw(f"front-end.asset.{asset:03d}", rom, [_asset_piece(rom, asset)]) for asset in AWARD_ASSETS]
    return entries + [table_entry(rom, *table) for table in AWARD_TABLES]


# Profile v21 (RACE-RIDERS-OPPONENTS, R-0061): what a one-player race takes from its pairing. The
# opponent's catch-up by track (`$83:C8B3`, 45 bytes): SILVIA's `$1283` is the track's byte + 0x20
# and GOLDWYN's + 0x40 (`$83:CC2B-CC56`). The riders' colour math (`$82:D4DC`, 4 bytes a rider:
# COLDATA's red, green and blue writes and CGADSUB, which HDMA channel 5 applies to the race's BG3
# ink, `$82:D57F-D5FB`).
RACE_PAIRING_TABLES = (
    ("race.opponent-catch-up", 0x83C8B3, 45),
    ("race.rider-colour-math", 0x82D4DC, 64),
)


def v21_new_entries(rom: bytes) -> list[dict[str, Any]]:
    """The entries profile v21 adds to v20 (RACE-RIDERS-OPPONENTS), in pack order. Run once with
    the ROM to append them to the rules; the tests check the rules against the compiled table."""
    return [table_entry(rom, *table) for table in RACE_PAIRING_TABLES]
