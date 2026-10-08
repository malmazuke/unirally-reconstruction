"""M4-12 private differential laboratory; original execution is reference-only.

The native subprocess receives copies of a seed, authenticated static content,
and controller rows in a fresh directory. Reference rows never enter that process.
"""
from __future__ import annotations
import argparse
import json
import shutil
import subprocess
import tempfile
from pathlib import Path
from .zoom_zoo_trial_reference import ROOT, ROM_SHA, PRIMARY_SHA, sha, digest, project, guards
from ..reference.core_identity import accepted_reference, require_core

BUTTONS=('b','y','select','start','up','down','left','right','a','x','l','r')

def contract():
    return json.loads((ROOT/'tests/manifests/native/zoom-zoo-trial-primary.reference.json').read_text())

def load_case(path):
    if path is None:return {'id':'primary','changes':[]}
    case=json.loads(path.read_text())
    if set(case)!={'id','changes'} or not isinstance(case['id'],str):raise ValueError('invalid case identity')
    seen=set()
    for change in case['changes']:
        if set(change)!={'from','to','buttons'}:raise ValueError('invalid controller change')
        first,last=change['from'],change['to']
        if type(first)!=int or type(last)!=int or not 1650<=first<=last<=1849:raise ValueError('case outside frozen horizon')
        if not isinstance(change['buttons'],list) or any(b not in BUTTONS for b in change['buttons']):raise ValueError('invalid button')
        frames=set(range(first,last+1))
        if seen&frames:raise ValueError('overlapping case changes')
        seen|=frames
    return case

def frame_buttons(case,frame):
    for c in case['changes']:
        if c['from']<=frame<=c['to']:return set(c['buttons'])
    return {'right'}

def controller_rows(case,first=1650):
    return ''.join(f"{f} {sum(1<<BUTTONS.index(b) for b in frame_buttons(case,f))} 0\n" for f in range(first,1850))

def capture(case,out,library,extra_guards=None):
    # Called in its own Python process, never by the native executable.
    from ..reference.bsnes import BsnesCore
    from ..reference.worker import inputs_for_frame
    from ..replay.manifest import derive_script
    if out.exists():raise ValueError('refusing to overwrite reference')
    raw=(ROOT/'tests/manifests/replay/race-crawler-zoom-zoo-3300.json').read_bytes()
    rom=Path((ROOT/'local/rom-location.txt').read_text().strip())
    core_sha=require_core(library)
    if sha(raw)!=PRIMARY_SHA or sha(rom.read_bytes())!=ROM_SHA:raise ValueError('reference identity differs')
    script=derive_script(json.loads(raw));rows=[];guard_rows=[];whole=[]
    out.parent.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='zz-reference-',dir=out.parent) as directory:
        core=BsnesCore(library,Path(directory),script.get('core_options'))
        try:
            core.load(rom);core.set_serialization_method('Strict')
            for frame in range(1850):
                inputs=inputs_for_frame(script,frame) if frame<1650 else {0:frame_buttons(case,frame),1:set()}
                for port,buttons in inputs.items():core.set_inputs(port,buttons)
                core.run_frame()
                if frame<1649:continue
                wram=core.wram();sram=core.cartridge_ram()
                for address,value in (extra_guards or {}).items():
                    if int.from_bytes(wram[address:address+2],"little")!=value:
                        raise ValueError(f"case changed supplemental guard {address:04x} at {frame}")
                extra=json.loads((ROOT/'tests/manifests/native/zoom-zoo-trial-mode-guards.reference.json').read_text())['guards']
                if any(int.from_bytes(wram[int(a,16):int(a,16)+2],'little')!=v for a,v in extra.items()):raise ValueError(f'case changed additive mode guard at {frame}')
                if wram[0x150b]!=101:raise ValueError('case writes retained opponent OAM byte')
                rows.append(project(wram,sram,frame).hex());whole.append(sha(wram))
                guard_rows.append(guards(wram,sram))
        finally:core.unload()
    expected=contract()
    if sha(bytes.fromhex(rows[0])[:394])!=expected['seed_sha256']:raise ValueError('case seed differs')
    if any(g!=expected['guards'] for g in guard_rows):raise ValueError('case changed excluded mode guard')
    result={'kind':'m4_12_case_reference','case':case,'case_sha256':digest(case),'rom_sha256':ROM_SHA,'core_sha256':core_sha,'rows':rows,'rows_sha256':digest(rows),'wram_sha256':whole,'guards':guard_rows[0]}
    if extra_guards is not None:result['extra_guards']={f'{a:04x}':v for a,v in extra_guards.items()}
    out.write_text(json.dumps(result,indent=2)+'\n')
    return {'frames':201,'rows_sha256':result['rows_sha256']}

