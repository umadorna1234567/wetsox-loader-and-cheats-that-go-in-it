#pragma once
#include "targeting.hpp"
#include "runtime.hpp"
#include <string>
namespace nexus {
constexpr unsigned sessionVersion=4;
struct Session {
    unsigned version{sessionVersion};
    fc5::Settings settings;
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
inline std::wstring sessionName(DWORD pid,bool fc4=defaultFc4) { return std::wstring(fc4?L"Local\\WetsoxFC4Session_v4_":L"Local\\NexusFC5Session_v4_")+std::to_wstring(pid); }
inline std::wstring mutexName(DWORD pid,bool fc4=defaultFc4) { return sessionName(pid,fc4)+L"_Lock"; }
}
