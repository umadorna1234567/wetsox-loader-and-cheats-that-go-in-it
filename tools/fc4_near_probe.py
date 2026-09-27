exec(open('tools/fc4_feature_probe.py').read().replace('r.close()',''))
from capstone import Cs,CS_ARCH_X86,CS_MODE_64
dis=Cs(CS_ARCH_X86,CS_MODE_64);dis.detail=True
cache={}
def name(vt):
 if vt in cache:return cache[vt]
 code=q(vt+8)
 for _ in range(3):
  ins=next(dis.disasm(rd(code,16),code),None)
  if ins and ins.mnemonic=='jmp':code=int(ins.op_str,16)
 for ins in dis.disasm(rd(code,128),code):
  if ins.mnemonic=='lea' and 'rip' in ins.op_str:
   ptr=ins.address+ins.size+ins.operands[1].mem.disp
   try:
    text=rd(ptr,96).split(b'\0')[0].decode('ascii')
    if len(text)>2 and text.startswith(('C','I','?')):cache[vt]=text;return text
   except UnicodeDecodeError:pass
 cache[vt]='?';return '?'
local=struct.unpack('<3f',rd(e+0x50,12));near=[]
for node in seen:
 o=q(q(node+16)+16)
 if not o:continue
 pos=struct.unpack('<3f',rd(o+0x50,12));dist=sum((a-b)**2 for a,b in zip(pos,local))**.5
 if dist<25:
  components=[];cnt=ui(o+0xa0)
  if cnt<256:
   for c in struct.unpack('<'+'Q'*cnt,rd(q(o+0x98),cnt*8)):
    if c:components.append(name(q(c)))
  near.append((dist,hex(o),hex(q(o)-r.base),name(q(o)),components))
for item in sorted(near)[:35]:print('NEAR',item)
r.close()
