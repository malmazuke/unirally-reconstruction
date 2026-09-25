"""Deterministic Classic content-pack extraction and validation (schema 1)."""

from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
from typing import Any

from . import rnc

MAGIC = b"URCP0001"
SCHEMA_VERSION = 1
RULES_PATH = "tests/manifests/content/classic-crawler-dragster-pack.json"
PROFILE_ID = "classic.pal.crawler.dragster.v1"
START_STATE_ID = "classic.crawler.dragster.race-start.v1"
# The current Classic pack: DRAGSTER, ZOOM ZOO and, from v10 (TRACK-BREADTH part 3), the other
# race tracks a cold start reaches; v11 (SPECIAL-TILE-RESPONSE) adds the corkscrew heights and
# v12 (LOCKED-TOURS) the locked tours' race tracks, v13 (TILE-PAIRS-8-12-26) the loop's x steps, v14 (HUNTER-EFFECTS) the HUNTER blink pattern, v15 (FRONT-END-MAIN-MENU) the boot screens and
# the main menu, v16 (FRONT-END-1P-SETUP) the rider menu, v17 PICK TOUR, PICK TRACK and NOW PLAYING. The names keep their two-track origin.
TWO_TRACK_RULES_PATH = "tests/manifests/content/classic-crawler-tracks-pack.json"
TWO_TRACK_PROFILE = "classic.pal.crawler.tracks.v17"
TWO_TRACK_START = "classic.crawler.race-start.v2"


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def load_rules(path: Path) -> tuple[dict[str, Any], str]:
    raw = path.read_bytes()
    doc = json.loads(raw)
    if ((doc.get("schema_version"), doc.get("kind"), doc.get("profile_id"),
         doc.get("start_state_id")) not in
            ((1, "classic_pack_rules", PROFILE_ID, START_STATE_ID),
             (1, "classic_pack_rules", TWO_TRACK_PROFILE, TWO_TRACK_START))):
        raise ValueError("unsupported Classic extraction rules identity")
    rom = doc.get("source_rom")
    if not isinstance(rom, dict) or type(rom.get("size")) is not int or not isinstance(rom.get("sha256"), str):
        raise ValueError("Classic extraction rules lack the source ROM identity")
    entries = doc.get("entries")
    if not isinstance(entries, list) or not entries:
        raise ValueError("Classic extraction rules require entries")
    ids: set[str] = set()
    for index, entry in enumerate(entries):
        if not isinstance(entry, dict) or not isinstance(entry.get("id"), str) or entry["id"] in ids:
            raise ValueError(f"invalid or duplicate Classic entry at index {index}")
        ids.add(entry["id"])
        if type(entry.get("size")) is not int or entry["size"] < 0 or not isinstance(entry.get("sha256"), str):
            raise ValueError(f"Classic entry {entry['id']} lacks size/hash")
        source = entry.get("source")
        if not isinstance(source, dict) or source.get("kind") not in ("raw", "rnc", "pre_race_matrix"):
            raise ValueError(f"Classic entry {entry['id']} has invalid extraction source")
    return doc, sha256(raw)


def decode_entry(rom: bytes, entry: dict[str, Any]) -> bytes:
    source = entry["source"]
    if source["kind"] == "raw":
        pieces = source.get("pieces")
        if not isinstance(pieces, list) or not pieces:
            raise ValueError(f"raw entry {entry['id']} has no pieces")
        out = bytearray()
        for piece in pieces:
            offset, length = piece.get("file_offset"), piece.get("length")
            if type(offset) is not int or type(length) is not int or offset < 0 or length <= 0 or offset + length > len(rom):
                raise ValueError(f"raw entry {entry['id']} has an out-of-range piece")
            out.extend(rom[offset:offset + length])
        return bytes(out)
    if source["kind"] == "pre_race_matrix":
        root=Path(__file__).resolve().parents[3]
        (root/'artifacts').mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='pack-static-',dir=root/'artifacts') as directory:
            temporary=Path(directory)
            rom_path=temporary/'input.sfc';rom_path.write_bytes(rom)
            output=temporary/'coefficients.bin'
            subprocess.run([sys.executable,'-m','tools.unirally_lab.content.landing_matrix',
                            '--rom',str(rom_path),'--out',str(output)],cwd=root,
                           check=True,capture_output=True,timeout=660)
            return output.read_bytes()
    source_bank, source_address = source.get("bank"), source.get("address")
    if type(source_bank) is not int or type(source_address) is not int:
        raise ValueError(f"RNC entry {entry['id']} lacks a bank/address")
    return rnc.decompress(rnc.lorom_reader(rom), source_bank, source_address)[0]


def _field(data: bytes, offset: int, width: int, label: str) -> tuple[bytes, int]:
    if offset + width > len(data):
        raise ValueError(f"Classic pack is truncated in {label}")
    return data[offset:offset + width], offset + width


def _text(data: bytes, offset: int, label: str) -> tuple[str, int]:
    raw, offset = _field(data, offset, 2, f"{label} length")
    width = int.from_bytes(raw, "little")
    raw, offset = _field(data, offset, width, label)
    try:
        value = raw.decode("utf-8")
    except UnicodeDecodeError as exc:
        raise ValueError(f"Classic pack {label} is not UTF-8") from exc
    if not value:
        raise ValueError(f"Classic pack {label} is empty")
    return value, offset


