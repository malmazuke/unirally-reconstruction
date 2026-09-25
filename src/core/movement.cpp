// The legacy CRAWLER/DRAGSTER movement state (URMV) and its one-race update.

#include "movement.hpp"
#include "reward_queue.hpp"
#include "rider_motion.hpp"
#include "rider_pose.hpp"
#include "state_bytes.hpp"
#include "word_arithmetic.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <vector>

namespace unirally {

namespace {

// The legacy race: the finish delay (240 updates) and the result's load (226 updates when the
// player won, 242 when it lost); the countdown brakes from 70; the finish line at x 0x62AC
// on DRAGSTER's 1,024-column track.
constexpr std::uint16_t finish_delay_updates = 240;
constexpr std::uint16_t stable_result_won = 226, stable_result_lost = 242;
constexpr std::uint16_t countdown_brake_from = 70;
constexpr std::uint16_t dragster_finish_x = 0x62ac, dragster_coarse_columns = 1024;
constexpr std::int16_t finish_drive = 24;
// The scripted opponent: the jump marker's flag, a launch after 4 updates in the air, and
// 30 updates of rotation; fully airborne after 9.
constexpr std::uint16_t marker_jump = 0x2000, launch_airborne_updates = 4, rotation_updates = 30;
constexpr std::uint16_t airborne_updates = 9;

void write_rider(std::vector<std::uint8_t>& out, const RiderMovementState& r) {
    for (auto v : {r.motion.x, r.motion.y, r.motion.velocity_x, r.motion.velocity_y,
                   r.motion.previous_x_displacement, r.motion.response_a, r.motion.response_b,
                   r.motion.orientation_impulse})
        put16(out, v);
    for (auto v : {r.contact.unsupported_count, r.contact.previous_unsupported_count,
                   r.contact.unsupported_duration, r.contact.previous_uncorrected_x,
                   r.contact.previous_uncorrected_y, r.contact.surface_angle,
                   r.contact.auxiliary_flag, r.contact.selected_word})
        put16(out, v);
    put_bool(out, r.contact.angle_unspecified);
    put8(out, r.contact.selected_high);
    put_bool(out, r.contact.recontact);
    for (auto v : {r.speed.boost, r.speed.vertical_boost, r.speed.progress_adjustment,
                   r.progress.marker_word, r.progress.previous_tag, r.progress.transition_count})
        put16(out, v);
    put_bool(out, r.progress.transition_rejected);
    for (auto v : {r.jump.pending, r.jump.impulse_phase, r.jump.baseline, r.jump.previous_input,
                   r.pose.orientation, r.pose.reflected_orientation, r.pose.animation_phase,
                   r.pose.animation_increment, r.pose.previous_x, r.pose.previous_y,
                   r.pose.displacement_remainder, r.pose.target_orientation, r.pose.pose_index})
        put16(out, v);
    for (auto v : r.pose.displacement_history) put16(out, v);
    put16(out, r.pose.rolling_level);
    put16(out, r.pose.alternate_animation_phase);
    put_bool(out, r.pose.rolling);
    put_bool(out, r.pose.reflected);
    for (auto v :
         {r.idle_pose.active, r.idle_pose.wobble_offset, r.idle_pose.bias, r.idle_pose.velocity,
          r.idle_pose.previous_bias, r.idle_pose.direction_adjustment, r.idle_pose.cycle_latched,
          r.idle_pose.cycle_counter, r.idle_pose.orientation_reference})
        put16(out, v);
    for (auto v : {r.quarter_turn.previous_quadrant, r.quarter_turn.forward_turns,
                   r.quarter_turn.reverse_turns, r.quarter_turn.forward_quarters,
                   r.quarter_turn.reverse_quarters})
        put16(out, v);
    put_bool(out, r.quarter_turn.initialized);
    put_bool(out, r.quarter_turn.reflected_at_start);
    for (auto v : {r.residue_x, r.residue_y, r.throttle, r.previous_brake, r.launch_override,
                   r.small_motion_counter})
        put16(out, v);
}

void read_rider(Reader& in, RiderMovementState& r) {
    for (auto* v : {&r.motion.x, &r.motion.y, &r.motion.velocity_x, &r.motion.velocity_y,
                    &r.motion.previous_x_displacement, &r.motion.response_a, &r.motion.response_b,
                    &r.motion.orientation_impulse})
        *v = in.u16();
    for (auto* v : {&r.contact.unsupported_count, &r.contact.previous_unsupported_count,
                    &r.contact.unsupported_duration, &r.contact.previous_uncorrected_x,
                    &r.contact.previous_uncorrected_y, &r.contact.surface_angle,
                    &r.contact.auxiliary_flag, &r.contact.selected_word})
        *v = in.u16();
    r.contact.angle_unspecified = in.flag();
    r.contact.selected_high = in.u8();
    r.contact.recontact = in.flag();
    for (auto* v :
         {&r.speed.boost, &r.speed.vertical_boost, &r.speed.progress_adjustment,
          &r.progress.marker_word, &r.progress.previous_tag, &r.progress.transition_count})
        *v = in.u16();
    r.progress.transition_rejected = in.flag();
    for (auto* v :
         {&r.jump.pending, &r.jump.impulse_phase, &r.jump.baseline, &r.jump.previous_input,
          &r.pose.orientation, &r.pose.reflected_orientation, &r.pose.animation_phase,
          &r.pose.animation_increment, &r.pose.previous_x, &r.pose.previous_y,
          &r.pose.displacement_remainder, &r.pose.target_orientation, &r.pose.pose_index})
        *v = in.u16();
    for (auto& v : r.pose.displacement_history) v = in.u16();
    r.pose.rolling_level = in.u16();
    r.pose.alternate_animation_phase = in.u16();
    r.pose.rolling = in.flag();
    r.pose.reflected = in.flag();
    for (auto* v :
         {&r.idle_pose.active, &r.idle_pose.wobble_offset, &r.idle_pose.bias, &r.idle_pose.velocity,
          &r.idle_pose.previous_bias, &r.idle_pose.direction_adjustment, &r.idle_pose.cycle_latched,
          &r.idle_pose.cycle_counter, &r.idle_pose.orientation_reference})
        *v = in.u16();
    for (auto* v : {&r.quarter_turn.previous_quadrant, &r.quarter_turn.forward_turns,
                    &r.quarter_turn.reverse_turns, &r.quarter_turn.forward_quarters,
                    &r.quarter_turn.reverse_quarters})
        *v = in.u16();
    r.quarter_turn.initialized = in.flag();
    r.quarter_turn.reflected_at_start = in.flag();
    for (auto* v : {&r.residue_x, &r.residue_y, &r.throttle, &r.previous_brake, &r.launch_override,
                    &r.small_motion_counter})
        *v = in.u16();
}

bool has_finish_state(const RaceFinishState& finish) {
    return finish.rider_finished[0] || finish.rider_finished[1] || finish.player_finish_delay != 0
        || finish.result_loading_updates != 0 || finish.phase != RacePhase::Racing
        || finish.outcome != RaceOutcome::Pending;
}

void update_opponent_finish_pose(RaceFinishState& finish, RiderMovementState& rider,
                                 std::uint32_t output_frame) {
    // $82:8953-$82:89C2 with selector table $17:C7D6. Calls occur on the two
    // nonzero phases of the original three-phase counter. Entries 0..47 are
    // $0A45..$0A5C, each duplicated. Entry 48 is a negative sentinel whose
    // reset call republishes the first pose without consuming selector zero.
    auto& selector = finish.opponent_finish_pose_selector;
    if (output_frame % 3U != 0U) {
        if (selector >= 48U) {
            selector = 0;
        } else {
            rider.pose.pose_index = static_cast<std::uint16_t>(0x0a45U + selector / 2U);
            ++selector;
            return;
        }
    }
    // `$0DF1` persists between animation-table calls and is copied through
    // `$0F59` on every update before collision sampling.
    rider.pose.pose_index =
        static_cast<std::uint16_t>(0x0a45U + (selector == 0U ? 0U : (selector - 1U) / 2U));
}

std::uint16_t finish_centiseconds(const RaceTimerDigits& timer, std::uint32_t frame) {
    const auto value = timer.minutes * 6000U + timer.tens_seconds * 1000U + timer.seconds * 100U
                     + timer.tenths * 10U + timer.subframe * 2U + (frame & 1U);
    return static_cast<std::uint16_t>(value);
}

void record_finish(RaceFinishState& finish, std::size_t rider, const RaceTimerDigits& timer,
                   std::uint32_t frame) {
    finish.rider_finished[rider] = true;
    finish.finish_time_centiseconds[rider] = finish_centiseconds(timer, frame);
    finish.finish_time_digits[rider] = {
        timer.minutes, timer.tens_seconds, timer.seconds, timer.tenths,
        static_cast<std::uint16_t>(timer.subframe * 2U + (frame & 1U))};
    finish.finish_animation_countdown[rider] = 120;
    if (rider == 0) {
        finish.outcome =
            finish.rider_finished[1] ? RaceOutcome::PlayerLost : RaceOutcome::PlayerWon;
        finish.phase = RacePhase::FinishDelay;
    }
}

// The result screen once the player has finished: after the 240-update finish delay the
// result loads for 226 updates when the player won and 242 when it lost, then is stable.
// Returns true when the update was a result update.
bool advance_result_phases(MovementState& state) {
    auto& finish = state.finish;
    if (finish.phase == RacePhase::FinishDelay
        && finish.player_finish_delay == finish_delay_updates) {
        finish.phase = RacePhase::ResultLoading;
        finish.result_loading_updates = 1;
        ++state.frame;
        return true;
    }
    if (finish.phase != RacePhase::ResultLoading && finish.phase != RacePhase::ResultScreen)
        return false;
    if (finish.phase == RacePhase::ResultLoading) {
        ++finish.result_loading_updates;
        const auto stable_update =
            finish.outcome == RaceOutcome::PlayerWon ? stable_result_won : stable_result_lost;
        if (finish.result_loading_updates >= stable_update) finish.phase = RacePhase::ResultScreen;
    }
    ++state.frame;
    return true;
}

// The legacy opponent's scripted controls: jump at a jump marker and, launching into the
// air, a positive rotation (trick selector 1). After a scored feature the recovered AI copies
// the alternating motion phase instead of holding the jump; a catch-up jump is unrecovered.
struct ScriptedOpponent {
    bool jump{}, trick{};
};

ScriptedOpponent scripted_opponent(MovementState& state, bool opponent_finished_first) {
    auto& ai = state.opponent_ai;
    const auto& opponent = state.riders[1];
    bool jump = !opponent_finished_first && (opponent.progress.marker_word & marker_jump) != 0;
    if (jump && opponent.contact.unsupported_count < launch_airborne_updates
        && state.rewards.feature_total != 0) {
        const auto catch_up = static_cast<std::int16_t>(state.riders[0].progress.transition_count
                                                        - opponent.progress.transition_count - 3U);
        if (catch_up >= 0)
            throw std::invalid_argument("opponent catch-up jump is outside the recovered domain");
        jump = state.contact_phase != 0;
    }
    bool trick = false;
    if (jump && ai.impulse_countdown) {
        trick = (ai.trick_selector & 1U) != 0;
    } else if (jump && opponent.contact.unsupported_count >= launch_airborne_updates) {
        ai.impulse_countdown = static_cast<std::uint16_t>(
            std::abs(static_cast<std::int16_t>(opponent.motion.velocity_y)) >> 1);
        ai.trick_selector = 1;
        ai.suppression_counter = rotation_updates;
        trick = true;
    } else if (!jump) {
        ai.impulse_countdown = 0;
        ai.trick_selector = 0;
        if (opponent_finished_first) ai.suppression_counter = 0;
    }
    return {jump, trick};
}

struct LegacyControls {
    bool player_brake{}, player_jump{}, forced_brake{}, finish_delay{}, opponent_finished_first{};
    ScriptedOpponent opponent{};
};

// The neutral finish response removes the 24-unit drive contribution after ordinary
// limiting, only when the subtraction cannot cross zero. Unlike the ten-unit
// pre-adjustment, a smaller remainder persists.
void remove_finish_drive(RiderMovementState& rider) {
    const auto limited = static_cast<std::int16_t>(rider.motion.velocity_x);
    if (limited >= finish_drive)
        rider.motion.velocity_x = static_cast<std::uint16_t>(limited - finish_drive);
    else if (limited <= -finish_drive)
        rider.motion.velocity_x = static_cast<std::uint16_t>(limited + finish_drive);
}

// One rider's legacy update. Returns whether its quarter turns completed a roll (event one),
// the only reward the legacy domain recovers, and only for the opponent.
bool update_legacy_rider(MovementState& state, unsigned index, unsigned active,
                         const LegacyControls& controls, const MovementContent& content) {
    auto& rider = state.riders[index];
    const auto speed_before = rider.motion.velocity_x;
    const bool slowing = controls.finish_delay || (index == 1 && controls.opponent_finished_first);
    decay_idle_wobble(rider);
    const auto horizontal =
        index == 0 ? (controls.finish_delay ? direction::neutral : state.player_input.horizontal)
                   : (slowing ? direction::neutral : direction::right);
    int animation_override =
        index == active
            ? stationary_animation_override(rider, static_cast<std::uint8_t>(horizontal))
            : 0;
    bool use_throttle_target = false;
    bool event_one = false;
    if (index == active) {
        event_one = update_quarter_turns(rider) != 0;
        if (index == 0 && event_one)
            throw std::invalid_argument("player reward is outside the primary domain");
        update_jump(rider, index == 0 ? controls.player_jump : controls.opponent.jump);
        // Rotation input is accepted only once the rider is fully airborne; the scripted
        // opponent's trick is the positive two-step rotation.
        rider.motion.response_b = index == 1 && controls.opponent.trick
                                       && rider.contact.unsupported_count >= airborne_updates
                                    ? 2
                                    : 0;
        update_active_low_speed_damping(rider);
    }
    // The dispatcher skips this pre-adjustment on each third update; the ordinary
    // limiter/damping still runs on every update.
    if (slowing && (state.frame + 1U) % 3U != 0U) apply_finish_slowdown(rider);
    update_horizontal(rider, index == 0 ? controls.player_brake : controls.forced_brake,
                      horizontal == direction::right, index == 1, state, content,
                      animation_override, use_throttle_target);
    if (slowing) remove_finish_drive(rider);
    update_rolling_mode(rider);
    update_gravity(rider);
    integrate_motion(rider);
    update_idle_pose(rider, state.countdown == 0, index == 1, state.animation_counter,
                     content.idle_pose_table);
    update_pose(rider, state.animation_counter, state.contact_phase, content, animation_override,
                use_throttle_target);
    if (index == 1 && controls.opponent_finished_first)
        update_opponent_finish_pose(state.finish, rider, state.frame + 1U);
    if (controls.finish_delay && index == 0 && state.finish.player_finish_delay == 2
        && speed_before == 460) {
        // The later-player path crosses a contact/pose boundary on its second finish update;
        // the source retains the prior value 15 for this one sample although position
        // advances by 13.
        rider.motion.previous_x_displacement = 15;
    }
    return event_one;
}

// Each rider's flat contact, the alternating progress update, the finish animations and
// the finish line. R-0013 bounds the tested crossing after x 0x62A8 and by 0x62AD; the
// aligned comparator 0x62AC is exact for both frozen DRAGSTER paths, and no general
// boundary for another track is claimed.
void update_contacts_and_finish(MovementState& state, const MovementContent& content,
                                const RaceTimerDigits& timer_at_start) {
    std::array<TrackSamples, 2> samples{};
    for (std::size_t rider = 0; rider < state.riders.size(); ++rider) {
        auto& movement = state.riders[rider];
        const auto points =
            collision_points(content.sampling, movement.pose.pose_index, movement.pose.reflected);
        samples[rider] = sample_track(content.sampling, points, movement.motion.x,
                                      movement.motion.y, dragster_coarse_columns);
        const auto summary = summarize_flat_contact(content.flat_contact, points, samples[rider],
                                                    movement.motion.x, movement.motion.y);
        resolve_flat_contact(movement.contact, movement.motion, summary,
                             {state.contact_phase, rider == 1, 0, 0});
    }
    ProgressUpdateState progress{{state.riders[0].progress, state.riders[1].progress},
                                 state.progress_phase};
    update_track_progress(progress, samples, content.progress_transitions);
    state.progress_phase = progress.phase;
    for (std::size_t rider = 0; rider < state.riders.size(); ++rider) {
        state.riders[rider].progress = progress.riders[rider];
        if (state.finish.finish_animation_countdown[rider] != 0)
            --state.finish.finish_animation_countdown[rider];
        if (!state.finish.rider_finished[rider]
            && state.riders[rider].motion.x >= dragster_finish_x)
            record_finish(state.finish, rider, timer_at_start, state.frame + 1U);
    }
}

} // namespace

std::vector<std::uint8_t> serialize_movement_state(const MovementState& s) {
    if (s.contact_phase > 1 || s.progress_phase > 1)
        throw std::invalid_argument("movement phase is not binary");
    const bool version_three = s.finish.opponent_finish_pose_selector != 0;
    const bool version_two = version_three || has_finish_state(s.finish);
    const auto& magic = version_three
                          ? movement_state_magic_v3
                          : (version_two ? movement_state_magic_v2 : movement_state_magic);
    std::vector<std::uint8_t> out(magic.begin(), magic.end());
    put32(out, s.frame);
    for (auto v : {s.player_input.low_image, s.player_input.high_image, s.player_input.vertical,
                   s.player_input.horizontal})
        put8(out, v);
    for (const auto& rider : s.riders) write_rider(out, rider);
    for (auto v :
         {s.timer.minutes, s.timer.tens_seconds, s.timer.seconds, s.timer.tenths, s.timer.subframe})
        put16(out, v);
    for (auto v : {s.opponent_ai.impulse_countdown, s.opponent_ai.trick_selector,
                   s.opponent_ai.suppression_counter})
        put16(out, v);
    for (auto v : s.rewards.entries) put8(out, v);
    put8(out, s.rewards.read_cursor);
    put8(out, s.rewards.write_cursor);
    put16(out, s.rewards.cooldown);
    put16(out, s.rewards.feature_total);
    put8(out, s.rewards.event_one_weight);
    put16(out, s.countdown);
    put8(out, s.contact_phase);
    put8(out, s.progress_phase);
    put8(out, s.animation_counter);
    put8(out, s.update_counter);
    if (version_two) {
        for (auto value : s.finish.rider_finished) put_bool(out, value);
        for (auto value : s.finish.finish_time_centiseconds) put16(out, value);
        for (const auto& digits : s.finish.finish_time_digits)
            for (auto value : digits) put16(out, value);
        for (auto value : s.finish.finish_animation_countdown) put16(out, value);
        put16(out, s.finish.player_finish_delay);
        put16(out, s.finish.result_loading_updates);
        put8(out, static_cast<std::uint8_t>(s.finish.phase));
        put8(out, static_cast<std::uint8_t>(s.finish.outcome));
        if (version_three) put16(out, s.finish.opponent_finish_pose_selector);
    }
    return out;
}

MovementState deserialize_movement_state(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < movement_state_magic.size())
        throw std::invalid_argument("movement state is truncated");
    const bool version_one =
        std::equal(movement_state_magic.begin(), movement_state_magic.end(), bytes.begin());
    const bool version_two =
        std::equal(movement_state_magic_v2.begin(), movement_state_magic_v2.end(), bytes.begin());
    const bool version_three =
        std::equal(movement_state_magic_v3.begin(), movement_state_magic_v3.end(), bytes.begin());
    if (!version_one && !version_two && !version_three)
        throw std::invalid_argument("movement state magic is unsupported");
    Reader in(bytes.subspan(movement_state_magic.size()));
    MovementState s{};
    s.frame = in.u32();
    s.player_input.low_image = in.u8();
    s.player_input.high_image = in.u8();
    s.player_input.vertical = in.u8();
    s.player_input.horizontal = in.u8();
    for (auto& rider : s.riders) read_rider(in, rider);
    for (auto* v : {&s.timer.minutes, &s.timer.tens_seconds, &s.timer.seconds, &s.timer.tenths,
                    &s.timer.subframe})
        *v = in.u16();
    s.opponent_ai.impulse_countdown = in.u16();
    s.opponent_ai.trick_selector = in.u16();
    s.opponent_ai.suppression_counter = in.u16();
    for (auto& v : s.rewards.entries) v = in.u8();
    s.rewards.read_cursor = in.u8();
    s.rewards.write_cursor = in.u8();
    s.rewards.cooldown = in.u16();
    s.rewards.feature_total = in.u16();
    s.rewards.event_one_weight = in.u8();
    s.countdown = in.u16();
    s.contact_phase = in.u8();
    s.progress_phase = in.u8();
    s.animation_counter = in.u8();
    s.update_counter = in.u8();
    if (version_two || version_three) {
        for (auto& value : s.finish.rider_finished) value = in.flag();
        for (auto& value : s.finish.finish_time_centiseconds) value = in.u16();
        for (auto& digits : s.finish.finish_time_digits)
            for (auto& value : digits) value = in.u16();
        for (auto& value : s.finish.finish_animation_countdown) value = in.u16();
        s.finish.player_finish_delay = in.u16();
        s.finish.result_loading_updates = in.u16();
        s.finish.phase = static_cast<RacePhase>(in.u8());
        s.finish.outcome = static_cast<RaceOutcome>(in.u8());
        if (version_three) s.finish.opponent_finish_pose_selector = in.u16();
    }
    if (s.contact_phase > 1 || s.progress_phase > 1 || s.animation_counter > 31
        || s.player_input.vertical > 2 || s.player_input.horizontal > 2
        || s.rewards.read_cursor > 31 || s.rewards.write_cursor > 31)
        throw std::invalid_argument("movement state contains an out-of-domain counter");
    if (static_cast<std::uint8_t>(s.finish.phase) > 3
        || static_cast<std::uint8_t>(s.finish.outcome) > 2 || s.finish.player_finish_delay > 240
        || s.finish.opponent_finish_pose_selector > 48)
        throw std::invalid_argument("movement state contains invalid finish state");
    for (const auto& rider : s.riders) {
        if (rider.idle_pose.active > 1 || rider.idle_pose.direction_adjustment > 1
            || rider.idle_pose.cycle_latched > 1 || rider.idle_pose.cycle_counter >= 120
            || rider.idle_pose.orientation_reference >= 64) {
            throw std::invalid_argument("movement state contains an out-of-domain idle pose field");
        }
    }
    (void)serialize_timer(s.timer); // Reuse the reviewed digit-domain validation.
    in.require_end();
    return s;
}

