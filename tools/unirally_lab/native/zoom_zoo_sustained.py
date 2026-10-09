"""M4-14 sustained reference, freeze, differential and fresh-process restore lab.

Original execution is confined to capture. Native children receive one canonical
seed, authenticated static content and controllers in an isolated directory.
"""
from __future__ import annotations
import argparse
import json
from pathlib import Path
import subprocess
import tempfile
from . import zoom_zoo_trial as trial
from .zoom_zoo_trial_reference import ROOT, ROM_SHA, PRIMARY_SHA, sha, digest, project, guards
from ..reference.core_identity import accepted_reference, require_core

RIDER_EXTRA = [('surface_mode', 0xb93), ('mode_angle', 0xb97), ('tile_mode_0b9b', 0xb9b),
               ('leading_support', 0xbab), ('tile_pose_0de3', 0xde3),
               ('animation_delta', 0xbc3), ('tile_pose_0e8f', 0xe8f)]
STATE_BYTES = 423
RECOVERY = ('First full player landing after a nonzero surface-mode episode and subsequent mode exit, '
            'with nonzero horizontal velocity and an authenticated positive progress-count transition '
            'since mode entry. Require 200 later traversal updates; later re-entry is retained, not '
            'excluded. This does not claim monotonic progress or obstacle clearance.')


def load_case(path, horizon):
    case = {'id': 'continuous-right', 'changes': []} if path is None else json.loads(path.read_text())
    if set(case) != {'id', 'changes'} or not isinstance(case['id'], str) or not isinstance(case['changes'], list):
        raise ValueError('invalid case')
    seen = set()
    for change in case['changes']:
        if set(change) != {'from', 'to', 'buttons'}:
            raise ValueError('invalid change')
        first, last = change['from'], change['to']
        if type(first) is not int or type(last) is not int or not 1650 <= first <= last <= horizon:
            raise ValueError('case outside continuation')
        if not isinstance(change['buttons'], list) or len(set(change['buttons'])) != len(change['buttons']) or any(b not in ('right', 'b') for b in change['buttons']):
            raise ValueError('case admits only Right/neutral/B')
        frames = set(range(first, last + 1))
        if seen & frames:
            raise ValueError('overlapping changes')
        seen |= frames
    return case


def sustained_project(wram, sram, frame):
    result = bytearray(project(wram, sram, frame))
    result[:8] = b'URZZ0002'
    for rider in (0, 1):
        for _, address in RIDER_EXTRA:
            result += wram[address + rider*2:address + rider*2 + 2]
    return bytes(result)


