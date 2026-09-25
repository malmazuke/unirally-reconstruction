// A rider's pose: the idle wobble, the pose and animation update, rolling and quarter turns.

#include "rider_pose.hpp"

#include "word_arithmetic.hpp"
#include "zoom_zoo_movement.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>

namespace unirally {

void decay_idle_wobble(RiderMovementState& rider, bool surface_mode) {
    // $81:8625-$81:8672 preserves an offset produced by the preceding idle
    // update for one frame. Otherwise it approaches zero by five, or by two
    // while the contact response word is nonzero, and then clears the marker.
    auto& idle = rider.idle_pose;
    if (idle.active == 0) {
        auto offset = static_cast<std::int16_t>(idle.wobble_offset);
        if (offset >= 512 || offset < -512) {
            idle.wobble_offset = 0;
        } else if (offset != 0) {
            const int step = surface_mode ? 2 : 5;
            offset = static_cast<std::int16_t>(offset > 0
                                                   ? std::max(0, static_cast<int>(offset) - step)
                                                   : std::min(0, static_cast<int>(offset) + step));
            idle.wobble_offset = static_cast<std::uint16_t>(offset);
        }
    }
    idle.active = 0;
}

namespace {

void clear_idle_cycle(IdlePoseState& idle) {
    // The reset path intentionally preserves wobble_offset and
    // orientation_reference ($0F37/$0F83).
    idle.bias = idle.velocity = idle.previous_bias = idle.direction_adjustment = 0;
    idle.active = idle.cycle_latched = idle.cycle_counter = 0;
}

} // namespace

void update_idle_pose(RiderMovementState& rider, bool race_active, bool opponent,
                      std::uint8_t animation_counter, std::span<const std::uint8_t> table) {
    if (table.size() != 64) throw std::invalid_argument("idle pose table has the wrong size");
    auto& idle = rider.idle_pose;
    if (!race_active || static_cast<std::int16_t>(rider.motion.previous_x_displacement) >= 2
        || static_cast<std::int16_t>(rider.contact.unsupported_count) >= 2) {
        clear_idle_cycle(idle);
        return;
    }
    if (idle.cycle_latched == 0) {
        const auto next = add_word(idle.cycle_counter, 1);
        if (static_cast<std::int16_t>(next) < 120) idle.cycle_counter = next;
    }
    idle.orientation_reference = rider.pose.reflected_orientation;
    if (idle.orientation_reference != 0 && rider.pose.reflected) {
        idle.orientation_reference = static_cast<std::uint16_t>(64 - idle.orientation_reference);
    }
    if (static_cast<std::int16_t>(rider.pose.pose_index) >= 0x0aec) {
        clear_idle_cycle(idle);
        return;
    }
    idle.active = 1;

    const auto reference = static_cast<std::int16_t>(idle.orientation_reference);
    auto bias = static_cast<std::int16_t>(idle.bias);
    if (bias != 0) {
        if (reference != 0) {
            idle.direction_adjustment = 1;
        } else if (bias > 0) {
            idle.bias = static_cast<std::uint16_t>(bias - 1);
            idle.direction_adjustment = 1;
        } else {
            idle.bias = 0;
            idle.direction_adjustment = 0;
        }
    } else {
        idle.bias = 0;
        idle.direction_adjustment = 0;
    }

    auto velocity = static_cast<std::int16_t>(idle.velocity);
    int candidate = velocity;
    // $0FF9 selects the counter pair at $04C7/$04C9. The opponent word is the
    // modulo-32 complement used by the original's second rider pass.
    const auto rider_counter =
        opponent ? static_cast<std::uint8_t>((32U - animation_counter) & 31U) : animation_counter;
    const bool increase = reference == 0 ? (rider_counter & 0x10U) != 0 : reference >= 32;
    if (increase) {
        candidate = velocity + 1 + static_cast<std::int16_t>(idle.direction_adjustment);
        if (candidate < 17) idle.velocity = static_cast<std::uint16_t>(candidate);
    } else {
        candidate = velocity - 1 - static_cast<std::int16_t>(idle.direction_adjustment);
        if (candidate >= -16) idle.velocity = static_cast<std::uint16_t>(candidate);
    }

    if (animation_counter == 0) {
        idle.bias = static_cast<std::uint16_t>(candidate);
        if (static_cast<std::int16_t>(idle.cycle_counter) >= 60 && idle.cycle_latched == 0) {
            idle.cycle_latched = 1;
            idle.cycle_counter = 0;
        }
        if (idle.previous_bias == idle.bias) idle.bias = 0;
        idle.previous_bias = idle.bias;
    }

    velocity = static_cast<std::int16_t>(idle.velocity);
    bool use_table = false;
    if (velocity == 0) {
        if (reference < 9 || reference >= 58) {
            use_table = true;
        } else {
            idle.velocity = static_cast<std::uint16_t>(reference < 32 ? -1 : 1);
        }
    } else if (velocity < 0) {
        // $82:A1F2-A1FF: a falling velocity below 32 is applied unchanged.
        // An earlier reset to -1 for 9..31 had no source; DRAGSTER diff fuzz
        // seed 66 reaches it at 1977 (R-0038).
        if (reference >= 32 && reference < 58) use_table = true;
    } else if (reference >= 9 && reference < 32) {
        use_table = true;
    }
    const auto delta = use_table
                         ? static_cast<std::int8_t>(table[static_cast<std::size_t>(reference)])
                         : static_cast<std::int16_t>(idle.velocity);
    idle.wobble_offset = add_word(idle.wobble_offset, static_cast<std::uint16_t>(delta));
}

void update_rolling_mode(RiderMovementState& rider, bool surface_mode) {
    if (rider.contact.selected_high & 0x80U) {
        rider.pose.rolling = true;
        return;
    }
    if (surface_mode) {
        rider.pose.rolling = false;
        return;
    }
    if (rider.contact.unsupported_count == 9 || rider.pose.reflected_orientation < 45) {
        rider.pose.rolling = false;
    } else if (rider.pose.rolling
               && static_cast<std::int16_t>(rider.motion.previous_x_displacement) < 14) {
        rider.pose.rolling = false;
    } else if (!rider.pose.rolling
               && static_cast<std::int16_t>(rider.motion.previous_x_displacement) < 16) {
        return;
    } else {
        const bool moving_nonnegative = static_cast<std::int16_t>(rider.motion.velocity_x) >= 0;
        rider.pose.rolling = rider.pose.reflected ? moving_nonnegative : !moving_nonnegative;
    }
}

void update_pose(RiderMovementState& rider, std::uint8_t counter, std::uint8_t contact_phase,
                 const MovementContent& content, int animation_override, bool use_throttle_target,
                 int surface_angle_offset, std::uint16_t mud_velocity, bool slow_tile) {
    if (content.pose_slopes.size() != 128 || content.displacement_table.size() != 512) {
        throw std::invalid_argument("movement pose tables have the wrong size");
    }
    if (!rider.contact.angle_unspecified) {
        int target_signed{};
        if (rider.pose.rolling && rider.pose.rolling_level != 0) {
            target_signed = static_cast<std::int16_t>(rider.contact.surface_angle)
                          + (rider.pose.reflected ? 23 : -23);
        } else {
            // $83:EF97-EFB2: throttle after a small-displacement drive,
            // otherwise mud's braked velocity ($0F3F) or velocity x. On flag
            // pair 8 ($0F2D, $83:EFA1) velocity x takes one shift, not five.
            const auto target_source =
                static_cast<std::int16_t>(use_throttle_target ? rider.throttle
                                          : mud_velocity      ? mud_velocity
                                                              : rider.motion.velocity_x);
            target_signed = target_source >> ((slow_tile && !use_throttle_target) ? 1 : 5);
        }
        if (rider.contact.selected_high & 0x80U) target_signed = 0;
        target_signed = std::clamp(target_signed + surface_angle_offset, -31, 31);
        const unsigned target_index = target_signed >= 0
                                        ? static_cast<unsigned>(target_signed)
                                        : static_cast<unsigned>(-target_signed + 32);
        rider.pose.target_orientation =
            content.pose_slopes[target_index + ((rider.contact.selected_high & 0x80U) ? 64U : 0U)];
    }
    auto orientation = static_cast<std::uint16_t>(rider.pose.orientation + rider.motion.response_b);
    if (rider.motion.response_a) {
        orientation = add_word(orientation, rider.motion.response_a);
    } else if (rider.contact.unsupported_count < 9
               && rider.pose.target_orientation == rider.pose.orientation) {
        // $83:F02D-F034 skips the store: a supported rider already at its
        // target keeps its orientation, so rotation input is not applied
        // (DRAGSTER diff fuzz seed 135, the update after an L-held landing).
        orientation = rider.pose.orientation;
    } else if (rider.contact.unsupported_count < 9) {
        const auto target = rider.pose.target_orientation;
        const bool increase =
            target >= 32 ? (static_cast<std::int16_t>(target - rider.pose.orientation) >= 1
                            && static_cast<std::int16_t>(target - rider.pose.orientation) <= 32)
                         : !(static_cast<std::int16_t>(rider.pose.orientation - target) >= 1
                             && static_cast<std::int16_t>(rider.pose.orientation - target) <= 32);
        const int direction = increase ? 1 : -1;
        orientation = static_cast<std::uint16_t>(rider.pose.orientation + direction);
        if (static_cast<std::int16_t>(rider.motion.previous_x_displacement) >= 16) {
            for (unsigned step = 0; step < 2 && (orientation & 63U) != target; ++step) {
                orientation =
                    static_cast<std::uint16_t>(static_cast<int>(orientation & 63U) + direction);
            }
        }
    }
    rider.pose.orientation = orientation & 63U;
    // $83:F09E-$83:F0B9 applies signed truncation toward zero to the idle
    // oscillator before the contact impulse and reflection operations.
    const int wobble = static_cast<std::int16_t>(rider.idle_pose.wobble_offset) / 16;
    const int combined = static_cast<int>(rider.pose.orientation) + wobble
                       + (static_cast<std::int16_t>(rider.motion.orientation_impulse) >> 1);
    rider.pose.reflected_orientation = static_cast<std::uint16_t>(combined) & 63U;
    if ((counter & 1U) == 0 && rider.motion.orientation_impulse) {
        const auto impulse = static_cast<std::int16_t>(rider.motion.orientation_impulse);
        rider.motion.orientation_impulse =
            static_cast<std::uint16_t>(impulse + (impulse >= 0 ? -1 : 1));
    }
    if (rider.pose.reflected_orientation && rider.pose.reflected) {
        rider.pose.reflected_orientation = 64 - rider.pose.reflected_orientation;
    }
    rider.pose.displacement_history[2] = rider.pose.displacement_history[1];
    rider.pose.displacement_history[1] = rider.pose.displacement_history[0];
    rider.pose.displacement_history[0] = rider.motion.previous_x_displacement;
    int steering{};
    if (rider.jump.impulse_phase >= 1 || rider.contact.angle_unspecified) {
        steering = static_cast<std::int16_t>(rider.pose.animation_increment);
        if ((counter & 3U) == 3) steering += steering > 0 ? -1 : (steering < 0 ? 1 : 0);
        rider.pose.animation_increment = static_cast<std::uint16_t>(steering);
    } else {
        int dx = static_cast<std::int16_t>(rider.pose.previous_x - rider.motion.x);
        int dy = static_cast<std::int16_t>(rider.pose.previous_y - rider.motion.y);
        if (std::abs(dy) < 3) dy = 0;
        const auto square_x = content_word(content.displacement_table, 2U * (std::abs(dx) & 255));
        const auto square_y = content_word(content.displacement_table, 2U * (std::abs(dy) & 255));
        int distance = static_cast<int>(integer_sqrt((square_x + square_y) & 65535U));
        if (dx < 0) distance = -distance;
        rider.motion.previous_x_displacement = static_cast<std::uint16_t>(distance);
        const int accumulation =
            static_cast<std::int16_t>(rider.pose.displacement_remainder + distance);
        int remainder{};
        if (accumulation) {
            steering = std::abs(accumulation) / 3;
            remainder = std::abs(accumulation) % 3;
            if (accumulation < 0) steering = -steering;
            if (dx < 0) remainder = -remainder;
        } else {
            steering = distance;
        }
        rider.pose.displacement_remainder = static_cast<std::uint16_t>(remainder);
        rider.pose.animation_increment = static_cast<std::uint16_t>(steering);
    }
    if (rider.pose.reflected) steering = -steering;
    if (animation_override) steering = animation_override;
    int phase = static_cast<std::int16_t>(rider.pose.animation_phase) + steering;
    phase %= 24;
    if (phase < 0) phase += 24;
    rider.pose.animation_phase = static_cast<std::uint16_t>(phase);
    int animation = phase * 64;
    if (rider.pose.rolling) {
        const auto rate = std::abs(static_cast<std::int16_t>(rider.pose.animation_increment));
        if (rate < 3 && static_cast<std::uint8_t>(rider.pose.rolling_level) != 0) {
            rider.pose.rolling_level = static_cast<std::uint16_t>(
                static_cast<std::uint8_t>(rider.pose.rolling_level) - 1U);
        } else if (rate >= 5 && static_cast<std::uint8_t>(rider.pose.rolling_level) != 2) {
            rider.pose.rolling_level = static_cast<std::uint16_t>(
                static_cast<std::uint8_t>(rider.pose.rolling_level) + 1U);
        }
        // $83:EED2-EEDB always returns to the ordinary pose after
        // the low-rate decrement, even when rolling level remains nonzero.
        if (rate >= 3 && static_cast<std::uint8_t>(rider.pose.rolling_level) != 0) {
            int alternate = static_cast<std::uint16_t>(rider.pose.alternate_animation_phase)
                          + (phase & 1) + contact_phase;
            if (alternate >= 3) alternate -= 3;
            rider.pose.alternate_animation_phase = static_cast<std::uint16_t>(alternate);
            animation = alternate * 64
                      + (static_cast<std::uint8_t>(rider.pose.rolling_level) == 1 ? 0x8e0 : 0x820);
        }
    }
    rider.pose.pose_index =
        static_cast<std::uint16_t>(animation + rider.pose.reflected_orientation);
    rider.motion.previous_x_displacement = static_cast<std::uint16_t>(
        std::abs(static_cast<std::int16_t>(rider.motion.previous_x_displacement)));
    rider.pose.previous_x = rider.motion.x;
    rider.pose.previous_y = rider.motion.y;
    (void)contact_phase;
}

int stationary_animation_override(const RiderMovementState& rider, std::uint8_t horizontal) {
    const bool prior_motion =
        std::any_of(rider.pose.displacement_history.begin(), rider.pose.displacement_history.end(),
                    [](auto value) { return static_cast<std::int16_t>(value) >= 2; })
        || static_cast<std::int16_t>(rider.motion.previous_x_displacement) >= 2;
    if ((rider.contact.unsupported_count < 2 && prior_motion) || horizontal == 1) return 0;
    int value = horizontal < 1 ? 2 : -2;
    return rider.pose.reflected ? -value : value;
}

unsigned update_quarter_turns(RiderMovementState& rider, bool leading_support, unsigned air_turns,
                              bool rolling, unsigned held_rotations) {
    auto& turns = rider.quarter_turn;
    const bool vertical_endpoint =
        std::abs(static_cast<std::int16_t>(rider.contact.surface_angle)) == 31;
    if (vertical_endpoint || (!rider.motion.response_a && rider.contact.unsupported_count >= 2)) {
        // $829ABD-AF3 re-bases an active roll's quadrant and clears only
        // partial quarters, then still compares the current angle this update.
        // Completed turns and entry reflection survive; full init is separate.
        if (rolling && !held_rotations
            && (vertical_endpoint || rider.contact.unsupported_count != 2)) {
            const auto rotation = static_cast<std::int8_t>(rider.motion.response_b & 0xffU);
            turns.previous_quadrant =
                (static_cast<std::uint16_t>(static_cast<int>(rider.pose.orientation) - rotation)
                 & 63U)
                >> 4U;
            turns.forward_quarters = turns.reverse_quarters = 0;
        }
        if ((!vertical_endpoint && rider.contact.unsupported_count == 2) || !turns.initialized) {
            const auto rotation = static_cast<std::int8_t>(rider.motion.response_b & 0xffU);
            const int prior = static_cast<int>(rider.pose.orientation) - rotation;
            turns.previous_quadrant = static_cast<std::uint16_t>(prior) & 63U;
            turns.previous_quadrant = static_cast<std::uint16_t>(turns.previous_quadrant >> 4U);
            turns.reflected_at_start = rider.pose.reflected;
            turns.initialized = true;
            turns.forward_turns = turns.reverse_turns = 0;
            turns.forward_quarters = turns.reverse_quarters = 0;
        } else {
            const auto current = static_cast<std::uint16_t>(rider.pose.orientation >> 4U);
            const auto previous = turns.previous_quadrant;
            if (current != previous) {
                bool increasing{};
                if (current == 3)
                    increasing = previous == 2;
                else if (previous == 3)
                    increasing = current != 2;
                else
                    increasing = current > previous;
                auto& quarters = increasing ? turns.forward_quarters : turns.reverse_quarters;
                auto& opposite = increasing ? turns.reverse_quarters : turns.forward_quarters;
                auto& completed = increasing ? turns.forward_turns : turns.reverse_turns;
                quarters = add_word(quarters, 1);
                if (quarters >= 4) {
                    completed = add_word(completed, 1);
                    quarters = 0;
                }
                opposite = 0;
            }
            turns.previous_quadrant = current;
        }
        return false;
    }
    if (leading_support) {
        const bool event = turns.forward_turns + turns.reverse_turns + air_turns != 0;
        if (event) rider.speed.boost = rider.speed.vertical_boost = 0;
        turns.previous_quadrant = turns.forward_turns = turns.reverse_turns = 0;
        turns.forward_quarters = turns.reverse_quarters = 0;
        turns.initialized = false;
        return event ? 14U : 0U;
    }
    if (!turns.initialized) return false;
    if (turns.forward_quarters == 3) turns.forward_turns = add_word(turns.forward_turns, 1);
    if (turns.reverse_quarters == 3) turns.reverse_turns = add_word(turns.reverse_turns, 1);
    const auto forward = std::min<std::uint16_t>(turns.forward_turns, 4);
    const auto reverse = std::min<std::uint16_t>(turns.reverse_turns, 4);
    unsigned event{};
    if (forward) event = forward + (turns.reflected_at_start ? 0U : 4U);
    if (reverse) {
        if (event)
            throw std::invalid_argument("combined rotation reward is outside the recovered domain");
        event = reverse + (turns.reflected_at_start ? 4U : 0U);
    }
    turns.previous_quadrant = turns.forward_turns = turns.reverse_turns = 0;
    turns.forward_quarters = turns.reverse_quarters = 0;
    turns.initialized = false;
    if (event == 0) return false;
    if (event == 1) return true;
    throw std::invalid_argument("rotation reward is outside the recovered event-one domain");
}

} // namespace unirally
