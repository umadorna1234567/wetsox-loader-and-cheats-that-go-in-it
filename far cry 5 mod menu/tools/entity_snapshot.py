"""Read-only entity-registry snapshot for the fingerprinted FC5 build."""
import argparse
from collections import Counter
import hashlib
import json
import math
from pathlib import Path
import struct
from inspect_runtime import Reader, ROOT
from list_lua_api import EXPECTED, u64, u32
from capstone import Cs, CS_ARCH_X86, CS_MODE_64
from capstone.x86 import X86_OP_MEM, X86_REG_RIP

def main():
    p=argparse.ArgumentParser();p.add_argument('--pid',type=int,required=True);args=p.parse_args()
    r=Reader(args.pid)
    try:
        if hashlib.sha256(Path(r.path).read_bytes()).hexdigest().upper()!=EXPECTED: raise RuntimeError('Unverified build')
        def q(address):return u64(r.read(address,8))
        manager=q(r.base+0x4fb3110);controller=q(q(manager+8));reference=q(q(controller+8)+24)
        local_id=q(reference);local_entity=q(reference+16)
        local_pos=struct.unpack('<3f',r.read(local_entity+0x60,12))
        registry=r.read(r.base+0x4eb41c8,48);count=q(r.base+0x4eb41c8+32);buckets=q(r.base+0x4eb41c8+40)
        if not 0<count<=65536:raise RuntimeError('Invalid registry bucket count')
        heads=r.read(buckets,count*8);seen=set();types={};entities=[];unknown=[];errors=0
        dis=Cs(CS_ARCH_X86,CS_MODE_64);dis.detail=True
        def typename(entity):
            vtable=q(entity)
            if vtable in types:return types[vtable]
            fn=q(vtable+8)
            if not r.base<=fn<r.base+r.size:return None
            for _ in range(4):
                code=r.read(fn,5)
                if code[0]!=0xe9:break
                fn=fn+5+struct.unpack_from('<i',code,1)[0]
            for ins in dis.disasm(r.read(fn,128),fn):
                if ins.mnemonic=='cmp' and ins.operands[0].type==X86_OP_MEM and ins.operands[0].mem.base==X86_REG_RIP:
                    desc=ins.address+ins.size+ins.operands[0].mem.disp
                    header=r.read(desc,48);name_address=u64(header);depth=u32(header,8)
                    if not r.base<=name_address<r.base+r.size or not 1<=depth<=8:break
                    name=r.read(name_address,128).split(b'\0')[0].decode('ascii')
                    if not name or not all(c.isalnum() or c in '_:<> ' for c in name):break
                    types[vtable]={'name':name,'descriptor_rva':hex(desc-r.base),'hierarchy':list(struct.unpack_from('<'+'I'*depth,header,16))}
                    return types[vtable]
            types[vtable]=None
            return None
        for index in range(count):
            node=u64(heads,index*8)
            while node and node not in seen:
                if len(seen)>50000:raise RuntimeError('Registry traversal exceeded limit')
                seen.add(node)
                try:
                    entry=r.read(node,24);node=u64(entry);entity_id=u64(entry,8);ref=u64(entry,16)
                    entity=q(ref+16)
                    if not entity:continue
                    info=typename(entity)
                    if not info:
                        candidate={'id':hex(entity_id),'object':hex(entity),'vtable':hex(q(entity))}
                        try:
                            xyz=struct.unpack('<3f',r.read(entity+0x60,12))
                            if all(math.isfinite(x) and abs(x)<1e7 for x in xyz):
                                candidate['unverified_position']=xyz
                                candidate['unverified_distance']=math.dist(xyz,local_pos)
                        except OSError:pass
                        unknown.append(candidate)
                        continue
                    result={'id':hex(entity_id),'object':hex(entity),'type':info['name']}
                    if info['name']=='CPawnEntity':
                        descriptor=q(entity+0xc8)
                        if descriptor:
                            slot=struct.unpack('<i',r.read(descriptor+0x20,4))[0]
                            if 0<=slot<256:
                                component=q(q(entity+0xa8)+slot*8)
                                subtype=typename(component) if component else None
                                if subtype:result['pawn_type']=subtype['name']
                    if 0x50c95067 in info['hierarchy']:
                        descriptor=q(entity+0xc8)
                        if not descriptor:continue
                        # Native IsAnAnimal resolves this component, then
                        # calls vtable+0x130 and the result's vtable+0x20.
                        animal_slot=struct.unpack('<i',r.read(descriptor+0x44,4))[0]
                        if 0<=animal_slot<256:
                            animal_component=q(q(entity+0xa8)+animal_slot*8)
                            if animal_component:
                                result['animal_query_component']=hex(animal_component)
                                result['animal_query_type']=typename(animal_component)
                                result['animal_query_getter_rva']=hex(q(q(animal_component)+0x130)-r.base)
                                if result['animal_query_getter_rva']=='0x269dba0':
                                    result['animal_classifier_rva']=hex(q(q(animal_component+0x20)+0x20)-r.base)
                    # 0x50c95067 is the reflected entity-base ID verified in the
                    # native position reader's type check, not a name guess.
                    if 0x50c95067 in info['hierarchy']:
                        xyz=struct.unpack('<3f',r.read(entity+0x60,12))
                        if all(math.isfinite(x) and abs(x)<1e7 for x in xyz):
                            result['position']=xyz;result['distance']=math.dist(xyz,local_pos)
                            if result['distance']<5:
                                component_count=(q(entity+0xb0)>>32)&0x7fffffff
                                result['nearby_components']=[]
                                if component_count<=256:
                                    for slot in range(component_count):
                                        component=q(q(entity+0xa8)+slot*8)
                                        if not component:continue
                                        detail=typename(component)
                                        if detail:result['nearby_components'].append({'slot':slot,'pointer':hex(component),'type':detail['name']})
                    entities.append(result)
                except (OSError,UnicodeError,ValueError):errors+=1;break
        report={'module_sha256':EXPECTED,'local_id':hex(local_id),'local_object':hex(local_entity),
                'local_position':local_pos,'visited_nodes':len(seen),'read_errors':errors,
                'types':{hex(k):v for k,v in types.items()},'entities':entities,'unknown':unknown}
        (ROOT/'research/entity-snapshot.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
        print(json.dumps({k:report[k] for k in ['local_id','local_position','visited_nodes','read_errors']}))
        print('Types:',Counter(e['type'] for e in entities))
        print('Nearest:',json.dumps(sorted([e for e in entities if 'distance' in e],key=lambda e:e['distance'])[:12]))
    finally:r.close()
if __name__=='__main__':main()
