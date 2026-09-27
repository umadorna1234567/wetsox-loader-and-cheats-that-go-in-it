#include "fc5/runtime.hpp"
#include "physics_thread.hpp"
#include "magazine.hpp"
#include "camera.hpp"
#include "aim_locations.hpp"
#include <MinHook.h>
#include <bcrypt.h>
#include <intrin.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <mutex>
#include <unordered_set>
#include <numbers>

// Far Cry 4 runtime. Shared ABI names are internal; exports and sessions are FC4-specific.
namespace fc5::runtime {
namespace {
std::uintptr_t base{};
std::mutex mutex;
std::atomic<bool> enabled{};
std::atomic<bool> preserveMagazine{};
std::atomic<bool> unlimitedReserve{};
std::atomic<std::uintptr_t> localInventory{};
std::atomic<std::uint64_t> localIdentity{};
std::atomic<std::uint64_t> stickyIdentity{};
fc4::MagazineState magazineState;
struct PouchOverride {std::uintptr_t pointer{},owner{};std::uint64_t id{};unsigned char before{};};
std::vector<PouchOverride> pouchOverrides;
std::mutex ammoMutex;
struct FovOverride {std::uintptr_t entity{},pawn{},camera{};std::uint64_t id{};std::array<unsigned char,24> original{};float radians{};};
FovOverride fovOverride;
std::mutex fovMutex;
Status state;
std::array<float,16> matrix{};
std::vector<Pawn> current;
ULONGLONG lastRefresh{};
ULONGLONG lastSnapshot{};
bool read(std::uintptr_t address,void* output,std::size_t size) {
    SIZE_T copied{};
    return address>0x10000&&ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(address),output,size,&copied)&&copied==size;
}
bool write(std::uintptr_t address,const void* input,std::size_t size) {
    SIZE_T copied{};
    return address>0x10000&&WriteProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(address),input,size,&copied)&&copied==size;
}
template<typename T> bool read(std::uintptr_t address,T& output){return read(address,&output,sizeof(output));}
std::uintptr_t q(std::uintptr_t address){std::uintptr_t value{};read(address,value);return value;}
std::uintptr_t typedComponent(std::uintptr_t entity,std::uint32_t id) {
    const auto descriptor=q(entity+0x68),pairs=q(descriptor+0x58);
    unsigned count{},components{};
    if(!descriptor||!read(descriptor+0x60,count)||count>512||!read(entity+0xa0,components)||components>256)return 0;
    struct Pair {unsigned type,index;};
    for(unsigned i=0;i<count;++i) {Pair pair{};if(!read(pairs+i*8,pair))return 0;
        if(pair.type==id&&pair.index<components)return q(q(entity+0x98)+pair.index*8);
    }
    return 0;
}
std::uintptr_t component(std::uintptr_t entity,int field) {
    switch(field) {
    case 0x20:return typedComponent(entity,0x911dd85f);
    case 0x2c:return typedComponent(entity,0x035982c6);
    case 0x44:return typedComponent(entity,0xc24df18d);
    case 0x10:return typedComponent(entity,0x37d2b3e9);
    default:return 0;
    }
}
float health(std::uintptr_t entity) {
    auto counters=typedComponent(entity,0x85615a15);float result=-1;
    if(counters)read(q(counters+0x40)+0x18,result);
    return std::isfinite(result)?result:-1;
}
std::uintptr_t localEntity() {
    auto manager=q(base+0x2e24c58),controller=q(q(manager+8));
    return q(q(q(controller+8)+0x18)+0x10);
}
std::uintptr_t weaponFor(std::uintptr_t entity) {
    auto pawn=component(entity,0x20),data=q(pawn+0x70);
    return component(q(q(data+0x1258)+0x10),0x10);
}
std::uintptr_t vehicleFor(std::uintptr_t pawn) {
    auto table=q(pawn+0x70)+0x800;unsigned count{};
    if(!read(table+0x18,count)||!count||count>4096)return 0;
    auto node=q(q(table+0x20)+8*((std::uint64_t(0xfd1ba782)^0xdeadbeef)%count));
    for(unsigned i=0;node&&i<128;++i,node=q(node)) {
        unsigned key{};if(!read(node+8,key))return 0;
        if(key==0xfd1ba782)return q(q(node+0x10)+0x10); // Vehicle entity ID; -1 means on foot.
    }
    return 0;
}
void readBones(std::uintptr_t object,Pawn& pawn,unsigned requestedBones=20) {
    auto graphic=component(object,0x2c),buffer=q(graphic+0xb0);unsigned count{};
    std::array<float,16> world{};
    if(!graphic||!read(graphic+0xb8,count)||!count||count>2048||!read(graphic+0x120,world))return;
    std::vector<unsigned char> entries(count*0x70);
    if(!read(buffer,entries.data(),entries.size()))return;
    constexpr std::uint32_t names[]{0x7c159a2,0x1630abf4,0x530ec1cb,0xded10611,0x8023796d,0x8f39fa4e,0x2d4660a8,0xeb830ada,0x89b93a80,0xb675f36c,0xf60647e5,0x6bb3f727,0x7257a1aa,0x75f94d30,0x176183f0,0x60df401,0x58988870,0x757f1291,0x863d09fc,0x9b14362c};
    for(unsigned n=0;n<std::min<unsigned>(requestedBones,std::size(names));++n)for(unsigned i=0;i<count;++i) {
        std::uint32_t hash{};std::memcpy(&hash,entries.data()+i*0x70,4);if(hash!=names[n])continue;
        std::array<float,3> p{};std::memcpy(p.data(),entries.data()+i*0x70+0x60,12);bool valid=true;
        for(int j=0;j<3;++j){pawn.bones[n][j]=p[0]*world[j]+p[1]*world[j+4]+p[2]*world[j+8]+world[j+12];
            valid=valid&&std::isfinite(pawn.bones[n][j])&&std::abs(pawn.bones[n][j]-pawn.position[j])<20;}
        if(valid)pawn.boneMask|=1u<<n;break;
    }
    if(pawn.animal&&requestedBones==20) {
        std::vector<std::array<float,3>> positions(count);
        std::vector<bool> valid(count);
        for(unsigned i=0;i<count;++i) {
            std::array<float,3> p{};std::memcpy(p.data(),entries.data()+i*0x70+0x60,12);
            valid[i]=true;
            for(unsigned j=0;j<3;++j) {
                positions[i][j]=p[0]*world[j]+p[1]*world[j+4]+p[2]*world[j+8]+world[j+12];
                valid[i]=valid[i]&&std::isfinite(positions[i][j])&&std::abs(positions[i][j]-pawn.position[j])<20;
            }
            if(valid[i])pawn.boundsPoints.push_back(positions[i]);
        }
        // The bird snapshot verifies parent index +4, independently of names.
        for(unsigned i=0;i<count;++i) {
            int parent{};std::memcpy(&parent,entries.data()+i*0x70+4,4);
            if(parent>=0&&unsigned(parent)<count&&unsigned(parent)!=i&&valid[i]&&valid[parent])
                pawn.skeleton.push_back({positions[parent],positions[i]});
        }
    }
    if(q(graphic+0xb0)!=buffer){pawn.boneMask=0;pawn.boundsPoints.clear();pawn.skeleton.clear();}
}
bool fingerprint(HMODULE engine) {
    wchar_t filename[32768]{};
    if(!GetModuleFileNameW(engine,filename,32768))return false;
    std::ifstream input(std::filesystem::path(filename),std::ios::binary);
    if(!input)return false;
    BCRYPT_ALG_HANDLE algorithm{};BCRYPT_HASH_HANDLE hash{};
    if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)return false;
    bool ok=BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)>=0;
    std::array<char,65536> buffer{};
    while(ok&&input) {
        input.read(buffer.data(),buffer.size());auto count=input.gcount();
        if(count)ok=BCryptHashData(hash,reinterpret_cast<PUCHAR>(buffer.data()),static_cast<ULONG>(count),0)>=0;
    }
    std::array<unsigned char,32> digest{};
    ok=ok&&!input.bad()&&BCryptFinishHash(hash,digest.data(),static_cast<ULONG>(digest.size()),0)>=0;
    if(hash)BCryptDestroyHash(hash);BCryptCloseAlgorithmProvider(algorithm,0);
    constexpr unsigned char expected[]{0x73,0x04,0x07,0x8f,0xb5,0xbf,0xec,0x14,0x9c,0x93,0x80,0x4f,0x84,0x22,0x95,0xfa,0x8a,0x6f,0x99,0x13,0xe6,0xb2,0x80,0x52,0x3c,0x74,0x38,0x9a,0x42,0x72,0x80,0x3a};
    return ok&&std::memcmp(expected,digest.data(),32)==0;
}
std::array<float,4> transform(const std::array<float,16>& m,const float* p) {
    std::array<float,4> clip{};
    for(int j=0;j<4;++j)clip[j]=p[0]*m[j]+p[1]*m[j+4]+p[2]*m[j+8]+m[j+12];
    return clip;
}
void sampleCamera() {
    const auto manager=q(base+0x2e44a88);unsigned count{};
    if(!manager||!read(manager+0x140,count)||count!=1)return;
    const auto camera=q(q(q(manager+0x138)+8)+0x60);
    std::array<float,16> candidate{};
    if(!camera||!read(camera+0x170,candidate))return;
    for(auto n:candidate)if(!std::isfinite(n))return;
    const auto pawn=component(localEntity(),0x20),data=q(pawn+0x70);
    std::array<float,3> eye{},angles{};
    if(!data||!read(data+0x1d0,eye)||!read(data+0x68c,angles))return;
    const double sx=std::sin(angles[0]),cx=std::cos(angles[0]),sy=std::sin(angles[1]),cy=std::cos(angles[1]),sz=std::sin(angles[2]),cz=std::cos(angles[2]);
    float ahead[]{float(eye[0]+50*(-sz*cx+cz*sy*sx)),float(eye[1]+50*(cz*cx+sz*sy*sx)),float(eye[2]+50*cy*sx)};
    auto clip=transform(candidate,ahead);
    float error=clip[3]>.01f?std::hypot(clip[0]/clip[3],clip[1]/clip[3]):100.f;
    std::lock_guard lock(mutex);++state.calls;state.lastCamera=camera;state.centerError=error;
    // Render projection remains valid through damage, climbing, sliding and
    // knockdown even when the animated camera disagrees with the look angles.
    // Validate a local perspective camera by position, not aim alignment.
    auto pose=fc4::cameraPose(candidate);
    if(!pose||length(pose->eye-Vec3{eye[0],eye[1],eye[2]})>20) {++state.mismatches;return;}
    matrix=candidate;++state.projections;state.lastProjectionMs=GetTickCount64();
}
}
bool start() {
    auto module=GetModuleHandleW(L"FC64.dll");
    if(!module){state.aimState=101;return false;}
    if(!fingerprint(module)){state.aimState=102;state.lastCaller=GetLastError();return false;}
    base=reinterpret_cast<std::uintptr_t>(module);enabled=true;
    std::lock_guard lock(mutex);state.supported=1;state.magazineHook=1;state.unlimitedHook=1;return true;
}
void stop() {
    enabled=false;cameraFov(false,90,false,90);setNoReload(false);setUnlimitedAmmo(false);localIdentity=0;
    std::lock_guard lock(mutex);current.clear();state.lastProjectionMs=0;
}
void refresh() {
    if(enabled)sampleCamera();
    if(!enabled||GetTickCount64()-lastRefresh<50)return;
    lastRefresh=GetTickCount64();
    std::vector<Pawn> found;
    auto entity=localEntity(),localId=q(entity+8);std::array<float,3> local{};
    if(!entity||health(entity)<=0||!read(entity+0x50,local)) {
        localIdentity=0;stickyIdentity=0;
        std::lock_guard lock(mutex);current.clear();state.lastProjectionMs=0;return;
    }
    localIdentity=localId;
    auto table=q(base+0x2dd48f8),buckets=q(table+0x10);unsigned count{};
    if(!read(table+8,count)||!buckets||!count||count>65536)return;
    std::vector<std::uintptr_t> heads(count);
    if(!read(buckets,heads.data(),heads.size()*sizeof(std::uintptr_t)))return;
    std::unordered_set<std::uintptr_t> visited;
    for(auto node:heads)while(node&&visited.size()<50000&&visited.insert(node).second) {
        std::array<std::uintptr_t,3> entry{};if(!read(node,entry))break;
        node=entry[0];auto object=q(entry[2]+16);if(entry[1]==localId)continue;
        // FC4 wildlife uses CAnimalAgentFC3 on ordinary CEntity instances.
        bool animal=typedComponent(object,0xafec1e66)!=0;
        bool human=!animal&&q(object)==base+0x280d7f0;
        if(!human&&!animal)continue;
        Pawn pawn{entry[1],{}};pawn.object=object;pawn.health=health(object);pawn.animal=animal;
        if(human){auto agent=component(object,0x44);if(q(agent+0x2b0)==base+0x28c8d90)read(agent+0x2b8,pawn.faction);}
        if(!read(object+0x50,pawn.position))continue;
        bool valid=true;for(float x:pawn.position)valid=valid&&std::isfinite(x)&&std::abs(x)<1e7;
        if(valid)found.push_back(pawn);
    }
    std::lock_guard lock(mutex);
    const double elapsed=(GetTickCount64()-lastSnapshot)*.001;
    if(lastSnapshot&&elapsed>=.02&&elapsed<=.35)for(auto& next:found) {
        auto old=std::find_if(current.begin(),current.end(),[&](const Pawn& p){return p.id==next.id&&p.object==next.object;});
        if(old==current.end())continue;
        auto velocity=motionVelocity({old->position[0],old->position[1],old->position[2]}, {next.position[0],next.position[1],next.position[2]},elapsed);
        next.velocityValid=velocity.has_value();
        next.velocity=velocity.value_or(Vec3{});
    }
    current=std::move(found);state.localPosition=local;
    lastSnapshot=GetTickCount64();
    state.animalCount=static_cast<std::uint32_t>(std::count_if(current.begin(),current.end(),[](const Pawn& p){return p.animal;}));
    state.pawnCount=static_cast<std::uint32_t>(current.size());++state.snapshots;
}
Status status(){std::lock_guard lock(mutex);auto output=state;output.blockedInputPolls=0;output.menuCapturing=0;return output;}
void setNoReload(bool value) {
    std::lock_guard guard(ammoMutex);
    auto entity=localEntity();
    if(!enabled||!entity||q(entity+8)!=localIdentity||health(entity)<=0) {magazineState.reset();return;}
    auto weapon=weaponFor(entity),data=q(weapon+0xa0),owner=q(q(weapon+8)+0x10);
    fc4::WeaponIdentity identity{entity,owner,weapon,data,q(entity+8),q(owner+8)};
    int count{},ammoPerShot{};
    // +1f8 is a stat lookup key, not a magazine capacity. +200 is the
    // ammo-per-shot field consumed by the native bullet firing path.
    auto config=q(data+0x68);
    if(!weapon||!data||!owner||component(owner,0x10)!=weapon||q(data+0x60)!=weapon||
       !read(config+0x200,ammoPerShot)||ammoPerShot<=0||ammoPerShot>1000||
       !read(data+0x104,count)) {magazineState.reset();return;}
    auto desired=magazineState.update(value,true,identity,count);
    if(!desired)return;
    // Re-resolve the equipped object immediately before bounded writes. Never
    // call native inventory/perk/strategy functions from the render thread.
    int latest{};
    if(localEntity()!=entity||q(entity+8)!=identity.playerId||health(entity)<=0||
       weaponFor(entity)!=weapon||q(owner+8)!=identity.weaponId||
       q(q(weapon+8)+0x10)!=owner||q(weapon+0xa0)!=data||q(data+0x68)!=config||
       !read(data+0x104,latest)||latest!=count) {magazineState.reset();return;}
    if(write(data+0x104,&*desired,sizeof(*desired))) {
        const unsigned char dirty=1;write(weapon+0x9c,&dirty,1);
        std::lock_guard lock(mutex);++state.magazineCalls;
        if(*desired>count)state.preservedRounds+=*desired-count;
        state.magazine=*desired;
    }
}
void setUnlimitedAmmo(bool value) {
    std::lock_guard guard(ammoMutex);
    auto entity=localEntity(),id=q(entity+8),pawn=component(entity,0x20),data=q(pawn+0x70);
    if(!entity||id!=localIdentity||health(entity)<=0) {pouchOverrides.clear();return;}
    const auto inventory=data+0x1198;unsigned count{};std::vector<std::uintptr_t> pouches;
    if(data&&read(inventory+8,count)&&count<=512)for(unsigned i=0;i<count;++i) {
        auto item=q(q(inventory)+8*i);if(q(item)==base+0x27e4ac8)pouches.push_back(item);
    }
    for(auto it=pouchOverrides.begin();it!=pouchOverrides.end();) {
        bool same=it->owner==entity&&it->id==id&&std::find(pouches.begin(),pouches.end(),it->pointer)!=pouches.end();
        if(!value||!enabled||!same) {
            unsigned char current{};
            if(same&&read(it->pointer+0x38,current)&&current==1)write(it->pointer+0x38,&it->before,1);
            it=pouchOverrides.erase(it);
        } else ++it;
    }
    if(!enabled||!value)return;
    for(auto ptr:pouches) {
        if(localEntity()!=entity||q(entity+8)!=id||q(pawn+0x70)!=data||health(entity)<=0) {
            pouchOverrides.clear();return;
        }
        if(std::any_of(pouchOverrides.begin(),pouchOverrides.end(),[&](const auto& p){return p.pointer==ptr;}))continue;
        unsigned char old{},one=1;
        if(read(ptr+0x38,old)&&old<=1&&write(ptr+0x38,&one,1)) {
            pouchOverrides.push_back({ptr,entity,id,old});std::lock_guard lock(mutex);++state.unlimitedQueries;
        }
    }
}
void cameraFov(bool playerRequested,double playerDegrees,bool vehicleRequested,double vehicleDegrees) {
    std::lock_guard guard(fovMutex);
    // Serialize render updates with shutdown. Only the local camera's native
    // FOV override fields are owned, never shared descriptor data.
    if(!enabled&&!fovOverride.camera)return;
    auto entity=localEntity(),pawn=component(entity,0x20),vehicleId=vehicleFor(pawn);
    if(!entity||q(entity+8)!=localIdentity||health(entity)<=0) {
        fovOverride={};std::lock_guard lock(mutex);state.playerFovState=state.vehicleFovState=0;return;
    }
    auto camera=q(pawn+0x70)+0x1e0;
    if(fovOverride.camera&&(fovOverride.entity!=entity||fovOverride.id!=q(entity+8)||
       fovOverride.pawn!=pawn||fovOverride.camera!=camera))fovOverride={};
    const bool riding=vehicleId&&vehicleId!=~std::uint64_t{};
    const bool requested=riding?vehicleRequested:playerRequested;
    const double degrees=riding?vehicleDegrees:playerDegrees;
    auto report=[&](unsigned value) { std::lock_guard lock(mutex); state.vehicleFovState=riding?value:(vehicleRequested?1:0);state.playerFovState=!riding?value:(playerRequested?1:0); };
    const bool want=enabled&&requested&&pawn&&q(pawn+0x70)&&q(entity+8)==localIdentity&&std::isfinite(degrees)&&degrees>=40&&degrees<=140;
    const float radians=static_cast<float>(degrees*std::numbers::pi/180.);
    if(fovOverride.camera) {
        unsigned char active{};float value{};
        const bool same=q(fovOverride.entity+8)==fovOverride.id&&q(fovOverride.pawn+0x70)+0x1e0==fovOverride.camera;
        const bool owned=same&&read(fovOverride.camera+0x81,active)&&active==1&&read(fovOverride.camera+0x90,value)&&value==fovOverride.radians;
        if(!owned){fovOverride={};report(3);return;}
        if(!want||camera!=fovOverride.camera) {
            if(!write(fovOverride.camera+0x80,fovOverride.original.data(),fovOverride.original.size())) {
                report(4);return;
            }
            fovOverride={};
        } else if(radians!=fovOverride.radians) {
            if(write(camera+0x90,&radians,sizeof(radians)))fovOverride.radians=radians;
        }
    }
    if(want&&!fovOverride.camera) {
        FovOverride next{entity,pawn,camera,q(entity+8),{},radians};
        if(!read(camera+0x80,next.original)||next.original[1]) {
            report(3);return;
        }
        // Equivalent to the native zero-blend setter when no prior override
        // is active: enable, zero blend/elapsed fields, target radians.
        auto fields=next.original;fields[1]=1;
        std::fill(fields.begin()+4,fields.begin()+16,0);
        std::memcpy(fields.data()+16,&radians,sizeof(radians));
        if(!write(camera+0x80,fields.data(),fields.size())) {report(4);return;}
        fovOverride=next;
    }
    report(fovOverride.camera?2:requested?1:0);
    std::lock_guard lock(mutex);
    state.vehicleFovDegrees=riding&&fovOverride.camera?fovOverride.radians*180.f/static_cast<float>(std::numbers::pi):0;
    state.playerFovDegrees=!riding&&fovOverride.camera?fovOverride.radians*180.f/static_cast<float>(std::numbers::pi):0;
}
bool hitscanWeapon(std::uintptr_t pawn) {
    auto data=q(pawn+0x70),weapon=component(q(q(data+0x1258)+0x10),0x10);
    auto weaponState=q(weapon+0xa0),strategy=q(weaponState+0x120),vtable=q(strategy);
    // Verified AK-47 CWeaponFireBulletStrategy: daeb90 computes a complete
    // ray (direction times range) and submits it to 4af770. No flight model.
    return weapon&&q(weaponState+0x60)==weapon&&vtable==base+0x28037e0&&
           q(vtable+0x1d8)==base+0xd9fd40;
}
fc4::PhysicsThreadBindings physicsThreadBindings() {
    fc4::PhysicsThreadBindings bindings;
    bindings.routerSlot=bindings.monitorSlot=TLS_OUT_OF_INDEXES;
    if(!read(base+0x32b77e8,bindings.routerSlot)||!read(base+0x32c1b68,bindings.monitorSlot))return {};
    auto system=q(base+0x32c1b60),vtable=q(system);
    if(!system||!vtable)return bindings;
    bindings.memorySystem=reinterpret_cast<void*>(system);
    bindings.construct=reinterpret_cast<decltype(bindings.construct)>(base+0x1dfbea0);
    bindings.destroy=reinterpret_cast<decltype(bindings.destroy)>(base+0x1dfdf70);
    bindings.memoryInit=reinterpret_cast<decltype(bindings.memoryInit)>(q(vtable+0x18));
    bindings.memoryQuit=reinterpret_cast<decltype(bindings.memoryQuit)>(q(vtable+0x20));
    bindings.threadInit=reinterpret_cast<decltype(bindings.threadInit)>(base+0x1e02190);
    bindings.threadQuit=reinterpret_cast<decltype(bindings.threadQuit)>(base+0x1e021e0);
    return bindings;
}
// Use the native bullet collision filter, with the verified synchronous
// query flags and array lifetime. The generic interaction-ray mask includes
// collision categories which the AK bullet ray deliberately excludes.
// The second vector is a displacement, not an endpoint. A clear result
// permits identified vehicles, including when a seated occupant has no
// independent hit. Walls and unresolved colliders still block the segment.
bool clearLine(Vec3 origin,Vec3 endpoint,std::uint64_t targetId,std::uint64_t ignoredVehicle=0) {
    struct NativeArray {std::uintptr_t data{};std::uint64_t sizes{};};
    struct NativeFilter {std::uintptr_t vtable{};std::uint32_t mask{},group{};};
    using Construct=void*(__fastcall*)(NativeFilter*,unsigned,unsigned);
    using Query=void(__fastcall*)(std::uintptr_t,const float*,const float*,NativeArray*,NativeFilter*,NativeArray*,unsigned);
    using Destroy=void(__fastcall*)(void*);
    using Resolve=std::uintptr_t(__fastcall*)(std::uintptr_t,unsigned);
    auto world=q(base+0x2defc38);
    if(!world||!q(world+0xf0)||!finite(origin)||!finite(endpoint))return false;
    const auto delta=endpoint-origin;
    if(length(delta)<.01||length(delta)>2000)return false;
    float start[]{float(origin.x),float(origin.y),float(origin.z)};
    float displacement[]{float(delta.x),float(delta.y),float(delta.z)};
    NativeArray hits{},auxiliary{};NativeFilter filter{};
    // Native AK firing path: FC64+daed11 -> daed39.
    reinterpret_cast<Construct>(base+0x4593c0)(&filter,0x1fbb,0);
    // Bit 0 selects the coarse collector (4a8110 -> 483a10), which
    // reports foliage proxy boxes as blockers. Use the detailed collector
    // (4a7a80) and retain bit 1 for additional geometry and bit 2 for bounds.
    // Live comparison: flags 5 hit an entity-less proxy; flags 6 hit the
    // target behind its branches and still reported terrain on a blocked ray.
    reinterpret_cast<Query>(base+0x4af730)(world,start,displacement,&hits,&filter,&auxiliary,6);
    reinterpret_cast<Destroy>(base+0x4535b0)(&filter);
    auto count=hits.sizes&0xffffffff;
    bool found=false,blocked=count>4096||(count&&!hits.data);
    std::vector<CoverHit> coverHits;
    std::uint64_t blocker{};
    if(!blocked)for(std::uint64_t i=0;i<count;++i) {
        unsigned collider{};
        if(!read(hits.data+52*i+0x18,collider)){blocked=true;break;}
        auto object=reinterpret_cast<Resolve>(base+0x4b5310)(world,collider);
        unsigned kind{};read(object+0x18,kind);auto id=object&&kind==1?q(object+8):0;
        if(id==targetId){found=true;coverHits.push_back(CoverHit::Target);}
        else if(id&&id==localIdentity.load())coverHits.push_back(CoverHit::LocalPlayer);
        else {
            auto entity=std::uintptr_t{}; // Vehicle cover resolved by the target relation below.
            const bool vehicle=id&&((ignoredVehicle&&id==ignoredVehicle)||
                (entity&&q(entity+8)==id&&typedComponent(entity,0x7efd7da9)));
            if(vehicle)coverHits.push_back(CoverHit::Vehicle);
            else {coverHits.push_back(id?CoverHit::Obstruction:CoverHit::Unknown);blocker=id;blocked=true;break;}
        }
    }
    reinterpret_cast<Destroy>(base+0x459e00)(&hits);
    reinterpret_cast<Destroy>(base+0x1cad60)(&auxiliary);
    const bool clear=!blocked&&clearVehicleCover(coverHits);
    std::lock_guard lock(mutex);++state.visibilityQueries;
    if(blocked){++state.coverObstructions;state.lastBlocker=blocker;state.lastBlockedTarget=targetId;}
    else if(!clear&&!found)++state.coverMissingTarget;
    if(clear)++state.visibilityClear;else ++state.visibilityBlocked;
    return clear;
}
void aim(const AimSettings& settings,bool held,double dt) {
    auto report=[&](unsigned reason,std::uint64_t target=0,float error=0) {
        if(reason==5)stickyIdentity=target;
        else if(reason==0||reason==3||reason==4)stickyIdentity=0;
        std::lock_guard lock(mutex);state.aimState=reason;state.aimTarget=target;state.centerError=error;
        if(reason<state.aimReasons.size())++state.aimReasons[reason];
        if(reason==6)state.lastAlignmentFailure=error;
    };
    if(!enabled||!settings.enabled||!held){report(0);return;}
    auto entity=localEntity(),pawn=component(entity,0x20),look=q(pawn+0x70)+0x600;
    if(!pawn||!q(pawn+0x70)||q(entity+8)!=localIdentity||health(entity)<=0){report(2);return;}
    auto vehicleId=vehicleFor(pawn);const bool riding=vehicleId&&vehicleId!=~std::uint64_t{};
    std::array<float,16> root{},projection{};std::array<float,3> angles{},eye{};
    root={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    if(!read(look+0x8c,angles)||!read(q(pawn+0x70)+0x1d0,eye)){report(2);return;}
    {
        std::lock_guard lock(mutex);
        if(!state.lastProjectionMs||GetTickCount64()-state.lastProjectionMs>250){state.aimState=2;return;}
        projection=matrix;
    }
    const double cx=std::cos(angles[0]),sx=std::sin(angles[0]),cy=std::cos(angles[1]),sy=std::sin(angles[1]),cz=std::cos(angles[2]),sz=std::sin(angles[2]);
    Vec3 local{-sz*cx+cz*sy*sx,cz*cx+sz*sy*sx,cy*sx};
    auto world=[&](Vec3 v){return Vec3{v.x*root[0]+v.y*root[4]+v.z*root[8],v.x*root[1]+v.y*root[5]+v.z*root[9],v.x*root[2]+v.y*root[6]+v.z*root[10]};};
    Vec3 view=world(local),origin{eye[0],eye[1],eye[2]};
    if(!finite(view)||!finite(origin)||std::abs(length(view)-1)>.05){report(2);return;}
    auto ahead=origin+view*50;float point[]{static_cast<float>(ahead.x),static_cast<float>(ahead.y),static_cast<float>(ahead.z)};
    auto clip=transform(projection,point);
    float error=clip[3]>.01f?std::hypot(clip[0]/clip[3],clip[1]/clip[3]):100.f;
    if(!std::isfinite(error)||error>.05f){report(riding?3:6,0,error);return;}
    auto rendered=fc4::cameraPose(projection);
    if(!rendered||length(rendered->eye-origin)>3) {report(6,0,error);return;}
    const auto baseLook=view;
    view=rendered->forward;origin=rendered->eye;
    Shot shot;shot.muzzle=origin;
    AimSettings effective=settings;
    // AK-47 bullets are hitscan. Time-of-flight, motion lead and gravity
    // corrections must be zero for this verified weapon model. Unverified
    // physical projectiles remain a reported direct-aim fallback.
    const bool hitscan=hitscanWeapon(pawn);
    const bool directFallback=settings.travelTime&&!hitscan;
    effective=weaponAimSettings(settings,false);
    std::vector<Target> targets;
    for(auto candidate:pawns()) {
        if(q(candidate.object+8)!=candidate.id||health(candidate.object)<=0)continue;
        if(!read(candidate.object+0x50,candidate.position))continue;
        candidate.boneMask=0;readBones(candidate.object,candidate,4);
        Target t;t.id=candidate.id;t.kind=candidate.animal?Kind::Animal:(candidate.faction==0||candidate.faction==1?Kind::Enemy:Kind::OtherHuman);
        t.velocity=candidate.velocityValid?candidate.velocity:Vec3{};
        if(!candidate.animal) {
            auto id=vehicleFor(component(candidate.object,0x20));if(id&&id!=~std::uint64_t{})t.vehicleId=id;
        }
        const auto locations=fc4::aimLocations(candidate.bones,candidate.boneMask,candidate.animal);
        for(unsigned i=0;i<4;++i)if(locations[i]) {
            auto p=*locations[i];float x{},y{};
            if(project(p,2,2,x,y))t.hitLocations[i]=Vec3{p[0],p[1],p[2]};
        }
        targets.push_back(t);
    }
    auto selected=selectTarget(targets,view,shot,effective,false,stickyIdentity);
    if(!selected){report(4,0,error);return;}
    // Present is not a Havok worker. The crash dump showed both native TLS
    // slots empty and a null monitor dereference in FC64+1e958ac.
    fc4::PhysicsThreadScope physicsThread(physicsThreadBindings());
    if(!physicsThread){report(7,0,error);return;}
    // Recheck every frame, even while holding the same target. Bound native
    // queries per frame; an untested candidate cannot receive an aim write.
    bool clear=false;
    for(unsigned attempts=0;selected&&attempts<8;++attempts) {
        auto candidate=std::find_if(targets.begin(),targets.end(),[&](const Target& t){return t.id==selected->id;});
        if(candidate==targets.end())break;
        auto endpoint=candidate->hitLocations[static_cast<unsigned>(settings.hitLocation)];
        const auto ignoreVehicle=candidate->vehicleId;
        if(endpoint&&clearLine(origin,*endpoint,selected->id,ignoreVehicle)){clear=true;break;}
        candidate->visible=false;
        selected=selectTarget(targets,view,shot,effective,false,stickyIdentity);
    }
    if(!clear||!selected){report(7,0,error);return;}
    auto direction=smoothDirection(view,selected->shot.direction,std::clamp(dt,0.,.1),settings.smoothingSeconds);
    // Bound each correction even with zero smoothing; input/animation races
    // must not turn a single frame into an arbitrarily large camera snap.
    const double angle=std::acos(std::clamp(dot(view,direction),-1.,1.));
    const double maxStep=4*std::numbers::pi*std::clamp(dt,0.,1./30);
    if(angle>maxStep&&maxStep>0)direction=smoothDirection(view,direction,1.,-1./std::log1p(-maxStep/angle));
    std::array<float,16> latestRoot{};std::array<float,3> latestAngles{},latestEye{};
    latestRoot=root;
    if(!read(look+0x8c,latestAngles)||!read(q(pawn+0x70)+0x1d0,latestEye))return;
    // The engine may update strafe/body orientation during the ray queries.
    // Recompute on the next frame instead of applying an angle from an old basis.
    for(unsigned i=0;i<12;++i)if(std::abs(latestRoot[i]-root[i])>.0001f)return;
    for(unsigned i=0;i<3;++i)if(std::abs(latestAngles[i]-angles[i])>.0001f||std::abs(latestEye[i]-eye[i])>.001f)return;
    auto corrected=fc4::compensateCamera(baseLook,view,direction);
    if(!corrected){report(6,0,error);return;}
    auto converted=lookAngles(*corrected,latestRoot,latestAngles[1]);
    if(!converted){report(6,0,error);return;}
    auto desired=*converted;
    if(localEntity()!=entity||q(entity+8)!=localIdentity||component(entity,0x20)!=pawn||
       q(pawn+0x70)+0x600!=look||health(entity)<=0) {report(2);return;}
    SIZE_T written{};
    if(!WriteProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(look+0x8c),desired.data(),sizeof(desired),&written)||written!=sizeof(desired)){report(2);return;}
    if(!write(q(pawn+0x70)+0x48c,desired.data(),sizeof(desired))){report(2);return;}
    report(5,selected->id,error);std::lock_guard lock(mutex);if(directFallback)state.aimState=1;++state.aimWrites;state.flightSeconds=float(selected->shot.flightSeconds);
}
std::vector<Pawn> pawns(){std::lock_guard lock(mutex);if(!enabled||GetTickCount64()-lastSnapshot>1000)return {};return current;}
std::vector<Pawn> visualPawns(const Filters& filters) {
    // Only discovery and velocity estimation use the 50 ms snapshot. Sample
    // selected entities' transforms now, once for this rendered frame.
    auto tracked=pawns();std::vector<Pawn> posed;posed.reserve(tracked.size());
    for(auto& pawn:tracked) {
        const auto kind=pawn.animal?Kind::Animal:(pawn.faction==0||pawn.faction==1?Kind::Enemy:Kind::OtherHuman);
        if(!filters.accepts(kind)||q(pawn.object+8)!=pawn.id)continue;
        pawn.health=health(pawn.object);
        if(pawn.health<=0||!read(pawn.object+0x50,pawn.position))continue;
        bool valid=true;
        for(float value:pawn.position)valid=valid&&std::isfinite(value)&&std::abs(value)<1e7;
        if(!valid)continue;
        pawn.boneMask=0;readBones(pawn.object,pawn);
        // Discard destroyed/recycled entities and incoherent animation reads.
        if((pawn.boneMask||!pawn.boundsPoints.empty())&&q(pawn.object+8)==pawn.id)posed.push_back(pawn);
    }
    {std::lock_guard lock(mutex);state.boneEntities=static_cast<std::uint32_t>(posed.size());}
    return posed;
}
bool aimFovRadii(double diameter,float width,float height,float& x,float& y) {
    std::lock_guard lock(mutex);
    if(!enabled||!state.lastProjectionMs||GetTickCount64()-state.lastProjectionMs>250)return false;
    auto radii=coneRadii(matrix,diameter,width,height);if(!radii)return false;
    x=(*radii)[0];y=(*radii)[1];return true;
}
bool project(std::array<float,3> point,float width,float height,float& x,float& y) {
    std::lock_guard lock(mutex);
    if(!enabled||!state.lastProjectionMs||GetTickCount64()-state.lastProjectionMs>1000)return false;
    auto clip=transform(matrix,point.data());
    if(!std::isfinite(clip[3])||clip[3]<.01f)return false;
    float nx=clip[0]/clip[3],ny=clip[1]/clip[3];
    if(!std::isfinite(nx)||!std::isfinite(ny)||std::abs(nx)>1||std::abs(ny)>1)return false;
    x=(nx+1)*.5f*width;y=(1-ny)*.5f*height;return true;
}
}
extern "C" __declspec(dllexport) DWORD WINAPI FC5RuntimeStatus(fc5::runtime::Status* output) {
    if(!output||output->size!=sizeof(*output))return ERROR_INVALID_PARAMETER;
    *output=fc5::runtime::status();return 0;
}
