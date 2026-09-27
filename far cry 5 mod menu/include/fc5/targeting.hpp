#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace fc5 {
struct Vec3 {
    double x{}, y{}, z{};
    Vec3 operator+(Vec3 b) const { return {x+b.x,y+b.y,z+b.z}; }
    Vec3 operator-(Vec3 b) const { return {x-b.x,y-b.y,z-b.z}; }
    Vec3 operator*(double s) const { return {x*s,y*s,z*s}; }
};
double dot(Vec3 a, Vec3 b);
double length(Vec3 v);
bool finite(Vec3 v);
enum class Kind { Enemy, OtherHuman, Animal };
enum class HitLocation { Head, Chest, Abdomen, Pelvis };
struct Filters {
    bool enemies{true}, otherHumans{false}, animals{false};
    bool accepts(Kind kind) const;
};
struct AimSettings {
    bool enabled{false}, vehicleWeapons{true};
    bool travelTime{true}, prediction{true}, bulletDrop{true};
    Filters targets;
    HitLocation hitLocation{HitLocation::Head};
    double fovDegrees{10}; // Full cone diameter, independent of camera FOV.
    double smoothingSeconds{0}; // Exponential time constant; zero means instant.
    bool stickyAim{true};
    // 0: normal visibility, 1: ignore occupant's own vehicle only.
    int vehicleOccupantCover{1};
};
struct Color { std::uint8_t r{}, g{}, b{}; };
struct Settings {
    AimSettings aim;
    bool esp{false};
    Filters espTargets;
    std::array<Color,3> colors{{{255,80,80},{80,180,255},{255,190,60}}};
    bool unlimitedAmmo{false}, noReload{false};
    bool vehicleFovOverride{false};
    double vehicleFovDegrees{90};
    bool vehicleSpeedOverride{false};
    double vehicleSpeedMultiplier{1};
};
// All values must use one consistent world coordinate system and units.
// inheritedVelocity is the actual velocity added to the projectile at launch,
// not an assumption that every weapon inherits the shooter's full velocity.
struct Shot {
    Vec3 muzzle, target, targetVelocity, inheritedVelocity, gravity;
    double muzzleSpeed{};
    double maxFlightSeconds{10};
    // Optional explicit-Euler model: gravity begins on the first full step
    // after the projectile reaches dropDistance. Zero step uses continuous gravity.
    double dropDistance{};
    double simulationStepSeconds{};
};
struct Solution {
    Vec3 direction;
    Vec3 predictedTarget;
    double flightSeconds{};
};
// Constant target velocity, constant gravity, no drag. Earliest valid solution.
// Returns nullopt for invalid input, coincident points, or unreachable targets.
std::optional<Solution> solve(const Shot&, const AimSettings&);
struct Target {
    std::uint64_t id{};
    Kind kind{Kind::Enemy};
    bool alive{true}, visible{true};
    Vec3 velocity;
    std::array<std::optional<Vec3>,4> hitLocations;
    std::uint64_t vehicleId{};
};
struct Selection { std::uint64_t id; Solution shot; double angleDegrees; };
std::optional<Selection> selectTarget(std::span<const Target>, Vec3 viewDirection,
                                    const Shot& weapon, const AimSettings&,
                                    bool mountedWeapon,std::uint64_t preferredTarget=0);
// Pitch/yaw callers must supply their own verified game coordinate conversion.
Vec3 smoothDirection(Vec3 current, Vec3 desired, double dt, double timeConstant);
}
