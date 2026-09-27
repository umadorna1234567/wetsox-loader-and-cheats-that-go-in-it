"""Read-only camera alignment check for a diagnostic camera address."""
import sys, struct, math
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'far cry 5 mod menu/tools'))
from inspect_runtime import Reader
r = Reader(int(sys.argv[1]))
def q(p): return struct.unpack('<Q', r.read(p,8))[0] if p else 0
def floats(p,n): return struct.unpack('<'+'f'*n,r.read(p,n*4))
try:
    camera=int(sys.argv[2],0)
    controller=q(q(q(r.base+0x4fb3110)+8))
    entity=q(q(q(controller+8)+24)+16)
    slot=struct.unpack('<i',r.read(q(entity+0xc8)+0x20,4))[0]
    pawn=q(q(entity+0xa8)+8*slot)
    look=0
    for i in range((q(pawn+0xb0)>>32)&0x7fffffff):
        ident,ptr=struct.unpack('<QQ',r.read(q(pawn+0xa8)+16*i,16))
        if ident&0xffffffff==0x5df9d3ca:look=ptr
    root=floats(entity+0x30,16); angles=floats(look+0x10,3);eye=floats(q(pawn+0x2a68)+0x190,3)
    cx,cy,cz=map(math.cos,angles);sx,sy,sz=map(math.sin,angles)
    local=(-sz*cx+cz*sy*sx,cz*cx+sz*sy*sx,cy*sx)
    view=[sum(local[k]*root[k*4+j] for k in range(3)) for j in range(3)]
    m=floats(camera+0x170,16)
    p=[eye[i]+50*view[i] for i in range(3)]
    clip=[sum(p[k]*m[k*4+j] for k in range(3))+m[12+j] for j in range(4)]
    print(dict(eye=eye,view=view,matrix=m,clip=clip,centerError=math.hypot(clip[0],clip[1])/abs(clip[3])))
finally:r.close()
