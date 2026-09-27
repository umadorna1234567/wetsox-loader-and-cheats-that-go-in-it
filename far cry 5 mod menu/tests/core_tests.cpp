#include "fc5/targeting.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace fc5;
void check(bool ok,const char* name) { if(!ok) throw std::runtime_error(name); }
void collision(const Shot& shot,const Solution& solution) {
    double t=solution.flightSeconds;
    Vec3 bullet=shot.muzzle+(solution.direction*shot.muzzleSpeed+shot.inheritedVelocity)*t+shot.gravity*(.5*t*t);
    Vec3 target=shot.target+shot.targetVelocity*t;
    check(length(bullet-target)<1e-5,"ballistic collision residual");
}
int main() try {
    AimSettings aim;aim.enabled=true;
    Shot shot{{0,0,0},{1000,0,0},{},{},{},800,10};
    auto result=solve(shot,aim);
    check(result&&std::abs(result->flightSeconds-1.25)<1e-10,"stationary time");
    shot.targetVelocity={0,15,0}; shot.gravity={0,0,-9.81};
    result=solve(shot,aim);check(result.has_value(),"moving gravity solution");collision(shot,*result);
    check(result->direction.y>0&&result->direction.z>0,"lead and drop");
    shot.inheritedVelocity={20,5,0};
    result=solve(shot,aim);check(result.has_value(),"vehicle solution");collision(shot,*result);
    aim.prediction=false;
    result=solve(shot,aim);check(result.has_value(),"prediction disabled");
    Shot stationary=shot;stationary.targetVelocity={};collision(stationary,*result);
    aim.bulletDrop=false;
    result=solve(shot,aim);check(result&&std::abs(result->direction.z)<1e-10,"drop disabled");
    aim.travelTime=false;
    result=solve(shot,aim);check(result&&result->flightSeconds==0&&result->direction.x==1,"direct aim");
    aim=AimSettings{};aim.enabled=true;
    shot={{},{100,0,0},{1000,0,0},{},{},100,10};
    check(!solve(shot,aim),"unreachable receding target");
    shot.targetVelocity={};shot.muzzleSpeed=0;check(!solve(shot,aim),"invalid projectile speed");
    shot.muzzleSpeed=100;shot.target={};check(!solve(shot,aim),"coincident target");
    shot.target={10000,0,0};shot.gravity={0,0,-9.81};check(!solve(shot,aim),"beyond ballistic range");
    shot.target={10000/9.81,0,0};shot.maxFlightSeconds=20;
    result=solve(shot,aim);check(result.has_value(),"maximum-range tangent root");collision(shot,*result);
    shot={{},{100,0,0},{},{},{},100,10};
    shot.maxFlightSeconds=.5;check(!solve(shot,aim),"flight time limit");
    shot.maxFlightSeconds=10;shot.muzzleSpeed=std::numeric_limits<double>::quiet_NaN();
    check(!solve(shot,aim),"NaN rejected");shot.muzzleSpeed=100;
    Target enemy;enemy.id=1;enemy.hitLocations[0]=Vec3{100,0,0};
    Target animal=enemy;animal.id=2;animal.kind=Kind::Animal;animal.hitLocations[0]=Vec3{100,1,0};
    std::array targets{enemy,animal};
    auto selected=selectTarget(targets,{1,0,0},shot,aim,false);
    check(selected&&selected->id==1,"enemy selection");
    aim.targets.enemies=false;check(!selectTarget(targets,{1,0,0},shot,aim,false),"animal filter off");
    aim.targets.animals=true;selected=selectTarget(targets,{1,0,0},shot,aim,false);
    check(selected&&selected->id==2,"animal filter on");
    aim.hitLocation=HitLocation::Chest;check(!selectTarget(targets,{1,0,0},shot,aim,false),"missing bone no fallback");
    aim.hitLocation=HitLocation::Head;aim.vehicleWeapons=false;
    check(!selectTarget(targets,{1,0,0},shot,aim,true),"vehicle toggle");
    aim.fovDegrees=.1;check(!selectTarget(targets,{1,0,0},shot,aim,false),"FOV exclusion");
    aim.fovDegrees=10;targets[1].visible=false;
    check(!selectTarget(targets,{1,0,0},shot,aim,false),"visibility exclusion");
    aim.targets.enemies=true;aim.targets.animals=true;
    targets[0].visible=false;targets[1].visible=true;
    selected=selectTarget(targets,{1,0,0},shot,aim,false);
    check(selected&&selected->id==2,"blocked nearest target skipped for exposed target");
    targets[1].visible=false;
    check(!selectTarget(targets,{1,0,0},shot,aim,false),"target released after moving behind cover");
    targets[0].visible=true;targets[1].visible=true;
    selected=selectTarget(targets,{1,0,0},shot,aim,false,2);
    check(selected&&selected->id==2,"sticky target kept over closer candidate");
    targets[1].visible=false;
    selected=selectTarget(targets,{1,0,0},shot,aim,false,2);
    check(!selected,"blocked sticky target pauses aim instead of switching");
    targets[1].visible=true;
    selected=selectTarget(targets,{1,0,0},shot,aim,false,2);
    check(selected&&selected->id==2,"sticky target resumes when exposed again");
    targets[1].visible=true;aim.stickyAim=false;
    selected=selectTarget(targets,{1,0,0},shot,aim,false,2);
    check(selected&&selected->id==1,"sticky toggle off restores nearest selection");
    Vec3 once=smoothDirection({1,0,0},{0,1,0},.1,.2);
    Vec3 twice=smoothDirection(smoothDirection({1,0,0},{0,1,0},.05,.2),{0,1,0},.05,.2);
    check(length(once-twice)<1e-10,"frame-independent smoothing");
    check(length(smoothDirection({1,0,0},{-1,0,0},.1,.2))>.99999,"opposite direction smoothing");
    check(smoothDirection({1,0,0},{0,1,0},.1,0).y==1,"instant aiming");
    // Independent forward integration verifies the explicit-Euler inverse.
    aim=AimSettings{};aim.enabled=true;
    for(double h:{1./30,1./60,1./120})for(double distance:{20.,40.,100.,220.}) {
        Shot discrete{{},{distance,0,0},{0,4,0},{},{0,0,-20},280,2,40,h};
        auto intercept=solve(discrete,aim);check(intercept.has_value(),"discrete intercept exists");
        Vec3 position{},velocity=intercept->direction*discrete.muzzleSpeed;
        double elapsed=0,traveled=0;
        while(elapsed+h<intercept->flightSeconds-1e-10) {
            Vec3 step=velocity*h;
            if(traveled>=discrete.dropDistance-1e-8)velocity=velocity+discrete.gravity*h;
            position=position+step;traveled+=length(step);elapsed+=h;
        }
        position=position+velocity*(intercept->flightSeconds-elapsed);
        check(length(position-(discrete.target+discrete.targetVelocity*intercept->flightSeconds))<1e-5,"discrete trajectory residual");
    }
    Shot delayed{{},{100,0,0},{},{},{0,0,-20},280,2,40,1./60};
    delayed.inheritedVelocity={1,0,0};check(!solve(delayed,aim),"unsupported distance-dependent inheritance rejected");
    delayed.inheritedVelocity={};delayed.simulationStepSeconds=0;
    check(!solve(delayed,aim),"delayed drop needs verified step model");
    aim.bulletDrop=false;result=solve(delayed,aim);
    check(result&&result->direction.z==0,"drop toggle bypasses delayed gravity");
    std::cout<<"All targeting checks passed.\n";
    return 0;
} catch(const std::exception& e) { std::cerr<<e.what()<<'\n';return 1; }
