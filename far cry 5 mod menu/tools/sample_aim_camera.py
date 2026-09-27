"""Read-only movement diagnostics for the fingerprint-verified local player."""
import argparse, hashlib, json, math, struct, time
from pathlib import Path
from inspect_runtime import Reader, ROOT
from list_lua_api import EXPECTED

p=argparse.ArgumentParser();p.add_argument('--pid',type=int,required=True);a=p.parse_args()
r=Reader(a.pid)
try:
    if hashlib.sha256(Path(r.path).read_bytes()).hexdigest().upper()!=EXPECTED:raise RuntimeError('Unverified engine')
    def q(x):return struct.unpack('<Q',r.read(x,8))[0]
    def v(x,n):return struct.unpack('<'+'f'*n,r.read(x,4*n))
    rows=[]
    for _ in range(150):
        manager=q(r.base+0x4fb3110);ref=q(q(q(q(manager+8))+8)+24);entity=q(ref+16)
        desc=q(entity+0xc8);slot=struct.unpack('<i',r.read(desc+0x20,4))[0]
        if not 0<=slot<256:raise RuntimeError('Invalid pawn slot')
        pawn=q(q(entity+0xa8)+8*slot);look=0
        count=(q(pawn+0xb0)>>32)&0x7fffffff
        if count>128:raise RuntimeError('Invalid aspect count')
        for n in range(count):
            key,ptr=struct.unpack('<QQ',r.read(q(pawn+0xa8)+16*n,16))
            if key&0xffffffff==0x5df9d3ca:look=ptr
        if not look:raise RuntimeError('Missing look aspect')
        root=v(entity+0x30,16);angles=v(look+0x10,3);body=q(pawn+0x2a68)
        x,y,z=angles;cx,sx,cy,sy,cz,sz=math.cos(x),math.sin(x),math.cos(y),math.sin(y),math.cos(z),math.sin(z)
        local=(-sz*cx+cz*sy*sx,cz*cx+sz*sy*sx,cy*sx)
        view=tuple(sum(local[j]*root[4*j+i] for j in range(3)) for i in range(3))
        quat=v(body+0x180,4);x,y,z,w=quat
        camera=(2*(x*y-w*z),1-2*(x*x+z*z),2*(y*z+w*x))
        cosine=sum(x*y for x,y in zip(view,camera))
        rows.append(dict(time=time.time(),position=root[12:15],angles=angles,root=root[:12],eye=v(body+0x190,3),view=view,camera=camera,error_degrees=math.degrees(math.acos(max(-1,min(1,cosine))))))
        time.sleep(.1)
    (ROOT/'research/aim-camera-samples.json').write_text(json.dumps(rows,indent=2))
    print(json.dumps(dict(samples=len(rows),first=rows[0],last=rows[-1],max_error=max(x['error_degrees'] for x in rows))))
finally:r.close()
