#include "speed_limits.hpp"
#include <algorithm>
#include <stdexcept>

namespace unirally {
namespace {

// $82:A705-A76F: drag slows by 3; a boost decays by 16 while the rider's screen byte says
// it is far ahead (0xB0 and up moving right) or behind (under 0x10, the opponent 0x40,
// moving left).
constexpr std::uint16_t drag_step = 3, fast_boost_decay = 16;
constexpr std::uint8_t far_right = 0xb0, far_left_player = 0x10, far_left_opponent = 0x40;
// $82:A774-A7F8: the extra speed over the cap is the boost plus the progress term, 0 to 384
// (PAL immediates at file 0x1277A and 0x127F1); a launch allows 80. Half of it adds to the
// horizontal base cap and to the vertical cap of 768 (file 0x12830).
constexpr std::uint16_t launch_extra = 80, most_extra = 384, vertical_base_cap = 768;
// $82:A875-A8A4: the decay content has nine buckets of 16 extra.
constexpr std::size_t decay_buckets = 9;
constexpr unsigned last_decay_bucket = 8;
constexpr std::uint16_t neutral_friction = 1;
// Cartridge modes 2 and 42 select an alternate vertical cap, not recovered.
constexpr std::uint8_t alternate_cap_mode = 2, alternate_cap_mode_42 = 42;

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

// $82:A6FD-A8A4: one update of a rider's speed limit: drag or a boost's decay, the caps
// from the boost and the progress term, the boost's counter-controlled decay, and a
// neutral D-pad's friction.
void limit_rider_speed(std::uint16_t& velocity_x, std::uint16_t& velocity_y,
                       SpeedModifiers& modifiers, const SpeedLimitContext& context,
                       const SpeedDecayContent& content) {
    if (context.skip) return;
    if (context.cartridge_mode == alternate_cap_mode
        || context.cartridge_mode == alternate_cap_mode_42) {
        throw std::invalid_argument("alternate cartridge vertical cap is not recovered");
    }
    if (content.masks.size() != decay_buckets || content.decrements.size() != 2 * decay_buckets) {
        throw std::invalid_argument("speed decay content must contain nine masks and nine words");
    }
    auto state = modifiers;
    auto horizontal = velocity_x;
    auto vertical = velocity_y;
    bool fast_decay{};
    // The pose branch only selects the boost's decay; its DEC/INC accumulator result is
    // discarded before any velocity store.
    if (context.drag) {
        horizontal =
            negative(horizontal) ? add(horizontal, drag_step) : subtract(horizontal, drag_step);
        fast_decay = true;
    } else if (!negative(horizontal)) {
        fast_decay = context.pose_byte >= far_right;
    } else {
        fast_decay = context.pose_byte < (context.opponent ? far_left_opponent : far_left_player);
    }
    if (fast_decay) subtract_if_nonnegative(state.boost, fast_boost_decay);

    std::uint16_t extra = launch_extra;
    if (!context.start_override) {
        extra = add(state.boost, progress_contribution(state, context));
        extra = negative(extra) ? 0 : std::min<std::uint16_t>(extra, most_extra);
    }
    horizontal = cap_velocity(
        horizontal, add(static_cast<std::uint16_t>(extra >> 1U), context.player_base_cap));
    vertical =
        cap_velocity(vertical, add(static_cast<std::uint16_t>(extra >> 1U), vertical_base_cap));
    subtract_if_nonnegative(state.vertical_boost, 1);

    // The counter-controlled decay follows this update's extra.
    const auto bucket = std::min<unsigned>(static_cast<unsigned>(extra >> 4U), last_decay_bucket);
    const auto mask = content.masks[bucket];
    if ((context.update_counter & mask) == mask) {
        const auto decrement = static_cast<std::uint16_t>(
            static_cast<unsigned>(content.decrements[bucket * 2])
            | (static_cast<unsigned>(content.decrements[bucket * 2 + 1]) << 8U));
        subtract_if_nonnegative(state.boost, decrement);
    }
    if (context.friction_mode == neutral_friction) {
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
