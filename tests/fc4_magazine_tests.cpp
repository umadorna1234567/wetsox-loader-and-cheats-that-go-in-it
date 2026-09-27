#include "magazine.hpp"
#include <cstdio>

int main() {
    fc4::MagazineState state;
    fc4::WeaponIdentity gun{1,2,3,4,5,6},other{1,7,8,9,5,10};
    bool ok=true;
    auto check=[&](bool result,const char* message){if(!result){std::fprintf(stderr,"%s\n",message);ok=false;}};
    check(!state.update(true,true,gun,14),"Initial magazine should remain unchanged");
    check(state.update(true,true,gun,13)==14,"Firing should preserve observed ammo");
    check(!state.update(true,false,gun,13),"Death must not write or restore ammo");
    check(!state.update(false,true,gun,14),"Death must discard restoration ownership");
    check(!state.update(true,true,gun,0),"An initially empty weapon must wait for reload");
    check(state.update(true,true,gun,1)==2,"Single-shot magazine needs a spare round");
    check(state.update(false,true,gun,2)==1,"Disabling should restore an owned spare round");
    state.update(true,true,gun,1);
    check(!state.update(false,true,other,30),"Switching weapons must not restore into the replacement");
    state.update(true,true,gun,1);auto respawn=gun;respawn.playerId++;
    check(!state.update(false,true,respawn,2),"Recycled player pointers must not retain ownership");
    state.update(true,true,gun,1);auto recycled=gun;recycled.weaponId++;
    check(!state.update(false,true,recycled,2),"Recycled weapon pointers must not retain ownership");
    state.update(true,true,gun,1);
    check(!state.update(false,true,gun,8),"Disabling must preserve a game-side ammo pickup");
    state.update(true,true,gun,14);
    check(!state.update(true,true,gun,-1),"Invalid magazine must reject updates");
    check(!state.update(false,true,gun,14),"Invalid magazine must clear stale ownership");
    return ok?0:1;
}
