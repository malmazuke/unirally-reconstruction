"""DRAGSTER ordinary-control original projection, freeze and native gate.

The original race engine is shared by DRAGSTER and ZOOM ZOO (R-0038), so the
742-byte M4-16 projection applies unchanged to DRAGSTER WRAM/SRAM. Native
DRAGSTER initializes 48 frames earlier (end-1328) on the accepted menu path.
Race rows are byte-exact original projections until result loading; the
result tail follows the M4-16 archive rules.
"""
from __future__ import annotations
import argparse
import json
from pathlib import Path
import subprocess
import tempfile
from .zoom_zoo_trial_reference import ROOT, ROM_SHA, CORE_SHA, sha, digest
from .zoom_zoo_race_reference import project
from .zoom_zoo_race import restore_frames
from .zoom_zoo_trial import BUTTONS
from .zoom_zoo_playable import ROLL_WORDS
from .dragster_playable_reference import INITIALIZATION_FRAME, FIRST_RECORDED_FRAME, MENU_MANIFEST
from .classic_race_layout import describe
from ..content.pack import load_rules, validate_pack, TWO_TRACK_RULES_PATH

MAGIC = b'URDG0001'
# ZOOM ZOO applies its constant-domain guards from end-1649, the M4-12 seed.
# The same race moment on DRAGSTER's menu path is 48 frames earlier.
GUARD_FROM = 1649 - 48
GUARDS_PATH = 'tests/manifests/native/zoom-zoo-race-guards.reference.json'
GUARD_OVERRIDES_PATH = 'tests/manifests/native/dragster-race-guards.reference.json'
STABLE_RESULT = dict(player_won=226, player_lost=242)


def guard_items():
    """ZOOM ZOO race guards with DRAGSTER's recovered scenario/playfield values."""
    base = json.loads((ROOT/GUARDS_PATH).read_text())['items']
    overrides = json.loads((ROOT/GUARD_OVERRIDES_PATH).read_text())
    if overrides['base'] != GUARDS_PATH:
        raise ValueError('DRAGSTER guard overrides name another base')
    replaced = {(o['address'], o['width']): o['value'] for o in overrides['overrides']}
    addresses = {(i['address'], i['width']) for i in base}
    if not set(replaced) <= addresses:
        raise ValueError('DRAGSTER guard override is not a base guard')
    return [dict(i, value=replaced.get((i['address'], i['width']), i['value'])) for i in base]


