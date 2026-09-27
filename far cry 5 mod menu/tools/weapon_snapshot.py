"""Read-only local-owned weapon/module snapshot on the verified engine."""
import argparse,hashlib,json,struct
from pathlib import Path
from inspect_runtime import Reader,ROOT
from list_lua_api import EXPECTED

p=argparse.ArgumentParser();p.add_argument('--pid',required=True,type=int);a=p.parse_args()
r=Reader(a.pid)
try:
    if hashlib.sha256(Path(r.path).read_bytes()).hexdigest().upper()!=EXPECTED:raise RuntimeError('Unverified engine')
    def q(x):return struct.unpack('<Q',r.read(x,8))[0]
    def i(x):return struct.unpack('<i',r.read(x,4))[0]
    def floats(x,n):return struct.unpack('<'+'f'*n,r.read(x,4*n))
    manager=q(r.base+0x4fb3110);ref=q(q(q(q(manager+8))+8)+24);local=q(ref)
    count=q(r.base+0x4eb41e8);buckets=q(r.base+0x4eb41f0)
    if not 0<count<=65536:raise RuntimeError('Bad registry')
    seen=set();weapons=[]
    for node, in struct.iter_unpack('<Q',r.read(buckets,count*8)):
        while node and node not in seen and len(seen)<50000:
            seen.add(node)
            try:
                nextnode,identity,reference=struct.unpack('<QQQ',r.read(node,24));node=nextnode
                entity=q(reference+16);desc=q(entity+0xc8)
                slot=i(desc+0x10)
                if not 0<=slot<256:continue
                weapon=q(q(entity+0xa8)+slot*8)
                if q(q(weapon+0x70))!=local:continue
                fields=r.read(weapon,0x900);modules=[]
                pointers=list(struct.iter_unpack('<Q',fields))
                for offset,(ptr,) in enumerate(pointers):
                    if ptr<0x10000 or ptr>=r.base:continue
                    try:
                        if q(ptr+0x50)!=weapon:continue
                        module={'weapon_offset':hex(offset*8),'pointer':hex(ptr),'vtable_rva':hex(q(ptr)-r.base)}
                        config=q(ptr+0xc0)
                        try:module.update(config=hex(config),physics=r.read(config+0x510,1)[0],speed_gravity_drop=floats(config+0x514,3))
                        except OSError:pass
                        modules.append(module)
                    except OSError:pass
                weapons.append({'entity':hex(entity),'id':hex(identity),'weapon':hex(weapon),
                    'vtable_rva':hex(q(weapon)-r.base),'clip':i(weapon+0x188),
                    'runtime_multipliers':floats(weapon+0x7d4,8),'modules':modules})
            except OSError:break
    out={'local_id':hex(local),'weapons':weapons}
    (ROOT/'research/weapon-snapshot.json').write_text(json.dumps(out,indent=2));print(json.dumps(out,indent=2))
finally:r.close()
