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
//   stunt event     the qualifying score, the clock's stop, the finish's settling and the
//                   trick tallies (R-0066;
//                   the stunt events: URTRnn07, 1,006 bytes)
//
// A two-view state appends the split trailer, then in a VS race both banner drivers (R-0081),
// and one more byte (1) only while pad 2's pause menu is open in the lower view
// (`pause.lower_view`, R-0079); a league pair's wrapper appends that byte too.
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
// A two-view state appends the second camera, two demo controllers, and the
// pairing/tier which a one-player restore normally derives from its menus.
constexpr std::size_t split_trailer_size = 42;
constexpr std::size_t split_race_size = native_race_size + split_trailer_size;
constexpr std::size_t extended_split_size = extended_size + split_trailer_size;
constexpr std::size_t special_tiles_size = 52, hunter_effects_size = 62;
constexpr std::size_t stunt_event_size = 90, stunt_track_size = other_track_size + stunt_event_size;
constexpr std::size_t one_view_demo_size = other_track_size + split_trailer_size;
// The byte a two-pad state appends while pad 2's pause menu is open (`pause.lower_view`).
constexpr std::size_t lower_view_size = 1;
// Flags a two-view state's trailer or a league wrapper holds that the base layout's checks need:
// a VS race (R-0081) and rider 1's tutorial hints (R-0082).
struct TrailerFlags {
    bool versus{}, opponent_hints{};
};
// A VS race's two banner drivers, index and life each.
constexpr std::size_t versus_block_size = 8;
constexpr std::uint16_t longest_banner_life = banner_life_updates - 1;
constexpr std::array<std::uint8_t, 8> race_state_magic{'U', 'R', 'Z', 'Z', '0', '0', '0', '1'};
// The layout letter in byte 7 of the identity.
constexpr std::uint8_t sustained_layout = '2', complete_race_layout = '3', native_race_layout = 'B';
constexpr std::uint8_t extended_zoom_zoo_layout = 'E', extended_dragster_layout = '4';
constexpr std::uint8_t split_layout = 'F', extended_split_layout = 'G';
constexpr std::uint8_t local_dragster_split_layout = 'H',
                       extended_local_dragster_split_layout = 'I';
constexpr std::uint8_t one_view_demo_layout = '8';
// DRAGSTER's one lap and ZOOM ZOO's three reach only the first 20 of the 80 checkpoint flags.
constexpr unsigned shared_checkpoint_flags = 20, checkpoint_flags = 80;
constexpr std::uint8_t checkpoint_unseen = 255;

// Race limits the guards check.
constexpr std::uint16_t checkpoints_per_lap = 4, longest_checkpoint_display = 120;
constexpr std::uint16_t finish_display_updates = 240, fastest_camera = 16;
constexpr std::uint16_t zoom_zoo_camera_x_limit = 0x3fff;
// R-0070: track 3's one-view demo holds the unused second position and OAM.
constexpr std::uint16_t one_view_camera_x = 0x0530, one_view_camera_y = 0x0120;
constexpr std::uint16_t one_view_camera_oam = 0x2065;
constexpr RacePairing one_view_demo_pairing{6, 1};
constexpr OpponentTier one_view_demo_opponent_tier{0xf1, 0, 0x60};
constexpr unsigned last_finish_pose_one = 48, last_finish_pose_two = 88;
constexpr unsigned lap_slots = 10;
// The native start: 30 fade updates, a 4-update delay, a 270-update countdown, start boosts
// of 384 until it reaches 128; the tutorial hints every 300 updates from 30, eight groups.
constexpr std::uint16_t brightest_fade = 30, start_boost = 384, last_start_boost_countdown = 129;
// A stunt event's countdown drops the start boosts from 160 (R-0066), so they stay whole while
// it is 160 or more after an update.
constexpr std::uint16_t last_stunt_start_boost_countdown = 160;
// A stunt event's clock stops on the tick after 0:00.0 with its digits at 0:59.9 ($81:C830).
constexpr RaceTimerDigits stopped_stunt_clock{0, 5, 9, 9, 0};
// A settled rider's vertical velocity in a stunt event ($83:E856, $83:E874).
constexpr std::uint16_t stunt_settled_fall = 0x80;
// The track header's opponent start, x then y in 16-unit cells (bytes 7-10).
constexpr unsigned opponent_start_x = 7, opponent_start_y = 9;
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

