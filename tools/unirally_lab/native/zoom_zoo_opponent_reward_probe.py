"""Artificial original-only opponent reward-queue probe; never an acceptance case.

The generic opponent landing producer can submit reward events 1-21 and the
fixed BRONSEN voice range 200-215, but the natural primary trajectory only ever
reaches events 1, 14, 15 and 39. This probe cold-starts the authenticated
primary timeline, verifies every pre-intervention frame against the frozen
original, and then performs exactly the enqueue the producer would perform:
one chosen event byte written at the opponent write cursor ($0CEB+cursor) and
that cursor advanced once ($0D13). Nothing else is touched.

The consumer under recovery is $81C219-C2C9. It mirrors the player consumer
$81C0CE-C18A with the opponent addresses and, unlike the player, adds the
*full* reward word to vertical boost rather than half. The probe supplies the
original's own answer for reward events the natural run never reaches, so the
native consumer is recovered from evidence rather than from the analogy alone.

This is an internal mechanics experiment. It cold-starts from the original CPU
and is explicitly not native initialization, not a seed-based playable
fallback, and not a start-to-result acceptance case.
"""
from __future__ import annotations
import argparse
import ctypes
import json
from pathlib import Path
import subprocess
import tempfile

from .zoom_zoo_playable import ROLL_WORDS
from .zoom_zoo_trial import BUTTONS
from .zoom_zoo_race_reference import project
from .zoom_zoo_trial_reference import ROOT, ROM_SHA, CORE_SHA, PRIMARY_SHA, sha, digest
from ..reference.bsnes import BsnesCore

OPPONENT_ENTRIES = 0xceb      # $0CEB-$0D0A, 32 ring entries.
OPPONENT_READ = 0xd11         # $0D11 read cursor.
OPPONENT_WRITE = 0xd13        # $0D13 write cursor.
OPPONENT_COOLDOWN = 0xca7     # $0CA7 publication cooldown.
OPPONENT_WEIGHTS = 0x2102     # $7E2102-$7E211B learned weights, event-1 indexed.
OPPONENT_BOOST = 0x11db
OPPONENT_VERTICAL_BOOST = 0x11e1
OPPONENT_FEATURE_TOTAL = 0x825  # cartridge $770825.


def _row(wram: bytes, sram: bytes, frame: int, paused_updates: int, countdown_paused: int) -> bytes:
    """The frozen 742-byte race projection, identical to the playable freeze."""
    row = bytearray(project(wram, sram, frame) + wram[0xff1:0xff3] + wram[0x1261:0x1265] + b'\0\0')
    row[7] = ord('B')
    charge = wram[0xd53:0xd57]
    if any(int.from_bytes(charge[i:i+2], 'little') > 1 for i in (0, 2)):
        raise ValueError('charge flag is not binary')
    announcements = (wram[0xcc1:0xce1] + wram[0xce7:0xce8] + wram[0xce9:0xcea] + wram[0xca5:0xca7] +
                     sram[0x7bb:0x7bd] + wram[0x20e8:0x20e9] + wram[0x12e3:0x12e5] +
                     wram[0x12eb:0x12ed] + wram[0x12ef:0x12f1] + wram[0x3ed:0x3ef])
    rolls = b''.join(wram[a+2*r:a+2*r+2] for r in (0, 1) for a in ROLL_WORDS)
    weights = wram[0x20e9:0x2102] + wram[0x2103:0x211c]
    pause = wram[0xef3:0xef7] + paused_updates.to_bytes(4, 'little') + countdown_paused.to_bytes(4, 'little')
    row += sram[0x106f:0x1073] + sram[0x618:0x61c] + charge + announcements + rolls + weights + pause
    if len(row) != 742:
        raise AssertionError('probe row width changed')
    return bytes(row)


