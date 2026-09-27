#include "aim_locations.hpp"
#include "fc5/targeting.hpp"
#include <cstdio>
int main() {
    bool ok=true;
    auto check=[&](bool value,const char* message){if(!value){std::fprintf(stderr,"%s\n",message);ok=false;}};
    std::array<fc4::AimPoint,20> bones{};
    // Validated slots from the captured 12-joint bird: Head and Spine.
    bones[0]={0,10,.4f};bones[2]={0,10,.28f};
    auto bird=fc4::aimLocations(bones,0b0101,true);
    fc5::Target target;target.id=1;target.kind=fc5::Kind::Animal;
    for(unsigned i=0;i<4;++i)if(bird[i])target.hitLocations[i]=fc5::Vec3{(*bird[i])[0],(*bird[i])[1],(*bird[i])[2]};
    fc5::AimSettings settings;settings.enabled=true;settings.targets.animals=true;settings.travelTime=false;
    for(unsigned i=0;i<4;++i){
        settings.hitLocation=static_cast<fc5::HitLocation>(i);
        check(fc5::selectTarget(std::span(&target,1),{0,1,0},{},settings,false).has_value(),"All four aim settings can select a bird");
    }
    settings.targets.animals=false;
    check(!fc5::selectTarget(std::span(&target,1),{0,1,0},{},settings,false),"Animal filter remains respected");
    auto human=fc4::aimLocations(bones,0b0101,false);
    check(!human[1]&&!human[3],"Do not synthesize human joints");
    auto invalid=fc4::aimLocations(bones,0b0001,true);
    check(!invalid[1]&&!invalid[2]&&!invalid[3],"Missing/invalid body stays unavailable");
    bones[1]={1,10,1};bones[3]={2,10,2};
    auto full=fc4::aimLocations(bones,15,true);
    check(full[0]==bones[0]&&full[1]==bones[1]&&full[3]==bones[3],"Preserve actual head/chest/pelvis joints");
    check(bird[1]==bird[2]&&bird[3]==bird[2],"Missing bird body locations use Spine");
    return ok?0:1;
}
