#include "input_timer.hpp"
#include <stdexcept>

namespace unirally {
namespace {
void validate(const RaceTimerDigits& state) {
    if (state.minutes > 9 || state.tens_seconds > 5 || state.seconds > 9 || state.tenths > 9
        || state.subframe > 4) {
        throw std::invalid_argument("timer digits are outside their supported domain");
    }
}
} // namespace

ControllerSample sample_controller(const ControllerButtons& buttons) {
    ControllerSample result;
    // $80:87E9–87F2 copies the register bytes. $82:AAA4–AB59 decodes them.
    result.low_image = static_cast<std::uint8_t>((buttons.a ? 0x80 : 0) | (buttons.x ? 0x40 : 0)
                                                 | (buttons.left_shoulder ? 0x20 : 0)
                                                 | (buttons.right_shoulder ? 0x10 : 0));
    result.high_image = static_cast<std::uint8_t>(
        (buttons.b ? 0x80 : 0) | (buttons.y ? 0x40 : 0) | (buttons.select ? 0x20 : 0)
        | (buttons.start ? 0x10 : 0) | (buttons.up ? 0x08 : 0) | (buttons.down ? 0x04 : 0)
        | (buttons.left ? 0x02 : 0) | (buttons.right ? 0x01 : 0));
    // The original branches establish precedence for contradictory directions.
    result.vertical = buttons.up ? 0 : (buttons.down ? 2 : 1);
    result.horizontal = buttons.left ? 0 : (buttons.right ? 2 : 1);
    return result;
}

ControllerButtons with_physical_dpad(ControllerButtons buttons) {
    if (buttons.up && buttons.down) buttons.up = buttons.down = false;
    if (buttons.left && buttons.right) buttons.left = buttons.right = false;
    return buttons;
}

bool advance_timer_digits(RaceTimerDigits& state, bool enabled) {
    validate(state);
    if (!enabled) return false;
    // $81:C6C5–C75B; constants are immediate operands in PAL ROM at
    // file offsets 0xC6E3 (5), 0xC703 (10), 0xC72D (6), 0xC73F (10).
    // Digit ranges make each addition fit before the original carry/reset.
    if (++state.subframe != 5) return false;
    state.subframe = 0;
    if (++state.tenths != 10) return false;
    state.tenths = 0;
    if (++state.seconds != 10) return false;
    state.seconds = 0;
    if (++state.tens_seconds != 6) return false;
    state.tens_seconds = 0;
    if (++state.minutes != 10) return false;
    state.minutes = 9;
    state.tens_seconds = 5;
    state.seconds = 9;
    state.tenths = 9;
    return true;
}

TimerBytes serialize_timer(const RaceTimerDigits& state) {
    validate(state);
    const std::array words{state.minutes, state.tens_seconds, state.seconds, state.tenths,
                           state.subframe};
    TimerBytes bytes{};
    for (std::size_t index = 0; index < words.size(); ++index) {
        bytes[index * 2] = static_cast<std::uint8_t>(words[index] & 0xFFU);
        bytes[index * 2 + 1] = static_cast<std::uint8_t>(words[index] >> 8U);
    }
    return bytes;
}
RaceTimerDigits deserialize_timer(std::span<const std::uint8_t> bytes) {
    if (bytes.size() != TimerBytes{}.size()) {
        throw std::invalid_argument("timer serialization must have ten bytes");
    }
    std::array<std::uint16_t, 5> words{};
    for (std::size_t index = 0; index < words.size(); ++index) {
        words[index] =
            static_cast<std::uint16_t>(static_cast<unsigned>(bytes[index * 2])
                                       | (static_cast<unsigned>(bytes[index * 2 + 1]) << 8U));
    }
    RaceTimerDigits state{words[0], words[1], words[2], words[3], words[4]};
    validate(state);
    return state;
}
} // namespace unirally