def _observation(wram: bytes, sram: bytes) -> dict:
    word = lambda a: int.from_bytes(wram[a:a+2], 'little')
    return dict(entries=list(wram[OPPONENT_ENTRIES:OPPONENT_ENTRIES+32]),
                read_cursor=wram[OPPONENT_READ], write_cursor=wram[OPPONENT_WRITE],
                cooldown=word(OPPONENT_COOLDOWN), boost=word(OPPONENT_BOOST),
                vertical_boost=word(OPPONENT_VERTICAL_BOOST),
                feature_total=int.from_bytes(sram[OPPONENT_FEATURE_TOTAL:OPPONENT_FEATURE_TOTAL+2], 'little'),
                event_one_weight=wram[OPPONENT_WEIGHTS],
                learned_weights=list(wram[OPPONENT_WEIGHTS+1:OPPONENT_WEIGHTS+26]))


def probe(reference: Path, core_path: Path, event: int, before_frame: int, through: int, out: Path) -> dict:
    if out.exists():
        raise ValueError('fresh probe output required')
    if not 1 <= event <= 255:
        raise ValueError('probe event must be a single byte')
    if not 1650 < before_frame < through <= 6400:
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
            rows: list[str] = []
            captured: list[dict] = []
            intervention = None
            for frame in range(through+1):
                if frame == before_frame:
                    before = core.wram()
                    cursor = before[OPPONENT_WRITE]
                    if before[OPPONENT_ENTRIES+cursor] != 0:
                        raise ValueError('probe refuses to overwrite a pending original entry')
                    size = ctypes.c_size_t()
                    pointer = core._lib.unirally_memory(0, ctypes.byref(size))
                    if not pointer or size.value != 131072:
                        raise ValueError('unexpected original WRAM size')
                    advanced = (cursor + 1) & 31
                    if advanced == before[OPPONENT_READ]:
                        raise ValueError('probe refuses to fill the opponent ring')
                    if cursor == before[OPPONENT_READ]:
                        # $81C5CD-C5D0 returns without publishing when the
                        # cursors already meet, so this would not be the
                        # enqueue the producer performs.
                        raise ValueError('the opponent ring is already at its publication limit')
                    ctypes.memmove(pointer + OPPONENT_ENTRIES + cursor, bytes([event]), 1)
                    ctypes.memmove(pointer + OPPONENT_WRITE, bytes([advanced]), 1)
                    modified = core.wram()
                    changed = {i for i in range(131072) if modified[i] != before[i]}
                    if changed != {OPPONENT_ENTRIES+cursor, OPPONENT_WRITE}:
                        raise ValueError('probe intervention changed unexpected memory')
                    intervention = dict(before_frame=before_frame, event=event, entry_index=cursor,
                                        entry_address=OPPONENT_ENTRIES+cursor, write_cursor_address=OPPONENT_WRITE,
                                        write_cursor=[cursor, advanced],
                                        read_cursor=before[OPPONENT_READ], cooldown=int.from_bytes(
                                            before[OPPONENT_COOLDOWN:OPPONENT_COOLDOWN+2], 'little'),
                                        before_wram_sha256=sha(before), modified_wram_sha256=sha(modified))
                    seed = _row(modified, core.cartridge_ram(), before_frame-1, paused_updates, countdown_paused)
                for port, buttons in enumerate(document['timeline'][frame]):
                    core.set_inputs(port, set(buttons))
                core.run_frame()
                wram, sram = core.wram(), core.cartridge_ram()
                if frame < before_frame:
                    if frame >= 1207 and sha(wram) != document['wram_sha256'][frame-1207]:
                        raise ValueError(f'pre-intervention original WRAM mismatch at {frame}')
                    if frame >= 1207 and sha(sram) != document['sram_sha256'][frame-1207]:
                        raise ValueError(f'pre-intervention original SRAM mismatch at {frame}')
                if frame >= 1376:
                    if previous is not None and (int.from_bytes(previous[0xef3:0xef5], 'little') or
                       (int.from_bytes(wram[0x339:0x33b], 'little') and not int.from_bytes(previous[0xeff:0xf01], 'little'))):
                        paused_updates += 1
                        if int.from_bytes(wram[0xff1:0xff3], 'little') >= 5 and int.from_bytes(previous[0x11c5:0x11c7], 'little'):
                            countdown_paused += 1
                    previous = wram
                if frame >= before_frame:
                    tripped = [f'{item["address"]:04x}' for item in guards
                               if item['address'] not in (0xd53, 0xd55)
                               and item['address'] not in {a+2*r for a in ROLL_WORDS for r in (0, 1)}
                               and item['address'] not in (0x31d, 0x321, 0x339)
                               and int.from_bytes(wram[item['address']:item['address']+item['width']], 'little') != item['value']]
                    rows.append(_row(wram, sram, frame, paused_updates, countdown_paused).hex())
                    captured.append(dict(frame=frame, wram_sha256=sha(wram), sram_sha256=sha(sram),
                                         guards_tripped=tripped, **_observation(wram, sram)))
            if seed is None:
                raise ValueError('probe never reached its intervention frame')
            # A window that ends before the cooldown expires would "pass"
            # without the consumer ever running. Require the read cursor to
            # reach the injected entry inside the captured window.
            consumed = next((o['frame'] for o in captured if o['read_cursor'] == intervention['entry_index']), None)
            if consumed is None:
                raise ValueError('probe window ended before the injected entry was consumed; '
                                 'widen --through past the publication cooldown')
            intervention['consumed_at_frame'] = consumed
            tripped = {o['frame']: o['guards_tripped'] for o in captured if o['guards_tripped']}
            if tripped:
                raise ValueError(f'artificial branch left the guarded original domain: {tripped}')
            (out/'seed.state').write_bytes(seed)
            report = dict(kind='artificial_original_opponent_reward_probe', acceptance=False,
                          scope=('Internal $81C219-C2C9 recovery evidence for reward events the natural primary '
                                 'never reaches. Not native initialization, not a seeded playable fallback, '
                                 'and not a start-to-result acceptance case.'),
                          rom_sha256=ROM_SHA, core_sha256=CORE_SHA,
                          timeline_sha256=document['timeline_sha256'], state_bytes=742,
                          intervention=intervention, frames=[before_frame, through],
                          seed_sha256=sha(seed), rows_sha256=digest(rows), rows=rows, observations=captured)
            (out/'report.json').write_text(json.dumps(report, indent=2)+'\n')
            return report
        finally:
            core.unload()


