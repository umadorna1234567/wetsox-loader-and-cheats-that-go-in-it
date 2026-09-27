#pragma once
#include <Windows.h>
#include <array>
#include <cstddef>

namespace fc4 {
// Matches the scoped native worker lifecycle at FC64+1e6ede0. Never borrow
// another thread's memory router or replace an existing partial context.
struct PhysicsThreadBindings {
    DWORD routerSlot{}, monitorSlot{};
    void* memorySystem{};
    void (*construct)(void*){};
    void (*destroy)(void*){};
    void (*memoryInit)(void*, void*, const char*, unsigned){};
    void (*memoryQuit)(void*, void*, unsigned){};
    void (*threadInit)(void*, void*){};
    void (*threadQuit)(void*){};
};
class PhysicsThreadScope {
    PhysicsThreadBindings bindings_;
    alignas(16) std::array<std::byte, 0x80> router_{};
    unsigned result_{};
    bool owned_{}, ready_{};
public:
    explicit PhysicsThreadScope(PhysicsThreadBindings bindings) : bindings_(bindings) {
        if (bindings.routerSlot == TLS_OUT_OF_INDEXES ||
            bindings.monitorSlot == TLS_OUT_OF_INDEXES ||
            bindings.routerSlot == bindings.monitorSlot) return;
        auto router = TlsGetValue(bindings.routerSlot);
        auto monitor = TlsGetValue(bindings.monitorSlot);
        if (router || monitor) { ready_ = router && monitor; return; }
        if (!bindings.memorySystem || !bindings.construct || !bindings.destroy ||
            !bindings.memoryInit || !bindings.memoryQuit ||
            !bindings.threadInit || !bindings.threadQuit) return;
        bindings.construct(router_.data());
        bindings.memoryInit(bindings.memorySystem, router_.data(), "Wetsox visibility", 3);
        bindings.threadInit(&result_, router_.data());
        owned_ = true;
        ready_ = TlsGetValue(bindings.routerSlot) == router_.data() &&
                 TlsGetValue(bindings.monitorSlot) != nullptr;
    }
    ~PhysicsThreadScope() {
        if (!owned_) return;
        bindings_.threadQuit(&result_);
        bindings_.memoryQuit(bindings_.memorySystem, router_.data(), 3);
        bindings_.destroy(router_.data());
    }
    PhysicsThreadScope(const PhysicsThreadScope&) = delete;
    PhysicsThreadScope& operator=(const PhysicsThreadScope&) = delete;
    explicit operator bool() const { return ready_; }
};
}
