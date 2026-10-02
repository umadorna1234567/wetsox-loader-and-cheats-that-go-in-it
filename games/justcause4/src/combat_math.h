#pragma once
#include "vector.h"
#include <algorithm>
#include <cmath>

namespace mod {
// The HUD uses row vectors against the camera's +0x194 view-projection matrix.
inline bool Project(const CMatrix4f& matrix,const CVector3f& p,CVector2f& screen) {
    float x=p.x*matrix.m[0].x+p.y*matrix.m[1].x+p.z*matrix.m[2].x+matrix.m[3].x;
    float y=p.x*matrix.m[0].y+p.y*matrix.m[1].y+p.z*matrix.m[2].y+matrix.m[3].y;
    float z=p.x*matrix.m[0].z+p.y*matrix.m[1].z+p.z*matrix.m[2].z+matrix.m[3].z;
    float w=p.x*matrix.m[0].w+p.y*matrix.m[1].w+p.z*matrix.m[2].w+matrix.m[3].w;
    if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(z)||!std::isfinite(w)||w<=.01f||z<=0) return false;
    screen={.5f+x/(2*w),.5f-y/(2*w)};
    return std::isfinite(screen.x)&&std::isfinite(screen.y);
}
inline float AimDistance(const CVector2f& p,float aspect) {
    float x=(p.x-.5f)*aspect,y=p.y-.5f;
    return std::sqrt(x*x+y*y);
}
inline float AimRadius(const CMatrix4f& matrix,float degrees) {
    float scale=std::sqrt(matrix.m[0].y*matrix.m[0].y+matrix.m[1].y*matrix.m[1].y+matrix.m[2].y*matrix.m[2].y);
    if(!std::isfinite(scale) || scale<=0) return 0;
    return std::clamp(.5f*scale*std::tan(std::clamp(degrees,5.f,60.f)*.00872664626f),.01f,.48f);
}
inline CVector2f AimStep(float errorX,float errorY,float smoothing,float dt) {
    if(!std::isfinite(errorX)||!std::isfinite(errorY)||!std::isfinite(smoothing)||!std::isfinite(dt)) return {};
    float alpha=1-std::exp(-12.f*std::clamp(dt,0.f,.05f)/std::clamp(smoothing,1.f,20.f));
    float x=std::abs(errorX)<2?0:errorX*alpha;
    float y=std::abs(errorY)<2?0:errorY*alpha;
    float length=std::sqrt(x*x+y*y),limit=1200*std::clamp(dt,0.f,.05f);
    if(length>limit && length>0) {x*=limit/length;y*=limit/length;}
    return {x,y};
}
}
