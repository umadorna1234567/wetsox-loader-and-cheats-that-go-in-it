import sys,struct
from pathlib import Path
s=Path('tools/kf2_probe.py').read_text();ns={};exec(s.split('count=i(objects+8)')[0],ns)
q,i,rd,oname=ns['q'],ns['i'],ns['rd'],ns['oname']
for o in struct.unpack('<'+'Q'*i(ns['objects']+8),rd(q(ns['objects']),8*i(ns['objects']+8))):
 if not o:continue
 label=oname(o);outer=oname(q(o+0x40))
 if label in ('ImpactInfo','TraceHitInfo') and oname(q(o+0x50))=='ScriptStruct' or label=='PlayerMove' and outer=='PlayerFlying' or label=='TraceComponent' and outer=='Actor':
  print(label,outer,hex(o),'size',i(o+0x88));f=q(o+0x80)
  for n in range(100):
   if not f:break
   print(' ',oname(f),i(f+0x8c),i(f+0x6c));f=q(f+0x60)
ns['k'].CloseHandle(ns['h'])
