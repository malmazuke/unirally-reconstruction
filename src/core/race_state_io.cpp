// The serialized race states (URZZ, URDG, URTRnn): writing, reading and validation.
//
// A race state is the legacy movement state's 333 bytes under the race's identity, then
// these sections, little-endian, each present by the layout the identity names:
//
//   controls        both riders' reflection transitions, the opponent's direction and OAM x
//   surfaces        both riders' surface transitions (sustained: URZZ0002 and later)
//   race progress   laps, times, camera, the first 20 checkpoint flags, finish poses
//                   (complete race: URZZ0003 and later)
//   native race     fade, start boosts, result, charge flags, the player's announcements,
//                   rolls, learned weights, pause (natively started: URZZ000B, 742 bytes)
//   special tiles   both riders' special-tile words, the drive target latch and the
//                   opponent's turnaround (R-0047; every track but DRAGSTER and ZOOM ZOO, and
//                   those two only while a word is live: URDG0004, URZZ000E, 794 bytes)
//   more flags      the last 60 checkpoint flags (R-0048; other tracks)
//   HUNTER effects  (R-0052; other tracks: URTRnn06, 916 bytes)
//
// Reading refuses any state the original cannot produce: each section's guards run as it is
// read, and the natively started race's cross-checks run in the order below.

