#ifndef FILTERS_HPP_
#define FILTERS_HPP_

#include <pico/stdlib.h>

#define PERCENT_TO_STICK_VAL(percentage) (uint8_t)(percentage * 255 / 100 / 2)

/**
 * @brief Applies a deadzone to an analog stick value, with optional scaling afterwards.
 *
 * @param value The analog value to apply the deadzone to
 * @param deadzone The deadzone radius
 * @param scale Whether or not to apply scaling after subtracting the deadzone
 *
 * @return The resulting value after applying deadzone/scaling
 */
uint8_t apply_deadzone(uint8_t value, uint8_t deadzone, bool scale);

/**
 * @brief Linearly scale an analog stick value such that the physical maximum value that it can
 * output gets scaled down to the specified radius.
 *
 * @param value The analog value to scale
 * @param radius The radius to which the value should be scaled
 *
 * @return The resulting value after linear scaling
 */
uint8_t apply_radius(uint8_t value, uint8_t radius);

#endif
