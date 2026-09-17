"""Original-only M4-16 boundary exploration, never a native runtime input.

Preserves the authenticated cold-start scenario and fixed M4-15 race inputs.
Records initialization, result loading and optional post-result button events.
Raw memories and pictures remain private; this is not an acceptance freeze.
"""
from __future__ import annotations
import argparse
import json
from pathlib import Path
import tempfile
from .zoom_zoo_trial_reference import ROOT, ROM_SHA, CORE_SHA, PRIMARY_SHA, sha, digest
from .zoom_zoo_race_explore import timeline
from ..reference.bsnes import BsnesCore, BUTTONS, frame_png
from ..replay.manifest import derive_script


def capture(core_path, out, horizon, post_events, variation=None):
    if out.exists():
        raise ValueError('fresh output directory required')
    raw = (ROOT/'tests/manifests/replay/race-crawler-zoom-zoo-3300.json').read_bytes()
    rom = Path((ROOT/'local/rom-location.txt').read_text().strip())
    if (sha(raw), sha(core_path.read_bytes()), sha(rom.read_bytes())) != (PRIMARY_SHA, CORE_SHA, ROM_SHA):
        raise ValueError('original identities differ')
    case = json.loads((ROOT/'tests/manifests/native/zoom-zoo-race-primary.case.json').read_text())
    inputs = timeline(case, horizon, derive_script(json.loads(raw)))
    for f in range(6725, horizon+1):
        inputs[f] = [[], []]
    idle=(variation or {}).get('idle')
    if idle is not None:
        # Ordinary controller pause in play: release every button from `from`
        # for `frames` updates (all remaining updates when null), then resume
        # the primary controller stream where it was left.
        first,count=idle.get('from'),idle.get('frames')
        if (type(first) is not int or not 1650<=first<=horizon or set(idle)!={'from','frames'} or
                (count is not None and (type(count) is not int or count<1))):
            raise ValueError('invalid idle variation')
        primary=inputs
        inputs=[primary[f] if f<first else [[],[]] if count is None or f<first+count else [list(p) for p in primary[f-count]]
                for f in range(horizon+1)]
    varied=set()
    for event in (variation or {}).get('changes',[]):
        first,last,buttons=event['from'],event['to'],event['buttons']
        if type(first) is not int or type(last) is not int or not 1377<=first<=last<=horizon or any(b not in BUTTONS for b in buttons):
            raise ValueError('invalid controller variation')
        for f in range(first,last+1):
            if f in varied:raise ValueError('overlapping controller variation')
            varied.add(f);inputs[f][0]=sorted(buttons)
    seen = set()
    for event in post_events:
        first, last, buttons = event['from'], event['to'], event['buttons']
        if not 6725 <= first <= last <= horizon or any(b not in BUTTONS for b in buttons):
            raise ValueError('invalid post-result event')
        for f in range(first, last+1):
            if f in seen:
                raise ValueError('overlapping post-result events')
            seen.add(f)
            inputs[f][0] = sorted(buttons)
    out.mkdir(parents=True)
    hashes, cartridge_hashes, video = [], [], []
    with tempfile.TemporaryDirectory(dir=out) as directory:
        core = BsnesCore(core_path, Path(directory), {})
        try:
            core.load(rom)
            core.set_serialization_method('Strict')
            with (out/'memory.wram').open('xb') as ws, (out/'memory.sram').open('xb') as ss:
                for f, ports in enumerate(inputs):
                    core.keep_frame = f in (1376,1377,1450,1583,1649,3208,4840,6484,6724,6725,6800,6900,6950,7000,7100,7200,7400,7600,7800,8000,8200,8400)
                    for p, buttons in enumerate(ports):
                        core.set_inputs(p, set(buttons))
                    result = core.run_frame()
                    if f < 1207:
                        continue
                    w, s = core.wram(), core.cartridge_ram()
                    ws.write(w); ss.write(s)
                    hashes.append(sha(w)); cartridge_hashes.append(sha(s))
                    video.append(result.video)
                    if core.keep_frame and core.frame_raw:
                        (out/f'frame-{f}.png').write_bytes(frame_png(*core.frame_raw))
        finally:
            core.unload()
    report = dict(kind='m4_16_original_boundary_exploration', frames=[1207,horizon],
                  rom_sha256=ROM_SHA, core_sha256=CORE_SHA, manifest_sha256=PRIMARY_SHA,
                  timeline_sha256=digest(inputs), timeline=inputs, post_events=post_events, variation=variation,
                  wram_sha256=hashes, sram_sha256=cartridge_hashes, video=video)
    (out/'reference.json').write_text(json.dumps(report,separators=(',',':'))+'\n')
    print(json.dumps(dict(frames=report['frames'],wram=digest(hashes),sram=digest(cartridge_hashes))))


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--core',type=Path,required=True)
    p.add_argument('--out',type=Path,required=True)
    p.add_argument('--horizon',type=int,default=7600)
    p.add_argument('--post-events',type=Path)
    p.add_argument('--case',type=Path)
    a=p.parse_args()
    variation=json.loads(a.case.read_text()) if a.case else None
    if not 6725 <= a.horizon <= (40000 if variation and 'idle' in variation else 15000):
        p.error('horizon must be 6725..15000, or up to 40000 for an idle variation')
    capture(a.core.resolve(),a.out,a.horizon,json.loads(a.post_events.read_text()) if a.post_events else [],variation)

if __name__=='__main__':main()
