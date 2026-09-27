#pragma once
#include "fc5/targeting.hpp"
#include <cmath>

namespace fc4 {
inline fc5::Vec3 cross(fc5::Vec3 a,fc5::Vec3 b) {
    return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};
}
struct CameraPose {fc5::Vec3 eye,forward;};
inline std::optional<CameraPose> cameraPose(const std::array<float,16>& matrix) {
    for(auto v:matrix)if(!std::isfinite(v))return {};
    const fc5::Vec3 x{matrix[0],matrix[4],matrix[8]},y{matrix[1],matrix[5],matrix[9]},w{matrix[3],matrix[7],matrix[11]};
    const auto determinant=fc5::dot(x,cross(y,w)),size=fc5::length(w);
    if(std::abs(determinant)<1e-8||size<1e-6)return {};
    // At the perspective camera origin, clip X, Y and W are all zero.
    const auto eye=(cross(y,w)*(-matrix[12])+cross(w,x)*(-matrix[13])+cross(x,y)*(-matrix[15]))*(1/determinant);
    if(!fc5::finite(eye))return {};
    return CameraPose{eye,w*(1/size)};
}
// Apply the rendered-view correction to the game's base look direction.
// Camera bob/recoil can make these directions differ even while standing still.
inline std::optional<fc5::Vec3> compensateCamera(fc5::Vec3 look,fc5::Vec3 rendered,fc5::Vec3 target) {
    if(!fc5::finite(look)||!fc5::finite(rendered)||!fc5::finite(target))return {};
    auto n=fc5::length(look),r=fc5::length(rendered),t=fc5::length(target);
    if(n<1e-6||r<1e-6||t<1e-6)return {};
    look=look*(1/n);rendered=rendered*(1/r);target=target*(1/t);
    const auto axis=cross(rendered,target);const auto cosine=fc5::dot(rendered,target);
    if(cosine<-.99)return {};
    auto corrected=look+cross(axis,look)+cross(axis,cross(axis,look))*(1/(1+cosine));
    return corrected*(1/fc5::length(corrected));
}
}
