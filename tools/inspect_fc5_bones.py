import sys,struct,zlib,json
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'far cry 5 mod menu/tools'))
from inspect_runtime import Reader
r=Reader(int(sys.argv[1]))
def q(p):
 try:return struct.unpack('<Q',r.read(p,8))[0]
 except OSError:return 0
def component(p,f):
 try:
  slot=struct.unpack('<i',r.read(q(p+0xc8)+f,4))[0]
  return q(q(p+0xa8)+8*slot) if 0<=slot<256 else 0
 except OSError:return 0
names=['Head','Neck','Spine2','Spine1','Spine','Hips']+[side+part for side in ['Left','Right'] for part in ['Shoulder','Arm','ForeArm','Hand','UpLeg','Leg','Foot','ToeBase']]
hashes={zlib.crc32(n.encode()):n for n in names}
results=[];seen=set()
try:
 buckets=q(r.base+0x4eb41f0);count=q(r.base+0x4eb41e8)
 if count>65536:raise RuntimeError('bad count')
 for node in struct.unpack('<'+'Q'*count,r.read(buckets,count*8)):
  while node and node not in seen and len(seen)<50000:
   seen.add(node);nxt,identity,ref=struct.unpack('<QQQ',r.read(node,24));node=nxt;obj=q(ref+16)
   if q(obj)!=r.base+0x445b658:continue
   graphic=component(obj,0x2c);skeleton=graphic+0x120
   count2=(q(skeleton+0xe0)>>32)&0x7fffffff
   if not graphic or not 0<count2<2048:continue
   entries=dict(struct.iter_unpack('<Ii',r.read(q(skeleton+0xd8),count2*8)))
   results.append({'id':identity,'matched':{name:entries[h] for h,name in hashes.items() if h in entries},'hash_count':count2})
   if len(results)>=3:break
  if len(results)>=3:break
 print(json.dumps(results))
finally:r.close()