#include "announcements.hpp"
#include "race_progress.hpp"
#include "reward_queue.hpp"
#include "state_bytes.hpp"
#include "word_arithmetic.hpp"
#include "zoom_zoo_movement.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace unirally {
namespace {

constexpr std::size_t movement_prefix_size = 333;
constexpr std::size_t first_layout_size = 395, sustained_size = 423, complete_race_size = 565;
constexpr std::size_t native_race_size = 742, extended_size = 794, other_track_size = 916;
constexpr std::size_t special_tiles_size = 52, hunter_effects_size = 62;
constexpr std::array<std::uint8_t, 8> race_state_magic{'U', 'R', 'Z', 'Z', '0', '0', '0', '1'};
// The layout letter in byte 7 of the identity.
constexpr std::uint8_t sustained_layout = '2', complete_race_layout = '3', native_race_layout = 'B';
constexpr std::uint8_t extended_zoom_zoo_layout = 'E', extended_dragster_layout = '4';
// DRAGSTER's one lap and ZOOM ZOO's three reach only the first 20 of the 80 checkpoint flags.
constexpr unsigned shared_checkpoint_flags = 20, checkpoint_flags = 80;
constexpr std::uint8_t checkpoint_unseen = 255;

// Race limits the guards check.
constexpr std::uint16_t checkpoints_per_lap = 4, longest_checkpoint_display = 120;
constexpr std::uint16_t finish_display_updates = 240, fastest_camera = 16;
constexpr std::uint16_t zoom_zoo_camera_x_limit = 0x3fff;
constexpr unsigned last_finish_pose_one = 48, last_finish_pose_two = 88;
constexpr unsigned lap_slots = 10;
// The native start: 30 fade updates, a 4-update delay, a 270-update countdown, start boosts
// of 384 until it reaches 128; the tutorial hints every 300 updates from 30, eight groups.
constexpr std::uint16_t brightest_fade = 30, start_boost = 384, last_start_boost_countdown = 129;
constexpr unsigned countdown_delay = 4, countdown_updates = 270;
constexpr unsigned hint_interval = 300, first_hint_updates = 30, hint_groups = 8;
// The announcement queues: 32 entries, a player's cooldown of up to 120 (hints) and an
// opponent's of up to 40 ($81:C2CC-C2D0).
constexpr std::uint8_t last_queue_slot = 31;
constexpr std::uint16_t longest_player_cooldown = 120, longest_opponent_cooldown = 40;
constexpr std::uint8_t heaviest_learned_weight = 64;
// The X trick's step runs -9 to 9; a bounce charges to 160; the opponent's trick selector
// is x & 7 at most ($83:E1F1).
constexpr std::int16_t spin_steps = 9;
constexpr std::uint16_t bounce_charge = 160, highest_trick_selector = 7;
constexpr std::uint16_t roll_pose_mirror = 0x8000, roll_pose_unused_bit = 0x4000;
constexpr std::uint16_t roll_pose_bits = 0x3fff;
// A state restored from a capture (not natively started) lies within the trial horizon.
constexpr std::uint32_t trial_first_frame = 1649, trial_last_frame = 1849,
                        sustained_last_frame = 9999;

void refuse_unless(bool condition, const char* refusal) {
    if (!condition) throw std::invalid_argument(refusal);
}

// The halvings of the learned-weight template's event-one byte ($82:D7A4) that a queue can
// hold. They coincide for the frozen pack, so a template change must revisit this bound.
bool reachable_event_one_weight(std::uint8_t weight) {
    return weight == 1 || weight == 2 || weight == 4;
}

// ------------------------------------------------------------------ writing

void write_reflection(std::vector<std::uint8_t>& bytes, const ReflectionTransition& r) {
    for (auto v : {r.step, r.end, r.pose_base, r.pose_override, r.completed, r.hold,
                   r.drive_pose_enabled, r.air_turns, r.direction_latch, r.base_velocity_cap,
                   r.brake_input, r.rotate_negative_input, r.rotate_positive_input, r.jump_input,
                   r.wrong_direction_counter})
        put16(bytes, v);
}

void write_surfaces(std::vector<std::uint8_t>& bytes, const ZoomZooState& state) {
    for (const auto& r : state.surface)
        for (auto v : {r.mode, r.angle, r.tile_mode, r.leading_support, r.tile_pose,
                       r.animation_delta, r.tile_pose_enabled})
            put16(bytes, v);
}

void write_race_progress(std::vector<std::uint8_t>& bytes, const ZoomZooRaceState& race) {
    for (const auto& r : race.riders) {
        for (auto v : {r.laps_remaining, r.checkpoint, r.next_checkpoint, r.start_line_latch,
                       r.checkpoint_display_countdown, r.finished})
            put16(bytes, v);
        for (auto v : r.time_digits) put16(bytes, v);
    }
    for (const auto& times : race.lap_times)
        for (auto v : times) put16(bytes, v);
    for (auto v : race.total_times) put16(bytes, v);
    for (auto v : {race.provisional_1225, race.provisional_1227, race.finish_delay})
        put16(bytes, v);
    const auto& c = race.camera;
    for (auto v : {c.x, c.y, c.velocity_x, c.velocity_y, c.lookahead, c.screen_xy}) put16(bytes, v);
    for (unsigned i = 0; i < shared_checkpoint_flags; ++i) put8(bytes, race.checkpoint_seen[i]);
    for (const auto& p : race.finish_pose)
        for (auto v : {p.selector, p.kind, p.locked, p.active}) put16(bytes, v);
}

void write_native_race(std::vector<std::uint8_t>& bytes, const ZoomZooState& state) {
    put16(bytes, state.fade_level);
    for (auto v : state.start_boost) put16(bytes, v);
    put16(bytes, state.result_updates);
    put16(bytes, state.result.graph_minimum);
    put16(bytes, state.result.graph_maximum);
    for (auto v : state.result.published_totals) put16(bytes, v);
    for (auto v : state.charge_announced) put16(bytes, v);
    const auto& a = state.player_announcements;
    const auto& q = a.queue;
    for (auto v : q.entries) put8(bytes, v);
    put8(bytes, q.read_cursor);
    put8(bytes, q.write_cursor);
    put16(bytes, q.cooldown);
    put16(bytes, q.feature_total);
    put8(bytes, q.event_one_weight);
    for (auto v : {a.hints_active, a.hint_updates, a.hint_group, a.empty_display}) put16(bytes, v);
    for (const auto& r : state.rolls)
        for (auto v : {r.input_latched, r.prior_orientation, r.prior_reflection, r.pose_base,
                       r.step, r.held_updates, r.bounce_charge, r.completed_rolls, r.held_rotations,
                       r.bounce_active, r.support_count_mirror, r.prior_step})
            put16(bytes, v);
    for (const auto& weights : state.learned_weights)
        for (auto v : weights) put8(bytes, v);
    put16(bytes, state.pause.selection);
    put16(bytes, state.pause.released);
    put32(bytes, state.pause.suspended_updates);
    put32(bytes, state.pause.suspended_countdown_updates);
}

void write_special_tiles(std::vector<std::uint8_t>& bytes, const ZoomZooState& state) {
    for (const auto& r : state.special_tiles)
        for (auto v : {r.mud_cooldown, r.mud_exit_pending, r.corkscrew_latch, r.corkscrew_step,
                       r.corkscrew_float, r.physics_hold, r.reflection_lock, r.raised_priority,
                       r.loop_direction, r.loop_step, r.loop_cooldown, r.slow_counter})
            put16(bytes, v);
    put16(bytes, state.drive_target_latch);
    put16(bytes, state.opponent_turnaround);
}

void write_hunter_effects(std::vector<std::uint8_t>& bytes, const HunterEffects& h) {
    put16(bytes, h.latched);
    put16(bytes, h.active);
    for (auto v : h.effect) put16(bytes, v);
    for (auto v : h.timer) put16(bytes, v);
    for (auto v :
         {h.pulse, h.pulse_shrinking, h.pulse_length, h.blink, h.wave_phase, h.hide_track, h.mosaic,
          h.skip_update, h.message, h.shown, h.hud_event, h.caption, h.mosaic_counter})
        put16(bytes, v);
}

bool is_other_track(ClassicRaceTrack track) {
    return track != ClassicRaceTrack::ZoomZoo && track != ClassicRaceTrack::Dragster;
}

// ------------------------------------------------------------------ reading

void read_reflection(Reader& in, ReflectionTransition& r) {
    for (auto* v : {&r.step, &r.end, &r.pose_base, &r.pose_override, &r.completed, &r.hold,
                    &r.drive_pose_enabled, &r.air_turns, &r.direction_latch, &r.base_velocity_cap,
                    &r.brake_input, &r.rotate_negative_input, &r.rotate_positive_input,
                    &r.jump_input, &r.wrong_direction_counter})
        *v = in.u16();
}

void read_surfaces(Reader& in, ZoomZooState& state) {
    for (auto& r : state.surface)
        for (auto* v : {&r.mode, &r.angle, &r.tile_mode, &r.leading_support, &r.tile_pose,
                        &r.animation_delta, &r.tile_pose_enabled})
            *v = in.u16();
}

void read_race_riders(Reader& in, ZoomZooRaceState& race, const ClassicRaceScenario& scenario) {
    for (auto& r : race.riders) {
        for (auto* v : {&r.laps_remaining, &r.checkpoint, &r.next_checkpoint, &r.start_line_latch,
                        &r.checkpoint_display_countdown, &r.finished})
            *v = in.u16();
        for (auto& v : r.time_digits) v = in.u16();
        refuse_unless(r.laps_remaining <= scenario.laps + 1U && r.checkpoint < checkpoints_per_lap
                          && r.next_checkpoint < checkpoints_per_lap && r.start_line_latch <= 1
                          && r.finished <= 1
                          && r.checkpoint_display_countdown <= longest_checkpoint_display,
                      "ZOOM ZOO race checkpoint state is invalid");
    }
}

void check_finish_poses(const ZoomZooRaceState& race) {
    for (unsigned index = 0; index < 2; ++index) {
        const auto& pose = race.finish_pose[index];
        const unsigned limit = pose.kind == 1 ? last_finish_pose_one
                             : pose.kind == 2 ? last_finish_pose_two
                                              : 0U;
        refuse_unless(!((pose.active && !race.riders[index].finished) || pose.selector > limit
                        || pose.kind > 2 || pose.locked > 1 || pose.active > 1
                        || (pose.active ? (!pose.kind || !pose.locked)
                                        : (pose.kind || pose.locked || pose.selector))),
                      "ZOOM ZOO finish pose state invalid");
    }
}

// $81:C73E-C75B finishes both riders, laps or not, once the clock holds 9:59.9.
void check_lap_clocks(const ZoomZooState& state) {
    const auto& clock = state.movement.timer;
    const bool clock_expired = state.native_initialization && clock.minutes == 9
                            && clock.tens_seconds == 5 && clock.seconds == 9 && clock.tenths == 9;
    const bool timed_out =
        state.race.riders[0].finished && state.race.riders[1].finished && clock_expired;
    for (const auto& lap : state.race.riders) {
        refuse_unless(
            !((lap.finished ? lap.laps_remaining != 0 && !timed_out : lap.laps_remaining == 0)
              || lap.time_digits[0] > 9 || lap.time_digits[1] > 5 || lap.time_digits[2] > 9
              || lap.time_digits[3] > 9 || lap.time_digits[4] > 9),
            "ZOOM ZOO lap time state invalid");
    }
}

void read_race_progress(Reader& in, ZoomZooState& state, const ClassicRaceScenario& scenario) {
    auto& race = state.race;
    read_race_riders(in, race, scenario);
    for (auto& times : race.lap_times)
        for (auto& v : times) v = in.u16();
    for (auto& v : race.total_times) v = in.u16();
    race.provisional_1225 = in.u16();
    race.provisional_1227 = in.u16();
    race.finish_delay = in.u16();
    auto& c = race.camera;
    for (auto* v : {&c.x, &c.y, &c.velocity_x, &c.velocity_y, &c.lookahead, &c.screen_xy})
        *v = in.u16();
    for (unsigned i = 0; i < shared_checkpoint_flags; ++i) race.checkpoint_seen[i] = in.u8();
    std::fill(race.checkpoint_seen.begin() + shared_checkpoint_flags, race.checkpoint_seen.end(),
              checkpoint_unseen);
    for (auto& p : race.finish_pose)
        for (auto* v : {&p.selector, &p.kind, &p.locked, &p.active}) *v = in.u16();
    refuse_unless(!(race.finish_delay > finish_display_updates
                    || (race.finish_delay && !race.riders[0].finished) || race.provisional_1225 > 1
                    || race.provisional_1227 > 1
                    || (state.track == ClassicRaceTrack::ZoomZoo && c.x > zoom_zoo_camera_x_limit)
                    || std::abs(static_cast<std::int16_t>(c.velocity_x)) > fastest_camera
                    || std::abs(static_cast<std::int16_t>(c.velocity_y)) > fastest_camera),
                  "ZOOM ZOO finish/camera state invalid");
    for (auto seen : race.checkpoint_seen)
        refuse_unless(seen == 0 || seen == checkpoint_unseen,
                      "ZOOM ZOO checkpoint seen flag invalid");
    check_finish_poses(race);
    check_lap_clocks(state);
}

void read_result_and_charges(Reader& in, ZoomZooState& state) {
    state.fade_level = in.u16();
    for (auto& v : state.start_boost) v = in.u16();
    state.result_updates = in.u16();
    state.result.graph_minimum = in.u16();
    state.result.graph_maximum = in.u16();
    for (auto& v : state.result.published_totals) v = in.u16();
    for (auto& v : state.charge_announced) {
        v = in.u16();
        refuse_unless(v <= 1, "invalid ZOOM ZOO charge flag");
    }
}

void read_player_announcements(Reader& in, ZoomZooPlayerAnnouncements& a) {
    auto& q = a.queue;
    for (auto& v : q.entries) v = in.u8();
    q.read_cursor = in.u8();
    q.write_cursor = in.u8();
    q.cooldown = in.u16();
    q.feature_total = in.u16();
    q.event_one_weight = in.u8();
    a.hints_active = in.u16();
    a.hint_updates = in.u16();
    a.hint_group = in.u16();
    a.empty_display = in.u16();
    refuse_unless(q.read_cursor <= last_queue_slot && q.write_cursor <= last_queue_slot
                      && q.cooldown <= longest_player_cooldown
                      && reachable_event_one_weight(q.event_one_weight) && a.hints_active <= 1
                      && a.hint_updates < hint_interval && a.hint_group < hint_groups
                      && a.empty_display <= 1,
                  "invalid ZOOM ZOO player announcement state");
}

// The opponent's queue fields, stored in the movement prefix, are bounded like the
// player's: $82:DB87-DB94 seeds both banks from one template and $81:C27B-C280 only halves
// toward 1. Its cooldown tops out at 40, with no tutorial. $83:E1F1 masks the sloped trick
// selector with 7 and the flat path writes only 0 or 1; a forged 14 would alias onto 6.
void check_opponent_queue(const ZoomZooState& state) {
    const auto& o = state.movement.rewards;
    refuse_unless(o.cooldown <= longest_opponent_cooldown
                      && reachable_event_one_weight(o.event_one_weight),
                  "invalid ZOOM ZOO opponent reward queue state");
    refuse_unless(state.movement.opponent_ai.trick_selector <= highest_trick_selector,
                  "invalid ZOOM ZOO opponent trick selector");
}

void check_start_and_result(const ZoomZooState& state, const ClassicRaceScenario& scenario) {
    refuse_unless(!(state.movement.countdown
                    && (state.race.riders[0].finished || state.race.riders[1].finished
                        || state.result_updates)),
                  "ZOOM ZOO finish/result conflicts with start phase");
    refuse_unless(state.result
                      == result_fields(state.race, state.result_updates, scenario.tour_race),
                  "inconsistent ZOOM ZOO result publication");
}

bool roll_is_clear(const ZoomZooRoll& r) {
    return !(r.input_latched || r.prior_orientation || r.prior_reflection || r.pose_base || r.step
             || r.held_updates || r.bounce_charge || r.completed_rolls || r.held_rotations
             || r.bounce_active || r.support_count_mirror || r.prior_step);
}

void read_rolls(Reader& in, ZoomZooState& state, const ClassicRaceScenario& scenario) {
    for (unsigned roll_index = 0; roll_index < state.rolls.size(); ++roll_index) {
        auto& r = state.rolls[roll_index];
        for (auto* v :
             {&r.input_latched, &r.prior_orientation, &r.prior_reflection, &r.pose_base, &r.step,
              &r.held_updates, &r.bounce_charge, &r.completed_rolls, &r.held_rotations,
              &r.bounce_active, &r.support_count_mirror, &r.prior_step})
            *v = in.u16();
        const auto step = static_cast<std::int16_t>(r.step);
        refuse_unless(!(r.input_latched > 1 || r.prior_orientation > 63 || r.prior_reflection > 1
                        || step < -spin_steps || step > spin_steps
                        || (r.bounce_charge != 0 && r.bounce_charge != bounce_charge)
                        || r.bounce_active > 1 || (r.bounce_charge && r.step) || r.prior_step
                        || (r.pose_base & roll_pose_unused_bit)),
                      "invalid ZOOM ZOO roll state");
        // $82:9641-9649 and $82:965E-9666 store a bounce charge ($1007) only for the player
        // while $0C6D is nonzero, which the reference guard holds at 1: the opponent never
        // charges.
        refuse_unless(!(roll_index == 1 && (r.bounce_charge || r.bounce_active)),
                      "invalid ZOOM ZOO opponent bounce state");
        refuse_unless(state.movement.frame != scenario.initialization_frame || roll_is_clear(r),
                      "inconsistent initial ZOOM ZOO roll state");
    }
}

void read_learned_weights(Reader& in, ZoomZooState& state) {
    for (auto& weights : state.learned_weights)
        for (auto& v : weights) {
            v = in.u8();
            refuse_unless(v <= heaviest_learned_weight, "invalid ZOOM ZOO learned reward weight");
        }
}

void read_pause(Reader& in, ZoomZooState& state, const ClassicRaceScenario& scenario) {
    auto& pause = state.pause;
    pause.selection = in.u16();
    pause.released = in.u16();
    pause.suspended_updates = in.u32();
    pause.suspended_countdown_updates = in.u32();
    refuse_unless(
        !((pause.selection != 0 && pause.selection != 1 && pause.selection != 0xffff)
          || pause.released > 1 || state.movement.frame < scenario.initialization_frame
          || pause.suspended_updates > state.movement.frame - scenario.initialization_frame
          || pause.suspended_countdown_updates > pause.suspended_updates
          || ((pause.selection || pause.released) && !pause.suspended_updates)),
        "invalid ZOOM ZOO pause state");
}

// $82:95D5-95F6 publishes the reflection flag and the pose base's bit 15 together on every
// spin pose. Each held or completed counter advances at most once a update, so none can
// exceed the elapsed updates before a word wraps. The hold is not bounded by the held
// rotations: a landing's reward pass ($82:9B69-9D97) clears the rotations while a
// released hold keeps counting across the bounce (DRAGSTER fuzz seeds 31 at 1623 and 383
// at 3472; R-0038).
void check_rolls(const ZoomZooState& state, const ClassicRaceScenario& scenario) {
    for (unsigned i = 0; i < 2; ++i) {
        const auto& roll = state.rolls[i];
        refuse_unless(
            !(roll.step
              && bool(roll.pose_base & roll_pose_mirror)
                     != (state.movement.riders[i].pose.reflected != (roll.prior_reflection != 0))),
            "inconsistent ZOOM ZOO active roll reflection");
        const auto elapsed = state.movement.frame >= scenario.initialization_frame
                               ? state.movement.frame - scenario.initialization_frame
                               : 0U;
        if (elapsed < 65536U) {
            const auto held = static_cast<std::int16_t>(roll.held_updates);
            const auto magnitude = static_cast<unsigned>(held < 0 ? -static_cast<int>(held) : held);
            refuse_unless(!(magnitude > elapsed || roll.held_rotations > elapsed
                            || roll.completed_rolls > elapsed),
                          "inconsistent ZOOM ZOO held roll counters");
        }
    }
    for (unsigned i = 0; i < 2; ++i)
        refuse_unless(state.rolls[i].support_count_mirror
                          == state.movement.riders[i].contact.unsupported_count,
                      "inconsistent ZOOM ZOO support-count mirror");
}

void check_result_and_start_fields(const ZoomZooState& state, const ClassicRaceScenario& scenario) {
    refuse_unless(
        !(state.result_updates > std::max(scenario.stable_result_won, scenario.stable_result_lost)
          || (state.result_updates && state.race.finish_delay != finish_display_updates)),
        "invalid ZOOM ZOO result phase");
    for (auto v : state.start_boost)
        refuse_unless(v == 0 || v == start_boost, "invalid ZOOM ZOO start boost");
    refuse_unless(state.fade_level <= brightest_fade, "invalid ZOOM ZOO fade level");
    refuse_unless(state.movement.frame >= scenario.initialization_frame,
                  "invalid ZOOM ZOO initialization frame");
}

// At the start the player's queue holds only the first hint phase (30 of 300) and a
// write cursor of 1; later, while the hints run, their phase follows the updates elapsed
// outside the pause.
void check_hint_timeline(const ZoomZooState& state, unsigned elapsed, bool tutorial_hints) {
    const auto& a = state.player_announcements;
    const auto& q = a.queue;
    refuse_unless(!(elapsed == 0
                    && (q.read_cursor || q.write_cursor != 1 || q.cooldown || q.feature_total
                        || q.event_one_weight != 4 || a.hints_active != (tutorial_hints ? 1 : 0)
                        || a.hint_updates != first_hint_updates || a.hint_group || a.empty_display
                        || std::any_of(q.entries.begin(), q.entries.end(),
                                       [](auto event) { return event != 0; }))),
                  "inconsistent initial ZOOM ZOO announcements");
    const auto hint_clock = elapsed - state.pause.suspended_updates + first_hint_updates;
    refuse_unless(!(!state.result_updates && a.hints_active
                    && (a.hint_updates != hint_clock % hint_interval
                        || a.hint_group != (hint_clock / hint_interval) % hint_groups)),
                  "inconsistent ZOOM ZOO hint phase");
}

// $82:9D47-9D5B produces the riders' voices: each rider's are the sixteen of its character
// (MIKE's 72-87; BRONSEN's 200-215, or ANTI-UNI's 232-247 on the HUNTER tour, R-0052). Admitting
// only these keeps update_opponent_announcements' other refusals unreachable and its
// learned-bank guards sufficient; widening the range means widening those guards. A queue
// never holds a zero between its cursors ($81:C598-C5C8 never queues one).
void check_queued_events(const ZoomZooState& state, const ClassicRaceScenario& scenario) {
    const auto own_voice = [](unsigned event, unsigned character) {
        const auto first = announcement::first_voice_of(character);
        return event < announcement::first_voice
            || (event >= first && event < first + announcement::voices_per_character_pair);
    };
    const auto& q = state.player_announcements.queue;
    for (auto event : q.entries)
        refuse_unless(own_voice(event, scenario.pairing.rider),
                      "invalid ZOOM ZOO player voice event");
    for (auto event : state.movement.rewards.entries)
        refuse_unless(own_voice(event, scenario.pairing.opponent),
                      "invalid ZOOM ZOO opponent voice event");
    for (unsigned cursor = (q.read_cursor + 1U) & last_queue_slot; cursor != q.write_cursor;
         cursor = (cursor + 1U) & last_queue_slot)
        refuse_unless(q.entries[cursor] != 0, "empty pending ZOOM ZOO announcement");
    const auto& o = state.movement.rewards;
    for (unsigned cursor = (o.read_cursor + 1U) & last_queue_slot; cursor != o.write_cursor;
         cursor = (cursor + 1U) & last_queue_slot)
        refuse_unless(o.entries[cursor] != 0, "empty pending ZOOM ZOO opponent reward");
}

// The countdown starts 4 updates after the fade and runs 270 updates, minus those paused;
// the start boosts stay whole until it passes 128.
void check_countdown(const ZoomZooState& state, unsigned elapsed) {
    refuse_unless(!(elapsed == 0 && (state.charge_announced[0] || state.charge_announced[1])),
                  "initial charge flag must be clear");
    const auto running = elapsed > countdown_delay ? elapsed - countdown_delay : 0U;
    refuse_unless(state.pause.suspended_countdown_updates <= running,
                  "invalid ZOOM ZOO suspended countdown clock");
    const auto decrements =
        elapsed > countdown_delay
            ? std::min(countdown_updates, running - state.pause.suspended_countdown_updates)
            : 0U;
    refuse_unless(state.fade_level == std::min<unsigned>(brightest_fade, elapsed)
                      && state.movement.countdown == countdown_updates - decrements,
                  "inconsistent ZOOM ZOO countdown/fade phase");
    refuse_unless(
        !(state.movement.countdown >= last_start_boost_countdown
          && (state.start_boost[0] != start_boost || state.start_boost[1] != start_boost)),
        "premature ZOOM ZOO start boost consumption");
}

// Completed laps have times and the others none; a rider finished on laps totals them, one
// finished by the clock limit keeps the no-time sentinel.
void check_lap_times(const ZoomZooState& state, const ClassicRaceScenario& scenario) {
    for (unsigned i = 0; i < 2; ++i) {
        const auto& rider = state.race.riders[i];
        const auto completed = unsigned(scenario.laps)
                             - std::min(unsigned(scenario.laps), unsigned(rider.laps_remaining));
        std::uint16_t sum = 0;
        for (unsigned slot = 0; slot < lap_slots; ++slot) {
            const auto lap = state.race.lap_times[i][slot];
            refuse_unless((slot < completed) != (lap == no_time),
                          "inconsistent ZOOM ZOO completed lap slots");
            if (slot < completed) sum = add_word(sum, lap);
        }
        refuse_unless(state.race.total_times[i]
                          == (rider.finished && !rider.laps_remaining ? sum : no_time),
                      "inconsistent ZOOM ZOO total time");
    }
}

void read_native_race(Reader& in, ZoomZooState& state, const ClassicRaceScenario& scenario) {
    read_result_and_charges(in, state);
    read_player_announcements(in, state.player_announcements);
    check_opponent_queue(state);
    check_start_and_result(state, scenario);
    read_rolls(in, state, scenario);
    read_learned_weights(in, state);
    read_pause(in, state, scenario);
    check_rolls(state, scenario);
    check_result_and_start_fields(state, scenario);
    const auto elapsed = state.movement.frame - scenario.initialization_frame;
    check_hint_timeline(state, elapsed, scenario.tutorial_hints);
    check_queued_events(state, scenario);
    check_countdown(state, elapsed);
    check_lap_times(state, scenario);
}

// $1277 holds what the opponent's last launch decision left ($83:E16B-E1C8): 0 or 30 below
// level 2, 0 or 60 above it, and at level 2 the player's lead + 15, never negative. A state read
// without its pairing is BRONSEN's (or ANTI-UNI's), so SILVIA's words are refused there.
void check_opponent_suppression(const ZoomZooState& state) {
    constexpr std::uint16_t lead_weighing_level = 2, low_level_word = 30, high_level_word = 60;
    const auto word = state.movement.opponent_ai.suppression_counter;
    const auto level = state.opponent_tier.ai_level;
    const bool possible =
        level == lead_weighing_level
            ? !negative(word)
            : word == 0 || word == (level < lead_weighing_level ? low_level_word : high_level_word);
    refuse_unless(possible, "the opponent's suppression word does not fit its AI level");
}

void check_controls_and_horizon(const ZoomZooState& state, const ClassicRaceScenario& scenario) {
    for (const auto& surface : state.surface)
        refuse_unless(!(surface.mode > 1 || surface.tile_mode > 1 || surface.leading_support > 1
                        || surface.tile_pose > 1 || surface.tile_pose_enabled > 1),
                      "ZOOM ZOO surface flags are invalid");
    const auto first =
        state.native_initialization ? scenario.initialization_frame : trial_first_frame;
    const auto last = state.sustained ? sustained_last_frame : trial_last_frame;
    refuse_unless(!(state.movement.frame < first
                    || (!state.native_initialization && state.movement.frame > last)),
                  "ZOOM ZOO state is outside trial horizon");
    for (const auto& r : state.reflection)
        refuse_unless(!(r.step > 16 || (r.end != 0 && r.end != 9 && r.end != 16) || r.completed > 1
                        || r.drive_pose_enabled > 1 || r.brake_input > 1
                        || r.rotate_negative_input > 1 || r.rotate_positive_input > 1
                        || r.jump_input > 1),
                      "ZOOM ZOO reflection/control state is invalid");
}

// Reads the shared layouts (URZZ0001 to URZZ000B) as a race on `track`.
ZoomZooState deserialize_classic_race(std::span<const std::uint8_t> bytes, ClassicRaceTrack track,
                                      std::optional<RacePairing> pairing, bool tutorial_hints,
                                      std::span<const std::uint8_t> opponent_catch_up) {
    const auto scenario = pairing ? classic_race_scenario(track, *pairing, tutorial_hints)
                                  : classic_race_scenario(track);
    const auto magic = race_state_magic;
    // Read only once the size is known to hold the identity.
    const auto family = [&] { return std::equal(magic.begin(), magic.begin() + 7, bytes.begin()); };
    const bool native_initialization =
        bytes.size() == native_race_size && bytes[7] == native_race_layout;
    const bool complete_race =
        ((bytes.size() == complete_race_size && bytes[7] == complete_race_layout)
         || native_initialization)
        && family();
    const bool sustained =
        complete_race
        || (bytes.size() == sustained_size && bytes[7] == sustained_layout && family());
    refuse_unless(sustained
                      || (bytes.size() == first_layout_size
                          && std::equal(magic.begin(), magic.end(), bytes.begin())),
                  "ZOOM ZOO state identity/width differs");
    std::vector<std::uint8_t> prefix(bytes.begin(), bytes.begin() + movement_prefix_size);
    std::copy(movement_state_magic.begin(), movement_state_magic.end(), prefix.begin());
    ZoomZooState state;
    state.track = track;
    state.pairing = scenario.pairing;
    state.opponent_tier = opponent_tier(scenario, opponent_catch_up);
    state.native_initialization = native_initialization;
    state.complete_race = complete_race;
    state.sustained = sustained;
    state.movement = deserialize_movement_state(prefix);
    Reader in{bytes.subspan(movement_prefix_size)};
    for (auto& r : state.reflection) read_reflection(in, r);
    state.opponent_horizontal = in.u8();
    state.opponent_retained_oam_x = in.u8();
    refuse_unless(state.opponent_horizontal <= 2, "ZOOM ZOO horizontal input is invalid");
    if (sustained) read_surfaces(in, state);
    if (complete_race) read_race_progress(in, state, scenario);
    if (native_initialization) read_native_race(in, state, scenario);
    check_controls_and_horizon(state, scenario);
    check_opponent_suppression(state);
    in.require_end();
    return state;
}

void read_special_tiles(Reader& in, ZoomZooState& state) {
    for (auto& r : state.special_tiles) {
        for (auto* v : {&r.mud_cooldown, &r.mud_exit_pending, &r.corkscrew_latch, &r.corkscrew_step,
                        &r.corkscrew_float, &r.physics_hold, &r.reflection_lock, &r.raised_priority,
                        &r.loop_direction, &r.loop_step, &r.loop_cooldown, &r.slow_counter})
            *v = in.u16();
        refuse_unless(!(r.mud_cooldown > 4 || r.mud_exit_pending > 4 || r.corkscrew_float > 1
                        || r.physics_hold > 8 || r.reflection_lock > 6 || r.raised_priority > 1
                        || (r.corkscrew_step > 0x31 && r.corkscrew_step != 0xffff)
                        || !(r.corkscrew_latch <= 1 || r.corkscrew_latch >= 0xfffc)
                        || r.loop_direction > 1 || (r.loop_step > 0x10 && r.loop_step < 0xfffe)
                        || r.loop_cooldown > 3 || r.slow_counter > 7),
                      "classic race special-tile state is invalid");
    }
    state.drive_target_latch = in.u16();
    refuse_unless(state.drive_target_latch <= 1, "classic race drive target latch is invalid");
    state.opponent_turnaround = in.u16();
    refuse_unless(state.opponent_turnaround <= 30, "classic race opponent turnaround is invalid");
    in.require_end();
}

void read_hunter_effects(Reader& in, HunterEffects& h, ClassicRaceTrack track) {
    h.latched = in.u16();
    h.active = in.u16();
    for (auto& v : h.effect) v = in.u16();
    for (auto& v : h.timer) v = in.u16();
    for (auto* v : {&h.pulse, &h.pulse_shrinking, &h.pulse_length, &h.blink, &h.wave_phase,
                    &h.hide_track, &h.mosaic, &h.skip_update, &h.message, &h.shown, &h.hud_event,
                    &h.caption, &h.mosaic_counter})
        *v = in.u16();
    in.require_end();
    // A HUNTER message is an effect's announcement or its end (27-36); the HUD never holds
    // the end's blank caption.
    const auto valid_event = [](std::uint16_t e) { return !e || (e >= 0x1b && e <= 0x24); };
    const bool message_valid = valid_event(h.message) && valid_event(h.hud_event)
                            && h.hud_event != 0x23 && h.shown <= 1 && h.caption <= 255
                            && h.mosaic_counter <= 255;
    refuse_unless(!(h.latched > 1 || h.active > 1 || h.pulse > 20 || h.pulse_shrinking > 1
                    || h.pulse_length > 20 || h.blink > 1 || h.wave_phase > 1 || h.hide_track > 1
                    || h.mosaic > 1 || h.skip_update > 1 || !message_valid || h.timer[1]
                    || std::any_of(h.effect.begin(), h.effect.end(), [](auto v) { return v > 2; })
                    || std::any_of(h.timer.begin(), h.timer.end(), [](auto v) { return v >= 500; })
                    || (!classic_race_scenario(track).hunter_tour && h != HunterEffects{})),
                  "classic race HUNTER effect state is invalid");
}

// The track and layout a state's identity names. A 742-byte state with no other identity
// is ZOOM ZOO's (URZZ); none means the shared layouts, read as ZOOM ZOO.
std::optional<ClassicRaceTrack> identified_track(std::span<const std::uint8_t> bytes) {
    const auto magic_is = [&](std::string_view text) {
        return std::equal(text.begin(), text.end(), bytes.begin());
    };
    if (bytes.size() == native_race_size)
        return std::equal(dragster_race_state_magic.begin(), dragster_race_state_magic.end(),
                          bytes.begin())
                 ? std::optional{ClassicRaceTrack::Dragster}
                 : std::nullopt;
    if (bytes.size() == extended_size) {
        if (magic_is("URZZ000E")) return ClassicRaceTrack::ZoomZoo;
        if (magic_is("URDG0004")) return ClassicRaceTrack::Dragster;
        throw std::invalid_argument("classic race state identity/width differs");
    }
    if (bytes.size() == other_track_size) {
        refuse_unless(magic_is("URTR") && bytes[4] >= '0' && bytes[4] <= '9' && bytes[5] >= '0'
                          && bytes[5] <= '9' && bytes[6] == '0' && bytes[7] == '6',
                      "classic race state identity/width differs");
        const ClassicRaceTrack other{
            static_cast<std::uint8_t>((bytes[4] - '0') * 10 + (bytes[5] - '0'))};
        refuse_unless(is_other_track(other) && classic_race_has_scenario(other),
                      "classic race state names a track without its own identity");
        return other;
    }
    return std::nullopt;
}

} // namespace

