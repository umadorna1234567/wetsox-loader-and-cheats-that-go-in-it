#include <windows.h>
#include <tlhelp32.h>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <cstdint>
#include "fc5/runtime.hpp"

namespace {
struct Handle {
    HANDLE value{};
    ~Handle(){if(value&&value!=INVALID_HANDLE_VALUE) CloseHandle(value);}
};
[[noreturn]] void fail(const std::string& text) {
    throw std::runtime_error(text+" (Windows error "+std::to_string(GetLastError())+")");
}
struct Module { std::wstring path;std::uintptr_t base{}; };
Module module(DWORD pid,const std::wstring& name) {
    Handle snapshot;
    for(unsigned attempt=0;attempt<50;++attempt) {
        snapshot.value=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,pid);
        if(snapshot.value!=INVALID_HANDLE_VALUE||GetLastError()!=ERROR_BAD_LENGTH)break;
        // A target loading system DLLs can change its module list mid-snapshot.
        if(attempt!=49)Sleep(10);
    }
    if(snapshot.value==INVALID_HANDLE_VALUE) fail("Cannot enumerate target modules");
    MODULEENTRY32W entry{sizeof(entry)};
    for(BOOL ok=Module32FirstW(snapshot.value,&entry);ok;ok=Module32NextW(snapshot.value,&entry))
        if(_wcsicmp(entry.szModule,name.c_str())==0) return {entry.szExePath,reinterpret_cast<std::uintptr_t>(entry.modBaseAddr)};
    return {};
}
DWORD findProcess(const wchar_t* name,DWORD specified) {
    Handle snapshot{CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0)};
    if(snapshot.value==INVALID_HANDLE_VALUE) fail("Cannot enumerate processes");
    std::vector<DWORD> matches;PROCESSENTRY32W entry{sizeof(entry)};
    for(BOOL ok=Process32FirstW(snapshot.value,&entry);ok;ok=Process32NextW(snapshot.value,&entry))
        if(_wcsicmp(entry.szExeFile,name)==0&&(!specified||specified==entry.th32ProcessID)) matches.push_back(entry.th32ProcessID);
    if(matches.empty()) throw std::runtime_error("Target is not running. Start the game and load an offline save first.");
    if(matches.size()!=1) throw std::runtime_error("Multiple targets found. Use --pid with the intended process ID.");
    return matches[0];
}
DWORD invoke(HANDLE process,std::uintptr_t address,void* argument) {
    Handle thread{CreateRemoteThread(process,nullptr,0,reinterpret_cast<LPTHREAD_START_ROUTINE>(address),argument,0,nullptr)};
    if(!thread.value) fail("Could not create the DLL entry thread");
    DWORD wait=WaitForSingleObject(thread.value,30000);
    if(wait!=WAIT_OBJECT_0) throw std::runtime_error("Target call did not finish within 30 seconds. Remote argument memory is retained because the thread may still use it. Do not retry until the game exits.");
    DWORD code{};if(!GetExitCodeThread(thread.value,&code)) fail("Could not read entry result");
    return code;
}
void* copyToProcess(HANDLE process,const void* data,SIZE_T bytes) {
    void* memory=VirtualAllocEx(process,nullptr,bytes,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    if(!memory) fail("Could not allocate DLL arguments");
    SIZE_T written{};
    if(!WriteProcessMemory(process,memory,data,bytes,&written)||written!=bytes) {
        VirtualFreeEx(process,memory,0,MEM_RELEASE);fail("Could not copy DLL arguments");
    }
    return memory;
}
std::uintptr_t remoteLoadLibrary(DWORD pid) {
    auto function=GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"LoadLibraryW");
    if(!function) fail("LoadLibraryW unavailable");
    HMODULE owner{};
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(function),&owner)) fail("Cannot identify LoadLibraryW module");
    wchar_t path[32768]{};GetModuleFileNameW(owner,path,32768);
    auto remote=module(pid,std::filesystem::path(path).filename().wstring());
    if(!remote.base) throw std::runtime_error("Target system loader module unavailable.");
    return remote.base+reinterpret_cast<std::uintptr_t>(function)-reinterpret_cast<std::uintptr_t>(owner);
}
struct Image {
    HMODULE value{};
    ~Image(){if(value) FreeLibrary(value);}
    std::uintptr_t offset(const char* name) {
        auto symbol=GetProcAddress(value,name);
        if(!symbol) throw std::runtime_error(std::string("Missing DLL entry: ")+name);
        return reinterpret_cast<std::uintptr_t>(symbol)-reinterpret_cast<std::uintptr_t>(value);
    }
};
struct Status { std::uint32_t size{sizeof(Status)},running{},initialized{},visible{};std::uint64_t frames{},resizes{}; };
}

