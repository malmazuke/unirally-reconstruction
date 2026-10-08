"""Check a reference core library by its outputs: replay a stored `track_reference` capture's inputs
on it and compare every recorded frame's work RAM, cartridge RAM and video hashes with the stored
ones (PORTABLE-CORE-IDENTITY). Equal hashes on every frame mean this library ran that capture
exactly as the library that made it. A library built from the lock's source on another host, or
rebuilt with another compiler, that passes here and reproduces the pack's landing-matrix pin may
be listed in `tools/locks/verified-cores.json`.

usage: python3 -m tools.unirally_lab.native.core_check --core LIBRARY --capture DIR --out JSON
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import tempfile

from .track_reference import FIRST_RECORDED_FRAME, unlocked_sram
from .zoom_zoo_trial_reference import ROOT, ROM_SHA, sha
from ..reference.bsnes import BsnesCore
from ..reference.commands import sha256_file
from ..reference.core_identity import lock_source, source_built_library


def check(core_path: Path, capture: Path, to_frame: int | None = None) -> dict:
    document = json.loads((capture / 'reference.json').read_text())
    if document.get('kind') != 'track_breadth_original' or document['rom_sha256'] != ROM_SHA:
        raise ValueError('a track_reference capture of the supported ROM is required')
    rom = Path((ROOT / 'local/rom-location.txt').read_text().strip())
    if sha(rom.read_bytes()) != ROM_SHA:
        raise ValueError('the ROM differs from the supported one')
    first, last = document['frames']
    last = min(last, to_frame) if to_frame is not None else last
    timeline = document['timeline']
    unlock, preload = document.get('unlock_tours', False), [tuple(p) for p in document.get('sram_preload', [])]
    image = unlocked_sram(core_path, rom, unlock, preload) if unlock or preload else None
    if image is not None and sha(image) != document['preload_sram_sha256']:
        return dict(status='failed', mismatch='preload cartridge RAM', frame=None)
    library_sha = sha256_file(core_path)
    result = dict(status='passed', core_sha256=library_sha, reference_core_sha256=document['core_sha256'],
                  source_built=library_sha == source_built_library(), lock_source=lock_source(),
                  capture=str(capture), frames=[first, last], compared=0, mismatch=None, frame=None)
    with tempfile.TemporaryDirectory() as directory:
        if image is not None:
            (Path(directory) / (rom.stem + '.srm')).write_bytes(image)
        core = BsnesCore(core_path, Path(directory), {})
        try:
            core.load(rom)
            core.set_serialization_method('Strict')
            for frame in range(last + 1):
                for port, buttons in enumerate(timeline[frame]):
                    core.set_inputs(port, set(buttons))
                output = core.run_frame()
                if frame < first:
                    continue
                k = frame - first
                for name, value, stored in (('wram', sha(core.wram()), document['wram_sha256'][k]),
                                            ('sram', sha(core.cartridge_ram()), document['sram_sha256'][k]),
                                            ('video', list(output.video) if output.video else None, document['video'][k])):
                    if value != stored:
                        result.update(status='failed', mismatch=name, frame=frame)
                        return result
                result['compared'] += 1
        finally:
            core.unload()
    return result


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--core', type=Path, required=True, help='the core library to check')
    parser.add_argument('--capture', type=Path, required=True, help='a track_reference capture directory')
    parser.add_argument('--to-frame', type=int, help='stop after this frame (default: the capture\'s last)')
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    if args.out.exists():
        raise ValueError('fresh report required')
    result = check(args.core.resolve(), args.capture.resolve(), args.to_frame)
    args.out.write_text(json.dumps(result, indent=1) + '\n')
    print(json.dumps({k: result[k] for k in ('status', 'core_sha256', 'compared', 'mismatch', 'frame')}))
    print(f"status={result['status']}")


if __name__ == '__main__':
    main()
