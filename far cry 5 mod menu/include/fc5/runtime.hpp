#pragma once
#include <windows.h>
#include <array>
#include <cstdint>
#include <vector>
#include "fc5/targeting.hpp"
namespace fc5::runtime {
struct Pawn {std::uint64_t id;std::array<float,3> position;bool animal{};int faction{-1};std::array<std::array<float,3>,4> bones{};std::uint32_t boneMask{};std::uintptr_t object{};float health{};Vec3 velocity{};bool velocityValid{};};
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
    std::uint32_t vehicleFovState{};float vehicleFovDegrees{};
    std::uint64_t blockedInputPolls{};std::uint32_t menuCapturing{};
    std::array<std::uint64_t,8> aimReasons{};
    float lastAlignmentFailure{};
    std::uint64_t coverObstructions{},coverMissingTarget{},lastBlocker{},lastBlockedTarget{};
};
// Requires MinHook to be initialized. Fingerprint-gated gameplay bindings.
bool start();
void stop();
void refresh();
Status status();
bool project(std::array<float,3> point,float width,float height,float& x,float& y);
std::vector<Pawn> pawns();
void setNoReload(bool enabled);
void setUnlimitedAmmo(bool enabled);
void aim(const AimSettings&,bool held,double dt);
void vehicleCamera(bool enabled,double degrees);
}