def content_inventory():
    inventory=contract()['static_content'].copy()
    extra=json.loads((ROOT/'tests/manifests/native/zoom-zoo-trial-landing-content.reference.json').read_text())['item']
    inventory[extra['name']]={'bytes':extra['bytes'],'sha256':extra['sha256']}
    return inventory

def check_content(directory):
    for name,identity in content_inventory().items():
        data=(directory/name).read_bytes()
        if len(data)!=identity['bytes'] or sha(data)!=identity['sha256']:raise ValueError(f'static content differs: {name}')

def execute(binary,seed,content,inputs):
    result=subprocess.run([str(binary),'--seed',str(seed),'--content-dir',str(content),'--inputs',str(inputs)],capture_output=True,text=True,timeout=30,cwd=seed.parent)
    if result.returncode:raise ValueError(f'native exit {result.returncode}: {result.stderr}')
    rows=[]
    for line in result.stdout.splitlines():
        frame,row=line.split();data=bytes.fromhex(row)
        if len(data)!=395 or int.from_bytes(data[8:12],'little')!=int(frame):raise ValueError('native output framing differs')
        rows.append(row)
    return rows

def compare(reference,repeat,binary,content,out):
    a=json.loads(reference.read_text());b=json.loads(repeat.read_text())
    if a!=b:raise ValueError('fresh reference processes differ')
    case=a['case'];rows=a['rows']
    if a['case_sha256']!=digest(case) or a['rows_sha256']!=digest(rows) or len(rows)!=201:raise ValueError('reference integrity differs')
    if a['rom_sha256']!=ROM_SHA or not accepted_reference(a['core_sha256']):raise ValueError('reference identity differs')
    if sha(bytes.fromhex(rows[0])[:394])!=contract()['seed_sha256']:raise ValueError('reference seed differs')
    if case=={'id':'primary','changes':[]}:
        expected=json.loads((ROOT/'tests/manifests/native/zoom-zoo-trial-retained-oam.reference.json').read_text())['state_sha256']
        if [sha(bytes.fromhex(r)) for r in rows]!=expected:raise ValueError('frozen primary differs')
    check_content(content);binary=binary.resolve()
    with tempfile.TemporaryDirectory(prefix='zz-native-') as directory:
        root=Path(directory);private_content=root/'content';private_content.mkdir()
        for name in content_inventory():shutil.copyfile(content/name,private_content/name)
        seed=root/'seed.bin';seed.write_bytes(bytes.fromhex(rows[0]));inputs=root/'inputs.txt';inputs.write_text(controller_rows(case))
        native=execute(binary,seed,private_content,inputs)
        if native!=rows:
            for i,(actual,expected) in enumerate(zip(native,rows)):
                if actual!=expected:raise ValueError(f'first native divergence at {1649+i}')
            raise ValueError('native frame count differs')
        restores=[]
        for frame in (1700,1804,1823):
            seed.write_bytes(bytes.fromhex(native[frame-1649]));inputs.write_text(controller_rows(case,frame+1))
            restored=execute(binary,seed,private_content,inputs)
            if restored!=rows[frame-1649:]:raise ValueError(f'restore differs at {frame}')
            restores.append(frame)
    report={'status':'passed','case':case,'updates':200,'state_bytes':395,'rows_sha256':digest(rows),'native_binary_sha256':sha(binary.read_bytes()),'fresh_process_restores':restores,'native_input_files':'seed, authenticated static files, controllers only'}
    out.parent.mkdir(parents=True,exist_ok=True);out.write_text(json.dumps(report,indent=2)+'\n');return report

def main():
    parser=argparse.ArgumentParser(description=__doc__);sub=parser.add_subparsers(dest='command',required=True)
    c=sub.add_parser('capture');c.add_argument('--case',type=Path);c.add_argument('--core',type=Path,required=True);c.add_argument('--out',type=Path,required=True)
    c=sub.add_parser('compare');c.add_argument('--reference',type=Path,required=True);c.add_argument('--repeat',type=Path,required=True);c.add_argument('--binary',type=Path,required=True);c.add_argument('--content-dir',type=Path,required=True);c.add_argument('--out',type=Path,required=True)
    args=parser.parse_args()
    try:
        result=capture(load_case(args.case),args.out,args.core) if args.command=='capture' else compare(args.reference,args.repeat,args.binary,args.content_dir,args.out)
        print(json.dumps(result,indent=2))
    except (ValueError,OSError,subprocess.TimeoutExpired) as error:parser.exit(1,f'{error}\n')
if __name__=='__main__':main()
