from pathlib import Path
import sys,json,struct
sys.stdout.reconfigure(encoding='utf-8');sys.argv=['probe',sys.argv[1]];s=Path('tools/kf2_probe.py').read_text();ns={};exec(s.split('count=i(objects+8)')[0],ns);exec(s[s.index('def prop('):s.index('for o in ptrs:',s.index('def prop('))],ns)
q,rd,prop,oname=ns['q'],ns['rd'],ns['prop'],ns['oname']
def addr(o,k):return o+prop(q(o+0x50),k)
def ptr(o,k):return q(addr(o,k))
pcs=[int(f[0],16) for f in json.loads(Path('build-Release/kf2-research/reflection.json').read_text()) if f[1]=='KFPlayerController' and 'Default__' not in f[2]]
for pc in pcs:
 pawn=ptr(pc,'Pawn');w=ptr(pawn,'Weapon');m=ptr(w,'Mesh');asset=ptr(m,'SkeletalMesh');print('weapon',oname(q(w+0x50)),'mesh',hex(m),'asset',oname(asset))
 if not asset:continue
 a,n,cap=struct.unpack('<Qii',rd(addr(asset,'Sockets'),16));print('sockets',n)
 for index in range(min(n,50)):
  sock=q(a+index*8);print('socket',ns['name'](ns['i'](addr(sock,'SocketName'))),'bone',ns['name'](ns['i'](addr(sock,'BoneName'))),struct.unpack('<3f',rd(addr(sock,'RelativeLocation'),12)))
 offset=prop(q(asset+0x50),'RefSkeleton');a,n,cap=struct.unpack('<Qii',rd(asset+offset,16));print('ref skeleton',hex(offset),n)
 if 0<n<1000:
  bones=[ns['name'](ns['i'](a+j*80)) for j in range(n)]
  if 'RW_Scope' in bones:
   index=bones.index('RW_Scope');ba,bn,bc=struct.unpack('<Qii',rd(addr(m,'SpaceBases'),16))
   print('scope index',index,'spacebases',bn)
   print('bone atom',struct.unpack('<8f',rd(ba+index*32,32)))
   print('localtoworld',struct.unpack('<16f',rd(addr(m,'LocalToWorld'),64)))
   print('mesh FOV',struct.unpack('<f',rd(addr(m,'FOV'),4)))
   capture=ptr(w,'SceneCapture')
   if capture:
    print('capture view',struct.unpack('<16f',rd(addr(capture,'ViewMatrix'),64)))
    print('capture projection',struct.unpack('<16f',rd(addr(capture,'ProjMatrix'),64)))
    for key in ('FieldOfView','FrameRate'):
     print('capture',key,struct.unpack('<f',rd(addr(capture,key),4))[0])
   for key in ('WeaponLag','Location'):
    print('weapon',key,struct.unpack('<3f',rd(addr(w,key),12)))
   print('weapon Rotation',struct.unpack('<3i',rd(addr(w,'Rotation'),12)))
   for key in ('ScopeTextureScale','IronSightMeshFOVCompensationScale'):
    print('weapon',key,struct.unpack('<f',rd(addr(w,key),4))[0])

   cam=ptr(pc,'PlayerCamera');cache=struct.unpack('<4f3if',rd(addr(cam,'CameraCache'),32));atom=struct.unpack('<8f',rd(ba+index*32,32));matrix=struct.unpack('<16f',rd(addr(m,'LocalToWorld'),64));point=[sum(atom[4+r]*matrix[r*4+c] for r in range(3))+matrix[12+c] for c in range(3)]
   import math
   pitch,yaw,roll=[v*math.pi/32768 for v in cache[4:7]];delta=[point[j]-cache[1+j] for j in range(3)];forward=[math.cos(pitch)*math.cos(yaw),math.cos(pitch)*math.sin(yaw),math.sin(pitch)];right=[-math.sin(yaw),math.cos(yaw),0];up=[-math.sin(pitch)*math.cos(yaw),-math.sin(pitch)*math.sin(yaw),math.cos(pitch)]
   dot=lambda a,b:sum(x*y for x,y in zip(a,b));depth=dot(delta,forward);fov=struct.unpack('<f',rd(addr(m,'FOV'),4))[0];scale=960/math.tan(fov*math.pi/360)
   print('scope pivot world',point,'depth',depth,'screen',[960+dot(delta,right)*scale/depth,540-dot(delta,up)*scale/depth])

ns['k'].CloseHandle(ns['h'])
