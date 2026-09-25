// A rider's drive, brake and throttle, jump, gravity, damping and position integration.

#include "rider_motion.hpp"

#include "word_arithmetic.hpp"
#include "zoom_zoo_movement.hpp"

#include <algorithm>
#include <cstdint>
#include <stdexcept>

namespace unirally {

namespace {

// The legacy DRAGSTER drive: 24 a update, a throttle charging by 16 up to 448 plus a
// positive boost, and a launch that lifts the speed limit by 256 for one update.
constexpr std::uint16_t dragster_drive_step = 24, dragster_throttle_cap = 448;
constexpr std::uint16_t throttle_step = 16, launch_override = 256;
// Braking takes effect from 16 rightward but only below -16 leftward ($82:9909): the
// test is asymmetric, so -16 does not brake. Keep it (see braking_fast_enough).
constexpr std::int16_t braking_speed = 16;
// Barely moving (an x displacement under 4) counts up to 4 updates of small motion,
// which picks the pushing animation.
constexpr std::int16_t small_motion_displacement = 4;
constexpr std::uint16_t most_small_motion_updates = 4;
// The surface angles of a vertical wall, facing either way.
constexpr std::uint16_t wall_angle_left = 0xffe1, wall_angle_right = 31;
constexpr std::uint16_t inverted_tile = 0x8000; // the selected tile word's flag

std::uint16_t speed_toward_zero(std::uint16_t velocity, std::int16_t amount) {
    const auto speed = static_cast<std::int16_t>(velocity);
    if (speed >= amount) return static_cast<std::uint16_t>(speed - amount);
    // PAL CMP #$FFF6 / BPL keeps negative equality unchanged; its positive
    // CMP #10 / BMI boundary is intentionally asymmetric.
    if (speed < -amount) return static_cast<std::uint16_t>(speed + amount);
    return velocity;
}

void count_small_motion(RiderMovementState& rider, int& animation_override, bool& throttle_target) {
    if (static_cast<std::int16_t>(rider.motion.previous_x_displacement)
        >= small_motion_displacement)
        return;
    if (rider.small_motion_counter != most_small_motion_updates)
        rider.small_motion_counter = add_word(rider.small_motion_counter, 1);
    animation_override = static_cast<std::int16_t>(rider.small_motion_counter);
    throttle_target = true;
}

bool braking_fast_enough(std::int16_t speed) {
    return speed >= braking_speed || speed < -braking_speed;
}

bool on_wall(const RiderMovementState& rider) {
    return rider.contact.surface_angle == wall_angle_left
        || rider.contact.surface_angle == wall_angle_right;
}

bool on_inverted_tile(const RiderMovementState& rider) {
    return (rider.contact.selected_word & inverted_tile) != 0;
}

// The velocity part of the drive routines $82:A9B3 (rightward) and $82:AA10 (leftward).
// On an inverted tile both push a nonzero velocity away from zero; a roll bounce ($042B)
// stores nothing.
void drive_velocity(RiderMovementState& rider, bool rightward, std::uint16_t step,
                    bool bounce_active) {
    if (bounce_active) return;
    const auto back = static_cast<std::uint16_t>(0U - step);
    auto& velocity = rider.motion.velocity_x;
    if (on_inverted_tile(rider)) {
        if (velocity) velocity = add_word(velocity, negative(velocity) ? back : step);
    } else {
        velocity = add_word(velocity, rightward ? step : back);
    }
}

// $82:9909-9945 brakes through the opposite drive routine and stops a velocity that
// crossed zero. A bounce keeps the velocity (diff fuzz seed 140).
void brake(RiderMovementState& rider, std::int16_t incoming_speed, std::uint16_t step,
           bool bounce_active) {
    if (incoming_speed > 0) {
        drive_velocity(rider, false, step, bounce_active);
        if (negative(rider.motion.velocity_x)) rider.motion.velocity_x = 0;
    } else {
        drive_velocity(rider, true, step, bounce_active);
        if (!negative(rider.motion.velocity_x)) rider.motion.velocity_x = 0;
    }
    rider.throttle = 0;
}

// The throttle charges 16 a update in the drive's direction while it stays under the cap:
// the transition's base cap, a positive boost, and a launch's override.
void charge_throttle(RiderMovementState& rider, const ReflectionTransition& transition,
                     bool rightward) {
    const auto cap = static_cast<std::uint16_t>(
        transition.base_velocity_cap
        + std::max(0, static_cast<int>(static_cast<std::int16_t>(rider.speed.boost)))
        + rider.launch_override);
    const auto next = add_word(
        rider.throttle, rightward ? throttle_step : static_cast<std::uint16_t>(-throttle_step));
    const bool under_cap =
        rightward ? negative(static_cast<std::uint16_t>(next - cap))
                  : !negative(static_cast<std::uint16_t>(next - static_cast<std::uint16_t>(-cap)));
    if (under_cap) rider.throttle = next;
}

// Holding the brake keeps the rider still and announces a charged throttle
// ($82:9995-99EF): a reflection step clears the announcement but keeps the throttle
// ($0F5F), a nonzero throttle sets it, a zero one leaves it (Left and Y through the
// countdown carry the throttle through zero, DRAGSTER fuzz seed 208). On flag pair 12
// ($0FB1) or after mud ($0F45) the brake drops the throttle too ($82:9981-9993).
void hold_brake(RiderMovementState& rider, const ReflectionTransition& transition,
                std::uint16_t& charge_announced, bool brake_drops_throttle) {
    rider.motion.velocity_x = 0;
    if (brake_drops_throttle) {
        rider.throttle = 0;
        charge_announced = 0;
    } else if (transition.step) {
        charge_announced = 0;
    } else if (rider.throttle) {
        charge_announced = 1;
    }
}

// $82:99F1-9A49: releasing the brake launches any throttle and clears the announcement.
void release_brake(RiderMovementState& rider, std::uint16_t& charge_announced) {
    if (!rider.previous_brake) return;
    if (rider.throttle) {
        rider.motion.velocity_x = add_word(rider.motion.velocity_x, rider.throttle);
        rider.throttle = 0;
        rider.launch_override = launch_override;
    }
    charge_announced = 0;
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
            rider.motion.velocity_x = add_word(rider.motion.velocity_x, dragster_drive_step);
            const auto signed_boost = static_cast<std::int16_t>(rider.speed.boost);
            const auto limit = static_cast<std::uint16_t>(
                dragster_throttle_cap
                + (signed_boost > 0 ? static_cast<unsigned>(signed_boost) : 0U));
            const auto candidate = add_word(rider.throttle, throttle_step);
            if (negative(static_cast<std::uint16_t>(candidate - limit))) rider.throttle = candidate;
        } else {
            rider.throttle = 0;
        }
        if (brake) {
            rider.motion.velocity_x = 0;
        } else if (accelerate && rider.previous_brake != 0 && rider.throttle != 0) {
            rider.motion.velocity_x = add_word(rider.motion.velocity_x, rider.throttle);
            rider.throttle = 0;
            rider.launch_override = launch_override;
        }
    }
    rider.previous_brake = brake ? 1 : 0;
    if (accelerate) count_small_motion(rider, animation_override, use_throttle_target);

