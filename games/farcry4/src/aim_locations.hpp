#pragma once
#include <array>
#include <cstdint>
#include <optional>

namespace fc4 {
using AimPoint = std::array<float, 3>;
inline std::array<std::optional<AimPoint>, 4> aimLocations(
    const std::array<AimPoint, 20>& bones, std::uint32_t validMask, bool animal) {
    std::array<std::optional<AimPoint>, 4> result{};
    for (unsigned i = 0; i < result.size(); ++i)
        if (validMask & (1u << i)) result[i] = bones[i];
    // Recorded FC4 bird rig: Head and Spine exist; Spine2 and Hips do not.
    // Keep real joints when present; use the validated body joint only for
    // missing animal chest/pelvis aim points. Never invent a root position.
    if (animal && result[2]) {
        if (!result[1]) result[1] = result[2];
        if (!result[3]) result[3] = result[2];
    }
    return result;
}
}
