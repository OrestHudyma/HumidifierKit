#pragma once
#include <stdint.h>

// GPIO numbers, not physical header positions. Prototype wiring, 3.3 V logic.
constexpr uint8_t POWER_PIN = 10;
constexpr uint8_t BACKLIGHT_PIN = 11;
constexpr uint8_t MAX_POWER_PIN = 12;
constexpr uint8_t WATER_EMPTY_PIN = 4;
constexpr uint8_t PAIR_BUTTON_PIN = 9;  // BOOT button, active low
constexpr bool RELAY_ACTIVE_HIGH = true;
constexpr bool WATER_EMPTY_ACTIVE_HIGH = false;  // close to GND = empty
constexpr uint32_t DEBOUNCE_MS = 50;
