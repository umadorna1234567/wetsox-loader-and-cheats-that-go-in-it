#pragma once
#include <windows.h>
#include <cstdint>
struct OverlayStatus {
    std::uint32_t size{sizeof(OverlayStatus)};
    std::uint32_t running{}, initialized{}, visible{};
    std::uint64_t frames{}, resizes{};
};
// Exported functions use WINAPI and may be invoked only outside loader lock.
extern "C" {
#ifdef FC5_DLL_BUILD
__declspec(dllexport) DWORD WINAPI FC5OverlayStart(void*);
__declspec(dllexport) DWORD WINAPI FC5OverlayStop(void*);
__declspec(dllexport) DWORD WINAPI FC5OverlayStatus(OverlayStatus*);
#endif
}
