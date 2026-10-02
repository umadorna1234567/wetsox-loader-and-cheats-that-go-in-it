#define NOMINMAX
#include "fc5/session.hpp"
#include "fc5/controller.hpp"
#include <cstdio>
#include <cstdlib>
int main(int argc,char** argv) {
 if(argc!=2)return 2;
 const auto pid=DWORD(std::strtoul(argv[1],nullptr,10));
 const auto mutex=OpenMutexW(SYNCHRONIZE|MUTEX_MODIFY_STATE,FALSE,nexus::mutexName(pid,true).c_str());
 const auto mapping=OpenFileMappingW(FILE_MAP_ALL_ACCESS,FALSE,nexus::sessionName(pid,true).c_str());
 if(!mutex||!mapping){std::puts("No Far Cry 4 session");return 1;}
 auto data=static_cast<nexus::Session*>(MapViewOfFile(mapping,FILE_MAP_ALL_ACCESS,0,0,sizeof(nexus::Session)));
 if(!data)return 1;
 auto lock=[&]{const auto r=WaitForSingleObject(mutex,1000);return r==WAIT_OBJECT_0||r==WAIT_ABANDONED;};
 if(!lock())return 1;
 const auto saved=*data;ReleaseMutex(mutex);
 if(saved.version!=nexus::sessionVersion||GetTickCount64()-saved.heartbeat<3000){std::puts("An active UI owns the session; close that connection before this temporary probe.");return 1;}
 unsigned lastState=999;bool lastHeld=false,observed=false,restore=true;unsigned long long heartbeat=saved.heartbeat;
 const auto end=GetTickCount64()+55000;
 std::puts("Temporary L2 test armed. Return to Far Cry 4, hold L2, release, and repeat.");std::fflush(stdout);
 while(GetTickCount64()<end) {
  if(!lock())break;
  if(data->heartbeat!=heartbeat){restore=false;ReleaseMutex(mutex);std::puts("Another client took ownership; stopped probe.");break;}
  data->settings=fc5::Settings{};data->settings.aim.enabled=true;data->settings.aim.smoothingSeconds=0.25;
  data->aimKey=wetsox::padCode("Pad LT");data->modifiers=0;data->menuActive=false;data->allHumans=false;
  data->heartbeat=heartbeat=GetTickCount64();const auto status=data->status;const auto frames=data->frames;
  ReleaseMutex(mutex);
  const bool held=wetsox::inputHeld(wetsox::padCode("Pad LT"));
  DWORD foreground{};GetWindowThreadProcessId(GetForegroundWindow(),&foreground);
  if(status.aimState!=lastState||held!=lastHeld) {
   std::printf("L2=%u gameForeground=%u aimState=%u writes=%llu frames=%llu entities=%u alignment=%.3f\n",held,foreground==pid,status.aimState,status.aimWrites,frames,status.pawnCount,status.centerError);std::fflush(stdout);
   lastState=status.aimState;lastHeld=held;
  }
  if(held&&foreground==pid&&status.aimState!=0)observed=true;
  Sleep(30);
 }
 if(restore&&lock()) {
  data->settings=saved.settings;data->aimKey=saved.aimKey;data->modifiers=saved.modifiers;data->menuActive=saved.menuActive;data->allHumans=saved.allHumans;data->heartbeat=saved.heartbeat;
  ReleaseMutex(mutex);
 }
 UnmapViewOfFile(data);CloseHandle(mapping);CloseHandle(mutex);
 std::printf("Controller input observed by game runtime: %s. Previous session settings %s. No config file changed.\n",observed?"YES":"NO",restore?"restored":"left to active UI");
 return observed?0:1;
}
