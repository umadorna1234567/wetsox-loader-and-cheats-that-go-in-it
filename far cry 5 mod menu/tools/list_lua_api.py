"""Enumerate Lua API names from the inspected FC5 build, without executing Lua."""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from inspect_runtime import Reader, ROOT

EXPECTED = "00833FAE4D5D70213158A146CA98439B28A8E962B934885ECCCD906204880AF2"
def u64(data, offset=0): return struct.unpack_from("<Q",data,offset)[0]
def u32(data, offset=0): return struct.unpack_from("<I",data,offset)[0]

class LuaReader:
    def __init__(self, reader): self.r=reader
    def string(self, pointer):
        header=self.r.read(pointer,24)
        if header[8]!=4: return None
        length=u32(header,12) # This game's TString stores length before its hash.
        if not 0<length<=512: return None
        return self.r.read(pointer+24,length).decode("utf-8",errors="replace")
    def table(self, pointer):
        header=self.r.read(pointer,64)
        if header[8]!=5 or header[11]>16: raise ValueError("Unexpected Lua table layout")
        count=1<<header[11];nodes=u64(header,32)
        if count*40>1024*1024: raise ValueError("Table exceeds bounded read limit")
        raw=self.r.read(nodes,count*40)
        output={}
        for offset in range(0,len(raw),40):
            node=raw[offset:offset+40]
            if u32(node,24)!=4 or u32(node,8)==0: continue
            name=self.string(u64(node,16))
            if name is None: continue
            kind=u32(node,8);value=u64(node)
            entry={"type":kind}
            if kind in (5,6): entry["pointer"]=hex(value)
            if kind==6:
                closure=self.r.read(value,40)
                if closure[8]==6 and closure[10]==1:
                    function=u64(closure,32)
                    entry["c_function_rva"]=hex(function-self.r.base) if self.r.base<=function<self.r.base+self.r.size else "outside engine"
            output[name]=entry
        if self.r.read(pointer,64)[24:40]!=header[24:40]: raise RuntimeError("Table changed during read; retry while paused")
        return output

def main():
    parser=argparse.ArgumentParser();parser.add_argument("--pid",type=int,required=True)
    args=parser.parse_args();reader=Reader(args.pid)
    try:
        if hashlib.sha256(Path(reader.path).read_bytes()).hexdigest().upper()!=EXPECTED: raise RuntimeError("Unverified engine build")
        # Verified MOV RCX,[RIP+disp32] at this build's script-system reference.
        instruction=reader.read(reader.base+0xdfa427,7)
        if instruction[:3]!=b"\x48\x8b\x0d": raise RuntimeError("Script-system reference changed")
        slot=reader.base+0xdfa42e+struct.unpack_from("<i",instruction,3)[0]
        system=u64(reader.read(slot,8));state=u64(reader.read(system+16,8))
        gt=reader.read(state+0x78,16)
        if u32(gt,8)!=5: raise RuntimeError("Global TValue is not a table")
        lua=LuaReader(reader);globals_=lua.table(u64(gt))
        tables={}
        for name,entry in globals_.items():
            if entry["type"]==5 and not name.startswith("_"):
                tables[name]=lua.table(int(entry["pointer"],16))
        report={"engine_sha256":EXPECTED,"script_system_slot_rva":hex(slot-reader.base),
                "globals":globals_,"tables":tables}
        destination=ROOT/"research/lua-api.json"
        destination.write_text(json.dumps(report,indent=2),encoding="utf-8")
        print(f"Read {len(globals_)} globals and {len(tables)} tables. Report: {destination}")
        for name,table in sorted(tables.items()):
            print(name+": "+", ".join(sorted(table)))
    finally: reader.close()
if __name__=="__main__": main()
