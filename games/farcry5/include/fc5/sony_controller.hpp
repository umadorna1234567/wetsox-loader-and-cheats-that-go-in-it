#pragma once
#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0800
#endif
#include <dinput.h>
#include <array>
#include <cstdint>
#include <mutex>
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

namespace wetsox {
// Sony HID/DirectInput layout, normalized to the same bindings as XInput.
// Restrict this mapping to known devices; arbitrary joysticks use different axes.
inline bool sonyProduct(DWORD product) {
    if (LOWORD(product) != 0x054c) return false;
    const auto pid = HIWORD(product);
    return pid == 0x0ce6 || pid == 0x0df2 || pid == 0x05c4 || pid == 0x09cc;
}
inline std::uint32_t sonyButtons(const DIJOYSTATE2& p) {
    std::uint32_t bits = 0;
    constexpr unsigned buttonBits[]{14,12,13,15,8,9,16,17,5,4,6,7};
    for (unsigned i=0; i<std::size(buttonBits); ++i)
        if (p.rgbButtons[i] & 0x80) bits |= 1u << buttonBits[i];
    if (LOWORD(p.rgdwPOV[0]) != 0xffff && p.rgdwPOV[0] < 36000) {
        const auto direction = ((p.rgdwPOV[0] + 2250) / 4500) % 8;
        if (direction == 0 || direction == 1 || direction == 7) bits |= 1u << 0;
        if (direction >= 3 && direction <= 5) bits |= 1u << 1;
        if (direction >= 5 && direction <= 7) bits |= 1u << 2;
        if (direction >= 1 && direction <= 3) bits |= 1u << 3;
    }
    if (p.lY < 16384) bits |= 1u << 18;
    if (p.lY > 49151) bits |= 1u << 19;
    if (p.lX < 16384) bits |= 1u << 20;
    if (p.lX > 49151) bits |= 1u << 21;
    // Sony's right stick is Z / Rz. Rx / Ry are its analog triggers.
    if (p.lRz < 16384) bits |= 1u << 22;
    if (p.lRz > 49151) bits |= 1u << 23;
    if (p.lZ < 16384) bits |= 1u << 24;
    if (p.lZ > 49151) bits |= 1u << 25;
    if (p.lRx > 7710) bits |= 1u << 16;
    if (p.lRy > 7710) bits |= 1u << 17;
    return bits;
}
class SonyControllers {
    struct Device {
        IDirectInputDevice8W* input = nullptr;
        GUID id{};
        bool seen = false;
        std::uint32_t buttons = 0;
    };
    IDirectInput8W* m_input = nullptr;
    std::array<Device,4> m_devices{};
    std::mutex m_mutex;
    ULONGLONG m_scan = 0, m_poll = 0;
    static BOOL CALLBACK enumerate(const DIDEVICEINSTANCEW* info, void* context) {
        if (!sonyProduct(info->guidProduct.Data1)) return DIENUM_CONTINUE;
        auto& self = *static_cast<SonyControllers*>(context);
        for (auto& device : self.m_devices) {
            if (device.input && device.id == info->guidInstance) {
                device.seen = true;
                return DIENUM_CONTINUE;
            }
        }
        for (auto& device : self.m_devices) if (!device.input) {
            IDirectInputDevice8W* input = nullptr;
            if (FAILED(self.m_input->CreateDevice(info->guidInstance, &input, nullptr))) break;
            // Nonexclusive background access permits both the game and UI to read.
            if (FAILED(input->SetDataFormat(&c_dfDIJoystick2)) ||
                FAILED(input->SetCooperativeLevel(nullptr, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE))) {
                input->Release(); break;
            }
            DIPROPRANGE range{};
            range.diph.dwSize = sizeof(range);
            range.diph.dwHeaderSize = sizeof(range.diph);
            range.diph.dwHow = DIPH_DEVICE;
            range.lMin = 0; range.lMax = 65535;
            if (FAILED(input->SetProperty(DIPROP_RANGE, &range.diph))) {
                input->Release(); break;
            }
            input->Acquire();
            device = {input, info->guidInstance, true, 0};
            break;
        }
        return DIENUM_CONTINUE;
    }
public:
    SonyControllers() {
        // Load the Windows implementation, not a game's local proxy DLL.
        using Create = HRESULT (WINAPI*)(HINSTANCE,DWORD,REFIID,LPVOID*,LPUNKNOWN);
        static const auto module = LoadLibraryExW(L"dinput8.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
        const auto create = module ? reinterpret_cast<Create>(GetProcAddress(module,"DirectInput8Create")) : nullptr;
        if (create) create(GetModuleHandleW(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8W,
                           reinterpret_cast<void**>(&m_input), nullptr);
    }
    ~SonyControllers() {
        for (auto& device : m_devices) if (device.input) {
            device.input->Unacquire(); device.input->Release();
        }
        if (m_input) m_input->Release();
    }
    std::uint32_t read(unsigned index) {
        if (index >= m_devices.size() || !m_input) return 0;
        std::lock_guard lock(m_mutex);
        const auto now = GetTickCount64();
        if (!m_scan || now - m_scan >= 2000) {
            m_scan = now;
            for (auto& device : m_devices) device.seen = false;
            m_input->EnumDevices(DI8DEVCLASS_GAMECTRL, enumerate, this, DIEDFL_ATTACHEDONLY);
            for (auto& device : m_devices) if (device.input && !device.seen) {
                device.input->Unacquire(); device.input->Release(); device = {};
            }
        }
        if (!m_poll || now - m_poll >= 4) {
            m_poll = now;
            for (auto& device : m_devices) if (device.input) {
                DIJOYSTATE2 state{};
                auto result = device.input->Poll();
                if (FAILED(result)) { device.input->Acquire(); device.input->Poll(); }
                result = device.input->GetDeviceState(sizeof(state), &state);
                device.buttons = SUCCEEDED(result) ? sonyButtons(state) : 0;
            }
        }
        return m_devices[index].buttons;
    }
};
inline std::uint32_t readSonyPad(unsigned index) {
    static SonyControllers controllers;
    return controllers.read(index);
}
}