std::array<std::uint8_t, 8> classic_race_state_magic(ClassicRaceTrack track) {
    if (track == ClassicRaceTrack::Dragster) return dragster_race_state_magic;
    if (track == ClassicRaceTrack::ZoomZoo) return {'U', 'R', 'Z', 'Z', '0', '0', '0', 'B'};
    return {'U',
            'R',
            'T',
            'R',
            static_cast<std::uint8_t>('0' + track.index / 10U),
            static_cast<std::uint8_t>('0' + track.index % 10U),
            '0',
            '6'};
}

std::vector<std::uint8_t> serialize_zoom_zoo(const ZoomZooState& state) {
    refuse_unless(!state.complete_race || state.sustained, "race state requires sustained prefix");
    auto bytes = serialize_movement_state(state.movement);
    refuse_unless(bytes.size() == movement_prefix_size, "ZOOM ZOO finish state is unsupported");
    std::copy(race_state_magic.begin(), race_state_magic.end(), bytes.begin());
    for (const auto& r : state.reflection) write_reflection(bytes, r);
    put8(bytes, state.opponent_horizontal);
    put8(bytes, state.opponent_retained_oam_x);
    if (state.sustained) {
        bytes[7] = sustained_layout;
        write_surfaces(bytes, state);
    }
    if (state.complete_race) {
        bytes[7] = state.native_initialization ? native_race_layout : complete_race_layout;
        write_race_progress(bytes, state.race);
    }
    if (state.native_initialization) write_native_race(bytes, state);
    // No accepted DRAGSTER or ZOOM ZOO race reaches a special tile, so their frozen 742-byte
    // states are unchanged; ZOOM ZOO's own tile table does hold the corkscrew (pair 10).
    const bool other_track = is_other_track(state.track);
    const bool special_tiles_live =
        state.special_tiles != std::array<SpecialTileRider, 2>{} || state.opponent_turnaround;
    if (other_track || special_tiles_live) {
        refuse_unless(state.native_initialization,
                      "special-tile words require a natively initialized race");
        write_special_tiles(bytes, state);
        if (state.track == ClassicRaceTrack::ZoomZoo) bytes[7] = extended_zoom_zoo_layout;
    }
    if (other_track) {
        for (unsigned i = shared_checkpoint_flags; i < checkpoint_flags; ++i)
            put8(bytes, state.race.checkpoint_seen[i]);
        write_hunter_effects(bytes, state.hunter);
    } else {
        refuse_unless(state.hunter == HunterEffects{},
                      "HUNTER effects run only on the HUNTER tour");
    }
    if (state.track != ClassicRaceTrack::ZoomZoo) {
        // Another track on the shared engine: the URZZ000B layout under its own identity, so
        // a restore can never run one track's state on another.
        refuse_unless(state.native_initialization,
                      "a race state of any track but ZOOM ZOO requires native initialization");
        const auto identity = classic_race_state_magic(state.track);
        std::copy(identity.begin(), identity.end(), bytes.begin());
        if (state.track == ClassicRaceTrack::Dragster && special_tiles_live)
            bytes[7] = extended_dragster_layout;
    }
    return bytes;
}

