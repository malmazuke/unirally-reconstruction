"""Original-only M4-15 strategy exploration; outputs are not native acceptance.

Retains the authenticated cold-start prefix. Allows every controller-0 button
for research, leaving the original opponent policy and controller 1 untouched.
Records raw WRAM observations rather than extending an accepted state contract.
"""
from __future__ import annotations
import argparse
import json
from pathlib import Path
import tempfile
from .zoom_zoo_trial_reference import ROOT, ROM_SHA, PRIMARY_SHA, sha, digest
from ..reference.bsnes import BsnesCore, BUTTONS
from ..reference.worker import inputs_for_frame
from ..replay.manifest import derive_script
from ..reference.core_identity import accepted_reference, require_core

ADDRESSES = [0x415,0x419,0xbbb,0xbb3,0x54b,0xb93,0xb97,0xfc5,0xfc9,0xfcd,
             0xfd1,0xeff,0xf0f,0xe57,0x11d9,0x11df,0xd39,0x4c3]

def timeline(case, horizon, script):
    result = []
    changes = {}
    for change in case['changes']:
        first,last,buttons = change['from'],change['to'],change['buttons']
        if type(first) is not int or type(last) is not int or not 1650 <= first <= last <= horizon:
            raise ValueError('change outside continuation')
        if len(set(buttons)) != len(buttons) or any(b not in BUTTONS for b in buttons):
            raise ValueError('invalid buttons')
        for f in range(first,last+1):
            if f in changes: raise ValueError('overlapping changes')
            changes[f] = buttons
    for f in range(horizon+1):
        inputs=inputs_for_frame(script,f) if f<1650 else {0:changes.get(f,['right']),1:[]}
        result.append([sorted(inputs.get(p,[])) for p in (0,1)])
    return result

def capture(out, library, case, horizon, keep_wram=False, policy=None):
    if out.exists() or out.with_suffix('.wram').exists(): raise ValueError('fresh output required')
    if not 3299 <= horizon <= 100000: raise ValueError('exploration horizon 3299..100000')
    raw=(ROOT/'tests/manifests/replay/race-crawler-zoom-zoo-3300.json').read_bytes()
    rom=Path((ROOT/'local/rom-location.txt').read_text().strip())
    core_sha=require_core(library)
    if (sha(raw),sha(rom.read_bytes())) != (PRIMARY_SHA,ROM_SHA):
        raise ValueError('reference identity differs')
    script=derive_script(json.loads(raw)); inputs=timeline(case,horizon,script)
    inventory=json.loads((ROOT/'tests/manifests/native/zoom-zoo-sustained-guards.reference.json').read_text())['items']
    changed={}; rows=[]; hashes=[]; seed=None
    out.parent.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(dir=out.parent) as directory:
        core=BsnesCore(library,Path(directory),script.get('core_options'))
        series=out.with_suffix('.wram').open('xb') if keep_wram else None
        cartridge=out.with_suffix('.sram').open('xb') if keep_wram else None
        try:
            core.load(rom);core.set_serialization_method('Strict')
            for f,ports in enumerate(inputs):
                if policy and f>=1650:
                    prior=core.wram(); marker=int.from_bytes(prior[0xfc5:0xfc7],"little")
                    ports[0]=["left" if marker&0x4000 else "right"]
                    if policy=="marker-jump" and marker&0x2000:ports[0].append("b")
                for p,buttons in enumerate(ports):core.set_inputs(p,set(buttons))
                core.run_frame()
                if f<1649:continue
                w=core.wram()
                if f==1649:
                    from .zoom_zoo_trial_reference import project
                    from .zoom_zoo_trial import contract
                    if sha(project(w,core.cartridge_ram(),f)[:394])!=contract()['seed_sha256']:
                        raise ValueError('authentic seed differs')
                    seed=sha(w)
                if series:series.write(w)
                if cartridge:cartridge.write(core.cartridge_ram())
                hashes.append(sha(w))
                rows.append([[int.from_bytes(w[a+r*2:a+r*2+2],'little') for a in ADDRESSES] for r in (0,1)])
                for item in inventory:
                    a,n=item['address'],item['width'];v=int.from_bytes(w[a:a+n],'little')
                    if v!=item['value'] and str(a) not in changed:changed[str(a)]={'frame':f,'value':v,'previous_constant':item['value']}
        finally:
            if series:series.close()
            if cartridge:cartridge.close()
            core.unload()
    if policy:
        changes=[]
        for f in range(1650,horizon+1):
            buttons=inputs[f][0]
            if changes and changes[-1]['buttons']==buttons:changes[-1]['to']=f
            else:changes.append({'from':f,'to':f,'buttons':buttons})
        case={'id':policy,'changes':changes}
        out.with_suffix('.case.json').write_text(json.dumps(case,indent=2)+'\n')
    result={'kind':'m4_15_original_exploration','case':case,'frames':[1649,horizon],
            'rom_sha256':ROM_SHA,'core_sha256':core_sha,'manifest_sha256':PRIMARY_SHA,
            'timeline_sha256':digest(inputs),'seed_wram_sha256':seed,'addresses':ADDRESSES,
            'rows':rows,'wram_sha256':hashes,'new_constant_transitions':changed}
    out.write_text(json.dumps(result,separators=(',',':'))+'\n')
    return {'case':case['id'],'frames':result['frames'],'last':rows[-1],
            'first_finish':[next((1649+i for i,row in enumerate(rows) if row[r][11]),None) for r in (0,1)],
            'ranges':[[[min(row[r][a] for row in rows),max(row[r][a] for row in rows)] for a in (0,1,7,8,9)] for r in (0,1)],
            'new_constant_transitions':changed}

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--policy',choices=['marker','marker-jump'])
    p.add_argument('--core',type=Path,required=True);p.add_argument('--out',type=Path,required=True)
    p.add_argument('--case',type=Path);p.add_argument('--horizon',type=int,default=11999);p.add_argument('--keep-wram',action='store_true')
    a=p.parse_args();case=json.loads(a.case.read_text()) if a.case else {'id':'continuous-right','changes':[]}
    print(json.dumps(capture(a.out,a.core.resolve(),case,a.horizon,a.keep_wram,a.policy),indent=2))
if __name__=='__main__':main()
