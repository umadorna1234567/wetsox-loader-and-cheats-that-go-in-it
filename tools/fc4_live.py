"""Bounded read-only snapshots of FC4 objects for binding verification."""
import sys,struct,json
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'far cry 5 mod menu/tools'))
from inspect_runtime import Reader
from capstone import Cs,CS_ARCH_X86,CS_MODE_64
d=Cs(CS_ARCH_X86,CS_MODE_64);d.detail=True
def type_name(vtable):
    code=q(vtable+8)
    for _ in range(2):
        first=next(d.disasm(r.read(code,16),code),None)
        if first and first.mnemonic=='jmp':code=int(first.op_str,16)
    for ins in d.disasm(r.read(code,160),code):
        if ins.mnemonic=='lea' and 'rip' in ins.op_str:
            ptr=ins.address+ins.size+ins.operands[1].mem.disp
            try:
                text=r.read(ptr,96).split(b'\0')[0].decode('ascii')
                if len(text)>2 and text.startswith(('C','I','?')):return text
            except (OSError,UnicodeDecodeError):pass
    return '?'
r=Reader(int(sys.argv[1]),'FC64.dll')
def q(p):return struct.unpack('<Q',r.read(p,8))[0] if p else 0
def dump(p,n=0x100):
    raw=r.read(p,n)
    return {hex(i):hex(struct.unpack_from('<Q',raw,i)[0]) for i in range(0,len(raw),8)}
try:
    manager=q(r.base+0x2e24c58);controller=q(q(manager+8))
    ref=q(q(controller+8)+0x18);entity=q(ref+16)
    print(json.dumps({'base':hex(r.base),'manager':hex(manager),'controller':hex(controller),'ref':hex(ref),'id':hex(q(ref)),'entity':hex(entity),'entityFields':dump(entity,0x180),'idGetterRva':hex(q(q(controller)+0x18)-r.base)},indent=2))
    for arg in sys.argv[2:]:print(arg,json.dumps(dump(int(arg,0)),indent=2))
    components=q(entity+0x98);count=q(entity+0xa0)&0xffffffff
    component_list=[]
    if count<256:
        for i in range(count):
            c=q(components+8*i)
            if c:
                name=type_name(q(c));component_list.append((name,c))
                print('COMPONENT',i,hex(c),'vt',hex(q(c)-r.base),name)
    folder=Path(__file__).resolve().parents[1]/'build-Release/fc4-research'
    metadata={'base':r.base,'entity':entity,'components':dict(component_list)}
    for name,c in component_list:
        if name in ('CPawn','CGraphicComponent','CPawnAgent','CFCXCountersComponentPlayerMP','CFCXPawn'):
            (folder/(name+'.bin')).write_bytes(r.read(c,0x3000))
        if name=='CPawn':
            metadata['pawnState']=q(c+0x70)
            (folder/'pawn-state.bin').write_bytes(r.read(q(c+0x70),0x3000))
        if name=='CGraphicComponent':
            bones=q(c+0xb0);boneCount=q(c+0xb8)&0xffffffff
            if 0<boneCount<2048:
                buf=r.read(bones,boneCount*0x70);(folder/'bones.bin').write_bytes(buf)
                print('BONES',boneCount,[(hex(struct.unpack_from('<I',buf,i*0x70)[0]),struct.unpack_from('<3f',buf,i*0x70+0x60)) for i in range(min(8,boneCount))])
    table=q(r.base+0x2dd48f8);count=q(table+8)&0xffffffff;buckets=q(table+16)
    seen=set();humans=[]
    if count<65536:
        for n in range(count):
            node=q(buckets+n*8)
            while node and node not in seen and len(seen)<50000:
                seen.add(node);obj=q(q(node+16)+16)
                if obj and q(obj)==r.base+0x280d7f0:
                    humans.append({'id':hex(q(obj+8)),'object':hex(obj),'position':struct.unpack('<3f',r.read(obj+0x50,12))})
                node=q(node)
    print('ENTITY_TABLE',hex(table),'buckets',count,'references',len(seen),'humans',humans[:12])
    metadata['humans']=humans
    for h in humans[:12]:
        obj=int(h['object'],16);items=[]
        for i in range(min(q(obj+0xa0)&0xffffffff,256)):
            c=q(q(obj+0x98)+8*i)
            if not c:continue
            name=type_name(q(c));items.append((name,hex(q(c)-r.base)))
            if name in ('CPawnAgent','CFCXPawn','CPawn'):
                (folder/(h['id']+'-'+name+'.bin')).write_bytes(r.read(c,0x1000))
        print('HUMAN_COMPONENTS',h['id'],items)
    state=metadata.get('pawnState',0)
    if state:
        weaponEntity=q(q(state+0x1258)+16)
        if weaponEntity:
            for i in range(min(q(weaponEntity+0xa0)&0xffffffff,256)):
                c=q(q(weaponEntity+0x98)+8*i)
                if not c:continue
                name=type_name(q(c));print('WEAPON_COMPONENT',name,hex(c),hex(q(c)-r.base))
                if name in ('CWeapon','CFCXWeapon'):
                    metadata['weapon']=c;metadata['weaponState']=q(c+0xa0)
                    (folder/'CWeapon.bin').write_bytes(r.read(c,0x1000))
                    (folder/'weapon-state.bin').write_bytes(r.read(q(c+0xa0),0x1000))
    (folder/'live.json').write_text(json.dumps(metadata,indent=2))
finally:r.close()

