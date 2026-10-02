"""Read-only capture of crossbow pose/camera timing during scoped firing."""
import sys,json,struct,time,functools
from pathlib import Path
source=Path('tools/kf2_probe.py').read_text(encoding='utf-8')
ns={};exec(source.split('count=i(objects+8)')[0],ns)
exec(source[source.index('def prop('):source.index('for o in ptrs:',source.index('def prop('))],ns)
q,rd,oname=ns['q'],ns['rd'],ns['oname']
prop=functools.lru_cache(None)(ns['prop'])
def address(obj,key):
 offset=prop(q(obj+0x50),key)
 if not offset:raise RuntimeError('Missing reflected field: '+key)
 return obj+offset
def pointer(obj,key):return q(address(obj,key))
def values(obj,key,fmt):return struct.unpack(fmt,rd(address(obj,key),struct.calcsize(fmt)))
pcs=[int(row[0],16) for row in json.loads(Path('build-Release/kf2-research/reflection.json').read_text()) if row[1]=='KFPlayerController' and 'Default__' not in row[2]]
records=[]
try:
 pc=next(pc for pc in pcs if pointer(pc,'Pawn'))
 pawn=pointer(pc,'Pawn');weapon=pointer(pawn,'Weapon');mesh=pointer(weapon,'Mesh');asset=pointer(mesh,'SkeletalMesh')
 assert oname(q(weapon+0x50))=='KFWeap_Bow_Crossbow','Equip the crossbow first'
 capture=pointer(weapon,'SceneCapture');camera=pointer(pc,'PlayerCamera')
 data,count,_=values(asset,'RefSkeleton','<Qii')
 index=next(j for j in range(count) if ns['name'](ns['i'](data+j*80))=='RW_Scope')
 fields={
  'camera':(camera,'CameraCache','<4f3if'),
  'view':(capture,'ViewMatrix','<16f'),
  'mesh':(mesh,'LocalToWorld','<16f'),
  'fov':(mesh,'FOV','<f'),
  'captureRate':(capture,'FrameRate','<f'),
  'location':(weapon,'Location','<3f'),
  'rotation':(weapon,'Rotation','<3i'),
  'bufferRotation':(pc,'WeaponBufferRotation','<3i'),
  'recoil':(weapon,'RecoilRotator','<3i'),
  'totalRecoil':(weapon,'TotalRecoilRotator','<3i'),
 }
 offsets={key:(address(obj,name),fmt,struct.calcsize(fmt)) for key,(obj,name,fmt) in fields.items()}
 start=time.perf_counter();duration=float(sys.argv[2]) if len(sys.argv)>2 else 30
 print('Recording scoped firing for',duration,'seconds',flush=True)
 while time.perf_counter()-start<duration:
  row={'t':time.perf_counter()-start}
  for key,(at,fmt,size) in offsets.items():row[key]=struct.unpack(fmt,rd(at,size))
  atoms,n,_=values(mesh,'SpaceBases','<Qii')
  if index<n:row['bone']=struct.unpack('<8f',rd(atoms+index*32,32))
  records.append(row);time.sleep(.01)
 path=Path('build-Release/kf2-research/recoil-motion.json')
 path.write_text(json.dumps(records),encoding='utf-8')
 print('Saved',len(records),'samples to',path)
 print('Scoped samples:',sum(r['fov'][0]<60 and r['captureRate'][0]>0 for r in records))
finally:ns['k'].CloseHandle(ns['h'])
