#define NOMINMAX
#include "fc5/controller.hpp"
#include <cstdio>
static BOOL CALLBACK inspect(const DIDEVICEINSTANCEW* info, void* context) {
 if(!wetsox::sonyProduct(info->guidProduct.Data1))return DIENUM_CONTINUE;
 auto input=static_cast<IDirectInput8W*>(context); IDirectInputDevice8W* device=nullptr;
 auto create=input->CreateDevice(info->guidInstance,&device,nullptr);
 std::wprintf(L"Sony device: %ls, product %08lx, create=%08lx\n",info->tszProductName,info->guidProduct.Data1,create);
 if(FAILED(create))return DIENUM_CONTINUE;
 auto format=device->SetDataFormat(&c_dfDIJoystick2);
 auto coop=device->SetCooperativeLevel(nullptr,DISCL_BACKGROUND|DISCL_NONEXCLUSIVE);
 auto acquired=device->Acquire(); auto poll=device->Poll(); DIJOYSTATE2 state{};
 auto read=device->GetDeviceState(sizeof(state),&state);
 std::printf("format=%08lx cooperative=%08lx acquire=%08lx poll=%08lx read=%08lx axes=%ld,%ld,%ld,%ld,%ld,%ld pov=%lu\n",format,coop,acquired,poll,read,state.lX,state.lY,state.lZ,state.lRx,state.lRy,state.lRz,state.rgdwPOV[0]);
 device->Unacquire();device->Release();return DIENUM_CONTINUE;
}
int main(int argc,char**) {
 IDirectInput8W* input=nullptr;
 if(SUCCEEDED(DirectInput8Create(GetModuleHandleW(nullptr),DIRECTINPUT_VERSION,IID_IDirectInput8W,reinterpret_cast<void**>(&input),nullptr))) {
  input->EnumDevices(DI8DEVCLASS_GAMECTRL,inspect,input,DIEDFL_ATTACHEDONLY);input->Release();
 }
 std::array<unsigned,wetsox::padCount> previous{};
 const auto end=GetTickCount64()+(argc>1?60000:100);
 do {
  for(unsigned i=0;i<wetsox::padCount;++i) {
   const auto buttons=wetsox::readPad(i);
   if(buttons!=previous[i]){
    std::printf("Controller slot %u mask %08x:",i,buttons);
    for(unsigned bit=0;bit<wetsox::padNames.size();++bit)if(buttons&(1u<<bit))std::printf(" %.*s",int(wetsox::padNames[bit].size()),wetsox::padNames[bit].data());
    std::puts("");std::fflush(stdout);previous[i]=buttons;
   }
  }
  Sleep(10);
 }while(GetTickCount64()<end);
}
