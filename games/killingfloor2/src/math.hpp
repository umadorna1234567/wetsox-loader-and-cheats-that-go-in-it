#pragma once
#include "reflection.hpp"
#include <cmath>
#include <numbers>
#include <optional>
namespace kf2 {
inline Vec operator+(Vec a,Vec b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline Vec operator-(Vec a,Vec b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline Vec operator*(Vec a,float b){return {a.x*b,a.y*b,a.z*b};}
inline float dot(Vec a,Vec b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline float length(Vec a){return std::sqrt(dot(a,a));}
inline Vec cross(Vec a,Vec b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
// Bone axes may face either way; capture NDC +X must move toward camera right.
inline Vec lensRightAxis(Vec boneRight,Vec captureRight){return dot(boneRight,captureRight)<0?boneRight*-1.f:boneRight;}
// Crossbow rear glass measured relative to RW_Scope in the shipped mesh.
// The previous mount-plane radius was calibrated at 13.18 units of eye depth.
inline constexpr float crossbowLensSetback=5.80976f;
inline constexpr float crossbowLensHeight=3.f;
inline constexpr float crossbowLensRadius=3.214f*(13.18f-crossbowLensSetback)/13.18f;
inline Vec crossbowLensCenter(Vec pivot,Vec forward,Vec up){return pivot-forward*crossbowLensSetback+up*crossbowLensHeight;}

inline bool validLensBasis(Vec right,Vec up,Vec forward){
 const float r=length(right),u=length(up),f=length(forward);
 return std::isfinite(r+u+f)&&r>.9f&&r<1.1f&&u>.9f&&u<1.1f&&f>.9f&&f<1.1f&&std::abs(dot(right,up))<.05f&&std::abs(dot(cross(right,up),forward))>.8f;
}
inline bool finite(Vec a){return std::isfinite(a.x)&&std::isfinite(a.y)&&std::isfinite(a.z);}
inline Vec direction(Rot r){const float p=r.pitch*std::numbers::pi_v<float>/32768,y=r.yaw*std::numbers::pi_v<float>/32768;return {std::cos(p)*std::cos(y),std::cos(p)*std::sin(y),std::sin(p)};}
inline Rot rotation(Vec d){return {int(std::atan2(d.z,std::hypot(d.x,d.y))*32768/std::numbers::pi_v<float>),int(std::atan2(d.y,d.x)*32768/std::numbers::pi_v<float>),0};}
// Resolve from the weapon's actual firing origin, not the previous camera position.
inline std::optional<Rot> shotRotation(Vec origin,Vec target){
 if(!finite(origin)||!finite(target)||length(target-origin)<.01f)return {};
 return rotation(target-origin);
}
// Constant-velocity interception, using game-world velocity units and seconds.
inline std::optional<Vec> interceptPoint(Vec origin,Vec target,Vec velocity,float speed){
 if(!finite(origin)||!finite(target)||!finite(velocity)||!std::isfinite(speed)||speed<=0)return {};
 const auto r=target-origin;const double a=double(dot(velocity,velocity))-double(speed)*speed,b=2.*dot(r,velocity),c=dot(r,r);
 if(c<.0001)return {};
 double t=-1;
 if(std::abs(a)<1e-8*double(speed)*speed){if(std::abs(b)>1e-8)t=-c/b;}
 else {const double discriminant=b*b-4*a*c;if(discriminant<0)return {};
  const double q=-.5*(b+std::copysign(std::sqrt(discriminant),b));
  const double t1=q/a,t2=std::abs(q)>1e-12?c/q:-1;
  if(t1>0)t=t1;if(t2>0&&(t<0||t2<t))t=t2;
 }
 if(!std::isfinite(t)||t<=0||t>5)return {};
 const auto point=target+velocity*float(t);return finite(point)?std::optional<Vec>(point):std::nullopt;
}
inline Vec noclipDirection(int yaw,float forward,float right,float up){
 const float angle=yaw*std::numbers::pi_v<float>/32768;
 Vec motion{std::cos(angle)*forward-std::sin(angle)*right,std::sin(angle)*forward+std::cos(angle)*right,up};
 const float magnitude=length(motion);return magnitude>1?motion*(1/magnitude):motion;
}
inline int turn(int delta){return int((static_cast<unsigned>(delta)+32768u)&65535u)-32768;}
inline float angle(Vec forward,Vec target){const float d=length(target)*length(forward);return d>.001f?std::acos(std::clamp(dot(forward,target)/d,-1.f,1.f))*180/std::numbers::pi_v<float>:180;}
inline std::optional<Vec> projectWorld(Vec point,Vec eye,Rot view,float horizontalFov,float width,float height){
 if(!finite(point)||!finite(eye)||!std::isfinite(horizontalFov)||horizontalFov<=.1f||horizontalFov>=179.f||width<=0||height<=0)return {};
 const auto forward=direction(view);const float yaw=view.yaw*std::numbers::pi_v<float>/32768,pitch=view.pitch*std::numbers::pi_v<float>/32768,roll=view.roll*std::numbers::pi_v<float>/32768;
 const Vec baseRight{-std::sin(yaw),std::cos(yaw),0},baseUp{-std::sin(pitch)*std::cos(yaw),-std::sin(pitch)*std::sin(yaw),std::cos(pitch)};
 const auto right=baseRight*std::cos(roll)-baseUp*std::sin(roll),up=baseRight*std::sin(roll)+baseUp*std::cos(roll),delta=point-eye;
 const float z=dot(delta,forward);if(z<=.01f)return {};
 const float scale=width*.5f/std::tan(horizontalFov*.5f*std::numbers::pi_v<float>/180.f);
 Vec result{width*.5f+dot(delta,right)*scale/z,height*.5f-dot(delta,up)*scale/z,z};return finite(result)?std::optional<Vec>(result):std::nullopt;
}
struct Matrix {float m[16]{};};
inline std::optional<Vec> scopeCoordinates(Vec point,const Matrix& view,const Matrix& projection){
 float input[4]{point.x,point.y,point.z,1},camera[4]{},clip[4]{};
 for(int c=0;c<4;++c)for(int r=0;r<4;++r)camera[c]+=input[r]*view.m[r*4+c];
 for(int c=0;c<4;++c)for(int r=0;r<4;++r)clip[c]+=camera[r]*projection.m[r*4+c];
 if(!std::isfinite(clip[3])||clip[3]<=.01f)return {};
 const float x=clip[0]/clip[3],y=clip[1]/clip[3];
 if(!std::isfinite(x+y)||x*x+y*y>1.f)return {};
 return Vec{x,y,clip[3]};
}
inline std::optional<Vec> projectScope(Vec point,const Matrix& view,const Matrix& projection,float diameter,float width,float height){
 auto uv=scopeCoordinates(point,view,projection);if(!uv)return {};
 return Vec{width*.5f+uv->x*diameter*.5f,height*.5f-uv->y*diameter*.5f,-uv->z};
}
inline bool insideLens(Vec point,Vec center,Vec rightEdge,Vec topEdge){
 auto a=rightEdge-center,b=topEdge-center,d=point-center;float determinant=a.x*b.y-a.y*b.x;
 if(!std::isfinite(determinant)||std::abs(determinant)<.001f)return false;
 const float u=(d.x*b.y-d.y*b.x)/determinant,v=(a.x*d.y-a.y*d.x)/determinant;
 return u*u+v*v<=1;
}
template<class Color>Color healthTint(Color low,Color high,float ratio){
 auto hsv=[](Color c){
  const float r=c.r/255.f,g=c.g/255.f,b=c.b/255.f,maximum=std::max({r,g,b}),minimum=std::min({r,g,b}),delta=maximum-minimum;
  float hue=0;if(delta>0){if(maximum==r)hue=std::fmod((g-b)/delta,6.f);else if(maximum==g)hue=(b-r)/delta+2;else hue=(r-g)/delta+4;hue/=6;if(hue<0)hue+=1;}
  return Vec{hue,maximum>0?delta/maximum:0,maximum};
 };
 ratio=std::clamp(ratio,0.f,1.f);if(ratio<=0)return low;if(ratio>=1)return high;
 auto a=hsv(low),b=hsv(high);if(a.y<.0001f)a.x=b.x;if(b.y<.0001f)b.x=a.x;
 float delta=b.x-a.x;if(delta>.5f)delta-=1;if(delta<-.5f)delta+=1;
 float h=std::fmod(a.x+delta*ratio+1,1.f)*6,s=a.y+(b.y-a.y)*ratio,v=a.z+(b.z-a.z)*ratio;
 float chroma=v*s,x=chroma*(1-std::abs(std::fmod(h,2.f)-1)),m=v-chroma;Vec rgb{};
 if(h<1)rgb={chroma,x,0};else if(h<2)rgb={x,chroma,0};else if(h<3)rgb={0,chroma,x};else if(h<4)rgb={0,x,chroma};else if(h<5)rgb={x,0,chroma};else rgb={chroma,0,x};
 return Color{static_cast<unsigned char>(std::lround((rgb.x+m)*255)),static_cast<unsigned char>(std::lround((rgb.y+m)*255)),static_cast<unsigned char>(std::lround((rgb.z+m)*255)),255};
}
inline float targetScore(float degrees,float distance,float fov,float maxDistance,int priority){if(priority==1)return distance;if(priority==2)return degrees/std::max(1.f,fov*.5f)+distance/std::max(1.f,maxDistance);return degrees;}
inline bool inCone(float angle,float diameter){return std::isfinite(angle)&&std::isfinite(diameter)&&diameter>0&&diameter<=360&&angle<=diameter*.5f;}
}