def original(directory, *, allow_incomplete=False, collect_guards=False):
    document = json.loads((directory/'reference.json').read_text())
    if (document['rom_sha256'], document['core_sha256']) != (ROM_SHA, CORE_SHA):
        raise ValueError('original identity differs')
    if document['menu_manifest'] != MENU_MANIFEST or document['menu_manifest_sha256'] != sha((ROOT/MENU_MANIFEST).read_bytes()):
        raise ValueError('original menu identity differs')
    first, last = document['frames']
    if first != FIRST_RECORDED_FRAME or len(document['timeline']) != last+1 or digest(document['timeline']) != document['timeline_sha256']:
        raise ValueError('original timeline identity differs')
    if len(document['wram_sha256']) != last-first+1 or len(document['sram_sha256']) != last-first+1:
        raise ValueError('original memory inventory incomplete')
    rows = []; finish = [None, None]; loading = None; archive = None; charge_archive = None
    announcements_archive = roll_archive = weights_archive = pause_archive = None
    paused_updates = 0; countdown_paused = 0; previous = None; guard_violations = {}
    guards = guard_items()
    with (directory/'memory.wram').open('rb') as ws, (directory/'memory.sram').open('rb') as ss:
        for frame in range(first, last+1):
            w, s = ws.read(131072), ss.read(8192)
            if len(w) != 131072 or len(s) != 8192 or sha(w) != document['wram_sha256'][frame-first] or sha(s) != document['sram_sha256'][frame-first]:
                raise ValueError(f'original memory differs at {frame}')
            if frame < INITIALIZATION_FRAME:
                continue
            if loading is None:
                for rider in (0, 1):
                    if finish[rider] is None and int.from_bytes(w[0xeff+rider*2:0xf01+rider*2], 'little'):
                        finish[rider] = frame
                if archive is not None and int.from_bytes(archive[515:517], 'little') == 240:
                    loading = frame
            if loading is None:
                if frame >= GUARD_FROM:
                    for item in guards:
                        at = item['address']
                        if at in (0xd53, 0xd55):
                            continue
                        if at in {a+2*r for a in ROLL_WORDS for r in (0, 1)}:
                            continue
                        if at in (0x31d, 0x321, 0x339):
                            if int.from_bytes(w[at:at+2], 'little') != int(({0x31d: 'a', 0x321: 'x', 0x339: 'start'}[at]) in document['timeline'][frame][0]):
                                raise ValueError('A/X/Start input publication differs from controller timeline')
                            continue
                        value = int.from_bytes(w[at:at+item['width']], 'little')
                        if value != item['value']:
                            if not collect_guards:
                                raise ValueError(f'new gameplay guard at {frame}: {at:04x}={value:#x}; recover before evaluating native')
                            guard_violations.setdefault(f'{at:04x}', dict(frame=frame, value=value, guarded=item['value']))
                row = bytearray(project(w, s, frame)+w[0xff1:0xff3]+w[0x1261:0x1265]+b'\0\0')
                row[:8] = MAGIC; archive = bytearray(row); charge_archive = w[0xd53:0xd57]
                announcements_archive = (w[0xcc1:0xce1]+w[0xce7:0xce8]+w[0xce9:0xcea]+w[0xca5:0xca7]+s[0x7bb:0x7bd]+w[0x20e8:0x20e9]
                                         +w[0x12e3:0x12e5]+w[0x12eb:0x12ed]+w[0x12ef:0x12f1]+w[0x3ed:0x3ef])
                roll_archive = b''.join(w[a+2*r:a+2*r+2] for r in (0, 1) for a in ROLL_WORDS)
                weights_archive = w[0x20e9:0x2102]+w[0x2103:0x211c]
                if previous is not None and (int.from_bytes(previous[0xef3:0xef5], 'little') or
                   (int.from_bytes(w[0x339:0x33b], 'little') and not int.from_bytes(previous[0xeff:0xf01], 'little'))):
                    paused_updates += 1
                    if int.from_bytes(w[0xff1:0xff3], 'little') >= 5 and int.from_bytes(previous[0x11c5:0x11c7], 'little'):
                        countdown_paused += 1
                pause_archive = w[0xef3:0xef7]+paused_updates.to_bytes(4, 'little')+countdown_paused.to_bytes(4, 'little')
                if any(int.from_bytes(charge_archive[i:i+2], 'little') > 1 for i in (0, 2)):
                    raise ValueError('charge flag is not binary')
            else:
                row = bytearray(archive); row[8:12] = frame.to_bytes(4, 'little')
                # Semantic load clock, held once the winner (226) or loser (242)
                # result screen is stable (R-0012, R-0019).
                stable = STABLE_RESULT['player_won' if finish[0] <= finish[1] else 'player_lost']
                row[-2:] = min(stable, frame-loading+1).to_bytes(2, 'little')
                wanted = s[0x755:0x769]+s[0x7bf:0x7d3]+s[0x769:0x76b]+s[0x7d3:0x7d5]
                if row[467:511] != wanted:
                    raise ValueError('result lap/total archive differs from original')
            previous = w
            row += s[0x106f:0x1073]+s[0x618:0x61c]+charge_archive+announcements_archive+roll_archive+weights_archive+pause_archive
            rows.append(row.hex())
        if ws.read(1) or ss.read(1):
            raise ValueError('extra original memory')
    events = dict(finish_frames=finish, loading_frame=loading)
    if collect_guards:
        events['guard_violations'] = guard_violations
    if loading is None or any(f is None for f in finish):
        if allow_incomplete:
            return document, rows, dict(events, complete=False)
        raise ValueError('case must finish both riders')
    video = document['video']
    changes = [f for f in range(loading+1, last+1) if video[f-first] != video[f-1-first]]
    outcome = 'player_won' if finish[0] <= finish[1] else 'player_lost'
    visible = changes[0] if changes else None
    # Same black load as ZOOM ZOO: the first new picture is load update 109.
    if visible != loading+108:
        raise ValueError(f'new result load timing requires recovery: {visible}, expected {loading+108}')
    stable = loading+STABLE_RESULT[outcome]-1
    if last-stable < 200:
        raise ValueError('200 stable result updates required')
    events.update(outcome=outcome, first_visible=visible, stable_result=stable)
    return document, rows, events


