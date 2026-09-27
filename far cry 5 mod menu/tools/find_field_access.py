"""Find direct memory accesses to a verified property offset in engine code."""
import argparse,hashlib,struct
from pathlib import Path
import pefile
from inspect_runtime import ROOT
from list_lua_api import EXPECTED
from capstone import Cs,CS_ARCH_X86,CS_MODE_64
from capstone.x86 import X86_OP_MEM
p=argparse.ArgumentParser();p.add_argument('offset',type=lambda x:int(x,0));a=p.parse_args()
raw=Path(r'C:\Program Files (x86)\Steam\steamapps\common\FarCry5\bin\FC_m64.dll').read_bytes()
if hashlib.sha256(raw).hexdigest().upper()!=EXPECTED:raise RuntimeError('Unverified engine')
pe=pefile.PE(data=raw,fast_load=True);d=Cs(CS_ARCH_X86,CS_MODE_64);d.detail=True;seen=set();out=[]
for section in pe.sections:
 if not section.Characteristics&0x20000000:continue
 b=section.get_data();pos=0;needle=struct.pack('<I',a.offset)
 while (pos:=b.find(needle,pos))>=0:
  for back in range(2,7):
   start=pos-back
   if start<0:continue
   ins=next(d.disasm(b[start:pos+4],section.VirtualAddress+start),None)
   if ins and ins.size==back+4 and ins.mnemonic in ('movss','mulss','addss','divss') and any(o.type==X86_OP_MEM and o.mem.disp==a.offset for o in ins.operands):
    if ins.address not in seen:seen.add(ins.address);out.append((hex(ins.address),ins.mnemonic,ins.op_str))
  pos+=4
print('\n'.join(map(str,out[:80])))
(ROOT/f'research/field-{a.offset:x}-accesses.json').write_text(__import__('json').dumps(out,indent=2))
