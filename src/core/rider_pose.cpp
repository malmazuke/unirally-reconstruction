// A rider's pose: the idle wobble, the pose and animation update, rolling and quarter turns.
//
// Orientation is 0-63, a whole turn. The rider leans toward a target orientation taken
// from the slope under it and its speed; an idle rider wobbles about it; the shown
// orientation adds the wobble and the contact impulse and mirrors when the rider faces
// left. The pose index is the animation frame (24 phases of the wheel, or a rolling
// frame) times 64 plus that orientation (R-0011, R-0038).

#include "rider_pose.hpp"

#include "announcements.hpp"
#include "word_arithmetic.hpp"
#include "zoom_zoo_movement.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>

namespace unirally {
namespace {

constexpr unsigned orientations = 64, orientation_mask = 63;
// Past these a rider is off the ground (2 updates) or fully airborne (9).
constexpr std::uint16_t off_ground_updates = 2, airborne_updates = 9;
// A wall: the surface angle +-31, also the steepest target slope.
constexpr int steepest_slope = 31;
// The pose tables: 64 slope targets for each of two tile kinds; the displacement squares.
constexpr std::size_t pose_slope_table_size = 128, displacement_table_size = 512;
constexpr unsigned slope_targets = 64;
constexpr std::uint8_t high_tile = 0x80; // the selected tile's high flag: a rolling surface
// A rolling rider leans 23 into its roll.
constexpr int rolling_lean = 23;
// Fast (an x displacement of 16 or more) the orientation turns up to three steps a update.
constexpr std::int16_t fast_displacement = 16;
constexpr unsigned extra_turn_steps = 2;
// The wheel's animation: 24 phases, 64 poses each. A rolling rider shows one of three
// rolling frames from 0x820 (rolling level 2) or 0x8E0 (level 1).
constexpr int wheel_phases = 24, poses_per_frame = 64, rolling_frames = 3;
constexpr int rolling_level_one_poses = 0x8e0, rolling_level_two_poses = 0x820;
// Rolling slows below an animation rate of 3 and speeds up from 5, between levels 0 and 2.
constexpr int slow_roll_rate = 3, fast_roll_rate = 5;
constexpr std::uint8_t fastest_rolling_level = 2;
// The idle wobble ($82:A0B7-$82:A236): a 64-entry signed table, a cycle counted to 120
// and latched at 60, velocities within -16 to 16, and a wobble cleared beyond +-512.
constexpr std::size_t idle_table_size = 64;
constexpr std::int16_t idle_cycle_limit = 120, idle_cycle_latch = 60;
constexpr int idle_fastest = 16, wobble_limit = 512;
// The idle pose stops at pose index 0x0AEC and above (the special poses).
constexpr std::int16_t first_special_pose = 0x0aec;
// The table drives the wobble near upright (orientations under 9 or from 58) and on the
// side the rider leans to (9-31 rising, 32-57 falling).
constexpr std::int16_t upright_end = 9, half_turn = 32, upright_start = 58;

void clear_idle_cycle(IdlePoseState& idle) {
    // The reset path intentionally preserves wobble_offset and
    // orientation_reference ($0F37/$0F83).
    idle.bias = idle.velocity = idle.previous_bias = idle.direction_adjustment = 0;
    idle.active = idle.cycle_latched = idle.cycle_counter = 0;
}

std::uint16_t mirrored(std::uint16_t orientation) {
    return static_cast<std::uint16_t>(orientations - orientation);
}

// The bias holds the wobble's direction for a while: it counts down while the rider is
// upright and ends at once when it is negative.
void update_idle_bias(IdlePoseState& idle, std::int16_t reference) {
    const auto bias = static_cast<std::int16_t>(idle.bias);
    if (bias != 0 && reference != 0) {
        idle.direction_adjustment = 1;
    } else if (bias > 0) {
        idle.bias = static_cast<std::uint16_t>(bias - 1);
        idle.direction_adjustment = 1;
    } else {
        idle.bias = 0;
        idle.direction_adjustment = 0;
    }
}

// The wobble's velocity rises while the rider leans back (or, upright, on the counter's
// rising half) and falls otherwise, one step faster while the bias holds. Returns the
// candidate velocity, kept even when it leaves the range.
int update_idle_velocity(IdlePoseState& idle, std::int16_t reference, bool opponent,
                         std::uint8_t animation_counter) {
    const auto velocity = static_cast<std::int16_t>(idle.velocity);
    const auto adjustment = static_cast<std::int16_t>(idle.direction_adjustment);
    // $0FF9 selects the counter pair at $04C7/$04C9. The opponent word is the modulo-32
    // complement used by the original's second rider pass.
    const auto rider_counter =
        opponent ? static_cast<std::uint8_t>((32U - animation_counter) & 31U) : animation_counter;
    const bool increase = reference == 0 ? (rider_counter & 0x10U) != 0 : reference >= half_turn;
    if (increase) {
        const int candidate = velocity + 1 + adjustment;
        if (candidate <= idle_fastest) idle.velocity = static_cast<std::uint16_t>(candidate);
        return candidate;
    }
    const int candidate = velocity - 1 - adjustment;
    if (candidate >= -idle_fastest) idle.velocity = static_cast<std::uint16_t>(candidate);
    return candidate;
}

// On counter 0 the candidate becomes the bias (unless it repeats), and a long enough cycle
// latches.
void close_idle_cycle(IdlePoseState& idle, int candidate) {
    idle.bias = static_cast<std::uint16_t>(candidate);
    if (static_cast<std::int16_t>(idle.cycle_counter) >= idle_cycle_latch
        && idle.cycle_latched == 0) {
        idle.cycle_latched = 1;
        idle.cycle_counter = 0;
    }
    if (idle.previous_bias == idle.bias) idle.bias = 0;
    idle.previous_bias = idle.bias;
}

// The wobble moves by the table near upright and on the leaning side, else by the
// velocity. A stopped wobble off upright restarts at +-1.
void apply_idle_wobble(IdlePoseState& idle, std::int16_t reference,
                       std::span<const std::uint8_t> table) {
    const auto velocity = static_cast<std::int16_t>(idle.velocity);
    bool use_table = false;
    if (velocity == 0) {
        if (reference < upright_end || reference >= upright_start)
            use_table = true;
        else
            idle.velocity = static_cast<std::uint16_t>(reference < half_turn ? -1 : 1);
    } else if (velocity < 0) {
        // $82:A1F2-A1FF: a falling velocity below 32 is applied unchanged. An earlier reset
        // to -1 for 9-31 had no source; DRAGSTER diff fuzz seed 66 reaches it at 1977
        // (R-0038).
        use_table = reference >= half_turn && reference < upright_start;
    } else {
        use_table = reference >= upright_end && reference < half_turn;
    }
    const auto delta = use_table
                         ? static_cast<std::int8_t>(table[static_cast<std::size_t>(reference)])
                         : static_cast<std::int16_t>(idle.velocity);
    idle.wobble_offset = add_word(idle.wobble_offset, static_cast<std::uint16_t>(delta));
}

// The target orientation: the slope table's entry for the surface angle, leaned into a
// roll, or for the speed. $83:EF97-EFB2: the speed is the throttle after a
// small-displacement drive, else mud's braked velocity ($0F3F), else velocity x, shifted
// by five; on flag pair 8 ($0F2D, $83:EFA1) velocity x takes one shift, not five.
void update_target_orientation(RiderMovementState& rider, const MovementContent& content,
                               bool use_throttle_target, int surface_angle_offset,
                               std::uint16_t mud_velocity, bool slow_tile) {
    const bool high = (rider.contact.selected_high & high_tile) != 0;
    int target{};
    if (rider.pose.rolling && rider.pose.rolling_level != 0) {
        target = static_cast<std::int16_t>(rider.contact.surface_angle)
               + (rider.pose.reflected ? rolling_lean : -rolling_lean);
    } else {
        const auto speed = static_cast<std::int16_t>(use_throttle_target ? rider.throttle
                                                     : mud_velocity      ? mud_velocity
                                                                         : rider.motion.velocity_x);
        target = speed >> ((slow_tile && !use_throttle_target) ? 1 : 5);
    }
    if (high) target = 0;
    target = std::clamp(target + surface_angle_offset, -steepest_slope, steepest_slope);
    const unsigned index =
        target >= 0 ? static_cast<unsigned>(target) : static_cast<unsigned>(-target + half_turn);
    rider.pose.target_orientation = content.pose_slopes[index + (high ? slope_targets : 0U)];
}

// The orientation takes the contact's rotation, or turns one step toward the target, up
// to three when fast.
void turn_toward_target(RiderMovementState& rider) {
    auto& pose = rider.pose;
    auto orientation = static_cast<std::uint16_t>(pose.orientation + rider.motion.response_b);
    if (rider.motion.response_a) {
        orientation = add_word(orientation, rider.motion.response_a);
    } else if (rider.contact.unsupported_count < airborne_updates
               && pose.target_orientation == pose.orientation) {
        // $83:F02D-F034 skips the store: a supported rider already at its target keeps
        // its orientation, so rotation input is not applied (DRAGSTER diff fuzz seed 135,
        // the update after an L-held landing).
        orientation = pose.orientation;
    } else if (rider.contact.unsupported_count < airborne_updates) {
        const auto target = pose.target_orientation;
        const auto ahead = static_cast<std::int16_t>(target - pose.orientation);
        const auto behind = static_cast<std::int16_t>(pose.orientation - target);
        const bool increase = target >= half_turn ? (ahead >= 1 && ahead <= half_turn)
                                                  : !(behind >= 1 && behind <= half_turn);
        const int direction = increase ? 1 : -1;
        orientation = static_cast<std::uint16_t>(pose.orientation + direction);
        if (static_cast<std::int16_t>(rider.motion.previous_x_displacement) >= fast_displacement) {
            for (unsigned step = 0;
                 step < extra_turn_steps && (orientation & orientation_mask) != target; ++step)
                orientation = static_cast<std::uint16_t>(
                    static_cast<int>(orientation & orientation_mask) + direction);
        }
    }
    pose.orientation = orientation & orientation_mask;
}

// $83:F09E-$83:F0B9: the shown orientation adds the idle wobble (a signed division by 16,
// truncated toward zero) and half the contact impulse, which decays every other update,
// and mirrors when the rider faces left.
void update_shown_orientation(RiderMovementState& rider, std::uint8_t counter) {
    const int wobble = static_cast<std::int16_t>(rider.idle_pose.wobble_offset) / 16;
    const int combined = static_cast<int>(rider.pose.orientation) + wobble
                       + (static_cast<std::int16_t>(rider.motion.orientation_impulse) >> 1);
    rider.pose.reflected_orientation = static_cast<std::uint16_t>(combined) & orientation_mask;
    if ((counter & 1U) == 0 && rider.motion.orientation_impulse) {
        const auto impulse = static_cast<std::int16_t>(rider.motion.orientation_impulse);
        rider.motion.orientation_impulse =
            static_cast<std::uint16_t>(impulse + (impulse >= 0 ? -1 : 1));
    }
    if (rider.pose.reflected_orientation && rider.pose.reflected)
        rider.pose.reflected_orientation = mirrored(rider.pose.reflected_orientation);
}

// How far the wheel turns this update. In a jump or with no contact angle the last rate
// decays every fourth update. On the ground it is the distance moved (the square root of
// the table's squares, signed by x), a third of it carried with its remainder.
int animation_increment(RiderMovementState& rider, std::uint8_t counter,
                        const MovementContent& content) {
    auto& pose = rider.pose;
    if (rider.jump.impulse_phase >= 1 || rider.contact.angle_unspecified) {
        int steering = static_cast<std::int16_t>(pose.animation_increment);
        if ((counter & 3U) == 3) steering += steering > 0 ? -1 : (steering < 0 ? 1 : 0);
        pose.animation_increment = static_cast<std::uint16_t>(steering);
        return steering;
    }
    const int dx = static_cast<std::int16_t>(pose.previous_x - rider.motion.x);
    int dy = static_cast<std::int16_t>(pose.previous_y - rider.motion.y);
    if (std::abs(dy) < 3) dy = 0;
    const auto square_x = content_word(content.displacement_table, 2U * (std::abs(dx) & 255));
    const auto square_y = content_word(content.displacement_table, 2U * (std::abs(dy) & 255));
    int distance = static_cast<int>(integer_sqrt((square_x + square_y) & 65535U));
    if (dx < 0) distance = -distance;
    rider.motion.previous_x_displacement = static_cast<std::uint16_t>(distance);
    const int accumulation = static_cast<std::int16_t>(pose.displacement_remainder + distance);
    int steering = distance;
    int remainder{};
    if (accumulation) {
        steering = std::abs(accumulation) / 3;
        remainder = std::abs(accumulation) % 3;
        if (accumulation < 0) steering = -steering;
        if (dx < 0) remainder = -remainder;
    }
    pose.displacement_remainder = static_cast<std::uint16_t>(remainder);
    pose.animation_increment = static_cast<std::uint16_t>(steering);
    return steering;
}

// A rolling rider's level follows its animation rate; above level 0 at a rate of 3 or
// more it shows a rolling frame. $83:EED2-EEDB returns to the ordinary pose after the
// low-rate decrement even when the level stays nonzero. Returns the rolling animation,
// or `animation` unchanged.
int rolling_animation(PoseState& pose, int phase, int animation, std::uint8_t contact_phase) {
    const auto rate = std::abs(static_cast<std::int16_t>(pose.animation_increment));
    const auto level = static_cast<std::uint8_t>(pose.rolling_level);
    if (rate < slow_roll_rate && level != 0)
        pose.rolling_level = static_cast<std::uint16_t>(level - 1U);
    else if (rate >= fast_roll_rate && level != fastest_rolling_level)
        pose.rolling_level = static_cast<std::uint16_t>(level + 1U);
    const auto now = static_cast<std::uint8_t>(pose.rolling_level);
    if (rate < slow_roll_rate || now == 0) return animation;
    int alternate =
        static_cast<std::uint16_t>(pose.alternate_animation_phase) + (phase & 1) + contact_phase;
    if (alternate >= rolling_frames) alternate -= rolling_frames;
    pose.alternate_animation_phase = static_cast<std::uint16_t>(alternate);
    return alternate * poses_per_frame
         + (now == 1 ? rolling_level_one_poses : rolling_level_two_poses);
}

} // namespace

void decay_idle_wobble(RiderMovementState& rider, bool surface_mode) {
    // $81:8625-$81:8672 preserves an offset produced by the preceding idle update for one
    // frame. Otherwise it approaches zero by five, or by two while the contact response
    // word is nonzero, and then clears the marker.
    auto& idle = rider.idle_pose;
    if (idle.active == 0) {
        auto offset = static_cast<std::int16_t>(idle.wobble_offset);
        if (offset >= wobble_limit || offset < -wobble_limit) {
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

// $82:A0B7-$82:A236: a rider standing still on the ground in a running race wobbles. The
// wobble offset adds to the shown orientation in update_pose.
void update_idle_pose(RiderMovementState& rider, bool race_active, bool opponent,
                      std::uint8_t animation_counter, std::span<const std::uint8_t> table) {
    if (table.size() != idle_table_size)
        throw std::invalid_argument("idle pose table has the wrong size");
    auto& idle = rider.idle_pose;
    if (!race_active
        || static_cast<std::int16_t>(rider.motion.previous_x_displacement) >= off_ground_updates
        || static_cast<std::int16_t>(rider.contact.unsupported_count) >= off_ground_updates) {
        clear_idle_cycle(idle);
        return;
    }
    if (idle.cycle_latched == 0) {
        const auto next = add_word(idle.cycle_counter, 1);
        if (static_cast<std::int16_t>(next) < idle_cycle_limit) idle.cycle_counter = next;
    }
    idle.orientation_reference = rider.pose.reflected_orientation;
    if (idle.orientation_reference != 0 && rider.pose.reflected)
        idle.orientation_reference = mirrored(idle.orientation_reference);
    if (static_cast<std::int16_t>(rider.pose.pose_index) >= first_special_pose) {
        clear_idle_cycle(idle);
        return;
    }
    idle.active = 1;
    const auto reference = static_cast<std::int16_t>(idle.orientation_reference);
    update_idle_bias(idle, reference);
    const int candidate = update_idle_velocity(idle, reference, opponent, animation_counter);
    if (animation_counter == 0) close_idle_cycle(idle, candidate);
    apply_idle_wobble(idle, reference, table);
}

void update_rolling_mode(RiderMovementState& rider, bool surface_mode) {
    if (rider.contact.selected_high & high_tile) {
        rider.pose.rolling = true;
        return;
    }
    if (surface_mode) {
        rider.pose.rolling = false;
        return;
    }
    const auto displacement = static_cast<std::int16_t>(rider.motion.previous_x_displacement);
    if (rider.contact.unsupported_count == airborne_updates
        || rider.pose.reflected_orientation < 45) {
        rider.pose.rolling = false;
    } else if (rider.pose.rolling && displacement < 14) {
        rider.pose.rolling = false;
    } else if (!rider.pose.rolling && displacement < fast_displacement) {
        return;
    } else {
        const bool moving_nonnegative = static_cast<std::int16_t>(rider.motion.velocity_x) >= 0;
        rider.pose.rolling = rider.pose.reflected ? moving_nonnegative : !moving_nonnegative;
    }
}

// One update of the pose: the target, the turn toward it, the shown orientation, and the
// wheel's animation frame.
void update_pose(RiderMovementState& rider, std::uint8_t counter, std::uint8_t contact_phase,
                 const MovementContent& content, int animation_override, bool use_throttle_target,
                 int surface_angle_offset, std::uint16_t mud_velocity, bool slow_tile) {
    if (content.pose_slopes.size() != pose_slope_table_size
        || content.displacement_table.size() != displacement_table_size)
        throw std::invalid_argument("movement pose tables have the wrong size");
    auto& pose = rider.pose;
    if (!rider.contact.angle_unspecified)
        update_target_orientation(rider, content, use_throttle_target, surface_angle_offset,
                                  mud_velocity, slow_tile);
    turn_toward_target(rider);
    update_shown_orientation(rider, counter);
    pose.displacement_history[2] = pose.displacement_history[1];
    pose.displacement_history[1] = pose.displacement_history[0];
    pose.displacement_history[0] = rider.motion.previous_x_displacement;
    int steering = animation_increment(rider, counter, content);
    if (pose.reflected) steering = -steering;
    if (animation_override) steering = animation_override;
    int phase = (static_cast<std::int16_t>(pose.animation_phase) + steering) % wheel_phases;
    if (phase < 0) phase += wheel_phases;
    pose.animation_phase = static_cast<std::uint16_t>(phase);
    int animation = phase * poses_per_frame;
    if (pose.rolling) animation = rolling_animation(pose, phase, animation, contact_phase);
    pose.pose_index = static_cast<std::uint16_t>(animation + pose.reflected_orientation);
    rider.motion.previous_x_displacement = static_cast<std::uint16_t>(
        std::abs(static_cast<std::int16_t>(rider.motion.previous_x_displacement)));
    pose.previous_x = rider.motion.x;
    pose.previous_y = rider.motion.y;
}

// A rider held still on the ground, or pushing in a direction, animates two phases a
// update that way; a rider that has just moved does not.
int stationary_animation_override(const RiderMovementState& rider, std::uint8_t horizontal) {
    const bool prior_motion =
        std::any_of(rider.pose.displacement_history.begin(), rider.pose.displacement_history.end(),
                    [](auto value) { return static_cast<std::int16_t>(value) >= 2; })
        || static_cast<std::int16_t>(rider.motion.previous_x_displacement) >= 2;
    if ((rider.contact.unsupported_count < off_ground_updates && prior_motion)
        || horizontal == direction::neutral)
        return 0;
    const int value = horizontal < direction::neutral ? 2 : -2;
    return rider.pose.reflected ? -value : value;
}

namespace {

// The orientation's quadrant, 0-3.
std::uint16_t quadrant(int orientation) {
    return static_cast<std::uint16_t>(static_cast<std::uint16_t>(orientation) & orientation_mask)
        >> 4U;
}

// A quadrant change is a quarter turn forward or back; four in a row make a turn.
void count_quarter_turn(QuarterTurnState& turns, std::uint16_t current) {
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

void clear_quarter_turns(QuarterTurnState& turns) {
    turns.previous_quadrant = turns.forward_turns = turns.reverse_turns = 0;
    turns.forward_quarters = turns.reverse_quarters = 0;
    turns.initialized = false;
}

// In the air, or on a wall, the quarter turns count. $82:9ABD-9AF3 re-bases an active
// roll's quadrant and clears only partial quarters, then still compares the current angle
// this update; completed turns and the take-off facing survive.
void count_turns_in_the_air(RiderMovementState& rider, bool on_wall, bool rolling,
                            unsigned held_rotations) {
    auto& turns = rider.quarter_turn;
    const auto rotation = static_cast<std::int8_t>(rider.motion.response_b & 0xffU);
    const int prior = static_cast<int>(rider.pose.orientation) - rotation;
    const bool take_off = !on_wall && rider.contact.unsupported_count == off_ground_updates;
    if (rolling && !held_rotations && !take_off) {
        turns.previous_quadrant = quadrant(prior);
        turns.forward_quarters = turns.reverse_quarters = 0;
    }
    if (take_off || !turns.initialized) {
        turns.previous_quadrant = quadrant(prior);
        turns.reflected_at_start = rider.pose.reflected;
        turns.initialized = true;
        turns.forward_turns = turns.reverse_turns = 0;
        turns.forward_quarters = turns.reverse_quarters = 0;
    } else {
        count_quarter_turn(turns, static_cast<std::uint16_t>(rider.pose.orientation >> 4U));
    }
}

} // namespace

// Quarter turns count while the rider is in the air or on a wall. On landing on the
// leading support any turn is a wipeout (the boosts are lost); a clean landing on the
// legacy DRAGSTER path returns its reward event, where only a single roll (event 1) is
// recovered. Returns the event, or 0.
unsigned update_quarter_turns(RiderMovementState& rider, bool leading_support, unsigned air_turns,
                              bool rolling, unsigned held_rotations) {
    auto& turns = rider.quarter_turn;
    const bool on_wall =
        std::abs(static_cast<std::int16_t>(rider.contact.surface_angle)) == steepest_slope;
    if (on_wall
        || (!rider.motion.response_a && rider.contact.unsupported_count >= off_ground_updates)) {
        count_turns_in_the_air(rider, on_wall, rolling, held_rotations);
        return 0;
    }
    if (leading_support) {
        const bool wipeout = turns.forward_turns + turns.reverse_turns + air_turns != 0;
        if (wipeout) rider.speed.boost = rider.speed.vertical_boost = 0;
        clear_quarter_turns(turns);
        return wipeout ? announcement::wipeout : 0U;
    }
    if (!turns.initialized) return 0;
    if (turns.forward_quarters == 3) turns.forward_turns = add_word(turns.forward_turns, 1);
    if (turns.reverse_quarters == 3) turns.reverse_turns = add_word(turns.reverse_turns, 1);
    const auto forward = std::min<std::uint16_t>(turns.forward_turns, 4);
    const auto reverse = std::min<std::uint16_t>(turns.reverse_turns, 4);
    // Forward is a flip and back a roll, relative to the take-off facing.
    unsigned event{};
    if (forward)
        event = announcement::trick(
            turns.reflected_at_start ? announcement::roll : announcement::flip, forward);
    if (reverse) {
        if (event)
            throw std::invalid_argument("combined rotation reward is outside the recovered domain");
        event = announcement::trick(
            turns.reflected_at_start ? announcement::flip : announcement::roll, reverse);
    }
    clear_quarter_turns(turns);
    if (event == 0 || event == announcement::roll) return event;
    throw std::invalid_argument("rotation reward is outside the recovered event-one domain");
}

} // namespace unirally
