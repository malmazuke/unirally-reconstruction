"""Reference-only M4-12 expanded contract capture; never a native runtime input.

Capture actual WRAM and cartridge RAM in fresh original processes. The existing
semantic movement projection supplies byte layout only, with no motion equations.
Every frame is independently checked against the frozen primary whole-WRAM hash.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import struct
import tempfile
from .prepare import canonical_seed
from ..reference.bsnes import BsnesCore
from ..reference.worker import inputs_for_frame
from ..replay.manifest import derive_script
from ..reference.core_identity import accepted_reference, require_core

ROOT=Path(__file__).resolve().parents[3]
ROM_SHA='a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e'
# The library every frozen reference was captured with: provenance, not the running core's identity
# (core_identity accepts any library built from the lock's source; PORTABLE-CORE-IDENTITY).
CORE_SHA='e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b'
PRIMARY_SHA='acd29bfb72aeaad0791923e22e17a791f5c182220f64b6686dd687984411aefd'
# Additional future-affecting state found by the dispatcher read/writer audit.
# Names remain provisional where evidence establishes only a control role.
RIDER_EXTRA=[
 ('reflection_step',0xe03),('reflection_end',0xe0b),('reflection_pose_base',0xe07),
 ('pose_override',0xdef),('reflection_completed',0xbcf),('reflection_hold',0xbd3),
 ('drive_pose_enabled',0xe73),('reflection_air_turns',0xd2d),
 ('direction_latch',0x11f9),('base_velocity_cap',0x11d3),
 ('brake_input',0x325),('rotate_negative_input',0x329),
 ('rotate_positive_input',0x32d),('jump_input',0x331),
 ('wrong_direction_counter',0xe57),
]
# These loads guard excluded modes in the reached routines. Authenticate them
# from the initial observation and require preservation across the reference.
RIDER_GUARDS=[0x42b,0x337,0x547,0xb93,0xb97,0xd3d,0xbe7,0xbcb,0xd57,0xdf7,0xbab,0xe6f,
              0x124f,0x1009,0x1221,0xfdf,0x1215,0x121b,0x123f,0x321,0xfd1]
GLOBAL_GUARDS=[(0x31d,2),(0x31f,2),(0x1225,2),(0x1227,2),(0xd4f,2),(0xd51,2)]

def sha(data:bytes)->str:return hashlib.sha256(data).hexdigest()
def digest(value)->str:return sha(json.dumps(value,sort_keys=True,separators=(',',':')).encode())
def project(wram:bytes,sram:bytes,frame:int)->bytes:
    result=bytearray(canonical_seed(wram,sram))
    result[:8]=b'URZZ0001';result[8:12]=struct.pack('<I',frame)
    for rider in (0,1):
        rejection=int.from_bytes(wram[0xfd1+2*rider:0xfd3+2*rider],'little')
        if rejection not in (0,1):raise ValueError('nonbinary original transition rejection')
        result[63+128*rider]=rejection
    for rider in (0,1):
        for _name,address in RIDER_EXTRA:result+=wram[address+2*rider:address+2*rider+2]
    result+=wram[0x31b:0x31c]
    result+=wram[0x150b:0x150c]
    return bytes(result)
def guards(wram:bytes,sram:bytes)->dict:
    result={f'rider_{r}_{a:04x}':int.from_bytes(wram[a+2*r:a+2*r+2],'little') for r in (0,1) for a in RIDER_GUARDS}
    result.update({f'global_{a:04x}':int.from_bytes(wram[a:a+w],'little') for a,w in GLOBAL_GUARDS})
    result.update({f'cartridge_{a:04x}':int.from_bytes(sram[a:a+w],'little') for a,w in [(0x748,2),(0x74a,1),(0x74b,1),(0x750,2)]})
    return result

def capture(access_dir:Path,out:Path)->dict:
    if out.exists():raise ValueError('refusing to overwrite a reference capture')
    samples=json.loads((access_dir/'samples.json').read_text())
    manifest_path=ROOT/'tests/manifests/replay/race-crawler-zoom-zoo-3300.json'
    raw=manifest_path.read_bytes();manifest=json.loads(raw)
    if sha(raw)!=PRIMARY_SHA:raise ValueError('primary replay identity differs')
    if samples['sample_digest']!=manifest['expected']['sample_digest']:raise ValueError('sample identity differs')
    library=Path(samples['core']['library']);rom=Path((ROOT/'local/rom-location.txt').read_text().strip())
    core_sha=require_core(library)
    if sha(rom.read_bytes())!=ROM_SHA:raise ValueError('ROM/core identity differs')
    script=derive_script(manifest);rows=[];observed_guards=[]
    out.parent.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='m4-12-reference-',dir=out.parent) as directory:
        core=BsnesCore(library,Path(directory),script.get('core_options'))
        try:
            core.load(rom);core.set_serialization_method('Strict')
            for frame in range(1850):
                for port,buttons in inputs_for_frame(script,frame).items():core.set_inputs(port,buttons)
                core.run_frame()
                if frame<1649:continue
                wram=core.wram();sram=core.cartridge_ram()
                if sha(wram)!=samples['frames'][frame]['wram_sha256']:raise ValueError(f'whole WRAM differs at {frame}')
                rows.append(project(wram,sram,frame).hex());observed_guards.append(guards(wram,sram))
        finally:core.unload()
    changed={k:sorted({g[k] for g in observed_guards}) for k in observed_guards[0] if len({g[k] for g in observed_guards})>1}
    result={'schema_version':1,'kind':'m4_12_reference_projection','frames':[1649,1849],
            'rom_sha256':ROM_SHA,'core_sha256':core_sha,'manifest_sha256':PRIMARY_SHA,
            'layout':{'movement_prefix_bytes':333,'movement_layout_source':'src/core/movement.cpp:write_rider/serialize_movement_state',
                      'rider_extra':RIDER_EXTRA,'transition_rejection_addresses':[0xfd1,0xfd3],'opponent_horizontal_address':0x31b},
            'guards':observed_guards[0],'changed_guards':changed,'rows_sha256':digest(rows),'rows':rows}
    out.write_text(json.dumps(result,indent=2)+'\n');return result

def expanded_access_command(out:Path)->list[str]:
    from .zoom_zoo_vertical_velocity import capture_command
    command=capture_command(ROOT/'tests/manifests/replay/race-crawler-zoom-zoo-3300.json',out)
    command[command.index('--to-frame')+1]='1849'
    command[command.index('--timeout')+1]='600'
    existing={int(command[i+1],0) for i,v in enumerate(command[:-1]) if v=='--watch-address'}
    for address in list(range(0x300,0x1370,2))+list(range(0x211e,0x212a,2)):
        if address not in existing:command+=['--watch-address',hex(address)]
    return command

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--access-dir',type=Path,required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args()
    r=capture(a.access_dir,a.out);print(json.dumps({k:v for k,v in r.items() if k not in ['rows','guards','layout']},indent=2))
if __name__=='__main__':main()
