#include "AppPaths.h"
#include "GameSession.h"
#include "GamePackages.h"
#include "fc5/controller.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QColor>
#include <QKeySequence>
#include <QRegularExpression>
#include <cmath>
#include <algorithm>
namespace {
std::pair<unsigned,unsigned> parseBinding(QString binding) {
 unsigned modifiers=0,keyCode=0;
 for(auto pair:{std::pair{"Win+",8u},std::pair{"Ctrl+",1u},{"Shift+",2u},{"Alt+",4u},{"Meta+",8u}})if(binding.contains(pair.first)){modifiers|=pair.second;binding.remove(pair.first);}
 const QMap<QString,unsigned> keys{{"LMB",VK_LBUTTON},{"RMB",VK_RBUTTON},{"MMB",VK_MBUTTON},{"Mouse4",VK_XBUTTON1},{"Mouse 4",VK_XBUTTON1},{"Mouse5",VK_XBUTTON2},{"Mouse 5",VK_XBUTTON2},{"Space",VK_SPACE},{"Tab",VK_TAB},{"Return",VK_RETURN},{"Enter",VK_RETURN},{"Shift",VK_SHIFT},{"Ctrl",VK_CONTROL},{"Alt",VK_MENU},{"Meta",VK_LWIN},{"Win",VK_LWIN},{"Up",VK_UP},{"Down",VK_DOWN},{"Left",VK_LEFT},{"Right",VK_RIGHT},{"Home",VK_HOME},{"End",VK_END},{"PgUp",VK_PRIOR},{"PgDown",VK_NEXT},{"Insert",VK_INSERT},{"CapsLock",VK_CAPITAL},{"Pause",VK_PAUSE},{"Print",VK_SNAPSHOT},{"NumLock",VK_NUMLOCK},{"ScrollLock",VK_SCROLL}};
 keyCode=wetsox::padCode(binding.toStdString());
 if(!keyCode)keyCode=keys.value(binding,0);
 if(binding.size()==1){auto key=VkKeyScanW(binding.at(0).unicode());if(key!=-1)keyCode=key&255;}
 if(binding.startsWith('F')){bool ok=false;int n=binding.mid(1).toInt(&ok);if(ok&&n>=1&&n<=24)keyCode=VK_F1+n-1;}
 return {keyCode,modifiers};
}
}
GameSession::GameSession(QObject* parent):QObject(parent) {
 timer_.setInterval(16); connect(&timer_,&QTimer::timeout,this,&GameSession::poll);
 loader_.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments* a){a->flags|=CREATE_NO_WINDOW;});
 connect(&loader_,&QProcess::errorOccurred,this,[this](QProcess::ProcessError e){
  if(e==QProcess::FailedToStart){busy_=false;message_="WetsoxGameLoader.exe could not start. Rebuild the application.";emit changed();}
 });
 connect(&loader_,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this](int code,QProcess::ExitStatus status){
  busy_=false;
  const QString output=QString::fromLocal8Bit(loader_.readAllStandardOutput());
  const QString errors=QString::fromLocal8Bit(loader_.readAllStandardError());
  if(code||status!=QProcess::NormalExit){message_=errors.isEmpty()?"Game module loading failed.":errors.trimmed();emit changed();return;}
  const auto match=QRegularExpression("PID=(\\d+)").match(output);
  const DWORD pid=match.captured(1).toUInt();
  if(!pid){message_="Loader did not return a game process.";emit changed();return;}
  process_=OpenProcess(SYNCHRONIZE|PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid);
  mutex_=OpenMutexW(SYNCHRONIZE|MUTEX_MODIFY_STATE,FALSE,nexus::mutexName(pid,game_=="farcry4",game_=="justcause4",game_=="killingfloor2").c_str());
  mapping_=OpenFileMappingW(FILE_MAP_ALL_ACCESS,FALSE,nexus::sessionName(pid,game_=="farcry4",game_=="justcause4",game_=="killingfloor2").c_str());
  if(mapping_)session_=static_cast<nexus::Session*>(MapViewOfFile(mapping_,FILE_MAP_ALL_ACCESS,0,0,sizeof(nexus::Session)));
  if(!session_||!mutex_||!process_){release();message_="Could not open the matching game session. Update the loader and game pack together, then restart the game.";emit changed();return;}
  connected_=true;message_="Connected - waiting for game frames";timer_.start();poll();emit changed();emit ready(game_);
 });
}
GameSession::~GameSession(){detach();}
bool GameSession::preview() const{return QCoreApplication::arguments().contains("--smoke-test");}
void GameSession::launch(const QString& game){
 if(busy_)return;
 if(game!="farcry5"&&game!="farcry4"&&game!="justcause4"&&game!="killingfloor2"){message_="This game backend is not supported by this loader. Install the latest Wetsox Loader and the matching game pack.";emit changed();return;}
 if(preview()){emit ready(game);return;}
 if(connected_&&game_==game&&WaitForSingleObject(process_,0)==WAIT_TIMEOUT){emit ready(game);return;}
 detach();game_=game;desired_=nexus::Session{};busy_=true;message_="Loading "+QString(game=="killingfloor2"?"Killing Floor 2":game=="justcause4"?"Just Cause 4":game=="farcry4"?"Far Cry 4":"Far Cry 5")+" module...";emit changed();
 auto dir=wetsoxRoot();
 QString module;
 for(const auto& value:scanGamePackages(wetsoxCheats())) {const auto pack=value.toMap();if(pack.value("id").toString()==game&&pack.value("backend").toString()==game)module=pack.value("modulePath").toString();}
 if(module.isEmpty()){busy_=false;message_="The game pack is missing or incomplete. Put its folder in cheats/"+game+".";emit changed();return;}
 QStringList args{module,"--game",game};
#ifdef NEXUS_SESSION_TEST
 args << "--test-host";
#endif
 loader_.start(wetsoxHelper(),args);
}
void GameSession::release(){timer_.stop();if(session_)UnmapViewOfFile(session_);session_=nullptr;
 for(auto h:{mapping_,mutex_,process_})if(h)CloseHandle(h);
 mapping_=mutex_=process_=nullptr;connected_=false;
}
void GameSession::detach(){
 if(session_){desired_=nexus::Session{};desired_.menuActive=true;poll();}
 release();hotkeyStates_={};hotkeys_={};emit changed();
}
nexus::Session GameSession::effectiveSettings() {
 auto output=desired_;
 DWORD foreground{};GetWindowThreadProcessId(GetForegroundWindow(),&foreground);
 const bool active=!desired_.menuActive&&process_&&foreground==GetProcessId(process_);
 std::array<int,256> keyboard;keyboard.fill(-1);
 unsigned pads=0;
 if(active&&std::any_of(hotkeys_.begin(),hotkeys_.end(),[](auto r){return r.enabled&&r.key>=wetsox::padBase;}))
  for(unsigned i=0;i<wetsox::padCount;++i)pads|=wetsox::readPad(i);
 auto held=[&](unsigned key){
  if(key>=wetsox::padBase&&key<wetsox::padBase+wetsox::padNames.size())return bool(pads&(1u<<(key-wetsox::padBase)));
  if(!key||key>=keyboard.size())return false;
  if(keyboard[key]<0)keyboard[key]=(GetAsyncKeyState(key)&0x8000)!=0;
  return keyboard[key]!=0;
 };
 auto& s=output.settings;auto& a=s.aim;auto& j=output.jc4;
 std::array<bool*,wetsox::featureKeys.size()> fields{&a.enabled,&a.showFov,&a.targets.enemies,&a.targets.otherHumans,&a.targets.animals,&a.stickyAim,&a.travelTime,&a.prediction,&a.bulletDrop,
  &s.esp,&output.allHumans,&s.espTargets.enemies,&s.espTargets.animals,&s.boneEsp,&s.boxEsp,&s.espOutline,&s.unlimitedAmmo,&s.noReload,&s.playerFovOverride,&s.vehicleFovOverride,
  &j.god,&j.vehicleBoost,&j.boost,&j.rockets,&j.grappleRange,&j.enemiesOnly,&j.tracers};
 if(game_=="justcause4"){fields[0]=&j.aim;fields[9]=&j.esp;fields[16]=&j.ammo;}
 if(game_=="killingfloor2") {
  fields.fill(nullptr);auto& k=output.kf2;
  auto assign=[&](std::string_view name,bool* value){for(size_t i=0;i<wetsox::featureKeys.size();++i)if(wetsox::featureKeys[i]==name)fields[i]=value;};
  assign("aim",&k.aim);
  assign("showFov",&k.showFov);
  assign("ammo",&k.ammo);
  assign("reload",&k.reload);
  assign("playerFov",&k.playerFov);
  assign("god",&k.god);
  assign("zedEsp",&k.zed.enabled);
  assign("zedSkeleton",&k.zed.skeleton);
  assign("zedOutline",&k.zed.outline);
  assign("zedHealth",&k.zed.health);
  assign("zedGradient",&k.zed.gradient);
  assign("zedSnaplines",&k.zed.snaplines);
  assign("playerEsp",&k.player.enabled);
  assign("playerSkeleton",&k.player.skeleton);
  assign("playerOutline",&k.player.outline);
  assign("playerHealth",&k.player.health);
  assign("playerGradient",&k.player.gradient);
  assign("playerSnaplines",&k.player.snaplines);
  assign("silent",&k.silent);
  assign("visibility",&k.visibility);
  assign("lootEsp",&k.lootEsp);
  assign("syringe",&k.syringe);
  assign("carry",&k.carry);
  assign("rapid",&k.rapid);
  assign("spread",&k.spread);
  assign("recoil",&k.recoil);
  assign("sway",&k.sway);
  assign("wallShots",&k.wallShots);
  assign("moveSpeed",&k.moveSpeed);
  assign("noclip",&k.noclip);
 }
 for(size_t i=0;i<fields.size();++i) {
  if(!fields[i])continue;
  const auto r=hotkeys_[i];
  const bool down=held(r.key)&&(!(r.modifiers&1)||held(VK_CONTROL))&&(!(r.modifiers&2)||held(VK_SHIFT))&&(!(r.modifiers&4)||held(VK_MENU))&&(!(r.modifiers&8)||held(VK_LWIN)||held(VK_RWIN));
  *fields[i]=hotkeyStates_[i].evaluate(r,*fields[i],down,active);
 }
 // The loader has already evaluated Hold/Toggle/Always for aim. Zero is the
 // shared always-active sentinel; the DLL still requires game foreground.
 output.aimKey=0;output.modifiers=0;
 return output;
}
void GameSession::poll(){
 if(!session_)return;
 if(WaitForSingleObject(process_,0)!=WAIT_TIMEOUT){release();message_="Game closed.";emit changed();return;}
 const auto effective=effectiveSettings();
 auto wait=WaitForSingleObject(mutex_,5);
 if(wait!=WAIT_OBJECT_0&&wait!=WAIT_ABANDONED)return;
 if(session_->version!=nexus::sessionVersion){ReleaseMutex(mutex_);release();message_="Game module version mismatch. Restart the game.";emit changed();return;}
 auto status=session_->status;auto frames=session_->frames;
 const QString detail=QString::fromUtf8(session_->detail,int(strnlen(session_->detail,sizeof(session_->detail))));
 session_->kf2=effective.kf2;session_->jc4=effective.jc4;session_->settings=effective.settings;session_->aimKey=effective.aimKey;session_->modifiers=effective.modifiers;
 session_->menuActive=desired_.menuActive;session_->allHumans=effective.allHumans;
 session_->heartbeat=GetTickCount64();ReleaseMutex(mutex_);
 const QStringList aimStates{"Off","Direct aim / ballistics unavailable","Waiting for player","Vehicle camera not aligned","No eligible target","Aiming","Alignment check failed","Target covered"};
 const QStringList fovStates{"Off","Waiting for camera mode","Applied","Another camera override active","Camera write failed"};
 const QString next=(game_=="justcause4"||game_=="killingfloor2")?QString("Connected | %1 entities | %2").arg(status.pawnCount).arg(detail):frames ? QString("Connected | %1 entities | %2 | Vehicle FOV: %3 | On-foot FOV: %4%5")
  .arg(status.pawnCount).arg(aimStates.value(status.aimState,"Unknown"))
  .arg(fovStates.value(status.vehicleFovState,"Unknown")).arg(fovStates.value(status.playerFovState,"Unknown"))
  .arg(status.magazineHook&&status.unlimitedHook?"":" | Ammunition hook unavailable") : "Connected - waiting for game frames";
 if(message_!=next){message_=next;emit changed();}
}
void GameSession::update(const QVariantMap& v,bool menuActive){
 auto b=[&](const char* key,bool fallback=false){return v.value(key,fallback).toBool();};
 auto d=[&](const char* key,double fallback,double lo,double hi){bool ok=false;double n=v.value(key,fallback).toDouble(&ok);return ok&&std::isfinite(n)?std::clamp(n,lo,hi):fallback;};
 auto& s=desired_.settings;s=fc5::Settings{};auto& a=s.aim;
 a.enabled=b("aim");a.showFov=b("showFov");a.vehicleWeapons=false;a.targets={b("aimEnemies",true),b("aimHumans"),b("aimAnimals")};
 a.stickyAim=b("sticky",true);
 const QStringList bones{"Head","Chest","Abdomen","Pelvis"};a.hitLocation=static_cast<fc5::HitLocation>(std::max<qsizetype>(0,bones.indexOf(v.value("bone","Head").toString())));
 a.fovDegrees=d("fov",10,.1,180);a.smoothingSeconds=d("smooth",0,0,2);
 a.travelTime=b("travel",true);a.prediction=b("prediction",true);a.bulletDrop=b("drop",true);
 s.esp=b("esp");s.espOutline=b("espOutline",true);s.boxEsp=b("boxEsp",true);s.boneEsp=b("boneEsp");s.espTargets={b("espEnemies",true),false,b("espAnimals")};
 s.noReload=b("reload");s.unlimitedAmmo=b("ammo");s.playerFovOverride=b("playerFov");s.playerFovDegrees=d("playerDegrees",90,40,140);s.vehicleFovOverride=b("vehicleFov");s.vehicleFovDegrees=d("vehicleDegrees",90,40,140);
 desired_.allHumans=b("allHumans",true);desired_.menuActive=menuActive;
 const char* colorKeys[]{"enemyColor","humanColor","animalColor"};
 for(int i=0;i<3;++i){QColor c(v.value(colorKeys[i]).toString());if(c.isValid())s.colors[i]={static_cast<uint8_t>(c.red()),static_cast<uint8_t>(c.green()),static_cast<uint8_t>(c.blue())};}
 const auto rules=v.value("_hotkeys").toMap();
 for(size_t i=0;i<wetsox::featureKeys.size();++i) {
  const auto key=QString::fromLatin1(wetsox::featureKeys[i].data(),int(wetsox::featureKeys[i].size()));
  const auto rule=rules.value(key).toMap();
  const bool legacyAim=key=="aim"&&!rules.contains(key);
  const auto binding=parseBinding(legacyAim?v.value("aimKey","RMB").toString():rule.value("binding").toString());
  hotkeys_[i]={legacyAim||rule.value("enabled").toBool(),rule.value("mode").toString()=="Toggle",binding.first,binding.second};
 }
 const auto aimBinding=parseBinding(v.value("aimKey","RMB").toString());
 desired_.aimKey=aimBinding.first;desired_.modifiers=aimBinding.second;
 if(game_=="killingfloor2") {
  const auto sequence=desired_.kf2.actionSequence;
  auto& k=desired_.kf2;k={};k.actionSequence=sequence;
  k.aim=b("aim");
  k.silent=b("silent");
  k.showFov=b("showFov");
  k.visibility=b("visibility",true);
  k.lootEsp=b("lootEsp");
  k.god=b("god");
  k.syringe=b("syringe");
  k.carry=b("carry");
  k.rapid=b("rapid");
  k.spread=b("spread");
  k.recoil=b("recoil");
  k.sway=b("sway");
  k.wallShots=b("wallShots");
  k.reload=b("reload");
  k.ammo=b("ammo");
  k.moveSpeed=b("moveSpeed");
  k.noclip=b("noclip");
  k.playerFov=b("playerFov");
  k.smooth=d("smooth",0.12,0,2);
  k.fov=d("fov",30,1,360);
  k.fireRate=d("fireRate",3,1,10);
  k.moveMultiplier=d("moveMultiplier",1,0.25,10);
  k.noclipMultiplier=d("noclipMultiplier",1,0.25,10);
  k.playerDegrees=d("playerDegrees",90,40,140);
  k.doshAmount=d("doshAmount",1000,1,100000);
  k.priority=std::max<qsizetype>(0,QStringList{"Closest to crosshair","Closest to you","Both"}.indexOf(v.value("priority").toString()));
  auto tint=[&](const QString& key,wetsox::kf2::Color& target){QColor c(v.value(key).toString());if(c.isValid())target={quint8(c.red()),quint8(c.green()),quint8(c.blue()),255};};
  tint("lootColor",k.lootColor);tint("fovColor",k.fovColor);
  for(auto entry:{std::pair{"zed",&k.zed},std::pair{"player",&k.player}}){
   const QString prefix=entry.first;auto& e=*entry.second;
   e.enabled=v.value(prefix+"Esp",false).toBool();e.skeleton=v.value(prefix+"Skeleton",true).toBool();e.outline=v.value(prefix+"Outline",false).toBool();
   e.health=v.value(prefix+"Health",true).toBool();e.gradient=v.value(prefix+"Gradient",true).toBool();e.snaplines=v.value(prefix+"Snaplines",false).toBool();
   e.origin=std::max<qsizetype>(0,QStringList{"Top","Middle","Bottom"}.indexOf(v.value(prefix+"LineOrigin","Bottom").toString()));
   e.healthPosition=std::max<qsizetype>(0,QStringList{"Left","Right","Top","Bottom"}.indexOf(v.value(prefix+"HealthPosition","Top").toString()));
   tint(prefix+"SkeletonColor",e.skeletonColor);tint(prefix+"OutlineColor",e.outlineColor);tint(prefix+"HealthColor",e.healthColor);tint(prefix+"HealthLow",e.low);tint(prefix+"HealthHigh",e.high);tint(prefix+"LineColor",e.line);
  }
 }
 if(game_=="justcause4") {
  auto& j=desired_.jc4;
  j.god=b("god");j.ammo=b("ammo");j.boost=b("boost");j.rockets=b("rockets");j.grappleRange=b("grappleRange");j.vehicleBoost=b("vehicleBoost");
  j.esp=b("esp");j.enemiesOnly=b("enemiesOnly",true);j.tracers=b("tracers");j.aim=b("aim");
  j.speed=float(d("speed",1,.25,3));j.wingsuitSpeed=float(d("wingsuitSpeed",1,.5,100));j.hoverboardSpeed=float(d("hoverboardSpeed",1,.5,100));j.grappleSpeed=float(d("grappleSpeed",1,.5,100));
  j.range=float(d("range",300,50,1000));j.fov=float(d("fov",15,5,60));j.smooth=float(d("smooth",6,1,20));
  auto actionBinding=[&](const char* action,const char* legacy) {
   if(!rules.contains(action))return parseBinding(v.value(legacy).toString());
   const auto rule=rules.value(action).toMap();
   return parseBinding(rule.value("enabled").toBool()?rule.value("binding").toString():QString{});
  };
  auto waypoint=actionBinding("teleportWaypoint","waypointKey"),objective=actionBinding("teleportObjective","objectiveKey");
  j.waypointKey=waypoint.first;j.waypointModifiers=waypoint.second;j.objectiveKey=objective.first;j.objectiveModifiers=objective.second;
 }
 poll();
}

void GameSession::action(const QString& key) {
 if(game_=="killingfloor2"&&connected_&&key=="addDosh"){++desired_.kf2.actionSequence;poll();return;}
 if(game_!="justcause4"||!connected_)return;
 if(key!="teleportWaypoint"&&key!="teleportObjective")return;
 desired_.jc4.actionCode=key=="teleportWaypoint"?0:1;++desired_.jc4.actionSequence;poll();
}