def freeze(first: Path, second: Path, out: Path) -> dict:
    """Require two independent captures of the same intervention to agree."""
    if out.exists():
        raise ValueError('fresh probe freeze required')
    if first.resolve() == second.resolve():
        raise ValueError('a freeze needs two independent captures, not one directory twice')
    left = json.loads((first/'report.json').read_text())
    right = json.loads((second/'report.json').read_text())
    if left['kind'] != 'artificial_original_opponent_reward_probe' or right['kind'] != left['kind']:
        raise ValueError('both inputs must be opponent reward probes')
    for report in (left, right):
        if report['intervention'].get('consumed_at_frame') is None:
            raise ValueError('probe predates the consumption assertion; recapture before freezing')
    for field in ('rom_sha256', 'core_sha256', 'timeline_sha256', 'intervention', 'frames',
                  'seed_sha256', 'rows_sha256', 'rows', 'observations'):
        if left[field] != right[field]:
            raise ValueError(f'the two probe captures differ at {field}')
    if (first/'seed.state').read_bytes() != (second/'seed.state').read_bytes():
        raise ValueError('the two probe seeds differ')
    result = dict(kind='artificial_opponent_reward_probe_freeze', acceptance=False, scope=left['scope'],
                  event=left['intervention']['event'], intervention=left['intervention'],
                  frames=left['frames'], state_bytes=742, rom_sha256=left['rom_sha256'],
                  core_sha256=left['core_sha256'], timeline_sha256=left['timeline_sha256'],
                  seed_sha256=left['seed_sha256'], rows_sha256=left['rows_sha256'],
                  captures=[str(first), str(second)])
    out.write_text(json.dumps(result, indent=2)+'\n')
    return result


