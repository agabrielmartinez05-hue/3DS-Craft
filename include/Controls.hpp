#pragma once
#include <algorithm>
#include <cmath>

namespace voxel {
struct ControlSettings {
    float lookSpeed = 2.2f;
    int deadzone = 15;
    bool invertY{};
};
inline float calibratedAxis(int value, int deadzone) {
    deadzone = std::clamp(deadzone, 0, 64);
    if (std::abs(value) <= deadzone)
        return 0;
    return std::clamp(float(std::abs(value) - deadzone) / float(156 - deadzone), 0.0f, 1.0f) *
           (value < 0 ? -1 : 1);
}
struct AnalogInput {
    int cpadX{}, cpadY{}, cstickX{}, cstickY{};
};
struct CalibratedInput {
    float forward{}, strafe{}, lookX{}, lookY{};
};
inline CalibratedInput mapAnalog(AnalogInput input, const ControlSettings& settings) {
    return {calibratedAxis(input.cpadY, settings.deadzone),
            calibratedAxis(input.cpadX, settings.deadzone),
            calibratedAxis(input.cstickX, settings.deadzone),
            calibratedAxis(input.cstickY, settings.deadzone) * (settings.invertY ? -1 : 1)};
}
} // namespace voxel
