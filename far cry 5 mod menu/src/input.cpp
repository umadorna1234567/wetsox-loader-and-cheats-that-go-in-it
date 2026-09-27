#define DIRECTINPUT_VERSION 0x0800
#include "fc5/input.hpp"
#include <dinput.h>
#include <MinHook.h>
#include <array>
#include <atomic>
#include <cstring>
#include <mutex>
#include <vector>
#include <algorithm>

namespace fc5::input {
namespace {
using Acquire=HRESULT(STDMETHODCALLTYPE*)(IDirectInputDevice8W*);
using State=HRESULT(STDMETHODCALLTYPE*)(IDirectInputDevice8W*,DWORD,void*);
using Data=HRESULT(STDMETHODCALLTYPE*)(IDirectInputDevice8W*,DWORD,DIDEVICEOBJECTDATA*,DWORD*,DWORD);
using Clip=BOOL(WINAPI*)(const RECT*);
using Position=BOOL(WINAPI*)(int,int);
std::array<Acquire,2> acquire{};std::array<State,2> state{};std::array<Data,2> data{};
Clip originalClip{};Position originalPosition{};
std::vector<void*> hooks;
std::atomic<bool> running{},opened{},releaseMouse{};
std::atomic<HWND> window{};
std::atomic<std::uint64_t> polls{};
std::mutex cursorMutex;
std::mutex devicesMutex;
struct SuspendedDevice {IDirectInputDevice8W* device;Acquire resume;};
std::vector<SuspendedDevice> suspended;
void resumeDevices() {
    std::vector<SuspendedDevice> pending;
    {std::lock_guard lock(devicesMutex);pending.swap(suspended);}
    for(auto entry:pending){entry.resume(entry.device);entry.device->Release();}
}
RECT savedClip{};bool restoreClip{};
bool mouseDown() {
    return (GetAsyncKeyState(VK_LBUTTON)&0x8000)||(GetAsyncKeyState(VK_RBUTTON)&0x8000)||
           (GetAsyncKeyState(VK_MBUTTON)&0x8000)||(GetAsyncKeyState(VK_XBUTTON1)&0x8000)||(GetAsyncKeyState(VK_XBUTTON2)&0x8000);
}
bool blocked() {
    if(!running)return false;
    if(opened)return true;
    if(releaseMouse) {
        if(mouseDown())return true;
        releaseMouse=false;
        resumeDevices();
    }
    return false;
}
template<int I> void unacquire(IDirectInputDevice8W* device) {
    // Release exclusive acquisition so Windows can deliver normal UI events.
    std::lock_guard lock(devicesMutex);
    if(std::none_of(suspended.begin(),suspended.end(),[&](auto entry){return entry.device==device;})) {
        device->AddRef();suspended.push_back({device,acquire[I]?acquire[I]:acquire[0]});
    }
    device->Unacquire();
}
template<int I> HRESULT STDMETHODCALLTYPE getState(IDirectInputDevice8W* device,DWORD bytes,void* output) {
    if(blocked()&&output&&bytes&&bytes<=4096) {
        unacquire<I>(device);std::memset(output,0,bytes);++polls;return DI_OK;
    }
    return state[I](device,bytes,output);
}
template<int I> HRESULT STDMETHODCALLTYPE getData(IDirectInputDevice8W* device,DWORD bytes,DIDEVICEOBJECTDATA* output,DWORD* count,DWORD flags) {
    if(blocked()&&count) {
        unacquire<I>(device);*count=0;++polls;return DI_OK;
    }
    return data[I](device,bytes,output,count,flags);
}
template<int I> HRESULT STDMETHODCALLTYPE acquireDevice(IDirectInputDevice8W* device) {
    if(blocked()){unacquire<I>(device);++polls;return DI_OK;}
    return acquire[I](device);
}
BOOL WINAPI clipCursor(const RECT* rectangle) {
    if(running&&opened&&GetForegroundWindow()==window.load()) {
        std::lock_guard lock(cursorMutex);
        restoreClip=rectangle!=nullptr;if(rectangle)savedClip=*rectangle;
        return originalClip(nullptr);
    }
    return originalClip(rectangle);
}
BOOL WINAPI setCursorPosition(int x,int y) {
    if(running&&opened&&GetForegroundWindow()==window.load())return TRUE;
    return originalPosition(x,y);
}
bool add(void* address,void* callback,void** original) {
    if(std::find(hooks.begin(),hooks.end(),address)!=hooks.end())return true;
    if(MH_CreateHook(address,callback,original)!=MH_OK)return false;
    hooks.push_back(address);return true;
}
bool prepare() {
    IDirectInput8W* input{};
    if(FAILED(DirectInput8Create(GetModuleHandleW(nullptr),DIRECTINPUT_VERSION,IID_IDirectInput8W,reinterpret_cast<void**>(&input),nullptr)))return false;
    const GUID ids[]{GUID_SysMouse,GUID_SysKeyboard};
    void* states[]{reinterpret_cast<void*>(getState<0>),reinterpret_cast<void*>(getState<1>)};
    void* datas[]{reinterpret_cast<void*>(getData<0>),reinterpret_cast<void*>(getData<1>)};
    void* acquires[]{reinterpret_cast<void*>(acquireDevice<0>),reinterpret_cast<void*>(acquireDevice<1>)};
    bool ok=true;
    for(int i=0;i<2&&ok;++i) {
        IDirectInputDevice8W* device{};
        if(FAILED(input->CreateDevice(ids[i],&device,nullptr))){ok=false;break;}
        auto vt=*reinterpret_cast<void***>(device);
        ok=add(vt[7],acquires[i],reinterpret_cast<void**>(&acquire[i]))&&
           add(vt[9],states[i],reinterpret_cast<void**>(&state[i]))&&
           add(vt[10],datas[i],reinterpret_cast<void**>(&data[i]));
        device->Release();
    }
    input->Release();
    auto user=GetModuleHandleW(L"user32.dll");
    ok=ok&&add(reinterpret_cast<void*>(GetProcAddress(user,"ClipCursor")),reinterpret_cast<void*>(clipCursor),reinterpret_cast<void**>(&originalClip))&&
           add(reinterpret_cast<void*>(GetProcAddress(user,"SetCursorPos")),reinterpret_cast<void*>(setCursorPosition),reinterpret_cast<void**>(&originalPosition));
    if(!ok){for(auto address:hooks)MH_RemoveHook(address);hooks.clear();}
    return ok;
}
}
bool start() {
    if(running)return true;
    if(hooks.empty()&&!prepare())return false;
    for(auto address:hooks)if(MH_EnableHook(address)!=MH_OK) {
        for(auto hook:hooks)MH_DisableHook(hook);return false;
    }
    polls=0;releaseMouse=false;opened=false;running=true;return true;
}
void menu(bool open,HWND target) {
    window=target;
    if(!running)return;
    const bool prior=opened.exchange(open);
    if(open&&!prior) {
        releaseMouse=false;
        if(GetForegroundWindow()==target) {
            std::lock_guard lock(cursorMutex);restoreClip=GetClipCursor(&savedClip)!=FALSE;originalClip(nullptr);
        }
        ReleaseCapture();
    } else if(!open&&prior) {
        releaseMouse=mouseDown();
        if(!releaseMouse)resumeDevices();
        std::lock_guard lock(cursorMutex);
        if(restoreClip&&GetForegroundWindow()==target)originalClip(&savedClip);
        restoreClip=false;
    }
    if(open&&GetForegroundWindow()==target)originalClip(nullptr);
}
void stop() {
    menu(false,window);running=false;opened=false;releaseMouse=false;
    resumeDevices();
    for(auto address:hooks)MH_DisableHook(address);
}
bool capturing(){return running&&opened;}
std::uint64_t blockedPolls(){return polls;}
}
