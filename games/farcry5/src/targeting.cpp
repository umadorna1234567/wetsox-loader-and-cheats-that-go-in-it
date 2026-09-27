#include "fc5/targeting.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

namespace fc5 {
double dot(Vec3 a, Vec3 b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
double length(Vec3 v) { return std::hypot(v.x,v.y,v.z); }
bool finite(Vec3 v) { return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z); }
bool Filters::accepts(Kind k) const {
    switch(k) { case Kind::Enemy:return enemies; case Kind::OtherHuman:return otherHumans;
                case Kind::Animal:return animals; }
    return false;
}
namespace {
Vec3 normalized(Vec3 v) { auto n=length(v); return n>1e-12?v*(1/n):Vec3{}; }
using Polynomial=std::vector<double>; // ascending coefficient order
double evaluate(const Polynomial& p,double x) {
    double result=0;
    for(auto it=p.rbegin();it!=p.rend();++it) result=result*x+*it;
    return result;
}
// Derivative roots partition the polynomial into monotone intervals. This also
// catches tangent roots (maximum-range shots) that a sign-change grid misses.
std::vector<double> roots(Polynomial p,double lo,double hi) {
    while(p.size()>1 && p.back()==0) p.pop_back();
    if(p.size()==1) return {};
    if(p.size()==2) {
        double t=-p[0]/p[1];
        return t>=lo&&t<=hi?std::vector<double>{t}:std::vector<double>{};
    }
    Polynomial derivative;
    for(std::size_t i=1;i<p.size();++i) derivative.push_back(p[i]*static_cast<double>(i));
    auto cuts=roots(derivative,lo,hi);
    cuts.insert(cuts.begin(),lo); cuts.push_back(hi);
    std::vector<double> out;
    for(double t:cuts) {
        double scale=0,power=1;
        for(double c:p) { scale+=std::abs(c)*power; power*=std::abs(t); }
        if(std::abs(evaluate(p,t))<=1e-12*std::max(1.0,scale)) out.push_back(t);
    }
    for(std::size_t i=1;i<cuts.size();++i) {
        double a=cuts[i-1],b=cuts[i],fa=evaluate(p,a),fb=evaluate(p,b);
        if((fa<0)==(fb<0)||fa==0||fb==0) continue;
        for(int n=0;n<90;++n) {
            double m=(a+b)/2,fm=evaluate(p,m);
            if((fa<0)==(fm<0)) {a=m;fa=fm;} else b=m;
        }
        out.push_back((a+b)/2);
    }
    std::sort(out.begin(),out.end());
    return out;
}
}
std::optional<Solution> solve(const Shot& s,const AimSettings& a) {
    if(!finite(s.muzzle)||!finite(s.target)||!finite(s.targetVelocity)||
       !finite(s.inheritedVelocity)||!finite(s.gravity)) return {};
    Vec3 r=s.target-s.muzzle;
    if(length(r)<1e-8) return {};
    if(!a.travelTime) return Solution{normalized(r),s.target,0};
    if(!std::isfinite(s.muzzleSpeed)||s.muzzleSpeed<=0||
       !std::isfinite(s.maxFlightSeconds)||s.maxFlightSeconds<=0) return {};
    Vec3 targetVelocity=a.prediction?s.targetVelocity:Vec3{};
    Vec3 v=targetVelocity-s.inheritedVelocity;
    Vec3 g=a.bulletDrop?s.gravity:Vec3{};
    if(!std::isfinite(s.dropDistance)||s.dropDistance<0||
       !std::isfinite(s.simulationStepSeconds)||s.simulationStepSeconds<0)return {};
    if(s.simulationStepSeconds>0&&length(g)>0) {
        // The distance threshold is direction-independent only without
        // inherited launch velocity. Other cases need a different model.
        if(length(s.inheritedVelocity)>1e-8)return {};
        const double h=s.simulationStepSeconds;
        if(h<1e-4||h>1||s.maxFlightSeconds/h>4096)return {};
        const double straightSteps=std::ceil(s.dropDistance/(s.muzzleSpeed*h));
        // Between simulation boundaries position is linear, making each
        // interception interval a quadratic rather than an iterative aim guess.
        for(unsigned n=0;n*h<s.maxFlightSeconds;++n) {
            const double k=std::max(0.,double(n)-straightSteps);
            const double constant=-k*h*straightSteps*h-.5*k*(k+1)*h*h;
            const Vec3 rr=r-g*constant,vv=v-g*(k*h);
            Polynomial p{dot(rr,rr),2*dot(rr,vv),dot(vv,vv)-s.muzzleSpeed*s.muzzleSpeed};
            for(double c:p)if(!std::isfinite(c))return {};
            for(double t:roots(p,n*h,std::min((n+1)*h,s.maxFlightSeconds))) {
                if(t<=1e-8)continue;
                Vec3 offset=rr+vv*t;const double distance=length(offset),expected=s.muzzleSpeed*t;
                if(!std::isfinite(distance)||distance<1e-8||std::abs(distance-expected)>1e-6*std::max(1.,expected))continue;
                return Solution{offset*(1/distance),s.target+targetVelocity*t,t};
            }
        }
        return {};
    }
    // A distance-delayed trajectory needs the discrete model above; do not
    // silently apply immediate continuous gravity to a different model.
    if(s.dropDistance>0&&length(g)>0)return {};
    // |r + v*t - .5*g*t^2|^2 = (muzzleSpeed*t)^2
    Polynomial p{dot(r,r),2*dot(r,v),dot(v,v)-dot(r,g)-s.muzzleSpeed*s.muzzleSpeed,
                 -dot(v,g),.25*dot(g,g)};
    for(double c:p) if(!std::isfinite(c)) return {};
    for(double t:roots(p,0,s.maxFlightSeconds)) {
        if(t<=1e-8) continue;
        Vec3 offset=r+v*t-g*(.5*t*t);
        double distance=length(offset),expected=s.muzzleSpeed*t;
        if(!std::isfinite(distance)||distance<1e-8||
           std::abs(distance-expected)>1e-6*std::max(1.0,expected)) continue;
        return Solution{offset*(1/distance),s.target+targetVelocity*t,t};
    }
    return {};
}
std::optional<Selection> selectTarget(std::span<const Target> targets,Vec3 view,
                                    const Shot& weapon,const AimSettings& a,bool mounted,std::uint64_t preferredTarget) {
    if(!a.enabled||(mounted&&!a.vehicleWeapons)||!finite(view)||length(view)<1e-8||
       !std::isfinite(a.fovDegrees)||a.fovDegrees<=0||a.fovDegrees>180) return {};
    auto index=static_cast<std::size_t>(a.hitLocation);
    if(index>=4) return {};
    // A sticky lock must not jump to a different pawn on a transient cover
    // result. Keep the identity in the caller, but return no aim solution.
    if(a.stickyAim&&preferredTarget)for(const auto& target:targets)
        if(target.id==preferredTarget&&target.alive&&a.targets.accepts(target.kind)&&
           target.hitLocations[index]&&!target.visible)return {};
    view=normalized(view);
    std::optional<Selection> best;
    for(const auto& target:targets) {
        if(!target.alive||!target.visible||!a.targets.accepts(target.kind)||!target.hitLocations[index]) continue;
        Shot shot=weapon; shot.target=*target.hitLocations[index];shot.targetVelocity=target.velocity;
        auto solution=solve(shot,a);
        if(!solution) continue;
        double angle=std::acos(std::clamp(dot(view,solution->direction),-1.0,1.0))*180/std::numbers::pi;
        if(angle>a.fovDegrees*.5) continue;
        if(a.stickyAim&&preferredTarget&&target.id==preferredTarget)return Selection{target.id,*solution,angle};
        if(!best||angle<best->angleDegrees) best=Selection{target.id,*solution,angle};
    }
    return best;
}
Vec3 smoothDirection(Vec3 current,Vec3 desired,double dt,double tau) {
    if(!finite(current)||!finite(desired)||length(current)<1e-8||length(desired)<1e-8) return current;
    current=normalized(current);desired=normalized(desired);
    if(!std::isfinite(dt)||dt<=0||!std::isfinite(tau)||tau<0) return current;
    if(tau==0) return desired;
    double alpha=-std::expm1(-dt/tau);
    double cosine=std::clamp(dot(current,desired),-1.0,1.0);
    if(cosine>.999999) return normalized(current*(1-alpha)+desired*alpha);
    Vec3 tangent=desired-current*cosine;
    if(length(tangent)<1e-8) {
        Vec3 axis=std::abs(current.x)<.8?Vec3{1,0,0}:Vec3{0,1,0};
        tangent=axis-current*dot(axis,current);
    }
    tangent=normalized(tangent);
    double angle=std::acos(cosine)*alpha;
    return normalized(current*std::cos(angle)+tangent*std::sin(angle));
}
}

