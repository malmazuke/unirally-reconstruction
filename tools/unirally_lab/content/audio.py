"""Identified cold title/menu data only (R-0075), including numeric pitch aliases.

The program resource is excluded. The 97 pitch words deliberately interleave
identified low/high byte lookups; indices85-96 include original code-as-data
bytes, supplied only as numbers. No original event, state or producer clock is
an extraction input.
"""
from __future__ import annotations

from typing import Any
from .tracks import _raw, ASSET_DIRECTORY_BUS
from .provenance import rom_file_offset

GRAPHICS_WORK_ASSETS = (1, 2, 5, 27, 28, 31, 68, 69, 70, 72, 74, 77, 78, 80, 88, 89, 91)


def resource_headers(rom: bytes) -> list[tuple[int, int]]:
    offset = 0x80000
    rows = []
    for _ in range(58):
        if offset + 2 > len(rom):
            raise ValueError("audio resource header outside ROM")
        length = int.from_bytes(rom[offset:offset+2], "little")
        if length < 6 or offset + length > len(rom):
            raise ValueError("invalid audio resource length")
        rows.append((offset, length))
        offset += length
    return rows


def v30_new_entries(rom: bytes) -> list[dict[str, Any]]:
    """Bounded audio payloads appended to the unchanged v29 inventory."""
    resources = resource_headers(rom)
    slots = rom[0x1fcf5:0x1fd35]
    if len(slots) != 64 or any(s != 255 and s >= 50 for s in slots):
        raise ValueError("unidentified cold sample selection")
    directory = rom_file_offset(ASSET_DIRECTORY_BUS, len(rom))
    entries = [
        _raw("audio.menu-tables", rom, [(0x9eaa0, 621)]),
        _raw("audio.title-score", rom, [(0xa026b, 2200)]),
        _raw("audio.pitch-values", rom, [(base+i, 1) for i in range(97)
                                         for base in (644797, 644882)]),
        _raw("audio.resource-lengths", rom, [(at, 2) for at, _ in resources]),
        _raw("audio.menu-transfer", rom, [(0x9eaa0, 627)]),
        _raw("audio.title-transfer", rom, [(0xa026b, 2206)]),
        _raw("audio.sample-slots", rom, [(0x1fcf5, 64)]),
        _raw("audio.sample-fractions", rom, [(resources[0 if s == 255 else s][0]+4, 1)
                                             for s in slots]),
        _raw("audio.sample-transpose", rom, [(resources[0 if s == 255 else s][0]+5, 1)
                                             for s in slots]),
        _raw("audio.graphics-work-directory", rom, [(directory+5*i, 5)
                                                   for i in GRAPHICS_WORK_ASSETS]),
        _raw("audio.cartridge-defaults", rom, [(0x18000, 1158)]),
        _raw("audio.track-types", rom, [(0x1a254, 50)]),
    ]
    entries += [_raw(f"audio.sample.{s:02d}", rom, [(resources[s][0]+2, resources[s][1]-2)])
                for s in sorted(set(slots) - {255})]
    entries[2]["reading"] = "97 numeric pitch words; R-0075 documents code-as-data aliases85-96"
    return entries

HUNTER_GRAPHICS_WORK_ASSETS = (0, 6, 7, 8, 9, 10, 11, 14, 58, 93, 102, 103,
                             104, 105, 107, 108, 109, 110, 111)


def v31_new_entries(rom: bytes) -> list[dict[str, Any]]:
    """Identified page/credit graphics work metadata; v30 entries are unchanged."""
    directory = rom_file_offset(ASSET_DIRECTORY_BUS, len(rom))
    return [_raw("audio.hunter-graphics-work-directory", rom,
                 [(directory+5*i, 5) for i in HUNTER_GRAPHICS_WORK_ASSETS])]
