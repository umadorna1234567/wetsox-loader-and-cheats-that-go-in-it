import sys,json,struct
from pathlib import Path
sys.path.insert(0,str(Path('build-Release/kf2-research/python').resolve()))
import capstone
s=Path('tools/kf2_probe.py').read_text();ns={};exec(s.split('count=i(objects+8)')[0],ns)
q,rd=ns['q'],ns['rd'];base=ns['base'];cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64)
rows=json.loads(Path('build-Release/kf2-research/functions.json').read_text())
seen=set()
for label,ptr,size,fields in rows:
 if label not in ['Engine.Weapon.GetAdjustedAim','Engine.Weapon.CalcWeaponFire','Engine.Actor.Trace']:continue
 obj=int(ptr,16);print(label,hex(obj),'size',size,fields)
 for off in range(0x90,0x110,8):
  v=q(obj+off);print(hex(off),hex(v))
  if base<v<base+0x2000000 and v not in seen:
   seen.add(v);print('CODE',hex(v-base))
   for ins in cs.disasm(rd(v,220),v):print(hex(ins.address-base),ins.mnemonic,ins.op_str)
ns['k'].CloseHandle(ns['h'])
