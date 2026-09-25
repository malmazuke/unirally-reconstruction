#include "speed_limits.hpp"
#include <algorithm>
#include <stdexcept>

namespace unirally {
namespace {
std::uint16_t add(std::uint16_t left, std::uint16_t right) {
    return static_cast<std::uint16_t>(static_cast<std::uint32_t>(left) + right);
}
std::uint16_t subtract(std::uint16_t left, std::uint16_t right) {
    return static_cast<std::uint16_t>(static_cast<std::uint32_t>(left) - right);
}
bool negative(std::uint16_t value) {
    return (value & 0x8000U) != 0;
}
void subtract_if_nonnegative(std::uint16_t& value, std::uint16_t amount) {
    const auto candidate = subtract(value, amount);
    if (!negative(candidate)) value = candidate;
}
std::uint16_t cap_velocity(std::uint16_t velocity, std::uint16_t cap) {
    // Original CMP followed by BMI/BPL tests wrapped subtraction, not C++'s
    // unbounded ordering. Keep that distinction even outside ordinary speeds.
    if (negative(velocity)) {
        const auto negative_cap = subtract(0, cap);
        return !negative(subtract(negative_cap, velocity)) ? negative_cap : velocity;
    }
    return negative(subtract(cap, velocity)) ? cap : velocity;
}
std::uint16_t progress_contribution(SpeedModifiers& state, const SpeedLimitContext& context) {
    if (context.ai_enabled && context.opponent) {
        const auto difference = subtract(context.player_progress, context.opponent_progress);
        return difference != 0 && !negative(difference)
                 ? add(context.ai_adjustment, context.ai_adjustment)
                 : 0;
    }
    const auto own = context.opponent ? context.opponent_progress : context.player_progress;
    const auto other = context.opponent ? context.player_progress : context.opponent_progress;
    const auto difference = subtract(other, own);
    if (negative(difference)) {
        const auto candidate = subtract(state.progress_adjustment, 1);
        if (negative(candidate)) return 0;
        state.progress_adjustment = candidate;
    } else if (difference != 0) {
        const auto candidate = add(state.progress_adjustment, 1);
        if (negative(subtract(candidate, context.adjustment_limit)))
            state.progress_adjustment = candidate;
    }
    return state.progress_adjustment;
}
} // namespace

void limit_rider_speed(std::uint16_t& velocity_x, std::uint16_t& velocity_y,
                       SpeedModifiers& modifiers, const SpeedLimitContext& context,
                       const SpeedDecayContent& content) {
    if (context.skip) return;
    if (context.cartridge_mode == 2 || context.cartridge_mode == 42) {
        throw std::invalid_argument("alternate cartridge vertical cap is not recovered");
    }
    if (content.masks.size() != 9 || content.decrements.size() != 18) {
        throw std::invalid_argument("speed decay content must contain nine masks and nine words");
    }
    auto state = modifiers;
    auto horizontal = velocity_x;
    auto vertical = velocity_y;
    bool fast_decay{};
    // $82:A705–A76F. The pose branch only selects boost decay; its DEC/INC
    // accumulator result is discarded before any velocity store.
    if (context.drag) {
        horizontal = negative(horizontal) ? add(horizontal, 3) : subtract(horizontal, 3);
        fast_decay = true;
    } else if (!negative(horizontal)) {
        fast_decay = context.pose_byte >= 0xB0;
    } else {
        fast_decay = context.pose_byte < (context.opponent ? 0x40 : 0x10);
    }
    if (fast_decay) subtract_if_nonnegative(state.boost, 16);

    // $82:A774–A7F8. PAL immediate operands: file 0x1277A (80), 0x127F1 (384).
    std::uint16_t extra = 80;
    if (!context.start_override) {
        extra = add(state.boost, progress_contribution(state, context));
        extra = negative(extra) ? 0 : std::min<std::uint16_t>(extra, 384);
    }
    horizontal = cap_velocity(
        horizontal, add(static_cast<std::uint16_t>(extra >> 1U), context.player_base_cap));
    // Ordinary cartridge mode; 768 is the immediate at PAL file 0x12830.
    vertical = cap_velocity(vertical, add(static_cast<std::uint16_t>(extra >> 1U), 768));
    subtract_if_nonnegative(state.vertical_boost, 1);

    // $82:A875–A8A4. Counter-controlled decay follows this frame's cap.
    const auto bucket = std::min<unsigned>(static_cast<unsigned>(extra >> 4U), 8U);
    const auto mask = content.masks[bucket];
    if ((context.update_counter & mask) == mask) {
        const auto decrement = static_cast<std::uint16_t>(
            static_cast<unsigned>(content.decrements[bucket * 2])
            | (static_cast<unsigned>(content.decrements[bucket * 2 + 1]) << 8U));
        subtract_if_nonnegative(state.boost, decrement);
    }
    if (context.friction_mode == 1) {
        if (negative(horizontal)) {
            const auto candidate = add(horizontal, 1);
            if (negative(candidate)) horizontal = candidate; // -1 remains -1
        } else {
            subtract_if_nonnegative(horizontal, 1);
        }
    }
    velocity_x = horizontal;
    velocity_y = vertical;
    modifiers = state;
}
} // namespace unirally