def build_pack(rom: bytes, rules: dict[str, Any], rules_sha256: str) -> tuple[bytes, list[dict[str, Any]]]:
    source = rules["source_rom"]
    if len(rom) != source["size"] or sha256(rom) != source["sha256"]:
        raise ValueError("source ROM identity differs from the exact supported PAL image")
    decoded: list[tuple[dict[str, Any], bytes]] = []
    for entry in rules["entries"]:
        payload = decode_entry(rom, entry)
        if len(payload) != entry["size"] or sha256(payload) != entry["sha256"]:
            raise ValueError(f"extracted entry identity differs: {entry['id']}")
        decoded.append((entry, payload))
    profile = rules["profile_id"].encode()
    start = rules["start_state_id"].encode()
    table_bytes = sum(2 + len(entry["id"].encode()) + 8 + 8 + 32 for entry, _ in decoded)
    payload_offset = 8 + 4 + 32 + 32 + 2 + len(profile) + 2 + len(start) + 2 + table_bytes
    header = bytearray(MAGIC + struct.pack("<I", SCHEMA_VERSION))
    header.extend(bytes.fromhex(source["sha256"]))
    header.extend(bytes.fromhex(rules_sha256))
    for value in (profile, start):
        header.extend(struct.pack("<H", len(value))); header.extend(value)
    header.extend(struct.pack("<H", len(decoded)))
    rows = []
    cursor = payload_offset
    for entry, payload in decoded:
        logical = entry["id"].encode()
        header.extend(struct.pack("<H", len(logical))); header.extend(logical)
        header.extend(struct.pack("<QQ", cursor, len(payload)))
        header.extend(bytes.fromhex(entry["sha256"]))
        rows.append({"id": entry["id"], "offset": cursor, "size": len(payload), "sha256": entry["sha256"]})
        cursor += len(payload)
    return bytes(header) + b"".join(payload for _, payload in decoded), rows


def validate_pack(data: bytes, rules: dict[str, Any], rules_sha256: str) -> dict[str, Any]:
    if len(data) < 76 or data[:8] != MAGIC:
        raise ValueError("Classic pack header magic is invalid")
    offset = 8
    schema = int.from_bytes(data[offset:offset + 4], "little"); offset += 4
    if schema != SCHEMA_VERSION:
        raise ValueError(f"Classic pack schema is incompatible: {schema}")
    source_raw, offset = _field(data, offset, 32, "source identity")
    rules_raw, offset = _field(data, offset, 32, "rules identity")
    if source_raw.hex() != rules["source_rom"]["sha256"]:
        raise ValueError("Classic pack source ROM identity is incompatible")
    if rules_raw.hex() != rules_sha256:
        raise ValueError("Classic pack extraction-rules identity is incompatible")
    profile, offset = _text(data, offset, "profile identity")
    start, offset = _text(data, offset, "start-state identity")
    if profile != rules["profile_id"]:
        raise ValueError("Classic pack profile identity is incompatible")
    if start != rules["start_state_id"]:
        raise ValueError("Classic pack start-state identity is incompatible")
    raw_count, offset = _field(data, offset, 2, "entry count")
    count = int.from_bytes(raw_count, "little")
    if count != len(rules["entries"]):
        raise ValueError("Classic pack entry count is incomplete")
    rows = []
    ids: set[str] = set()
    for index in range(count):
        logical, offset = _text(data, offset, f"entry {index} logical ID")
        fixed, offset = _field(data, offset, 48, f"entry {logical} table row")
        entry_offset, size = struct.unpack("<QQ", fixed[:16])
        digest = fixed[16:].hex()
        if logical in ids:
            raise ValueError(f"Classic pack has duplicate logical ID: {logical}")
        ids.add(logical)
        rows.append({"id": logical, "offset": entry_offset, "size": size, "sha256": digest})
    expected_by_id = {entry["id"]: entry for entry in rules["entries"]}
    if ids != set(expected_by_id):
        raise ValueError("Classic pack logical entry inventory is incompatible")
    cursor = offset
    for row in rows:
        expected = expected_by_id[row["id"]]
        if row["offset"] != cursor:
            raise ValueError(f"Classic pack entry layout is non-canonical: {row['id']}")
        if row["size"] != expected["size"] or row["sha256"] != expected["sha256"]:
            raise ValueError(f"Classic pack entry identity is incompatible: {row['id']}")
        end = cursor + row["size"]
        if end > len(data):
            raise ValueError(f"Classic pack entry is truncated: {row['id']}")
        if sha256(data[cursor:end]) != row["sha256"]:
            raise ValueError(f"Classic pack entry payload hash differs: {row['id']}")
        cursor = end
    if cursor != len(data):
        raise ValueError("Classic pack has trailing or overlapping payload bytes")
    return {"schema_version": schema, "profile_id": profile, "source_rom_sha256": source_raw.hex(),
            "rules_sha256": rules_raw.hex(), "start_state_id": start, "entries": rows,
            "pack_sha256": sha256(data), "pack_size": len(data)}


def write_atomic(path: Path, data: bytes, *, interrupt_before_commit: bool = False) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.exists():
        raise ValueError("refusing to overwrite an existing Classic pack")
    temporary = path.with_name(f".{path.name}.tmp-{os.getpid()}")
    try:
        with temporary.open("xb") as stream:
            stream.write(data); stream.flush(); os.fsync(stream.fileno())
        if interrupt_before_commit:
            raise InterruptedError("simulated interruption before atomic commit")
        os.replace(temporary, path)
    finally:
        if temporary.exists():
            temporary.unlink()