MovementState classic_crawler_dragster_start() {
    MovementState state{};
    state.frame = 1533;
    state.player_input.high_image = 1;
    state.player_input.vertical = 1;
    state.player_input.horizontal = 2;
    for (auto& rider : state.riders) {
        rider.motion.x = 0x0440;
        rider.motion.y = 0x035a;
        rider.contact.previous_uncorrected_x = 0x0440;
        rider.contact.previous_uncorrected_y = 0x035b;
        rider.contact.selected_word = 0x1804;
        rider.contact.selected_high = 0x18;
        rider.pose.orientation = 6;
        rider.pose.reflected_orientation = 0x3a;
        rider.pose.animation_phase = 0x13;
        rider.pose.previous_x = 0x0440;
        rider.pose.previous_y = 0x035b;
        rider.pose.target_orientation = 6;
        rider.pose.pose_index = 0x04fa;
        rider.pose.reflected = true;
        rider.quarter_turn.reflected_at_start = true;
        rider.residue_y = 0x17;
        rider.throttle = 0x01b0;
        rider.previous_brake = 1;
        rider.small_motion_counter = 4;
    }
    state.riders[0].idle_pose.orientation_reference = 1;
    state.riders[0].residue_x = 0xffff;
    state.riders[1].idle_pose.orientation_reference = 60;
    state.rewards.write_cursor = 1;
    state.rewards.cooldown = 2;
    state.rewards.event_one_weight = 4;
    state.countdown = 0x45;
    state.contact_phase = 1;
    state.progress_phase = 1;
    state.animation_counter = 0x0d;
    state.update_counter = 0xcd;
    return state;
}

