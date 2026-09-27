#include "fc5/session.hpp"
#include <iostream>
// Read-only snapshot of the existing launcher/module connection.
int main(int argc,char** argv) {
    if(argc!=2)return 2;
    const auto pid=static_cast<DWORD>(std::stoul(argv[1]));
    auto mutex=OpenMutexW(SYNCHRONIZE|MUTEX_MODIFY_STATE,FALSE,nexus::mutexName(pid).c_str());
    auto mapping=OpenFileMappingW(FILE_MAP_READ,FALSE,nexus::sessionName(pid).c_str());
    if(!mutex||!mapping){std::cerr<<"Session unavailable: "<<GetLastError()<<'\n';return 1;}
    auto data=static_cast<const nexus::Session*>(MapViewOfFile(mapping,FILE_MAP_READ,0,0,sizeof(nexus::Session)));
    if(!data)return 1;
    for(int i=0;i<3;++i){
        auto wait=WaitForSingleObject(mutex,1000);
        if(wait!=WAIT_OBJECT_0&&wait!=WAIT_ABANDONED)return 1;
        auto s=*data;ReleaseMutex(mutex);
        std::cout<<"version="<<s.version<<" heartbeatAge="<<GetTickCount64()-s.heartbeat
          <<" frames="<<s.frames<<" esp="<<s.settings.esp<<" bones="<<s.settings.boneEsp
          <<" aim="<<s.settings.aim.enabled<<" ammo="<<s.settings.unlimitedAmmo
          <<" menuActive="<<s.menuActive<<" supported="<<s.status.supported
          <<" entities="<<s.status.pawnCount<<" snapshots="<<s.status.snapshots
          <<" projections="<<s.status.projections<<" boneEntities="<<s.status.boneEntities
          <<" aimState="<<s.status.aimState<<'\n';
        if(i<2)Sleep(1000);
    }
    UnmapViewOfFile(data);CloseHandle(mapping);CloseHandle(mutex);
}