def capture(out, library, horizon=3299, case=None):
    """Retain full WRAM for future-state inventory and targeted writer audits."""
    from ..reference.bsnes import BsnesCore
    from ..reference.worker import inputs_for_frame
    from ..replay.manifest import derive_script
    if out.exists() or out.with_suffix('.wram').exists():
        raise ValueError('refusing to overwrite reference')
    if not 3299 <= horizon <= 9999:
        raise ValueError('sustained horizon must be 3299..9999')
    case = case or {'id': 'continuous-right', 'changes': []}
    raw = (ROOT/'tests/manifests/replay/race-crawler-zoom-zoo-3300.json').read_bytes()
    rom = Path((ROOT/'local/rom-location.txt').read_text().strip())
    core_sha = require_core(library)
    if sha(raw) != PRIMARY_SHA or sha(rom.read_bytes()) != ROM_SHA:
        raise ValueError('reference identity differs')
    script = derive_script(json.loads(raw))
    constant_inventory=json.loads((ROOT/'tests/manifests/native/zoom-zoo-sustained-guards.reference.json').read_text())['items']
    rows, whole, guard_rows = [], [], []
    out.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='sustained-reference-', dir=out.parent) as directory:
        core = BsnesCore(library, Path(directory), script.get('core_options'))
        try:
            core.load(rom)
            core.set_serialization_method('Strict')
            with out.with_suffix('.wram').open('xb') as series:
                for frame in range(horizon + 1):
                    inputs = inputs_for_frame(script, frame) if frame < 1650 else {0: trial.frame_buttons(case, frame), 1: set()}
                    for port, buttons in inputs.items():
                        core.set_inputs(port, buttons)
                    core.run_frame()
                    if frame < 1649:
                        continue
                    wram, sram = core.wram(), core.cartridge_ram()
                    for item in constant_inventory:
                        address,width=item['address'],item['width']
                        if int.from_bytes(wram[address:address+width],'little')!=item['value']:
                            raise ValueError(f"new constant-input transition at {frame}, address {address:04x}; inventory recovery required")
                    series.write(wram)
                    rows.append(sustained_project(wram, sram, frame).hex())
                    whole.append(sha(wram))
                    guard_rows.append(guards(wram, sram))
        finally:
            core.unload()
    # The original magic is restored only for checking the accepted seed prefix.
    seed = bytearray.fromhex(rows[0]); seed[7] = ord('1')
    if sha(seed[:394]) != trial.contract()['seed_sha256']:
        raise ValueError('authentic seed differs')
    result = {'kind': 'm4_14_reference', 'frames': [1649, horizon], 'case': case,
              'rom_sha256': ROM_SHA, 'core_sha256': core_sha, 'manifest_sha256': PRIMARY_SHA,
              'rows': rows, 'rows_sha256': digest(rows), 'wram_sha256': whole,
              'guard_rows': guard_rows, 'constant_inventory_sha256': digest(constant_inventory)}
    out.write_text(json.dumps(result, indent=2) + '\n')
    return {'frames': len(rows), 'rows_sha256': digest(rows)}


def word(row, offset):
    return int.from_bytes(bytes.fromhex(row)[offset:offset+2], 'little')


def events(rows):
    landings = [1649+i for i in range(1, len(rows)) if word(rows[i],32) == 0 and
                word(rows[i],34) == 9 and bytes.fromhex(rows[i])[50] == 1]
    entries = [1649+i for i in range(1,len(rows)) if word(rows[i],395) and not word(rows[i-1],395)]
    exits = [1649+i for i in range(1,len(rows)) if not word(rows[i],395) and word(rows[i-1],395)]
    if not entries or not exits:
        raise ValueError('case does not exercise surface-mode entry and exit')
    recovery = next((f for f in landings if f > exits[0] and word(rows[f-1649],20) and
                     0 < ((word(rows[f-1649],61)-word(rows[entries[0]-1649],61)) & 65535) < 32768), None)
    if recovery is None or 1648+len(rows)-recovery < 200:
        raise ValueError('extend the same case for 200 updates after qualified recovery')
    opponent_landings=[1649+i for i in range(1,len(rows)) if word(rows[i],160)==0 and
                       word(rows[i],162)==9 and bytes.fromhex(rows[i])[178]==1]
    boundaries = set()
    for frame in landings+opponent_landings:
        boundaries.update((frame-1,frame))
    boundaries.update((entries[0]-1,entries[0],exits[0]-1,exits[0],entries[-1],exits[-1],recovery,1648+len(rows)-200))
    return {'landing_frames': landings, 'opponent_landing_frames': opponent_landings, 'mode_entries': entries, 'mode_exits': exits,
            'recovery_frame': recovery, 'subsequent_updates': 1648+len(rows)-recovery,
            'restore_frames': sorted(f for f in boundaries if 1649 < f < 1648+len(rows))}


