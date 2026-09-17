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
from ..reference.worker import inputs_for_frame
from ..replay.manifest import derive_script

ROOT=Path(__file__).resolve().parents[3]
CORE_SHA='e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b'
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
    if hashlib.sha256(library.read_bytes()).hexdigest()!=CORE_SHA:
        raise ValueError('pre-race extractor core identity differs')
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
