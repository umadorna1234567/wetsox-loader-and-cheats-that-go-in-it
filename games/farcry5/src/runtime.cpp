#include "fc5/runtime.hpp"
#include "fc5/camera.hpp"
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
using SetMagazine=void(__fastcall*)(void*,int);
SetMagazine originalSetMagazine{};
bool magazineInstalled{};
using UnlimitedFlag=bool(__fastcall*)(void*);
UnlimitedFlag originalUnlimited{};
bool unlimitedInstalled{};
using Launch=std::uintptr_t(__fastcall*)(void*,void*,const float*,const float*,const float*);
Launch originalLaunch{};
bool launchInstalled{};
struct FovOverride {std::uintptr_t entity{},pawn{},camera{};std::uint64_t id{};std::array<unsigned char,24> original{};float radians{};};
FovOverride fovOverride;
std::mutex fovMutex;
Status state;
std::array<float,16> matrix{};
std::vector<Pawn> current;
ULONGLONG lastRefresh{};
ULONGLONG lastSnapshot{};
using Project=void*(__fastcall*)(void*,float*,const float*,void*);
Project original{};
bool installed{};
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
std::uintptr_t component(std::uintptr_t entity,int field) {
    auto descriptor=q(entity+0xc8);int slot=-1;
    return descriptor&&read(descriptor+field,slot)&&slot>=0&&slot<256?q(q(entity+0xa8)+8*slot):0;
}
std::uintptr_t typedComponent(std::uintptr_t entity,std::uint32_t id) {
    auto descriptor=q(entity+0xc8),count=(q(descriptor+0x60)>>32)&0x7fffffff;
    if(!descriptor||count>256)return 0;
    auto pairs=q(descriptor+0x58);
    for(std::uintptr_t i=0;i<count;++i) {
        std::array<std::uint64_t,2> item{};
        if(!read(pairs+16*i,item))return 0;
        if(static_cast<std::uint32_t>(item[0])==id&&item[1]<256)return q(q(entity+0xa8)+8*(item[1]+1));
    }
    return 0;
}
float health(std::uintptr_t entity) {
    auto counters=typedComponent(entity,0x85615a15);float result=-1;
    if(counters)read(q(counters+0x48)+0x18,result);
    return std::isfinite(result)?result:-1;
}
std::uintptr_t aspect(std::uintptr_t pawn,std::uint32_t id) {
    auto count=(q(pawn+0xb0)>>32)&0x7fffffff;
    if(count>128)return 0;
    for(std::uintptr_t i=0;i<count;++i) {
        std::array<std::uint64_t,2> item{};
        if(!read(q(pawn+0xa8)+16*i,item))return 0;
        if(static_cast<std::uint32_t>(item[0])==id)return item[1];
    }
    return 0;
}
bool __fastcall unlimitedHook(void* self) {
    const auto ammo=reinterpret_cast<std::uintptr_t>(self);
    // Inventory item interface is at +0x40; its owner is at item+0x20.
    if(enabled&&unlimitedReserve&&localInventory&&q(ammo)==base+0x447c720&&q(ammo-0x20)==localInventory) {
        std::lock_guard lock(mutex);++state.unlimitedQueries;return true;
    }
    return originalUnlimited(self);
}
void __fastcall magazineHook(void* self,int requested) {
    auto weapon=reinterpret_cast<std::uintptr_t>(self);
    auto owner=q(weapon+0x70);int existing=-1;
    if(enabled&&localIdentity&&q(owner)==localIdentity&&read(weapon+0x188,existing)) {
        bool kept=preserveMagazine&&existing>0&&existing<=10000&&requested==existing-1;
        if(kept)requested=existing;
        std::lock_guard lock(mutex);++state.magazineCalls;state.magazine=requested;if(kept)++state.preservedRounds;
    }
    originalSetMagazine(self,requested);
}
void readBones(std::uintptr_t object,Pawn& pawn,unsigned requestedBones=20) {
    auto graphic=component(object,0x2c);
    if(!graphic||q(q(graphic)+0x130)!=base+0xc46080)return;
    auto skeleton=graphic+0x120;
    auto count=(q(skeleton+0xe0)>>32)&0x7fffffff;
    auto boneCount=(q(skeleton+0x28)>>32)&0x7fffffff;
    if(!count||count>2048||!boneCount||boneCount>2048)return;
    struct Entry {std::uint32_t hash;std::int32_t index;};
    std::vector<Entry> entries(count);
    if(!read(q(skeleton+0xd8),entries.data(),entries.size()*sizeof(Entry)))return;
    std::array<float,16> world{};
    if(!read(graphic+0x2a0,world))return;
    auto buffer=q(skeleton+0xa8),flags=q(skeleton+0xb0);
    // CRC32 names verified against the human and cougar skeleton maps.
    constexpr std::uint32_t names[]{0x7c159a2,0x1630abf4,0x530ec1cb,0xded10611,0x8023796d,0x8f39fa4e,0x2d4660a8,0xeb830ada,0x89b93a80,0xb675f36c,0xf60647e5,0x6bb3f727,0x7257a1aa,0x75f94d30,0x176183f0,0x60df401,0x58988870,0x757f1291,0x863d09fc,0x9b14362c};
    for(unsigned n=0;n<std::min<unsigned>(requestedBones,std::size(names));++n) {
        auto entry=std::find_if(entries.begin(),entries.end(),[&](const Entry& e){return e.hash==names[n];});
        if(entry==entries.end()||entry->index<0||static_cast<unsigned>(entry->index)>=boneCount)continue;
        unsigned char before{},after{};std::array<float,3> p{};
        auto index=entry->index;
        if(!read(flags+index,before)||(before&8)||!read(buffer+32*index+16,p)||!read(flags+index,after)||(after&8)||q(skeleton+0xa8)!=buffer)continue;
        bool valid=true;
        for(int j=0;j<3;++j) {
            pawn.bones[n][j]=p[0]*world[j]+p[1]*world[j+4]+p[2]*world[j+8]+world[j+12];
            valid=valid&&std::isfinite(pawn.bones[n][j])&&std::abs(pawn.bones[n][j]-pawn.position[j])<10;
        }
        if(valid)pawn.boneMask|=1u<<n;
    }
    std::array<float,16> afterWorld{};
    if(!read(graphic+0x2a0,afterWorld)||q(skeleton+0xa8)!=buffer) {pawn.boneMask=0;return;}
    for(unsigned i=0;i<16;++i)if(std::abs(afterWorld[i]-world[i])>.001f){pawn.boneMask=0;break;}
}


