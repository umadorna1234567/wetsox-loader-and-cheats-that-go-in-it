#include "fc5/camera.hpp"
#include "fc5/targeting.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace fc5;
#include "walking_samples.hpp"
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
    // These checks deliberately compare stored angles, not just reconstructed
    // rays: wrapping a negative look yaw to 2*pi passes ray-only assertions.
    for(const auto& sample:walkingSamples) {
        auto restored=lookAngles(sample.view,sample.root,sample.angles[1]);
        check(restored.has_value(),"recorded walking look converts");
        check(std::abs((*restored)[0]-sample.angles[0])<1e-5,"recorded walking pitch preserved");
        check(std::abs((*restored)[2]-sample.angles[2])<1e-5,"recorded signed walking yaw preserved");
    }
    std::array<float,16> identity{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    for(double yaw:{-.2,-.001,0.,.001,.2}) {
        auto look=lookAngles({-std::sin(yaw),std::cos(yaw),0},identity,0);
        check(look&&std::abs((*look)[2]-yaw)<1e-6,"small yaw across zero must not become a full turn");
    }
    // Reconstruct a desired ray through independently composed body yaw and
    // look pitch/roll/yaw, including strafing lean and wraparound.
    for(double body:{-3.,-1.,0.,1.,3.})for(float roll:{-.35f,0.f,.35f})for(double pitch:{-.6,0.,.6})for(double yaw:{.001,1.5,3.14,6.282}) {
        std::array<float,16> root{float(std::cos(body)),float(std::sin(body)),0,0,float(-std::sin(body)),float(std::cos(body)),0,0,0,0,1,0,0,0,0,1};
        Vec3 local{-std::sin(yaw)*std::cos(pitch)+std::cos(yaw)*std::sin(roll)*std::sin(pitch),std::cos(yaw)*std::cos(pitch)+std::sin(yaw)*std::sin(roll)*std::sin(pitch),std::cos(roll)*std::sin(pitch)};
        Vec3 desired{local.x*root[0]+local.y*root[4],local.x*root[1]+local.y*root[5],local.z};
        auto out=lookAngles(desired,root,roll);check(out.has_value(),"movement look conversion exists");
        const double p=(*out)[0],r=(*out)[1],y=(*out)[2];
        Vec3 actual{-std::sin(y)*std::cos(p)+std::cos(y)*std::sin(r)*std::sin(p),std::cos(y)*std::cos(p)+std::sin(y)*std::sin(r)*std::sin(p),std::cos(r)*std::sin(p)};
        check(length(actual-local)<1e-6,"strafing roll and body yaw preserve target ray");
    }
    check(!motionVelocity({0,0,0},{100,0,0},.05),"teleport excluded from prediction");
    check(!motionVelocity({0,0,0},{1,0,0},1),"stale motion sample rejected");
    auto velocity=motionVelocity({0,0,0},{.25,0,0},.05);check(velocity&&std::abs(velocity->x-5)<1e-8,"valid motion preserved");
    std::array<float,16> projection{};projection[0]=9.f/16;projection[5]=1;
    auto circle=coneRadii(projection,90,1920,1080);check(circle&&std::abs((*circle)[0]-540)<.001&&std::abs((*circle)[1]-540)<.001,"FOV matches perspective projection and aspect");
    auto smallCircle=coneRadii(projection,10,1920,1080);check(smallCircle&&(*smallCircle)[0]<(*circle)[0],"FOV ring scales with aim cone");
    check(!coneRadii(projection,180,1920,1080),"no infinite radius at 180 degrees");
    const std::array vehicleOnly{CoverHit::Vehicle};
    const std::array vehicleOccupant{CoverHit::Vehicle,CoverHit::Target};
    const std::array wallBehindVehicle{CoverHit::Vehicle,CoverHit::Obstruction,CoverHit::Target};
    const std::array wallBeforeVehicle{CoverHit::Obstruction,CoverHit::Vehicle};
    const std::array unresolvedPart{CoverHit::Vehicle,CoverHit::Unknown};
    const std::array severalVehicles{CoverHit::LocalPlayer,CoverHit::Vehicle,CoverHit::Vehicle};
    const std::array playerOnly{CoverHit::LocalPlayer};
    check(clearVehicleCover(vehicleOnly),"vehicle-only query permits seated occupant with no collision hit");
    check(clearVehicleCover(vehicleOccupant),"vehicle does not block directly hit occupant");
    check(clearVehicleCover(severalVehicles),"identified vehicles always pass cover checks");
    check(!clearVehicleCover(wallBehindVehicle)&&!clearVehicleCover(wallBeforeVehicle),"walls block regardless of hit order");
    check(!clearVehicleCover(unresolvedPart),"unidentified collision is not assumed to be a vehicle");
    check(!clearVehicleCover(playerOnly)&&!clearVehicleCover({}),"missing target and vehicle remain unconfirmed");
    AimSettings vectorAim;vectorAim.enabled=true;vectorAim.fovDegrees=60;
    const auto fallback=weaponAimSettings(vectorAim,false);
    Shot unrecognized;unrecognized.muzzle={0,0,0};unrecognized.target={1,20,0};
    auto direct=solve(unrecognized,fallback);
    check(direct&&direct->flightSeconds==0,"unrecognized weapon uses direct aim instead of stopping");
    check(!fallback.travelTime&&vectorAim.travelTime,"fallback does not erase requested ballistic settings");
    check(weaponAimSettings(vectorAim,true).travelTime,"recognized weapon retains projectile compensation");
    for(float aspect:{4.f/3,16.f/9,21.f/9})for(float zoom:{1.f,2.f,4.f,8.f}) {
        std::array<float,16> projection{zoom/aspect,0,0,0, 0,0,1,1, 0,zoom,0,0, 0,0,-.1f,0};
        const auto pose=camera::cameraPose(projection);
        const double tilt=2*std::numbers::pi/180;
        check(pose.has_value(),"FC5 perspective camera remains valid across scope and aspect changes");
        const auto angle=camera::alignmentDegrees({std::sin(tilt),std::cos(tilt),0},pose->forward);
        check(angle&&std::abs(*angle-2)<1e-5,"FC5 aim alignment is two degrees at every zoom/aspect");
        check(camera::alignmentDegrees({1,0,0},pose->forward).value_or(0)>3,"FC5 rejects unrelated camera orientation");
    }
    std::cout<<"All targeting checks passed.\n";
    return 0;
} catch(const std::exception& e) { std::cerr<<e.what()<<'\n';return 1; }
