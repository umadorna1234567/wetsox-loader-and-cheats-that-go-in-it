#pragma once
#include <windows.h>
#include <array>
#include <cstdint>
#include <vector>
#include "fc5/targeting.hpp"
namespace fc5::runtime {
struct Pawn {std::uint64_t id;std::array<float,3> position;bool animal{};int faction{-1};std::array<std::array<float,3>,20> bones{};std::uint32_t boneMask{};std::uintptr_t object{};float health{};Vec3 velocity{};bool velocityValid{};
    // Optional native skeleton for wildlife whose joints do not follow human names.
    std::vector<std::array<float,3>> boundsPoints;
    std::vector<std::array<std::array<float,3>,2>> skeleton;
};
struct Status {
    std::uint32_t size{sizeof(Status)},supported{},pawnCount{},animalCount{};
    std::uint64_t snapshots{},projections{},mismatches{},lastProjectionMs{};
    std::array<float,3> localPosition{};
    std::uint64_t calls{},lastCaller{},lastCamera{};
    std::uint64_t magazineCalls{},preservedRounds{};
    std::int32_t magazine{-1};std::uint32_t magazineHook{};
    std::uint64_t unlimitedQueries{};std::uint32_t unlimitedHook{},boneEntities{};
    std::uint64_t aimWrites{},aimTarget{};std::uint32_t aimState{};float centerError{};
    std::uint64_t visibilityQueries{},visibilityClear{},visibilityBlocked{};
    std::uint64_t launchSamples{},launchWeapon{};
    std::uint32_t launchHook{},launchPhysics{};
    float projectileSpeed{},projectileGravity{},projectileDropDistance{};
    std::array<float,3> launchOrigin{},launchDirection{},launchInheritedVelocity{};
    std::uint64_t activeWeapon{};float activeSpeed{},activeGravity{},activeDrop{},simulationStep{},flightSeconds{};
    std::uint32_t vehicleFovState{},playerFovState{};float vehicleFovDegrees{},playerFovDegrees{};
    std::uint64_t blockedInputPolls{};std::uint32_t menuCapturing{};
    std::array<std::uint64_t,8> aimReasons{};
    float lastAlignmentFailure{};
    std::uint64_t coverObstructions{},coverMissingTarget{},lastBlocker{},lastBlockedTarget{};
};
// Requires MinHook to be initialized. Build-specific gameplay bindings; fingerprints are advisory.
bool start();
void stop();
void refresh();
Status status();
bool project(std::array<float,3> point,float width,float height,float& x,float& y);
std::vector<Pawn> pawns();
// Samples the current pose each call; does not reuse discovery-time bones.
std::vector<Pawn> visualPawns(const Filters& filters);
void setNoReload(bool enabled);
void setUnlimitedAmmo(bool enabled);
void aim(const AimSettings&,bool held,double dt);
void cameraFov(bool playerEnabled,double playerDegrees,bool vehicleEnabled,double vehicleDegrees);
bool aimFovRadii(double diameter,float width,float height,float& x,float& y);
}
