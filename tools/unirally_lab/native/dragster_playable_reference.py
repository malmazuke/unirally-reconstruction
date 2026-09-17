"""Original-only DRAGSTER ordinary-control captures; never a native runtime input.

Cold-starts the accepted 1P, MIKE, CRAWLER, DRAGSTER menu path of
`tests/manifests/replay/race-crawler-dragster-3000.json` (Start presses only,
all before frame 1206), then presents a case's controller-0 buttons from the
first race update (frame 1329) onward. Every frame not named by the case is
neutral. The original's race initialization boundary is the end of frame 1328
(fade `$0FF1` 0, countdown `$11C5` 270), 48 frames earlier than ZOOM ZOO's.

Raw WRAM/SRAM series stay private under ignored `artifacts/`; the report keeps
the timeline, per-frame memory and video hashes for later authentication.
"""
from __future__ import annotations
import argparse
import json
from pathlib import Path
import tempfile
from .zoom_zoo_trial_reference import ROOT, ROM_SHA, CORE_SHA, sha, digest
from ..reference.bsnes import BsnesCore, BUTTONS, frame_png
from ..reference.worker import inputs_for_frame
from ..replay.manifest import derive_script

MENU_MANIFEST = 'tests/manifests/replay/race-crawler-dragster-3000.json'
INITIALIZATION_FRAME = 1328
FIRST_RECORDED_FRAME = INITIALIZATION_FRAME - 169
MENU_LAST_INPUT = 1205


def menu_identity():
    raw = (ROOT/MENU_MANIFEST).read_bytes()
    manifest = json.loads(raw)
    events = [e for c in manifest['inputs']['controllers'] for e in c['events'] if e['from'] <= INITIALIZATION_FRAME]
    if any(e['buttons'] != ['start'] or e['to'] > MENU_LAST_INPUT for e in events):
        raise ValueError('DRAGSTER menu prefix changed')
    return raw, manifest


def load_case(case, horizon):
    if set(case) != {'id', 'changes'} or not isinstance(case['id'], str) or not isinstance(case['changes'], list):
        raise ValueError('case needs exactly id and changes')
    frames = {}
    for change in case['changes']:
        if set(change) != {'from', 'to', 'buttons'}:
            raise ValueError('invalid change')
        first, last, buttons = change['from'], change['to'], change['buttons']
        if type(first) is not int or type(last) is not int or not INITIALIZATION_FRAME < first <= last <= horizon:
            raise ValueError('change outside the race')
        if not isinstance(buttons, list) or len(set(buttons)) != len(buttons) or any(b not in BUTTONS for b in buttons):
            raise ValueError('invalid buttons')
        for frame in range(first, last+1):
            if frame in frames:
                raise ValueError('overlapping changes')
            frames[frame] = sorted(buttons)
    return frames


def timeline(case, horizon):
    _, manifest = menu_identity()
    script = derive_script(manifest)
    changes = load_case(case, horizon)
    rows = []
    for frame in range(horizon+1):
        if frame <= INITIALIZATION_FRAME:
            ports = inputs_for_frame(script, frame)
            rows.append([sorted(ports.get(0, [])), sorted(ports.get(1, []))])
        else:
            rows.append([changes.get(frame, []), []])
    return rows


def capture(core_path, out, case, horizon, frame_images=()):
    if out.exists():
        raise ValueError('fresh output directory required')
    raw, _ = menu_identity()
    rom = Path((ROOT/'local/rom-location.txt').read_text().strip())
    if (sha(core_path.read_bytes()), sha(rom.read_bytes())) != (CORE_SHA, ROM_SHA):
        raise ValueError('original identities differ')
    inputs = timeline(case, horizon)
    out.mkdir(parents=True)
    hashes, cartridge_hashes, video = [], [], []
    with tempfile.TemporaryDirectory(dir=out) as directory:
        core = BsnesCore(core_path, Path(directory), {})
        try:
            core.load(rom)
            core.set_serialization_method('Strict')
            with (out/'memory.wram').open('xb') as ws, (out/'memory.sram').open('xb') as ss:
                for frame, ports in enumerate(inputs):
                    for port, buttons in enumerate(ports):
                        core.set_inputs(port, set(buttons))
                    core.keep_frame = frame in frame_images
                    result = core.run_frame()
                    if core.keep_frame and core.frame_raw:
                        (out/f'frame-{frame}.png').write_bytes(frame_png(*core.frame_raw))
                    if frame < FIRST_RECORDED_FRAME:
                        continue
                    w, s = core.wram(), core.cartridge_ram()
                    ws.write(w); ss.write(s)
                    hashes.append(sha(w)); cartridge_hashes.append(sha(s))
                    video.append(result.video)
        finally:
            core.unload()
    report = dict(kind='dragster_ordinary_controls_original', case=case, frames=[FIRST_RECORDED_FRAME, horizon],
                  initialization_frame=INITIALIZATION_FRAME, rom_sha256=ROM_SHA, core_sha256=CORE_SHA,
                  menu_manifest=MENU_MANIFEST, menu_manifest_sha256=sha(raw),
                  timeline_sha256=digest(inputs), timeline=inputs,
                  wram_sha256=hashes, sram_sha256=cartridge_hashes, video=video)
    (out/'reference.json').write_text(json.dumps(report, separators=(',', ':'))+'\n')
    return dict(case=case['id'], frames=report['frames'], wram=digest(hashes), sram=digest(cartridge_hashes), video=digest(video))


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--core', type=Path, required=True)
    p.add_argument('--case', type=Path, required=True)
    p.add_argument('--out', type=Path, required=True)
    p.add_argument('--horizon', type=int, required=True)
    p.add_argument('--frame-image', type=int, action='append', default=[], help='also write this frame as PNG (presentation evidence only)')
    a = p.parse_args()
    if not 1400 <= a.horizon <= 40000:
        p.error('horizon must be 1400..40000')
    print(json.dumps(capture(a.core.resolve(), a.out, json.loads(a.case.read_text()), a.horizon, set(a.frame_image))))


if __name__ == '__main__':
    main()