    SpeedLimitContext context{};
    context.opponent = opponent;
    context.pose_byte = 0; // Primary screen-coordinate values 43..104 keep this branch inactive.
    context.start_override = rider.launch_override != 0;
    context.ai_enabled = true;
    context.player_progress = whole.riders[0].progress.transition_count;
    context.opponent_progress = whole.riders[1].progress.transition_count;
    context.adjustment_limit = 96;
    context.player_base_cap = dragster_throttle_cap;
    context.update_counter = whole.update_counter;
    context.friction_mode = accelerate ? 2 : 1;
    limit_rider_speed(rider.motion.velocity_x, rider.motion.velocity_y, rider.speed, context,
                      content.speed_decay);
}

namespace {
// A held jump pushes velocity y up to its baseline less 144 for four updates, then less 192
// until the ninth; gravity adds 19 a update less a 32nd of the fall, until 512.
constexpr std::uint16_t jump_updates = 9, strong_jump_updates = 5;
constexpr int strong_jump = -144, late_jump = -192;
constexpr int fastest_fall = 512, gravity = 19;
} // namespace

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
        if (rider.jump.impulse_phase >= jump_updates) {
            rider.jump.impulse_phase = 0;
        } else {
            const auto impulse = static_cast<std::uint16_t>(
                rider.jump.baseline
                + (rider.jump.impulse_phase < strong_jump_updates ? strong_jump : late_jump));
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
    if (vertical >= fastest_fall) return;
    const auto increment = vertical < 0 ? gravity : gravity - (vertical >> 5);
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

// $82:98CF-9A52, one update of a race rider's drive: brake hard, or drive in the held
// direction and charge the throttle, then hold or release the brake. Only a rider on the
// ground drives; in the air, on a wall or with no direction held the throttle and its
// announcement ($0F63, serialized from $0D53/$0D55) clear ($82:999A, $82:9A2B).
void update_drive(RiderMovementState& rider, ReflectionTransition& transition, unsigned horizontal,
                  int& animation_override, bool& throttle_target, std::uint16_t& charge_announced,
                  bool leading_support, bool bounce_active, std::uint16_t drive_step,
                  bool brake_drops_throttle) {
    const auto incoming_speed = static_cast<std::int16_t>(rider.motion.velocity_x);
    const bool grounded =
        !leading_support && rider.contact.unsupported_count < 2 && !on_wall(rider);
    const bool braking = transition.brake_input && braking_fast_enough(incoming_speed);
    if (grounded && braking) {
        brake(rider, incoming_speed, drive_step, bounce_active);
        return; // before the previous-brake latch
    }
    if (!grounded || horizontal == direction::neutral) {
        rider.throttle = 0;
        charge_announced = 0;
    } else {
        // $82:A9C1-A9E0 / AA1E-AA3D: an inverted tile with no velocity returns before the
        // throttle ($82:A9CC / $82:AA29); a roll bounce keeps the velocity but still
        // charges ($82:AA42-AA49).
        const bool rightward = horizontal == direction::right;
        if (!on_inverted_tile(rider) || rider.motion.velocity_x != 0) {
            drive_velocity(rider, rightward, drive_step, bounce_active);
            charge_throttle(rider, transition, rightward);
        }
        count_small_motion(rider, animation_override, throttle_target);
        if (transition.brake_input)
            hold_brake(rider, transition, charge_announced, brake_drops_throttle);
        else
            release_brake(rider, charge_announced);
    }
    rider.previous_brake = transition.brake_input;
}

std::uint16_t finish_speed_toward_zero(std::uint16_t velocity) {
    return speed_toward_zero(velocity, 10);
}

} // namespace unirally
