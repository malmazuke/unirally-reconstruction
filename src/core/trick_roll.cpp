// Rolls: the rider's roll, bounce and their trick rewards.

#include "trick_roll.hpp"

#include "reward_queue.hpp"
#include "word_arithmetic.hpp"
#include "zoom_zoo_movement.hpp"

#include <algorithm>
#include <cstdint>
#include <stdexcept>

namespace unirally {

namespace {

// $8296C3-9711: completion is shared by ordinary rolls and bounce release.
void complete_zoom_roll(ZoomZooState& state, unsigned index, bool interrupted = false) {
    auto& roll = state.rolls[index];
    auto& rider = state.movement.riders[index];
    if (!roll.bounce_active && !state.surface[index].leading_support && !interrupted)
        roll.completed_rolls = add_word(roll.completed_rolls, 1);
    roll.bounce_charge = 0;
    state.reflection[index].pose_override = 0;
    if (!(roll.pose_base & 0x8000U)) rider.pose.reflected = !rider.pose.reflected;
    rider.pose.orientation = static_cast<std::uint16_t>(rider.pose.orientation - 32U) & 63U;
}

} // namespace

// $829398-9714: X enters a roll on unsupported contact. The signed
// step selects the original nine-frame pose strip and is advanced only on
// this rider's active update. Pose/reflection feed the next collision sample.
void update_zoom_roll(ZoomZooState& state, unsigned index, bool pressed,
                      const ZoomZooContent& content) {
    auto& roll = state.rolls[index];
    auto& rider = state.movement.riders[index];
    auto& turn = state.reflection[index];
    if (roll.bounce_charge) {
        // $82965E-96C3. Charge is a retained160, not a per-update ramp:
        // the original's below512 store writes the same loaded value.
        const auto bounce = [&] {
            rider.motion.velocity_y =
                static_cast<std::uint16_t>(~std::min<std::uint16_t>(roll.bounce_charge, 256));
            roll.bounce_active = 1;
        };
        if (index == 0 && pressed && !state.surface[index].tile_pose_enabled) {
            roll.input_latched = 0;
            if (state.surface[index].leading_support) bounce();
            return;
        }
        if (rider.contact.unsupported_count < 2) bounce();
        complete_zoom_roll(state, index);
        return;
    }
    if (!roll.step) {
        if (turn.pose_override || rider.contact.unsupported_count < 9) return;
        if (!pressed) {
            roll.input_latched = 0;
            return;
        }
        if (roll.input_latched) return;
        roll.input_latched = 1;
        roll.prior_orientation = rider.pose.orientation;
        roll.prior_reflection = rider.pose.reflected;
        const auto entry = content_word(content.roll_poses, 2U * (rider.pose.orientation & 63U));
        roll.step = static_cast<std::uint16_t>(-9);
        if (entry & 0x8000U) {
            roll.step = 9;
            rider.pose.reflected = !rider.pose.reflected;
        }
        roll.pose_base = static_cast<std::uint16_t>(entry & 0x3fffU);
    }
    const auto orientation = rider.pose.orientation & 63U;
    if (orientation >= content.roll_directions.size())
        throw std::invalid_argument("ZOOM ZOO roll direction table is missing");
    if ((content.roll_directions[orientation] == 1 && negative(roll.step))
        || (content.roll_directions[orientation] != 1 && !negative(roll.step)))
        roll.step = static_cast<std::uint16_t>(~roll.step);
    bool interrupted = false;
    auto& held_weight = state.learned_weights[index][15]; // Event17, $7E20F8/$7E2112.
    if (rider.contact.unsupported_count < 2 && !negative(roll.held_updates)) {
        roll.held_updates = static_cast<std::uint16_t>(-roll.held_updates);
        held_weight = 0;
        auto& quarters = rider.quarter_turn;
        if (quarters.forward_turns + quarters.reverse_turns + roll.held_rotations
            + turn.air_turns) {
            if (index == 0)
                enqueue_zoom_player(state, 14);
            else
                enqueue_zoom_opponent(state.movement, 14);
        }
        quarters.previous_quadrant = quarters.forward_turns = quarters.reverse_turns = 0;
        quarters.forward_quarters = quarters.reverse_quarters = 0;
        turn.air_turns = 0;
        roll.completed_rolls = roll.held_rotations = 0;
        interrupted = true;
    }
    std::uint16_t step_index{};
    if (negative(roll.held_updates) || (pressed && roll.held_updates)) {
        if (!negative(roll.held_updates)) {
            roll.held_updates = static_cast<std::uint16_t>(-roll.held_updates);
            held_weight = static_cast<std::uint8_t>(
                std::min(12U, unsigned(held_weight)
                                  + unsigned(static_cast<std::uint8_t>(-roll.held_updates))));
        }
        if ((negative(roll.step) && roll.step == static_cast<std::uint16_t>(-9))
            || (!negative(roll.step) && roll.step == 8)) {
            rider.pose.reflected = roll.prior_reflection != 0;
            roll.held_updates = 0;
            turn.pose_override = 0;
            roll.step = 0;
            return;
        }
        roll.step = add_word(roll.step, negative(roll.step) ? static_cast<std::uint16_t>(-1) : 1);
        step_index = negative(roll.step) ? add_word(roll.step, 9) : roll.step;
    } else if (!pressed
               && ((!negative(roll.step) && roll.step >= 4)
                   || (negative(roll.step) && static_cast<std::int16_t>(roll.step) <= -5))) {
        // $82955F-9598: release moves toward the central held pose.
        if (!negative(roll.step) && roll.step != 4) --roll.step;
        if (negative(roll.step) && roll.step != static_cast<std::uint16_t>(-5)) ++roll.step;
        roll.held_updates = add_word(roll.held_updates, 1);
        roll.held_rotations = add_word(roll.held_rotations, 1);
        step_index = negative(roll.step) ? add_word(roll.step, 9) : roll.step;
    } else if (roll.step) {
        // $82:959B-95BD. A step the direction flip has just made zero
        // (~-1, $82:9439) completes without moving (DRAGSTER diff fuzz 53).
        roll.step = add_word(roll.step, negative(roll.step) ? 1 : static_cast<std::uint16_t>(-1));
        step_index = negative(roll.step) ? add_word(roll.step, 9) : roll.step;
    }
    if (!roll.step) {
        // $829636-965B keeps the final central pose while X charges a bounce.
        if (turn.pose_override == 0x9a8 && index == 0 && pressed
            && !state.surface[index].tile_pose_enabled) {
            roll.bounce_charge = 160;
            return;
        }
        complete_zoom_roll(state, index, interrupted);
        return;
    }
    auto angle = rider.pose.orientation & 63U;
    auto entry = content_word(content.roll_poses, 2U * angle);
    if (entry & 0x8000U) {
        rider.pose.reflected = !roll.prior_reflection;
        roll.pose_base |= 0x8000U;
    } else {
        rider.pose.reflected = roll.prior_reflection != 0;
        roll.pose_base &= 0x7fffU;
    }
    if (rider.pose.reflected) angle = (64U - angle) & 63U;
    entry = content_word(content.roll_poses, 2U * angle);
    if (entry & 0x8000U) entry = content_word(content.roll_poses, 2U * (64U - angle));
    turn.pose_override = static_cast<std::uint16_t>((entry & 0x3fffU) + step_index + 0x9a0U);
}

} // namespace unirally
