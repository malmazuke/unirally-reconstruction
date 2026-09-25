// A rider's horizontal drive, jump, gravity, damping and position integration.

#include "rider_motion.hpp"

#include "word_arithmetic.hpp"
#include "zoom_zoo_movement.hpp"

#include <cstdint>
#include <stdexcept>

namespace unirally {

namespace {

std::uint16_t speed_toward_zero(std::uint16_t velocity, std::int16_t amount) {
    const auto speed = static_cast<std::int16_t>(velocity);
    if (speed >= amount) return static_cast<std::uint16_t>(speed - amount);
    // PAL CMP #$FFF6 / BPL keeps negative equality unchanged; its positive
    // CMP #10 / BMI boundary is intentionally asymmetric.
    if (speed < -amount) return static_cast<std::uint16_t>(speed + amount);
    return velocity;
}

} // namespace

void update_horizontal(RiderMovementState& rider, bool brake, bool accelerate, bool opponent,
                       const MovementState& whole, const MovementContent& content,
                       int& animation_override, bool& use_throttle_target) {
    // $81:8592 common reset clears the one-update launch override before the
    // throttle routine. The value written by a launch remains in the serialized
    // end-of-frame state and is cleared at the next call.
    rider.launch_override = 0;
    if (rider.contact.unsupported_count >= 2) {
        rider.throttle = 0;
    } else {
        if (brake && rider.motion.velocity_x != 0) {
            throw std::invalid_argument("moving brake is outside the recovered movement domain");
        }
        if (accelerate) {
            rider.motion.velocity_x = add_word(rider.motion.velocity_x, 24);
            const auto signed_boost = static_cast<std::int16_t>(rider.speed.boost);
            const auto limit = static_cast<std::uint16_t>(
                448U + (signed_boost > 0 ? static_cast<unsigned>(signed_boost) : 0U));
            const auto candidate = add_word(rider.throttle, 16);
            if (negative(static_cast<std::uint16_t>(candidate - limit))) rider.throttle = candidate;
        } else {
            rider.throttle = 0;
        }
        if (brake) {
            rider.motion.velocity_x = 0;
        } else if (accelerate && rider.previous_brake != 0 && rider.throttle != 0) {
            rider.motion.velocity_x = add_word(rider.motion.velocity_x, rider.throttle);
            rider.throttle = 0;
            rider.launch_override = 256;
        }
    }
    rider.previous_brake = brake ? 1 : 0;

    if (accelerate && static_cast<std::int16_t>(rider.motion.previous_x_displacement) < 4) {
        if (rider.small_motion_counter != 4)
            rider.small_motion_counter = add_word(rider.small_motion_counter, 1);
        animation_override = static_cast<std::int16_t>(rider.small_motion_counter);
        use_throttle_target = true;
    }

    SpeedLimitContext context{};
    context.opponent = opponent;
    context.pose_byte = 0; // Primary screen-coordinate values 43..104 keep this branch inactive.
    context.start_override = rider.launch_override != 0;
    context.ai_enabled = true;
    context.player_progress = whole.riders[0].progress.transition_count;
    context.opponent_progress = whole.riders[1].progress.transition_count;
    context.adjustment_limit = 96;
    context.player_base_cap = 448;
    context.update_counter = whole.update_counter;
    context.friction_mode = accelerate ? 2 : 1;
    limit_rider_speed(rider.motion.velocity_x, rider.motion.velocity_y, rider.speed, context,
                      content.speed_decay);
}

void update_jump(RiderMovementState& rider, bool jump_input) {
    bool advance = rider.jump.impulse_phase != 0;
    if (!advance && rider.jump.pending) {
        if (rider.contact.angle_unspecified) {
            rider.jump.previous_input = jump_input ? 1 : 0;
            return;
        }
        rider.jump.pending = 0;
        advance = true;
    }
    if (!advance) {
        if (!rider.jump.previous_input && jump_input && rider.contact.unsupported_count < 2) {
            rider.jump.pending = 1;
        }
    } else if (!jump_input) {
        rider.jump.impulse_phase = 0;
    } else {
        rider.jump.impulse_phase = add_word(rider.jump.impulse_phase, 1);
        if (rider.jump.impulse_phase >= 9) {
            rider.jump.impulse_phase = 0;
        } else {
            const auto impulse = static_cast<std::uint16_t>(
                rider.jump.baseline + (rider.jump.impulse_phase < 5 ? -144 : -192));
            if (!negative(static_cast<std::uint16_t>(rider.motion.velocity_y - impulse))) {
                rider.motion.velocity_y = impulse;
            }
        }
    }
    rider.jump.previous_input = jump_input ? 1 : 0;
}

void update_active_low_speed_damping(RiderMovementState& rider) {
    // $82:A5FA-A61E runs only on the rider's alternating active phase. In the
    // recovered flat branch it moves nonzero velocities with magnitude below
    // 64 one unit toward zero before the general speed limiter/friction pass.
    if (rider.contact.surface_angle != 0 || rider.motion.velocity_x == 0) return;
    const auto velocity = static_cast<std::int16_t>(rider.motion.velocity_x);
    if (velocity > 0 && velocity < 64) {
        rider.motion.velocity_x = static_cast<std::uint16_t>(velocity - 1);
    } else if (velocity < 0 && velocity >= -64) {
        rider.motion.velocity_x = static_cast<std::uint16_t>(velocity + 1);
    }
}

void apply_finish_slowdown(RiderMovementState& rider) {
    // $83:E90D-$83:E932 runs before the ordinary horizontal update. It moves
    // signed velocity ten units toward zero only when it cannot cross zero.
    // Keeping that ordering is what produces both the 38->2 accepted
    // tail and the reviewer-owned 37->1 neighboring case.
    rider.motion.velocity_x = finish_speed_toward_zero(rider.motion.velocity_x);
}

void update_gravity(RiderMovementState& rider) {
    const auto vertical = static_cast<std::int16_t>(rider.motion.velocity_y);
    if (vertical >= 512) return;
    const auto increment = vertical < 0 ? 19 : 19 - (vertical >> 5);
    rider.motion.velocity_y = static_cast<std::uint16_t>(vertical + increment);
    rider.motion.y = add_word(rider.motion.y, 1);
}

void integrate_motion(RiderMovementState& rider) {
    auto axis = [](std::uint16_t& position, std::uint16_t velocity, std::uint16_t& residue) {
        const auto total = static_cast<std::int32_t>(static_cast<std::int16_t>(velocity))
                         + static_cast<std::int16_t>(residue);
        const auto magnitude = total < 0 ? -total : total;
        auto whole = magnitude / 32;
        auto remainder = magnitude % 32;
        if (total < 0) {
            whole = -whole;
            remainder = -remainder;
        }
        position = static_cast<std::uint16_t>(position + whole);
        residue = static_cast<std::uint16_t>(remainder);
        return static_cast<std::int16_t>(whole);
    };
    (void)axis(rider.motion.x, rider.motion.velocity_x, rider.residue_x);
    (void)axis(rider.motion.y, rider.motion.velocity_y, rider.residue_y);
}

std::uint16_t finish_speed_toward_zero(std::uint16_t velocity) {
    return speed_toward_zero(velocity, 10);
}

} // namespace unirally
