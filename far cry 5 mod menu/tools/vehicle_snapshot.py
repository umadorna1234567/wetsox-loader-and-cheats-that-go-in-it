"""Read-only local vehicle/component snapshot for verified FC5."""
import argparse,hashlib,json,struct
from pathlib import Path
from inspect_runtime import Reader,ROOT
from list_lua_api import EXPECTED
from capstone import Cs,CS_ARCH_X86,CS_MODE_64
from capstone.x86 import X86_OP_MEM,X86_REG_RIP
p=argparse.ArgumentParser();p.add_argument('--pid',type=int,required=True);a=p.parse_args()
r=Reader(a.pid)
try:
    if hashlib.sha256(Path(r.path).read_bytes()).hexdigest().upper()!=EXPECTED:raise RuntimeError('Unverified engine')
    def q(x):return struct.unpack('<Q',r.read(x,8))[0]
    def component(e,field):
        desc=q(e+0xc8)
        slot=struct.unpack('<i',r.read(desc+field,4))[0]
        return q(q(e+0xa8)+slot*8) if 0<=slot<256 else 0
    manager=q(r.base+0x4fb3110);ref=q(q(q(q(manager+8))+8)+24);entity=q(ref+16)
    pawn=component(entity,0x20);count=(q(pawn+0xb0)>>32)&0x7fffffff
    if count>256:raise RuntimeError('Bad aspect count')
    ridable=0
    for n in range(count):
        key,ptr=struct.unpack('<QQ',r.read(q(pawn+0xa8)+n*16,16))
        if key&0xffffffff==0x3940e81d:ridable=ptr
    vehicle=q(q(ridable+8)+16) if ridable else 0
    out={'ridable':hex(ridable),'vehicle':hex(vehicle),'components':[]}
    if vehicle:
        out['id']=hex(q(vehicle+8));out['position']=struct.unpack('<3f',r.read(vehicle+0x60,12))
        count=(q(vehicle+0xb0)>>32)&0x7fffffff
        if count>256:raise RuntimeError('Bad component count')
        dis=Cs(CS_ARCH_X86,CS_MODE_64);dis.detail=True
        for n in range(count):
            obj=q(q(vehicle+0xa8)+n*8)
            if not obj:continue
            try:
                vt=q(obj);fn=q(vt+8);name=None
                for _ in range(4):
                    code=r.read(fn,5)
                    if code[0]!=0xe9:break
                    fn+=5+struct.unpack_from('<i',code,1)[0]
                for ins in dis.disasm(r.read(fn,128),fn):
                    if ins.mnemonic=='cmp' and ins.operands[0].type==X86_OP_MEM and ins.operands[0].mem.base==X86_REG_RIP:
                        desc=ins.address+ins.size+ins.operands[0].mem.disp
                        name=r.read(q(desc),96).split(b'\0')[0].decode('ascii',errors='replace');break
                out['components'].append({'slot':n,'object':hex(obj),'vtable_rva':hex(vt-r.base),'name':name})
            except OSError:pass
    (ROOT/'research/vehicle-snapshot.json').write_text(json.dumps(out,indent=2));print(json.dumps(out,indent=2))
finally:r.close()
