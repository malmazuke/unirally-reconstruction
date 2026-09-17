"""M4-16 original freeze and pack-only native start/result/restore gate.

Original result loading reuses race WRAM. The canonical result retains an
archived final race and a semantic load phase; it does not interpret overwritten
rendering bytes as rider state. Race rows remain byte-exact original projections.
"""
from __future__ import annotations
import argparse
import json
from pathlib import Path
import subprocess
import tempfile
from .zoom_zoo_trial_reference import ROOT,ROM_SHA,CORE_SHA,PRIMARY_SHA,sha,digest
from .zoom_zoo_race_reference import project
from .zoom_zoo_race import restore_frames
from .zoom_zoo_trial import BUTTONS
from ..content.pack import load_rules,validate_pack,TWO_TRACK_RULES_PATH


ROLL_WORDS=[0x1215,0x123f,0x121b,0x1221,0x124f,0xfdf,0x1009,0x42f,0x433,0x42b,0x54b,0x124b]

def original(directory, *, allow_incomplete=False):
    document=json.loads((directory/'reference.json').read_text())
    if (document['rom_sha256'],document['core_sha256'],document['manifest_sha256'])!=(ROM_SHA,CORE_SHA,PRIMARY_SHA):
        raise ValueError('original identity differs')
    first,last=document['frames']
    if first!=1207 or len(document['timeline'])!=last+1 or digest(document['timeline'])!=document['timeline_sha256']:
        raise ValueError('original timeline identity differs')
    if len(document['wram_sha256'])!=last-first+1 or len(document['sram_sha256'])!=last-first+1:
        raise ValueError('original memory inventory incomplete')
    rows=[];finish=[None,None];loading=None;archive=None;charge_archive=None;announcements_archive=None;roll_archive=None;weights_archive=None;pause_archive=None;paused_updates=0;countdown_paused=0;previous=None
    guards=json.loads((ROOT/"tests/manifests/native/zoom-zoo-race-guards.reference.json").read_text())["items"]
    with (directory/'memory.wram').open('rb') as ws,(directory/'memory.sram').open('rb') as ss:
        for frame in range(first,last+1):
            w,s=ws.read(131072),ss.read(8192)
            if len(w)!=131072 or len(s)!=8192 or sha(w)!=document['wram_sha256'][frame-first] or sha(s)!=document['sram_sha256'][frame-first]:
                raise ValueError(f'original memory differs at {frame}')
            if frame<1376:continue
            if loading is None:
                for rider in (0,1):
                    if finish[rider] is None and int.from_bytes(w[0xeff+rider*2:0xf01+rider*2],'little'):
                        finish[rider]=frame
                if archive is not None and int.from_bytes(archive[515:517],'little')==240:
                    loading=frame
            if loading is None:
                if frame>=1649:
                    for item in guards:
                        at=item['address']
                        if at in (0xd53,0xd55):continue # Now serialized, no longer a constant-domain guard.
                        if at in {a+2*r for a in ROLL_WORDS for r in (0,1)}:continue # Explicit original roll projection below.
                        if at in (0x31d,0x321,0x339):
                            if int.from_bytes(w[at:at+2],'little')!=int(({0x31d:'a',0x321:'x',0x339:'start'}[at]) in document['timeline'][frame][0]):
                                raise ValueError('A input publication differs from controller timeline')
                            continue # $82AAA4-AAB4; redundant with serialized controller low image.
                        if int.from_bytes(w[at:at+item['width']],'little')!=item['value']:
                            raise ValueError(f'new gameplay guard at {frame}: {at:04x}; recover before evaluating native')
                row=bytearray(project(w,s,frame)+w[0xff1:0xff3]+w[0x1261:0x1265]+b'\0\0')
                row[7]=ord('B');archive=bytearray(row);charge_archive=w[0xd53:0xd57]
                announcements_archive=w[0xcc1:0xce1]+w[0xce7:0xce8]+w[0xce9:0xcea]+w[0xca5:0xca7]+s[0x7bb:0x7bd]+w[0x20e8:0x20e9]+w[0x12e3:0x12e5]+w[0x12eb:0x12ed]+w[0x12ef:0x12f1]+w[0x3ed:0x3ef]
                roll_archive=b''.join(w[a+2*r:a+2*r+2] for r in (0,1) for a in ROLL_WORDS)
                weights_archive=w[0x20e9:0x2102]+w[0x2103:0x211c]
                # $83CD05-CD35 diverts this update before race work. Count
                # that observed branch semantically; phase clocks still run.
                if previous is not None and (int.from_bytes(previous[0xef3:0xef5],'little') or
                   (int.from_bytes(w[0x339:0x33b],'little') and not int.from_bytes(previous[0xeff:0xf01],'little'))):
                    paused_updates+=1
                    if int.from_bytes(w[0xff1:0xff3],'little')>=5 and int.from_bytes(previous[0x11c5:0x11c7],'little'):
                        countdown_paused+=1
                pause_archive=w[0xef3:0xef7]+paused_updates.to_bytes(4,'little')+countdown_paused.to_bytes(4,'little')
                if any(int.from_bytes(charge_archive[i:i+2],'little')>1 for i in (0,2)):raise ValueError('charge flag is not binary')
            else:
                row=bytearray(archive);row[8:12]=frame.to_bytes(4,'little')
                row[-2:]=min(115,frame-loading+1).to_bytes(2,'little')
                # Future-relevant race result values survive in cartridge RAM.
                wanted=s[0x755:0x769]+s[0x7bf:0x7d3]+s[0x769:0x76b]+s[0x7d3:0x7d5]
                if row[467:511]!=wanted:raise ValueError('result lap/total archive differs from original')
            previous=w
            row+=s[0x106f:0x1073]+s[0x618:0x61c]+charge_archive+announcements_archive+roll_archive+weights_archive+pause_archive
            rows.append(row.hex())
        if ws.read(1) or ss.read(1):raise ValueError('extra original memory')
    if loading is None or any(f is None for f in finish):
        if allow_incomplete:return document,rows,dict(complete=False,finish_frames=finish,scope='Internal recovery experiment only; does not satisfy playable acceptance')
        raise ValueError('case must finish both riders')
    # Freeze visible readiness from original video, independently of native.
    black=document['video'][loading+75-first]
    visible=next((f for f in range(loading+76,last+1) if document['video'][f-first]!=black),None)
    if visible!=loading+108:raise ValueError(f'new result load timing requires recovery: {visible}, expected {loading+108}')
    stable=visible+6
    if last-stable<200:raise ValueError('200 stable result updates required')
    return document,rows,dict(finish_frames=finish,loading_frame=loading,first_visible=visible,stable_result=stable,
                            outcome='player_won' if finish[0]<finish[1] else 'player_lost')