namespace {

ZoomZooState deserialize_race(std::span<const std::uint8_t> bytes,
                              std::optional<RacePairing> pairing, bool tutorial_hints,
                              std::span<const std::uint8_t> opponent_catch_up) {
    const auto track = identified_track(bytes);
    if (!track)
        return deserialize_classic_race(bytes, ClassicRaceTrack::ZoomZoo, pairing, tutorial_hints,
                                        opponent_catch_up);
    std::vector<std::uint8_t> shared(bytes.begin(), bytes.begin() + native_race_size);
    const auto zoom_zoo_magic = classic_race_state_magic(ClassicRaceTrack::ZoomZoo);
    std::copy(zoom_zoo_magic.begin(), zoom_zoo_magic.end(), shared.begin());
    auto state =
        deserialize_classic_race(shared, *track, pairing, tutorial_hints, opponent_catch_up);
    if (bytes.size() == native_race_size) return state;
    Reader tiles{bytes.subspan(native_race_size, special_tiles_size)};
    read_special_tiles(tiles, state);
    if (bytes.size() == other_track_size) {
        const auto more_flags = native_race_size + special_tiles_size;
        std::copy(bytes.begin() + more_flags,
                  bytes.begin() + more_flags + (checkpoint_flags - shared_checkpoint_flags),
                  state.race.checkpoint_seen.begin() + shared_checkpoint_flags);
        Reader hunter{bytes.subspan(other_track_size - hunter_effects_size, hunter_effects_size)};
        read_hunter_effects(hunter, state.hunter, *track);
        for (auto seen : state.race.checkpoint_seen)
            refuse_unless(seen == 0 || seen == checkpoint_unseen,
                          "classic race checkpoint-seen flag is invalid");
    }
    // DRAGSTER and ZOOM ZOO take the extended layout only while a word is live.
    refuse_unless(is_other_track(*track) || state.special_tiles != std::array<SpecialTileRider, 2>{}
                      || state.opponent_turnaround,
                  "an extended DRAGSTER or ZOOM ZOO state carries no special-tile word");
    return state;
}
} // namespace

