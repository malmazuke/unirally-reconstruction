"""Artificial original-only opponent multi-axis trick probe; never acceptance.

`$83E1CB-E21A` picks the opponent's trick on a jump marker. On a flat surface it
only chooses a rotation direction, but on a slope it takes `selector = x & 7`
and then presses three independent inputs: bit 0 the rotation, bit 1 the
opponent's A at `$031F`, bit 2 the opponent's X at `$0323`. Six of the eight
values are unrecovered natively.

No reference capture reaches them. The branch is gated on
`feature_total == 0 || player transitions - opponent transitions >= 3`, and in
every capture the opponent leaves `feature_total` zero at frame 1672 while the
player never leads. The primary nevertheless passes through 148 frames where the
opponent is airborne on a slope under a jump marker, with `x & 7` spanning all
eight values, so forcing *only* the gate lets the original itself choose the
selector and show what it does.

The intervention is therefore one SRAM word: the opponent feature total at
`$770825` set to zero immediately before the chosen frame. That word is read by
twenty-two sites across banks `$80`-`$83`, not only by this gate, so it is the
*smallest* lever rather than a gate-private one; it is preferred over forcing
the transition counts, which additionally feed the speed limiter's progress
terms, the progress adjustment and the lap logic. Both sides of the comparison
see the same forced value and the field is inside the projection, so the
differential stays sound. Everything
downstream, including the selector, is the original's own arithmetic. This is an
internal mechanics experiment, not native initialization and not a playable
acceptance case.
"""
from __future__ import annotations
import argparse
import ctypes
import json
from pathlib import Path
import subprocess
import tempfile

from .zoom_zoo_opponent_reward_probe import _row
from .zoom_zoo_playable import ROLL_WORDS
from .zoom_zoo_trial import BUTTONS
from .zoom_zoo_trial_reference import ROOT, ROM_SHA, CORE_SHA, PRIMARY_SHA, sha, digest

FEATURE_TOTAL = 0x825         # cartridge $770825, the opponent's accumulated reward.
SELECTOR = 0xc75              # $0C75 trick selector.
IMPULSE = 0xc6f               # $0C6F impulse countdown.
SUPPRESSION = 0x1277          # $1277 suppression counter.
OPPONENT_A = 0x31f            # $031F, stride-2 partner of the player's $031D.
OPPONENT_X = 0x323            # $0323, stride-2 partner of the player's $0321.
ROTATE_NEGATIVE = 0x32b
ROTATE_POSITIVE = 0x32f


def _observation(wram: bytes, sram: bytes) -> dict:
    word = lambda a: int.from_bytes(wram[a:a+2], 'little')
    return dict(selector=word(SELECTOR), impulse=word(IMPULSE), suppression=word(SUPPRESSION),
                opponent_a=word(OPPONENT_A), opponent_x=word(OPPONENT_X),
                rotate_negative=word(ROTATE_NEGATIVE), rotate_positive=word(ROTATE_POSITIVE),
                opponent_motion_x=word(0x417), opponent_surface_angle=word(0xb70),
                opponent_velocity_y=word(0x4c1), opponent_marker=word(0xfc7),
                player_transitions=word(0xfcd), opponent_transitions=word(0xfcf),
                feature_total=int.from_bytes(sram[FEATURE_TOTAL:FEATURE_TOTAL+2], 'little'))


