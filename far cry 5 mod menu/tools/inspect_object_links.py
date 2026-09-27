"""Bounded read-only typed links from explicitly supplied objects."""
import argparse,hashlib,struct
from pathlib import Path
from inspect_runtime import Reader
from list_lua_api import EXPECTED
from capstone import Cs,CS_ARCH_X86,CS_MODE_64
from capstone.x86 import X86_OP_MEM,X86_REG_RIP
p=argparse.ArgumentParser();p.add_argument('--pid',type=int,required=True);p.add_argument('address',nargs='+',type=lambda x:int(x,0));p.add_argument('--bytes',type=int,default=512);a=p.parse_args()
r=Reader(a.pid)
try:
    if hashlib.sha256(Path(r.path).read_bytes()).hexdigest().upper()!=EXPECTED:raise RuntimeError('Unverified engine')
    def q(x):return struct.unpack('<Q',r.read(x,8))[0]
    def string(x):
        s=r.read(x,100).split(b'\0')[0].decode('ascii')
        return s if s and all(c.isalnum() or c in ' _:.?@$<>' for c in s) else None
    dis=Cs(CS_ARCH_X86,CS_MODE_64);dis.detail=True
    def typename(vt):
        try:
            locator=q(vt-8)
            if r.base<=locator<r.base+r.size:
                record=r.read(locator,24)
                if struct.unpack_from('<I',record)[0]==1:
                    name=string(r.base+struct.unpack_from('<I',record,12)[0]+16)
                    if name:return name
        except (OSError,UnicodeError):pass
        try:
            fn=q(vt+8)
            for _ in range(4):
                code=r.read(fn,5)
                if code[0]!=0xe9:break
                fn+=5+struct.unpack_from('<i',code,1)[0]
            for ins in dis.disasm(r.read(fn,100),fn):
                if ins.mnemonic in ('cmp','lea'):
                    for op in ins.operands:
                        if op.type==X86_OP_MEM and op.mem.base==X86_REG_RIP:
                            try:
                                name=string(q(ins.address+ins.size+op.mem.disp))
                                if name:return name
                            except (OSError,UnicodeError):pass
                if ins.mnemonic in ('ret','int3'):break
        except OSError:pass
        return None
    for address in a.address:
        print('OBJECT',hex(address))
        for offset in range(0,min(a.bytes,4096),8):
            try:
                ptr=q(address+offset)
                if not 0x10000<ptr<r.base:continue
                vt=q(ptr)
                if not r.base<=vt<r.base+r.size:continue
                print(hex(offset),hex(ptr),'vtable',hex(vt-r.base),typename(vt))
            except OSError:pass
finally:r.close()
