"""M4-13 frozen player landing laboratory, end-1649 through 1849.

Reference-only freeze precedes native comparison. Both cases and every expected
state digest are required; native restores straddle each observed player landing.
"""
from __future__ import annotations
import argparse
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
from . import zoom_zoo_trial as trial
from .zoom_zoo_trial_reference import ROOT, ROM_SHA, sha, digest
from ..reference.core_identity import accepted_reference

EXTRA_GUARDS = {0x132b: 0}  # $81:94B9 player landing matrix override is inactive.


def landing_frames(rows):
    """$054B current count, $054F previous count, $127B recontact publication."""
    result = []
    for index, row in enumerate(rows[1:], 1):
        state = bytes.fromhex(row)
        if (int.from_bytes(state[32:34], 'little') == 0 and
                int.from_bytes(state[34:36], 'little') == 9 and state[50] == 1):
            result.append(1649 + index)
    return result


def authenticate(reference, repeat):
    a = json.loads(reference.read_text())
    if a != json.loads(repeat.read_text()):
        raise ValueError('fresh reference processes differ')
    if a['rom_sha256'] != ROM_SHA or not accepted_reference(a['core_sha256']):
        raise ValueError('reference identity differs')
    if a.get('extra_guards') != {'132b': 0}:
        raise ValueError('player landing guard was not authenticated')
    if a['case_sha256'] != digest(a['case']) or a['rows_sha256'] != digest(a['rows']):
        raise ValueError('reference integrity differs')
    rows = a['rows']
    if len(rows) != 201:
        raise ValueError('reference horizon differs')
    for frame, row in enumerate(rows, 1649):
        state = bytes.fromhex(row)
        if len(state) != 395 or state[:8] != b'URZZ0001' or int.from_bytes(state[8:12], 'little') != frame:
            raise ValueError('reference framing differs')
    if sha(bytes.fromhex(rows[0])[:394]) != trial.contract()['seed_sha256']:
        raise ValueError('reference seed differs')
    return a


def freeze(reference, repeat, out):
    if out.exists():
        raise ValueError('refusing to overwrite frozen expectations')
    a = authenticate(reference, repeat)
    landings = landing_frames(a['rows'])
    if not landings or 1849 - landings[0] < 100:
        raise ValueError('case does not cover a player landing plus 100 updates')
    result = {'kind': 'm4_13_player_landing_freeze', 'case': a['case'],
              'frames': [1649, 1849], 'state_bytes': 395,
              'rom_sha256': ROM_SHA, 'core_sha256': a['core_sha256'],
              'landing_frames': landings, 'post_landing_updates': 1849-landings[0],
              'restores': sorted({f for landing in landings for f in (landing-1, landing)}),
              'rows_sha256': a['rows_sha256'],
              'state_sha256': [sha(bytes.fromhex(row)) for row in a['rows']],
              'wram_sha256': a['wram_sha256'], 'extra_guards': a['extra_guards']}
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(result, indent=2)+'\n')
    return {k: v for k, v in result.items() if k not in ('state_sha256', 'wram_sha256')}


def compare(reference, repeat, contract, binary, content, out):
    a = authenticate(reference, repeat)
    expected = json.loads(contract.read_text())
    if (expected['case'] != a['case'] or expected['rows_sha256'] != a['rows_sha256'] or
            expected['state_sha256'] != [sha(bytes.fromhex(r)) for r in a['rows']] or
            expected['wram_sha256'] != a['wram_sha256']):
        raise ValueError('reference differs from preregistered expectation')
    landings = landing_frames(a['rows'])
    restores = sorted({f for landing in landings for f in (landing-1, landing)})
    if (not landings or 1849-landings[0] < 100 or expected['landing_frames'] != landings or
            expected['restores'] != restores):
        raise ValueError('landing/restoration contract differs')
    trial.check_content(content)
    binary = binary.resolve()
    with tempfile.TemporaryDirectory(prefix='zz-player-native-') as directory:
        root = Path(directory)
        private_content = root/'content'
        private_content.mkdir()
        for name in trial.content_inventory():
            shutil.copyfile(content/name, private_content/name)
        seed = root/'seed.bin'
        inputs = root/'inputs.txt'
        seed.write_bytes(bytes.fromhex(a['rows'][0]))
        inputs.write_text(trial.controller_rows(a['case']))
        native = trial.execute(binary, seed, private_content, inputs)
        if native != a['rows']:
            for frame, (actual, wanted) in enumerate(zip(native, a['rows']), 1649):
                if actual != wanted:
                    raise ValueError(f'first native divergence at {frame}')
            raise ValueError('native frame count differs')
        for frame in restores:
            seed.write_bytes(bytes.fromhex(native[frame-1649]))
            inputs.write_text(trial.controller_rows(a['case'], frame+1))
            if trial.execute(binary, seed, private_content, inputs) != a['rows'][frame-1649:]:
                raise ValueError(f'fresh-process restore differs at {frame}')
    report = {'status': 'passed', 'case': a['case'], 'updates': 200, 'state_bytes': 395,
              'landing_frames': landings, 'post_landing_updates': 1849-landings[0],
              'fresh_process_restores': restores, 'rows_sha256': a['rows_sha256'],
              'native_binary_sha256': sha(binary.read_bytes()),
              'native_inputs': 'one seed, authenticated immutable content, controllers only'}
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(report, indent=2)+'\n')
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='command', required=True)
    capture = sub.add_parser('capture')
    capture.add_argument('--case', type=Path, required=True)
    capture.add_argument('--core', type=Path, required=True)
    capture.add_argument('--out', type=Path, required=True)
    for name in ('freeze', 'compare'):
        command = sub.add_parser(name)
        for flag in ('reference', 'repeat', 'out'):
            command.add_argument('--'+flag, type=Path, required=True)
        if name == 'compare':
            for flag in ('contract', 'binary', 'content-dir'):
                command.add_argument('--'+flag, type=Path, required=True)
    args = parser.parse_args()
    try:
        if args.command == 'capture':
            result = trial.capture(trial.load_case(args.case), args.out, args.core, EXTRA_GUARDS)
        elif args.command == 'freeze':
            result = freeze(args.reference, args.repeat, args.out)
        else:
            result = compare(args.reference, args.repeat, args.contract, args.binary, args.content_dir, args.out)
        print(json.dumps(result, indent=2))
    except (ValueError, OSError, subprocess.TimeoutExpired) as error:
        parser.exit(1, f'{error}\n')

if __name__ == '__main__':
    main()