void write_split_trailer(std::vector<std::uint8_t>& bytes, const ZoomZooState& state) {
    const auto& camera = state.race.second_camera;
    for (auto value : {camera.x, camera.y, camera.velocity_x, camera.velocity_y, camera.lookahead,
                       camera.screen_xy})
        put16(bytes, value);
    for (const auto& words : {state.demo.trick_bits, state.demo.rotation_window,
                              state.demo.turnaround, state.demo.airborne_rotation})
        for (auto value : words) put16(bytes, value);
    put16(bytes, state.demo.elapsed);
    put8(bytes, state.demo.exit_requested);
    put8(bytes, state.opponent_hints.active);
    put8(bytes, state.pairing.rider);
    put8(bytes, state.pairing.opponent);
    for (auto value : {state.opponent_tier.ai_level, state.opponent_tier.catch_up,
                       state.opponent_tier.adjustment_limit})
        put16(bytes, value);
    put8(bytes, state.split_screen);
    put8(bytes, state.demo_ai);
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

// Each tally column is the original's four bytes: the count, a zero byte, the points word.
void write_stunt_event(std::vector<std::uint8_t>& bytes, const StuntEvent& stunt) {
    put16(bytes, stunt.qualifying_score);
    put16(bytes, stunt.clock_stopped);
    put16(bytes, stunt.finish_display);
    for (auto v : stunt.settled) put16(bytes, v);
    for (const auto& family : stunt.tallies)
        for (const auto& tally : family) {
            put8(bytes, tally.shown);
            put8(bytes, 0);
            put16(bytes, tally.points);
        }
}

bool is_other_track(ClassicRaceTrack track) {
    return track != ClassicRaceTrack::ZoomZoo && track != ClassicRaceTrack::Dragster;
}

bool is_stunt_track(ClassicRaceTrack track) {
    return classic_race_has_scenario(track) && classic_race_scenario(track).stunt_event;
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

void read_race_riders(Reader& in, ZoomZooState& state, const ClassicRaceScenario& scenario) {
    for (auto& r : state.race.riders) {
        for (auto* v : {&r.laps_remaining, &r.checkpoint, &r.next_checkpoint, &r.start_line_latch,
                        &r.checkpoint_display_countdown, &r.finished})
            *v = in.u16();
        for (auto& v : r.time_digits) v = in.u16();
        refuse_unless(r.laps_remaining <= scenario.laps + 1U && r.checkpoint < checkpoints_per_lap
                          && r.next_checkpoint < checkpoints_per_lap && r.start_line_latch <= 1
                          && (r.finished <= 1 || (state.versus && r.finished == forced_finish))
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

// $81:C73E-C75B finishes both riders, laps or not, once the clock holds 9:59.9. A stunt
// event's riders finish by its clock with their lap left; read_stunt_event checks that the
// clock has stopped.
void check_lap_clocks(const ZoomZooState& state, const ClassicRaceScenario& scenario) {
    const auto& clock = state.movement.timer;
    const bool clock_expired = state.native_initialization && clock.minutes == 9
                            && clock.tens_seconds == 5 && clock.seconds == 9 && clock.tenths == 9;
    const bool timed_out =
        scenario.stunt_event
        || (state.race.riders[0].finished && state.race.riders[1].finished && clock_expired);
    for (const auto& lap : state.race.riders) {
        refuse_unless(
            !((lap.finished ? lap.laps_remaining != 0 && !timed_out && lap.finished != forced_finish
                            : lap.laps_remaining == 0)
              || lap.time_digits[0] > 9 || lap.time_digits[1] > 5 || lap.time_digits[2] > 9
              || lap.time_digits[3] > 9 || lap.time_digits[4] > 9),
            "ZOOM ZOO lap time state invalid");
    }
}

void read_race_progress(Reader& in, ZoomZooState& state, const ClassicRaceScenario& scenario) {
    auto& race = state.race;
    read_race_riders(in, state, scenario);
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
    check_lap_clocks(state, scenario);
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
// toward 1. Its cooldown tops out at 40, or 120 while rider 1's tutorial hints run (R-0082). $83:E1F1 masks the sloped trick
// selector with 7 and the flat path writes only 0 or 1; a forged 14 would alias onto 6.
void check_opponent_queue(const ZoomZooState& state) {
    const auto& o = state.movement.rewards;
    refuse_unless(o.cooldown <= (state.opponent_hints.active ? longest_player_cooldown
                                                             : longest_opponent_cooldown)
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
// the start boosts stay whole until it passes 128 (a stunt event's 159).
void check_countdown(const ZoomZooState& state, unsigned elapsed,
                     const ClassicRaceScenario& scenario) {
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
        !(state.movement.countdown >= (scenario.stunt_event ? last_stunt_start_boost_countdown
                                                            : last_start_boost_countdown)
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
    check_countdown(state, elapsed, scenario);
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
ZoomZooState deserialize_classic_race(
    std::span<const std::uint8_t> bytes, ClassicRaceTrack track, std::optional<RacePairing> pairing,
    bool tutorial_hints, std::span<const std::uint8_t> opponent_catch_up,
    std::optional<std::uint32_t> initialization_frame = std::nullopt, TrailerFlags flags = {}) {
    auto scenario = pairing && pairing->opponent < rider_characters
                      ? classic_local_race_scenario(track, *pairing, tutorial_hints)
                  : pairing ? classic_race_scenario(track, *pairing, tutorial_hints)
                            : classic_race_scenario(track);
    if (initialization_frame) {
        scenario.initialization_frame = *initialization_frame;
        if (track.index == 3) scenario.pairing = one_view_demo_pairing; // R-0070.
    }
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
    state.versus = flags.versus;
    state.opponent_hints.active = flags.opponent_hints;
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

// R-0066: the stunt event's words. A column's second byte stays 0 (its count is one byte); a
// stopped clock holds 0:59.9; a rider finishes only by the stopped clock, with its lap left; and
// the columns' points add up to the score, as every weight the queue pays goes to both
// ($81:C12A-C132, $81:C17A-C184).
void read_stunt_event(Reader& in, ZoomZooState& state) {
    auto& stunt = state.stunt;
    stunt.qualifying_score = in.u16();
    stunt.clock_stopped = in.u16();
    stunt.finish_display = in.u16();
    for (auto& v : stunt.settled) v = in.u16();
    std::uint16_t points = 0;
    for (auto& family : stunt.tallies)
        for (auto& tally : family) {
            tally.shown = in.u8();
            refuse_unless(in.u8() == 0, "a stunt tally's count is one byte");
            tally.points = in.u16();
            points = add_word(points, tally.points);
            // A trick is counted before it pays ($81:C111-C116, then $81:C173-C184). A count
            // wrapped past 255 (256 tricks of one kind in one run) is outside the domain.
            refuse_unless(tally.shown || !tally.points, "a stunt tally pays for no trick");
        }
    in.require_end();
    const auto& clock = state.movement.timer;
    const bool stopped_clock = clock.minutes == stopped_stunt_clock.minutes
                            && clock.tens_seconds == stopped_stunt_clock.tens_seconds
                            && clock.seconds == stopped_stunt_clock.seconds
                            && clock.tenths == stopped_stunt_clock.tenths
                            && clock.subframe == stopped_stunt_clock.subframe;
    refuse_unless(stunt.clock_stopped <= 1 && (!stunt.clock_stopped || stopped_clock),
                  "a stunt event's clock stops at its end");
    for (unsigned index = 0; index < 2; ++index) {
        const bool finished = state.race.riders[index].finished != 0;
        refuse_unless((!finished || stunt.clock_stopped) && stunt.settled[index] <= 1
                          && (!stunt.settled[index] || finished),
                      "a stunt event's rider finishes only when its clock stops, then settles");
    }
    // The display waits for both finishes; the finish poses and the count towards the result
    // wait for it.
    const auto& race = state.race;
    const bool posed = race.finish_pose[0].active || race.finish_pose[1].active;
    // It starts only on an update both stand, the update that settles them both.
    refuse_unless(stunt.finish_display <= 1
                      && (!stunt.finish_display
                          || (race.riders[0].finished && race.riders[1].finished && stunt.settled[0]
                              && stunt.settled[1]))
                      && (stunt.finish_display || (!race.finish_delay && !posed)),
                  "a stunt event's finish display is out of order");
    // The switched-off opponent never moves: no horizontal velocity, and a vertical one only
    // once it has settled, held at 128 ($83:E874).
    const auto& opponent = state.movement.riders[1].motion;
    refuse_unless(state.pairing.opponent < rider_characters
                      || (opponent.velocity_x == 0
                          && (opponent.velocity_y == 0
                              || (opponent.velocity_y == stunt_settled_fall && stunt.settled[1]))),
                  "a stunt event's opponent moves");
    refuse_unless(points == state.player_announcements.queue.feature_total,
                  "a stunt event's tallies do not add up to its score");
}

// A stunt event against its content: the qualifying score is one of its tour's three
// ($83:9EEB); a running clock reads no later than the header's start (it only counts down); the
// switched-off opponent stays at its start.
void check_stunt_content(const ZoomZooState& state, const ZoomZooContent& content) {
    bool of_tour = state.pairing.opponent < rider_characters && state.stunt.qualifying_score == 0;
    for (std::uint8_t medal = 0; medal < 3; ++medal)
        of_tour = of_tour
               || stunt_qualifying_score(content.qualifying_scores, state.track, medal)
                      == state.stunt.qualifying_score;
    refuse_unless(of_tour, "a stunt event's qualifying score is not its tour's");
    const auto& track = content.movement.sampling.track;
    refuse_unless(track.size() >= 11, "a stunt event's track header is missing");
    constexpr unsigned tenths_a_minute = 600, tenths_a_ten = 100, tenths_a_second = 10;
    const auto& clock = state.movement.timer;
    const unsigned shown = clock.minutes * tenths_a_minute + clock.tens_seconds * tenths_a_ten
                         + clock.seconds * tenths_a_second + clock.tenths;
    const unsigned start = track[1] * tenths_a_minute + track[2] * tenths_a_second; // $82:D7FD
    refuse_unless(state.stunt.clock_stopped || shown <= start,
                  "a stunt event's clock reads later than its start");
    const auto& opponent = state.movement.riders[1].motion;
    refuse_unless(
        state.pairing.opponent < rider_characters
            || (opponent.x
                    == static_cast<std::uint16_t>(content_word(track, opponent_start_x) << 4U)
                && opponent.y
                       == static_cast<std::uint16_t>(content_word(track, opponent_start_y) << 4U)),
        "a stunt event's opponent left its start");
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
    if (bytes.size() == other_track_size || bytes.size() == stunt_track_size) {
        const bool stunt = bytes.size() == stunt_track_size;
        refuse_unless(magic_is("URTR") && bytes[4] >= '0' && bytes[4] <= '9' && bytes[5] >= '0'
                          && bytes[5] <= '9' && bytes[6] == '0' && bytes[7] == (stunt ? '7' : '6'),
                      "classic race state identity/width differs");
        const ClassicRaceTrack other{
            static_cast<std::uint8_t>((bytes[4] - '0') * 10 + (bytes[5] - '0'))};
        refuse_unless(is_other_track(other) && classic_race_has_scenario(other)
                          && is_stunt_track(other) == stunt,
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
            static_cast<std::uint8_t>(is_stunt_track(track) ? '7' : '6')};
}

namespace {
constexpr std::array<std::uint8_t, 8> league_magic{'U', 'R', 'L', 'G', '0', '0', '0', '1'};
// An opt-in wrapper retains the established base plus the second camera and league bonus words.
// Existing race formats are byte-identical when league statistics are disabled (R-0073).
std::vector<std::uint8_t> serialize_league_race(const ZoomZooState& state) {
    refuse_unless(state.native_initialization && !state.demo_ai,
                  "league save requires native human play");
    auto base = state;
    base.league_statistics = {};
    base.split_screen = false;
    base.pause.lower_view = false;
    const auto payload = serialize_zoom_zoo(base);
    std::vector<std::uint8_t> bytes(league_magic.begin(), league_magic.end());
    put16(bytes, static_cast<std::uint16_t>(payload.size()));
    put8(bytes, state.pairing.rider);
    put8(bytes, state.pairing.opponent);
    put_bool(bytes, state.split_screen);
    bytes.insert(bytes.end(), payload.begin(), payload.end());
    const auto& camera = state.race.second_camera;
    for (const auto word : {camera.x, camera.y, camera.velocity_x, camera.velocity_y,
                            camera.lookahead, camera.screen_xy})
        put16(bytes, word);
    for (const auto& counts : state.league_statistics.tricks)
        for (const auto count : counts) put8(bytes, count);
    for (const auto count : state.league_statistics.wipeouts) put16(bytes, count);
    for (const auto points : state.league_statistics.opponent_points) put16(bytes, points);
    put_bool(bytes, !state.opponent_hints.active); // rider 1's hints over
    if (state.pause.lower_view) put8(bytes, 1);
    return bytes;
}

// A two-view state: its base layout's letter for the split layouts, the split trailer, a VS race's
// banner drivers and the lower view's byte.
void write_split_state(std::vector<std::uint8_t>& bytes, const ZoomZooState& state) {
    const bool local_dragster = state.track == ClassicRaceTrack::Dragster && !state.demo_ai;
    refuse_unless(state.native_initialization
                      && (state.track == ClassicRaceTrack::ZoomZoo || local_dragster),
                  "split race state requires native ZOOM ZOO or local DRAGSTER initialization");
    refuse_unless(bytes.size() == native_race_size || bytes.size() == extended_size,
                  "split race base layout is unsupported");
    bytes[7] = local_dragster                   ? bytes.size() == native_race_size
                                                    ? local_dragster_split_layout
                                                    : extended_local_dragster_split_layout
             : bytes.size() == native_race_size ? split_layout
                                                : extended_split_layout;
    write_split_trailer(bytes, state);
    if (state.versus)
        for (const auto& driver : state.race.banners) {
            put16(bytes, driver.index);
            put16(bytes, driver.life);
        }
    if (state.pause.lower_view) put8(bytes, 1);
}
} // namespace

std::vector<std::uint8_t> serialize_zoom_zoo(const ZoomZooState& state) {
    refuse_unless(!state.versus
                      || (state.split_screen && !state.demo_ai && !state.league_statistics.enabled),
                  "only a two-pad race from the VS menu is a VS race");
    refuse_unless(state.versus || state.race.banners == std::array<ZoomZooBannerDriver, 2>{},
                  "only a VS race keeps banner drivers in its state");
    if (state.league_statistics.enabled) return serialize_league_race(state);
    refuse_unless(!state.pause.lower_view || (state.split_screen && !state.demo_ai),
                  "only a two-pad race's pause menu sits in the lower view");
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
        if (is_stunt_track(state.track)) write_stunt_event(bytes, state.stunt);
    } else {
        refuse_unless(state.hunter == HunterEffects{},
                      "HUNTER effects run only on the HUNTER tour");
    }
    refuse_unless(is_stunt_track(state.track) || state.stunt == StuntEvent{},
                  "only a stunt event keeps a score to qualify, a clock's stop and trick tallies");
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
    if (state.demo_ai && !state.split_screen) {
        refuse_unless(state.native_initialization && state.track.index == 3
                          && bytes.size() == other_track_size,
                      "one-view demo state requires the recovered track 3 race");
        bytes[7] = one_view_demo_layout;
        write_split_trailer(bytes, state);
    } else if (state.split_screen) {
        write_split_state(bytes, state);
    }
    return bytes;
}

namespace {

void validate_split_demo_words(const DemoControllers& demo) {
    // $83:E2C4-E55B; R-0069: the rotation window is set to 50, never decremented.
    for (unsigned rider = 0; rider < 2; ++rider)
        refuse_unless(demo.trick_bits[rider] <= 7
                          && (demo.rotation_window[rider] == 0 || demo.rotation_window[rider] == 50)
                          && demo.turnaround[rider] <= 30 && demo.airborne_rotation[rider] <= 0x4000
                          && (demo.airborne_rotation[rider] || !demo.trick_bits[rider]),
                      "split demo controller words are invalid");
}

// Rider 1's hint counter and group are not stored: while its hints run they follow the race's
// unpaused updates from 0, as the player's follow them from 30 (check_hint_timeline, R-0082).
void derive_opponent_hint_clock(ZoomZooState& state, std::uint32_t initialization_frame) {
    auto& hints = state.opponent_hints;
    hints.updates = hints.group = 0;
    if (!hints.active) return;
    refuse_unless(state.movement.frame >= initialization_frame + state.pause.suspended_updates,
                  "rider 1's hint clock is before the race's initialization");
    const auto clock = state.movement.frame - initialization_frame - state.pause.suspended_updates;
    hints.updates = static_cast<std::uint16_t>(clock % hint_interval);
    hints.group = static_cast<std::uint16_t>((clock / hint_interval) % hint_groups);
}

// $83:E7C3-E7E4: a split race's finish display counts only once both riders have finished
// (R-0081).
void check_split_finish_display(const ZoomZooState& state) {
    const auto& riders = state.race.riders;
    refuse_unless(!state.split_screen || !state.race.finish_delay
                      || (riders[0].finished && riders[1].finished),
                  "a split race's finish display counts before both riders have finished");
}

// A VS race's banner drivers: each idle (0, 0) until its rider finishes, its index 0 or 7..24 and
// its life under 360, never two alive at once, and a forced finish only after the other rider's
// driver has ended (R-0081).
void read_versus_block(Reader& in, ZoomZooState& state) {
    refuse_unless(!state.demo_ai, "a demo is not a VS race");
    auto& drivers = state.race.banners;
    for (unsigned rider = 0; rider < 2; ++rider) {
        auto& driver = drivers[rider];
        driver.index = in.u16();
        driver.life = in.u16();
        refuse_unless(
            (driver.index == 0 || (driver.index >= first_banner && driver.index <= last_banner))
                && driver.life <= longest_banner_life
                && (state.race.riders[rider].finished || (!driver.index && !driver.life)),
            "a VS banner driver is invalid");
    }
    refuse_unless(!drivers[0].life || !drivers[1].life, "two VS banner drivers live at once");
    for (unsigned rider = 0; rider < 2; ++rider) {
        const auto& other = drivers[1 - rider];
        refuse_unless(state.race.riders[rider].finished != forced_finish
                          || (other.index && !other.life),
                      "a forced finish needs the other rider's banner to have ended");
    }
}

// The byte a two-pad state appends while pad 2's pause menu is open: always 1, and only with
// the menu open and two pads in play.
void read_lower_view(Reader& in, ZoomZooState& state) {
    refuse_unless(in.u8() == 1 && state.pause.selection != 0 && state.split_screen
                      && !state.demo_ai,
                  "a lower-view pause needs a two-pad race with its menu open");
    state.pause.lower_view = true;
}

ZoomZooState read_demo_trailer(ZoomZooState state, std::span<const std::uint8_t> trailer,
                               bool one_view, bool local_dragster) {
    Reader in{trailer};
    auto& camera = state.race.second_camera;
    for (auto* value : {&camera.x, &camera.y, &camera.velocity_x, &camera.velocity_y,
                        &camera.lookahead, &camera.screen_xy})
        *value = in.u16();
    for (auto* words : {&state.demo.trick_bits, &state.demo.rotation_window, &state.demo.turnaround,
                        &state.demo.airborne_rotation})
        for (auto& value : *words) value = in.u16();
    state.demo.elapsed = in.u16();
    const auto exit_requested = in.u8(), hints_active = in.u8();
    refuse_unless(exit_requested <= 1 && hints_active <= 1, "split demo flags are invalid");
    state.demo.exit_requested = exit_requested != 0;
    state.opponent_hints.active = hints_active != 0;
    state.pairing = {in.u8(), in.u8()};
    state.opponent_tier.ai_level = in.u16();
    state.opponent_tier.catch_up = in.u16();
    state.opponent_tier.adjustment_limit = in.u16();
    const auto split = in.u8(), demo_ai = in.u8();
    refuse_unless(split == static_cast<unsigned>(!one_view)
                      && (one_view         ? demo_ai == 1
                          : local_dragster ? demo_ai == 0
                                           : demo_ai <= 1),
                  "demo race mode flags are invalid");
    state.split_screen = !one_view;
    state.demo_ai = demo_ai != 0;
    in.require_end();
    validate_split_demo_words(state.demo);
    const bool camera_valid =
        one_view ? camera.x == one_view_camera_x && camera.y == one_view_camera_y
                       && camera.screen_xy == one_view_camera_oam
                       && std::abs(static_cast<std::int16_t>(camera.velocity_x)) <= fastest_camera
                       && std::abs(static_cast<std::int16_t>(camera.velocity_y)) <= fastest_camera
                 : (local_dragster || camera.x <= zoom_zoo_camera_x_limit)
                       && std::abs(static_cast<std::int16_t>(camera.velocity_x)) <= fastest_camera
                       && std::abs(static_cast<std::int16_t>(camera.velocity_y)) <= fastest_camera;
    refuse_unless(
        state.native_initialization && state.demo.elapsed <= 0x076c && camera_valid
            && (one_view ? state.track.index == 3 && state.pairing == one_view_demo_pairing
                               && state.opponent_tier == one_view_demo_opponent_tier
                : local_dragster ? state.track == ClassicRaceTrack::Dragster
                                       && state.pairing.rider < rider_characters
                                       && state.pairing.opponent < rider_characters
                                       && state.pairing.rider != state.pairing.opponent
                                       && state.opponent_tier == OpponentTier{0, 0, 0x60}
                                       && state.demo.elapsed == 0 && !state.demo.exit_requested
                                 : state.track == ClassicRaceTrack::ZoomZoo
                                       && state.pairing.rider < rider_characters
                                       && state.pairing.opponent < rider_characters),
        "demo race state is outside its recovered domain");
    return state;
}

ZoomZooState deserialize_native_race(std::span<const std::uint8_t> bytes,
                                     std::optional<RacePairing> pairing, bool tutorial_hints,
                                     std::span<const std::uint8_t> opponent_catch_up,
                                     std::optional<std::uint32_t> initialization_frame,
                                     TrailerFlags flags = {}) {
    const auto track = identified_track(bytes);
    if (!track)
        return deserialize_classic_race(bytes, ClassicRaceTrack::ZoomZoo, pairing, tutorial_hints,
                                        opponent_catch_up, initialization_frame, flags);
    std::vector<std::uint8_t> shared(bytes.begin(), bytes.begin() + native_race_size);
    const auto zoom_zoo_magic = classic_race_state_magic(ClassicRaceTrack::ZoomZoo);
    std::copy(zoom_zoo_magic.begin(), zoom_zoo_magic.end(), shared.begin());
    auto state = deserialize_classic_race(shared, *track, pairing, tutorial_hints,
                                          opponent_catch_up, initialization_frame, flags);
    if (bytes.size() == native_race_size) return state;
    Reader tiles{bytes.subspan(native_race_size, special_tiles_size)};
    read_special_tiles(tiles, state);
    if (bytes.size() >= other_track_size) {
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
    if (bytes.size() == stunt_track_size) {
        Reader stunt{bytes.subspan(other_track_size, stunt_event_size)};
        read_stunt_event(stunt, state);
    }
    // DRAGSTER and ZOOM ZOO take the extended layout only while a word is live.
    refuse_unless(is_other_track(*track) || state.special_tiles != std::array<SpecialTileRider, 2>{}
                      || state.opponent_turnaround,
                  "an extended DRAGSTER or ZOOM ZOO state carries no special-tile word");
    return state;
}

bool two_view_size(std::size_t size) {
    return size == split_race_size || size == extended_split_size;
}

// The optional blocks after a two-view state's split trailer, by the state's size.
struct SplitSuffixes {
    bool versus{}, lower_view{};
    std::size_t size() const {
        return (versus ? versus_block_size : 0U) + (lower_view ? lower_view_size : 0U);
    }
};
SplitSuffixes split_suffixes(std::size_t size) {
    SplitSuffixes suffixes;
    suffixes.lower_view = two_view_size(size - lower_view_size)
                       || two_view_size(size - lower_view_size - versus_block_size);
    if (suffixes.lower_view) size -= lower_view_size;
    suffixes.versus = two_view_size(size - versus_block_size);
    return suffixes;
}

ZoomZooState deserialize_race(std::span<const std::uint8_t> bytes,
                              std::optional<RacePairing> pairing, bool tutorial_hints,
                              std::span<const std::uint8_t> opponent_catch_up,
                              std::optional<std::uint32_t> initialization_frame = std::nullopt,
                              TrailerFlags flags = {}) {
    const auto suffixes = split_suffixes(bytes.size());
    const auto whole = bytes;
    bytes = bytes.first(bytes.size() - suffixes.size());
    if (bytes.size() == split_race_size || bytes.size() == extended_split_size
        || bytes.size() == one_view_demo_size) {
        const bool one_view = bytes.size() == one_view_demo_size;
        const bool extended = bytes.size() == extended_split_size;
        const bool local_dragster =
            !one_view
            && std::equal(bytes.begin(), bytes.begin() + 7,
                          classic_race_state_magic(ClassicRaceTrack::Dragster).begin());
        const auto expected = one_view       ? one_view_demo_layout
                            : local_dragster ? extended ? extended_local_dragster_split_layout
                                                        : local_dragster_split_layout
                            : extended       ? extended_split_layout
                                             : split_layout;
        const bool identity =
            one_view
                ? std::equal(bytes.begin(), bytes.begin() + 7,
                             classic_race_state_magic(ClassicRaceTrack{3}).begin())
                : local_dragster
                      || std::equal(bytes.begin(), bytes.begin() + 7, race_state_magic.begin());
        refuse_unless(identity && bytes[7] == expected, "demo race state identity/width differs");
        const auto base_size = one_view ? other_track_size
                             : extended ? extended_size
                                        : native_race_size;
        if (one_view) {
            const std::uint32_t frame = std::uint32_t(bytes[8]) | (std::uint32_t(bytes[9]) << 8U)
                                      | (std::uint32_t(bytes[10]) << 16U)
                                      | (std::uint32_t(bytes[11]) << 24U);
            const auto elapsed =
                unsigned(bytes[base_size + 28]) | (unsigned(bytes[base_size + 29]) << 8U);
            const auto exit = unsigned(bytes[base_size + 30]);
            refuse_unless(exit <= 1 && frame >= elapsed + exit,
                          "one-view demo clock is outside its recovered domain");
            initialization_frame = frame - elapsed - exit;
        }
        const auto base_view = bytes.first(base_size);
        std::vector<std::uint8_t> base(base_view.begin(), base_view.end());
        base[7] = one_view ? '6'
                : local_dragster
                    ? extended ? extended_dragster_layout : dragster_race_state_magic[7]
                : extended ? extended_zoom_zoo_layout
                           : native_race_layout;
        const auto human = local_dragster ? std::optional<RacePairing>{RacePairing{
                                                bytes[base_size + 32], bytes[base_size + 33]}}
                                          : pairing;
        if (local_dragster && pairing)
            refuse_unless(*pairing == *human, "local race pairing differs from state trailer");
        auto state = deserialize_race(base, one_view ? std::nullopt : human, tutorial_hints,
                                      opponent_catch_up, initialization_frame,
                                      TrailerFlags{suffixes.versus, bytes[base_size + 31] != 0});
        state =
            read_demo_trailer(std::move(state), bytes.subspan(base_size), one_view, local_dragster);
        refuse_unless(state.demo_ai || !state.opponent_hints.active || state.pairing.opponent != 0,
                      "a rider 1 who is MIKE has no tutorial hints");
        derive_opponent_hint_clock(
            state,
            initialization_frame.value_or(classic_race_scenario(state.track).initialization_frame));
        check_split_finish_display(state);
        Reader in{whole.subspan(bytes.size())};
        if (suffixes.versus) read_versus_block(in, state);
        if (suffixes.lower_view) read_lower_view(in, state);
        in.require_end();
        return state;
    }
    return deserialize_native_race(bytes, pairing, tutorial_hints, opponent_catch_up,
                                   initialization_frame, flags);
}
} // namespace

namespace {
std::optional<ZoomZooState> deserialize_league_race(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < league_magic.size()
        || !std::equal(league_magic.begin(), league_magic.end(), bytes.begin()))
        return std::nullopt;
    Reader header{bytes.subspan(8)};
    const auto size = header.u16();
    const RacePairing pairing{header.u8(), header.u8()};
    const auto split = header.flag();
    const bool lower_view = bytes.size() == 13U + size + 97U + lower_view_size;
    refuse_unless(size <= 1006 && size >= 742 && (bytes.size() == 13U + size + 97U || lower_view),
                  "league race wrapper width differs");
    // The wrapper's last byte before the lower view: rider 1's hints over.
    const bool opponent_hints = bytes[13U + size + 96U] == 0;
    auto state = deserialize_race(bytes.subspan(13, size), pairing, true, {}, std::nullopt,
                                  TrailerFlags{false, opponent_hints});
    refuse_unless(!state.versus, "a league race is not a VS race");
    refuse_unless(state.native_initialization && !state.demo_ai && pairing.rider < rider_characters
                      && split == (pairing.opponent < rider_characters)
                      && pairing.rider != pairing.opponent,
                  "league race wrapper pairing differs");
    Reader trailer{bytes.subspan(13U + size)};
    auto& camera = state.race.second_camera;
    for (auto* word : {&camera.x, &camera.y, &camera.velocity_x, &camera.velocity_y,
                       &camera.lookahead, &camera.screen_xy})
        *word = trailer.u16();
    refuse_unless(
        !split
            || ((state.track != ClassicRaceTrack::ZoomZoo || camera.x <= zoom_zoo_camera_x_limit)
                && std::abs(static_cast<std::int16_t>(camera.velocity_x)) <= fastest_camera
                && std::abs(static_cast<std::int16_t>(camera.velocity_y)) <= fastest_camera),
        "league second camera state invalid");
    state.split_screen = split;
    check_split_finish_display(state);
    state.league_statistics.enabled = true;
    for (auto& counts : state.league_statistics.tricks)
        for (auto& count : counts) count = trailer.u8();
    for (auto& count : state.league_statistics.wipeouts) count = trailer.u16();
    for (auto& points : state.league_statistics.opponent_points) points = trailer.u16();
    state.opponent_hints.active = !trailer.flag();
    refuse_unless(!state.opponent_hints.active || (split && pairing.opponent != 0),
                  "only a human rider 1 other than MIKE has tutorial hints");
    derive_opponent_hint_clock(state, classic_race_scenario(state.track).initialization_frame);
    if (lower_view) read_lower_view(trailer, state);
    trailer.require_end();
    return state;
}
} // namespace

ZoomZooState deserialize_zoom_zoo(std::span<const std::uint8_t> bytes) {
    if (const auto league = deserialize_league_race(bytes)) return *league;
    return deserialize_race(bytes, std::nullopt, true, {});
}

ZoomZooState deserialize_zoom_zoo(std::span<const std::uint8_t> bytes, RacePairing pairing,
                                  bool tutorial_hints,
                                  std::span<const std::uint8_t> opponent_catch_up) {
    if (const auto league = deserialize_league_race(bytes)) {
        refuse_unless(league->pairing == pairing, "league pairing differs from caller");
        return *league;
    }
    return deserialize_race(bytes, pairing, tutorial_hints, opponent_catch_up);
}

void validate_zoom_zoo_content_state(const ZoomZooState& state, const ZoomZooContent& content) {
    if (!state.native_initialization) return;
    refuse_unless(content.reward_weights.size() == 26, "ZOOM ZOO reward weights missing");
    if (is_stunt_track(state.track)) check_stunt_content(state, content);
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
