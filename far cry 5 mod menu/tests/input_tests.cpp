#define DIRECTINPUT_VERSION 0x0800
#include "fc5/input.hpp"
#include <dinput.h>
#include <MinHook.h>
#include <array>
#include <iostream>
#include <stdexcept>
void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
int main() try {
    check(MH_Initialize()==MH_OK,"initialize hooks");
    check(fc5::input::start(),"start capture hooks");
    IDirectInput8W* input{};IDirectInputDevice8W* mouse{},*keyboard{};
    check(SUCCEEDED(DirectInput8Create(GetModuleHandleW(nullptr),DIRECTINPUT_VERSION,IID_IDirectInput8W,reinterpret_cast<void**>(&input),nullptr)),"create input");
    check(SUCCEEDED(input->CreateDevice(GUID_SysMouse,&mouse,nullptr)),"create mouse");
    check(SUCCEEDED(input->CreateDevice(GUID_SysKeyboard,&keyboard,nullptr)),"create keyboard");
    mouse->SetDataFormat(&c_dfDIMouse2);keyboard->SetDataFormat(&c_dfDIKeyboard);
    DIMOUSESTATE2 movement{};std::array<unsigned char,256> keys{};
    check(FAILED(mouse->GetDeviceState(sizeof(movement),&movement)),"closed menu preserves unacquired device error");
    fc5::input::menu(true,nullptr);
    movement.lX=100;movement.rgbButtons[0]=0x80;keys.fill(0x80);
    check(SUCCEEDED(mouse->GetDeviceState(sizeof(movement),&movement))&&movement.lX==0&&movement.rgbButtons[0]==0,"menu suppresses mouse movement and fire");
    check(SUCCEEDED(keyboard->GetDeviceState(DWORD(keys.size()),keys.data())),"menu returns neutral keyboard");
    for(auto key:keys)check(key==0,"menu suppresses keyboard actions");
    DWORD count=8;std::array<DIDEVICEOBJECTDATA,8> events{};
    check(SUCCEEDED(mouse->GetDeviceData(sizeof(DIDEVICEOBJECTDATA),events.data(),&count,0))&&count==0,"menu drains buffered actions");
    check(SUCCEEDED(mouse->Acquire()),"exclusive reacquisition suppressed while menu open");
    fc5::input::stop();
    mouse->Unacquire();
    check(FAILED(mouse->GetDeviceState(sizeof(movement),&movement)),"stop restores original input path");
    check(fc5::input::start(),"restart hooks");fc5::input::menu(true,nullptr);
    check(SUCCEEDED(mouse->GetDeviceState(sizeof(movement),&movement)),"restart captures again");
    fc5::input::stop();
    HWND window=CreateWindowExW(0,L"STATIC",L"Input recovery test",WS_OVERLAPPED,0,0,100,100,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    check(window!=nullptr,"create input owner window");
    keyboard->Unacquire();
    check(SUCCEEDED(keyboard->SetCooperativeLevel(window,DISCL_BACKGROUND|DISCL_NONEXCLUSIVE)),"configure keyboard acquisition");
    check(SUCCEEDED(keyboard->Acquire()),"acquire keyboard before capture");
    check(fc5::input::start(),"start recovery test");
    fc5::input::menu(true,window);
    check(SUCCEEDED(keyboard->GetDeviceState(DWORD(keys.size()),keys.data())),"suspend acquired keyboard");
    fc5::input::menu(false,window);
    // Bypass the state hook to prove closing explicitly reacquired the device,
    // rather than relying on an error recovery performed by the next poll.
    fc5::input::stop();
    check(SUCCEEDED(keyboard->GetDeviceState(DWORD(keys.size()),keys.data())),"closing restores acquired keyboard");
    keyboard->Unacquire();DestroyWindow(window);
    mouse->Release();keyboard->Release();input->Release();
    // Trampolines intentionally retained, matching the overlay lifecycle.
    std::cout<<"Input capture checks passed.\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
