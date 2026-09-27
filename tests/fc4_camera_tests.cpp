#include "camera.hpp"
#include <cstdio>
#include <limits>

int main() {
    bool ok=true;
    auto check=[&](bool result,const char* message){if(!result){std::fprintf(stderr,"%s\n",message);ok=false;}};
    // Camera at (10,20,30), looking +Y with +Z up. Damage/rope animation
    // moves the rendered eye independently of the pawn's base look pose.
    std::array<float,16> matrix{2,0,0,0, 0,0,1,1, 0,3,0,0, -20,-90,-20.1f,-20};
    auto pose=fc4::cameraPose(matrix);
    check(pose&&fc5::length(pose->eye-fc5::Vec3{10,20,30})<1e-6,"Extract the rendered camera origin");
    check(pose&&fc5::length(pose->forward-fc5::Vec3{0,1,0})<1e-6,"Extract rendered forward");
    matrix[12]=-24;matrix[13]=-96;matrix[15]=-21;
    pose=fc4::cameraPose(matrix);
    check(pose&&fc5::length(pose->eye-fc5::Vec3{12,21,32})<1e-6,"Animated camera motion must not invalidate perspective");
    matrix[3]=matrix[7]=matrix[11]=0;
    check(!fc4::cameraPose(matrix),"Reject orthographic/degenerate map projection");
    matrix[0]=std::numeric_limits<float>::quiet_NaN();
    check(!fc4::cameraPose(matrix),"Reject invalid projection data");
    const fc5::Vec3 look{0,1,0},rendered{.1,std::sqrt(.99),0};
    auto correction=fc4::compensateCamera(look,rendered,look);
    check(correction&&correction->x<-.099&&std::abs(correction->y-rendered.y)<1e-6,"Correct camera offset instead of reproducing it");
    correction=fc4::compensateCamera(look,rendered,rendered);
    check(correction&&fc5::length(*correction-look)<1e-6,"Already aligned rendered view needs no look correction");
    check(!fc4::compensateCamera(look,{},look),"Reject invalid camera direction");
    return ok?0:1;
}
