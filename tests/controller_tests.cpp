#define NOMINMAX
#include "fc5/controller.hpp"
#include <cstdio>
int main(){
 bool ok=true;auto check=[&](bool v,const char* text){if(!v){std::fprintf(stderr,"%s\n",text);ok=false;}};
 XINPUT_GAMEPAD pad{};
 check(wetsox::padButtons(pad)==0,"Idle controller produces no held bind");
 pad.wButtons=XINPUT_GAMEPAD_A|XINPUT_GAMEPAD_RIGHT_SHOULDER;
 check((wetsox::padButtons(pad)&(1u<<(wetsox::padCode("Pad A")-wetsox::padBase)))!=0,"A binding matches runtime button state");
 check((wetsox::padButtons(pad)&(1u<<(wetsox::padCode("Pad RB")-wetsox::padBase)))!=0,"Shoulder binding matches runtime state");
 pad={};pad.bLeftTrigger=XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
 check(wetsox::padButtons(pad)==0,"Trigger resting noise does not activate");
 ++pad.bLeftTrigger;
 check(wetsox::padButtons(pad)==(1u<<16),"LT activates above threshold");
 pad={};pad.sThumbRX=20000;pad.sThumbLY=-20000;
 check((wetsox::padButtons(pad)&((1u<<25)|(1u<<19)))==((1u<<25)|(1u<<19)),"Stick direction binds use signed axes");
 for(unsigned i=0;i<wetsox::padNames.size();++i)if(!wetsox::padNames[i].empty())check(wetsox::padCode(wetsox::padNames[i])==wetsox::padBase+i,"Every recorded name round-trips to its runtime code");
 check(wetsox::padCode("")==0&&wetsox::padCode("invalid")==0,"Unbound and invalid names stay disabled");
 DIJOYSTATE2 sony{};
 sony.lX=sony.lY=sony.lZ=sony.lRz=32767; sony.rgdwPOV[0]=0xffffffff;
 check(wetsox::sonyButtons(sony)==0,"Resting Sony controller does not trigger binds");
 sony.rgbButtons[1]=0x80;
 check(wetsox::sonyButtons(sony)==(1u<<12),"DualSense Cross maps to existing Pad A binding");
 sony.rgbButtons[1]=0; sony.rgbButtons[6]=0x80;
 check(wetsox::sonyButtons(sony)==(1u<<16),"Sony L2 uses the same runtime binding as Xbox LT");
 sony.rgbButtons[6]=0; sony.lRx=8000;
 check(wetsox::sonyButtons(sony)==(1u<<16),"Sony analog trigger threshold matches Xbox");
 sony.lRx=0; sony.rgdwPOV[0]=4500;
 check(wetsox::sonyButtons(sony)==((1u<<0)|(1u<<3)),"Sony diagonal D-pad maps to both directions");
 sony.rgdwPOV[0]=0xffffffff; sony.lZ=60000;sony.lRz=1000;
 check(wetsox::sonyButtons(sony)==((1u<<22)|(1u<<25)),"Sony right stick uses Z and Rz, not trigger axes");
 check(wetsox::sonyProduct(0x0ce6054c)&&wetsox::sonyProduct(0x09cc054c),"Known DualSense and DS4 accepted");
 check(!wetsox::sonyProduct(0x028e045e),"Unrelated device layout never interpreted as Sony");
 return ok?0:1;
}