std::array<float,4> transform(const std::array<float,16>& m,const float* p) {
    std::array<float,4> clip{};
    for(int j=0;j<4;++j)clip[j]=p[0]*m[j]+p[1]*m[j+4]+p[2]*m[j+8]+m[j+12];
    return clip;
}
void* __fastcall capture(void* camera,float* result,const float* point,void* model) {
    const auto caller=reinterpret_cast<std::uintptr_t>(_ReturnAddress())-base;
    if(enabled) {
        std::lock_guard lock(mutex);
        ++state.calls;state.lastCaller=caller;state.lastCamera=reinterpret_cast<std::uintptr_t>(camera);
    }
    std::array<float,16> candidate{};std::array<float,3> position{};
    // Gameplay HUD/world-marker projection call sites, not the editor camera.
    // The world-marker path remains active when the other HUD paths are absent.
    // Verified 0x22ba4df against the live player view (center error 0.00023).
    // Every accepted call still has to match the engine's projection below.
    bool eligible=enabled&&!model&&(caller==0x25220a1||caller==0x250d659||caller==0x250ddd9||caller==0x25281d9||caller==0x5876ba||caller==0x22ba4df);
    eligible=eligible&&read(reinterpret_cast<std::uintptr_t>(camera)+0x170,candidate)&&read(reinterpret_cast<std::uintptr_t>(point),position);
    void* output=original(camera,result,point,model);
    if(eligible) {
        std::array<float,3> actual{};
        auto clip=transform(candidate,position.data());
        if(read(reinterpret_cast<std::uintptr_t>(result),actual)&&std::isfinite(clip[3])&&std::abs(clip[3])>.001f) {
            bool matches=true;
            // The engine folds the sign of negative clip-space Z after division.
            for(int i=0;i<3;++i) {
                float expected=clip[i]/clip[3];
                if(i==2&&clip[2]<0)expected=-expected;
                matches=matches&&std::isfinite(actual[i])&&std::isfinite(clip[i])&&std::abs(expected-actual[i])<.002f;
            }
            for(auto x:candidate)matches=matches&&std::isfinite(x);
            std::lock_guard lock(mutex);
            if(matches){matrix=candidate;++state.projections;state.lastProjectionMs=GetTickCount64();}
            else ++state.mismatches;
        }
    }
    return output;
}
}
std::uintptr_t __fastcall observeLaunch(void* self,void* pawn,const float* origin,const float* direction,const float* inherited) {
    auto module=reinterpret_cast<std::uintptr_t>(self),weapon=q(module+0x50);
    if(enabled&&localIdentity&&q(q(weapon+0x70))==localIdentity) {
        auto config=q(module+0xc0);std::array<float,3> properties{},multipliers{},p{},d{},v{};unsigned char physics{};
        if(read(config+0x510,physics)&&read(config+0x514,properties)&&read(weapon+0x7dc,multipliers)&&
           read(reinterpret_cast<std::uintptr_t>(origin),p)&&read(reinterpret_cast<std::uintptr_t>(direction),d)&&read(reinterpret_cast<std::uintptr_t>(inherited),v)) {
            std::lock_guard lock(mutex);++state.launchSamples;state.launchWeapon=weapon;state.launchPhysics=physics;
            state.projectileSpeed=properties[0]*multipliers[0];state.projectileGravity=properties[1]*multipliers[1];
            state.projectileDropDistance=properties[2]*multipliers[2];state.launchOrigin=p;state.launchDirection=d;state.launchInheritedVelocity=v;
        }
    }
    // Observe and forward unchanged, including the original return register.
    return originalLaunch(self,pawn,origin,direction,inherited);
}
bool start() {
    auto module=GetModuleHandleW(L"FC_m64.dll");
    if(!module)return false;
    base=reinterpret_cast<std::uintptr_t>(module);
    if(!installed) {
        if(MH_CreateHook(reinterpret_cast<void*>(base+0x3a3160),reinterpret_cast<void*>(capture),reinterpret_cast<void**>(&original))!=MH_OK)return false;
        installed=true;
    }
    if(MH_EnableHook(reinterpret_cast<void*>(base+0x3a3160))!=MH_OK)return false;
    enabled=true;
    if(!magazineInstalled&&MH_CreateHook(reinterpret_cast<void*>(base+0x1e08890),reinterpret_cast<void*>(magazineHook),reinterpret_cast<void**>(&originalSetMagazine))==MH_OK)magazineInstalled=true;
    bool ammo=magazineInstalled&&MH_EnableHook(reinterpret_cast<void*>(base+0x1e08890))==MH_OK;
    if(!unlimitedInstalled&&MH_CreateHook(reinterpret_cast<void*>(base+0x1a90cb0),reinterpret_cast<void*>(unlimitedHook),reinterpret_cast<void**>(&originalUnlimited))==MH_OK)unlimitedInstalled=true;
    bool reserve=unlimitedInstalled&&MH_EnableHook(reinterpret_cast<void*>(base+0x1a90cb0))==MH_OK;
    if(!launchInstalled&&MH_CreateHook(reinterpret_cast<void*>(base+0x1e10cf0),reinterpret_cast<void*>(observeLaunch),reinterpret_cast<void**>(&originalLaunch))==MH_OK)launchInstalled=true;
    bool launch=launchInstalled&&MH_EnableHook(reinterpret_cast<void*>(base+0x1e10cf0))==MH_OK;
    std::lock_guard lock(mutex);state.supported=1;state.magazineHook=ammo;state.unlimitedHook=reserve;state.launchHook=launch;return true;
}
void stop() {
    enabled=false;
    cameraFov(false,90,false,90);
    preserveMagazine=false;localIdentity=0;
    unlimitedReserve=false;localInventory=0;
    if(launchInstalled)MH_DisableHook(reinterpret_cast<void*>(base+0x1e10cf0));
    if(unlimitedInstalled)MH_DisableHook(reinterpret_cast<void*>(base+0x1a90cb0));
    if(magazineInstalled)MH_DisableHook(reinterpret_cast<void*>(base+0x1e08890));
    if(installed)MH_DisableHook(reinterpret_cast<void*>(base+0x3a3160));
    std::lock_guard lock(mutex);state.supported=0;state.pawnCount=0;state.animalCount=0;state.lastProjectionMs=0;lastSnapshot=0;current.clear();
}
void refresh() {
    if(!enabled||GetTickCount64()-lastRefresh<50)return;
    lastRefresh=GetTickCount64();
    std::vector<Pawn> found;
    auto manager=q(base+0x4fb3110),controller=q(q(manager+8)),reference=q(q(controller+8)+24);
    auto localId=q(reference),entity=q(reference+16);
    std::array<float,3> local{};
    if(!entity||!read(entity+0x60,local))return;
    localIdentity=localId;
    localInventory=component(entity,0x28);
    auto buckets=q(base+0x4eb41f0),count=q(base+0x4eb41e8);
    if(!buckets||!count||count>65536)return;
    std::vector<std::uintptr_t> heads(count);
    if(!read(buckets,heads.data(),heads.size()*sizeof(std::uintptr_t)))return;
    std::unordered_set<std::uintptr_t> visited;
    for(auto node:heads) {
        while(node&&visited.size()<50000&&visited.insert(node).second) {
            std::array<std::uintptr_t,3> entry{};
            if(!read(node,entry))break;
            node=entry[0];auto object=q(entry[2]+16);
            if(entry[1]==localId)continue;
            auto vtable=q(object);
            bool human=vtable==base+0x445b658,animal=false;
            // Animals use CEntity plus CAnimalAgent, not CPawnEntity.
            if(vtable==base+0x425d400) {
                auto descriptor=q(object+0xc8);int slot=-1;
                if(descriptor&&read(descriptor+0x44,slot)&&slot>=0&&slot<256) {
                    auto agent=q(q(object+0xa8)+8*slot);
                    animal=q(agent)==base+0x45728b8&&q(q(agent+0x20)+0x20)==base+0x2898ce0;
                }
            }
            if(!human&&!animal)continue;
            Pawn pawn{entry[1],{}};
            pawn.object=object;pawn.health=health(object);
            pawn.animal=animal;
            if(human) {
                auto agent=component(object,0x44);
                if(agent&&q(q(agent+0x298))==base+0x202de10)read(agent+0x2b0,pawn.faction);
            }
            if(!read(object+0x60,pawn.position))continue;
            bool valid=true;
            for(float x:pawn.position)valid=valid&&std::isfinite(x)&&std::abs(x)<1e7;
            if(valid)found.push_back(pawn);
        }
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
void setNoReload(bool value){preserveMagazine=value;}
void setUnlimitedAmmo(bool value){unlimitedReserve=value;}
void cameraFov(bool playerRequested,double playerDegrees,bool vehicleRequested,double vehicleDegrees) {
    std::lock_guard guard(fovMutex);
    // Serialize render updates with shutdown. Only the local camera's native
    // FOV override fields are owned, never shared descriptor data.
    if(!enabled&&!fovOverride.camera)return;
    auto manager=q(base+0x4fb3110),ref=q(q(q(q(manager+8))+8)+24),entity=q(ref+16);
    auto pawn=component(entity,0x20),ridable=aspect(pawn,0x3940e81d),vehicle=q(q(ridable+8)+16);
    auto camera=q(pawn+0x2a68)+0x1a0;
    const bool riding=vehicle&&typedComponent(vehicle,0x7efd7da9);
    const bool requested=riding?vehicleRequested:playerRequested;
    const double degrees=riding?vehicleDegrees:playerDegrees;
    auto report=[&](unsigned value) { std::lock_guard lock(mutex); state.vehicleFovState=riding?value:(vehicleRequested?1:0);state.playerFovState=!riding?value:(playerRequested?1:0); };
    const bool want=enabled&&requested&&pawn&&q(pawn+0x2a68)&&q(ref)==localIdentity&&std::isfinite(degrees)&&degrees>=40&&degrees<=140;
    const float radians=static_cast<float>(degrees*std::numbers::pi/180.);
    if(fovOverride.camera) {
        unsigned char active{};float value{};
        const bool same=q(fovOverride.entity+8)==fovOverride.id&&q(fovOverride.pawn+0x2a68)+0x1a0==fovOverride.camera;
        const bool owned=same&&read(fovOverride.camera+0x80,active)&&active==1&&read(fovOverride.camera+0x90,value)&&value==fovOverride.radians;
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
        FovOverride next{entity,pawn,camera,q(ref),{},radians};
        if(!read(camera+0x80,next.original)||next.original[0]) {
            report(3);return;
        }
        // Equivalent to the native zero-blend setter when no prior override
        // is active: enable, zero blend/elapsed fields, target radians.
        auto fields=next.original;fields[0]=1;
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
bool weaponShot(std::uintptr_t pawn,Shot& shot) {
    const auto inventory=localInventory.load(),equipped=q(inventory+0xa8);
    if(!inventory||!equipped||equipped==~std::uint64_t{})return false;
    std::vector<std::uintptr_t> pending{q(inventory+0x1a8)};
    std::unordered_set<std::uintptr_t> visited;
    std::uintptr_t item{};
    while(!pending.empty()&&visited.size()<512) {
        auto node=pending.back();pending.pop_back();
        if(!node||node==inventory+0x198||!visited.insert(node).second)continue;
        if(q(node+0x20)==equipped){item=q(node+0x28);break;}
        pending.push_back(q(node));pending.push_back(q(node+8));
    }
    if(!item||q(item+0x20)!=inventory)return false;
    auto reference=q(item+0x48),entity=q(reference+16),weapon=component(entity,0x10);
    if(!entity||q(entity+8)!=q(reference)||q(weapon)!=base+0x44018e0||q(q(weapon+0x70))!=localIdentity)return false;
    auto module=q(weapon+0x1e0),config=q(module+0xc0);
    if(q(module)!=base+0x4439fa0||q(module+0x50)!=weapon)return false;
    unsigned char physics{},mounted{},special{};
    std::array<float,3> values{},multipliers{},offset{};float range{};double step{};
    if(!read(config+0x510,physics)||physics!=1||!read(weapon+0x95,mounted)||mounted||!read(module+0x48,special)||special||
       !read(config+0x514,values)||!read(weapon+0x7dc,multipliers)||!read(weapon+0x7d4,range)||
       !read(q(pawn+0x2a68)+0x528,offset)||!read(q(base+0x4eb59f8)+0x58,step))return false;
    // The traced on-foot muzzle is the camera origin plus this local offset.
    // Nonzero offsets and mounted inheritance are not yet supported here.
    if(offset[0]!=0||offset[1]!=0||offset[2]!=0)return false;
    const double speed=values[0]*multipliers[0],gravity=values[1]*multipliers[1],drop=values[2]*multipliers[2];
    if(!std::isfinite(speed)||speed<1||speed>10000||!std::isfinite(gravity)||gravity>0||gravity< -1000||
       !std::isfinite(drop)||drop<0||drop>10000||!std::isfinite(range)||range<=0||range>10000||
       !std::isfinite(step)||step<1./1000||step>.1)return false;
    shot.muzzleSpeed=speed;shot.gravity={0,0,gravity};shot.dropDistance=drop;shot.simulationStepSeconds=step;
    shot.maxFlightSeconds=std::min(10.,range/speed);
    std::lock_guard lock(mutex);state.activeWeapon=weapon;state.activeSpeed=float(speed);state.activeGravity=float(gravity);
    state.activeDrop=float(drop);state.simulationStep=float(step);return true;
}
// Same filter, query flags and array lifetime as native IsEntityInRay.
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
    auto world=q(base+0x4f7e828);
    if(!world||!q(world+0x118)||!finite(origin)||!finite(endpoint))return false;
    const auto delta=endpoint-origin;
    if(length(delta)<.01||length(delta)>2000)return false;
    float start[]{float(origin.x),float(origin.y),float(origin.z)};
    float displacement[]{float(delta.x),float(delta.y),float(delta.z)};
    NativeArray hits{},auxiliary{};NativeFilter filter{};
    reinterpret_cast<Construct>(base+0x70ad30)(&filter,0x2dbf,0);
    reinterpret_cast<Query>(base+0x793250)(world,start,displacement,&hits,&filter,&auxiliary,5);
    reinterpret_cast<Destroy>(base+0x70f750)(&filter);
    auto count=(hits.sizes>>32)&0x7fffffff;
    bool found=false,blocked=count>4096||(count&&!hits.data);
    std::vector<CoverHit> coverHits;
    std::uint64_t blocker{};
    if(!blocked)for(std::uint64_t i=0;i<count;++i) {
        unsigned collider{};
        if(!read(hits.data+48*i+0x18,collider)){blocked=true;break;}
        auto object=reinterpret_cast<Resolve>(base+0x78d890)(world,collider);
        auto id=object?q(q(object+8)):0;
        if(id==targetId){found=true;coverHits.push_back(CoverHit::Target);}
        else if(id&&id==localIdentity.load())coverHits.push_back(CoverHit::LocalPlayer);
        else {
            auto ref=object?q(object+8):0,entity=ref?q(ref+16):0;
            const bool vehicle=id&&((ignoredVehicle&&id==ignoredVehicle)||
                (entity&&q(entity+8)==id&&typedComponent(entity,0x7efd7da9)));
            if(vehicle)coverHits.push_back(CoverHit::Vehicle);
            else {coverHits.push_back(id?CoverHit::Obstruction:CoverHit::Unknown);blocker=id;blocked=true;break;}
        }
    }
    reinterpret_cast<Destroy>(base+0x6df020)(&hits);
    reinterpret_cast<Destroy>(base+0xe81370)(&auxiliary);
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
    auto manager=q(base+0x4fb3110),controller=q(q(manager+8)),ref=q(q(controller+8)+24),entity=q(ref+16);
    auto pawn=component(entity,0x20),look=aspect(pawn,0x5df9d3ca),ridable=aspect(pawn,0x3940e81d);
    if(!pawn||!look||q(ref)!=localIdentity||health(entity)<=0){report(2);return;}
    const bool riding=ridable&&q(q(ridable+8)+16);
    std::array<float,16> root{},projection{};std::array<float,3> angles{},eye{};
    if(!read(entity+0x30,root)||!read(look+0x10,angles)||!read(q(pawn+0x2a68)+0x190,eye)){report(2);return;}
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
    // Screen-space distance magnifies camera bob with scope zoom and aspect
    // ratio. Validate the actual camera orientation and proximity instead.
    // Keep a conservative 3-degree limit; do not relax the pose-race checks.
    auto rendered=camera::cameraPose(projection);
    if(!rendered||length(rendered->eye-origin)>3){report(6,0,180);return;}
    const float error=float(camera::alignmentDegrees(view,rendered->forward).value_or(180));
    if(error>3){report(riding?3:6,0,error);return;}
    Shot shot;shot.muzzle=origin;
    AimSettings effective=settings;
    // The verified handheld look path is also usable in a vehicle when its
    // live camera agrees with this pose. Vehicle/projectile inheritance is
    // not calibrated; keep direct aim instead of suppressing all aiming.
    const bool directFallback=settings.travelTime&&(riding||!weaponShot(pawn,shot));
    effective=weaponAimSettings(settings,!directFallback);
    std::vector<Target> targets;
    for(auto candidate:pawns()) {
        if(q(candidate.object+8)!=candidate.id||health(candidate.object)<=0)continue;
        if(!read(candidate.object+0x60,candidate.position))continue;
        candidate.boneMask=0;readBones(candidate.object,candidate,4);
        Target t;t.id=candidate.id;t.kind=candidate.animal?Kind::Animal:(candidate.faction==0||candidate.faction==1?Kind::Enemy:Kind::OtherHuman);
        t.velocity=candidate.velocityValid?candidate.velocity:Vec3{};
        if(!candidate.animal) {
            auto otherPawn=component(candidate.object,0x20),seat=aspect(otherPawn,0x3940e81d),vehicleRef=q(seat+8),vehicle=q(vehicleRef+16);
            if(vehicle&&q(vehicle+8)==q(vehicleRef)&&typedComponent(vehicle,0x7efd7da9))t.vehicleId=q(vehicleRef);
        }
        for(unsigned i=0;i<4;++i)if(candidate.boneMask&(1u<<i)) {
            auto p=candidate.bones[i];float x{},y{};
            if(project(p,2,2,x,y))t.hitLocations[i]=Vec3{p[0],p[1],p[2]};
        }
        targets.push_back(t);
    }
    auto selected=selectTarget(targets,view,shot,effective,false,stickyIdentity);
    if(!selected){report(4,0,error);return;}
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
    if(!read(entity+0x30,latestRoot)||!read(look+0x10,latestAngles)||!read(q(pawn+0x2a68)+0x190,latestEye))return;
    // The engine may update strafe/body orientation during the ray queries.
    // Recompute on the next frame instead of applying an angle from an old basis.
    for(unsigned i=0;i<12;++i)if(std::abs(latestRoot[i]-root[i])>.0001f)return;
    for(unsigned i=0;i<3;++i)if(std::abs(latestAngles[i]-angles[i])>.0001f||std::abs(latestEye[i]-eye[i])>.001f)return;
    auto converted=lookAngles(direction,latestRoot,latestAngles[1]);
    if(!converted){report(6,0,error);return;}
    auto desired=*converted;
    SIZE_T written{};
    if(!WriteProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(look+0x10),desired.data(),sizeof(desired),&written)||written!=sizeof(desired)){report(2);return;}
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
        if(pawn.health<=0||!read(pawn.object+0x60,pawn.position))continue;
        bool valid=true;
        for(float value:pawn.position)valid=valid&&std::isfinite(value)&&std::abs(value)<1e7;
        if(!valid)continue;
        pawn.boneMask=0;readBones(pawn.object,pawn);
        // Discard destroyed/recycled entities and incoherent animation reads.
        if(pawn.boneMask&&q(pawn.object+8)==pawn.id)posed.push_back(pawn);
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