def probe(reference: Path, core_path: Path, frame: int, through: int, out: Path) -> dict:
    from ..reference.bsnes import BsnesCore
    if out.exists():
        raise ValueError('fresh probe output required')
    if not 1650 < frame < through <= 6400:
        raise ValueError('probe window must sit inside the raced domain')
    document = json.loads((reference/'reference.json').read_text())
    if (document['rom_sha256'], document['core_sha256'], document['manifest_sha256']) != (ROM_SHA, CORE_SHA, PRIMARY_SHA):
        raise ValueError('probe requires the frozen primary original identity')
    if document['frames'][0] != 1207 or digest(document['timeline']) != document['timeline_sha256']:
        raise ValueError('original timeline identity differs')
    rom_path = Path((ROOT/'local/rom-location.txt').read_text().strip())
    if sha(rom_path.read_bytes()) != ROM_SHA or sha(core_path.read_bytes()) != CORE_SHA:
        raise ValueError('probe identity mismatch')
    guards = json.loads((ROOT/'tests/manifests/native/zoom-zoo-race-guards.reference.json').read_text())['items']
    out.mkdir(parents=True)
    with tempfile.TemporaryDirectory(dir=out) as directory:
        core = BsnesCore(core_path.resolve(), Path(directory), {})
        try:
            core.load(rom_path)
            core.set_serialization_method('Strict')
            paused_updates = countdown_paused = 0
            previous = None
            seed = None
            intervention = None
            rows: list[str] = []
            captured: list[dict] = []
            for step in range(through+1):
                if step == frame:
                    before_wram, before_sram = core.wram(), core.cartridge_ram()
                    if not (int.from_bytes(before_wram[0xfc7:0xfc9], 'little') & 0x2000):
                        raise ValueError('probe frame is not under an opponent jump marker')
                    if int.from_bytes(before_sram[FEATURE_TOTAL:FEATURE_TOTAL+2], 'little') == 0:
                        raise ValueError('opponent feature total is already zero; the gate needs no forcing here')
                    if any(int.from_bytes(before_wram[a:a+2], 'little')
                           for a in (SELECTOR, OPPONENT_A, OPPONENT_X, IMPULSE)):
                        raise ValueError('a trick is already in flight here, so the captured selector would be '
                                         'retained rather than freshly chosen')
                    size = ctypes.c_size_t()
                    pointer = core._lib.unirally_memory(1, ctypes.byref(size))
                    if not pointer or size.value != 8192:
                        raise ValueError('unexpected original cartridge RAM size')
                    ctypes.memmove(pointer + FEATURE_TOTAL, (0).to_bytes(2, 'little'), 2)
                    modified = core.cartridge_ram()
                    changed = {i for i in range(8192) if modified[i] != before_sram[i]}
                    # A low total leaves its high byte already zero, so the
                    # change is a subset of the word rather than both bytes.
                    if not changed or not changed <= {FEATURE_TOTAL, FEATURE_TOTAL+1} or core.wram() != before_wram:
                        raise ValueError('probe intervention changed unexpected memory')
                    intervention = dict(frame=frame, lever='cartridge $770825 opponent feature total set to 0',
                                        address=FEATURE_TOTAL, width=2,
                                        feature_total_before=int.from_bytes(
                                            before_sram[FEATURE_TOTAL:FEATURE_TOTAL+2], 'little'),
                                        opponent_motion_x=int.from_bytes(before_wram[0x417:0x419], 'little'),
                                        expected_selector=int.from_bytes(before_wram[0x417:0x419], 'little') & 7,
                                        before_sram_sha256=sha(before_sram), modified_sram_sha256=sha(modified))
                    seed = _row(before_wram, modified, frame-1, paused_updates, countdown_paused)
                for port, buttons in enumerate(document['timeline'][step]):
                    core.set_inputs(port, set(buttons))
                core.run_frame()
                wram, sram = core.wram(), core.cartridge_ram()
                if step < frame and step >= 1207:
                    if sha(wram) != document['wram_sha256'][step-1207] or sha(sram) != document['sram_sha256'][step-1207]:
                        raise ValueError(f'pre-intervention original mismatch at {step}')
                if step >= 1376:
                    if previous is not None and (int.from_bytes(previous[0xef3:0xef5], 'little') or
                       (int.from_bytes(wram[0x339:0x33b], 'little') and not int.from_bytes(previous[0xeff:0xf01], 'little'))):
                        paused_updates += 1
                        if int.from_bytes(wram[0xff1:0xff3], 'little') >= 5 and int.from_bytes(previous[0x11c5:0x11c7], 'little'):
                            countdown_paused += 1
                    previous = wram
                if step >= frame:
                    rows.append(_row(wram, sram, step, paused_updates, countdown_paused).hex())
                    captured.append(dict(frame=step, wram_sha256=sha(wram), sram_sha256=sha(sram),
                                         guards_tripped=[f'{item["address"]:04x}' for item in guards
                                                         if item['address'] not in (0xd53, 0xd55)
                                                         and item['address'] not in {a+2*r for a in ROLL_WORDS for r in (0, 1)}
                                                         and item['address'] not in (0x31d, 0x321, 0x339, 0x31f, 0x323)
                                                         and int.from_bytes(wram[item['address']:item['address']+item['width']],
                                                                            'little') != item['value']],
                                         **_observation(wram, sram)))
            if seed is None:
                raise ValueError('probe never reached its intervention frame')
            tripped = {o['frame']: o['guards_tripped'] for o in captured if o['guards_tripped']}
            if tripped:
                raise ValueError(f'artificial branch left the guarded original domain: {tripped}')
            fired = [o for o in captured if o['selector'] or o['opponent_a'] or o['opponent_x']]
            if not fired:
                raise ValueError('the opponent trick never fired in this window; choose a frame one after a '
                                 'qualifying stored state, since an impulse already counting down ($0C6F set) '
                                 'replays the retained selector instead of choosing a new one')
            selector = fired[0]['selector']
            # The AI reads position and surface angle part-way through the
            # update, so the pre-frame x&7 is a hint for choosing a frame, not
            # a prediction. Record both and let the observation stand.
            intervention['preframe_x_and_7'] = intervention.pop('expected_selector')
            intervention['fired_at_frame'] = fired[0]['frame']
            intervention['selector'] = selector
            intervention['multi_axis'] = bool(selector & 6)
            (out/'seed.state').write_bytes(seed)
            report = dict(kind='artificial_original_opponent_trick_probe', acceptance=False,
                          scope=('Internal $83E1CB-E21A recovery evidence for opponent trick selectors the '
                                 'natural race never reaches. Only the gate is forced; the selector and every '
                                 'later value are the original\'s own. Not native initialization, not a seeded '
                                 'playable fallback, and not a start-to-result acceptance case.'),
                          rom_sha256=ROM_SHA, core_sha256=CORE_SHA,
                          timeline_sha256=document['timeline_sha256'], state_bytes=742,
                          intervention=intervention, frames=[frame, through],
                          seed_sha256=sha(seed), rows_sha256=digest(rows), rows=rows, observations=captured)
            (out/'report.json').write_text(json.dumps(report, indent=2)+'\n')
            return report
        finally:
            core.unload()