namespace fc5 {
std::optional<std::array<float,3>> lookAngles(Vec3 world,const std::array<float,16>& root,float roll) {
    if(!finite(world)||length(world)<1e-8||!std::isfinite(roll))return {};
    Vec3 axes[3]{{root[0],root[1],root[2]},{root[4],root[5],root[6]},{root[8],root[9],root[10]}};
    for(auto axis:axes)if(!finite(axis)||std::abs(length(axis)-1)>.01)return {};
    if(std::abs(dot(axes[0],axes[1]))>.01||std::abs(dot(axes[1],axes[2]))>.01||std::abs(dot(axes[0],axes[2]))>.01)return {};
    world=world*(1/length(world));Vec3 local{dot(world,axes[0]),dot(world,axes[1]),dot(world,axes[2])};
    double cr=std::cos(roll);if(std::abs(cr)<.1||std::abs(local.z/cr)>1)return {};
    double pitch=std::asin(std::clamp(local.z/cr,-1.,1.));
    double yaw=std::atan2(-local.x,local.y)+std::atan2(std::sin(roll)*std::sin(pitch),std::cos(pitch));
    // Look-aspect yaw is signed relative to the pawn, as recorded during
    // native walking updates. -epsilon must stay negative, not become 2*pi:
    // those rays are equivalent, but the locomotion state is not modulo-only.
    yaw=std::remainder(yaw,2*std::numbers::pi);
    return std::array<float,3>{float(pitch),roll,float(yaw)};
}
std::optional<std::array<float,2>> coneRadii(const std::array<float,16>& m,double diameter,float width,float height) {
    if(!std::isfinite(diameter)||diameter<=0||diameter>=180||width<=0||height<=0)return {};
    const double tangent=std::tan(diameter*std::numbers::pi/360);
    const double x=std::hypot(m[0],m[4],m[8])*tangent*width*.5;
    const double y=std::hypot(m[1],m[5],m[9])*tangent*height*.5;
    if(!std::isfinite(x)||!std::isfinite(y)||x<=0||y<=0)return {};
    return std::array<float,2>{float(x),float(y)};
}
std::optional<Vec3> motionVelocity(Vec3 previous,Vec3 position,double elapsed) {
    if(!finite(previous)||!finite(position)||!std::isfinite(elapsed)||elapsed<.02||elapsed>.35)return {};
    auto velocity=(position-previous)*(1/elapsed);
    // Reject discontinuities (streaming, teleports, inconsistent transforms).
    if(!finite(velocity)||length(velocity)>35)return {};
    return velocity;
}
}

namespace fc5 {
bool clearVehicleCover(std::span<const CoverHit> hits) {
    bool reachable=false;
    for(auto hit:hits) {
        switch(hit) {
        case CoverHit::Target: case CoverHit::Vehicle: reachable=true;break;
        case CoverHit::LocalPlayer: break;
        case CoverHit::Obstruction: case CoverHit::Unknown: return false;
        }
    }
    return reachable;
}
}

namespace fc5 {
AimSettings weaponAimSettings(AimSettings settings,bool ballisticDataAvailable) {
    if(!ballisticDataAvailable)settings.travelTime=false;
    return settings;
}
}
