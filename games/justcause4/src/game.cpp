#include "game.h"
#include "menu.h"
#include "render.h"
#include "game/game_world.h"
#include <MinHook.h>
#include <stdexcept>
#include <fstream>
#include <filesystem>
#include <cmath>
#include <cstring>
#include <deque>
#include <algorithm>
#include <unordered_map>
#include <memory>
#include <cstdio>
#include "tuning.h"
#include "vehicle_math.h"

namespace mod {
std::mutex stateMutex;
Settings settings;
Snapshot snapshot;
std::array<unsigned,3> teleportKeys{0,0,VK_RBUTTON};
std::wstring dataDirectory;
namespace {
template<class F> F* InstallHook(uintptr_t address,F* replacement) {
 void* original{};
 if(MH_CreateHook(reinterpret_cast<void*>(address),reinterpret_cast<void*>(replacement),&original)!=MH_OK)
  throw std::runtime_error("Could not create JC4 hook");
 return reinterpret_cast<F*>(original);
}
uintptr_t base=0;
std::deque<int> actions;
using WindowFn=LRESULT(HWND,UINT,WPARAM,LPARAM);
using UpdateFn=void(void*,float);
using FlipFn=int64_t(jc::HDevice_t*);
WindowFn* originalWindow=nullptr;
UpdateFn* originalUpdate=nullptr;
FlipFn* originalFlip=nullptr;
using WorldUpdateFn=void(uintptr_t,float);
using CharacterResetFn=void(uintptr_t);
using RemoveVehicleFn=uint32_t(uintptr_t,uintptr_t,bool);
WorldUpdateFn* originalWorldUpdate=nullptr;
CharacterResetFn* originalCharacterReset=nullptr;
RemoveVehicleFn* originalRemoveVehicle=nullptr;
thread_local uintptr_t preservedRider=0;
thread_local uintptr_t preservedVehicle=0;
thread_local const char* vehicleStage="idle";
int LogGameFault(EXCEPTION_POINTERS* exception,const char* context) {
    auto record=exception->ExceptionRecord;
    char message[384];
    std::snprintf(message,sizeof(message),
        "%s fault: stage=%s code=%08lX instruction=%llX gameRVA=%llX access=%llu address=%llX",
        context,vehicleStage,record->ExceptionCode,
        static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(record->ExceptionAddress)),
        static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(record->ExceptionAddress)-base),
        record->NumberParameters>0?static_cast<unsigned long long>(record->ExceptionInformation[0]):0,
        record->NumberParameters>1?static_cast<unsigned long long>(record->ExceptionInformation[1]):0);
    Log(message);
    return EXCEPTION_EXECUTE_HANDLER;
}
uintptr_t previousCharacter=0;
bool appliedGod=false, appliedAmmo=false, originalGod=false, originalAmmo=false;
bool faulted=false;
uintptr_t changedClock=0;
float originalScale=1;
bool scaleApplied=false;
struct MovementOverrides {
    std::array<ScalarOverride,5> wing;
    std::array<ScalarOverride,7> board;
    std::array<ScalarOverride,6> wingGrapple;
    std::array<ScalarOverride,12> boardGrapple;
};
std::unordered_map<uintptr_t,MovementOverrides> movementOverrides;
struct GrappleOverrides { ValueOverride range; std::array<ScalarOverride,6> reel; };
std::unordered_map<uintptr_t,GrappleOverrides> grappleOverrides;
template<class T> T& Field(uintptr_t object,uintptr_t offset) {return *reinterpret_cast<T*>(object+offset);}
uintptr_t Instance(uintptr_t address) {return *reinterpret_cast<uintptr_t*>(Address(address));}
void Status(const std::string& text) {snapshot.status=text; Log(text);}
bool WritableRange(uintptr_t address,size_t size) {
    if(address<0x10000 || address+size<address) return false;
    auto end=address+size;
    while(address<end) {
        MEMORY_BASIC_INFORMATION info{};
        if(!VirtualQuery(reinterpret_cast<void*>(address),&info,sizeof(info)) || info.State!=MEM_COMMIT ||
           (info.Protect&(PAGE_GUARD|PAGE_NOACCESS)) ||
           !(info.Protect&(PAGE_READWRITE|PAGE_WRITECOPY|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY))) return false;
        auto next=reinterpret_cast<uintptr_t>(info.BaseAddress)+info.RegionSize;
        if(next<=address) return false;
        address=next;
    }
    return true;
}
void TuneGrapple(uintptr_t character) {
    // CCharacter::GetGrapplingHook (140542110 -> 148389cb0) returns +a40.
    // CGrapplingHook +6b0 points to GrappleTuning; ADF field zero is GrappleDistance.
    auto hook=Field<uintptr_t>(character,0xa40);
    if(!WritableRange(hook,0x6b8)) return;
    auto data=Field<uintptr_t>(hook,0x6b0);
    if(!WritableRange(data,0x228)) return;
    auto& distance=Field<float>(data,0);
    if(!std::isfinite(distance) || distance<=0 || distance>100000) return;
    if(!settings.grappleRange && settings.grappleSpeed==1 && !grappleOverrides.count(data)) return;
    // Finite 50 km avoids infinities in raycasts and squared-distance math.
    // Only revisit the current live resource, never an old grapple or wire.
    auto& values=grappleOverrides[data];
    values.range.Apply(distance,settings.grappleRange,50000.f);
    // GrappleTuning ADF: max/retract/min reel speed and clamp/reel/retract acceleration.
    // GrappleFireSpeed and wire visual retraction are separate and stay native.
    constexpr uintptr_t reelOffsets[]={0x28,0x2c,0x30,0x34,0x38,0x3c};
    const float factor=std::clamp(settings.grappleSpeed,.5f,100.f);
    for(size_t i=0;i<values.reel.size();++i)
        values.reel[i].Apply(Field<float>(data,reelOffsets[i]),factor);
}
void TuneMovement(uintptr_t character) {
    auto data=Field<uintptr_t>(character,0x940);
    snapshot.movementAvailable=false;
    if(!WritableRange(data,0x1008)) return;
    // Embedded CustomMovementSettings ADF, confirmed against the wingsuit task.
    // Tune speed caps and forward drag; gravity, lift and steering remain native.
    constexpr uintptr_t wingOffsets[]={0x7bc,0x7c0,0x7ec,0x7f0,0x7f4};
    // Ground movement, then normal and boosted magrail speed/acceleration.
    // Preserve native deceleration so higher speeds do not amplify braking.
    constexpr uintptr_t boardOffsets[]={0x99c,0x9a0,0xc08,0xc0c,0xc18,0xc1c,0xc20};
    // Air-control slingshot acceleration/speed adjustment and its retract path.
    constexpr uintptr_t wingGrappleOffsets[]={0x93c,0x940,0x944,0x970,0x974,0x978};
    // Hoverboard grapple fire, reel, half-turn and reel-boost bursts.
    // Deceleration, timers and blend-in/out settings retain native values.
    constexpr uintptr_t boardGrappleOffsets[]={0xbb4,0xbb8,0xbbc,0xbc8,0xbcc,0xbd0,0xbdc,0xbe0,0xbe4,0xbf0,0xbf4,0xbf8};
    for(auto off:wingOffsets) if(!std::isfinite(Field<float>(data,off)) || std::abs(Field<float>(data,off))>100000) return;
    for(auto off:boardOffsets) if(!std::isfinite(Field<float>(data,off)) || std::abs(Field<float>(data,off))>100000) return;
    snapshot.movementAvailable=true;
    if(settings.wingsuitSpeed==1 && settings.hoverboardSpeed==1 && settings.grappleSpeed==1 && !movementOverrides.count(data)) return;
    // Keep inactive blocks as bookkeeping only; never dereference a retired block.
    // This retains the baseline if a resource is reused after respawning.
    auto& values=movementOverrides[data];
    float wing=std::clamp(settings.wingsuitSpeed,.5f,100.f);
    float board=std::clamp(settings.hoverboardSpeed,.5f,100.f);
    for(size_t i=0;i<values.wing.size();++i)
        values.wing[i].Apply(Field<float>(data,wingOffsets[i]),i<2?wing:1/(wing*wing));
    for(size_t i=0;i<values.board.size();++i)
        values.board[i].Apply(Field<float>(data,boardOffsets[i]),board);
    const float grapple=std::clamp(settings.grappleSpeed,.5f,100.f);
    for(size_t i=0;i<values.wingGrapple.size();++i)
        values.wingGrapple[i].Apply(Field<float>(data,wingGrappleOffsets[i]),grapple);
    for(size_t i=0;i<values.boardGrapple.size();++i)
        values.boardGrapple[i].Apply(Field<float>(data,boardGrappleOffsets[i]),grapple);
}
bool SameBytes(uintptr_t address, std::initializer_list<unsigned char> expected) {
    const bool matches=std::memcmp(reinterpret_cast<void*>(Address(address)),expected.begin(),expected.size())==0;
    if(!matches)Log("Compatibility code check failed at preferred address " + std::to_string(address) + ". Different engine code or an existing hook; no hooks installed.");
    return matches;
}
struct PoiLock {
    uintptr_t mutex;
    explicit PoiLock(uintptr_t value):mutex(value) {
        reinterpret_cast<void(*)(uintptr_t)>(Address(0x140f29f00))(mutex);
    }
    ~PoiLock() {reinterpret_cast<void(*)(uintptr_t)>(Address(0x140f29f20))(mutex);}
};
struct PoiReference {
    uintptr_t object=0,control=0;
    explicit PoiReference(uintptr_t weak) {
        reinterpret_cast<void*(*)(uintptr_t,void*)>(Address(0x140256a80))(weak,this);
    }
    ~PoiReference() {if(control) reinterpret_cast<void(*)(uintptr_t*)>(Address(0x1400991a0))(&control);}
    PoiReference(const PoiReference&)=delete;
    PoiReference& operator=(const PoiReference&)=delete;
};
struct VehicleReference {
    uintptr_t object=0,control=0;
    explicit VehicleReference(uintptr_t character) {
        reinterpret_cast<void*(*)(uintptr_t,void*)>(Address(0x140542af0))(character,this);
    }
    ~VehicleReference() {
        if(control) reinterpret_cast<void(*)(uintptr_t*)>(Address(0x1400991a0))(&control);
    }
    VehicleReference(const VehicleReference&)=delete;
    VehicleReference& operator=(const VehicleReference&)=delete;
};
struct VehicleTeleport {
    std::shared_ptr<VehicleReference> vehicle;
    uintptr_t character=0;
    ULONGLONG deadline=0;
    bool moved=false;
};
VehicleTeleport vehicleTeleport;
void TuneVehicleBoost(uintptr_t character) {
    if(!settings.vehicleBoost) return;
    vehicleStage="boost vehicle reference";
    VehicleReference vehicle(character);
    static ULONGLONG nextReport=0;
    bool report=GetTickCount64()>=nextReport;
    if(report) nextReport=GetTickCount64()+3000;
    if(!WritableRange(vehicle.object,0x13d11)) {
        if(report) Log("Vehicle boost: no accessible occupied vehicle.");
        return;
    }
    // Native nitro update 149fec41d..149fec48e drains/refills this normalized
    // CVehicle field using VehicleMisc durations at 13ce0/13cec. The optional
    // 14528 HUD component belongs to a different ability and is absent on cars.
    // Preserve input and active flags so releasing boost still stops it.
    vehicleStage="boost fuel";
    auto& fuel=Field<float>(vehicle.object,0x13d00);
    if(report) {
        char message[160];
        std::snprintf(message,sizeof(message),"Vehicle boost: vehicle=%llX fuel=%g active=%u",
            static_cast<unsigned long long>(vehicle.object),fuel,
            static_cast<unsigned>(Field<uint8_t>(vehicle.object,0x13d10)));
        Log(message);
    }
    if(std::isfinite(fuel) && fuel>=0 && fuel<=1.01f) fuel=1.f;
}
uintptr_t PrepareVehicleTeleport(uintptr_t world) {
    vehicleStage="teleport phase check";
    std::shared_ptr<VehicleReference> vehicle;
    uintptr_t character=0;
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        if(!vehicleTeleport.vehicle) return 0;
        if(faulted || GetTickCount64()>vehicleTeleport.deadline || Field<int>(world,0x108)==0) {
            vehicleTeleport={};return 0;
        }
        if(Field<uint8_t>(world,0x10c)) return 0;
        vehicle=vehicleTeleport.vehicle;character=vehicleTeleport.character;
    }
    auto manager=Instance(0x142cb2388);
    auto player=manager?Field<uintptr_t>(manager,0x30):0;
    if(!player || Field<uintptr_t>(player,0x68)!=character) return 0;
    vehicleStage="teleport current vehicle";
    VehicleReference current(character);
    if(current.object!=vehicle->object || !WritableRange(vehicle->object,0x14800)) return 0;
    // Player deactivation calls the population removal routine on the occupied
    // vehicle (14052d432..44a). A strong reference alone does not prevent that.
    // Keep protection through ALL native teleport phases, not just the move.
    preservedVehicle=vehicle->object+0x10;
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        if(vehicleTeleport.moved || Field<int>(world,0x108)!=2) return character;
    }
    if(!WritableRange(Field<uintptr_t>(vehicle->object,0x13de8),0x140)) return 0;
    auto entity=vehicle->object+0x10;
    vehicleStage="teleport world matrix";
    if(Field<uint8_t>(entity,0x144)&1)
        reinterpret_cast<void(*)(uintptr_t)>(Address(0x14024deb0))(entity);
    auto matrix=Field<CMatrix4f>(entity,0x104);
    auto target=Field<CMatrix4f>(world,0x110);
    auto rider=Field<CMatrix4f>(character,0x114);
    CVector3f old{matrix.m[3].x,matrix.m[3].y,matrix.m[3].z};
    CVector3f destination{target.m[3].x,target.m[3].y,target.m[3].z};
    if(!ValidPosition(old) || !ValidPosition(destination)) return 0;
    // Preserve vehicle orientation and the rider's offset, rather than dropping
    // Rico at the vehicle origin. This executes in the native world teleport
    // update, alongside the engine's existing streaming/physics sequence.
    if(!VehicleDestination(matrix,rider,destination)) return 0;
    vehicleStage="native vehicle physics transform";
    // The generic entry is also reached by 140c4b500 -> 149f76c90.
    // 140c4b510 is a specialized wrapper with extra component assumptions;
    // it faults on ordinary cars at 140c4b57b before the generic move runs.
    reinterpret_cast<void(*)(uintptr_t,CMatrix4f*)>(Address(0x140c4b600))(entity,&matrix);
    vehicleStage="teleport rider target";
    Field<CMatrix4f>(world,0x110)=rider;
    Field<uint32_t>(world,0x150)=0; // Skip the native forced character-state change.
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        vehicleTeleport.moved=true;
        Status("Vehicle and rider teleport requested.");
    }
    return character;
}
uintptr_t PrepareVehicleTeleportGuarded(uintptr_t world) {
    __try {return PrepareVehicleTeleport(world);}
    __except(LogGameFault(GetExceptionInformation(),"Vehicle teleport")) {faulted=true;return 0;}
}
void CharacterReset(uintptr_t character) {
    // The normal teleport reset tears down character tasks, including seating.
    // Suppress only this rider's reset during our own vehicle teleport update.
    if(character!=preservedRider) originalCharacterReset(character);
}
uint32_t RemoveVehicle(uintptr_t manager,uintptr_t entity,bool force) {
    if(entity && entity==preservedVehicle && !force) {
        Log("Vehicle teleport: suppressed occupied vehicle removal during player deactivation.");
        return 0;
    }
    return originalRemoveVehicle(manager,entity,force);
}
void ReportTeleportPhase(uintptr_t world) {
    static int lastPhase=0;
    static ULONGLONG phaseStarted=0;
    const auto phase=Field<int>(world,0x108);
    if(phase!=lastPhase) {
        const auto now=GetTickCount64();
        char message[160];
        std::snprintf(message,sizeof(message),"Teleport phase %d -> %d; previous phase lasted %llu ms",
            lastPhase,phase,static_cast<unsigned long long>(phaseStarted?now-phaseStarted:0));
        Log(message);
        lastPhase=phase;phaseStarted=now;
    }
}
void WorldUpdate(uintptr_t world,float dt) {
    ReportTeleportPhase(world);
    auto previous=preservedRider;
    auto previousVehicle=preservedVehicle;
    preservedVehicle=0;
    preservedRider=PrepareVehicleTeleportGuarded(world);
    __try {originalWorldUpdate(world,dt);}
    __finally {preservedRider=previous;preservedVehicle=previousVehicle;}
}
bool FindDestination(int action,CVector3f& destination) {
    // CCommMapUI owns the current user waypoint. The POI registry uses weak
    // references; follow the game's lock/strong-reference sequence before reads.
    uintptr_t waypoint=0;
    if(action==0) {
        auto map=Instance(0x142cb1e88);
        if(!WritableRange(map,0x4d8)) return false;
        waypoint=Field<uintptr_t>(map,0x4c8);
        if(!waypoint) return false;
    }
    auto mutex=Instance(0x142ccb0f8);
    if(!WritableRange(mutex,sizeof(CRITICAL_SECTION))) return false;
    PoiLock lock(mutex);
    auto begin=Instance(0x142ccb068),end=Instance(0x142ccb070);
    if(!begin || end<begin || (end-begin)%24 || (end-begin)/24>4096 || !WritableRange(begin,end-begin)) return false;
    bool found=false;float best=0;int bestRank=2;
    for(auto cursor=begin;cursor<end;cursor+=24) {
        if(action==0 && Field<uintptr_t>(cursor,0)!=waypoint) continue;
        PoiReference ref(cursor);auto poi=ref.object;
        if(!WritableRange(poi,0x3c0) || Field<uintptr_t>(poi,0)!=Address(0x141ed09d0)) continue;
        int rank=0;
        if(action==1) {
            // Skip templates, map-only, hidden/disabled POIs and anything without
            // a currently active objective/sequence, using the native predicate.
            // +2c0 is the HUD's current visibility decision (140dd8410).
            if(!Field<uint8_t>(poi,0x2c0) || Field<uint8_t>(poi,0x284) || Field<int>(poi,0x328)==1 ||
               Field<uint8_t>(poi,0x2c5) || !Field<uint8_t>(poi,0x2c6)) continue;
            if(!reinterpret_cast<bool(*)(uintptr_t)>(Address(0x140db7330))(poi)) {
                // A tracked mission entrance may have no objective reference yet.
                // These are the mission POI subtypes singled out by 140dd8410.
                int subtype=Field<int>(poi,0x338);
                if(Field<int>(poi,0x334)!=12 || subtype<21 || subtype>26 ||
                   (Field<uint64_t>(poi,0x298)&0xffffffffffffull)) continue;
                rank=1;
            }
        }
        if(Field<uint8_t>(poi,0x144)&1)
            reinterpret_cast<void(*)(uintptr_t)>(Address(0x14024deb0))(poi);
        auto p=Field<CVector3f>(poi,0x134);
        if(!ValidPosition(p)) continue;
        float dx=p.x-snapshot.position.x,dy=p.y-snapshot.position.y,dz=p.z-snapshot.position.z;
        float distance=dx*dx+dy*dy+dz*dz;
        if(!found || rank<bestRank || (rank==bestRank && distance<best)) {
            destination=p;best=distance;bestRank=rank;found=true;
        }
        if(action==0) break;
    }
    if(found) Log(std::string(action==0?"Waypoint":"Objective")+" destination: "+
        std::to_string(destination.x)+", "+std::to_string(destination.y)+", "+std::to_string(destination.z));
    return found;
}
void Teleport(CVector3f position) {
    if(!ValidPosition(position)) { Status("Invalid destination."); return; }
    auto world=reinterpret_cast<jc::CGameWorld*>(Instance(0x142cb0a48));
    if(!world || world->IsTeleporting()) {Status("Wait for the current teleport to finish."); return;}
    vehicleTeleport={};
    auto manager=Instance(0x142cb2388);
    auto player=manager?Field<uintptr_t>(manager,0x30):0;
    auto character=player?Field<uintptr_t>(player,0x68):0;
    if(character) {
        auto vehicle=std::make_shared<VehicleReference>(character);
        if(vehicle->object) vehicleTeleport={vehicle,character,GetTickCount64()+30000,false};
    }
    if(position.x==0 && position.z==0) position.x=1; // Upstream documented origin crash.
    CMatrix4f matrix;
    matrix.m[3]={position.x,position.y,position.z,1};
    world->TeleportPlayer(&matrix,nullptr,false,false,0,0,0,nullptr,false,nullptr);
    Status("Teleport requested.");
}
void TickImpl() {
    std::lock_guard<std::mutex> lock(stateMutex);
    snapshot.ready=false;
    bool playing=InGameplay() && !faulted;
    auto clock=Instance(0x142c846b0);
    // CClock::Update scales simulation delta by +0x24 at 0x14764cad0.
    // The unscaled real delta at +0x20 is left untouched.
    if(scaleApplied && (clock!=changedClock || !playing || settings.speed==1)) {
        if(clock==changedClock) Field<float>(clock,0x24)=originalScale;
        scaleApplied=false;
    }
    if(!playing) { actions.clear(); snapshot.characters.clear(); return; }
    auto manager=Instance(0x142cb2388);
    auto player=manager ? Field<uintptr_t>(manager,0x30):0;
    auto character=player ? Field<uintptr_t>(player,0x68):0;
    if(!character) {actions.clear(); return;}
    if(character!=previousCharacter) {
        previousCharacter=character;
        originalGod=(Field<uint8_t>(character,0x418)&2)!=0;
        originalAmmo=Field<bool>(character,0x2288);
        appliedGod=appliedAmmo=false;
        Status("Player connected. Insert opens the menu.");
    }
    snapshot.position=Field<CVector3f>(character,0x144);
    if(!ValidPosition(snapshot.position)) { actions.clear(); return; }
    snapshot.ready=true;
    TuneMovement(character);
    TuneGrapple(character);
    TuneVehicleBoost(character);
    if(settings.speed!=1 && clock) {
        if(!scaleApplied) {originalScale=Field<float>(clock,0x24);changedClock=clock;scaleApplied=true;}
        if(std::isfinite(originalScale) && originalScale>=0 && originalScale<=4)
            Field<float>(clock,0x24)=originalScale*std::clamp(settings.speed,.25f,3.f);
    }
    // Only touch the invulnerable bit; preserve unrelated flags.
    if(settings.god) {Field<uint8_t>(character,0x418)|=2; appliedGod=true;}
    else if(appliedGod) {
        auto& flags=Field<uint8_t>(character,0x418);
        flags=static_cast<uint8_t>((flags&~2u)|(originalGod?2u:0u)); appliedGod=false;
    }
    if(settings.ammo) {Field<bool>(character,0x2288)=true; appliedAmmo=true;}
    else if(appliedAmmo) {Field<bool>(character,0x2288)=originalAmmo; appliedAmmo=false;}
    // CUIWeaponizedWingsuit -> CPlayer::GetBoostCharges (0x140b260e0)
    // returns player+0x5a8. Count getter (0x140b260f0) reads player+0x5a4.
    // These are the gameplay charges, not the cached UI copies.
    int count=Field<int>(player,0x5a4);
    snapshot.boostCount=count;
    snapshot.boostAvailable=count>0 && count<=3;
    // Native IsBoosting getter 140b26120 -> 149878d00 reads +5b4.
    // Do not freeze a burning charge: it prevents the burst from ending.
    RefillIdleBoost(reinterpret_cast<float*>(player+0x5a8),count,settings.boost,
        Field<uint8_t>(player,0x5b4)!=0);
    // Rocket charges are separate from boost: getters at 0x149878890/0x149878a20.
    int rockets=Field<int>(player,0x5cc);
    snapshot.rocketCount=rockets;
    snapshot.rocketsAvailable=rockets>0 && rockets<=3;
    if(settings.rockets && snapshot.rocketsAvailable) {
        for(int i=0;i<rockets;i++) {
            auto& charge=Field<float>(player,0x5d0+i*4);
            if(std::isfinite(charge) && charge>=0 && charge<=1.01f) charge=1.f;
        }
    }
    // Character vector and native faction relationship used by the AR scanner.
    static ULONGLONG lastCharacters=0;
    if(!settings.esp && !settings.aim) snapshot.characters.clear();
    else if(GetTickCount64()-lastCharacters>=16) {
        lastCharacters=GetTickCount64(); snapshot.characters.clear();
        snapshot.charactersTime=lastCharacters;
        auto chars=Instance(0x142cb1d40);
        int playerFaction=Field<int>(character,0x27a4);
        float range=std::clamp(settings.espRange,50.f,1000.f);
        if(chars) {
            auto begin=Field<uintptr_t>(chars,0x150),end=Field<uintptr_t>(chars,0x158);
            if(begin && end>=begin && (end-begin)%8==0 && (end-begin)/8<=4096 && WritableRange(begin,end-begin)) {
                for(auto cursor=begin;cursor<end;cursor+=8) {
                    auto entity=Field<uintptr_t>(cursor,0);
                    if(entity==character || !WritableRange(entity,0x27a8)) continue;
                    int health=Field<int16_t>(entity,0x3ac);
                    if(health<=0) continue;
                    auto p=Field<CVector3f>(entity,0x144);
                    float dx=p.x-snapshot.position.x,dy=p.y-snapshot.position.y,dz=p.z-snapshot.position.z;
                    float distance=std::sqrt(dx*dx+dy*dy+dz*dz);
                    if(!ValidPosition(p) || distance>range || distance<.5f) continue;
                    bool enemy=reinterpret_cast<bool(*)(int,int)>(Address(0x1403034c0))(Field<int>(entity,0x27a4),playerFaction);
                    // Standing character bounds/torso estimate, not skeleton bones.
                    auto top=p,torso=p;top.y+=1.85f;torso.y+=1.05f;
                    snapshot.characters.push_back({entity,p,top,torso,distance,health,enemy});
                }
            }
        }
    }
    while(!actions.empty()) {
        int action=actions.front(); actions.pop_front();
        if(action==0 || action==1) {
            CVector3f destination{};
            if(FindDestination(action,destination)) {destination.y+=3;Teleport(destination);}
            else Status(action==0?"Place a waypoint on the map first.":"No active objective marker is available.");
        } else if(action==3) {
            settings={}; Status("All features reset. Original flags restore on the next update.");
        }
    }
}
void TickGuarded() {
    vehicleStage="player update";
    __try {TickImpl();}
    __except(LogGameFault(GetExceptionInformation(),"Player update")) {faulted=true;}
}
void Update(void* manager,float dt) {
    originalUpdate(manager,dt);
    GameTick();
}
int64_t Flip(jc::HDevice_t* device) {Render(device); return originalFlip(device);}
LRESULT Window(HWND window,UINT message,WPARAM key,LPARAM data) {
    if(MenuEvent(window,message,key,data))
        return (message==WM_XBUTTONDOWN || message==WM_XBUTTONUP || message==WM_XBUTTONDBLCLK)?TRUE:0;
    return originalWindow(window,message,key,data);
}
}
uintptr_t Address(uintptr_t preferred) { return base+(preferred-0x140000000ull); }
void Log(const std::string& text) {
    std::ofstream out(std::filesystem::path(dataDirectory)/L"WetsoxJC4.log",std::ios::app);
    out<<GetTickCount64()<<" "<<text<<"\n";
}
bool ValidPosition(const CVector3f& p) {
    return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z)
        && std::abs(p.x)<100000 && std::abs(p.z)<100000 && p.y>-10000 && p.y<100000;
}
bool InGameplay() {
    if(!base || Field<uint32_t>(Address(0x142cb8f24),0)!=3 || Field<bool>(Address(0x142cbdaf0),0)) return false;
    auto clock=Instance(0x142c846b0);
    return clock && !Field<bool>(clock,0x30);
}
bool ReadCamera(CMatrix4f& matrix) {
    __try {
        auto manager=Instance(0x142c84b90);
        auto camera=manager?Field<uintptr_t>(manager,0x5c0):0;
        if(!camera) return false;
        std::memcpy(&matrix,reinterpret_cast<void*>(camera+0x194),sizeof(matrix));
        const auto* values=reinterpret_cast<const float*>(&matrix);
        for(int i=0;i<16;i++) if(!std::isfinite(values[i]) || std::abs(values[i])>1000000) return false;
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
void SetMenuFocus(bool open) {
    auto input=Instance(0x142ce1af0);
    if(input) reinterpret_cast<void(*)(void*)>(Address(open?0x140fc8560:0x140fc84b0))(reinterpret_cast<void*>(input));
}
void RequestAction(int action) {
    std::lock_guard<std::mutex> lock(stateMutex);
    if(actions.size()<16) actions.push_back(action);
}
void GameTick() {
    ExchangeSession();
    TickGuarded();
    if(faulted) {
        std::lock_guard<std::mutex> lock(stateMutex);
        settings={}; snapshot.ready=false;
        if(snapshot.status!="Game data check failed. Restart the game; see WetsoxJC4.log.")
            Status("Game data check failed. Restart the game; see WetsoxJC4.log.");
    }
}
void LoadHotkeys() {
    std::ifstream input(std::filesystem::path(dataDirectory)/L"SolisMenu.hotkeys.txt");
    int action;unsigned code;
    while(input>>action>>code) if(action>=0 && action<3 && code<256 &&
        code!=VK_INSERT && code!=VK_ESCAPE && code!=VK_BACK) {
        teleportKeys[action]=code;
        for(int i=0;i<3;i++) if(i!=action && code && teleportKeys[i]==code) teleportKeys[i]=0;
    }
}
bool SaveHotkeys() {
    auto path=std::filesystem::path(dataDirectory)/L"SolisMenu.hotkeys.txt";
    auto temporary=path;temporary+=L".tmp";
    std::ofstream out(temporary,std::ios::trunc);
    for(size_t i=0;i<teleportKeys.size();++i) out<<i<<" "<<teleportKeys[i]<<"\n";
    out.close();
    return !out.fail() && MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
}
bool InitializeGame() {
    base=reinterpret_cast<uintptr_t>(GetModuleHandleW(L"JustCause4.exe"));
    if(!base)return false;
    auto dos=reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    auto nt=reinterpret_cast<IMAGE_NT_HEADERS64*>(base+dos->e_lfanew);
    if(nt->FileHeader.TimeDateStamp!=1565905714) {
        Log("Unsupported executable timestamp: " + std::to_string(nt->FileHeader.TimeDateStamp) + "; expected 1565905714 (Steam build 4110618). No hooks installed.");
        return false;
    }
    if(std::memcmp(reinterpret_cast<void*>(Address(0x141e7ee40)),"Avalanche Engine",17)!=0 ||
       !SameBytes(0x140c7fb50,{0xe9,0x2b,0x80,0x38,0x09}) ||
       !SameBytes(0x140fa2c70,{0xe9,0xbb,0xd0,0xdd,0x09}) ||
       !SameBytes(0x148389cb0,{0x48,0x8d,0x81,0x40,0x0a,0x00,0x00,0xc3}) ||
       !SameBytes(0x149878d00,{0x0f,0xb6,0x81,0xb4,0x05,0x00,0x00,0xc3}) ||
       !SameBytes(0x140875640,{0x48,0x8b,0xc4,0x41,0x56,0x48,0x83,0xec,0x60}) ||
       !SameBytes(0x14053c0a0,{0x48,0x8b,0xc4,0x55,0x41,0x54}) ||
       !SameBytes(0x140b82240,{0xe9,0x7b,0x9f,0xde,0x08}) ||
       !SameBytes(0x140c4b600,{0xe9,0x6b,0xb8,0x32,0x09}) ||
       !SameBytes(0x149fec41d,{0xf3,0x0f,0x10,0x8b,0x00,0x3d,0x01,0x00}) ||
       !SameBytes(0x149fec48e,{0xf3,0x0f,0x11,0x8b,0x00,0x3d,0x01,0x00}) ||
       !SameBytes(0x1498aa770,{0x48,0x83,0xec,0x28,0x48,0x8b,0x51,0x30})) {
        Log("Unsupported executable or conflicting hook. Menu not installed."); return false;
    }
    if(MH_Initialize()!=MH_OK)return false;
    try {
    originalWindow=InstallHook(Address(0x140c7fb50),Window);
    originalUpdate=InstallHook(Address(0x1498aa770),Update);
    originalFlip=InstallHook(Address(0x140fa2c70),Flip);
    originalCharacterReset=InstallHook(Address(0x14053c0a0),CharacterReset);
    originalRemoveVehicle=InstallHook(Address(0x140b82240),RemoveVehicle);
    originalWorldUpdate=InstallHook(Address(0x140875640),WorldUpdate);
    if(MH_EnableHook(MH_ALL_HOOKS)!=MH_OK)throw std::runtime_error("Could not enable JC4 hooks");
    } catch(...) {MH_DisableHook(MH_ALL_HOOKS);MH_Uninitialize();return false;}
    Log("Wetsox JC4 initialized for Steam build 4110618. Features default OFF.");
    return true;
}
}
