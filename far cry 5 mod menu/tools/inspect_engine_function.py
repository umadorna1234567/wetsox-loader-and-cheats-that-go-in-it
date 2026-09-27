"""Disassemble the PE unwind-delimited function containing an engine RVA."""
import argparse, hashlib, struct
from pathlib import Path
from inspect_runtime import ROOT
from list_lua_api import EXPECTED
import pefile
from capstone import Cs, CS_ARCH_X86, CS_MODE_64

p=argparse.ArgumentParser()
p.add_argument('rva',type=lambda s:int(s,0),nargs='+')
p.add_argument('--limit',type=int,default=180)
a=p.parse_args()
raw=Path(r'C:\Program Files (x86)\Steam\steamapps\common\FarCry5\bin\FC_m64.dll').read_bytes()
if hashlib.sha256(raw).hexdigest().upper()!=EXPECTED: raise RuntimeError('Unverified engine')
pe=pefile.PE(data=raw,fast_load=True)
directory=pe.OPTIONAL_HEADER.DATA_DIRECTORY[3]
entries=list(struct.iter_unpack('<III',pe.get_data(directory.VirtualAddress,directory.Size)))
dis=Cs(CS_ARCH_X86,CS_MODE_64)
for rva in a.rva:
    match=next(((s,e) for s,e,_ in entries if s<=rva<e),None)
    start,end=match if match else (rva,rva+min(4096,a.limit*15))
    print(f'FUNCTION {start:#x}..{end:#x}; requested {rva:#x}; unwind={match is not None}')
    for i,ins in enumerate(dis.disasm(pe.get_data(start,end-start),start)):
        if i>=a.limit or ins.mnemonic=='int3':break
        print(f'{ins.address:08x}: {ins.mnemonic} {ins.op_str}')