ZoomZooState deserialize_zoom_zoo(std::span<const std::uint8_t> bytes) {
    return deserialize_race(bytes, std::nullopt, true, {});
}

ZoomZooState deserialize_zoom_zoo(std::span<const std::uint8_t> bytes, RacePairing pairing,
                                  bool tutorial_hints,
                                  std::span<const std::uint8_t> opponent_catch_up) {
    return deserialize_race(bytes, pairing, tutorial_hints, opponent_catch_up);
}

void validate_zoom_zoo_content_state(const ZoomZooState& state, const ZoomZooContent& content) {
    if (!state.native_initialization) return;
    refuse_unless(content.reward_weights.size() == 26, "ZOOM ZOO reward weights missing");
    for (unsigned i = 0; i < 2; ++i) {
        const auto& roll = state.rolls[i];
        // The low pose base is latched once from the entry orientation at $82:93E5-941D and
        // kept after the spin. Bit 15 is checked with the state.
        refuse_unless(!((roll.step || roll.pose_base)
                        && ((roll.pose_base & roll_pose_bits)
                            != (content_word(content.roll_poses, 2U * roll.prior_orientation)
                                & roll_pose_bits))),
                      "ZOOM ZOO roll base differs from static entry pose");
        refuse_unless(
            state.movement.frame != classic_race_scenario(state.track).initialization_frame
                || std::equal(state.learned_weights[i].begin(), state.learned_weights[i].end(),
                              content.reward_weights.begin() + 1),
            "ZOOM ZOO initial reward weights differ from static content");
    }
}

} // namespace unirally
