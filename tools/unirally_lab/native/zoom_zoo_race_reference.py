"""Additive M4-15 reference state inventory, frozen before native tuning.

The original lap/checkpoint routine $818050-$8182B6 owns the appended rider
words and lap-time slots; $83E81D owns the finish delay. $1225/$1227 remain
provisional pending the continuous read/writer audit. No native equations here.
"""
from __future__ import annotations
import argparse
import json
from pathlib import Path
from .zoom_zoo_trial_reference import sha,digest,ROOT,ROM_SHA,PRIMARY_SHA
from ..reference.core_identity import accepted_reference
from .zoom_zoo_sustained import sustained_project

RIDER_WORDS=[('laps_remaining',0xefb),('checkpoint',0x119f),('next_checkpoint',0x11a3),
             ('start_line_latch',0x11a7),('checkpoint_display_countdown',0xfff),('finished',0xeff),
             ('time_minutes',0xe43),('time_tens_seconds',0xe47),('time_seconds',0xe4b),
             ('time_tenths',0xe4f),('time_hundredths',0xe3f)]
GLOBAL_WORDS=[('provisional_1225',0x1225),('provisional_1227',0x1227),('finish_delay',0xf0f)]
CAMERA_WORDS=[('x',0x41d),('y',0x421),('velocity_x',0x4f9),('velocity_y',0x4fd),('lookahead',0x553),('screen_xy',0x1513)]
FINISH_POSE_WORDS=[('selector',0x11e5),('kind',0x11e9),('locked',0x11ed),('active',0x30d)]
STATE_BYTES=565

def project(wram,sram,frame):
    result=bytearray(sustained_project(wram,sram,frame));result[7]=ord('3')
    for rider in (0,1):
        for _,a in RIDER_WORDS:result+=wram[a+2*rider:a+2*rider+2]
    for base in (0x755,0x7bf):result+=sram[base:base+20]
    for a in (0x769,0x7d3):result+=sram[a:a+2]
    for _,a in GLOBAL_WORDS:result+=wram[a:a+2]
    for _,a in CAMERA_WORDS:result+=wram[a:a+2]
    result+=wram[0x114d:0x1161]
    for rider in (0,1):
        for _,a in FINISH_POSE_WORDS:result+=wram[a+2*rider:a+2*rider+2]
    assert len(result)==STATE_BYTES
    return bytes(result)

def rows(path):
    reference=json.loads(path.read_text());result=[]
    if (reference['rom_sha256'],reference['manifest_sha256'])!=(ROM_SHA,PRIMARY_SHA) or not accepted_reference(reference['core_sha256']):raise ValueError('reference identity differs')
    guards=json.loads((ROOT/'tests/manifests/native/zoom-zoo-race-guards.reference.json').read_text())['items']
    if reference['frames'][0]!=1649 or not 3299<=reference['frames'][1]<=9999:raise ValueError('race reference horizon invalid')
    if len(reference['wram_sha256'])!=reference['frames'][1]-1648:raise ValueError('reference frame inventory incomplete')
    previous=None
    with path.with_suffix('.wram').open('rb') as ws,path.with_suffix('.sram').open('rb') as ss:
        for i,f in enumerate(range(reference['frames'][0],reference['frames'][1]+1)):
            w,s=ws.read(0x20000),ss.read(0x2000)
            if len(w)!=0x20000 or len(s)!=0x2000 or sha(w)!=reference['wram_sha256'][i]:raise ValueError('raw WRAM differs')
            for item in guards:
                a=item['address']
                if int.from_bytes(w[a:a+item['width']],'little')!=item['value']:raise ValueError(f'new constant transition at {f}, {a:04x}')
            if previous is not None and f%3!=0:
                for rider in (0,1):
                    word=lambda a:int.from_bytes(previous[a:a+2],'little')
                    if word(0xeff+2*rider) and not word(0x30d+2*rider):
                        read,write=(0xce7,0xce9) if rider==0 else (0xd11,0xd13)
                        if ((word(write)-word(read)-1)&31)!=0:raise ValueError(f'finish animation queue dependency reached at {f}, rider {rider}')
            previous=w
            row=project(w,s,f)
            if f==1649:
                from .zoom_zoo_trial import contract
                seed=bytearray(row[:394]);seed[7]=ord('1')
                if sha(seed)!=contract()['seed_sha256']:raise ValueError('authentic seed differs')
            result.append(row.hex())
        if ws.read(1) or ss.read(1):raise ValueError('extra raw memory')
    return reference,result

def freeze(a,b,out):
    if out.exists():raise ValueError('fresh freeze required')
    reference,left=rows(a);repeat,right=rows(b)
    if reference!=repeat or left!=right:raise ValueError('fresh references differ')
    finish=[next((1649+i for i,row in enumerate(reference['rows']) if row[r][11]),None) for r in (0,1)]
    if any(f is None for f in finish) or reference['frames'][1]-max(finish)<200:raise ValueError('both finishes and 200 subsequent updates required')
    result={k:reference[k] for k in ['case','frames','rom_sha256','core_sha256','manifest_sha256','timeline_sha256','seed_wram_sha256','wram_sha256']}
    result.update(kind='m4_15_race_freeze',state_bytes=STATE_BYTES,rider_words=RIDER_WORDS,global_words=GLOBAL_WORDS,
                  finish_pose_words=FINISH_POSE_WORDS,checkpoint_seen_range=[0x114d,20],camera_words=CAMERA_WORDS,cartridge_lap_slots=[0x755,0x7bf],cartridge_totals=[0x769,0x7d3],
                  rows_sha256=digest(left),state_sha256=[sha(bytes.fromhex(r)) for r in left],finish_frames=finish,
                  outcome='player_won' if finish[0]<finish[1] else 'player_lost',
                  horizon_rationale=(f"Complete three-lap race; {reference['frames'][1]-finish[0]} updates after player finish and "
                                    f"{reference['frames'][1]-finish[1]} after opponent finish. Result-screen loading/rendering follows this simulation domain."),
                  escape_criterion='Advance from authentic seed into distinct downstream sections, cross all ordered lap checkpoints and set both finish flags. Initial Left follows the initial marker direction; this leaves the old repeated Right section without a later seed.')
    out.write_text(json.dumps(result,indent=2)+'\n');return result

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--reference',type=Path,required=True);p.add_argument('--repeat',type=Path,required=True);p.add_argument('--out',type=Path,required=True)
    a=p.parse_args();r=freeze(a.reference,a.repeat,a.out);print(r['finish_frames'],r['rows_sha256'])
if __name__=='__main__':main()
