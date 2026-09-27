"""Read-only inspection of local pawn/aspect links to owned weapon entities."""
import argparse,hashlib,json,struct
from pathlib import Path
from inspect_runtime import Reader,ROOT
from list_lua_api import EXPECTED
p=argparse.ArgumentParser();p.add_argument('--pid',type=int,required=True);a=p.parse_args()
r=Reader(a.pid)
try:
    if hashlib.sha256(Path(r.path).read_bytes()).hexdigest().upper()!=EXPECTED:raise RuntimeError('Unverified engine')
    pawn=json.loads((ROOT/'research/player-snapshot.json').read_text())
    weapons=json.loads((ROOT/'research/weapon-snapshot.json').read_text())['weapons']
    wanted={int(w[k],16):f'{k}:{w["id"]}' for w in weapons for k in ('weapon','entity','id')}
    def q(x):return struct.unpack('<Q',r.read(x,8))[0]
    blocks=[('pawn',int(pawn['pawn'],16),0x2b00),('inventory',int(pawn['equipment'],16),0x500)]
    blocks.extend((x['key'],int(x['pointer'],16),0x400) for x in pawn['aspects'])
    inventory=int(pawn['equipment'],16);pending=[q(inventory+0x1a8)];seen=set()
    while pending and len(seen)<512:
        node=pending.pop()
        if not node or node==inventory+0x198 or node in seen:continue
        seen.add(node)
        try:
            pending.extend([q(node),q(node+8)])
            blocks.append(('item:'+hex(q(node+0x20)),q(node+0x28),0x300))
        except OSError:pass
    for name,address,size in blocks:
        try:
            data=r.read(address,size)
            for off in range(0,size,8):
                value=struct.unpack_from('<Q',data,off)[0]
                if value in wanted:print(name,hex(address),hex(off),'direct',wanted[value])
                if 0x10000<value<r.base:
                    try:
                        obj=q(value+16);ident=q(value)
                        if obj in wanted and ident in wanted:print(name,hex(address),hex(off),'entityref',wanted[obj],hex(value))
                    except OSError:pass
        except OSError:pass
finally:r.close()
