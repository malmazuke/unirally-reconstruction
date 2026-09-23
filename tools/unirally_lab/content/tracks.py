"""Every track stream in the ROM, located and unpacked the way the race loader does it.

`$82:E140-E152` reads the track index from SRAM `$77:074A`, adds `$C2` and
passes the sum as an asset id to `$82:B2DD`, which resolves a five-byte entry
of the asset directory at `$82:B332` (bank with the `$80` compressed flag,
address, length) and, for a compressed entry, calls the RNC decompressor at
`$81:B8E2` (see ``rnc``). Track *i* is therefore asset `$C2 + i`. Assets
`$C2`-`$EE` are the 45 RNC method-1 streams in banks `$98`-`$9F`, in ROM order,
and the entries on either side are uncompressed (TRACK-BREADTH, R-0046).

The first 15 decoded bytes are the track header. The fields read here are the
ones the recovered code reads (R-0021, `track_geometry` and
`classic_race_start` in ``src/core/movement.cpp``):

- bytes 3-6 and 7-10: the player's and the opponent's start x and y words;
- bytes 11-12: the offset of the tile-set list, which `$82:E1A1-E1B5` walks
  up to its `$FF` terminator;
- byte 13: the playfield shape `$81:A304-A342` switches on (see ``SHAPES``).

Bytes 0-2 and 14 are recorded without a meaning.

For each tile-set id the loader resolves the tile directory at `$82:B7DD`
(five bytes per id: bank, address, length, 128 bytes per 16x16 tile) and the
table directory at `$17:A000` (four bytes per id: address, bank), and copies
``length / 4`` bytes of tile columns from the table pointer and one flag byte per
tile from `$17:C4E4 + (pointer - $A0A4) / 32` (R-0008 finding 4, R-0021). The
BG1 tile bytes are kept in the order of the loader's 64-byte VRAM transfers:
per group of up to eight tiles, each tile's top half then its bottom half.
"""

from __future__ import annotations

import hashlib
from typing import Any

from . import provenance, rnc

ASSET_DIRECTORY_BUS = 0x82B332
FIRST_TRACK_ASSET = 0xC2
TILE_DIRECTORY_BUS = 0x82B7DD
TABLE_DIRECTORY_BUS = 0x17A000
FLAGS_BASE_BUS = 0x17C4E4
HEADER_LENGTH = 15

# Byte 13 of the header -> the arm of `$81:A304-A342` it selects, as
# (columns, rows) of 64-unit coarse cells. Every arm covers 16,384 cells.
# Read statically from the listing: the 0x00 and 0x40 arms are observed
# (DRAGSTER and ZOOM ZOO), the others have not executed under capture. Any
# other value falls through to a BRK at `$81:A342`.
SHAPES = {
    0x00: ("$81:A4C1", 1024, 16),
    0x80: ("$81:A483", 512, 32),
    0x40: ("$81:A445", 256, 64),
    0x20: ("$81:A406", 128, 128),
    0x10: ("$81:A3C7", 64, 256),
    0x08: ("$81:A388", 32, 512),
    0x04: ("$81:A343", 16, 1024),
}


class TrackError(ValueError):
    pass


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def _rom(rom: bytes, bus: int, length: int) -> bytes:
    offset = provenance.rom_file_offset(bus, len(rom))
    if offset is None or offset + length > len(rom):
        raise TrackError(f"ROM range ${bus:06X}+{length} is unavailable")
    return rom[offset:offset + length]


def _u16(data: bytes, offset: int) -> int:
    return data[offset] | (data[offset + 1] << 8)


def asset_entry(rom: bytes, asset: int) -> dict[str, Any]:
    raw = _rom(rom, ASSET_DIRECTORY_BUS + 5 * asset, 5)
    return {"asset": asset, "bank": raw[0] & 0x7F, "compressed": bool(raw[0] & 0x80),
            "address": _u16(raw, 1), "length": _u16(raw, 3)}


