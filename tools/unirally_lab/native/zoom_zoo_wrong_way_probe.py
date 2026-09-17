"""Artificial original-only boundary probe, never a playable acceptance case.

Cold-start using an authenticated constant-left timeline, then change only the
player wrong-direction counter to 179 immediately before frame1997. This probes
$82973F-9792 without claiming a naturally reached start-to-result trajectory.
No native code or dynamic runtime initialization is produced by this tool.
"""
import argparse
import ctypes
import json
from pathlib import Path
import tempfile
from .zoom_zoo_playable import original, ROM_SHA, CORE_SHA, sha
from ..reference.bsnes import BsnesCore


def probe(reference, core_path, out):
    if out.exists():
        raise ValueError('fresh probe output required')
    document, _, _ = original(reference, allow_incomplete=True)
    if document['variation']['id'] != 'm4-16-constant-left':
        raise ValueError('probe requires the frozen constant-left case')
    rom_path = Path(Path('local/rom-location.txt').read_text().strip())
    if sha(rom_path.read_bytes()) != ROM_SHA or sha(core_path.read_bytes()) != CORE_SHA:
        raise ValueError('probe identity mismatch')
    out.mkdir(parents=True)
    with tempfile.TemporaryDirectory(dir=out) as directory:
        core = BsnesCore(core_path.resolve(), Path(directory), {})
        try:
            core.load(rom_path)
            core.set_serialization_method('Strict')
            for frame in range(1998):
                if frame == 1997:
                    before = core.wram()
                    size = ctypes.c_size_t()
                    pointer = core._lib.unirally_memory(0, ctypes.byref(size))
                    if not pointer or size.value != 131072:
                        raise ValueError('unexpected original WRAM size')
                    ctypes.memmove(pointer + 0xe57, (179).to_bytes(2, 'little'), 2)
                    modified = core.wram()
                    assert modified[:0xe57] == before[:0xe57] and modified[0xe59:] == before[0xe59:]
                for port, buttons in enumerate(document['timeline'][frame]):
                    core.set_inputs(port, set(buttons))
                core.run_frame()
                if 1207 <= frame < 1997 and sha(core.wram()) != document['wram_sha256'][frame-1207]:
                    raise ValueError(f'pre-intervention original mismatch at {frame}')
            after = core.wram()
            for name, data in [('before', before), ('modified', modified), ('after', after)]:
                (out/(name+'.wram')).write_bytes(data)
            report = dict(kind='artificial_original_counter_probe', acceptance=False,
                          rom_sha256=ROM_SHA, core_sha256=CORE_SHA, timeline_sha256=document['timeline_sha256'],
                          intervention={'before_frame':1997, 'address':0xe57, 'value':179, 'width':2},
                          before_sha256=sha(before), modified_sha256=sha(modified), after_sha256=sha(after),
                          counter_after=int.from_bytes(after[0xe57:0xe59], 'little'),
                          queue_before=list(modified[0xcc1:0xce1]), queue_after=list(after[0xcc1:0xce1]),
                          cursors_before=list(modified[0xce7:0xcea:2]), cursors_after=list(after[0xce7:0xcea:2]))
            (out/'report.json').write_text(json.dumps(report, indent=2)+'\n')
            return report
        finally:
            core.unload()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reference', type=Path, required=True)
    parser.add_argument('--core', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(probe(args.reference, args.core, args.out), indent=2))

if __name__ == '__main__':
    main()
