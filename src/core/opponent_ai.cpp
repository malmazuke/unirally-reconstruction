// The opponent's controller: what the AI presses each update.
//
// The AI rides by the track's markers ($83:E0A7-E21C): each marker word under the
// opponent says which way to ride and where to jump. A marker can turn the AI off for an
// update; a stall against a steep slope turns it around for a while; a jump marker can
// launch a trick with a rotation chosen from the opponent's x.

#include "opponent_ai.hpp"

#include "word_arithmetic.hpp"
#include "zoom_zoo_movement.hpp"

#include <cstdint>

namespace unirally {
namespace {

// The marker word's flags: the AI is off, ride left (else right), jump here.
constexpr std::uint16_t marker_ai_off = 0x8000, marker_leftward = 0x4000, marker_jump = 0x2000;
// $83:E0DD-E111: a stall (x displacement under 3) against a slope of 26 or more starts a
// 30-update turnaround.
constexpr std::int16_t steep_slope = 26, stalled_displacement = 3;
constexpr std::uint16_t turnaround_updates = 30;
// A trick launches when the opponent, rising, has been off the ground 4 updates or more.
constexpr std::uint16_t launch_airborne_updates = 4;
// A launch sets the AI's suppression word $1277: 30 below level 2, 60 above it, and at level 2
// the player's lead plus 15. The native AI only tests whether it is set: while it is, the rider
// rotates with its fall (below). Nothing in the recovered domain counts it down.
constexpr std::uint16_t rotation_updates = 30, level_three_rotation_updates = 60;
constexpr std::uint16_t level_two_lead_margin = 15;
// The AI lets the player catch up: below level 2 it launches only when the player has
// no feature total yet, or leads by 3 progress transitions or more.
constexpr std::int16_t launch_lead = 3;
// Level 2 (SILVIA) weighs the player's lead itself; at level 3 (GOLDWYN, HUNTER) the AI always
// launches.
constexpr std::uint16_t lead_weighing_level = 2;
// At level 2 no trick launches on an update whose animation counter ($04C7) ends in 7.
constexpr std::uint8_t skipped_launch_phase = 7;
// While rotating after a launch, it rotates with its fall only between these
// orientations (16-47).
constexpr std::uint16_t rotate_orientation_first = 16, rotate_orientation_end = 48;

enum class MarkerJump { launched, keep_jump, drop_jump };

void rotate(ReflectionTransition& input, bool positive) {
    if (positive)
        input.rotate_positive_input = 1;
    else
        input.rotate_negative_input = 1;
}

// The player's lead in progress transitions, $0FCD - $0FCF.
std::uint16_t player_lead(const ZoomZooState& state) {
    const auto& riders = state.movement.riders;
    return static_cast<std::uint16_t>(riders[0].progress.transition_count
                                      - riders[1].progress.transition_count);
}

bool player_lets_launch(const ZoomZooState& state) {
    return state.movement.rewards.feature_total == 0
        || static_cast<std::int16_t>(player_lead(state)) >= launch_lead;
}

// $83:E16B-E1C8: whether a rising opponent launches its trick, and the suppression word $1277
// the decision leaves. Below level 2 the player must let it (30, else 0). At level 2
// ($83:E17D-E1A5) the word is the player's lead + 15; a negative sum leaves 0 and no launch, and
// no trick launches when the animation counter ends in 7 either, though the word stays. Above
// level 2 it always launches, with 60 ($83:E175).
bool opponent_launches(ZoomZooState& state, unsigned ai_level) {
    auto& suppression = state.movement.opponent_ai.suppression_counter;
    if (ai_level < lead_weighing_level) {
        const bool lets = player_lets_launch(state);
        suppression = lets ? rotation_updates : 0;
        return lets;
    }
    if (ai_level > lead_weighing_level) {
        suppression = level_three_rotation_updates;
        return true;
    }
    const auto weighed = static_cast<std::uint16_t>(player_lead(state) + level_two_lead_margin);
    if (negative(weighed)) {
        suppression = 0;
        return false;
    }
    suppression = weighed;
    return (state.movement.animation_counter & skipped_launch_phase) != skipped_launch_phase;
}

// $83:E0DD-E111: stalled on a steep slope against the marker's direction, in surface
// mode.
bool stalled_against_slope(const ZoomZooState& state) {
    const auto& rider = state.movement.riders[1];
    if (!state.surface[1].mode) return false;
    const auto angle = static_cast<std::int16_t>(rider.contact.surface_angle);
    const bool against = angle < 0
                           ? (state.opponent_horizontal == direction::right && angle < -steep_slope)
                           : (state.opponent_horizontal == direction::left && angle >= steep_slope);
    return against
        && static_cast<std::int16_t>(rider.motion.previous_x_displacement) < stalled_displacement;
}

// $83:E1CB-E21A: a trick's rotation. Off a flat surface it follows the velocity's sign;
// off a slope it is x & 7, whose three bits drive three inputs: bit 0 the rotation
// ($032F/$032B), bit 1 the opponent's A ($031F) and bit 2 its X ($0323). Bits 1 and 2 are
// read where the opponent's reflection and roll run.
void launch_trick(ZoomZooState& state) {
    auto& rider = state.movement.riders[1];
    auto& ai = state.movement.opponent_ai;
    if (rider.contact.surface_angle == 0)
        ai.trick_selector = negative(rider.motion.velocity_x) ? 0 : 1;
    else
        ai.trick_selector = rider.motion.x & 7U;
    rotate(state.reflection[1], ai.trick_selector & 1U);
}

// A jump marker: keep rotating a launched trick, launch one while rising in the air
// ($83:E16B compares the AI level $1275 with 2), or decide whether the jump is held.
MarkerJump jump_at_marker(ZoomZooState& state) {
    auto& rider = state.movement.riders[1];
    auto& ai = state.movement.opponent_ai;
    if (ai.impulse_countdown) {
        rotate(state.reflection[1], ai.trick_selector & 1U);
        return MarkerJump::launched;
    }
    const auto level = state.opponent_tier.ai_level;
    if (rider.contact.unsupported_count >= launch_airborne_updates
        && negative(rider.motion.velocity_y)) {
        ai.impulse_countdown =
            static_cast<std::uint16_t>(-static_cast<std::int16_t>(rider.motion.velocity_y) / 2);
        if (opponent_launches(state, level)) {
            launch_trick(state);
            return MarkerJump::launched;
        }
        return MarkerJump::drop_jump;
    }
    // $83:E135-E13D: an AI level other than 1 (SILVIA's 2, GOLDWYN's and HUNTER's 3) takes the
    // $83:E222 path at once, keeping the jump (LOCKED-TOURS).
    if (rider.contact.unsupported_count < launch_airborne_updates
        && (level != 1 || player_lets_launch(state)))
        return MarkerJump::keep_jump;
    return MarkerJump::drop_jump;
}

} // namespace

// The opponent's inputs for one update. Returns true when a marker turned the AI off,
// ending the update early (R-0048).
bool update_opponent_controller(ZoomZooState& state) {
    auto& whole = state.movement;
    auto& input = state.reflection[1];
    auto& rider = whole.riders[1];
    auto& ai = whole.opponent_ai;
    input.brake_input = input.jump_input = input.rotate_negative_input =
        input.rotate_positive_input = 0;
    const auto marker = rider.progress.marker_word;
    // $83:E0A7-E0AF: with the AI off the opponent keeps what the port-2 reader left for an
    // AI rider ($82:AB6F-AB8B): every input released and the direction neutral ($031B), A
    // ($031F) and X ($0323) included. The selector ($0C75), countdown ($0C6F) and
    // suppression ($1277) keep their values.
    if (marker & marker_ai_off) {
        state.opponent_horizontal = direction::neutral;
        return true;
    }
    state.opponent_horizontal = (marker & marker_leftward) ? direction::left : direction::right;
    // $83:E0C5-E0DA: while the turnaround counter $0C73 runs the opponent rides against
    // the marker, without jumping (LOCKED-TOURS).
    if (state.opponent_turnaround) {
        state.opponent_horizontal = static_cast<std::uint8_t>(2U - state.opponent_horizontal);
        --state.opponent_turnaround;
        return false;
    }
    if (stalled_against_slope(state)) {
        state.opponent_turnaround = turnaround_updates;
        return false;
    }
    bool keep_jump = false;
    if (marker & marker_jump) {
        input.jump_input = 1;
        const auto jump = jump_at_marker(state);
        if (jump == MarkerJump::launched) return false;
        keep_jump = jump == MarkerJump::keep_jump;
    }
    // $83:E21C / $83:E222: without a held jump the jump input follows the contact phase.
    if (!keep_jump) input.jump_input = whole.contact_phase;
    ai.trick_selector = 0;
    ai.impulse_countdown = 0;
    if (!negative(rider.motion.velocity_y) && ai.suppression_counter > 0
        && rider.pose.reflected_orientation >= rotate_orientation_first
        && rider.pose.reflected_orientation < rotate_orientation_end)
        rotate(input, !negative(rider.motion.velocity_x));
    return false;
}

} // namespace unirally