def authenticate(reference, repeat):
    a = json.loads(reference.read_text())
    if a != json.loads(repeat.read_text()):
        raise ValueError('fresh reference processes differ')
    if (a['rom_sha256'],a['manifest_sha256']) != (ROM_SHA,PRIMARY_SHA) or not accepted_reference(a['core_sha256']):
        raise ValueError('reference identities differ')
    expected_guards=json.loads((ROOT/'tests/manifests/native/zoom-zoo-sustained-guards.reference.json').read_text())['items']
    if a.get('constant_inventory_sha256')!=digest(expected_guards):
        raise ValueError('constant input inventory was not authenticated')
    first,last = a['frames']
    if first != 1649 or not 3299 <= last <= 9999 or len(a['rows']) != last-first+1 or digest(a['rows']) != a['rows_sha256']:
        raise ValueError('reference horizon/digest differs')
    for frame,row in enumerate(a['rows'],1649):
        data=bytes.fromhex(row)
        if len(data)!=STATE_BYTES or data[:8]!=b'URZZ0002' or int.from_bytes(data[8:12],'little')!=frame:
            raise ValueError('reference framing differs')
    seed=bytearray.fromhex(a['rows'][0]);seed[7]=ord('1')
    if sha(seed[:394])!=trial.contract()['seed_sha256']:
        raise ValueError('authentic seed differs')
    return a


def freeze(reference, repeat, out):
    if out.exists():
        raise ValueError('refusing to overwrite frozen reference')
    a=authenticate(reference,repeat)
    result={k:a[k] for k in ('frames','case','rom_sha256','core_sha256','manifest_sha256','rows_sha256','wram_sha256')}
    result.update(kind='m4_14_sustained_freeze',state_bytes=STATE_BYTES,rider_extra=RIDER_EXTRA,
                  recovery_criterion=RECOVERY,state_sha256=[sha(bytes.fromhex(r)) for r in a['rows']],**events(a['rows']))
    out.parent.mkdir(parents=True,exist_ok=True)
    out.write_text(json.dumps(result,indent=2)+'\n')
    return {k:result[k] for k in ('frames','recovery_frame','subsequent_updates','restore_frames')}


def inventory():
    result=trial.content_inventory()
    result.update(json.loads((ROOT/'tests/manifests/native/zoom-zoo-sustained-content.reference.json').read_text())['items'])
    return result


def extract_content(out, library):
    from .zoom_zoo_trial_extract import extract
    extract(out,library)
    rom=Path((ROOT/'local/rom-location.txt').read_text().strip()).read_bytes()
    if sha(rom)!=ROM_SHA:
        raise ValueError('ROM identity differs')
    for name,item in json.loads((ROOT/'tests/manifests/native/zoom-zoo-sustained-content.reference.json').read_text())['items'].items():
        data=rom[item['file_offset']:item['file_offset']+item['bytes']]
        if sha(data)!=item['sha256']:
            raise ValueError('static extraction identity differs')
        (out/name).write_bytes(data)
    return {'status':'passed','items':inventory()}


def controller_rows(case, first, last):
    return ''.join(f"{f} {sum(1<<trial.BUTTONS.index(b) for b in trial.frame_buttons(case,f))} 0\n" for f in range(first,last+1))


def execute(binary, seed, content, inputs):
    result=subprocess.run([str(binary),'--seed',str(seed),'--content-dir',str(content),'--inputs',str(inputs)],
                          capture_output=True,text=True,timeout=30,cwd=seed.parent)
    rows=[]
    for line in result.stdout.splitlines():
        frame,row=line.split();data=bytes.fromhex(row)
        if len(data)!=STATE_BYTES or int.from_bytes(data[8:12],'little')!=int(frame):
            raise ValueError('native framing differs')
        rows.append(row)
    if result.returncode:
        raise ValueError(f'native exit {result.returncode}: {result.stderr}')
    return rows