int wmain(int argc,wchar_t** argv) try {
    std::wstring game=L"farcry5";
    bool test=false,stop=false,status=false,runtimeStatus=false;DWORD pid{};std::filesystem::path dll;
    for(int i=1;i<argc;++i) {
        std::wstring arg=argv[i];
        if(arg==L"--test-host") test=true;
        else if(arg==L"--game"&&i+1<argc) game=argv[++i];
        else if(arg==L"--stop") stop=true;
        else if(arg==L"--status") status=true;
        else if(arg==L"--runtime-status") runtimeStatus=true;
        else if(arg==L"--pid"&&i+1<argc) pid=std::stoul(argv[++i]);
        else if(arg==L"--help") {
            std::cout<<"WetsoxGameLoader [--stop | --status] [--pid N] [--test-host] [DLL path]\n";return 0;
        } else if(arg.starts_with(L"--")||!dll.empty()) throw std::runtime_error("Invalid command. Use --help.");
        else dll=arg;
    }
    if(game!=L"farcry5"&&game!=L"farcry4") throw std::runtime_error("Unsupported game backend.");
    const bool fc4=game==L"farcry4";
    if(static_cast<int>(stop)+status+runtimeStatus>1) throw std::runtime_error("Choose --stop, --status, or --runtime-status.");
    if(dll.empty()) {wchar_t self[32768]{};GetModuleFileNameW(nullptr,self,32768);dll=std::filesystem::path(self).parent_path()/(fc4?L"cheats/farcry4/WetsoxFC4.dll":FC5_DLL_FILENAME);}
    dll=std::filesystem::canonical(dll);
    Image image{LoadLibraryExW(dll.c_str(),nullptr,DONT_RESOLVE_DLL_REFERENCES)};
    if(!image.value) fail("Cannot inspect DLL exports");
    auto startOffset=image.offset(fc4?"FC4OverlayStart":"FC5OverlayStart"),stopOffset=image.offset(fc4?"FC4OverlayStop":"FC5OverlayStop"),statusOffset=image.offset(fc4?"FC4OverlayStatus":"FC5OverlayStatus");
    pid=findProcess(test?L"NexusFC5TestHost.exe":fc4?L"FarCry4.exe":L"FarCry5.exe",pid);
    if(module(pid,L"NexusFC5.dll").base||module(pid,L"NexusFC5_v2.dll").base||module(pid,L"NexusFC5_v3.dll").base||module(pid,L"NexusFC5_v4.dll").base||module(pid,L"NexusFC5_v5.dll").base) throw std::runtime_error("Restart Far Cry 5 before loading this update; the previous Nexus module is still in memory.");
    Handle process{OpenProcess(PROCESS_CREATE_THREAD|PROCESS_QUERY_INFORMATION|PROCESS_VM_OPERATION|PROCESS_VM_WRITE|PROCESS_VM_READ|SYNCHRONIZE,FALSE,pid)};
    if(!process.value) fail("Cannot open target process");
    USHORT machine{},native{};
    if(!IsWow64Process2(process.value,&machine,&native)||machine!=IMAGE_FILE_MACHINE_UNKNOWN||native!=IMAGE_FILE_MACHINE_AMD64)
        throw std::runtime_error("Target must be a native x64 process.");
    auto remote=module(pid,dll.filename().wstring());
    if(remote.base&&_wcsicmp(std::filesystem::weakly_canonical(remote.path).c_str(),dll.c_str())!=0)
        throw std::runtime_error("A DLL with this filename is loaded from a different path. Restart the game first.");
    if(!remote.base) {
        if(stop||status||runtimeStatus) {std::cout<<"Overlay DLL is not loaded.\n";return 0;}
        auto path=dll.wstring();void* argument=copyToProcess(process.value,path.c_str(),(path.size()+1)*sizeof(wchar_t));
        invoke(process.value,remoteLoadLibrary(pid),argument);
        VirtualFreeEx(process.value,argument,0,MEM_RELEASE);
        remote=module(pid,dll.filename().wstring());
        if(!remote.base) throw std::runtime_error("The target could not load the DLL. Check DLL dependencies and target permissions.");
    }
    if(runtimeStatus) {
        fc5::runtime::Status output;
        auto offset=image.offset(fc4?"FC4RuntimeStatus":"FC5RuntimeStatus");
        auto argument=copyToProcess(process.value,&output,sizeof(output));
        DWORD code=invoke(process.value,remote.base+offset,argument);
        SIZE_T read{};BOOL ok=ReadProcessMemory(process.value,argument,&output,sizeof(output),&read);
        VirtualFreeEx(process.value,argument,0,MEM_RELEASE);
        if(code||!ok||read!=sizeof(output)) throw std::runtime_error("Cannot retrieve runtime status.");
        std::cout<<"Supported: "<<output.supported<<" | Entities: "<<output.pawnCount<<" | Animals: "<<output.animalCount<<" | Snapshots: "<<output.snapshots
                 <<" | Projections: "<<output.projections<<" | Mismatches: "<<output.mismatches
                 <<" | Position: "<<output.localPosition[0]<<", "<<output.localPosition[1]<<", "<<output.localPosition[2]
                 <<" | Magazine hook: "<<output.magazineHook<<" | Magazine calls: "<<output.magazineCalls<<" | Preserved: "<<output.preservedRounds<<" | Clip: "<<output.magazine
                 <<" | Reserve hook: "<<output.unlimitedHook<<" | Reserve overrides: "<<output.unlimitedQueries<<" | Bone entities: "<<output.boneEntities
                 <<" | Aim state: "<<output.aimState<<" | Aim writes: "<<output.aimWrites<<" | Alignment: "<<output.centerError
                 <<" | Aim target: "<<std::hex<<output.aimTarget<<std::dec
                 <<" | Aim rejects physics/player/mounted/target/alignment/cover: "<<output.aimReasons[1]<<", "<<output.aimReasons[2]<<", "<<output.aimReasons[3]<<", "<<output.aimReasons[4]<<", "<<output.aimReasons[6]<<", "<<output.aimReasons[7]
                 <<" | Last alignment rejection: "<<output.lastAlignmentFailure
                 <<" | Cover obstruction/missing target: "<<output.coverObstructions<<", "<<output.coverMissingTarget
                 <<" | Last blocker/target: "<<std::hex<<output.lastBlocker<<"/"<<output.lastBlockedTarget<<std::dec
                 <<" | Visibility queries: "<<output.visibilityQueries<<" | Clear: "<<output.visibilityClear<<" | Blocked/unconfirmed: "<<output.visibilityBlocked
                 <<" | Launch hook: "<<output.launchHook<<" | Launch samples: "<<output.launchSamples<<" | Physics: "<<output.launchPhysics
                 <<" | Bullet speed/gravity/drop: "<<output.projectileSpeed<<", "<<output.projectileGravity<<", "<<output.projectileDropDistance
                 <<" | Muzzle: "<<output.launchOrigin[0]<<", "<<output.launchOrigin[1]<<", "<<output.launchOrigin[2]
                 <<" | Inherited velocity: "<<output.launchInheritedVelocity[0]<<", "<<output.launchInheritedVelocity[1]<<", "<<output.launchInheritedVelocity[2]
                 <<" | Active speed/gravity/drop: "<<output.activeSpeed<<", "<<output.activeGravity<<", "<<output.activeDrop
                 <<" | Step: "<<output.simulationStep<<" | Flight: "<<output.flightSeconds
                 <<" | Vehicle FOV state: "<<output.vehicleFovState<<" | Vehicle FOV: "<<output.vehicleFovDegrees
                 <<" | Menu capture: "<<output.menuCapturing<<" | Blocked input polls: "<<output.blockedInputPolls
                 <<" | Calls: "<<output.calls<<" | Caller RVA: "<<std::hex<<output.lastCaller<<" | Camera: "<<output.lastCamera<<'\n';
        return 0;
    }
    if(status) {
        Status output;auto argument=copyToProcess(process.value,&output,sizeof(output));
        DWORD code=invoke(process.value,remote.base+statusOffset,argument);
        SIZE_T read{};BOOL ok=ReadProcessMemory(process.value,argument,&output,sizeof(output),&read);
        VirtualFreeEx(process.value,argument,0,MEM_RELEASE);
        if(code||!ok||read!=sizeof(output)) throw std::runtime_error("Cannot retrieve overlay status.");
        std::cout<<"Running: "<<output.running<<" | DX11 ready: "<<output.initialized<<" | Visible: "<<output.visible
                 <<" | Frames: "<<output.frames<<" | Resizes: "<<output.resizes<<'\n';return 0;
    }
    DWORD code=invoke(process.value,remote.base+(stop?stopOffset:startOffset),nullptr);
    if(code==ERROR_REVISION_MISMATCH) throw std::runtime_error("Unsupported game engine build, or game is still loading. No features enabled.");
    if(code) throw std::runtime_error("Wetsox module returned Windows error "+std::to_string(code));
    std::cout<<"PID="<<pid<<"\n";
    std::cout<<(stop?"Overlay stopped. DLL stays resident until target exit.\n":"Hooks installed. Wetsox owns the Qt menu. Use --status to verify rendered frames.\n");
    return 0;
} catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