def freeze(first: Path, second: Path, out: Path) -> dict:
    if first.resolve() == second.resolve():
        raise ValueError('a freeze needs two independent captures, not one directory twice')
    if out.exists():
        raise ValueError('fresh probe freeze required')
    left = json.loads((first/'report.json').read_text())
    right = json.loads((second/'report.json').read_text())
    if left['kind'] != 'artificial_original_opponent_trick_probe' or right['kind'] != left['kind']:
        raise ValueError('both inputs must be opponent trick probes')
    for field in ('rom_sha256', 'core_sha256', 'timeline_sha256', 'intervention', 'frames',
                  'seed_sha256', 'rows_sha256', 'rows', 'observations'):
        if left[field] != right[field]:
            raise ValueError(f'the two probe captures differ at {field}')
    result = dict(kind='artificial_opponent_trick_probe_freeze', acceptance=False, scope=left['scope'],
                  selector=left['intervention']['selector'], intervention=left['intervention'],
                  frames=left['frames'], state_bytes=742, rom_sha256=left['rom_sha256'],
                  core_sha256=left['core_sha256'], timeline_sha256=left['timeline_sha256'],
                  seed_sha256=left['seed_sha256'], rows_sha256=left['rows_sha256'],
                  captures=[str(first), str(second)])
    out.write_text(json.dumps(result, indent=2)+'\n')
    return result


