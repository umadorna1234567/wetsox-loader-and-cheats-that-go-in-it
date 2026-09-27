"""Read-only FC4 engine inspection; never executes game code or writes memory."""
import sys, argparse, struct, re, hashlib
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'far cry 5 mod menu/tools'))
from inspect_runtime import Reader
import pefile
from capstone import Cs,CS_ARCH_X86,CS_MODE_64
p=argparse.ArgumentParser()
p.add_argument('--pid',type=int)
p.add_argument('--function',nargs='*',type=lambda x:int(x,0),default=[])
p.add_argument('--strings',nargs='*',default=[])
p.add_argument('--limit',type=int,default=100)
p.add_argument('--snapshot',action='store_true')
p.add_argument('--raw',action='store_true')
p.add_argument('--brief',action='store_true')
p.add_argument('--calls',nargs='*',type=lambda x:int(x,0),default=[])
a=p.parse_args()
path=Path(r'C:\Program Files (x86)\Steam\steamapps\common\Far Cry 4\bin\FC64.dll')
raw=path.read_bytes();pe=pefile.PE(data=raw,fast_load=True)
print('SHA256',hashlib.sha256(raw).hexdigest(),'imageSize',hex(pe.OPTIONAL_HEADER.SizeOfImage))
r=Reader(a.pid,'FC64.dll') if a.pid else None
if r:print('liveBase',hex(r.base))
cache=Path(__file__).resolve().parents[1]/'build-Release/fc4-research'
cache.mkdir(parents=True,exist_ok=True)
sections={}
for section in pe.sections:
    name=section.Name.rstrip(b'\0').decode()
    if name not in ('.text','.rdata','.pdata'):continue
    file=cache/(name+'.bin')
    if a.snapshot:
        if not r:raise ValueError('--snapshot needs --pid')
        chunks=[]
        for off in range(0,section.Misc_VirtualSize,1024*1024):
            chunks.append(r.read(r.base+section.VirtualAddress+off,min(1024*1024,section.Misc_VirtualSize-off)))
        file.write_bytes(b''.join(chunks))
    if file.exists():sections[section.VirtualAddress]=file.read_bytes()
def data(rva,size):
    if r and size<=1024*1024:return r.read(r.base+rva,size)
    for start,buf in sections.items():
        if start<=rva<start+len(buf):return buf[rva-start:rva-start+size]
    return pe.get_data(rva,size)
d=Cs(CS_ARCH_X86,CS_MODE_64);d.detail=True
directory=pe.OPTIONAL_HEADER.DATA_DIRECTORY[3]
entries=list(struct.iter_unpack('<III',data(directory.VirtualAddress,directory.Size)))
for rva in a.function:
    start,end=next(((s,e) for s,e,_ in entries if s<=rva<e),(rva,rva+512))
    if a.raw:start,end=rva,rva+a.limit*15
    print('FUNCTION',hex(start),hex(end))
    for i,ins in enumerate(d.disasm(data(start,min(end-start,65536)),start)):
        if i>=a.limit:break
        annotation=''
        if ins.mnemonic=='lea' and 'rip' in ins.op_str:
            target=ins.address+ins.size+ins.operands[1].mem.disp
            try:
                value=data(target,128).split(b'\0')[0].decode('ascii')
                if len(value)>2 and value.isprintable():annotation=' ; '+repr(value)
            except (UnicodeDecodeError,TypeError):pass
        print(f'{ins.address:08x}: {ins.mnemonic} {ins.op_str}{annotation}')
targets={}
for section in pe.sections:
    if not a.calls or not section.Characteristics&0x20000000:continue
    code=sections.get(section.VirtualAddress,section.get_data())
    for m in re.finditer(rb'[\xe8\xe9]',code):
        off=m.start();rva=section.VirtualAddress+off
        if off+5>len(code):continue
        target=rva+5+struct.unpack_from('<i',code,off+1)[0]
        if target in a.calls:print('CALL',hex(target),'from',hex(rva))
for name in a.strings:
    for start,buf in sections.items():
        for m in re.finditer(re.escape(name.encode())+b'\0',buf):
            rva=start+m.start();targets[rva]=name;print('STRING',name,hex(rva))
for section in pe.sections:
    if not targets or not section.Characteristics&0x20000000:continue
    code=sections.get(section.VirtualAddress,section.get_data())
    for m in re.finditer(rb'[\x48\x4c]\x8d[\x05\x0d\x15\x1d\x25\x2d\x35\x3d]',code):
        off=m.start();rva=section.VirtualAddress+off
        target=rva+7+struct.unpack_from('<i',code,off+3)[0]
        if target in targets:
            print('REFERENCE',targets[target],hex(rva))
            if a.brief:
                for ins in d.disasm(data(rva-18,7),rva-18):print(f'{ins.address:08x}: {ins.mnemonic} {ins.op_str}')
            else:
                for ins in d.disasm(data(rva,72),rva):print(f'{ins.address:08x}: {ins.mnemonic} {ins.op_str}')
if r:r.close()