def compare(reference, repeat, contract, binary, content, out):
    if out.exists():
        raise ValueError('comparison report must be fresh')
    a=authenticate(reference,repeat);frozen=json.loads(contract.read_text());rows=a['rows']
    for key in ('frames','case','rom_sha256','core_sha256','manifest_sha256','rows_sha256','wram_sha256'):
        if a[key]!=frozen[key]:
            raise ValueError(f'frozen {key} differs')
    if frozen['rider_extra']!=[list(item) for item in RIDER_EXTRA]:
        raise ValueError('frozen field layout differs')
    if [sha(bytes.fromhex(r)) for r in rows]!=frozen['state_sha256'] or frozen['state_bytes']!=STATE_BYTES:
        raise ValueError('frozen state inventory differs')
    observed=events(rows)
    if observed['recovery_frame']!=frozen['recovery_frame'] or frozen['recovery_criterion']!=RECOVERY:
        raise ValueError('frozen recovery differs')
    binary=binary.resolve();content=content.resolve();last=a['frames'][1]
    with tempfile.TemporaryDirectory(prefix='sustained-native-') as directory:
        root=Path(directory);private=root/'content';private.mkdir()
        for name,identity in inventory().items():
            data=(content/name).read_bytes()
            if len(data)!=identity['bytes'] or sha(data)!=identity['sha256']:
                raise ValueError(f'static content differs: {name}')
            (private/name).write_bytes(data)
        seed=root/'seed.bin';seed.write_bytes(bytes.fromhex(rows[0]));inputs=root/'inputs.txt'
        inputs.write_text(controller_rows(a['case'],1650,last))
        native=execute(binary,seed,private,inputs)
        if native!=rows:
            for i,(actual,expected) in enumerate(zip(native,rows)):
                if actual!=expected:
                    differences=[n for n,(x,y) in enumerate(zip(bytes.fromhex(actual),bytes.fromhex(expected))) if x!=y]
                    raise ValueError(f'first divergence at {1649+i}, byte offsets {differences}')
            raise ValueError('native frame count differs')
        if execute(binary,seed,private,inputs)!=rows:
            raise ValueError('native fresh-process repeat differs')
        for frame in observed['restore_frames']:
            seed.write_bytes(bytes.fromhex(native[frame-1649]));inputs.write_text(controller_rows(a['case'],frame+1,last))
            if execute(binary,seed,private,inputs)!=rows[frame-1649:]:
                raise ValueError(f'fresh-process restore differs at {frame}')
    result={'status':'passed','case':a['case'],'frames':a['frames'],'updates':last-1649,'state_bytes':STATE_BYTES,
            'rows_sha256':a['rows_sha256'],'native_binary_sha256':sha(binary.read_bytes()),
            'native_inputs':'one seed, authenticated immutable content, controllers only',**observed}
    out.parent.mkdir(parents=True,exist_ok=True);out.write_text(json.dumps(result,indent=2)+'\n')
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__);sub=parser.add_subparsers(dest='command',required=True)
    ext=sub.add_parser('extract-content');ext.add_argument('--core',type=Path,required=True);ext.add_argument('--out',type=Path,required=True)
    cap=sub.add_parser('capture');cap.add_argument('--core',type=Path,required=True);cap.add_argument('--out',type=Path,required=True)
    cap.add_argument('--horizon',type=int,default=3299);cap.add_argument('--case',type=Path)
    for name in ('freeze','compare'):
        cmd=sub.add_parser(name)
        for arg in ('reference','repeat','out'):cmd.add_argument('--'+arg,type=Path,required=True)
        if name=='compare':
            for arg in ('contract','binary','content-dir'):cmd.add_argument('--'+arg,type=Path,required=True)
    args=parser.parse_args()
    try:
        if args.command=='extract-content':result=extract_content(args.out,args.core)
        elif args.command=='capture':result=capture(args.out,args.core,args.horizon,load_case(args.case,args.horizon))
        elif args.command=='freeze':result=freeze(args.reference,args.repeat,args.out)
        else:result=compare(args.reference,args.repeat,args.contract,args.binary,args.content_dir,args.out)
        print(json.dumps(result,indent=2))
    except (ValueError,OSError,subprocess.TimeoutExpired) as error:
        parser.exit(1,str(error)+'\n')


if __name__=='__main__':main()
