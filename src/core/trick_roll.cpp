// The X trick: a z flip, held as a tabletop, and the head bounce it can charge.
//
// Pressing X in the air starts a z flip: the rider spins through a nine-pose strip, one
// step per active update, from its end toward zero, and reaching zero completes a z flip
// (counted, and announced on landing as events 18-21). Letting X go past the strip's
// middle holds the middle pose: a tabletop, whose held updates count (three or more
// announce "tabletop") and grow its reward weight. Pressing X again after a hold spins out
// to the strip's end instead, which ends the trick uncounted. With X still held on the
// last pose, the player charges a head bounce ($82:9398-9714, R-0035).

#include "trick_roll.hpp"

#include "announcements.hpp"
#include "reward_queue.hpp"
#include "word_arithmetic.hpp"
#include "zoom_zoo_movement.hpp"

#include <algorithm>
#include <cstdint>
#include <stdexcept>

namespace unirally {
namespace {

// The spin's signed step runs through the nine poses of the strip: -9 to -1 one way, 1 to
// 9 the other; -9 and 8 are its ends, -5 and 4 the tabletop in its middle.
constexpr std::uint16_t strip_poses = 9;
constexpr std::uint16_t first_step_back = static_cast<std::uint16_t>(-9), last_step = 8;
constexpr std::uint16_t tabletop_step = 4, tabletop_step_back = static_cast<std::uint16_t>(-5);
// The roll pose table's words: bit 15 mirrors the rider, the low 14 bits are the pose.
constexpr std::uint16_t mirror_flag = 0x8000, pose_bits = 0x3fff;
// The spin's poses start at 0x9A0; 0x9A8 is the last, where a bounce can charge.
constexpr std::uint16_t spin_poses = 0x9a0, bounce_charging_pose = 0x9a8;
// A bounce charges to 160 and jumps at most 256 high ($82:965E-96C3).
constexpr std::uint16_t bounce_charge = 160, highest_bounce = 256;
// A tabletop's learned weight grows with the updates held, up to 12. It is event 17's
// weight, index 15 of the bank for events 2-26 ($7E:20F8, $7E:2112).
constexpr unsigned heaviest_tabletop = 12;
constexpr std::size_t tabletop_weight = announcement::tabletop - 2;
constexpr std::uint16_t airborne_updates = 9, off_ground_updates = 2;
constexpr unsigned orientation_mask = 63, orientations = 64, half_turn = 32;

std::uint16_t strip_index(std::uint16_t step) {
    return negative(step) ? add_word(step, strip_poses) : step;
}

// $82:96C3-9711: an ordinary spin or a bounce's release completes: a z flip is counted
// unless a bounce, a leading-support landing or a landing interrupted it; the rider turns
// half a turn and faces the other way unless the pose base says it already does.
void complete_z_flip(ZoomZooState& state, unsigned index, bool interrupted = false) {
    auto& roll = state.rolls[index];
    auto& rider = state.movement.riders[index];
    if (!roll.bounce_active && !state.surface[index].leading_support && !interrupted)
        roll.completed_rolls = add_word(roll.completed_rolls, 1);
    roll.bounce_charge = 0;
    state.reflection[index].pose_override = 0;
    if (!(roll.pose_base & mirror_flag)) rider.pose.reflected = !rider.pose.reflected;
    rider.pose.orientation =
        static_cast<std::uint16_t>(rider.pose.orientation - half_turn) & orientation_mask;
}

// $82:965E-96C3: a charged bounce jumps (the charge is a retained 160, not a ramp: the
// original's below-512 store writes the same value) when the player lets X go on the
// leading support, or when the rider touches down; touching down also completes the spin.
void release_bounce(ZoomZooState& state, unsigned index, bool pressed) {
    auto& roll = state.rolls[index];
    auto& rider = state.movement.riders[index];
    const auto bounce = [&] {
        rider.motion.velocity_y = static_cast<std::uint16_t>(
            ~std::min<std::uint16_t>(roll.bounce_charge, highest_bounce));
        roll.bounce_active = 1;
    };
    if (index == 0 && pressed && !state.surface[index].tile_pose_enabled) {
        roll.input_latched = 0;
        if (state.surface[index].leading_support) bounce();
        return;
    }
    if (rider.contact.unsupported_count < off_ground_updates) bounce();
    complete_z_flip(state, index);
}

// A new press of X in the air, with no pose override, starts a spin from the rider's
// orientation: the roll pose table gives its pose base and whether it spins the other
// way. Returns false when no spin runs.
bool start_z_flip(ZoomZooState& state, unsigned index, bool pressed,
                  const ZoomZooContent& content) {
    auto& roll = state.rolls[index];
    auto& rider = state.movement.riders[index];
    if (state.reflection[index].pose_override || rider.contact.unsupported_count < airborne_updates)
        return false;
    if (!pressed) {
        roll.input_latched = 0;
        return false;
    }
    if (roll.input_latched) return false;
    roll.input_latched = 1;
    roll.prior_orientation = rider.pose.orientation;
    roll.prior_reflection = rider.pose.reflected;
    const auto entry =
        content_word(content.roll_poses, 2U * (rider.pose.orientation & orientation_mask));
    roll.step = first_step_back;
    if (entry & mirror_flag) {
        roll.step = strip_poses;
        rider.pose.reflected = !rider.pose.reflected;
    }
    roll.pose_base = static_cast<std::uint16_t>(entry & pose_bits);
    return true;
}

// The roll direction table says which way the spin runs at each orientation.
void follow_spin_direction(ZoomZooRoll& roll, const RiderMovementState& rider,
                           const ZoomZooContent& content) {
    const auto orientation = rider.pose.orientation & orientation_mask;
    if (orientation >= content.roll_directions.size())
        throw std::invalid_argument("ZOOM ZOO roll direction table is missing");
    const bool forward = content.roll_directions[orientation] == 1;
    if (forward == negative(roll.step)) roll.step = static_cast<std::uint16_t>(~roll.step);
}

// Touching down during the spin, unless a hold has already ended, ends the hold: the
// tabletop weight resets, any turn counted is a wipeout, and the trick counts start again.
// Returns true when the spin was interrupted.
bool land_during_spin(ZoomZooState& state, unsigned index) {
    auto& roll = state.rolls[index];
    auto& rider = state.movement.riders[index];
    auto& turn = state.reflection[index];
    if (rider.contact.unsupported_count >= off_ground_updates || negative(roll.held_updates))
        return false;
    roll.held_updates = static_cast<std::uint16_t>(-roll.held_updates);
    state.learned_weights[index][tabletop_weight] = 0;
    auto& quarters = rider.quarter_turn;
    if (quarters.forward_turns + quarters.reverse_turns + roll.held_rotations + turn.air_turns) {
        if (index == 0)
            queue_player_announcement(state, announcement::wipeout);
        else
            queue_opponent_announcement(state.movement, announcement::wipeout);
    }
    quarters.previous_quadrant = quarters.forward_turns = quarters.reverse_turns = 0;
    quarters.forward_quarters = quarters.reverse_quarters = 0;
    turn.air_turns = 0;
    roll.completed_rolls = roll.held_rotations = 0;
    return true;
}

enum class Spin { stepped, ended_at_strip_end };

// One step of the spin: after a hold, with X pressed again (or once the hold has ended),
// out toward the strip's end, where the trick ends uncounted; with X released past the
// middle, to the middle pose and held there, counting the hold ($82:955F-9598); otherwise
// toward zero ($82:959B-95BD; a step the direction flip has just made zero, ~-1 at
// $82:9439, completes without moving, DRAGSTER diff fuzz 53).
Spin step_spin(ZoomZooState& state, unsigned index, bool pressed, std::uint16_t& step_index) {
    auto& roll = state.rolls[index];
    if (negative(roll.held_updates) || (pressed && roll.held_updates)) {
        if (!negative(roll.held_updates)) {
            roll.held_updates = static_cast<std::uint16_t>(-roll.held_updates);
            auto& weight = state.learned_weights[index][tabletop_weight];
            weight = static_cast<std::uint8_t>(std::min(
                heaviest_tabletop,
                unsigned(weight) + unsigned(static_cast<std::uint8_t>(-roll.held_updates))));
        }
        if ((negative(roll.step) && roll.step == first_step_back)
            || (!negative(roll.step) && roll.step == last_step)) {
            state.movement.riders[index].pose.reflected = roll.prior_reflection != 0;
            roll.held_updates = 0;
            state.reflection[index].pose_override = 0;
            roll.step = 0;
            return Spin::ended_at_strip_end;
        }
        roll.step = add_word(roll.step, negative(roll.step) ? static_cast<std::uint16_t>(-1) : 1);
        step_index = strip_index(roll.step);
    } else if (!pressed
               && ((!negative(roll.step) && roll.step >= tabletop_step)
                   || (negative(roll.step)
                       && static_cast<std::int16_t>(roll.step)
                              <= static_cast<std::int16_t>(tabletop_step_back)))) {
        if (!negative(roll.step) && roll.step != tabletop_step) --roll.step;
        if (negative(roll.step) && roll.step != tabletop_step_back) ++roll.step;
        roll.held_updates = add_word(roll.held_updates, 1);
        roll.held_rotations = add_word(roll.held_rotations, 1);
        step_index = strip_index(roll.step);
    } else if (roll.step) {
        roll.step = add_word(roll.step, negative(roll.step) ? 1 : static_cast<std::uint16_t>(-1));
        step_index = strip_index(roll.step);
    }
    return Spin::stepped;
}

// The spin's pose: the roll pose table's entry for the rider's orientation (mirrored when
// it faces left) plus the strip's index, over the spin poses.
void show_spin_pose(ZoomZooState& state, unsigned index, std::uint16_t step_index,
                    const ZoomZooContent& content) {
    auto& roll = state.rolls[index];
    auto& rider = state.movement.riders[index];
    auto angle = rider.pose.orientation & orientation_mask;
    auto entry = content_word(content.roll_poses, 2U * angle);
    if (entry & mirror_flag) {
        rider.pose.reflected = !roll.prior_reflection;
        roll.pose_base |= mirror_flag;
    } else {
        rider.pose.reflected = roll.prior_reflection != 0;
        roll.pose_base &= static_cast<std::uint16_t>(~mirror_flag);
    }
    if (rider.pose.reflected) angle = (orientations - angle) & orientation_mask;
    entry = content_word(content.roll_poses, 2U * angle);
    if (entry & mirror_flag) entry = content_word(content.roll_poses, 2U * (orientations - angle));
    state.reflection[index].pose_override =
        static_cast<std::uint16_t>((entry & pose_bits) + step_index + spin_poses);
}

} // namespace

// $82:9398-9714: the X trick for one update. The step advances only on this rider's
// active update; the pose and facing feed the next collision sample.
void update_z_flip(ZoomZooState& state, unsigned index, bool pressed,
                   const ZoomZooContent& content) {
    auto& roll = state.rolls[index];
    if (roll.bounce_charge) {
        release_bounce(state, index, pressed);
        return;
    }
    if (!roll.step && !start_z_flip(state, index, pressed, content)) return;
    follow_spin_direction(roll, state.movement.riders[index], content);
    const bool interrupted = land_during_spin(state, index);
    std::uint16_t step_index{};
    if (step_spin(state, index, pressed, step_index) == Spin::ended_at_strip_end) return;
    if (!roll.step) {
        // $82:9636-965B keeps the last pose while the player's X charges a bounce.
        if (state.reflection[index].pose_override == bounce_charging_pose && index == 0 && pressed
            && !state.surface[index].tile_pose_enabled) {
            roll.bounce_charge = bounce_charge;
            return;
        }
        complete_z_flip(state, index, interrupted);
        return;
    }
    show_spin_pose(state, index, step_index, content);
}

} // namespace unirally
