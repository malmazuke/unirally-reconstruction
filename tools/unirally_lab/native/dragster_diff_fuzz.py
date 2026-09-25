"""Differential fuzz for DRAGSTER ordinary controls: original captures against native.

Each seed generates an ordinary controller timeline (mostly Right, with Left,
opposing directions, B, Y, A, X, L, R, Up, Down, Select and short Start pauses
resumed without Up/Down in between), captures it from a cold start in the
original, and reports the first native divergence over the 742-byte projection
(`dragster_playable explore`). Out-of-domain originals are reported, not
compared: a retire from the pause menu, leaving the result screen with other
buttons, or a finish too late for the horizon. Memory series of matching seeds
are deleted; divergent captures are kept. Exploration evidence only; divergent
timelines must be frozen as cases before they back a claim.
"""
from __future__ import annotations
import argparse
import json
import random
import shutil
import subprocess
import sys
from pathlib import Path

HORIZON = 5600


def timeline_case(seed, horizon=HORIZON):
    rng = random.Random(seed*7919+13)
    frame = 1329; changes = []; paused = False
    while frame <= horizon-760:
        length = rng.randint(1, 70); buttons = set()
        roll = rng.random()
        if roll < 0.74: buttons.add('right')
        elif roll < 0.88: buttons.add('left')
        if rng.random() < 0.06: buttons |= {'left', 'right'}
        for name, odds in (('b', 0.25), ('y', 0.08), ('a', 0.1), ('x', 0.15), ('l', 0.12), ('r', 0.12),
                           ('up', 0.06), ('down', 0.06), ('select', 0.02)):
            if rng.random() < odds: buttons.add(name)
        if rng.random() < 0.03:
            buttons.add('start'); length = rng.randint(1, 5); paused = not paused
        if paused: buttons -= {'up', 'down'}
        last = min(horizon-700, frame+length-1)
        if buttons: changes.append({'from': frame, 'to': last, 'buttons': sorted(buttons)})
        frame = last+1
        if paused and 'start' in buttons:
            frame += rng.randint(2, 40)
            changes.append({'from': frame, 'to': frame+1, 'buttons': ['start']}); frame += 2; paused = False
    changes.append({'from': horizon-699, 'to': horizon, 'buttons': ['right']})
    return {'id': f'diff-fuzz-{seed}', 'changes': changes}


def run(out, first, count, binary, core, pack):
    out.mkdir(parents=True, exist_ok=True); results = []
    for seed in range(first, first+count):
        case = timeline_case(seed); case_path = out/f'diff-fuzz-{seed}.case.json'; case_path.write_text(json.dumps(case))
        reference = out/f'diff-fuzz-{seed}'
        if reference.exists(): shutil.rmtree(reference)
        capture = subprocess.run([sys.executable, '-m', 'tools.unirally_lab.native.dragster_playable_reference', '--core', str(core),
                                  '--case', str(case_path), '--horizon', str(HORIZON), '--out', str(reference)], capture_output=True, text=True)
        if capture.returncode:
            result = dict(seed=seed, status='capture failed', detail=capture.stderr.strip()[-200:])
        else:
            explore = subprocess.run([sys.executable, '-m', 'tools.unirally_lab.native.dragster_playable', 'explore', '--reference', str(reference),
                                      '--binary', str(binary), '--pack', str(pack)], capture_output=True, text=True)
            try:
                report = json.loads(explore.stdout); events = report['events']
                status = 'diverged' if report['first_divergence'] or report['native_error'] else 'matched'
                result = dict(seed=seed, status=status, rows=report['original_rows'], divergence=report['first_divergence'],
                              native_error=report['native_error'], finish=events.get('finish_frames'),
                              out_of_domain={k: events[k] for k in ('projection_error', 'left_result_frame') if k in events})
            except (ValueError, KeyError):
                result = dict(seed=seed, status='explore failed', detail=explore.stderr.strip()[-300:])
        results.append(result); print(json.dumps(result), flush=True)
        if result['status'] == 'matched':
            for name in ('memory.wram', 'memory.sram'): (reference/name).unlink()
    (out/f'summary-{first}-{first+count-1}.json').write_text(json.dumps(results, indent=1)+'\n')
    return results


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--out', type=Path, required=True); p.add_argument('--first-seed', type=int, default=1)
    p.add_argument('--seeds', type=int, default=10); p.add_argument('--binary', type=Path, required=True)
    p.add_argument('--core', type=Path, default=Path('local/emulators/bsnes/bsnes/out/bsnes_libretro.dylib'))
    p.add_argument('--pack', type=Path, default=Path('local/classic-pal-crawler-tracks-v13.pack'))
    a = p.parse_args()
    results = run(a.out, a.first_seed, a.seeds, a.binary.resolve(), a.core, a.pack)
    counts = {}
    for r in results: counts[r['status']] = counts.get(r['status'], 0)+1
    print(json.dumps(counts))


if __name__ == '__main__':
    main()