def native(probe_dir: Path, reference: Path, binary: Path, pack: Path) -> dict:
    """Continue the frozen artificial original from its seed and compare every byte.

    The native runner is given only the probe seed, the validated content pack
    and the same controller timeline. It is not given any later original state.
    """
    report = json.loads((probe_dir/'report.json').read_text())
    if report['kind'] != 'artificial_original_opponent_reward_probe':
        raise ValueError('not an opponent reward probe directory')
    seed = (probe_dir/'seed.state').read_bytes()
    if sha(seed) != report['seed_sha256'] or digest(report['rows']) != report['rows_sha256']:
        raise ValueError('probe evidence identity differs')
    if report['intervention'].get('consumed_at_frame') is None:
        raise ValueError('probe predates the consumption assertion; recapture before comparing')
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
    (probe_dir/'native.log').write_text(result.stderr)
    produced = [line.split() for line in result.stdout.splitlines()]
    mismatch = None
    # The runner echoes its seed before the first update, so the update for
    # frame `first` is the second line.
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
    outcome = dict(kind='artificial_opponent_reward_native_probe', acceptance=False,
                   scope=report['scope'], probe=str(probe_dir), event=report['intervention']['event'],
                   frames=report['frames'], state_bytes=742, returncode=result.returncode,
                   observations=len(report['rows']), consumed_at_frame=report['intervention'].get('consumed_at_frame'),
                   seed_sha256=report['seed_sha256'],
                   rows_sha256=report['rows_sha256'], binary_sha256=sha(binary.read_bytes()),
                   pack_sha256=sha(pack.read_bytes()), stderr=result.stderr.strip()[:400],
                   status='passed' if mismatch is None and result.returncode == 0 and
                          len(produced) == len(report['rows'])+1 else 'failed', mismatch=mismatch)
    (probe_dir/'native-compare.json').write_text(json.dumps(outcome, indent=2)+'\n')
    return outcome


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='command', required=True)
    capture = sub.add_parser('capture', help='run the artificial original intervention')
    capture.add_argument('--reference', type=Path, required=True)
    capture.add_argument('--core', type=Path, required=True)
    capture.add_argument('--event', type=int, required=True)
    capture.add_argument('--before-frame', type=int, default=1718)
    capture.add_argument('--through', type=int, default=1726)
    capture.add_argument('--out', type=Path, required=True)
    agree = sub.add_parser('freeze', help='require two captures of one intervention to agree')
    agree.add_argument('--first', type=Path, required=True)
    agree.add_argument('--second', type=Path, required=True)
    agree.add_argument('--out', type=Path, required=True)
    compare = sub.add_parser('native', help='continue the frozen probe natively and compare')
    compare.add_argument('--probe', type=Path, required=True)
    compare.add_argument('--reference', type=Path, required=True)
    compare.add_argument('--binary', type=Path, required=True)
    compare.add_argument('--pack', type=Path, required=True)
    args = parser.parse_args()
    if args.command == 'capture':
        report = probe(args.reference, args.core, args.event, args.before_frame, args.through, args.out)
        print(json.dumps({k: v for k, v in report.items() if k not in ('rows', 'observations')}, indent=2))
    elif args.command == 'freeze':
        print(json.dumps(freeze(args.first, args.second, args.out), indent=2))
    else:
        outcome = native(args.probe, args.reference, args.binary, args.pack)
        print(json.dumps(outcome, indent=2))
        raise SystemExit(0 if outcome['status'] == 'passed' else 1)


if __name__ == '__main__':
    main()
