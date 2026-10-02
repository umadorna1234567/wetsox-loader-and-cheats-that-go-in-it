import ctypes as C,subprocess,os,time,struct
from pathlib import Path
from ctypes import wintypes as W
k=C.WinDLL('kernel32',use_last_error=True); ps=C.WinDLL('psapi',use_last_error=True)
class Ev(C.Structure):_fields_=[('code',W.DWORD),('pid',W.DWORD),('tid',W.DWORD),('pad',W.DWORD),('data',C.c_ubyte*160)]
k.WaitForDebugEvent.argtypes=[C.POINTER(Ev),W.DWORD];k.ContinueDebugEvent.argtypes=[W.DWORD,W.DWORD,W.DWORD]
k.OpenThread.argtypes=[W.DWORD,W.BOOL,W.DWORD];k.OpenThread.restype=W.HANDLE
k.GetThreadContext.argtypes=[W.HANDLE,C.c_void_p];k.CloseHandle.argtypes=[W.HANDLE]
k.ReadProcessMemory.argtypes=[W.HANDLE,C.c_void_p,C.c_void_p,C.c_size_t,C.c_void_p]
ps.EnumProcessModulesEx.argtypes=[W.HANDLE,C.c_void_p,W.DWORD,C.POINTER(W.DWORD),W.DWORD]
ps.GetModuleFileNameExW.argtypes=[W.HANDLE,W.HMODULE,W.LPWSTR,W.DWORD]
root=Path.cwd();qt=root/'tools/qt/6.8.3/msvc2022_64'
env=os.environ.copy();env.update(PATH=str(qt/'bin')+';'+env['PATH'],QT_PLUGIN_PATH=str(qt/'plugins'),QML_IMPORT_PATH=str(qt/'qml'),QT_QPA_PLATFORM='offscreen',QT_QUICK_BACKEND='software',QT_QPA_FONTDIR='C:/Windows/Fonts',QT_FORCE_STDERR_LOGGING='1')
f=open(root/'debug-ui.log','w')
p=subprocess.Popen([str(root/'build-Release/backend/WetsoxApp.exe'),'--smoke-test','--smoke-game','killingfloor2'],env=env,creationflags=2|0x08000000,stdout=f,stderr=f)
ev=Ev();deadline=time.time()+30
while time.time()<deadline:
 if not k.WaitForDebugEvent(C.byref(ev),500):continue
 cont=0x10002
 if ev.code==1:
  data=bytes(ev.data);code=struct.unpack_from('<I',data)[0];address=struct.unpack_from('<Q',data,16)[0];first=struct.unpack_from('<I',data,152)[0]
  if code!=0x80000003:cont=0x80010001
  if code==0xc0000005:
   print('exception',hex(code),hex(address))
   mods=(W.HMODULE*1024)();needed=W.DWORD();ps.EnumProcessModulesEx(int(p._handle),mods,C.sizeof(mods),C.byref(needed),3)
   bases=[]
   for mod in mods[:needed.value//8]:
    path=C.create_unicode_buffer(32768);ps.GetModuleFileNameExW(int(p._handle),mod,path,32768);bases.append((int(mod),path.value))
   bases.sort()
   def label(at):
    found=[(base,path) for base,path in bases if base<=at]
    if not found:return hex(at)
    base,path=found[-1]
    return Path(path).name+'+'+hex(at-base) if at-base<0x8000000 else hex(at)
   print('fault',label(address))
   thread=k.OpenThread(0x8,False,ev.tid);buf=C.create_string_buffer(1248);aligned=(C.addressof(buf)+15)&~15;C.c_uint32.from_address(aligned+48).value=0x100003
   if k.GetThreadContext(thread,aligned):
    rsp=C.c_uint64.from_address(aligned+152).value;stack=C.create_string_buffer(2048)
    if k.ReadProcessMemory(int(p._handle),rsp,stack,2048,None):
     values=struct.unpack('<256Q',stack.raw);print('stack candidates',*[label(v) for v in values if '.dll+' in label(v) or '.exe+' in label(v)][:45],sep='\n')
   k.CloseHandle(thread)
 if ev.code==5:
  k.ContinueDebugEvent(ev.pid,ev.tid,cont);break
 k.ContinueDebugEvent(ev.pid,ev.tid,cont)
else:p.kill()
f.close()