def freeze(a,b,out):
    if out.exists():raise ValueError('fresh freeze required')
    left,rows,events=original(a);right,other,repeated=original(b)
    if left!=right or rows!=other or events!=repeated:raise ValueError('two original runs differ')
    result=dict(kind='m4_16_playable_freeze',frames=[1376,left['frames'][1]],state_bytes=742,
                original_sha256=digest(left),rows_sha256=digest(rows),events=events,
                timeline_sha256=left['timeline_sha256'],rom_sha256=ROM_SHA,core_sha256=CORE_SHA,
                result_inventory='Race bytes are original projections until loading; thereafter frozen archive is checked against surviving SRAM lap/totals. Graph extrema at SRAM106f/1071 and published totals618/61a are independently projected every frame. Load count is a semantic clock. Tour records excluded by fresh-scenario restart.')
    out.write_text(json.dumps(result,indent=2)+'\n');return result


def compare(a,b,contract,binary,pack,out):
    if out.exists():raise ValueError('fresh report required')
    binary=binary.resolve();pack=pack.resolve()
    head=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT);diff=sha(subprocess.check_output(['git','diff','HEAD'],cwd=ROOT));binary_sha=sha(binary.read_bytes());pack_sha=sha(pack.read_bytes())
    reference,rows,events=original(a);repeat,other,other_events=original(b);frozen=json.loads(contract.read_text())
    if reference!=repeat or rows!=other or events!=other_events:raise ValueError('original repeats differ')
    expected_inventory=dict(kind='m4_16_playable_freeze',frames=[1376,reference['frames'][1]],state_bytes=742,
                            original_sha256=digest(reference),rows_sha256=digest(rows),events=events,
                            timeline_sha256=reference['timeline_sha256'],rom_sha256=ROM_SHA,core_sha256=CORE_SHA)
    if any(frozen.get(key)!=value for key,value in expected_inventory.items()):
        raise ValueError('frozen original inventory differs')
    rules,rules_sha=load_rules(ROOT/TWO_TRACK_RULES_PATH);validate_pack(pack.read_bytes(),rules,rules_sha)
    last=reference['frames'][1]
    boundaries={1376,1377,1380,1381,1550,1551,1582,1583,1649,events['loading_frame']-1,events['loading_frame'],events['first_visible'],events['stable_result'],last-1}
    boundaries.update(restore_frames(rows[1649-1376:events['loading_frame']-1376]))
    # Restore across queue publication/consumption, learned reward weight,
    # tutorial interruption/group changes and charge latches. Cooldowns/ticks
    # alone intentionally do not turn every ordinary update into a restore.
    for i in range(1,events['loading_frame']-1376):
        before,after=bytes.fromhex(rows[i-1]),bytes.fromhex(rows[i])
        if any(before[a:b]!=after[a:b] for a,b in [(12,16),(581,619),(621,626),(628,632),(632,652),(654,676),(678,742)]):
            boundaries.update((1375+i,1376+i))
    boundaries=sorted(f for f in boundaries if 1376<=f<last)
    with tempfile.TemporaryDirectory(prefix='zoom-playable-native-') as directory:
        root=Path(directory);local_pack=root/'classic.pack';local_pack.write_bytes(pack.read_bytes());inputs=root/'inputs.txt';seed=root/'restore.bin'
        def execute(first,restore=None,restart=False):
            inputs.write_text(''.join(f'{f} {sum(1<<BUTTONS.index(b) for b in reference["timeline"][f][0])} 0\n' for f in range(first+1,last+1)))
            start=['--start','classic.crawler.zoom-zoo']
            if restore is not None:seed.write_bytes(bytes.fromhex(restore));start=['--restart-from' if restart else '--seed',str(seed)]
            run=subprocess.run([str(binary),*start,'--content-pack',str(local_pack),'--inputs',str(inputs)],cwd=root,capture_output=True,text=True,timeout=30)
            if run.returncode:raise ValueError(f'native failed: {run.stderr}')
            output=[]
            for f,line in enumerate(run.stdout.splitlines(),first):
                label,row=line.split();data=bytes.fromhex(row)
                if int(label)!=f or len(data)!=742 or data[:8]!=b'URZZ000B' or int.from_bytes(data[8:12],'little')!=f:raise ValueError('native protocol differs')
                output.append(row)
            return output
        actual=execute(1376)
        if actual!=rows:
            for i,(x,y) in enumerate(zip(actual,rows)):
                if x!=y:raise ValueError(f'first mismatch {1376+i}: {[n for n,(v,w) in enumerate(zip(bytes.fromhex(x),bytes.fromhex(y))) if v!=w]}')
            raise ValueError('native frame count differs')
        if execute(1376)!=actual:raise ValueError('fresh native initialization differs')
        if execute(1376,actual[-1],True)!=actual:raise ValueError('restart retains stale race/result state')
        for frame in boundaries:
            if execute(frame,actual[frame-1376])!=actual[frame-1376:]:raise ValueError(f'restore differs at {frame}')
    if (subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT)!=head or
        sha(subprocess.check_output(['git','diff','HEAD'],cwd=ROOT))!=diff or sha(binary.read_bytes())!=binary_sha or sha(pack.read_bytes())!=pack_sha):
        raise ValueError('source/binary/pack changed during validation')
    result=dict(status='passed',source_commit=head.decode().strip(),source_diff_sha256=diff,binary_sha256=binary_sha,
                pack_sha256=pack_sha,contract_sha256=sha(contract.read_bytes()),frames=[1376,last],state_bytes=742,
                rows_sha256=digest(rows),restore_frames=boundaries,events=events,
                comparison_domains=dict(race=[1376,events['loading_frame']-1],race_projected_bytes=724,
                    result_archived_race_bytes=724,result_original_publication_bytes=8,semantic_loading_clock_bytes=2,semantic_pause_counter_bytes=8,
                    restart='fresh-process result restore then shared native restart; entire new race compared'),
                native_inputs='validated static pack and live-compatible controller stream; no original dynamic initialization')
    out.parent.mkdir(parents=True,exist_ok=True);out.write_text(json.dumps(result,indent=2)+'\n')
    with (out.parent/'validation-ledger.jsonl').open('a') as stream:stream.write(json.dumps(dict(result,report=str(out),report_sha256=sha(out.read_bytes())))+'\n')
    return result


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('command',choices=['freeze','compare'])
    for flag in ['reference','repeat','out']:p.add_argument('--'+flag,type=Path,required=True)
    for flag in ['contract','binary','pack']:p.add_argument('--'+flag,type=Path)
    a=p.parse_args()
    r=freeze(a.reference,a.repeat,a.out) if a.command=='freeze' else compare(a.reference,a.repeat,a.contract,a.binary,a.pack,a.out)
    print(json.dumps({k:v for k,v in r.items() if k!='restore_frames'},indent=2));print('restores',len(r.get('restore_frames',[])))
if __name__=='__main__':main()