def native_rows(binary, pack, timeline, first, last, restore=None):
    with tempfile.TemporaryDirectory(prefix='dragster-native-') as directory:
        root = Path(directory); inputs = root/'inputs.txt'; seed = root/'restore.bin'
        inputs.write_text(''.join(f'{f} {sum(1<<BUTTONS.index(b) for b in timeline[f][0])} 0\n' for f in range(first+1, last+1)))
        start = ['--start', 'classic.crawler.dragster']
        if restore is not None:
            seed.write_bytes(bytes.fromhex(restore)); start = ['--seed', str(seed)]
        run = subprocess.run([str(binary), *start, '--content-pack', str(pack), '--inputs', str(inputs)], cwd=root, capture_output=True, text=True, timeout=120)
        output = []
        for f, line in enumerate(run.stdout.splitlines(), first):
            label, row = line.split()
            if int(label) != f:
                raise ValueError('native protocol differs')
            output.append(row)
        return output, run.returncode, run.stderr.strip()


def explore(reference, binary, pack):
    document, rows, events = original(reference, allow_incomplete=True, collect_guards=True)
    last = INITIALIZATION_FRAME+len(rows)-1
    actual, code, error = native_rows(binary.resolve(), pack.resolve(), document['timeline'], INITIALIZATION_FRAME, last)
    divergence = None
    for i, (x, y) in enumerate(zip(actual, rows)):
        if x != y:
            divergence = dict(frame=INITIALIZATION_FRAME+i, fields=describe(bytes.fromhex(x), bytes.fromhex(y)))
            break
    return dict(case=document['case']['id'], original_rows=len(rows), native_rows=len(actual), native_exit=code, native_error=error,
                first_divergence=divergence, events=events)


def inventory(document, rows, events):
    return dict(kind='dragster_ordinary_controls_freeze', case=document['case'], frames=[INITIALIZATION_FRAME, document['frames'][1]],
                state_bytes=742, original_sha256=digest(document), rows_sha256=digest(rows), events=events,
                timeline_sha256=document['timeline_sha256'], rom_sha256=ROM_SHA, core_sha256=CORE_SHA,
                menu_manifest_sha256=document['menu_manifest_sha256'],
                guards_sha256=sha((ROOT/GUARDS_PATH).read_bytes()), guard_overrides_sha256=sha((ROOT/GUARD_OVERRIDES_PATH).read_bytes()))


def freeze(a, b, out):
    if out.exists():
        raise ValueError('fresh freeze required')
    left, rows, events = original(a); right, other, repeated = original(b)
    if left != right or rows != other or events != repeated:
        raise ValueError('two original runs differ')
    result = inventory(left, rows, events)
    result['result_inventory'] = ('Race bytes are original projections until loading; thereafter the archived race is checked against '
                                  'surviving SRAM lap slots and totals. Totals at SRAM 618/61a are projected every frame; the '
                                  'mode-0 result screen publishes no graph extrema. The load count is a semantic clock held at the '
                                  'stable winner (226) or loser (242) result.')
    out.write_text(json.dumps(result, indent=2)+'\n')
    return result


def restore_boundaries(document, rows, events):
    first, last = INITIALIZATION_FRAME, document['frames'][1]
    loading = events['loading_frame']
    frames = {first, first+1, first+4, first+5, first+174, first+175, first+206, first+207, loading-1, loading,
              events['first_visible']-1, events['first_visible'], events['stable_result'], last-1}
    # Landings, direction, checkpoint, finish and visibility transitions, as M4-15.
    frames.update(f-48 for f in restore_frames([rows[i] for i in range(1601-first, loading-first)]))
    # Announcement queues, learned weights, charge latches, rolls and pause (M4-16).
    for i in range(1, loading-first):
        before, after = bytes.fromhex(rows[i-1]), bytes.fromhex(rows[i])
        if any(before[x:y] != after[x:y] for x, y in [(12, 16), (581, 619), (621, 626), (628, 632), (632, 652), (654, 676), (678, 742)]):
            frames.update((first+i-1, first+i))
    # Every controller change, so each ordinary control is resumed from a fresh process.
    timeline = document['timeline']
    frames.update(f-1 for f in range(first+1, last+1) if timeline[f] != timeline[f-1])
    return sorted(f for f in frames if first <= f < last)


