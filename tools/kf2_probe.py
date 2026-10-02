import ctypes as C,struct,re,json,sys
from pathlib import Path
from ctypes import wintypes as W
k=C.WinDLL('kernel32',use_last_error=True);ps=C.WinDLL('psapi',use_last_error=True)
k.OpenProcess.argtypes=[W.DWORD,W.BOOL,W.DWORD];k.OpenProcess.restype=W.HANDLE
k.ReadProcessMemory.argtypes=[W.HANDLE,C.c_void_p,C.c_void_p,C.c_size_t,C.POINTER(C.c_size_t)];k.ReadProcessMemory.restype=W.BOOL
k.CloseHandle.argtypes=[W.HANDLE]
ps.EnumProcessModulesEx.argtypes=[W.HANDLE,C.c_void_p,W.DWORD,C.POINTER(W.DWORD),W.DWORD]
h=k.OpenProcess(0x410,False,int(sys.argv[1]));assert h
mods=(C.c_void_p*1024)();needed=W.DWORD();assert ps.EnumProcessModulesEx(h,mods,C.sizeof(mods),C.byref(needed),3);base=mods[0]
def rd(a,n):
 b=C.create_string_buffer(n);got=C.c_size_t()
 return b.raw if a and k.ReadProcessMemory(h,a,b,n,C.byref(got)) and got.value==n else bytes(n)
def q(a):return struct.unpack('<Q',rd(a,8))[0]
def i(a):return struct.unpack('<i',rd(a,4))[0]
exe=Path(r'C:\Program Files (x86)\Steam\steamapps\common\killingfloor2\Binaries\Win64\KFGame.exe').read_bytes()
pe=struct.unpack_from('<I',exe,60)[0];num=struct.unpack_from('<H',exe,pe+6)[0];op=struct.unpack_from('<H',exe,pe+20)[0]
sections=[struct.unpack_from('<4I',exe,pe+24+op+n*40+8) for n in range(num)]
def va(off):
 for size,rva,raw,at in sections:
  if at<=off<at+raw:return base+rva+off-at
 raise ValueError(off)
def sig(s):
 p=b''.join(b'.' if v=='??' else re.escape(bytes([int(v,16)])) for v in s.split());m=list(re.finditer(p,exe,re.DOTALL));assert len(m)==1
 return va(m[0].start())
o=sig('3B ?? ?? ?? ?? ?? 7D ?? 48 8B C8 48 8B ?? ?? ?? ?? ?? 48 8B 0C C8 E8 ?? ?? ?? ?? 48 8B C8');objects=o+6+i(o+2)-8
n=sig('48 8B 0D ?? ?? ?? ?? 48 83 3C F9');names=n+7+i(n+3)
print('globals',hex(objects-base),hex(names-base),'counts',i(objects+8),i(names+8),'base',hex(base))
nameData=q(names)
nameCache={}
def name(idx):
 if idx not in nameCache:
  raw=rd(q(nameData+idx*8),256)
  nameCache[idx]=raw[20:].split(b'\0',1)[0].decode('utf-8',errors='replace')
 return nameCache[idx]
def oname(o):return name(i(o+0x48)) if o else ''
def fullname(o):
 parts=[]
 for _ in range(8):
  if not o:break
  parts.append(oname(o));o=q(o+0x40)
 return '.'.join(reversed(parts))
count=i(objects+8);assert 0<count<2000000
ptrs=struct.unpack('<'+'Q'*count,rd(q(objects),count*8))
print('firstnames',[(x,name(x)) for x in range(6)])
found=[]
for o in ptrs:
 if not o:continue
 label=oname(o);cls=oname(q(o+0x50))
 if label in ('KFPlayerController','KFPawn_Human','KFPawn_Monster','WorldInfo','KFWeapon','Health','Location','NetMode','Pawn','PlayerTick','PostRender','DrawHUD') or cls in ('KFPlayerController','KFPawn_Human'):
  found.append((hex(o),cls,fullname(o),rd(o+0x60,80).hex()))