def track_entry(rom: bytes, index: int) -> dict[str, Any]:
    return asset_entry(rom, FIRST_TRACK_ASSET + index)


def track_count(rom: bytes) -> int:
    """Consecutive compressed asset entries from `$C2`."""
    count = 0
    while FIRST_TRACK_ASSET + count <= 0xFF and track_entry(rom, count)["compressed"]:
        count += 1
    return count


def decode_track(rom: bytes, index: int) -> tuple[bytes, dict[str, Any]]:
    """Unpack track ``index``; the returned entry also carries the stream's RNC header and consumed length."""
    entry = track_entry(rom, index)
    if not entry["compressed"]:
        raise TrackError(f"asset ${entry['asset']:02X} is not compressed")
    read = rnc.lorom_reader(rom)
    bank = entry["bank"] | 0x80
    header = rnc.parse_header(read, bank, entry["address"])
    if header["magic"] != "RNC" or header["method"] != 1:
        raise TrackError(f"track {index} does not start with an RNC method-1 header")
    data, (end_bank, end_address) = rnc.decompress(read, bank, entry["address"])
    start = provenance.rom_file_offset((bank << 16) | entry["address"], len(rom))
    end = provenance.rom_file_offset((end_bank << 16) | end_address, len(rom))
    if len(data) != header["unpacked_length"]:
        raise TrackError(f"track {index} unpacked {len(data)} bytes, header says {header['unpacked_length']}")
    return data, {**entry, "file_offset": start, "consumed": end - start, "rnc": header}


def parse_header(decoded: bytes) -> dict[str, Any]:
    if len(decoded) < HEADER_LENGTH:
        raise TrackError("track header missing")
    shape = decoded[13]
    if shape not in SHAPES:
        raise TrackError(f"playfield shape byte 0x{shape:02X} selects no arm of $81:A304")
    arm, columns, rows = SHAPES[shape]
    list_offset = _u16(decoded, 11)
    end = decoded.find(b"\xff", list_offset)
    if list_offset >= len(decoded) or end < 0:
        raise TrackError("tile-set list is unterminated")
    return {
        "unnamed_bytes": {"0-2": decoded[0:3].hex(), "14": decoded[14]},
        "player_start": [_u16(decoded, 3), _u16(decoded, 5)],
        "opponent_start": [_u16(decoded, 7), _u16(decoded, 9)],
        "tile_set_list_offset": list_offset,
        "tile_set_ids": list(decoded[list_offset:end]),
        "shape": {"byte": shape, "arm": arm, "columns": columns, "rows": rows},
    }