def compare(a, b, contract, binary, pack, out, restores=True):
    if out.exists():
        raise ValueError('fresh report required')
    binary = binary.resolve(); pack = pack.resolve()
    head = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT); diff = sha(subprocess.check_output(['git', 'diff', 'HEAD'], cwd=ROOT))
    binary_sha = sha(binary.read_bytes()); pack_sha = sha(pack.read_bytes())
    reference, rows, events = original(a); repeat, other, other_events = original(b)
    if reference != repeat or rows != other or events != other_events:
        raise ValueError('original repeats differ')
    frozen = json.loads(contract.read_text())
    if any(frozen.get(key) != value for key, value in inventory(reference, rows, events).items()):
        raise ValueError('frozen original inventory differs')
    rules, rules_sha = load_rules(ROOT/TWO_TRACK_RULES_PATH); validate_pack(pack.read_bytes(), rules, rules_sha)
    first, last = INITIALIZATION_FRAME, reference['frames'][1]
    boundaries = restore_boundaries(reference, rows, events) if restores else []
    with tempfile.TemporaryDirectory(prefix='dragster-playable-native-') as directory:
        root = Path(directory); local_pack = root/'classic.pack'; local_pack.write_bytes(pack.read_bytes())
        inputs = root/'inputs.txt'; seed = root/'restore.bin'

        def execute(start, restore=None, restart=False):
            inputs.write_text(''.join(f'{f} {sum(1<<BUTTONS.index(x) for x in reference["timeline"][f][0])} 0\n' for f in range(start+1, last+1)))
            options = ['--start', 'classic.crawler.dragster']
            if restore is not None:
                seed.write_bytes(bytes.fromhex(restore)); options = ['--restart-from' if restart else '--seed', str(seed)]
            run = subprocess.run([str(binary), *options, '--content-pack', str(local_pack), '--inputs', str(inputs)], cwd=root, capture_output=True, text=True, timeout=60)
            if run.returncode:
                raise ValueError(f'native failed from {start}: {run.stderr.strip()}')
            output = []
            for f, line in enumerate(run.stdout.splitlines(), start):
                label, row = line.split(); data = bytes.fromhex(row)
                if int(label) != f or len(data) != 742 or data[:8] != MAGIC or int.from_bytes(data[8:12], 'little') != f:
                    raise ValueError('native protocol differs')
                output.append(row)
            return output

        actual = execute(first)
        if actual != rows:
            for i, (x, y) in enumerate(zip(actual, rows)):
                if x != y:
                    raise ValueError(f'first mismatch {first+i}: {describe(bytes.fromhex(x), bytes.fromhex(y))}')
            raise ValueError('native frame count differs')
        if execute(first) != actual:
            raise ValueError('fresh native initialization differs')
        if execute(first, actual[-1], True) != actual:
            raise ValueError('restart retains stale race/result state')
        for frame in boundaries:
            if execute(frame, actual[frame-first]) != actual[frame-first:]:
                raise ValueError(f'restore differs at {frame}')
    if (subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT) != head or sha(subprocess.check_output(['git', 'diff', 'HEAD'], cwd=ROOT)) != diff
            or sha(binary.read_bytes()) != binary_sha or sha(pack.read_bytes()) != pack_sha):
        raise ValueError('source/binary/pack changed during validation')
    result = dict(status='passed', case=reference['case']['id'], source_commit=head.decode().strip(), source_diff_sha256=diff,
                  binary_sha256=binary_sha, pack_sha256=pack_sha, contract_sha256=sha(contract.read_bytes()), frames=[first, last],
                  state_bytes=742, rows_sha256=digest(rows), restore_frames=boundaries, events=events,
                  native_inputs='validated two-track pack and live-compatible controller stream; native initialization, no original state')
    out.parent.mkdir(parents=True, exist_ok=True); out.write_text(json.dumps(result, indent=2)+'\n')
    with (out.parent/'validation-ledger.jsonl').open('a') as stream:
        stream.write(json.dumps(dict(result, report=str(out), report_sha256=sha(out.read_bytes())))+'\n')
    return result


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('command', choices=['explore', 'freeze', 'compare'])
    p.add_argument('--reference', type=Path, required=True)
    p.add_argument('--repeat', type=Path)
    p.add_argument('--contract', type=Path)
    p.add_argument('--binary', type=Path)
    p.add_argument('--pack', type=Path)
    p.add_argument('--out', type=Path)
    p.add_argument('--no-restores', action='store_true')
    a = p.parse_args()
    if a.command == 'explore':
        print(json.dumps(explore(a.reference, a.binary, a.pack), indent=1))
    elif a.command == 'freeze':
        r = freeze(a.reference, a.repeat, a.out); print(json.dumps(dict(case=r['case']['id'], rows=r['rows_sha256'], events=r['events'])))
    else:
        r = compare(a.reference, a.repeat, a.contract, a.binary, a.pack, a.out, not a.no_restores)
        print(json.dumps({k: v for k, v in r.items() if k != 'restore_frames'}, indent=1)); print('restores', len(r['restore_frames']))


if __name__ == '__main__':
    main()
