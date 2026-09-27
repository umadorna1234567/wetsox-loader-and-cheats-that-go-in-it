"""Read-only inspection of the running Far Cry 5 engine module.

No allocations, remote threads, hooks, or writes in the target process.
Only the named engine's code and explicitly requested data ranges are read.
"""
import argparse
import ctypes as C
from ctypes import wintypes as W
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "research/python"))
from capstone import Cs, CS_ARCH_X86, CS_MODE_64
import pefile

class Module(C.Structure):
    _fields_ = [("size", W.DWORD), ("id", W.DWORD), ("pid", W.DWORD),
                ("globalUsage", W.DWORD), ("usage", W.DWORD),
                ("base", C.c_void_p), ("length", W.DWORD), ("handle", W.HMODULE),
                ("name", W.WCHAR * 256), ("path", W.WCHAR * 260)]

k = C.WinDLL("kernel32", use_last_error=True)
k.OpenProcess.argtypes = [W.DWORD, W.BOOL, W.DWORD]; k.OpenProcess.restype = W.HANDLE
k.CloseHandle.argtypes = [W.HANDLE]
k.CreateToolhelp32Snapshot.argtypes = [W.DWORD, W.DWORD]; k.CreateToolhelp32Snapshot.restype = W.HANDLE
k.Module32FirstW.argtypes = [W.HANDLE, C.POINTER(Module)]
k.Module32NextW.argtypes = [W.HANDLE, C.POINTER(Module)]
k.ReadProcessMemory.argtypes = [W.HANDLE, C.c_void_p, C.c_void_p, C.c_size_t, C.POINTER(C.c_size_t)]

class Reader:
    def __init__(self, pid, engine_name="FC_m64.dll"):
        self.handle = k.OpenProcess(0x1010, False, pid)
        if not self.handle: raise C.WinError(C.get_last_error())
        snapshot = k.CreateToolhelp32Snapshot(0x18, pid)
        if snapshot == C.c_void_p(-1).value: raise C.WinError(C.get_last_error())
        try:
            entry = Module(); entry.size = C.sizeof(entry)
            ok = k.Module32FirstW(snapshot, C.byref(entry))
            while ok:
                if entry.name.lower() == engine_name.lower():
                    self.base, self.size, self.path = entry.base, entry.length, entry.path
                    break
                ok = k.Module32NextW(snapshot, C.byref(entry))
            else: raise RuntimeError("FC_m64.dll not loaded")
        finally: k.CloseHandle(snapshot)
    def read(self, address, length):
        if not 0 < length <= 1024*1024: raise ValueError("Read size must be 1..1 MiB")
        data = C.create_string_buffer(length); got = C.c_size_t()
        if not k.ReadProcessMemory(self.handle, address, data, length, C.byref(got)) or got.value != length:
            raise C.WinError(C.get_last_error())
        return data.raw
    def close(self): k.CloseHandle(self.handle)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--pid", type=int, required=True)
    parser.add_argument("--rva", type=lambda s:int(s,0), action="append", default=[])
    parser.add_argument("--address", type=lambda s:int(s,0), action="append", default=[])
    parser.add_argument("--bytes", type=int, default=128)
    args = parser.parse_args()
    reader = Reader(args.pid)
    try:
        print(json.dumps({"module":reader.path,"base":hex(reader.base),"size":reader.size}))
        pe = pefile.PE(reader.path, fast_load=True)
        disassembler = Cs(CS_ARCH_X86, CS_MODE_64)
        for rva in args.rva:
            if rva < 0 or rva + args.bytes > reader.size: raise ValueError("RVA outside engine")
            for _ in range(4):
                first=reader.read(reader.base+rva,5)
                if first[0]!=0xe9: break
                target=rva+5+struct.unpack_from('<i',first,1)[0]
                if not 0<=target<reader.size-args.bytes: break
                print(f"Thunk {rva:#x} -> {target:#x}")
                rva=target
            data = reader.read(reader.base+rva, args.bytes)
            print(f"RVA {rva:#x}; disk_matches_live={data == pe.get_data(rva,args.bytes)}")
            for instruction in disassembler.disasm(data, reader.base+rva):
                if instruction.mnemonic=='int3': break
                print(f"{instruction.address-reader.base:08x}: {instruction.mnemonic} {instruction.op_str}")
        for address in args.address:
            data = reader.read(address, args.bytes)
            print(f"DATA {address:#x}")
            for offset in range(0,len(data)-7,8):
                print(f"+{offset:04x}: {struct.unpack_from('<Q',data,offset)[0]:016x}")
    finally: reader.close()

if __name__ == "__main__": main()