def tile_content(rom: bytes, tile_set_ids: list[int]) -> dict[str, bytes]:
    """BG1 tiles (loader transfer order), tile columns and tile flags for a tile-set list."""
    tiles, columns, flags = bytearray(), bytearray(), bytearray()
    for identifier in tile_set_ids:
        directory = _rom(rom, TILE_DIRECTORY_BUS + 5 * identifier, 5)
        tile_bus = (directory[0] << 16) | _u16(directory, 1)
        tile_bytes = _u16(directory, 3)
        if tile_bytes == 0 or tile_bytes % 128:
            raise TrackError(f"tile set {identifier} has length {tile_bytes}")
        tile_count = tile_bytes // 128
        table = _rom(rom, TABLE_DIRECTORY_BUS + 4 * identifier, 4)
        table_pointer = _u16(table, 0)
        columns += _rom(rom, (table[2] << 16) | table_pointer, tile_bytes // 4)
        flags += _rom(rom, FLAGS_BASE_BUS + (table_pointer - 0xA0A4) // 32, tile_count)
        source = _rom(rom, tile_bus, tile_bytes)
        for group_start in range(0, tile_count, 8):
            group = min(8, tile_count - group_start)
            base = group_start * 128
            for within in range(group):
                top = base + within * 64
                bottom = base + group * 64 + within * 64
                tiles += source[top:top + 64] + source[bottom:bottom + 64]
    return {"bg1_tiles": bytes(tiles), "tile_columns": bytes(columns), "tile_flags": bytes(flags)}


def inventory(rom: bytes) -> dict[str, Any]:
    """The track stream manifest: locations, sizes, header field values (with the tile-set ids) and digests."""
    count = track_count(rom)
    streams = []
    for index in range(count):
        decoded, entry = decode_track(rom, index)
        header = parse_header(decoded)
        derived = tile_content(rom, header["tile_set_ids"])
        streams.append({
            "index": index,
            "asset": f"${entry['asset']:02X}",
            "address": f"${entry['bank'] | 0x80:02X}:{entry['address']:04X}",
            "file_offset": entry["file_offset"],
            "directory_length": entry["length"],
            "consumed_length": entry["consumed"],
            "packed_length": entry["rnc"]["packed_length"],
            "unpacked_length": len(decoded),
            "chunks": entry["rnc"]["chunks"],
            "unpacked_sha256": sha256(decoded),
            "header": header,
            "derived": {name: {"length": len(data), "sha256": sha256(data)} for name, data in derived.items()},
        })
    return {"schema_version": 1, "kind": "track_stream_inventory", "rom_sha256": sha256(rom),
            "asset_directory": f"${ASSET_DIRECTORY_BUS:06X}", "first_track_asset": f"${FIRST_TRACK_ASSET:02X}",
            "track_count": count, "streams": streams}


# ------------------------------------------------------------- pack entries (TRACK-BREADTH part 3)

# The race tracks a cold start reaches, beyond DRAGSTER (0) and ZOOM ZOO (1), whose
# scenario (race mode, laps, initialization frame) is observed (R-0046 observations
# 6-8). The stunt events (2, 12, 22, 32) are a separate mode and are not listed.
NEW_RACE_TRACKS = (3, 4, 10, 11, 13, 14, 20, 21, 23, 24, 30, 31, 33, 34)
SCENERY_COUNT = 14
# `$82:DC20-DD84`: BG2 tiles asset `$70 + s`, map `$82 + s`, palette row `$93 + s`
# for scenery s = track mod 14 (track 42, NEON, has an extra case not listed here),
# then a six-row palette block chosen by the class byte `$82:DC12 + s`.
SCENERY_CLASS_TABLE_BUS = 0x82DC12
CLASS_PALETTE_ASSETS = {0: 0xA4, 1: 0xB0, 2: 0xAA}
# The four palette rows both accepted tracks load after the scenery (`$82:DD84-DDCC`);
# the loader picks them by the rider selection (`$77:0748`/`$0749`), MIKE throughout.
FIXED_PALETTE_PIECES = ((0x07B9A0, 32), (0x020180, 32), (0x020380, 32), (0x0203A0, 32))


def _asset_piece(rom: bytes, asset: int) -> tuple[int, int]:
    e = asset_entry(rom, asset)
    if e["compressed"]:
        raise TrackError(f"asset ${asset:02X} is compressed")
    return provenance.rom_file_offset(((e["bank"] | 0x80) << 16) | e["address"], len(rom)), e["length"]


def _merge(pieces: list[tuple[int, int]]) -> list[tuple[int, int]]:
    merged: list[tuple[int, int]] = []
    for offset, length in pieces:
        if merged and merged[-1][0] + merged[-1][1] == offset:
            merged[-1] = (merged[-1][0], merged[-1][1] + length)
        else:
            merged.append((offset, length))
    return merged


def tile_pieces(rom: bytes, tile_set_ids: list[int]) -> dict[str, list[tuple[int, int]]]:
    """The ROM pieces of ``tile_content``'s three outputs, in the same order."""
    tiles, columns, flags = [], [], []
    for identifier in tile_set_ids:
        directory = _rom(rom, TILE_DIRECTORY_BUS + 5 * identifier, 5)
        tile_bus = (directory[0] << 16) | _u16(directory, 1)
        tile_bytes = _u16(directory, 3)
        tile_count = tile_bytes // 128
        table = _rom(rom, TABLE_DIRECTORY_BUS + 4 * identifier, 4)
        table_pointer = _u16(table, 0)
        columns.append((provenance.rom_file_offset((table[2] << 16) | table_pointer, len(rom)), tile_bytes // 4))
        flags.append((provenance.rom_file_offset(FLAGS_BASE_BUS + (table_pointer - 0xA0A4) // 32, len(rom)), tile_count))
        base = provenance.rom_file_offset(tile_bus, len(rom))
        for group_start in range(0, tile_count, 8):
            group = min(8, tile_count - group_start)
            for within in range(group):
                top = base + group_start * 128 + within * 64
                tiles += [(top, 64), (top + group * 64, 64)]
    return {"bg1_tiles": tiles, "tile_columns": columns, "tile_flags": flags}


def scenery(track: int) -> int:
    return track % SCENERY_COUNT


def scenery_pieces(rom: bytes, s: int) -> dict[str, list[tuple[int, int]]]:
    """BG2 tiles, BG2 map and the 352-byte race palette of scenery ``s``."""
    klass = _rom(rom, SCENERY_CLASS_TABLE_BUS + s, 1)[0]
    block = [_asset_piece(rom, CLASS_PALETTE_ASSETS[klass] + k) for k in range(6)]
    palette = _merge(block) + [_asset_piece(rom, 0x93 + s)] + list(FIXED_PALETTE_PIECES)
    return {"bg2_tiles": [_asset_piece(rom, 0x70 + s)], "bg2_map": [_asset_piece(rom, 0x82 + s)], "palette": palette}


def _raw(entry_id: str, rom: bytes, pieces: list[tuple[int, int]]) -> dict[str, Any]:
    data = b"".join(rom[o:o + n] for o, n in pieces)
    return {"id": entry_id, "source": {"kind": "raw", "pieces": [{"file_offset": o, "length": n} for o, n in pieces]},
            "size": len(data), "sha256": sha256(data)}


def track_pack_entries(rom: bytes, index: int) -> list[dict[str, Any]]:
    decoded, entry = decode_track(rom, index)
    pieces = tile_pieces(rom, parse_header(decoded)["tile_set_ids"])
    name = f"track.{index:02d}"
    return [{"id": f"{name}.data", "source": {"kind": "rnc", "bank": entry["bank"], "address": entry["address"]},
             "size": len(decoded), "sha256": sha256(decoded)},
            _raw(f"{name}.tile-columns", rom, pieces["tile_columns"]),
            _raw(f"{name}.tile-flags", rom, pieces["tile_flags"]),
            _raw(f"{name}.bg1-tiles", rom, pieces["bg1_tiles"])]


def scenery_pack_entries(rom: bytes, s: int) -> list[dict[str, Any]]:
    pieces = scenery_pieces(rom, s)
    name = f"scenery.{s:02d}"
    return [_raw(f"{name}.bg2-tiles", rom, pieces["bg2_tiles"]), _raw(f"{name}.bg2-map", rom, pieces["bg2_map"]),
            _raw(f"{name}.palette", rom, pieces["palette"])]


# The track names, `$FF`-terminated lowercase ASCII from `$83:9FFA` in track order
# (R-0046 observation 5; observed for the 20 cold-start tracks).
TRACK_NAMES_BUS = 0x839FFA


def track_names_entry(rom: bytes, count: int = 45) -> dict[str, Any]:
    start = provenance.rom_file_offset(TRACK_NAMES_BUS, len(rom))
    end = start
    for _ in range(count):
        end = rom.index(b"\xff", end) + 1
    return _raw("presentation.classic.track-names.v1", rom, [(start, end - start)])


def v10_new_entries(rom: bytes) -> list[dict[str, Any]]:
    """The entries profile v10 adds to v9, in pack order."""
    entries = []
    for index in NEW_RACE_TRACKS:
        entries += track_pack_entries(rom, index)
    for s in sorted({scenery(index) for index in NEW_RACE_TRACKS}):
        entries += scenery_pack_entries(rom, s)
    return entries + [track_names_entry(rom)]
