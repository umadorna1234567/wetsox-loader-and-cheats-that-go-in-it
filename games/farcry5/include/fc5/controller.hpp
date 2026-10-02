#pragma once
#include <windows.h>
#include <Xinput.h>
#include "sony_controller.hpp"
#include <array>
#include <cstdint>
#include <string_view>
namespace wetsox {
inline constexpr unsigned padBase=0x10000;
inline constexpr unsigned padCount=8; // Four XInput slots plus four native Sony slots.
inline constexpr std::array<std::string_view,26> padNames{
 "Pad Up","Pad Down","Pad Left","Pad Right","Pad Start","Pad Back","Pad LS","Pad RS",
 "Pad LB","Pad RB","","","Pad A","Pad B","Pad X","Pad Y",
 "Pad LT","Pad RT","Pad LS Up","Pad LS Down","Pad LS Left","Pad LS Right","Pad RS Up","Pad RS Down","Pad RS Left","Pad RS Right"};
inline unsigned padCode(std::string_view name) {
 for(unsigned i=0;i<padNames.size();++i)if(!name.empty()&&padNames[i]==name)return padBase+i;
 return 0;
}
inline std::uint32_t padButtons(const XINPUT_GAMEPAD& p) {
 std::uint32_t b=p.wButtons;
 if(p.bLeftTrigger>XINPUT_GAMEPAD_TRIGGER_THRESHOLD)b|=1u<<16;
 if(p.bRightTrigger>XINPUT_GAMEPAD_TRIGGER_THRESHOLD)b|=1u<<17;
 if(p.sThumbLY>16000)b|=1u<<18;if(p.sThumbLY<-16000)b|=1u<<19;
 if(p.sThumbLX<-16000)b|=1u<<20;if(p.sThumbLX>16000)b|=1u<<21;
 if(p.sThumbRY>16000)b|=1u<<22;if(p.sThumbRY<-16000)b|=1u<<23;
 if(p.sThumbRX<-16000)b|=1u<<24;if(p.sThumbRX>16000)b|=1u<<25;
 return b;
}
inline std::uint32_t readPad(unsigned index) {
 if(index>=padCount)return 0;
 if(index>=XUSER_MAX_COUNT)return readSonyPad(index-XUSER_MAX_COUNT);
 using GetState=DWORD(WINAPI*)(DWORD,XINPUT_STATE*);
 static const auto get=[]()->GetState {
  for(auto name:{L"xinput1_4.dll",L"xinput1_3.dll",L"xinput9_1_0.dll"})
   if(auto dll=LoadLibraryExW(name,nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32)) {
    if(auto fn=GetProcAddress(dll,"XInputGetState"))return reinterpret_cast<GetState>(fn);
    FreeLibrary(dll);
   }
  return nullptr;
 }();
 XINPUT_STATE state{};
 return get&&get(index,&state)==ERROR_SUCCESS?padButtons(state.Gamepad):0;
}
inline bool inputHeld(unsigned code) {
 if(code>=padBase&&code<padBase+padNames.size()) {
  const auto bit=1u<<(code-padBase);
  for(unsigned i=0;i<padCount;++i)if(readPad(i)&bit)return true;
  return false;
 }
 return code>0&&code<=255&&(GetAsyncKeyState(code)&0x8000);
}
}
