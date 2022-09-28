#include "filters.hpp"

#include <math.h>

#define SIGNUM(x) ((x > 0) - (x < 0))
#define RADIUS_SCALING_MULTIPLIER = 128.0 / (128 - deadzone)

uint8_t apply_deadzone(uint8_t value, uint8_t deadzone, bool scale) {
    int8_t value_signed = value - 128;
    // TODO: Dolphin actually uses a circular deadzone, not square, which makes sense, so it would
    // be better to do some more complicated calculations to check if the value is within a circle
    // of that radius. This would also require both x and y coordinates to be passed in, which
    // further complicates matters.
    if (abs(value_signed) > deadzone) {
        // If outside deadzone, must subtract deadzone from result so that axis values start from 1
        // instead of having lower values cut off.
        int8_t post_deadzone = value_signed - deadzone * SIGNUM(value_signed);
        // If a radius value is passed in, scale up the values linearly so that the same effective
        // value is given on the rim.
        if (scale) {
            int8_t post_scaling = post_deadzone * 128.0 / (128 - deadzone);
            return post_scaling + 128;
        }
        return post_deadzone + 128;
    }
    return 128;
}

uint8_t apply_radius(uint8_t value, uint8_t radius) {
    int8_t value_signed = value - 128;
    return value_signed * radius / 128.0 + 128;
}
