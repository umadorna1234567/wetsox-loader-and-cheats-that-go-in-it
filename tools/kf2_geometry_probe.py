import sys,json,struct
from pathlib import Path
s=Path('tools/kf2_probe.py').read_text();ns={};exec(s.split('count=i(objects+8)')[0],ns);exec(s[s.index('def prop('):s.index('for o in ptrs:',s.index('def prop('))],ns)
q,rd,prop,oname=ns['q'],ns['rd'],ns['prop'],ns['oname']
def ptr(o,k):return q(o+prop(q(o+0x50),k))
pc=next(int(f[0],16) for f in json.loads(Path('build-Release/kf2-research/reflection.json').read_text()) if f[1]=='KFPlayerController' and 'Default__' not in f[2])
pawn=ptr(pc,'Pawn');world=ptr(pc,'WorldInfo');targets=[ptr(pawn,'Weapon')]
p=ptr(world,'PawnList')
while p and len(targets)<4:
 if p!=pawn:targets.append(p)
 p=ptr(p,'NextPawn')
for obj in targets:
 mesh=ptr(obj,'Mesh');asset=ptr(mesh,'SkeletalMesh')
 if not asset:continue
 print('object',oname(q(obj+0x50)),'asset',oname(asset),hex(asset))
 for owner,key in [(asset,'RefBasesInvMatrix'),(mesh,'SpaceBases')]:
  off=prop(q(owner+0x50),key);a,n,c=struct.unpack('<Qii',rd(owner+off,16));print(key,hex(off),n,'first atom',struct.unpack('<8f',rd(a,32)))

 off=prop(q(asset+0x50),'LODModels');data,count,cap=struct.unpack('<Qii',rd(asset+off,16));print('LODs',hex(off),hex(data),count,cap)
 if not 0<count<10:continue
 lod=q(data+8*min(1,count-1));raw=rd(lod,1024);print('selected LOD',min(1,count-1),hex(lod))
 vertices=struct.unpack_from('<i',raw,0x54)[0];stride=struct.unpack_from('<i',raw,0xbc)[0]
 idx=q(lod+0x48);ia,ni,ci=struct.unpack('<Qii',rd(idx+0x40,16));indices=struct.unpack('<'+'H'*ni,rd(ia,ni*2))
 print('geometry validation',vertices,'vertices',stride,'stride',ni,'indices','max index',max(indices),'16bit flag',struct.unpack_from('<I',raw,0x40)[0])
 ca,cn,cc=struct.unpack_from('<Qii',raw,0x10);covered=0
 for j in range(cn):
  chunk=rd(ca+j*64,64);base=struct.unpack_from('<i',chunk)[0];ba,bn,bc=struct.unpack_from('<Qii',chunk,0x24);rigid,soft=struct.unpack_from('<ii',chunk,0x34)
  print('chunk',j,'base',base,'bone count',bn,'rigid',rigid,'soft',soft);assert base==covered;covered=base+rigid+soft
 assert covered==vertices and max(indices)<vertices and ni%3==0

 Path('build-Release/kf2-research/lod-'+oname(asset)+'.bin').write_bytes(raw)
 for offset in [0x10,0x48,0x64,0xac,0xb4]:
  at=q(lod+offset);print('pointer',hex(offset),hex(at),'bytes',rd(at,96).hex(' '))
  Path('build-Release/kf2-research/lod-'+oname(asset)+'-'+hex(offset)+'.bin').write_bytes(rd(at,512))

 for i in range(0,1024,8):
  at,n,c=struct.unpack_from('<Qii',raw+bytes(16),i)
  if 0<n<=200000 and n<=c<=400000 and at>0x10000:
   print('array?',hex(i),hex(at),n,c,'first',rd(at,32).hex())
ns['k'].CloseHandle(ns['h'])