// One update of the legacy CRAWLER/DRAGSTER race (R-0011, R-0017): the result phases once
// the player has finished, else the countdown, the opponent's scripted controls, each rider,
// the race clock and the opponent's queue, then each rider's contact, the progress markers
// and the finish line.
void update_movement(MovementState& state, const ControllerButtons& player_buttons,
                     const MovementContent& content) {
    state.player_input = sample_controller(player_buttons);
    if (state.player_input.horizontal == direction::left)
        throw std::invalid_argument("leftward movement is outside the recovered primary domain");
    if (advance_result_phases(state)) return;
    const auto timer_at_start = state.timer;
    const bool finish_delay = state.finish.phase == RacePhase::FinishDelay;
    // $83:EA72-$83:EAC3: when the opponent finishes first, its next update receives the same
    // neutral horizontal/action response and phased signed slowdown while the player's timer
    // and ordinary race remain live. This is distinct from the later player-owned global
    // finish delay (R-0017).
    const bool opponent_finished_first =
        state.finish.rider_finished[1] && !state.finish.rider_finished[0];
    if (finish_delay) {
        state.player_input.horizontal = direction::neutral;
        ++state.finish.player_finish_delay;
    }
    state.update_counter = static_cast<std::uint8_t>(state.update_counter + 1U);
    state.animation_counter = static_cast<std::uint8_t>((state.animation_counter + 1U) & 31U);
    state.contact_phase = static_cast<std::uint8_t>(1U - state.contact_phase);
    // The countdown handler publishes a forced brake while entering with 70 or more, then
    // decrements. End-1533 contains 69, so frame 1534 releases the stored brake and takes the
    // ordinary launch transition.
    const bool forced_brake = state.countdown >= countdown_brake_from;
    const bool timer_enabled = state.countdown < countdown_brake_from - 1U;
    if (state.countdown != 0) --state.countdown;
    const LegacyControls controls{forced_brake || player_buttons.b,
                                  player_buttons.b,
                                  forced_brake,
                                  finish_delay,
                                  opponent_finished_first,
                                  scripted_opponent(state, opponent_finished_first)};
    const unsigned active = state.contact_phase ? 0U : 1U;
    state.rewards.cooldown =
        state.rewards.cooldown > 2 ? static_cast<std::uint16_t>(state.rewards.cooldown - 2U) : 0;
    bool opponent_event_one = false;
    for (unsigned index = 0; index < state.riders.size(); ++index) {
        const bool event_one = update_legacy_rider(state, index, active, controls, content);
        if (index == 1) opponent_event_one = event_one;
    }
    (void)advance_timer_digits(state.timer, timer_enabled);
    update_opponent_announcements(state, opponent_event_one, content, {});
    update_contacts_and_finish(state, content, timer_at_start);
    ++state.frame;
}

} // namespace unirally
