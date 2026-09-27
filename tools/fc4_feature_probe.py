import sys,struct,zlib,collections
from pathlib import Path
sys.path.insert(0,str(Path.cwd()/'far cry 5 mod menu/tools'))
from inspect_runtime import Reader
r=Reader(int(sys.argv[1]),'FC64.dll')
def rd(p,n):
 try:return r.read(p,n) if n else bytes()
 except OSError:return bytes(n)
def q(p):return struct.unpack('<Q',rd(p,8))[0]
def ui(p):return struct.unpack('<I',rd(p,4))[0]
def comp(e,k):
 desc=q(e+0x68);count=ui(desc+0x60);arr=q(desc+0x58)
 if count>512:return 0
 for typ,idx in struct.iter_unpack('<II',rd(arr,count*8)):
  if typ==k:return q(q(e+0x98)+idx*8)
 return 0
manager=q(r.base+0x2e24c58);controller=q(q(manager+8));p=q(q(controller+8)+0x18);e=q(p+16);pawn=comp(e,0x911dd85f);state=q(pawn+0x70)
we=q(q(state+0x1258)+16);w=comp(we,0x37d2b3e9);d=q(w+0xa0);cfg=q(d+0x68)
print('local',hex(e),'weaponEntity',hex(we),'weapon',hex(w),'ownerRef',hex(q(w+8)),'ownerViaRef',hex(q(q(w+8)+16)),'stateOwner',hex(q(d+0x60)))
print('magazine',ui(d+0x104),'cfg1f8',ui(cfg+0x1f8),'cfg1fc',ui(cfg+0x1fc),'weapon cfg',hex(cfg),'config204',rd(cfg+0x204,1).hex())
if w:
 Path('build-Release/fc4-research/current-weapon-state.bin').write_bytes(rd(d,0x1200))
 Path('build-Release/fc4-research/current-weapon-config.bin').write_bytes(rd(cfg,0x1400))
 s=q(d+0x120);print('strategy',hex(s),'vt',hex(q(s)-r.base),'config',hex(q(s+0xc0)))
t=q(r.base+0x2dd48f8);n=ui(t+8);seen=set();stats=collections.Counter();animals=[]
for node in struct.unpack('<'+'Q'*n,rd(q(t+16),n*8)):
 while node and node not in seen and len(seen)<50000:
  seen.add(node);o=q(q(node+16)+16);node=q(node)
  a=comp(o,0x344a3710);vt=q(o)-r.base
  stats[vt]+=1
  if a:
   counters=comp(o,0x85615a15);graphic=comp(o,0x035982c6)
   animals.append((hex(o),hex(vt),hex(a),hex(counters),rd(q(counters+0x40)+0x18,4).hex(),ui(graphic+0xb8)))
print('animals',animals);print('top entity vtables',[(hex(k),v) for k,v in stats.most_common(12)])
r.close()
