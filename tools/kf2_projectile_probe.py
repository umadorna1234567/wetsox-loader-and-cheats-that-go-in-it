import sys,struct
from pathlib import Path
s=Path('tools/kf2_probe.py').read_text();ns={};exec(s.split('count=i(objects+8)')[0],ns);exec(s[s.index('def prop('):s.index('for o in ptrs:',s.index('def prop('))],ns)
q,i,rd,oname,prop=ns['q'],ns['i'],ns['rd'],ns['oname'],ns['prop']
for o in struct.unpack('<'+'Q'*i(ns['objects']+8),rd(q(ns['objects']),8*i(ns['objects']+8))):
 if o and oname(o)=='Default__KFProj_Bolt_Crossbow':
  cls=q(o+0x50);print('default',hex(o),'class',hex(cls))
  for off in range(0x124,0x270,4):
   if q(cls+off)==o:print('default pointer offset',hex(off))
  for key in ['Speed','MaxSpeed','GravityScale','TossZ','Physics','Acceleration']:
   off=prop(cls,key);print(key,hex(off),rd(o+off,12).hex(),struct.unpack('<f',rd(o+off,4))[0])
ns['k'].CloseHandle(ns['h'])
