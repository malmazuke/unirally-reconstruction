// The opponent's controller: the AI's inputs and its throttle.

#include "opponent_ai.hpp"

#include "word_arithmetic.hpp"
#include "zoom_zoo_movement.hpp"

#include <algorithm>
#include <cstdint>
#include <stdexcept>

namespace unirally {

// Returns true when an inverted marker ended the update early (R-0048).
bool update_zoom_ai(ZoomZooState& state) {
    auto& whole = state.movement;
    auto& input = state.reflection[1];
    auto& rider = whole.riders[1];
    auto& ai = whole.opponent_ai;
    input.brake_input = input.jump_input = input.rotate_negative_input =
        input.rotate_positive_input = 0;
    const auto marker = rider.progress.marker_word;
    // $83:E0A7-E0AF: a marker with bit 15 set returns before the AI sets any
    // input, so the opponent keeps what the port-2 reader left for an AI rider
    // ($82:AB6F-AB8B): every input released and the direction neutral ($031B
    // = 1), A ($031F) and X ($0323) included. The selector ($0C75), countdown
    // ($0C6F) and suppression ($1277) keep their values (R-0048).
    if (marker & 0x8000U) {
        state.opponent_horizontal = 1;
        return true;
    }
    state.opponent_horizontal = (marker & 0x4000U) ? 0 : 2;
    // $83:E0C5-E0DA: while the turnaround counter $0C73 runs the opponent
    // rides against the marker, without jumping (LOCKED-TOURS).
    if (state.opponent_turnaround) {
        state.opponent_horizontal = static_cast<std::uint8_t>(2U - state.opponent_horizontal);
        --state.opponent_turnaround;
        return false;
    }
    // $83:E0DD-E111: stalled (previous x displacement below 3) in surface mode
    // on a slope of 26 or more against the marker's direction, it starts a
    // 30-update turnaround.
    if (state.surface[1].mode) {
        const auto angle = static_cast<std::int16_t>(rider.contact.surface_angle);
        const bool against = angle < 0 ? (state.opponent_horizontal == 2 && angle < -26)
                                       : (state.opponent_horizontal == 0 && angle >= 26);
        if (against && static_cast<std::int16_t>(rider.motion.previous_x_displacement) < 3) {
            state.opponent_turnaround = 30;
            return false;
        }
    }
    bool keep_jump = false;
    if (marker & 0x2000U) {
        input.jump_input = 1;
        if (ai.impulse_countdown) {
            if (ai.trick_selector & 1U)
                input.rotate_positive_input = 1;
            else
                input.rotate_negative_input = 1;
            return false;
        }
        if (rider.contact.unsupported_count >= 4 && negative(rider.motion.velocity_y)) {
            ai.impulse_countdown =
                static_cast<std::uint16_t>(-static_cast<std::int16_t>(rider.motion.velocity_y) / 2);
            // $83E16B compares the AI level $1275 with 2: below it ($83:E1A8)
            // the launch needs no feature total or a lead of three progress
            // transitions and suppresses for 30; above it (the HUNTER tour,
            // level 3) it always launches and suppresses for 60 (LOCKED-TOURS).
            // Level 2 is on no observed track and stays unrecovered.
            const auto level = classic_race_scenario(state.track).ai_level;
            if (level == 2) throw std::invalid_argument("AI level 2 is unrecovered");
            if (level > 2 || whole.rewards.feature_total == 0
                || static_cast<std::int16_t>(whole.riders[0].progress.transition_count
                                             - rider.progress.transition_count)
                       >= 3) {
                ai.suppression_counter = level > 2 ? 60 : 30;
                // $83E1CB-E21A. A flat launch only picks a rotation from the
                // velocity sign; a sloped one takes x&7, whose three bits drive
                // three independent inputs -- bit 0 the rotation at
                // $032F/$032B, bit 1 the opponent's A at $031F and bit 2 its X
                // at $0323. Bits 1 and 2 are consumed where the opponent's
                // reflection and roll run, below.
                if (rider.contact.surface_angle == 0) {
                    ai.trick_selector = negative(rider.motion.velocity_x) ? 0 : 1;
                } else
                    ai.trick_selector = rider.motion.x & 7U;
                if (ai.trick_selector & 1U)
                    input.rotate_positive_input = 1;
                else
                    input.rotate_negative_input = 1;
                return false;
            }
            ai.suppression_counter = 0;
        } else if (rider.contact.unsupported_count < 4 &&
                   // $83:E135-E13D: an AI level other than 1 (HUNTER's 3) takes
                   // the $83:E222 path at once, keeping the jump (LOCKED-TOURS).
                   (classic_race_scenario(state.track).ai_level != 1
                    || whole.rewards.feature_total == 0
                    || static_cast<std::int16_t>(whole.riders[0].progress.transition_count
                                                 - rider.progress.transition_count)
                           >= 3)) {
            // $83:E222: the jump stays set; the rotation check follows.
            keep_jump = true;
        }
    }
    if (!keep_jump) input.jump_input = whole.contact_phase; // $83:E21C
    ai.trick_selector = 0;
    ai.impulse_countdown = 0;
    if (!negative(rider.motion.velocity_y) && ai.suppression_counter > 0
        && rider.pose.reflected_orientation >= 16 && rider.pose.reflected_orientation < 48) {
        if (negative(rider.motion.velocity_x))
            input.rotate_negative_input = 1;
        else
            input.rotate_positive_input = 1;
    }
    return false;
}

void update_zoom_throttle(RiderMovementState& rider, ReflectionTransition& transition,
                          unsigned horizontal, int& animation_override, bool& throttle_target,
                          std::uint16_t& charge_announced, bool leading_support, bool bounce_active,
                          std::uint16_t drive_step, bool on_mud) {
    // Velocity part of the drive routines $82:A9B3 (rightward, +24) and
    // $82:AA10 (leftward, -24); on mud the step is 4 ($0F3B, R-0047). On an
    // inverted tile both move a nonzero velocity away from zero, and a roll
    // bounce ($042B) stores nothing.
    const auto drive_velocity = [&](bool rightward) {
        if (bounce_active) return;
        const auto back = static_cast<std::uint16_t>(0U - drive_step);
        if (rider.contact.selected_word & 0x8000U) {
            if (rider.motion.velocity_x)
                rider.motion.velocity_x = add_word(
                    rider.motion.velocity_x, negative(rider.motion.velocity_x) ? back : drive_step);
        } else
            rider.motion.velocity_x =
                add_word(rider.motion.velocity_x, rightward ? drive_step : back);
    };
    const auto incoming_speed = static_cast<std::int16_t>(rider.motion.velocity_x);
    const bool braking = transition.brake_input && (incoming_speed >= 16 || incoming_speed < -16);
    if (!leading_support && rider.contact.unsupported_count < 2
        && rider.contact.surface_angle != 0xffe1U && rider.contact.surface_angle != 31 && braking) {
        // $829909-9945 brakes through the opposite drive routine, clamps a
        // velocity that crossed zero, and returns before the previous-brake
        // latch publication. A bounce keeps the velocity (diff fuzz seed 140).
        if (incoming_speed > 0) {
            drive_velocity(false);
            if (negative(rider.motion.velocity_x)) rider.motion.velocity_x = 0;
        } else {
            drive_velocity(true);
            if (!negative(rider.motion.velocity_x)) rider.motion.velocity_x = 0;
        }
        rider.throttle = 0;
        return;
    }
    // $82:98CF-9A52. The charge latch ($0F63, serialized from $0D53/$0D55)
    // clears on the airborne, steep and neutral paths ($82:999A, $82:9A2B).
    if (leading_support || rider.contact.unsupported_count >= 2
        || rider.contact.surface_angle == 0xffe1U || rider.contact.surface_angle == 31) {
        rider.throttle = 0;
        charge_announced = 0;
    } else if (horizontal == 1) {
        rider.throttle = 0;
        charge_announced = 0;
    } else {

        // $82:A9C1–A9E0 / AA1E–AA3D: on inverted tiles the drive
        // increment follows the existing velocity sign, including zero hold.
        // Both drive routines return before throttle accumulation when an
        // inverted tile has zero incoming velocity ($82A9CC / $82AA29).
        if (!(rider.contact.selected_word & 0x8000U) || rider.motion.velocity_x != 0) {
            // $82:AA42-AA49 / its mirror: a roll bounce keeps the velocity;
            // throttle still accumulates below.
            drive_velocity(horizontal == 2);
            const auto cap = static_cast<std::uint16_t>(
                transition.base_velocity_cap
                + std::max(0, static_cast<int>(static_cast<std::int16_t>(rider.speed.boost)))
                + rider.launch_override);
            const auto next =
                add_word(rider.throttle, horizontal == 2 ? 16 : static_cast<std::uint16_t>(-16));
            if (horizontal == 2 ? negative(static_cast<std::uint16_t>(next - cap))
                                : !negative(static_cast<std::uint16_t>(
                                      next - static_cast<std::uint16_t>(-cap))))
                rider.throttle = next;
        }
        if (static_cast<std::int16_t>(rider.motion.previous_x_displacement) < 4) {
            if (rider.small_motion_counter != 4)
                rider.small_motion_counter = add_word(rider.small_motion_counter, 1);
            animation_override = static_cast<std::int16_t>(rider.small_motion_counter);
            throttle_target = true;
        }
        if (transition.brake_input) {
            rider.motion.velocity_x = 0;
            // $829995-99EF: a reflection step clears the announcement but
            // preserves accumulated throttle ($0F5F); nonzero throttle sets
            // it; zero throttle leaves it as it was (Left and Y through the
            // countdown carry throttle through zero, DRAGSTER fuzz seed 208).
            // $82:9981-9993: on flag pair 12 ($0FB1) or after mud ($0F45)
            // braking also drops the throttle and the announcement.
            if (on_mud) {
                rider.throttle = 0;
                charge_announced = 0;
            } else if (transition.step)
                charge_announced = 0;
            else if (rider.throttle)
                charge_announced = 1;
        } else {
            // $82:99F1-9A49: releasing the brake launches any throttle and
            // clears the announcement; without a previous brake nothing
            // changes, and the latch is already clear there.
            if (rider.previous_brake && rider.throttle) {
                rider.motion.velocity_x = add_word(rider.motion.velocity_x, rider.throttle);
                rider.throttle = 0;
                rider.launch_override = 256;
            }
            if (rider.previous_brake) charge_announced = 0;
        }
    }
    rider.previous_brake = transition.brake_input;
}

} // namespace unirally
