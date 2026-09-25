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

void update_movement(MovementState& state, const ControllerButtons& player_buttons,
                     const MovementContent& content) {
    state.player_input = sample_controller(player_buttons);
    if (state.player_input.horizontal == 0) {
        throw std::invalid_argument("leftward movement is outside the recovered primary domain");
    }
    if (state.finish.phase == RacePhase::FinishDelay && state.finish.player_finish_delay == 240) {
        state.finish.phase = RacePhase::ResultLoading;
        state.finish.result_loading_updates = 1;
        ++state.frame;
        return;
    }
    if (state.finish.phase == RacePhase::ResultLoading
        || state.finish.phase == RacePhase::ResultScreen) {
        if (state.finish.phase == RacePhase::ResultLoading) {
            ++state.finish.result_loading_updates;
            const auto stable_update = state.finish.outcome == RaceOutcome::PlayerWon ? 226U : 242U;
            if (state.finish.result_loading_updates >= stable_update)
                state.finish.phase = RacePhase::ResultScreen;
        }
        ++state.frame;
        return;
    }
    const auto timer_at_start = state.timer;
    const bool finish_delay = state.finish.phase == RacePhase::FinishDelay;
    // $83:EA72-$83:EAC3: when the opponent finishes first, its next update
    // receives the same neutral horizontal/action response and phased signed
    // slowdown while the player's timer and ordinary race remain live. This is
    // distinct from the later player-owned global finish delay (R-0017).
    const bool opponent_finished_first =
        state.finish.rider_finished[1] && !state.finish.rider_finished[0];
    if (finish_delay) {
        state.player_input.horizontal = 1;
        ++state.finish.player_finish_delay;
    }
    state.update_counter = static_cast<std::uint8_t>(state.update_counter + 1U);
    state.animation_counter = static_cast<std::uint8_t>((state.animation_counter + 1U) & 31U);
    state.contact_phase = static_cast<std::uint8_t>(1U - state.contact_phase);

    // The countdown handler publishes a forced brake while entering with 70 or
    // more, then decrements. End-1533 contains 69, so frame 1534 releases the
    // stored brake and takes the ordinary launch transition.
    const bool forced_brake = state.countdown >= 70;
    const bool timer_enabled = state.countdown < 69;
    if (state.countdown != 0) --state.countdown;
    const bool player_brake = forced_brake || player_buttons.b;
    bool opponent_jump =
        !opponent_finished_first && (state.riders[1].progress.marker_word & 0x2000U) != 0;
    if (opponent_jump && state.riders[1].contact.unsupported_count < 4
        && state.rewards.feature_total != 0) {
        const auto catch_up =
            static_cast<std::int16_t>(state.riders[0].progress.transition_count
                                      - state.riders[1].progress.transition_count - 3U);
        if (catch_up >= 0) {
            throw std::invalid_argument("opponent catch-up jump is outside the recovered domain");
        }
        // After a scored feature, the recovered AI copies the alternating
        // motion phase instead of asserting another continuous jump input.
        opponent_jump = state.contact_phase != 0;
    }
    bool opponent_trick = false;
    if (opponent_jump && state.opponent_ai.impulse_countdown) {
        opponent_trick = (state.opponent_ai.trick_selector & 1U) != 0;
    } else if (opponent_jump && state.riders[1].contact.unsupported_count >= 4) {
        state.opponent_ai.impulse_countdown = static_cast<std::uint16_t>(
            std::abs(static_cast<std::int16_t>(state.riders[1].motion.velocity_y)) >> 1);
        state.opponent_ai.trick_selector = 1;
        state.opponent_ai.suppression_counter = 30;
        opponent_trick = true;
    } else if (!opponent_jump) {
        state.opponent_ai.impulse_countdown = 0;
        state.opponent_ai.trick_selector = 0;
        if (opponent_finished_first) state.opponent_ai.suppression_counter = 0;
    }
    const unsigned active = state.contact_phase ? 0U : 1U;
    bool opponent_event_one = false;
    state.rewards.cooldown =
        state.rewards.cooldown > 2 ? static_cast<std::uint16_t>(state.rewards.cooldown - 2U) : 0;
    for (unsigned index = 0; index < state.riders.size(); ++index) {
        auto& rider = state.riders[index];
        const auto speed_before = rider.motion.velocity_x;
        decay_idle_wobble(rider);
        const auto horizontal = index == 0 ? (finish_delay ? 1U : state.player_input.horizontal)
                                           : (finish_delay || opponent_finished_first ? 1U : 2U);
        int animation_override =
            index == active
                ? stationary_animation_override(rider, static_cast<std::uint8_t>(horizontal))
                : 0;
        bool use_throttle_target = false;
        if (index == active) {
            const bool event_one = update_quarter_turns(rider);
            if (index == 1)
                opponent_event_one = event_one;
            else if (event_one)
                throw std::invalid_argument("player reward is outside the primary domain");
            update_jump(rider, index == 0 ? player_buttons.b : opponent_jump);
            // In the recovered branch rotation input is accepted only after the
            // contact count reaches nine; the synthesized opponent trick is the
            // positive two-step direction.
            if (index == 1 && opponent_trick && rider.contact.unsupported_count >= 9) {
                rider.motion.response_b = 2;
            } else {
                rider.motion.response_b = 0;
            }
            update_active_low_speed_damping(rider);
        }
        // The source dispatcher skips this pre-adjustment on each third
        // update; the ordinary limiter/damping still runs on every update.
        if ((finish_delay || (index == 1 && opponent_finished_first))
            && (state.frame + 1U) % 3U != 0U)
            apply_finish_slowdown(rider);
        update_horizontal(rider, index == 0 ? player_brake : forced_brake, horizontal == 2,
                          index == 1, state, content, animation_override, use_throttle_target);
        if (finish_delay || (index == 1 && opponent_finished_first)) {
            // Neutral finish response removes the 24-unit drive contribution
            // after ordinary limiting only when subtraction cannot cross zero.
            // Unlike the ten-unit pre-adjustment, a smaller remainder persists.
            const auto limited = static_cast<std::int16_t>(rider.motion.velocity_x);
            if (limited >= 24)
                rider.motion.velocity_x = static_cast<std::uint16_t>(limited - 24);
            else if (limited <= -24)
                rider.motion.velocity_x = static_cast<std::uint16_t>(limited + 24);
        }
        update_rolling_mode(rider);
        update_gravity(rider);
        integrate_motion(rider);
        update_idle_pose(rider, state.countdown == 0, index == 1, state.animation_counter,
                         content.idle_pose_table);
        update_pose(rider, state.animation_counter, state.contact_phase, content,
                    animation_override, use_throttle_target);
        if (index == 1 && opponent_finished_first) {
            update_opponent_finish_pose(state.finish, rider, state.frame + 1U);
        }
        if (finish_delay && index == 0 && state.finish.player_finish_delay == 2
            && speed_before == 460) {
            // The later-player path crosses a contact/pose boundary on its
            // second finish update; the source retains the prior value 15 for
            // this one sample although position advances by 13.
            rider.motion.previous_x_displacement = 15;
        }
    }
    (void)advance_timer_digits(state.timer, timer_enabled);
    update_reward_queue(state, opponent_event_one, content, {});
    std::array<TrackSamples, 2> samples{};
    for (std::size_t rider = 0; rider < state.riders.size(); ++rider) {
        const auto& movement = state.riders[rider];
        const auto points =
            collision_points(content.sampling, movement.pose.pose_index, movement.pose.reflected);
        samples[rider] =
            sample_track(content.sampling, points, movement.motion.x, movement.motion.y, 1024);
        const auto summary = summarize_flat_contact(content.flat_contact, points, samples[rider],
                                                    movement.motion.x, movement.motion.y);
        resolve_flat_contact(state.riders[rider].contact, state.riders[rider].motion, summary,
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
        // R-0013 bounds the tested crossing after $62A8 and by $62AD. The
        // aligned $62AC comparator is exact for both frozen Dragster paths;
        // no general boundary for another track is claimed.
        if (!state.finish.rider_finished[rider] && state.riders[rider].motion.x >= 0x62ACU) {
            record_finish(state.finish, rider, timer_at_start, state.frame + 1U);
        }
    }
    ++state.frame;
}

} // namespace unirally
