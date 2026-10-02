#include "game.h"
#include "menu.h"
#include "render.h"
#include "fc5/session.hpp"
#include "fc5/controller.hpp"
#include <atomic>
#include <filesystem>
#include <cstring>
namespace {
HMODULE self{};
HANDLE mapping{},guard{};
nexus::Session* session{};
std::mutex startup;
std::atomic<bool> active{},menuActive{true};
bool initialized{};
unsigned aimKey=VK_RBUTTON,aimModifiers{},lastAction{};
bool previousWaypoint{},previousObjective{};
std::atomic<unsigned long long> frames{};
bool held(unsigned key,unsigned modifiers) {
 return wetsox::inputHeld(key)&&(!(modifiers&1)||wetsox::inputHeld(VK_CONTROL))&&
 (!(modifiers&2)||wetsox::inputHeld(VK_SHIFT))&&(!(modifiers&4)||wetsox::inputHeld(VK_MENU))&&
 (!(modifiers&8)||(wetsox::inputHeld(VK_LWIN)||wetsox::inputHeld(VK_RWIN)));
}
}
namespace mod {
bool MenuEvent(HWND,UINT,WPARAM,LPARAM){return false;}
bool MenuOpen(){return menuActive;}
bool AimInputHeld(){return !aimKey||held(aimKey,aimModifiers);}
void DrawOverlay(){std::lock_guard lock(stateMutex);DrawCombat();++frames;}
void ExchangeSession() {
 nexus::Session next;bool fresh=false;
 if(session) {
  auto wait=WaitForSingleObject(guard,0);
  if(wait==WAIT_OBJECT_0||wait==WAIT_ABANDONED) {
   fresh=active&&session->version==nexus::sessionVersion&&GetTickCount64()-session->heartbeat<3000;
   if(fresh)next=*session;
   {std::lock_guard lock(stateMutex);
    session->frames=frames.load();session->status.supported=1;session->status.pawnCount=unsigned(snapshot.characters.size());
    strncpy_s(session->detail,snapshot.status.c_str(),_TRUNCATE);
   }
   ReleaseMutex(guard);
  } else return;
 }
 menuActive=!fresh||next.menuActive;
 {
  std::lock_guard lock(stateMutex);
  if(!fresh){settings={};aimKey=0;}else {
   const auto& j=next.jc4;
   settings.god=j.god;settings.ammo=j.ammo;settings.boost=j.boost;settings.rockets=j.rockets;
   settings.grappleRange=j.grappleRange;settings.vehicleBoost=j.vehicleBoost;
   settings.speed=j.speed;settings.wingsuitSpeed=j.wingsuitSpeed;settings.hoverboardSpeed=j.hoverboardSpeed;settings.grappleSpeed=j.grappleSpeed;
   settings.esp=j.esp;settings.espEnemiesOnly=j.enemiesOnly;settings.espTracers=j.tracers;
   settings.aim=j.aim;settings.espRange=j.range;settings.aimFov=j.fov;settings.aimSmooth=j.smooth;
   aimKey=next.aimKey;aimModifiers=next.modifiers;teleportKeys[2]=aimKey;
  }
 }
 if(!fresh){previousWaypoint=previousObjective=false;return;}
 if(next.jc4.actionSequence!=lastAction) {
  lastAction=next.jc4.actionSequence;
  if(next.jc4.actionSequence&&next.jc4.actionCode<=1)RequestAction(int(next.jc4.actionCode));
 }
 DWORD foreground{};GetWindowThreadProcessId(GetForegroundWindow(),&foreground);
 const bool hotkeys=!menuActive&&foreground==GetCurrentProcessId();
 const bool waypoint=hotkeys&&held(next.jc4.waypointKey,next.jc4.waypointModifiers);
 const bool objective=hotkeys&&held(next.jc4.objectiveKey,next.jc4.objectiveModifiers);
 if(waypoint&&!previousWaypoint)RequestAction(0);
 if(objective&&!previousObjective)RequestAction(1);
 previousWaypoint=waypoint;previousObjective=objective;
}
}
extern "C" __declspec(dllexport) DWORD WINAPI JC4OverlayStart(void*) {
 std::lock_guard lock(startup);
 if(initialized){active=true;return 0;}
 if(!GetModuleHandleW(L"JustCause4.exe"))return ERROR_REVISION_MISMATCH;
 wchar_t filename[32768]{};GetModuleFileNameW(self,filename,32768);
 mod::dataDirectory=std::filesystem::path(filename).parent_path().wstring();
 guard=CreateMutexW(nullptr,FALSE,nexus::mutexName(GetCurrentProcessId()).c_str());
 mapping=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,sizeof(nexus::Session),nexus::sessionName(GetCurrentProcessId()).c_str());
 if(!guard||!mapping)return ERROR_NOT_ENOUGH_MEMORY;
 session=static_cast<nexus::Session*>(MapViewOfFile(mapping,FILE_MAP_ALL_ACCESS,0,0,sizeof(nexus::Session)));
 if(!session)return ERROR_NOT_ENOUGH_MEMORY;
 const auto wait=WaitForSingleObject(guard,1000);
 if(wait!=WAIT_OBJECT_0&&wait!=WAIT_ABANDONED)return ERROR_BUSY;
 *session=nexus::Session{};ReleaseMutex(guard);
 try {if(!mod::InitializeGame())return ERROR_REVISION_MISMATCH;}
 catch(...){return ERROR_REVISION_MISMATCH;}
 initialized=true;active=true;return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI JC4OverlayStop(void*) {
 active=false;menuActive=true;
 std::lock_guard lock(mod::stateMutex);mod::settings={};return 0;
}
struct OverlayStatus {unsigned size,running,initialized,visible;unsigned long long frames,resizes;};
extern "C" __declspec(dllexport) DWORD WINAPI JC4OverlayStatus(OverlayStatus* out) {
 if(!out||out->size!=sizeof(*out))return ERROR_INVALID_PARAMETER;
 *out={sizeof(*out),unsigned(active.load()),unsigned(frames.load()>0),unsigned(active.load()),frames.load(),0};return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI JC4RuntimeStatus(fc5::runtime::Status* out) {
 if(!out||out->size!=sizeof(*out))return ERROR_INVALID_PARAMETER;
 std::lock_guard lock(mod::stateMutex);*out={};out->supported=initialized;out->pawnCount=unsigned(mod::snapshot.characters.size());return 0;
}
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,LPVOID) {
 if(reason==DLL_PROCESS_ATTACH){self=module;DisableThreadLibraryCalls(module);}return TRUE;
}
