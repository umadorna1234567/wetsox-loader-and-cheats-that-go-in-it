#include "fc5/menu.hpp"
#include <atomic>

namespace {
HINSTANCE module{};
std::atomic_flag running=ATOMIC_FLAG_INIT;
}
// Flags: bit 0 runs the hidden creation/cleanup check. All other bits reserved.
// Caller owns the LoadLibrary reference and must retain it until this returns.
extern "C" __declspec(dllexport) DWORD WINAPI FC5MenuRun(DWORD flags) noexcept {
    if(flags&~1u) return ERROR_INVALID_PARAMETER;
    if(running.test_and_set()) return ERROR_BUSY;
    DWORD result;
    try { result=static_cast<DWORD>(runMenu(module,(flags&1)!=0,SW_SHOW)); }
    catch(...) { result=ERROR_UNHANDLED_EXCEPTION; }
    running.clear();
    return result;
}
BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID) {
    if(reason==DLL_PROCESS_ATTACH) module=instance;
    return TRUE;
}