out=Path('build-Release/kf2-research');out.mkdir(parents=True,exist_ok=True)
(out/'reflection.json').write_text(json.dumps(found,indent=2))
print('matches',len(found));print(*found[:8],sep='\n')
for o in ptrs:
 if o and oname(q(o+0x50))=='Class' and oname(o) in ('KFPlayerController','KFPawn','KFPawn_Human','KFPawn_Monster','KFWeapon','WorldInfo','Actor','Pawn','PlayerController','KFSkeletalMeshComponent','Canvas','KFInventoryManager','Controller','KFWeap_Healer_Syringe','KFWeap_HealerBase','KFPlayerReplicationInfo','HUD','SkeletalMeshComponent','Weapon','Camera','KFPlayerCamera','LocalPlayer','KFWeap_ScopedBase','TWSceneCapture2DDPGComponent','SceneCapture2DComponent'):
  fields=[]; f=q(o+0x80); seen=set()
  while f and f not in seen:
   seen.add(f);fields.append((oname(f),oname(q(f+0x50)),hex(f),[hex(i(f+x)&0xffffffff) for x in range(0x68,0xb0,4)]));f=q(f+0x60)
  (out/(oname(o)+'.json')).write_text(json.dumps(fields,indent=2))
wanted={'GetPlayerViewPoint','GetBoneLocation','Trace','Project','Draw2DLine','GetBoneName','GetParentBone','GetNumBones','SetRotation','SetPhysics','SetCollision','GotoState','GetAdjustedAim','GetWeaponAim','CalcWeaponFire','InstantFire','TraceFire','GetTraceRange','GetActorEyesViewPoint','SetFOV','UnlockFOV','GetFOVAngle','GetCameraViewPoint','GetBoneMatrix'}
functions=[]
for o in ptrs:
 if o and oname(q(o+0x50))=='Function' and oname(o) in wanted:
  fields=[];f=q(o+0x80);seen=set()
  while f and f not in seen:
   seen.add(f);fields.append([oname(f),oname(q(f+0x50)),i(f+0x8c),i(f+0x6c),hex(i(f+0xa8)&0xffffffff)]);f=q(f+0x60)
  functions.append([fullname(o),hex(o),i(o+0x88),fields])
(out/'functions.json').write_text(json.dumps(functions,indent=2))
def prop(t,key):
 for _ in range(64):
  if not t:return 0
  f=q(t+0x80)
  for z in range(4096):
   if not f:break
   if oname(f)==key:return i(f+0x8c)
   f=q(f+0x60)
  t=q(t+0x78)
 return 0
for o in ptrs:
 if o and oname(q(o+0x50))=='KFPlayerController' and not oname(o).startswith('Default__'):
  pawn=q(o+prop(q(o+0x50),'Pawn'))
  weapon=q(pawn+prop(q(pawn+0x50),'Weapon')) if pawn else 0
  print('live weapon',hex(weapon),oname(q(weapon+0x50)))
  if weapon:
   camera=q(o+prop(q(o+0x50),'PlayerCamera'));capture=q(weapon+prop(q(weapon+0x50),'SceneCapture'))
   state={'address':hex(weapon),'class':oname(q(weapon+0x50)),'camera':hex(camera),'cache':struct.unpack('<4f3if',rd(camera+prop(q(camera+0x50),'CameraCache'),32)),'weaponLocation':struct.unpack('<3f',rd(weapon+prop(q(weapon+0x50),'Location'),12)),'weaponRotation':struct.unpack('<3i',rd(weapon+prop(q(weapon+0x50),'Rotation'),12))}
   if capture:
    state.update(scopeScale=struct.unpack('<f',rd(weapon+prop(q(weapon+0x50),'ScopeTextureScale'),4))[0],capture=hex(capture),captureClass=oname(q(capture+0x50)),scopeFov=struct.unpack('<f',rd(capture+prop(q(capture+0x50),'FieldOfView'),4))[0],view=struct.unpack('<16f',rd(capture+prop(q(capture+0x50),'ViewMatrix'),64)),projection=struct.unpack('<16f',rd(capture+prop(q(capture+0x50),'ProjMatrix'),64)))
   (out/'live-weapon.json').write_text(json.dumps(state,indent=2))
k.CloseHandle(h)
