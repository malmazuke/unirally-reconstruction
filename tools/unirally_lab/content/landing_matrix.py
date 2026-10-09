"""Bounded original execution to extract immutable pre-race coefficient tables.

This installation-time helper never supplies dynamic rider state. See R-0030's
static landing matrix audit; ordinary native play does not import this module.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
from ..reference.bsnes import BsnesCore
from ..reference.core_identity import require_core
from ..reference.worker import inputs_for_frame
from ..replay.manifest import derive_script

ROOT=Path(__file__).resolve().parents[3]
ROM_SHA='a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e'


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom',type=Path,required=True)
    parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args()
    if args.out.exists() or hashlib.sha256(args.rom.read_bytes()).hexdigest()!=ROM_SHA:
        raise ValueError('fresh output and exact supported ROM required')
    suffix='dylib' if sys.platform=='darwin' else 'so'
    library=ROOT/f'local/emulators/bsnes/bsnes/out/bsnes_libretro.{suffix}'
    if not library.exists():
        subprocess.run([sys.executable,str(ROOT/'tools/project.py'),'reference','build'],cwd=ROOT,check=True,timeout=600)
    # Any library built from the lock's source runs here; the pack rules' SHA-256 of the extracted
    # bytes is the evidence of equal behaviour (pack.build_pack, PORTABLE-CORE-IDENTITY).
    require_core(library)
    script=derive_script(json.loads((ROOT/'tests/manifests/replay/race-crawler-zoom-zoo-3300.json').read_text()))
    with tempfile.TemporaryDirectory(dir=args.out.parent) as directory:
        core=BsnesCore(library,Path(directory),{})
        try:
            core.load(args.rom);core.set_serialization_method('Strict')
            for frame in range(1298):
                for port,buttons in inputs_for_frame(script,frame).items():core.set_inputs(port,buttons)
                core.run_frame()
            matrices=core.wram()[0x572:0xb5a]
        finally:core.unload()
    args.out.write_bytes(matrices)

if __name__=='__main__':main()
