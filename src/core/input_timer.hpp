#pragma once
#include <array>
#include <cstdint>
#include <span>

namespace unirally {

// Controller buttons are sampled once per update. Keep SNES register layout
// at this boundary; motion code consumes named buttons and decoded directions.
struct ControllerButtons {
    bool a{}, b{}, x{}, y{}, left_shoulder{}, right_shoulder{};
    bool select{}, start{}, up{}, down{}, left{}, right{};
};
struct ControllerSample {
    std::uint8_t low_image{};   // $4218 -> $0311
    std::uint8_t high_image{};  // $4219 -> $0313
    std::uint8_t vertical{1};   // 0 up, 1 neutral, 2 down ($0315)
    std::uint8_t horizontal{1}; // 0 left, 1 neutral, 2 right ($0319)
};
ControllerSample sample_controller(const ControllerButtons& buttons);
// A SNES pad's rocker D-pad cannot report opposing directions, so the
// controller shift register never publishes Up with Down or Left with Right.
// Keyboards can hold both; this drops both, as the audited reference core's
// gamepad does (bsnes sfc/controller/gamepad, `up & !down` per direction).
ControllerButtons with_physical_dpad(ControllerButtons buttons);

// Five original 16-bit digit words, in display-to-subframe order. In PAL,
// five enabled updates make a tenth. This state excludes display/audio flags
// and the race-start gate: the caller supplies whether the timer ticks.
struct RaceTimerDigits {
    std::uint16_t minutes{}, tens_seconds{}, seconds{}, tenths{}, subframe{};
};
using TimerBytes = std::array<std::uint8_t, 10>;
TimerBytes serialize_timer(const RaceTimerDigits& state);
RaceTimerDigits deserialize_timer(std::span<const std::uint8_t> bytes);
// Returns true when the original 9:59.9 saturation branch executes. The
// caller owns its audio flags; this function changes only the five digits.
bool advance_timer_digits(RaceTimerDigits& state, bool enabled);

} // namespace unirally
