#include "math.hpp"
#include <cstdio>
#include <limits>
int main(){
 using namespace kf2;
 int failures=0;
 auto check=[&](bool value,const char* label){if(!value){std::fprintf(stderr,"FAIL: %s\n",label);++failures;}};
 check(turn(65530)==-6,"wrap positive yaw");check(turn(-65530)==6,"wrap negative yaw");
 check(inCone(180,360),"360 includes behind");check(!inCone(91,180),"180 excludes rear hemisphere");
 check(!inCone(std::numeric_limits<float>::quiet_NaN(),360),"reject invalid angle");
 check(std::abs(angle(direction({0,0,0}),{1,0,0}))<.01f,"forward");
 check(std::abs(angle(direction({0,0,0}),{-1,0,0})-180)<.01f,"behind");
 for(Rot r: {Rot{0,32760,0},Rot{8000,-32760,0},Rot{-6000,12000,0}}){auto out=rotation(direction(r));check(std::abs(turn(out.yaw-r.yaw))<=1&&std::abs(out.pitch-r.pitch)<=1,"rotation roundtrip");}
 check(targetScore(2,100,60,100,0)<targetScore(5,10,60,100,0),"crosshair priority");
 check(targetScore(2,100,60,100,1)>targetScore(5,10,60,100,1),"distance priority");
 check(targetScore(2,10,60,100,2)<targetScore(5,100,60,100,2),"combined priority");
 auto wide=projectWorld({100,10,0},{},{},90,1920,1080),zoom=projectWorld({100,10,0},{},{},30,1920,1080);
 check(wide&&zoom&&zoom->x>wide->x,"zoom expands projected offsets");
 auto center=projectWorld({100,0,0},{},{},10,1920,1080);
 check(center&&std::abs(center->x-960)<.01f&&std::abs(center->y-540)<.01f,"center remains aligned at high magnification");
 check(!projectWorld({-1,0,0},{},{},90,1920,1080),"rear points rejected");
 Matrix identity{};for(int i=0;i<4;++i)identity.m[i*4+i]=1;
 Matrix perspective{};perspective.m[0]=6.14f;perspective.m[5]=6.14f;perspective.m[11]=1;
 auto scope=projectScope({1,0,10},identity,perspective,960,1920,1080);
 check(scope&&std::abs(scope->x-(960+6.14f/10*480))<.01f,"scope uses independent camera and lens viewport");
 check(!projectScope({10,0,10},identity,perspective,960,1920,1080),"scope clipped to circular lens");
 auto lensRest=projectWorld({13.18f,0,0},{},{},52,1920,1080),lensSway=projectWorld({13.18f,1,0},{},{},52,1920,1080);
 check(lensRest&&lensSway&&lensSway->x>lensRest->x+100,"scope lens follows translated weapon pose");
 check(insideLens({5,5,0},{5,5,0},{5,7,0},{3,5,0}),"rotated lens contains center");
 check(!insideLens({8,5,0},{5,5,0},{5,7,0},{3,5,0}),"rotated lens excludes exterior");
 check(validLensBasis({0,1,0},{0,0,1},{1,0,0}),"lens plane faces the capture camera");
 check(!validLensBasis({-1,0,0},{0,-1,0},{1,0,0}),"reject incorrectly indexed sideways lens axes");
 // Follow a target across the lens using either bone-axis sign and multiple camera headings.
 for(Rot camera: {Rot{},Rot{0,16384,0},Rot{2000,-12000,1000}}){
  const float yaw=camera.yaw*std::numbers::pi_v<float>/32768;
  const Vec captureRight{-std::sin(yaw),std::cos(yaw),0};
  const Vec lensCenter=direction(camera)*15.f;
  for(float sign: {-1.f,1.f}){
   const auto right=lensRightAxis(captureRight*sign,captureRight);
   auto left=projectWorld(lensCenter+right*(-.5f),{},camera,52,1920,1080);
   auto middle=projectWorld(lensCenter,{},camera,52,1920,1080);
   auto rightPoint=projectWorld(lensCenter+right*.5f,{},camera,52,1920,1080);
   check(left&&middle&&rightPoint&&left->x<middle->x&&middle->x<rightPoint->x,"capture left-to-right motion stays left-to-right on lens");
  }
 }
 // Rear glass preserves the validated idle viewport but follows the animated
 // lens plane when recoil tips the mount or pushes the weapon backwards.
 const Vec mount{13.18f,0,-3.f};
 const auto glass=crossbowLensCenter(mount,{1,0,0},{0,0,1});
 auto restGlass=projectWorld(glass,{},{},52,1920,1080);
 auto restGlassEdge=projectWorld(glass+Vec{0,crossbowLensRadius,0},{},{},52,1920,1080);
 auto oldEdge=projectWorld({13.18f,3.214f,0},{},{},52,1920,1080);
 check(restGlass&&restGlassEdge&&oldEdge&&std::abs(restGlass->x-960)<.01f&&std::abs(restGlass->y-540)<.01f&&std::abs(restGlassEdge->x-oldEdge->x)<.01f,"rear lens preserves resting alignment and scale");
 const float tilt=.08f;
 const Vec recoilForward{std::cos(tilt),0,std::sin(tilt)},recoilUp{-std::sin(tilt),0,std::cos(tilt)};
 auto recoiled=projectWorld(crossbowLensCenter(mount,recoilForward,recoilUp),{},{},52,1920,1080);
 auto mountPlane=projectWorld(mount+recoilUp*3.f,{},{},52,1920,1080);
 check(recoiled&&mountPlane&&recoiled->y>mountPlane->y+100,"tilted rear glass includes recoil lever arm instead of mount-plane approximation");
 auto translated=projectWorld(crossbowLensCenter(mount+Vec{2,0,0},{1,0,0},{0,0,1})+Vec{0,crossbowLensRadius,0},{},{},52,1920,1080);
 check(translated&&restGlassEdge&&translated->x<restGlassEdge->x-80,"lens viewport contracts with animated axial movement");
 struct Color{unsigned char r,g,b,a{255};};
 auto middle=healthTint(Color{255,0,0},Color{0,255,0},.5f);
 check(middle.r==255&&middle.g==255&&middle.b==0,"half health is yellow for red/green endpoints");
 auto full=healthTint(Color{255,0,0},Color{0,255,0},1.f),empty=healthTint(Color{255,0,0},Color{0,255,0},0.f);
 check(full.g==255&&full.r==0&&empty.r==255&&empty.g==0,"health color preserves configured endpoints");
 auto muzzleAim=shotRotation({10,20,30},{10,120,30});
 check(muzzleAim&&std::abs(muzzleAim->yaw-16384)<2&&muzzleAim->pitch==0,"silent aim resolves from actual weapon muzzle origin");
 check(!shotRotation({1,2,3},{1,2,3})&&!shotRotation({NAN,0,0},{1,2,3}),"invalid and coincident shot positions leave native aim unchanged");
 auto rearAim=shotRotation({},{-100,0,0});
 check(rearAim&&angle(direction(*rearAim),{-1,0,0})<.1f,"silent aim supports targets behind camera at 360 degree FOV");
 // Captured live KF2 executor bytes; catches an omitted instruction prefix.
 unsigned char executor[0x36]{0x40,0x53,0x55,0x56,0x57,0x41,0x56,0x48,0x81,0xec,0x90,0,0,0};
 executor[0x32]=0x48;executor[0x33]=0x8b;executor[0x34]=0x52;executor[0x35]=0x14;
 check(scriptExecutorAbi(executor,sizeof(executor)),"accept verified KF2 executor including REX prefix");
 check(!scriptExecutorAbi(executor,sizeof(executor)-1),"reject truncated executor validation");
 executor[0x35]=0x18;check(!scriptExecutorAbi(executor,sizeof(executor)),"reject changed frame-node offset");
 auto intercept=interceptPoint({},{15000,0,0},{0,300,0},15000);
 check(intercept&&intercept->y>300&&intercept->y<301,"crossbow leads a laterally moving target by flight time");
 if(intercept){float t=length(*intercept)/15000;check(length(*intercept-(Vec{15000,0,0}+Vec{0,300,0}*t))<.01f,"projectile and moving target meet at the solved time");}
 auto still=interceptPoint({},{15000,0,0},{},15000);
 check(still&&length(*still-Vec{15000,0,0})<.01f,"stationary targets retain their aim point");
 auto away=interceptPoint({},{100,0,0},{50,0,0},100),toward=interceptPoint({},{100,0,0},{-50,0,0},100);
 check(away&&std::abs(away->x-200)<.01f&&toward&&std::abs(toward->x-66.6667f)<.01f,"approaching and receding targets use different flight times");
 check(!interceptPoint({},{100,0,0},{200,0,0},100)&&!interceptPoint({},{100,0,0},{},0),"unreachable targets and invalid speeds fall back to direct aim");
 auto linear=interceptPoint({},{100,0,0},{-100,0,0},100);
 check(linear&&std::abs(linear->x-50)<.01f,"equal-speed approach handles linear intercept equation");
 // Exercise real call-wrapper index handling against local mock metadata.
 std::vector<unsigned char> nativeMetadata(0x110);
 const Ptr nativeFn=reinterpret_cast<Ptr>(nativeMetadata.data());
 const unsigned nativeFlags=0x400;const unsigned short nativeIndex=299;
 std::memcpy(nativeMetadata.data()+0xd0,&nativeFlags,4);std::memcpy(nativeMetadata.data()+0xd4,&nativeIndex,2);
 static bool dispatched=false,zeroDuringDispatch=false;
 const auto savedEvent=processEvent;
 processEvent=[](Ptr,Ptr fn,void*,void*){dispatched=true;zeroDuringDispatch=read<unsigned short>(fn+0xd4)==0;};
 Call nativeCall(0,"");nativeCall.fn=nativeFn;nativeCall.bytes.resize(16);
 check(nativeCall.run(1)&&dispatched&&zeroDuringDispatch,"numbered native reaches dispatcher with index cleared");
 check(read<unsigned short>(nativeFn+0xd4)==nativeIndex,"native index restored after call");
 unsigned noFlags=0;std::memcpy(nativeMetadata.data()+0xd0,&noFlags,4);dispatched=false;
 check(!nativeCall.run(1)&&!dispatched&&read<unsigned short>(nativeFn+0xd4)==nativeIndex,"reject inconsistent native metadata without changing it");
 processEvent=savedEvent;
 check(length(noclipDirection(0,1,0,0)-Vec{1,0,0})<.001f,"noclip W follows horizontal heading");
 check(length(noclipDirection(16384,1,0,0)-Vec{0,1,0})<.001f,"noclip rotates horizontal controls with yaw");
 check(length(noclipDirection(1234,0,0,1)-Vec{0,0,1})<.001f&&length(noclipDirection(0,0,0,-1)-Vec{0,0,-1})<.001f,"noclip Space and Ctrl move vertically independent of heading");
 check(std::abs(length(noclipDirection(0,1,1,1))-1)<.001f,"noclip diagonal speed remains bounded");
 return failures?1:0;
}
