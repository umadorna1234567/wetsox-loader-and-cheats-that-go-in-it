"""Find engine string LEA references in the fingerprinted on-disk image."""
import argparse,hashlib,re,struct
from pathlib import Path
import pefile
from list_lua_api import EXPECTED
from inspect_runtime import ROOT
from capstone import Cs,CS_ARCH_X86,CS_MODE_64

p=argparse.ArgumentParser();p.add_argument('names',nargs='+');a=p.parse_args()
path=Path(r'C:\Program Files (x86)\Steam\steamapps\common\FarCry5\bin\FC_m64.dll')
raw=path.read_bytes()
if hashlib.sha256(raw).hexdigest().upper()!=EXPECTED:raise RuntimeError('Unverified engine')
pe=pefile.PE(data=raw,fast_load=True);targets={}
for name in a.names:
    start=0
    while (start:=raw.find(name.encode()+b'\0',start))>=0:
        targets[pe.get_rva_from_offset(start)]=name;start+=1
dis=Cs(CS_ARCH_X86,CS_MODE_64);found=[]
for section in pe.sections:
    if not section.Characteristics&0x20000000:continue
    data=section.get_data()
    for m in re.finditer(rb'[\x48\x4c]\x8d[\x05\x0d\x15\x1d\x25\x2d\x35\x3d]',data):
        offset=m.start();rva=section.VirtualAddress+offset
        target=rva+7+struct.unpack_from('<i',data,offset+3)[0]
        if target not in targets:continue
        found.append((targets[target],rva))
        print(targets[target],hex(target),'reference',hex(rva))
        for ins in dis.disasm(data[offset:offset+100],rva):
            print(hex(ins.address),ins.mnemonic,ins.op_str)
            if ins.mnemonic in ('ret','int3'):break
(ROOT/'research/field-references.json').write_text(__import__('json').dumps(found,indent=2))
