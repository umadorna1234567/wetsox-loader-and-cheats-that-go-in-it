#include "GameSession.h"
#include "GamePackages.h"
#include <QCoreApplication>
#include <QDir>
#include <QColor>
#include <QKeySequence>
#include <QRegularExpression>
#include <cmath>
#include <algorithm>
GameSession::GameSession(QObject* parent):QObject(parent) {
 timer_.setInterval(100); connect(&timer_,&QTimer::timeout,this,&GameSession::poll);
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
  mutex_=OpenMutexW(SYNCHRONIZE|MUTEX_MODIFY_STATE,FALSE,nexus::mutexName(pid,game_=="farcry4").c_str());
  mapping_=OpenFileMappingW(FILE_MAP_ALL_ACCESS,FALSE,nexus::sessionName(pid,game_=="farcry4").c_str());
  if(mapping_)session_=static_cast<nexus::Session*>(MapViewOfFile(mapping_,FILE_MAP_ALL_ACCESS,0,0,sizeof(nexus::Session)));
  if(!session_||!mutex_||!process_){release();message_="Could not open the Wetsox game session.";emit changed();return;}
  connected_=true;message_="Connected - waiting for game frames";timer_.start();poll();emit changed();emit ready(game_);
 });
}
GameSession::~GameSession(){detach();}
bool GameSession::preview() const{return QCoreApplication::arguments().contains("--smoke-test");}
void GameSession::launch(const QString& game){
 if(busy_)return;
 if(game!="farcry5"&&game!="farcry4"){message_="No cheat module has been added for this game yet.";emit changed();return;}
 if(preview()){emit ready(game);return;}
 if(connected_&&game_==game&&WaitForSingleObject(process_,0)==WAIT_TIMEOUT){emit ready(game);return;}
 detach();game_=game;desired_=nexus::Session{};busy_=true;message_="Loading "+QString(game=="farcry4"?"Far Cry 4":"Far Cry 5")+" module...";emit changed();
 auto dir=QCoreApplication::applicationDirPath();
 QString module;
 for(const auto& value:scanGamePackages(dir+"/cheats")) {const auto pack=value.toMap();if(pack.value("id").toString()==game&&pack.value("backend").toString()==game)module=pack.value("modulePath").toString();}
 if(module.isEmpty()){busy_=false;message_="The game pack is missing or incomplete. Put its folder in cheats/"+game+".";emit changed();return;}
 QStringList args{module,"--game",game};
#ifdef NEXUS_SESSION_TEST
 args << "--test-host";
#endif
 loader_.start(dir+"/WetsoxGameLoader.exe",args);
}
void GameSession::release(){timer_.stop();if(session_)UnmapViewOfFile(session_);session_=nullptr;
 for(auto h:{mapping_,mutex_,process_})if(h)CloseHandle(h);
 mapping_=mutex_=process_=nullptr;connected_=false;
}
void GameSession::detach(){
 if(session_){desired_=nexus::Session{};desired_.menuActive=true;poll();}
 release();emit changed();
}
void GameSession::poll(){
 if(!session_)return;
 if(WaitForSingleObject(process_,0)!=WAIT_TIMEOUT){release();message_="Game closed.";emit changed();return;}
 auto wait=WaitForSingleObject(mutex_,5);
 if(wait!=WAIT_OBJECT_0&&wait!=WAIT_ABANDONED)return;
 if(session_->version!=nexus::sessionVersion){ReleaseMutex(mutex_);release();message_="Game module version mismatch. Restart the game.";emit changed();return;}
 auto status=session_->status;auto frames=session_->frames;
 session_->settings=desired_.settings;session_->aimKey=desired_.aimKey;session_->modifiers=desired_.modifiers;
 session_->menuActive=desired_.menuActive;session_->allHumans=desired_.allHumans;
 session_->heartbeat=GetTickCount64();ReleaseMutex(mutex_);
 const QStringList aimStates{"Off","Direct aim / ballistics unavailable","Waiting for player","Vehicle camera not aligned","No eligible target","Aiming","Alignment check failed","Target covered"};
 const QStringList fovStates{"Off","Waiting for camera mode","Applied","Another camera override active","Camera write failed"};
 const QString next=frames ? QString("Connected | %1 entities | %2 | Vehicle FOV: %3 | On-foot FOV: %4%5")
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
 QString binding=v.value("aimKey","RMB").toString();desired_.modifiers=0;
 for(auto pair:{std::pair{"Ctrl+",1u},{"Shift+",2u},{"Alt+",4u},{"Meta+",8u}})if(binding.contains(pair.first)){desired_.modifiers|=pair.second;binding.remove(pair.first);}
 const QMap<QString,unsigned> keys{{"LMB",VK_LBUTTON},{"RMB",VK_RBUTTON},{"MMB",VK_MBUTTON},{"Mouse4",VK_XBUTTON1},{"Mouse5",VK_XBUTTON2},{"Space",VK_SPACE},{"Tab",VK_TAB},{"Return",VK_RETURN},{"Enter",VK_RETURN},{"Shift",VK_SHIFT},{"Ctrl",VK_CONTROL},{"Alt",VK_MENU},{"Meta",VK_LWIN},{"Up",VK_UP},{"Down",VK_DOWN},{"Left",VK_LEFT},{"Right",VK_RIGHT},{"Home",VK_HOME},{"End",VK_END},{"PgUp",VK_PRIOR},{"PgDown",VK_NEXT},{"Insert",VK_INSERT},{"CapsLock",VK_CAPITAL},{"Pause",VK_PAUSE},{"Print",VK_SNAPSHOT},{"NumLock",VK_NUMLOCK},{"ScrollLock",VK_SCROLL}};
 desired_.aimKey=keys.value(binding,0);
 if(binding.size()==1){auto key=VkKeyScanW(binding.at(0).unicode());if(key!=-1)desired_.aimKey=key&255;}
 if(binding.startsWith('F')){bool ok=false;int n=binding.mid(1).toInt(&ok);if(ok&&n>=1&&n<=24)desired_.aimKey=VK_F1+n-1;}
 poll();
}