def native(probe_dir: Path, reference: Path, binary: Path, pack: Path) -> dict:
    report = json.loads((probe_dir/'report.json').read_text())
    if report['kind'] != 'artificial_original_opponent_trick_probe':
        raise ValueError('not an opponent trick probe directory')
    seed = (probe_dir/'seed.state').read_bytes()
    if sha(seed) != report['seed_sha256'] or digest(report['rows']) != report['rows_sha256']:
        raise ValueError('probe evidence identity differs')
    document = json.loads((reference/'reference.json').read_text())
    if document['timeline_sha256'] != report['timeline_sha256']:
        raise ValueError('probe and reference timelines differ')
    first, last = report['frames']
    inputs = probe_dir/'native-inputs.txt'
    inputs.write_text(''.join(f'{f} {sum(1 << BUTTONS.index(b) for b in document["timeline"][f][0])} 0\n'
                              for f in range(first, last+1)))
    result = subprocess.run([str(binary), '--seed', str(probe_dir/'seed.state'), '--content-pack', str(pack),
                             '--inputs', str(inputs)], capture_output=True, text=True)
    (probe_dir/'native.txt').write_text(result.stdout)
    produced = [line.split() for line in result.stdout.splitlines()]
    mismatch = None
    if not produced or int(produced[0][0]) != first-1 or produced[0][1] != seed.hex():
        mismatch = dict(frame=first-1, reason='native did not start from the frozen probe seed')
    for index, expected in enumerate(report['rows']):
        if mismatch:
            break
        if index+1 >= len(produced):
            mismatch = dict(frame=first+index, reason='native produced no state'); break
        frame, actual = int(produced[index+1][0]), produced[index+1][1]
        if frame != first+index:
            mismatch = dict(frame=first+index, reason=f'native reported frame {frame}'); break
        if actual != expected:
            mismatch = dict(frame=frame, reason='state differs',
                            bytes=[[i, a, b] for i, (a, b) in enumerate(zip(bytes.fromhex(actual),
                                                                           bytes.fromhex(expected))) if a != b][:24])
            break
    outcome = dict(kind='artificial_opponent_trick_native_probe', acceptance=False, scope=report['scope'],
                   selector=report['intervention']['selector'], frames=report['frames'], state_bytes=742,
                   returncode=result.returncode, observations=len(report['rows']),
                   stderr=result.stderr.strip()[:400],
                   status='passed' if mismatch is None and result.returncode == 0 and
                          len(produced) == len(report['rows'])+1 else 'failed', mismatch=mismatch)
    (probe_dir/'native-compare.json').write_text(json.dumps(outcome, indent=2)+'\n')
    return outcome


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='command', required=True)
    capture = sub.add_parser('capture')
    capture.add_argument('--reference', type=Path, required=True)
    capture.add_argument('--core', type=Path, required=True)
    capture.add_argument('--frame', type=int, required=True)
    capture.add_argument('--through', type=int, required=True)
    capture.add_argument('--out', type=Path, required=True)
    agree = sub.add_parser('freeze')
    agree.add_argument('--first', type=Path, required=True)
    agree.add_argument('--second', type=Path, required=True)
    agree.add_argument('--out', type=Path, required=True)
    compare = sub.add_parser('native')
    compare.add_argument('--probe', type=Path, required=True)
    compare.add_argument('--reference', type=Path, required=True)
    compare.add_argument('--binary', type=Path, required=True)
    compare.add_argument('--pack', type=Path, required=True)
    args = parser.parse_args()
    if args.command == 'capture':
        r = probe(args.reference, args.core, args.frame, args.through, args.out)
        print(json.dumps({k: v for k, v in r.items() if k not in ('rows', 'observations')}, indent=2))
    elif args.command == 'freeze':
        print(json.dumps(freeze(args.first, args.second, args.out), indent=2))
    else:
        o = native(args.probe, args.reference, args.binary, args.pack)
        print(json.dumps(o, indent=2))
        raise SystemExit(0 if o['status'] == 'passed' else 1)


if __name__ == '__main__':
    main()
