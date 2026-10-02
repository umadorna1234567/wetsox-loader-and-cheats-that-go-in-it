#pragma once
#include "targeting.hpp"
#include "runtime.hpp"
#include "kf2_settings.hpp"
#include <string>
namespace nexus {
constexpr unsigned sessionVersion=8;
struct Jc4Settings {
 bool god{},ammo{},boost{},rockets{},grappleRange{},vehicleBoost{};
 bool esp{},enemiesOnly{true},tracers{},aim{};
 float speed{1},wingsuitSpeed{1},hoverboardSpeed{1},grappleSpeed{1},range{300},fov{15},smooth{6};
 unsigned waypointKey{},objectiveKey{},waypointModifiers{},objectiveModifiers{};
 unsigned actionSequence{},actionCode{};
};
struct Session {
    unsigned version{sessionVersion};
    fc5::Settings settings;
    Jc4Settings jc4;
    wetsox::kf2::Settings kf2;
    char detail[256]{};
    bool allHumans{true},menuActive{true};
    unsigned aimKey{VK_RBUTTON}, modifiers{};
    unsigned long long heartbeat{},frames{};
    fc5::runtime::Status status;
};
#ifdef WETSOX_FC4
inline constexpr bool defaultFc4=true;
#else
inline constexpr bool defaultFc4=false;
#endif
#ifdef WETSOX_JC4
inline constexpr bool defaultJc4=true;
#else
inline constexpr bool defaultJc4=false;
#endif
#ifdef WETSOX_KF2
inline constexpr bool defaultKf2=true;
#else
inline constexpr bool defaultKf2=false;
#endif
inline std::wstring sessionName(DWORD pid,bool fc4=defaultFc4,bool jc4=defaultJc4,bool kf2=defaultKf2) { return std::wstring(kf2?L"Local\\WetsoxKF2Session_v8_":jc4?L"Local\\WetsoxJC4Session_v8_":fc4?L"Local\\WetsoxFC4Session_v8_":L"Local\\NexusFC5Session_v8_")+std::to_wstring(pid); }
inline std::wstring mutexName(DWORD pid,bool fc4=defaultFc4,bool jc4=defaultJc4,bool kf2=defaultKf2) { return sessionName(pid,fc4,jc4,kf2)+L"_Lock"; }
}
