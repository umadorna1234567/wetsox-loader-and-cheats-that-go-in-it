#include "tuning.h"
#include "combat_math.h"
#include "vehicle_math.h"
#include <iostream>
#include <limits>
#include <stdexcept>
#define CHECK(value) do {if(!(value)) throw std::runtime_error(#value);} while(0)
using namespace mod;
int main(){try {
        CMatrix4f camera{};camera.m[0].x=1;camera.m[1].y=1;
        camera.m[2].z=1;camera.m[2].w=1;camera.m[3].w=0;
        CVector2f projected{};
        CHECK(Project(camera,{0,0,10},projected) && projected.x==.5f && projected.y==.5f);
        CHECK(Project(camera,{5,5,10},projected) && projected.x==.75f && projected.y==.25f);
        CHECK(!Project(camera,{0,0,-10},projected));
        CHECK(!Project(camera,{0,0,0},projected));
        CHECK(!Project(camera,{std::numeric_limits<float>::infinity(),0,10},projected));
        CHECK(std::abs(AimDistance({.6f,.5f},2)-AimDistance({.5f,.7f},2))<.00001f);
        float radius=AimRadius(camera,20);camera.m[1].y=2;
        CHECK(std::abs(AimRadius(camera,20)-2*radius)<.00001f);
        auto fast=AimStep(100,0,2,.016f),slow=AimStep(100,0,12,.016f);
        CHECK(fast.x>slow.x && slow.x>0 && fast.x<100);
        CHECK(AimStep(1,-1,1,.016f).x==0 && AimStep(100,100,1,0).x==0);
        auto capped=AimStep(100000,0,1,1);CHECK(capped.x<=60.001f);
        CHECK(AimStep(std::numeric_limits<float>::infinity(),0,1,.016f).x==0);
        std::cout<<"PASS ESP/Aim controls, hold binding, projection, behind-camera rejection, FOV aspect/zoom and smoothing limits\n";

        float charges[]={.25f,.5f,1.f};
        RefillIdleBoost(charges,3,true,true);
        CHECK(charges[0]==.25f && charges[1]==.5f);
        // Simulate the engine draining a running burst to completion. The old
        // refill kept resetting this value and made this sequence endless.
        int burstTicks=0;charges[0]=1;
        while(charges[0]>0 && burstTicks<20) {
            charges[0]=std::max(0.f,charges[0]-.125f);
            RefillIdleBoost(charges,3,true,true);++burstTicks;
        }
        CHECK(burstTicks==8 && charges[0]==0);
        RefillIdleBoost(charges,3,true,false);
        CHECK(charges[0]==1 && charges[1]==1 && charges[2]==1);
        charges[0]=.5f;RefillIdleBoost(charges,3,false,false);CHECK(charges[0]==.5f);
        RefillIdleBoost(charges,4,true,false);CHECK(charges[0]==.5f);
        charges[1]=std::numeric_limits<float>::quiet_NaN();charges[2]=-1;
        RefillIdleBoost(charges,3,true,false);
        CHECK(charges[0]==1 && std::isnan(charges[1]) && charges[2]==-1);
        std::cout<<"PASS boost burst completes, idle refill, disabled mode and invalid charges\n";

        CMatrix4f car,rider;
        car.m[0]={0,0,-1,0};car.m[2]={1,0,0,0};
        car.m[3]={100,20,300,1};rider=car;rider.m[3]={100.5f,21,299,1};
        CHECK(VehicleDestination(car,rider,{500,70,-200}));
        CHECK(car.m[3].x==500 && car.m[3].y==70 && car.m[3].z==-200);
        CHECK(rider.m[3].x==500.5f && rider.m[3].y==71 && rider.m[3].z==-201);
        CHECK(car.m[0].z==-1 && car.m[2].x==1 && rider.m[0].z==-1);
        CHECK(!VehicleDestination(car,rider,{std::numeric_limits<float>::quiet_NaN(),0,0}));
        CHECK(car.m[3].x==500 && rider.m[3].x==500.5f);
        std::cout<<"PASS vehicle boost control, teleport orientation, rider offset and invalid destination\n";

        float value=40;ScalarOverride tune;
        CHECK(tune.Apply(value,2) && value==80);
        for(int i=0;i<1000;i++) CHECK(tune.Apply(value,2) && value==80);
        CHECK(tune.Apply(value,3) && value==120);
        CHECK(tune.Apply(value,1) && value==40);
        CHECK(tune.Apply(value,100) && value==4000);
        for(int i=0;i<1000;i++) CHECK(tune.Apply(value,100) && value==4000);
        CHECK(tune.Apply(value,1) && value==40);
        tune.Apply(value,2);value=50; // Engine replaces the setting.
        CHECK(tune.Apply(value,2) && value==100);
        CHECK(tune.Apply(value,1) && value==50);
        tune.Apply(value,2);value=35;
        CHECK(tune.Apply(value,1) && value==35); // Do not undo an external update.
        CHECK(!tune.Apply(value,0) && value==35);
        CHECK(!tune.Apply(value,std::numeric_limits<float>::infinity()) && value==35);
        std::cout<<"PASS non-compounding multipliers, baseline restore, external updates, invalid factors\n";
        float distance=100;ValueOverride range;
        CHECK(range.Apply(distance,true,50000) && distance==50000);
        for(int i=0;i<1000;i++) CHECK(range.Apply(distance,true,50000) && distance==50000);
        CHECK(range.Apply(distance,false,50000) && distance==100);
        range.Apply(distance,true,50000);distance=200;
        CHECK(range.Apply(distance,true,50000) && distance==50000);
        CHECK(range.Apply(distance,false,50000) && distance==200);
        range.Apply(distance,true,50000);distance=150;
        CHECK(range.Apply(distance,false,50000) && distance==150);
        CHECK(!range.Apply(distance,true,std::numeric_limits<float>::infinity()) && distance==150);
        std::cout<<"PASS grapple toggle, range restore, repeated updates and external changes\n";
    } catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<"\n";return 1;}
}
