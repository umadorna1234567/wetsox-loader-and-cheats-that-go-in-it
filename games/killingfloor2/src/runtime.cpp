#include "reflection.hpp"
#include "math.hpp"
#include "silhouette.hpp"
#include "fc5/session.hpp"
#include <MinHook.h>
#include <mutex>
#include <atomic>
#include <fstream>
#include <filesystem>
#include <map>
#include <set>
#include <sstream>

namespace {
using namespace kf2;
HMODULE self{};HANDLE mapping{},guard{};nexus::Session* session{};
std::atomic<bool> running{},stopRequested{};bool installed{};Ptr eventAddress{};
using ScriptEvent=void(*)(Ptr,Ptr,void*);
ScriptEvent scriptEvent{};Ptr scriptAddress{};
std::atomic<Ptr> aimFunction{},projectileFunction{},spawnFunction{};
Ptr weaponTraceFunction{},flyingMoveFunction{};
thread_local Ptr projectileWeapon{};
ULONGLONG targetUpdated{};unsigned long long silentWrites{};
thread_local bool editingShot{},editingWallShot{};
std::recursive_mutex stateMutex;std::mutex startup;
wetsox::kf2::Settings settings;fc5::runtime::Status status;
std::uint64_t frames{};unsigned lastAction{};bool menuActive{true};
std::string detail="Waiting for a solo match";
std::ofstream logFile;
Ptr controller{},pawn{},world{},canvas{},selectedTarget{};Vec selectedPoint{},eye{};Rot view{};float renderFov{90};bool scopeActive{};float scopeScale{};Matrix scopeView{},scopeProjection{};
struct ScopeLens {bool tracked{};Vec center{},right{},up{};float radius{},meshFov{};} scopeLens;
void log(const std::string& value){if(logFile){logFile<<GetTickCount64()<<" "<<value<<'\n';logFile.flush();}}
struct Patch {Ptr owner{},type{};Name identity{};std::vector<unsigned char> original;unsigned mask{};bool used{};};
std::map<std::pair<Ptr,unsigned>,Patch> patches;
bool valid(const Patch& p){auto n=read<Name>(p.owner+0x48);return objectClass(p.owner)==p.type&&n.index==p.identity.index&&n.number==p.identity.number;}
void restore(){for(auto it=patches.begin();it!=patches.end();){auto& p=it->second;if(p.used){++it;continue;}if(valid(p)){if(p.mask){auto n=read<unsigned>(it->first.first),old=*reinterpret_cast<const unsigned*>(p.original.data());n=(n&~p.mask)|(old&p.mask);write(it->first.first,&n,4);}else write(it->first.first,p.original.data(),p.original.size());}it=patches.erase(it);}}
template<class T>void patchAt(Ptr object,Ptr address,T value,unsigned mask=0){
 auto [it,added]=patches.try_emplace(std::make_pair(address,mask));auto& p=it->second;
 if(added){p.owner=object;p.type=objectClass(object);p.identity=read<Name>(object+0x48);p.mask=mask;p.original.resize(sizeof(T));SIZE_T n{};if(!ReadProcessMemory(GetCurrentProcess(),(void*)address,p.original.data(),sizeof(T),&n)||n!=sizeof(T)){patches.erase(it);return;}}
 if(!valid(p)){patches.erase(it);return;}p.used=true;write(address,&value,sizeof(T));
}
template<class T>void patch(Ptr object,std::string_view key,T value){auto f=field(objectClass(object),key);if(f&&f.size>=int(sizeof(T)))patchAt(object,object+f.offset,value);}
void patchFlag(Ptr object,std::string_view key,bool value){auto f=field(objectClass(object),key);if(f&&f.type=="BoolProperty"){auto n=read<unsigned>(object+f.offset);patchAt(object,object+f.offset,(n&~f.mask)|(value?f.mask:0),f.mask);}}
float originalFloat(Ptr object,std::string_view key){auto f=field(objectClass(object),key);if(!f)return 0;auto it=patches.find({object+f.offset,0});if(it!=patches.end()&&it->second.original.size()==4){float v;std::memcpy(&v,it->second.original.data(),4);return v;}return read<float>(object+f.offset);}
void exchange(){
 if(!session)return;const auto wait=WaitForSingleObject(guard,0);if(wait!=WAIT_OBJECT_0&&wait!=WAIT_ABANDONED)return;
 const bool fresh=running&&session->version==nexus::sessionVersion&&GetTickCount64()-session->heartbeat<3000;
 settings=fresh?session->kf2:wetsox::kf2::Settings{};menuActive=!fresh||session->menuActive;
 session->frames=frames;session->status=status;strncpy_s(session->detail,detail.c_str(),_TRUNCATE);ReleaseMutex(guard);
}
void weaponFeatures(Ptr weapon){
 if(!isA(weapon,"KFWeapon"))return;
 if(settings.rapid){
  // Let the normal held-trigger state handle repeats and trigger release.
  // Preserve special alt-fire, melee and grenade states. Restore on toggle-off.
  auto states=get<Array>(weapon,"FiringStatesArray");
  static const Name automatic=findName("WeaponFiring");
  if(automatic.index&&states.data&&states.count>0&&states.count<=16){
   auto original=read<Name>(states.data);
   auto old=patches.find({states.data,0});
   if(old!=patches.end()&&old->second.original.size()==sizeof(Name))std::memcpy(&original,old->second.original.data(),sizeof(Name));
   const auto stateName=name(original.index);
   if(stateName=="WeaponSingleFiring"||stateName=="WeaponSingleFireAndReload")patchAt(weapon,states.data,automatic);
  }
 }
 if(settings.ammo)patchFlag(weapon,"bInfiniteSpareAmmo",true);
 if(settings.reload){auto ammo=field(objectClass(weapon),"AmmoCount"),cap=field(objectClass(weapon),"MagazineCapacity");if(ammo&&cap)for(int i=0;i<std::min({ammo.count,cap.count,2});++i){int value=read<int>(weapon+cap.offset+i*4);if(value>0&&value<100000)write(weapon+ammo.offset+i*4,&value,4);}}
 if(settings.recoil){for(auto key:{"maxRecoilPitch","minRecoilPitch","maxRecoilYaw","minRecoilYaw"})patch(weapon,key,0);patch(weapon,"RecoilViewRotationScale",0.f);patch(weapon,"RecoilRotator",Rot{});patch(weapon,"TotalRecoilRotator",Rot{});}
 if(settings.sway){
  // Strength values multiply spring tension: zeroing them prevents the weapon
  // from returning to rest while walking. Suppress displacement/input instead
  // and leave the native weapon positioning and scope camera updates running.
  for(auto key:{"BobDamping","JumpDamping","LagLimit","LagYawCoefficient","StrafeLagLimit","StrafeLagRate"})patch(weapon,key,0.f);
  // KFPawn.GetPawnViewLocation adds WalkBob. WeaponBob with bWeaponBob=false
  // returns that same WalkBob, cancelling motion relative to the camera.
  // Zero BobDamping alone leaves the camera moving past a stationary weapon.
  patchFlag(pawn,"bWeaponBob",false);
 }
 for(auto entry:{std::pair{"Spread",settings.spread},std::pair{"FireInterval",settings.rapid}})if(entry.second){
  auto arr=get<Array>(weapon,entry.first);if(arr.count>0&&arr.count<=16&&arr.data)for(int i=0;i<arr.count;++i){auto at=arr.data+i*4;float original=read<float>(at);auto it=patches.find({at,0});if(it!=patches.end())std::memcpy(&original,it->second.original.data(),4);if(std::isfinite(original)&&original>=0&&original<100)patchAt(weapon,at,entry.first==std::string_view("Spread")?0.f:std::max(.015f,original/std::clamp(settings.fireRate,1.f,10.f)));}
 }
}
struct Target {Ptr object{},mesh{};Vec position{},head{};bool zed{};int health{},maxHealth{};};
std::vector<Target> targets;
struct Loot {Ptr object{},type{};Name identity{};};
std::vector<Loot> loot;
std::unordered_map<Ptr,bool> lootClasses;
int lootScan{};
void scanLoot(){
 if(!settings.lootEsp){loot.clear();return;}
 const int count=read<int>(objects+8);const auto data=read<Ptr>(objects);
 if(count<1||count>2000000||!data)return;
 for(int n=0;n<1024;++n){
  if(lootScan>=count)lootScan=0;
  auto object=read<Ptr>(data+8ull*lootScan++),type=objectClass(object);if(!type)continue;
  auto [it,added]=lootClasses.try_emplace(type,false);
  if(added)it->second=isA(object,"PickupFactory")||isA(object,"DroppedPickup");
  if(!it->second||get<Ptr>(object,"WorldInfo")!=world||flag(object,"bDeleteMe")||flag(object,"bHidden"))continue;
  if(std::none_of(loot.begin(),loot.end(),[&](const Loot& v){return v.object==object;}))loot.push_back({object,type,read<Name>(object+0x48)});
 }
 std::erase_if(loot,[&](const Loot& v){auto n=read<Name>(v.object+0x48);return objectClass(v.object)!=v.type||n.index!=v.identity.index||n.number!=v.identity.number||get<Ptr>(v.object,"WorldInfo")!=world||flag(v.object,"bDeleteMe")||flag(v.object,"bHidden");});
}
Vec bone(Ptr mesh,Name nameValue){Call call(mesh,"GetBoneLocation");call.set("BoneName",nameValue);call.set("Space",0);return call.run(mesh)?call.result<Vec>():Vec{};}

Vec boneAxis(Ptr mesh,Name boneName,BoneAxis axis){Call call(mesh,"GetBoneAxis");call.set("BoneName",boneName);call.set("Axis",static_cast<unsigned char>(axis));return call.run(mesh)?call.result<Vec>():Vec{};}
void trackScopeLens(Ptr weapon){
 scopeLens={};if(!scopeActive||!isA(weapon,"KFWeap_Bow_Crossbow"))return;
 static const Name scopeBone=findName("RW_Scope");auto mesh=get<Ptr>(weapon,"Mesh");if(!scopeBone.index||!mesh)return;
 const Vec captureRight{scopeView.m[0],scopeView.m[4],scopeView.m[8]};
 const Vec pivot=bone(mesh,scopeBone),forward=boneAxis(mesh,scopeBone,BoneAxis::X),up=boneAxis(mesh,scopeBone,BoneAxis::Z),right=lensRightAxis(boneAxis(mesh,scopeBone,BoneAxis::Y),captureRight);
 const float fov=get<float>(mesh,"FOV");
 const Vec captureForward{scopeView.m[2],scopeView.m[6],scopeView.m[10]};
 if(!finite(pivot)||!finite(forward)||!validLensBasis(right,up,captureForward)||std::abs(dot(forward,captureForward))<.8f||fov<=1||fov>=179)return;
 // Follow the rear glass, not a plane at the bone pivot. Their resting centers
 // nearly coincide onscreen, but firing animation rotates/translates the glass
 // around that pivot. Keep the previously validated resting viewport size.
 scopeLens={true,crossbowLensCenter(pivot,forward,up),right,up,crossbowLensRadius,fov};
}

bool visiblePoint(Ptr target,Vec start,Vec end){
 ++status.visibilityQueries;bool clear=false;
 Call trace(pawn,"Trace");
 if(finite(start)&&finite(end)&&field(trace.fn,"ReturnValue")&&
    trace.set("TraceStart",start)&&trace.set("TraceEnd",end)&&trace.set("bTraceActors",1u)&&
    trace.set("ExtraTraceFlags",1)&&trace.run(pawn)){
  const auto hit=trace.result<Ptr>();clear=!hit||hit==target;
 }
 clear?++status.visibilityClear:++status.visibilityBlocked;return clear;
}
bool visibleTarget(const Target& t){return visiblePoint(t.object,eye,t.head);}
void updateTargets(){
 targets.clear();std::set<Ptr> visited;
 for(auto p=get<Ptr>(world,"PawnList");p&&visited.size()<512&&visited.insert(p).second;p=get<Ptr>(p,"NextPawn")){
  if(p==pawn)continue;bool zed=isA(p,"KFPawn_Monster");if(!zed&&!isA(p,"KFPawn_Human"))continue;
  int hp=get<int>(p,"Health"),maxHp=get<int>(p,"HealthMax");if(hp<=0||maxHp<=0||flag(p,"bDeleteMe"))continue;
  auto mesh=get<Ptr>(p,"Mesh");Vec pos=get<Vec>(p,"Location"),head=bone(mesh,get<Name>(p,"HeadBoneName"));
  if(!finite(pos))continue;if(!finite(head)||length(head-pos)>1000||length(head-pos)<1)head=pos+Vec{0,0,get<float>(p,"BaseEyeHeight")};
  targets.push_back({p,mesh,pos,head,zed,hp,maxHp});
 }status.pawnCount=unsigned(targets.size());
}
void aim(float dt){
 selectedTarget=0;targetUpdated=GetTickCount64();status.aimState=0;
 DWORD foreground{};GetWindowThreadProcessId(GetForegroundWindow(),&foreground);
 if(!settings.aim||menuActive||foreground!=GetCurrentProcessId())return;
 float maxDistance=1;for(const auto& t:targets)if(t.zed)maxDistance=std::max(maxDistance,length(t.head-eye));
 float best=1e30f;const Target* target=nullptr;
 for(const auto& t:targets){if(!t.zed)continue;const auto delta=t.head-eye;const float degrees=angle(direction(view),delta);if(!inCone(degrees,settings.fov))continue;
  const float score=targetScore(degrees,length(delta),settings.fov,maxDistance,settings.priority);if(score>=best)continue;if(settings.visibility&&!visibleTarget(t))continue;best=score;target=&t;
 }
 if(!target){status.aimState=4;return;}selectedTarget=target->object;selectedPoint=target->head;
 if(settings.silent){status.aimState=scriptEvent?6u:0u;if(!scriptEvent)detail="Silent aim callback unavailable";return;}
 const auto desired=rotation(selectedPoint-eye),current=get<Rot>(controller,"Rotation");const float weight=settings.smooth<=0?1.f:1-std::exp(-std::clamp(dt,0.f,.1f)/settings.smooth);
 Rot next{current.pitch+int(turn(desired.pitch-view.pitch)*weight),current.yaw+int(turn(desired.yaw-view.yaw)*weight),current.roll};
 Call set(controller,"SetRotation");if(set.set("NewRotation",next)&&set.run(controller)&&set.result<unsigned>()){++status.aimWrites;status.aimState=5;}
}
struct NoclipState {Ptr pc{},body{},type{};Name identity{},state{};bool actors{},blocks{},collideWorld{};unsigned char physics{};} flying;
void setFlag(Ptr object,std::string_view key,bool enabled){auto f=field(objectClass(object),key);if(f&&f.type=="BoolProperty"){auto n=read<unsigned>(object+f.offset);n=(n&~f.mask)|(enabled?f.mask:0);write(object+f.offset,&n,4);}}
void setCollision(Ptr object,bool actors,bool blocks){Call c(object,"SetCollision");c.set("bNewColActors",unsigned(actors));c.set("bNewBlockActors",unsigned(blocks));c.set("bNewIgnoreEncroachers",unsigned(flag(object,"bIgnoreEncroachers")));c.run(object);}
void state(Ptr pc,Name value){if(!value.index)return;Call c(pc,"GotoState");c.set("NewState",value);c.run(pc);}
void endNoclip(){
 if(!flying.body)return;const auto old=flying;flying={};auto identity=read<Name>(old.body+0x48);
 if(objectClass(old.body)!=old.type||identity.index!=old.identity.index||identity.number!=old.identity.number)return;
 setCollision(old.body,old.actors,old.blocks);setFlag(old.body,"bCollideWorld",old.collideWorld);
 Call physics(old.body,"SetPhysics");physics.set("newPhysics",old.physics);physics.run(old.body);
 if(get<Ptr>(old.pc,"Pawn")==old.body&&get<int>(old.body,"Health")>0)state(old.pc,old.state);
}
void beginNoclip(){
 if(!scriptEvent||!flyingMoveFunction){detail="Noclip movement callback unavailable";return;}
 if(flying.body==pawn&&flying.pc==controller)return;endNoclip();
 static const Name flight=findName("PlayerFlying");if(!flight.index){detail="Flying state unavailable";return;}
 Call current(controller,"GetStateName");if(!current.run(controller))return;
 flying={controller,pawn,objectClass(pawn),read<Name>(pawn+0x48),current.result<Name>(),flag(pawn,"bCollideActors"),flag(pawn,"bBlockActors"),flag(pawn,"bCollideWorld"),get<unsigned char>(pawn,"Physics")};
 setCollision(pawn,false,false);setFlag(pawn,"bCollideWorld",false);
 Call physics(pawn,"SetPhysics");physics.set("newPhysics",static_cast<unsigned char>(4));physics.run(pawn);state(controller,flight);
 // Move explicitly in PlayerMove; native flying integration must not add drift.
 Call stopPhysics(pawn,"SetPhysics");stopPhysics.set("newPhysics",static_cast<unsigned char>(0));stopPhysics.run(pawn);
}
void moveNoclip(float dt){
 if(!settings.noclip||flying.body!=pawn||get<int>(pawn,"Health")<=0)return;
 static bool recorded{};if(!recorded){recorded=true;log("Explicit noclip movement callback active");}
 const Vec zero{};auto velocity=field(objectClass(pawn),"Velocity"),acceleration=field(objectClass(pawn),"Acceleration");
 if(velocity)write(pawn+velocity.offset,&zero,sizeof(zero));if(acceleration)write(pawn+acceleration.offset,&zero,sizeof(zero));
 DWORD focused{};GetWindowThreadProcessId(GetForegroundWindow(),&focused);
 if(menuActive||focused!=GetCurrentProcessId()||!std::isfinite(dt)||dt<=0)return;
 auto down=[](int key){return (GetAsyncKeyState(key)&0x8000)?1.f:0.f;};
 float forward=down('W')-down('S'),right=down('D')-down('A'),up=down(VK_SPACE)-down(VK_CONTROL);
 // Retain horizontal controller-stick navigation when keyboard movement is idle.
 const auto input=get<Ptr>(controller,"PlayerInput");
 if(forward==0){float value=get<float>(input,"RawJoyUp");if(std::isfinite(value)&&std::abs(value)>.15f)forward=std::clamp(value,-1.f,1.f);}
 if(right==0){float value=get<float>(input,"RawJoyRight");if(std::isfinite(value)&&std::abs(value)>.15f)right=std::clamp(value,-1.f,1.f);}
 const auto motion=noclipDirection(get<Rot>(controller,"Rotation").yaw,forward,right,up);
 float speed=get<float>(pawn,"AirSpeed");if(!std::isfinite(speed)||speed<=0||speed>100000)return;
 const auto destination=get<Vec>(pawn,"Location")+motion*(speed*std::min(dt,.1f));
 if(!finite(destination))return;Call move(pawn,"SetLocation");if(move.set("NewLocation",destination))move.run(pawn);
}
struct WallProjectile {Ptr object{},type{};Name identity{};};
std::vector<WallProjectile> wallProjectiles;
void updateWallProjectiles(){
 std::erase_if(wallProjectiles,[](const WallProjectile& p){const auto n=read<Name>(p.object+0x48);return objectClass(p.object)!=p.type||n.index!=p.identity.index||n.number!=p.identity.number||flag(p.object,"bDeleteMe")||get<Ptr>(p.object,"WorldInfo")!=world;});
 if(!settings.wallShots){wallProjectiles.clear();return;}
 for(const auto& p:wallProjectiles)patchFlag(p.object,"bCollideWorld",false);
}
struct FovState {Ptr camera{},type{};Name identity{};float original{},applied{};bool sighted{};} fovState;
void endFov(){
 if(!fovState.camera)return;const auto old=fovState;fovState={};auto n=read<Name>(old.camera+0x48);
 if(objectClass(old.camera)!=old.type||n.index!=old.identity.index||n.number!=old.identity.number)return;
 if(!flag(get<Ptr>(pawn,"Weapon"),"bUsingSights")){Call set(old.camera,"SetFOV");set.set("NewFOV",old.original);set.run(old.camera);}
}
void updateFov(Ptr cameraObject){
 if(!settings.playerFov||fovState.camera!=cameraObject)endFov();
 if(!settings.playerFov||!cameraObject)return;
 const bool sighted=flag(get<Ptr>(pawn,"Weapon"),"bUsingSights");
 Call options(cameraObject,"GetOptionsFOVScale");float optionsScale=options.run(cameraObject)?options.result<float>():1.f;
 if(!std::isfinite(optionsScale)||optionsScale<.1f||optionsScale>4)optionsScale=1;
 if(!fovState.camera)fovState={cameraObject,objectClass(cameraObject),read<Name>(cameraObject+0x48),get<float>(cameraObject,"DefaultFOV")*optionsScale,0,false};
 const float unscaled=settings.playerDegrees/optionsScale;
 patch(controller,"DefaultFOV",unscaled);patch(cameraObject,"DefaultFOV",unscaled);
 if(!sighted&&(fovState.applied!=settings.playerDegrees||fovState.sighted)){Call set(cameraObject,"SetFOV");set.set("NewFOV",settings.playerDegrees);set.run(cameraObject);}
 fovState.applied=settings.playerDegrees;fovState.sighted=sighted;
}
void gameplay(float dt){
 for(auto& [at,p]:patches)p.used=false;
 const bool local=isA(controller,"KFPlayerController")&&isA(get<Ptr>(controller,"Player"),"LocalPlayer");
 pawn=get<Ptr>(controller,"Pawn");world=get<Ptr>(controller,"WorldInfo");
 if(!local||!pawn||!world||!field(objectClass(world),"NetMode")||get<unsigned char>(world,"NetMode")!=0||get<int>(pawn,"Health")<=0){detail="Waiting for a living player in a solo match";selectedTarget=0;aimFunction=0;projectileFunction=0;spawnFunction=0;targets.clear();wallProjectiles.clear();endNoclip();endFov();restore();return;}
 detail="Solo match | reflection active";
 Call camera(controller,"GetPlayerViewPoint");if(!camera.run(controller)){detail="Camera unavailable";restore();return;}eye=camera.result<Vec>("out_Location");view=camera.result<Rot>("out_Rotation");
 // Parameter names vary between overrides; check the inherited spelling too.
 if(!field(camera.fn,"out_Location")){eye=camera.result<Vec>("Location");view=camera.result<Rot>("Rotation");}
 auto cameraObject=get<Ptr>(controller,"PlayerCamera");
 struct CameraCache {float time;Vec position;Rot rotation;float fov;};
 auto cache=get<CameraCache>(cameraObject,"CameraCache");
 if(finite(cache.position)){eye=cache.position;view=cache.rotation;}
 Call actualFov(cameraObject,"GetActualFOV");if(actualFov.run(cameraObject)){const float value=actualFov.result<float>();if(std::isfinite(value)&&value>.1f&&value<179)renderFov=value;}
 auto scopedWeapon=get<Ptr>(pawn,"Weapon"),capture=get<Ptr>(scopedWeapon,"SceneCapture");
 aimFunction=function(scopedWeapon,"GetAdjustedAim");
 projectileFunction=function(scopedWeapon,"ProjectileFire");
 spawnFunction=function(scopedWeapon,"SpawnProjectile");
 scopeActive=capture&&isA(scopedWeapon,"KFWeap_ScopedBase")&&flag(scopedWeapon,"bUsingSights")&&!flag(scopedWeapon,"bZoomingIn")&&!flag(scopedWeapon,"bZoomingOut");
 if(scopeActive){scopeScale=get<float>(scopedWeapon,"ScopeTextureScale");scopeView=get<Matrix>(capture,"ViewMatrix");scopeProjection=get<Matrix>(capture,"ProjMatrix");scopeActive=std::isfinite(scopeScale)&&scopeScale>.05f&&scopeScale<=1.f&&scopeProjection.m[0]>0;}

 trackScopeLens(scopedWeapon);
 if(!finite(eye)){restore();return;}
 status.localPosition={eye.x,eye.y,eye.z};status.lastCamera=cameraObject;status.playerFovDegrees=renderFov;status.playerFovState=settings.playerFov?1u:0u;++status.snapshots;
 if(settings.god){patchFlag(controller,"bGodMode",true);const int maximum=get<int>(pawn,"HealthMax");auto hp=field(objectClass(pawn),"Health");if(hp&&maximum>0&&maximum<100000)write(pawn+hp.offset,&maximum,4);}
 if(settings.moveSpeed)for(auto key:{"GroundSpeed","SprintSpeed","SprintStrafeSpeed"}){float base=originalFloat(pawn,key);if(base>0&&base<10000)patch(pawn,key,base*std::clamp(settings.moveMultiplier,.25f,10.f));}
 if(settings.noclip){beginNoclip();float base=originalFloat(pawn,"AirSpeed");if(base>0)patch(pawn,"AirSpeed",base*settings.noclipMultiplier);}else endNoclip();
 updateFov(cameraObject);
 auto inventory=get<Ptr>(pawn,"InvManager");if(settings.carry)patchFlag(inventory,"bInfiniteWeight",true);
 weaponFeatures(get<Ptr>(pawn,"Weapon"));
 if(settings.syringe){std::set<Ptr> visited;for(auto item=get<Ptr>(inventory,"InventoryChain");item&&visited.size()<128&&visited.insert(item).second;item=get<Ptr>(item,"Inventory"))if(isA(item,"KFWeap_HealerBase")){patch(item,"HealRechargeTime",.01f);patch(item,"HealRechargePerSecond",10000.f);auto ammo=field(objectClass(item),"AmmoCount");int full=get<int>(item,"MagazineCapacity");if(ammo&&full>0)write(item+ammo.offset,&full,4);}}
 if(settings.actionSequence!=lastAction){lastAction=settings.actionSequence;if(lastAction){auto pri=get<Ptr>(controller,"PlayerReplicationInfo");auto score=field(objectClass(pri),"Score");float dosh=get<float>(pri,"Score");if(score&&std::isfinite(dosh)){dosh=std::clamp(dosh+float(settings.doshAmount),0.f,10000000.f);write(pri+score.offset,&dosh,4);}}}
 updateTargets();scanLoot();aim(dt);updateWallProjectiles();restore();
 if(settings.wallShots&&(!scriptEvent||!weaponTraceFunction))detail="Wall-shot callback unavailable";

}
struct Bgra {unsigned char b,g,r,a;};
void line(float x1,float y1,float x2,float y2,wetsox::kf2::Color c){
 if(!std::isfinite(x1+y1+x2+y2))return;
 struct LineCall {Ptr type{},fn{};std::vector<unsigned char> bytes;std::array<int,5> offsets{};bool valid{};};static LineCall call;
 if(call.type!=objectClass(canvas)){
  call={};call.type=objectClass(canvas);call.fn=function(canvas,"Draw2DLine");const int size=read<int>(call.fn+0x88);
  if(call.fn&&size>=20&&size<=256){call.bytes.resize(size);call.valid=true;int i=0;
   for(auto key:{"X1","Y1","X2","Y2","LineColor"}){auto f=field(call.fn,key);if(!f||f.size!=4||f.offset<0||f.offset+4>size){call.valid=false;break;}call.offsets[i++]=f.offset;}
  }
 }
 if(!call.valid)return;const float coords[]{x1,y1,x2,y2};for(int i=0;i<4;++i)std::memcpy(call.bytes.data()+call.offsets[i],&coords[i],4);
 const Bgra color{c.b,c.g,c.r,c.a};std::memcpy(call.bytes.data()+call.offsets[4],&color,4);processEvent(canvas,call.fn,call.bytes.data(),nullptr);
}
std::optional<Vec> project(Vec p,float width,float height){
 auto ordinary=projectWorld(p,eye,view,renderFov,width,height);if(!scopeActive)return ordinary;
 const float diameter=width*scopeScale;
 if(scopeLens.tracked){
  if(auto uv=scopeCoordinates(p,scopeView,scopeProjection)){
   auto point=projectWorld(scopeLens.center+scopeLens.right*(uv->x*scopeLens.radius)+scopeLens.up*(uv->y*scopeLens.radius),eye,view,scopeLens.meshFov,width,height);
   if(point){point->z=-uv->z;return point;}
  }
  auto center=projectWorld(scopeLens.center,eye,view,scopeLens.meshFov,width,height);
  auto right=projectWorld(scopeLens.center+scopeLens.right*scopeLens.radius,eye,view,scopeLens.meshFov,width,height);
  auto top=projectWorld(scopeLens.center+scopeLens.up*scopeLens.radius,eye,view,scopeLens.meshFov,width,height);
  if(ordinary&&center&&right&&top&&!insideLens(*ordinary,*center,*right,*top))return ordinary;
 }else{
  if(auto lens=projectScope(p,scopeView,scopeProjection,diameter,width,height))return lens;
  if(ordinary&&std::hypot(ordinary->x-width*.5f,ordinary->y-height*.5f)>diameter*.5f)return ordinary;
 }
 return {};
}
std::optional<Vec> project(Vec p){return project(p,float(get<int>(canvas,"SizeX")),float(get<int>(canvas,"SizeY")));}
struct CachedSilhouette {SilhouetteGeometry geometry;bool valid{};};
std::map<std::pair<Ptr,int>,CachedSilhouette> silhouettes;
void rasterSilhouette(const Target& t,unsigned owner,DepthSilhouetteMask& mask,float width,float height){
 auto asset=get<Ptr>(t.mesh,"SkeletalMesh");if(!asset)return;
 const auto lods=get<Array>(asset,"LODModels");const int lodIndex=silhouetteLod(get<int>(t.mesh,"PredictedLODLevel"),lods.count);if(lodIndex<0)return;
 const auto cacheKey=std::make_pair(asset,lodIndex);
 auto found=silhouettes.find(cacheKey);auto identity=read<Name>(asset+0x48);
 if(found!=silhouettes.end()&&(found->second.geometry.type!=objectClass(asset)||found->second.geometry.identity.index!=identity.index||found->second.geometry.identity.number!=identity.number)){silhouettes.erase(found);found=silhouettes.end();}
 if(found==silhouettes.end()){
  if(silhouettes.size()>=64)silhouettes.clear();
  auto& entry=silhouettes[cacheKey];entry.valid=loadSilhouetteGeometry(asset,lodIndex,entry.geometry);found=silhouettes.find(cacheKey);
  log("Silhouette geometry "+objectName(asset)+" | LOD "+std::to_string(lodIndex)+(entry.valid?" ready | vertices "+std::to_string(entry.geometry.vertices.size()):" unavailable"));
 }
 if(!found->second.valid)return;
 auto& geometry=found->second.geometry;std::vector<Vec> points;if(!silhouettePositions(t.mesh,geometry,points))return;
 for(auto& point:points){auto p=project(point,width,height);point=p?*p:Vec{};}
 for(size_t i=0;i<geometry.indices.size();i+=3)mask.triangle(points[geometry.indices[i]],points[geometry.indices[i+1]],points[geometry.indices[i+2]],owner);
}
void drawSilhouettes(float width,float height){
 LARGE_INTEGER started{},frequency{};QueryPerformanceCounter(&started);QueryPerformanceFrequency(&frequency);
 unsigned outlinedActors=0;size_t outlinedSegments=0;

 // Separate depth layers: Zeds occlude other Zeds, without importing map/world
 // depth. Walls never suppress the outlines. Player settings remain separate.
 static DepthSilhouetteMask zeds,players;
 const bool zed=settings.zed.enabled&&settings.zed.outline,player=settings.player.enabled&&settings.player.outline;
 if(!zed&&!player)return;if(zed&&!zeds.begin(width,height))return;if(player&&!players.begin(width,height))return;
 unsigned id=0;for(const auto& t:targets){++id;if(t.zed?!zed:!player)continue;
  auto center=project(t.position,width,height);auto head=project(t.head,width,height);
  if(!center&&!head)continue;
  if(center&&head){float margin=std::max(120.f,std::abs(center->y-head->y)*3);
   if(center->x < -margin&&head->x < -margin)continue;if(center->x>width+margin&&head->x>width+margin)continue;
   if(center->y < -margin&&head->y < -margin)continue;if(center->y>height+margin&&head->y>height+margin)continue;
  }
  rasterSilhouette(t,id,t.zed?zeds:players,width,height);++outlinedActors;
 }
 if(zed)for(auto edge:zeds.edges()){auto e=edge.line;line(e.x1,e.y1,e.x2,e.y2,settings.zed.outlineColor);++outlinedSegments;}
 if(player)for(auto edge:players.edges()){auto e=edge.line;line(e.x1,e.y1,e.x2,e.y2,settings.player.outlineColor);++outlinedSegments;}
 static unsigned measuredFrames=0;static double totalMs=0,peakMs=0;LARGE_INTEGER ended{};QueryPerformanceCounter(&ended);
 const double elapsed=1000.*double(ended.QuadPart-started.QuadPart)/double(frequency.QuadPart);totalMs+=elapsed;peakMs=std::max(peakMs,elapsed);
 if(++measuredFrames==300){log("Outline timing | avg ms "+std::to_string(totalMs/measuredFrames)+" | peak ms "+std::to_string(peakMs)+" | actors "+std::to_string(outlinedActors)+" | segments "+std::to_string(outlinedSegments));measuredFrames=0;totalMs=peakMs=0;}
}

void draw(){
 const float width=float(get<int>(canvas,"SizeX")),height=float(get<int>(canvas,"SizeY"));if(width<=0||height<=0)return;
 drawSilhouettes(width,height);
 for(const auto& t:targets){const auto& e=t.zed?settings.zed:settings.player;if(!e.enabled)continue;
  auto feet=project(t.position-Vec{0,0,get<float>(t.object,"BaseEyeHeight")}),head=project(t.head+Vec{0,0,8});if(!feet||!head)continue;

  if(e.snaplines)line(width*.5f,e.origin==0?0:e.origin==1?height*.5f:height,feet->x,feet->y,e.line);
  if(e.health){
   const float ratio=std::clamp(float(t.health)/t.maxHealth,0.f,1.f);
   const bool vertical=e.healthPosition<2;
   const float bodyHeight=std::max(12.f,std::abs(feet->y-head->y));
   const float extent=vertical?std::clamp(bodyHeight,12.f,500.f):48.f;
   const float x=vertical?(head->x+feet->x)*.5f+(e.healthPosition==0?-bodyHeight*.25f-8:bodyHeight*.25f+8):head->x-24;
   const float y=vertical?std::min(head->y,feet->y):e.healthPosition==3?std::max(head->y,feet->y)+7:std::min(head->y,feet->y)-9;
   for(int i=0;i<4;++i)if(vertical)line(x+i,y,x+i,y+extent,{20,20,26,255});else line(x,y+i,x+extent,y+i,{20,20,26,255});
   const int pixels=int(extent*ratio);
   const auto c=e.gradient?healthTint(e.low,e.high,ratio):e.healthColor;
   if(pixels>0)for(int stroke=0;stroke<4;++stroke){
    if(vertical)line(x+stroke,y+extent-pixels,x+stroke,y+extent,c);
    else line(x,y+stroke,x+pixels,y+stroke,c);
   }
  }
  if(e.skeleton){
   std::set<std::pair<int,int>> drawn;
   const auto pelvis=get<Name>(t.object,"PelvisBoneName");
   for(auto key:{"HeadBoneName","LeftHandBoneName","RightHandBoneName","LeftFootBoneName","RightFootBoneName","TorsoBoneName","PelvisBoneName"}){
    auto n=get<Name>(t.object,key);
    for(int depth=0;n.index&&n.index!=pelvis.index&&depth<24;++depth){
     Call parent(t.mesh,"GetParentBone");parent.set("BoneName",n);if(!parent.run(t.mesh))break;
     auto pn=parent.result<Name>();const auto label=name(pn.index);
     if(!pn.index||pn.index==n.index)break;
     // Some rigs attach spine and pelvis to the same motion root. Bridge the
     // body chain to the pelvis without drawing the motion root itself.
     if(pn.index!=pelvis.index&&(label=="Root"||label=="root"||label=="Bip01"||label=="Origin")){if(!pelvis.index)break;pn=pelvis;}
     if(!drawn.insert({n.index,pn.index}).second)break;
     auto a=project(bone(t.mesh,n)),b=project(bone(t.mesh,pn));
     if(a&&b&&std::signbit(a->z)==std::signbit(b->z))line(a->x,a->y,b->x,b->y,e.skeletonColor);n=pn;
    }
   }
  }
 }
 if(settings.lootEsp)for(const auto& item:loot)if(auto p=project(get<Vec>(item.object,"Location"))){
  const auto c=settings.lootColor;
  line(p->x,p->y-6,p->x+6,p->y,c);line(p->x+6,p->y,p->x,p->y+6,c);
  line(p->x,p->y+6,p->x-6,p->y,c);line(p->x-6,p->y,p->x,p->y-6,c);
 }
 if(settings.showFov){const float cameraFov=std::clamp(renderFov,1.f,170.f);float radius=settings.fov>=179?std::hypot(width,height)*.5f:std::tan(settings.fov*.5f*std::numbers::pi_v<float>/180)/std::tan(cameraFov*.5f*std::numbers::pi_v<float>/180)*width*.5f;radius=std::min(radius,std::hypot(width,height)*.5f);for(int i=0;i<128;++i){float a=i*2*std::numbers::pi_v<float>/128,b=(i+1)*2*std::numbers::pi_v<float>/128;line(width*.5f+std::cos(a)*radius,height*.5f+std::sin(a)*radius,width*.5f+std::cos(b)*radius,height*.5f+std::sin(b)*radius,settings.fovColor);}}
}
#pragma pack(push,4)
struct ShotImpact {Ptr actor{};Vec location{},normal{},ray{},start{};std::array<unsigned char,40> info{};};
#pragma pack(pop)
static_assert(sizeof(ShotImpact)==96);
bool localWeaponCall(Ptr weapon){
 DWORD focused{};GetWindowThreadProcessId(GetForegroundWindow(),&focused);
 return !menuActive&&!stopRequested&&focused==GetCurrentProcessId()&&world&&field(objectClass(world),"NetMode")&&
  get<unsigned char>(world,"NetMode")==0&&get<int>(pawn,"Health")>0&&get<Ptr>(controller,"Pawn")==pawn&&
  get<Ptr>(pawn,"Weapon")==weapon&&get<Ptr>(weapon,"Instigator")==pawn&&GetTickCount64()-targetUpdated<250;
}
void wallTrace(Ptr object,Ptr stack,void* result){
 if(!result||!settings.wallShots||!localWeaponCall(object)||editingWallShot)return;
 const auto fn=read<Ptr>(stack+0x14),locals=read<Ptr>(stack+0x2c);
 const auto startField=field(fn,"StartTrace"),endField=field(fn,"EndTrace"),listField=field(fn,"ImpactList"),returnField=field(fn,"ReturnValue");
 if(!locals||!startField||!endField||!listField||!returnField||returnField.size!=sizeof(ShotImpact))return;
 const auto start=read<Vec>(locals+startField.offset),end=read<Vec>(locals+endField.offset),delta=end-start;
 const float range=length(delta);if(!finite(start)||!finite(end)||range<.01f)return;
 // Out arrays live in the caller. FFrame.OutParms and FOutParmRec layout are
 // verified from ProcessEvent construction; never allocate/free engine arrays.
 Ptr arrayAddress{};for(auto record=read<Ptr>(stack+0x3c),n=Ptr{};record&&n<32;record=read<Ptr>(record+16),++n)
  if(read<Ptr>(record)==listField.object){arrayAddress=read<Ptr>(record+8);break;}
 if(!arrayAddress)return;const auto impacts=read<Array>(arrayAddress);
 if(!impacts.data||impacts.count<1||impacts.count>128||impacts.capacity<impacts.count)return;
 struct Guard{Guard(){editingWallShot=true;}~Guard(){editingWallShot=false;}} guard;
 std::optional<ShotImpact> nearest;float best=range+1;
 for(const auto& target:targets){
  if(!target.zed||get<int>(target.object,"Health")<=0||flag(target.object,"bDeleteMe"))continue;
  Call trace(pawn,"TraceComponent");
  if(!trace.set("InComponent",target.mesh)||!trace.set("TraceStart",start)||!trace.set("TraceEnd",end)||!trace.set("bComplexCollision",1u)||!trace.run(pawn)||!trace.result<unsigned>())continue;
  auto location=trace.result<Vec>("HitLocation");const float distance=length(location-start);
  if(!finite(location)||distance>=best)continue;
  ShotImpact hit{target.object,location,trace.result<Vec>("HitNormal"),delta*(1/range),start,trace.result<std::array<unsigned char,40>>("HitInfo")};
  best=distance;nearest=hit;
 }
 if(!nearest)return;
 // Replace the blocked trace with a real character-component hit. The normal
 // weapon code still resolves hit zones, damage, ammo and impact effects.
 if(write(impacts.data,&*nearest,sizeof(ShotImpact))){const int one=1;write(arrayAddress+8,&one,sizeof(one));write(reinterpret_cast<Ptr>(result),&*nearest,sizeof(ShotImpact));
  static unsigned hits{};if(++hits<=3||hits%100==0)log("Wall-shot character hit | count "+std::to_string(hits));}
}
// KF2 x64 FFrame offsets verified against the live script executor: Node +0x14,
// Object +0x1c, Code +0x24, Locals +0x2c. Script calls bypass ProcessEvent,
// so intercept the executor and change only the local player's aim result.
void scriptHook(Ptr object,Ptr stack,void* result){
 const Ptr fn=read<Ptr>(stack+0x14);
 if(running&&fn&&fn==flyingMoveFunction){
  std::lock_guard lock(stateMutex);scriptEvent(object,stack,result);
  if(object==controller&&world&&get<unsigned char>(world,"NetMode")==0){const auto f=field(fn,"DeltaTime");const auto locals=read<Ptr>(stack+0x2c);if(f&&locals)moveNoclip(read<float>(locals+f.offset));}return;
 }
 if(running&&fn&&fn==weaponTraceFunction&&!editingWallShot){
  std::lock_guard lock(stateMutex);scriptEvent(object,stack,result);wallTrace(object,stack,result);return;
 }
 if(running&&fn&&fn==spawnFunction.load(std::memory_order_relaxed)){
  std::lock_guard lock(stateMutex);scriptEvent(object,stack,result);
  if(result&&settings.wallShots&&localWeaponCall(object)){
   const auto projectile=read<Ptr>(reinterpret_cast<Ptr>(result));
   if(isA(projectile,"KFProjectile")&&get<Ptr>(projectile,"Instigator")==pawn&&wallProjectiles.size()<512){
    wallProjectiles.push_back({projectile,objectClass(projectile),read<Name>(projectile+0x48)});patchFlag(projectile,"bCollideWorld",false);
    static unsigned shots{};if(++shots<=3||shots%100==0)log("Wall-shot projectile | count "+std::to_string(shots));
   }
  }return;
 }
 if(running&&fn&&fn==projectileFunction.load(std::memory_order_relaxed)){
  struct Scope {Ptr previous;~Scope(){projectileWeapon=previous;}} scope{projectileWeapon};
  projectileWeapon=object;scriptEvent(object,stack,result);return;
 }
 if(!running||editingShot||!fn||fn!=aimFunction.load(std::memory_order_relaxed)){
  scriptEvent(object,stack,result);return;
 }
 std::lock_guard lock(stateMutex);
 scriptEvent(object,stack,result);
 if(!result||!settings.aim||!settings.silent||menuActive||!selectedTarget||
    GetTickCount64()-targetUpdated>250||stopRequested||!world||
    !field(objectClass(world),"NetMode")||get<unsigned char>(world,"NetMode")!=0||
    object!=get<Ptr>(pawn,"Weapon")||get<Ptr>(object,"Instigator")!=pawn||
    get<int>(pawn,"Health")<=0)return;
 DWORD foreground{};GetWindowThreadProcessId(GetForegroundWindow(),&foreground);
 if(foreground!=GetCurrentProcessId()||!isA(selectedTarget,"KFPawn_Monster")||
    get<Ptr>(selectedTarget,"WorldInfo")!=world||get<int>(selectedTarget,"Health")<=0||flag(selectedTarget,"bDeleteMe"))return;
 const auto locals=read<Ptr>(stack+0x2c);const auto start=field(fn,"StartFireLoc"),ret=field(fn,"ReturnValue");
 if(!locals||!start||start.size!=sizeof(Vec)||!ret||ret.size!=sizeof(Rot))return;
 editingShot=true;
 auto mesh=get<Ptr>(selectedTarget,"Mesh");Vec point=bone(mesh,get<Name>(selectedTarget,"HeadBoneName"));
 const auto position=get<Vec>(selectedTarget,"Location");
 if(!finite(point)||length(point-position)>1000||length(point-position)<1)point=position+Vec{0,0,get<float>(selectedTarget,"BaseEyeHeight")};
 const auto origin=read<Vec>(locals+start.offset);
 // Recheck visibility at firing time; the HUD target can be a frame old.
 bool clear=true;
 if(settings.visibility)clear=visiblePoint(selectedTarget,origin,point);
 if(clear&&projectileWeapon==object){
  Call projectileClass(object,"GetKFProjectileClass");
  if(projectileClass.run(object)){
   const auto cls=projectileClass.result<Ptr>(),defaults=read<Ptr>(cls+0x1d4);
   // UClass default pointer offset verified on the live Steam crossbow; validate
   // both class and default name before reading any projectile properties.
   if(cls&&defaults&&objectClass(defaults)==cls&&objectName(defaults)=="Default__"+objectName(cls)){
    const float speed=get<float>(defaults,"Speed");
    if(speed>1&&speed<10000000)if(auto predicted=interceptPoint(origin,point,get<Vec>(selectedTarget,"Velocity"),speed))point=*predicted;
   }
  }
 }
 if(clear)if(auto desired=shotRotation(origin,point)){
  if(write(reinterpret_cast<Ptr>(result),&*desired,sizeof(Rot))){++silentWrites;++status.aimWrites;status.aimState=5;
   if(silentWrites<=3||silentWrites%100==0)log("Silent aim result updated | count "+std::to_string(silentWrites));}
 }
 editingShot=false;
}
Ptr scriptExecutor(){
 Ptr aim{},trace{};const int count=read<int>(objects+8);const auto data=read<Ptr>(objects);
 if(count<1||count>2000000||!data)return 0;
 for(int i=0;i<count;++i){const auto object=read<Ptr>(data+8ull*i);
  if(objectName(objectClass(object))=="Function"&&objectName(object)=="PlayerMove"&&objectName(read<Ptr>(object+0x40))=="PlayerFlying")flyingMoveFunction=object;
  if(objectName(read<Ptr>(object+0x40))!="Weapon"||objectName(objectClass(object))!="Function")continue;
  const auto label=objectName(object);if(label=="GetAdjustedAim")aim=object;if(label=="CalcWeaponFire")trace=object;
 }
 weaponTraceFunction=trace;
 const Ptr address=read<Ptr>(aim+0xf0);if(!aim||!trace||!address||address!=read<Ptr>(trace+0xf0))return 0;
 // Validate the known executor ABI before using FFrame offsets.
 unsigned char code[0x36]{};
 if(!copyReadable(address,code,sizeof(code))||!scriptExecutorAbi(code,sizeof(code)))return 0;
 return address;
}
thread_local bool inside{};
void hook(Ptr object,Ptr functionObject,void* parameters,void* result){
 processEvent(object,functionObject,parameters,result);
 if(!running||inside)return;
 std::lock_guard eventLock(stateMutex);
 if(objectName(functionObject)!="PostRender"||!isA(object,"HUD"))return;
 inside=true;{
  std::lock_guard lock(stateMutex);if(stopRequested.exchange(false)){endNoclip();endFov();for(auto& [a,p]:patches)p.used=false;restore();running=false;inside=false;return;}++frames;exchange();controller=get<Ptr>(object,"PlayerOwner");canvas=get<Ptr>(object,"Canvas");
  static ULONGLONG last{};auto now=GetTickCount64();float dt=last?float(now-last)*.001f:1.f/60;last=now;
  gameplay(dt);if(canvas&&world&&get<unsigned char>(world,"NetMode")==0)draw();
  if(frames==1||frames==60)log("HUD frame "+std::to_string(frames)+" | "+detail+" | pawns "+std::to_string(status.pawnCount));
 }inside=false;
}
}
extern "C" __declspec(dllexport) DWORD WINAPI KF2OverlayStart(void*){
 std::lock_guard lock(startup);if(installed){stopRequested=false;running=true;return 0;}
 wchar_t path[32768]{};GetModuleFileNameW(self,path,32768);logFile.open(std::filesystem::path(path).parent_path()/"WetsoxKF2.log",std::ios::app);
 eventAddress=initializeReflection();if(!eventAddress){log("Reflection validation failed");return ERROR_REVISION_MISMATCH;}
 guard=CreateMutexW(nullptr,FALSE,nexus::mutexName(GetCurrentProcessId()).c_str());mapping=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,sizeof(nexus::Session),nexus::sessionName(GetCurrentProcessId()).c_str());
 if(!guard||!mapping)return ERROR_NOT_ENOUGH_MEMORY;session=(nexus::Session*)MapViewOfFile(mapping,FILE_MAP_ALL_ACCESS,0,0,sizeof(nexus::Session));if(!session)return ERROR_NOT_ENOUGH_MEMORY;
 auto wait=WaitForSingleObject(guard,1000);if(wait!=WAIT_OBJECT_0&&wait!=WAIT_ABANDONED)return ERROR_BUSY;*session=nexus::Session{};ReleaseMutex(guard);
 auto mh=MH_Initialize();if(mh!=MH_OK&&mh!=MH_ERROR_ALREADY_INITIALIZED)return ERROR_INVALID_FUNCTION;
 if(MH_CreateHook((void*)eventAddress,(void*)hook,(void**)&processEvent)!=MH_OK||MH_EnableHook((void*)eventAddress)!=MH_OK){log("Event hook failed");return ERROR_REVISION_MISMATCH;}
 scriptAddress=scriptExecutor();
 if(scriptAddress){
  auto created=MH_CreateHook((void*)scriptAddress,(void*)scriptHook,(void**)&scriptEvent);
  if(created!=MH_OK||MH_EnableHook((void*)scriptAddress)!=MH_OK){if(created==MH_OK)MH_RemoveHook((void*)scriptAddress);scriptEvent=nullptr;log("Silent aim script callback unavailable");}
  else log("Verified weapon script callback installed");
 }else log("Weapon script ABI validation failed; silent aim unavailable");
 installed=true;status.supported=1;running=true;log("Reflection ready; gameplay restricted to solo NetMode 0");return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI KF2OverlayStop(void*){stopRequested=true;return 0;}
struct OverlayStatus{unsigned size,running,initialized,visible;unsigned long long frames,resizes;};
extern "C" __declspec(dllexport) DWORD WINAPI KF2OverlayStatus(OverlayStatus* out){if(!out||out->size!=sizeof(*out))return ERROR_INVALID_PARAMETER;std::lock_guard lock(stateMutex);*out={sizeof(*out),unsigned(running),unsigned(frames>0),0,frames,0};return 0;}
extern "C" __declspec(dllexport) DWORD WINAPI KF2RuntimeStatus(fc5::runtime::Status* out){if(!out||out->size!=sizeof(*out))return ERROR_INVALID_PARAMETER;std::lock_guard lock(stateMutex);*out=status;return 0;}
BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){self=instance;DisableThreadLibraryCalls(instance);}return TRUE;}
