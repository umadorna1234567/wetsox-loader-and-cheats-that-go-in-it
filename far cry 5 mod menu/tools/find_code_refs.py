"""Find direct call/jump references in the fingerprinted engine image."""
import argparse,hashlib,struct
from pathlib import Path
from inspect_runtime import ROOT
from list_lua_api import EXPECTED
import pefile
p=argparse.ArgumentParser();p.add_argument('target',type=lambda s:int(s,0));a=p.parse_args()
raw=Path(r'C:\Program Files (x86)\Steam\steamapps\common\FarCry5\bin\FC_m64.dll').read_bytes()
if hashlib.sha256(raw).hexdigest().upper()!=EXPECTED:raise RuntimeError('Unverified engine')
pe=pefile.PE(data=raw,fast_load=True)
for section in pe.sections:
    if not section.Characteristics&0x20000000:continue
    data=section.get_data()
    for opcode in (b'\xe8',b'\xe9'):
        pos=0
        while (pos:=data.find(opcode,pos))>=0:
            if pos+5<=len(data) and section.VirtualAddress+pos+5+struct.unpack_from('<i',data,pos+1)[0]==a.target:
                print('call' if opcode==b'\xe8' else 'jump',hex(section.VirtualAddress+pos))
            pos+=1
