"""Read verified local-player component fields; no gameplay calls or writes."""
import argparse, hashlib, json, struct
from pathlib import Path
from inspect_runtime import Reader, ROOT
from list_lua_api import EXPECTED

def main():
    p=argparse.ArgumentParser();p.add_argument('--pid',required=True,type=int);a=p.parse_args()
    r=Reader(a.pid)
    try:
        if hashlib.sha256(Path(r.path).read_bytes()).hexdigest().upper()!=EXPECTED:raise RuntimeError('Unverified engine')
        def q(x):return struct.unpack('<Q',r.read(x,8))[0]
        def i(x):return struct.unpack('<i',r.read(x,4))[0]
        def vec(x,n=3):return struct.unpack('<'+'f'*n,r.read(x,n*4))
        manager=q(r.base+0x4fb3110);ref=q(q(q(q(manager+8))+8)+24);entity=q(ref+16)
        desc=q(entity+0xc8);components=q(entity+0xa8)
        pawn=q(components+8*i(desc+0x20));equipment=q(components+8*i(desc+0x28))
        body=q(pawn+0x2a68)+0x10
        count=(q(pawn+0xb0)>>32)&0x7fffffff
        if count>256:raise RuntimeError('Invalid aspect count')
        aspects=[]
        for n in range(count):
            pair=r.read(q(pawn+0xa8)+n*16,16)
            key,pointer=struct.unpack('<QQ',pair)
            aspects.append({'key':hex(key),'pointer':hex(pointer),'vtable_rva':hex(q(pointer)-r.base) if pointer else None})
        result={'entity':hex(entity),'pawn':hex(pawn),'equipment':hex(equipment),
                'equipped_id':hex(q(equipment+0xa8)),
                'equipped_getter_rva':hex(q(q(equipment)+0x2c8)-r.base),
                'position':vec(entity+0x60),'body':hex(body),
                'body_indices':list(struct.unpack('<16i',r.read(body+0x20,64))),
                'animation_component':hex(q(body+0x10)),
                'animation_getter_rva':hex(q(q(q(body+0x10))+0x130)-r.base),
                'aspects':aspects}
        skeleton=q(body+0x10)+0x120
        result['skeleton']=hex(skeleton)
        result['bone_count']=(q(skeleton+0x28)>>32)&0x7fffffff
        result['bone_buffer']=hex(q(skeleton+0xa8))
        result['body_samples']=[]
        for index in result['body_indices'][:6]:
            if 0<=index<result['bone_count']:
                result['body_samples'].append({'index':index,'flag':r.read(q(skeleton+0xb0)+index,1)[0],
                    'local':vec(q(skeleton+0xa8)+index*32+16)})
        (ROOT/'research/player-snapshot.json').write_text(json.dumps(result,indent=2))
        print(json.dumps(result,indent=2))
    finally:r.close()
if __name__=='__main__':main()
