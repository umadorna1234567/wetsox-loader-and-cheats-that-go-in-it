import sys,json,struct,functools
from pathlib import Path
s=Path('tools/kf2_probe.py').read_text();ns={};exec(s.split('count=i(objects+8)')[0],ns);exec(s[s.index('def prop('):s.index('for o in ptrs:',s.index('def prop('))],ns)
q,rd,oname,name=ns['q'],ns['rd'],ns['oname'],ns['name'];prop=functools.lru_cache(None)(ns['prop'])
def at(o,k):return o+prop(q(o+0x50),k)
def ptr(o,k):return q(at(o,k))
def arr(o,k):return struct.unpack('<Qii',rd(at(o,k),16))
pc=next(int(f[0],16) for f in json.loads(Path('build-Release/kf2-research/reflection.json').read_text()) if f[1]=='KFPlayerController' and 'Default__' not in f[2]);pawn=ptr(pc,'Pawn');world=ptr(pc,'WorldInfo');p=ptr(world,'PawnList');seen=set()
while p and p not in seen and len(seen)<128:
 seen.add(p)
 if p!=pawn and ns['i'](at(p,'Health'))>0:
  m=ptr(p,'Mesh');asset=ptr(m,'SkeletalMesh');parent=ptr(m,'ParentAnimComponent')
  print('pawn',oname(q(p+0x50)),hex(p),'mesh',hex(m),'parent',hex(parent),'asset',oname(asset),'LOD',ns['i'](at(m,'PredictedLODLevel')))
  print('position',struct.unpack('<3f',rd(at(p,'Location'),12)),'mesh world',struct.unpack('<16f',rd(at(m,'LocalToWorld'),64)))
  a,n,c=arr(m,'SpaceBases');ca,cn,cc=arr(m,'CachedSpaceBases');print('pose',n,'cached',cn)
  required,rn,rc=arr(m,'RequiredBones');requiredSet=set(rd(required,rn))
  la,ln,lc=arr(asset,'LODModels')
  for level in sorted(set([min(1,ln-1),max(0,min(ln-1,ns['i'](at(m,'PredictedLODLevel'))))])):
   lod=q(la+level*8);chunks,nn,nc=struct.unpack('<Qii',rd(lod+0x10,16));used=set()
   for j in range(nn):
    ba,bb,bc=struct.unpack('<Qii',rd(chunks+j*64+0x24,16));used.update(struct.unpack('<'+'H'*bb,rd(ba,bb*2)))
   print('LOD',level,'used bones',len(used),'not currently evaluated',sorted(used-requiredSet))

  ba,bn,bc=arr(asset,'RefSkeleton');names=[name(ns['i'](ba+i*80)) for i in range(bn)]
  for key in ['HeadBoneName','PelvisBoneName','LeftFootBoneName','RightFootBoneName']:
   label=name(ns['i'](at(p,key)))
   if label in names:
    j=names.index(label);print(key,label,j,'pose',struct.unpack('<8f',rd(a+j*32,32)),'cached',struct.unpack('<8f',rd(ca+j*32,32)) if cn>j else None)
  if parent:print('parent asset',oname(ptr(parent,'SkeletalMesh')),'map',arr(m,'ParentBoneMap'))
 p=ptr(p,'NextPawn')
ns['k'].CloseHandle(ns['h'])
