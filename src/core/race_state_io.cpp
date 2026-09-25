// The serialized race states (URZZ, URDG, URTRnn): writing, reading and validation.

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

void write_reflection(std::vector<std::uint8_t>& bytes, const ReflectionTransition& r) {
    for (auto v : {r.step, r.end, r.pose_base, r.pose_override, r.completed, r.hold,
                   r.drive_pose_enabled, r.air_turns, r.direction_latch, r.base_velocity_cap,
                   r.brake_input, r.rotate_negative_input, r.rotate_positive_input, r.jump_input,
                   r.wrong_direction_counter})
        put16(bytes, v);
}

void read_reflection(Reader& in, ReflectionTransition& r) {
    for (auto* v : {&r.step, &r.end, &r.pose_base, &r.pose_override, &r.completed, &r.hold,
                    &r.drive_pose_enabled, &r.air_turns, &r.direction_latch, &r.base_velocity_cap,
                    &r.brake_input, &r.rotate_negative_input, &r.rotate_positive_input,
                    &r.jump_input, &r.wrong_direction_counter})
        *v = in.u16();
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
    if (state.complete_race && !state.sustained)
        throw std::invalid_argument("race state requires sustained prefix");
    auto bytes = serialize_movement_state(state.movement);
    if (bytes.size() != 333) throw std::invalid_argument("ZOOM ZOO finish state is unsupported");
    const std::array<std::uint8_t, 8> magic{'U', 'R', 'Z', 'Z', '0', '0', '0', '1'};
    std::copy(magic.begin(), magic.end(), bytes.begin());
    for (const auto& r : state.reflection) write_reflection(bytes, r);
    put8(bytes, state.opponent_horizontal);
    put8(bytes, state.opponent_retained_oam_x);
    if (state.sustained) {
        bytes[7] = '2';
        for (const auto& r : state.surface)
            for (auto v : {r.mode, r.angle, r.tile_mode, r.leading_support, r.tile_pose,
                           r.animation_delta, r.tile_pose_enabled})
                put16(bytes, v);
    }
    if (state.complete_race) {
        bytes[7] = state.native_initialization ? 'B' : '3';
        for (const auto& r : state.race.riders) {
            for (auto v : {r.laps_remaining, r.checkpoint, r.next_checkpoint, r.start_line_latch,
                           r.checkpoint_display_countdown, r.finished})
                put16(bytes, v);
            for (auto v : r.time_digits) put16(bytes, v);
        }
        for (const auto& times : state.race.lap_times)
            for (auto v : times) put16(bytes, v);
        for (auto v : state.race.total_times) put16(bytes, v);
        for (auto v :
             {state.race.provisional_1225, state.race.provisional_1227, state.race.finish_delay})
            put16(bytes, v);
        const auto& c = state.race.camera;
        for (auto v : {c.x, c.y, c.velocity_x, c.velocity_y, c.lookahead, c.screen_xy})
            put16(bytes, v);
        for (unsigned i = 0; i < 20; ++i) put8(bytes, state.race.checkpoint_seen[i]);
        for (const auto& p : state.race.finish_pose)
            for (auto v : {p.selector, p.kind, p.locked, p.active}) put16(bytes, v);
    }
    if (state.native_initialization) {
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
        for (auto v : {a.hints_active, a.hint_updates, a.hint_group, a.empty_display})
            put16(bytes, v);
        for (const auto& r : state.rolls)
            for (auto v : {r.input_latched, r.prior_orientation, r.prior_reflection, r.pose_base,
                           r.step, r.held_updates, r.bounce_charge, r.completed_rolls,
                           r.held_rotations, r.bounce_active, r.support_count_mirror, r.prior_step})
                put16(bytes, v);
        for (const auto& weights : state.learned_weights)
            for (auto v : weights) put8(bytes, v);
        put16(bytes, state.pause.selection);
        put16(bytes, state.pause.released);
        put32(bytes, state.pause.suspended_updates);
        put32(bytes, state.pause.suspended_countdown_updates);
    }
    // R-0047: the special-tile words follow the shared 742 bytes in the other
    // tracks' layout (URTRnn05), and in DRAGSTER's and ZOOM ZOO's only while
    // one is live (URDG0004, URZZ000E): no accepted race reaches a special
    // tile, so their frozen 742-byte states are unchanged, but ZOOM ZOO's own
    // tile table holds the corkscrew (pair 10).
    const bool other_track =
        state.track != ClassicRaceTrack::ZoomZoo && state.track != ClassicRaceTrack::Dragster;
    const bool special_tiles_live =
        state.special_tiles != std::array<SpecialTileRider, 2>{} || state.opponent_turnaround;
    if (other_track || special_tiles_live) {
        if (!state.native_initialization)
            throw std::invalid_argument("special-tile words require a natively initialized race");
        for (const auto& r : state.special_tiles)
            for (auto v : {r.mud_cooldown, r.mud_exit_pending, r.corkscrew_latch, r.corkscrew_step,
                           r.corkscrew_float, r.physics_hold, r.reflection_lock, r.raised_priority,
                           r.loop_direction, r.loop_step, r.loop_cooldown, r.slow_counter})
                put16(bytes, v);
        put16(bytes, state.drive_target_latch);
        put16(bytes, state.opponent_turnaround);
        if (state.track == ClassicRaceTrack::ZoomZoo) bytes[7] = 'E';
    }
    // R-0048: the other tracks' layout also carries the last 60 first-seen
    // flags. DRAGSTER's one lap and ZOOM ZOO's three index at most flag 19,
    // so their layouts hold only the first 20 and a restore sets the rest to
    // $FF, as race setup does.
    if (other_track)
        for (unsigned i = 20; i < 80; ++i) put8(bytes, state.race.checkpoint_seen[i]);
    // R-0052: the HUNTER effects' words close the other tracks' layout
    // (URTRnn06); DRAGSTER and ZOOM ZOO never run them.
    if (other_track) {
        const auto& h = state.hunter;
        put16(bytes, h.latched);
        put16(bytes, h.active);
        for (auto v : h.effect) put16(bytes, v);
        for (auto v : h.timer) put16(bytes, v);
        for (auto v : {h.pulse, h.pulse_shrinking, h.pulse_length, h.blink, h.wave_phase,
                       h.hide_track, h.mosaic, h.skip_update, h.message, h.shown, h.hud_event,
                       h.caption, h.mosaic_counter})
            put16(bytes, v);
    } else if (state.hunter != HunterEffects{})
        throw std::invalid_argument("HUNTER effects run only on the HUNTER tour");
    if (state.track != ClassicRaceTrack::ZoomZoo) {
        // Another track on the shared engine: the URZZ000B layout under its own
        // identity, so a restore can never run one track's state on another.
        if (!state.native_initialization)
            throw std::invalid_argument(
                "a race state of any track but ZOOM ZOO requires native initialization");
        const auto identity = classic_race_state_magic(state.track);
        std::copy(identity.begin(), identity.end(), bytes.begin());
        if (state.track == ClassicRaceTrack::Dragster && special_tiles_live) bytes[7] = '4';
    }
    return bytes;
}

static ZoomZooState deserialize_classic_race(std::span<const std::uint8_t> bytes,
                                             ClassicRaceTrack track) {
    const std::array<std::uint8_t, 8> magic{'U', 'R', 'Z', 'Z', '0', '0', '0', '1'};
    const auto scenario = classic_race_scenario(track);
    const bool native_initialization = bytes.size() == 742 && bytes[7] == 'B';
    const bool complete_race = ((bytes.size() == 565 && bytes[7] == '3') || native_initialization)
                            && std::equal(magic.begin(), magic.begin() + 7, bytes.begin());
    const bool sustained = complete_race
                        || (bytes.size() == 423 && bytes[7] == '2'
                            && std::equal(magic.begin(), magic.begin() + 7, bytes.begin()));
    if (!sustained
        && (bytes.size() != 395 || !std::equal(magic.begin(), magic.end(), bytes.begin())))
        throw std::invalid_argument("ZOOM ZOO state identity/width differs");
    std::vector<std::uint8_t> prefix(bytes.begin(), bytes.begin() + 333);
    std::copy(movement_state_magic.begin(), movement_state_magic.end(), prefix.begin());
    ZoomZooState state;
    state.track = track;
    state.native_initialization = native_initialization;
    state.complete_race = complete_race;
    state.sustained = sustained;
    state.movement = deserialize_movement_state(prefix);
    Reader in{bytes.subspan(333)};
    for (auto& r : state.reflection) read_reflection(in, r);
    state.opponent_horizontal = in.u8();
    state.opponent_retained_oam_x = in.u8();
    if (state.opponent_horizontal > 2)
        throw std::invalid_argument("ZOOM ZOO horizontal input is invalid");
    if (sustained)
        for (auto& r : state.surface)
            for (auto* v : {&r.mode, &r.angle, &r.tile_mode, &r.leading_support, &r.tile_pose,
                            &r.animation_delta, &r.tile_pose_enabled})
                *v = in.u16();
    if (complete_race) {
        for (auto& r : state.race.riders) {
            for (auto* v : {&r.laps_remaining, &r.checkpoint, &r.next_checkpoint,
                            &r.start_line_latch, &r.checkpoint_display_countdown, &r.finished})
                *v = in.u16();
            for (auto& v : r.time_digits) v = in.u16();
            if (r.laps_remaining > scenario.laps + 1U || r.checkpoint > 3 || r.next_checkpoint > 3
                || r.start_line_latch > 1 || r.finished > 1 || r.checkpoint_display_countdown > 120)
                throw std::invalid_argument("ZOOM ZOO race checkpoint state is invalid");
        }
        for (auto& times : state.race.lap_times)
            for (auto& v : times) v = in.u16();
        for (auto& v : state.race.total_times) v = in.u16();
        state.race.provisional_1225 = in.u16();
        state.race.provisional_1227 = in.u16();
        state.race.finish_delay = in.u16();
        auto& c = state.race.camera;
        for (auto* v : {&c.x, &c.y, &c.velocity_x, &c.velocity_y, &c.lookahead, &c.screen_xy})
            *v = in.u16();
        for (unsigned i = 0; i < 20; ++i) state.race.checkpoint_seen[i] = in.u8();
        std::fill(state.race.checkpoint_seen.begin() + 20, state.race.checkpoint_seen.end(),
                  std::uint8_t{255});
        for (auto& p : state.race.finish_pose)
            for (auto* v : {&p.selector, &p.kind, &p.locked, &p.active}) *v = in.u16();
        if (state.race.finish_delay > 240
            || (state.race.finish_delay && !state.race.riders[0].finished)
            || state.race.provisional_1225 > 1 || state.race.provisional_1227 > 1
            || (track == ClassicRaceTrack::ZoomZoo && c.x > 0x3fffU)
            || std::abs(static_cast<std::int16_t>(c.velocity_x)) > 16
            || std::abs(static_cast<std::int16_t>(c.velocity_y)) > 16)
            throw std::invalid_argument("ZOOM ZOO finish/camera state invalid");
        for (auto seen : state.race.checkpoint_seen)
            if (seen != 0 && seen != 255)
                throw std::invalid_argument("ZOOM ZOO checkpoint seen flag invalid");
        for (unsigned index = 0; index < 2; ++index) {
            const auto& pose = state.race.finish_pose[index];
            const unsigned limit = pose.kind == 1 ? 48U : pose.kind == 2 ? 88U : 0U;
            if ((pose.active && !state.race.riders[index].finished) || pose.selector > limit
                || pose.kind > 2 || pose.locked > 1 || pose.active > 1
                || (pose.active ? (!pose.kind || !pose.locked)
                                : (pose.kind || pose.locked || pose.selector)))
                throw std::invalid_argument("ZOOM ZOO finish pose state invalid");
        }
        // $81:C73E-C75B finishes both riders, laps or not, once the clock holds 9:59.9.
        const auto& clock = state.movement.timer;
        const bool clock_expired = native_initialization && clock.minutes == 9
                                && clock.tens_seconds == 5 && clock.seconds == 9
                                && clock.tenths == 9;
        const bool timed_out =
            state.race.riders[0].finished && state.race.riders[1].finished && clock_expired;
        for (const auto& lap : state.race.riders) {
            if ((lap.finished ? lap.laps_remaining != 0 && !timed_out : lap.laps_remaining == 0)
                || lap.time_digits[0] > 9 || lap.time_digits[1] > 5 || lap.time_digits[2] > 9
                || lap.time_digits[3] > 9 || lap.time_digits[4] > 9)
                throw std::invalid_argument("ZOOM ZOO lap time state invalid");
        }
    }
    if (native_initialization) {
        state.fade_level = in.u16();
        for (auto& v : state.start_boost) v = in.u16();
        state.result_updates = in.u16();
        state.result.graph_minimum = in.u16();
        state.result.graph_maximum = in.u16();
        for (auto& v : state.result.published_totals) v = in.u16();
        for (auto& v : state.charge_announced) {
            v = in.u16();
            if (v > 1) throw std::invalid_argument("invalid ZOOM ZOO charge flag");
        }
        auto& a = state.player_announcements;
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
        if (q.read_cursor > 31 || q.write_cursor > 31 || q.cooldown > 120 ||
            // The reachable weights are the halvings of the $82D7A4 template
            // byte the initializer now reads from content; they coincide for
            // the frozen pack, so a template change must revisit this bound.
            (q.event_one_weight != 1 && q.event_one_weight != 2 && q.event_one_weight != 4)
            || a.hints_active > 1 || a.hint_updates >= 300 || a.hint_group >= 8
            || a.empty_display > 1)
            throw std::invalid_argument("invalid ZOOM ZOO player announcement state");
        // The opponent's identical queue fields had no bound at all, so a
        // forged bank could publish an arbitrary feature total or a weight
        // that halving can never reach. $82DB87-DB94 seeds both banks from
        // one template, and $81C27B-C280 only ever halves toward a floor of
        // one, so the reachable set matches the player's. The opponent has
        // no tutorial path, so its cooldown tops out at the 40 of
        // $81C2CC-C2D0 rather than the player's hint value of 120.
        {
            const auto& o = state.movement.rewards;
            if (o.cooldown > 40
                || (o.event_one_weight != 1 && o.event_one_weight != 2 && o.event_one_weight != 4))
                throw std::invalid_argument("invalid ZOOM ZOO opponent reward queue state");
            // $83E1F1 masks the sloped selector with 7 and the flat path writes
            // only 0 or 1, so nothing above 7 is producible. The removed
            // multi-axis guards were this field's only bound, and without one a
            // forged 14 or 65535 would alias onto 6 and 7 and play identically
            // to them, which the original never does.
            if (state.movement.opponent_ai.trick_selector > 7)
                throw std::invalid_argument("invalid ZOOM ZOO opponent trick selector");
        }
        if (state.movement.countdown
            && (state.race.riders[0].finished || state.race.riders[1].finished
                || state.result_updates))
            throw std::invalid_argument("ZOOM ZOO finish/result conflicts with start phase");
        if (state.result
            != zoom_result_fields(state.race, state.result_updates, scenario.tour_race))
            throw std::invalid_argument("inconsistent ZOOM ZOO result publication");
        for (unsigned roll_index = 0; roll_index < state.rolls.size(); ++roll_index) {
            auto& r = state.rolls[roll_index];
            for (auto* v :
                 {&r.input_latched, &r.prior_orientation, &r.prior_reflection, &r.pose_base,
                  &r.step, &r.held_updates, &r.bounce_charge, &r.completed_rolls, &r.held_rotations,
                  &r.bounce_active, &r.support_count_mirror, &r.prior_step})
                *v = in.u16();
            if (r.input_latched > 1 || r.prior_orientation > 63 || r.prior_reflection > 1
                || static_cast<std::int16_t>(r.step) < -9 || static_cast<std::int16_t>(r.step) > 9
                || (r.bounce_charge != 0 && r.bounce_charge != 160) || r.bounce_active > 1
                || (r.bounce_charge && r.step) || r.prior_step || (r.pose_base & 0x4000U))
                throw std::invalid_argument("invalid ZOOM ZOO roll state");
            // $829641-9649 and $82965E-9666 read $0C6D and, when it is
            // non-zero, require rider $0FF9 to be the player before storing
            // charge $A0 at $1007. The reference guard holds $0C6D at 1 on
            // every authenticated frame, so the opponent can never charge and
            // never reach the bounce that only a charge enables. The native
            // producer already gates on the rider index; reject the forged
            // restore too instead of continuing from a state the original
            // cannot produce.
            if (roll_index == 1 && (r.bounce_charge || r.bounce_active))
                throw std::invalid_argument("invalid ZOOM ZOO opponent bounce state");
            if (state.movement.frame == scenario.initialization_frame
                && (r.input_latched || r.prior_orientation || r.prior_reflection || r.pose_base
                    || r.step || r.held_updates || r.bounce_charge || r.completed_rolls
                    || r.held_rotations || r.bounce_active || r.support_count_mirror
                    || r.prior_step))
                throw std::invalid_argument("inconsistent initial ZOOM ZOO roll state");
        }
        for (auto& weights : state.learned_weights)
            for (auto& v : weights) {
                v = in.u8();
                if (v > 64) throw std::invalid_argument("invalid ZOOM ZOO learned reward weight");
            }
        state.pause.selection = in.u16();
        state.pause.released = in.u16();
        state.pause.suspended_updates = in.u32();
        state.pause.suspended_countdown_updates = in.u32();
        if ((state.pause.selection != 0 && state.pause.selection != 1
             && state.pause.selection != 0xffff)
            || state.pause.released > 1 || state.movement.frame < scenario.initialization_frame
            || state.pause.suspended_updates > state.movement.frame - scenario.initialization_frame
            || state.pause.suspended_countdown_updates > state.pause.suspended_updates
            || ((state.pause.selection || state.pause.released) && !state.pause.suspended_updates))
            throw std::invalid_argument("invalid ZOOM ZOO pause state");
        for (unsigned i = 0; i < 2; ++i) {
            const auto& roll = state.rolls[i];
            // $8295D5-95F6 publishes the reflection flag and bit15 together
            // on every active roll pose, including held/returning poses.
            if (roll.step
                && bool(roll.pose_base & 0x8000U)
                       != (state.movement.riders[i].pose.reflected != (roll.prior_reflection != 0)))
                throw std::invalid_argument("inconsistent ZOOM ZOO active roll reflection");
            // Each held/completed counter advances at most once per update,
            // so neither can exceed the elapsed updates before a word wraps.
            // Hold duration is not bounded by held rotations: a landing's
            // reward pass ($829B69-9D97) clears the rotations while a released
            // roll keeps counting its hold across the bounce, and the original
            // reaches holds above rotations (DRAGSTER fuzz seeds 31 at 1623 and
            // 383 at 3472; R-0038). The earlier M4-16 bound rejected them.
            const auto elapsed = state.movement.frame >= scenario.initialization_frame
                                   ? state.movement.frame - scenario.initialization_frame
                                   : 0U;
            if (elapsed < 65536U) {
                const auto held = static_cast<std::int16_t>(roll.held_updates);
                const auto magnitude =
                    static_cast<unsigned>(held < 0 ? -static_cast<int>(held) : held);
                if (magnitude > elapsed || roll.held_rotations > elapsed
                    || roll.completed_rolls > elapsed)
                    throw std::invalid_argument("inconsistent ZOOM ZOO held roll counters");
            }
        }
        for (unsigned i = 0; i < 2; ++i)
            if (state.rolls[i].support_count_mirror
                != state.movement.riders[i].contact.unsupported_count)
                throw std::invalid_argument("inconsistent ZOOM ZOO support-count mirror");
        if (state.result_updates > std::max(scenario.stable_result_won, scenario.stable_result_lost)
            || (state.result_updates && state.race.finish_delay != 240))
            throw std::invalid_argument("invalid ZOOM ZOO result phase");
        for (auto v : state.start_boost)
            if (v != 0 && v != 384) throw std::invalid_argument("invalid ZOOM ZOO start boost");
        if (state.fade_level > 30) throw std::invalid_argument("invalid ZOOM ZOO fade level");
        if (state.movement.frame < scenario.initialization_frame)
            throw std::invalid_argument("invalid ZOOM ZOO initialization frame");
        const auto elapsed = state.movement.frame - scenario.initialization_frame;
        if (elapsed == 0
            && (q.read_cursor || q.write_cursor != 1 || q.cooldown || q.feature_total
                || q.event_one_weight != 4 || a.hints_active != 1 || a.hint_updates != 30
                || a.hint_group || a.empty_display
                || std::any_of(q.entries.begin(), q.entries.end(),
                               [](auto event) { return event != 0; })))
            throw std::invalid_argument("inconsistent initial ZOOM ZOO announcements");
        if (!state.result_updates && a.hints_active
            && (a.hint_updates != (elapsed - state.pause.suspended_updates + 30U) % 300U
                || a.hint_group != ((elapsed - state.pause.suspended_updates + 30U) / 300U) % 8U))
            throw std::invalid_argument("inconsistent ZOOM ZOO hint phase");
        // $829D47-D5B constrains voice producers for MIKE/BRONSEN. The
        // opponent consumer's domain test $81C238 is signed, so its 200-215
        // range reaches the reward path and exits on the zero learned weight
        // beyond the bank; only these produced values are admitted here.
        for (auto event : q.entries)
            if (event >= 88) throw std::invalid_argument("invalid ZOOM ZOO player voice event");
        // This clause is what keeps update_reward_queue's other rejections
        // unreachable and its learned-bank guards sufficient: the opponent's
        // sixteen voices follow its character (200-215, or 232-247 on the
        // HUNTER tour, R-0052); widening the admitted range means widening
        // those guards too.
        const unsigned first_voice = 72U + (scenario.opponent_character >> 1U) * 16U;
        for (auto event : state.movement.rewards.entries)
            if (event >= 72 && (event < first_voice || event > first_voice + 15U))
                throw std::invalid_argument("invalid ZOOM ZOO opponent voice event");
        for (unsigned cursor = (q.read_cursor + 1U) & 31U; cursor != q.write_cursor;
             cursor = (cursor + 1U) & 31U)
            if (q.entries[cursor] == 0)
                throw std::invalid_argument("empty pending ZOOM ZOO announcement");
        // The opponent's ring needs the same closure. $81C598-C5C8 never
        // publishes a zero, and the consumer rejects one, so a restore that
        // carried a published zero would only fail on the following update.
        {
            const auto& o = state.movement.rewards;
            for (unsigned cursor = (o.read_cursor + 1U) & 31U; cursor != o.write_cursor;
                 cursor = (cursor + 1U) & 31U)
                if (o.entries[cursor] == 0)
                    throw std::invalid_argument("empty pending ZOOM ZOO opponent reward");
        }
        if (elapsed == 0 && (state.charge_announced[0] || state.charge_announced[1]))
            throw std::invalid_argument("initial charge flag must be clear");
        if (state.pause.suspended_countdown_updates > (elapsed > 4U ? elapsed - 4U : 0U))
            throw std::invalid_argument("invalid ZOOM ZOO suspended countdown clock");
        const auto decrements =
            elapsed > 4U ? std::min(270U, elapsed - 4U - state.pause.suspended_countdown_updates)
                         : 0U;
        if (state.fade_level != std::min(30U, elapsed)
            || state.movement.countdown != 270U - decrements)
            throw std::invalid_argument("inconsistent ZOOM ZOO countdown/fade phase");
        if (state.movement.countdown >= 129U
            && (state.start_boost[0] != 384 || state.start_boost[1] != 384))
            throw std::invalid_argument("premature ZOOM ZOO start boost consumption");
        for (unsigned i = 0; i < 2; ++i) {
            const auto completed =
                unsigned(scenario.laps)
                - std::min(unsigned(scenario.laps), unsigned(state.race.riders[i].laps_remaining));
            std::uint16_t sum = 0;
            for (unsigned slot = 0; slot < 10; ++slot) {
                const auto lap = state.race.lap_times[i][slot];
                if ((slot < completed) == (lap == 60000))
                    throw std::invalid_argument("inconsistent ZOOM ZOO completed lap slots");
                if (slot < completed) sum = add_word(sum, lap);
            }
            // A rider finished by the clock limit keeps the no-time sentinel.
            if (state.race.total_times[i]
                != (state.race.riders[i].finished && !state.race.riders[i].laps_remaining ? sum
                                                                                          : 60000))
                throw std::invalid_argument("inconsistent ZOOM ZOO total time");
        }
    }
    for (const auto& surface : state.surface) {
        if (surface.mode > 1 || surface.tile_mode > 1 || surface.leading_support > 1
            || surface.tile_pose > 1 || surface.tile_pose_enabled > 1)
            throw std::invalid_argument("ZOOM ZOO surface flags are invalid");
    }
    if (state.movement.frame < (state.native_initialization ? scenario.initialization_frame : 1649U)
        || (!state.native_initialization && state.movement.frame > (sustained ? 9999U : 1849U)))
        throw std::invalid_argument("ZOOM ZOO state is outside trial horizon");
    for (const auto& r : state.reflection) {
        if (r.step > 16 || (r.end != 0 && r.end != 9 && r.end != 16) || r.completed > 1
            || r.drive_pose_enabled > 1 || r.brake_input > 1 || r.rotate_negative_input > 1
            || r.rotate_positive_input > 1 || r.jump_input > 1)
            throw std::invalid_argument("ZOOM ZOO reflection/control state is invalid");
    }
    in.require_end();
    return state;
}

ZoomZooState deserialize_zoom_zoo(std::span<const std::uint8_t> bytes) {
    // Any track but ZOOM ZOO carries its own identity over the URZZ000B layout;
    // a 794-byte DRAGSTER or ZOOM ZOO state (URDG0004, URZZ000E) appends the
    // special-tile words, $0E7B and $0C73 (R-0047, R-0050, R-0051), and a
    // 916-byte URTRnn06 state those, the last 60 checkpoint flags (R-0048) and
    // the HUNTER effects' 31 words (R-0052).
    const auto magic_is = [&](std::string_view text) {
        return std::equal(text.begin(), text.end(), bytes.begin());
    };
    std::optional<ClassicRaceTrack> track;
    bool extended = false;
    if (bytes.size() == 742) {
        if (std::equal(dragster_race_state_magic.begin(), dragster_race_state_magic.end(),
                       bytes.begin()))
            track = ClassicRaceTrack::Dragster;
    } else if (bytes.size() == 794) {
        extended = true;
        if (magic_is("URZZ000E"))
            track = ClassicRaceTrack::ZoomZoo;
        else if (magic_is("URDG0004"))
            track = ClassicRaceTrack::Dragster;
        else
            throw std::invalid_argument("classic race state identity/width differs");
    } else if (bytes.size() == 916) {
        extended = true;
        if (magic_is("URTR") && bytes[4] >= '0' && bytes[4] <= '9' && bytes[5] >= '0'
            && bytes[5] <= '9' && bytes[6] == '0' && bytes[7] == '6') {
            const ClassicRaceTrack other{
                static_cast<std::uint8_t>((bytes[4] - '0') * 10 + (bytes[5] - '0'))};
            if (other == ClassicRaceTrack::Dragster || other == ClassicRaceTrack::ZoomZoo
                || !classic_race_has_scenario(other))
                throw std::invalid_argument(
                    "classic race state names a track without its own identity");
            track = other;
        } else
            throw std::invalid_argument("classic race state identity/width differs");
    }
    if (!track) return deserialize_classic_race(bytes, ClassicRaceTrack::ZoomZoo);
    std::vector<std::uint8_t> shared(bytes.begin(), bytes.begin() + 742);
    const auto zoom_zoo_magic = classic_race_state_magic(ClassicRaceTrack::ZoomZoo);
    std::copy(zoom_zoo_magic.begin(), zoom_zoo_magic.end(), shared.begin());
    auto state = deserialize_classic_race(shared, *track);
    if (extended) {
        Reader in{bytes.subspan(742, 52)};
        for (auto& r : state.special_tiles) {
            for (auto* v :
                 {&r.mud_cooldown, &r.mud_exit_pending, &r.corkscrew_latch, &r.corkscrew_step,
                  &r.corkscrew_float, &r.physics_hold, &r.reflection_lock, &r.raised_priority,
                  &r.loop_direction, &r.loop_step, &r.loop_cooldown, &r.slow_counter})
                *v = in.u16();
            if (r.mud_cooldown > 4 || r.mud_exit_pending > 4 || r.corkscrew_float > 1
                || r.physics_hold > 8 || r.reflection_lock > 6 || r.raised_priority > 1
                || (r.corkscrew_step > 0x31 && r.corkscrew_step != 0xffff)
                || !(r.corkscrew_latch <= 1 || r.corkscrew_latch >= 0xfffc) || r.loop_direction > 1
                || (r.loop_step > 0x10 && r.loop_step < 0xfffe) || r.loop_cooldown > 3
                || r.slow_counter > 7)
                throw std::invalid_argument("classic race special-tile state is invalid");
        }
        state.drive_target_latch = in.u16();
        if (state.drive_target_latch > 1)
            throw std::invalid_argument("classic race drive target latch is invalid");
        state.opponent_turnaround = in.u16();
        if (state.opponent_turnaround > 30)
            throw std::invalid_argument("classic race opponent turnaround is invalid");
        in.require_end();
        if (bytes.size() == 916) {
            std::copy(bytes.begin() + 794, bytes.begin() + 854,
                      state.race.checkpoint_seen.begin() + 20);
            Reader hunter{bytes.subspan(854, 62)};
            auto& h = state.hunter;
            h.latched = hunter.u16();
            h.active = hunter.u16();
            for (auto& v : h.effect) v = hunter.u16();
            for (auto& v : h.timer) v = hunter.u16();
            for (auto* v : {&h.pulse, &h.pulse_shrinking, &h.pulse_length, &h.blink, &h.wave_phase,
                            &h.hide_track, &h.mosaic, &h.skip_update, &h.message, &h.shown,
                            &h.hud_event, &h.caption, &h.mosaic_counter})
                *v = hunter.u16();
            hunter.require_end();
            const auto valid_event = [](std::uint16_t e) { return !e || (e >= 0x1b && e <= 0x24); };
            const bool message_valid = valid_event(h.message) && valid_event(h.hud_event)
                                    && h.hud_event != 0x23 && h.shown <= 1 && h.caption <= 255
                                    && h.mosaic_counter <= 255;
            if (h.latched > 1 || h.active > 1 || h.pulse > 20 || h.pulse_shrinking > 1
                || h.pulse_length > 20 || h.blink > 1 || h.wave_phase > 1 || h.hide_track > 1
                || h.mosaic > 1 || h.skip_update > 1 || !message_valid || h.timer[1]
                || std::any_of(h.effect.begin(), h.effect.end(), [](auto v) { return v > 2; })
                || std::any_of(h.timer.begin(), h.timer.end(), [](auto v) { return v >= 500; })
                || (!classic_race_scenario(*track).hunter_tour && h != HunterEffects{}))
                throw std::invalid_argument("classic race HUNTER effect state is invalid");
            for (auto seen : state.race.checkpoint_seen)
                if (seen != 0 && seen != 255)
                    throw std::invalid_argument("classic race checkpoint-seen flag is invalid");
        }
        // DRAGSTER and ZOOM ZOO take the extended layout only while a word is live.
        if ((*track == ClassicRaceTrack::ZoomZoo || *track == ClassicRaceTrack::Dragster)
            && state.special_tiles == std::array<SpecialTileRider, 2>{}
            && !state.opponent_turnaround)
            throw std::invalid_argument(
                "an extended DRAGSTER or ZOOM ZOO state carries no special-tile word");
    }
    return state;
}

void validate_zoom_zoo_content_state(const ZoomZooState& state, const ZoomZooContent& content) {
    if (!state.native_initialization) return;
    if (content.reward_weights.size() != 26)
        throw std::invalid_argument("ZOOM ZOO reward weights missing");
    for (unsigned i = 0; i < 2; ++i) {
        const auto& roll = state.rolls[i];
        // The low pose base is latched once from the entry orientation at
        // $8293E5-941D, and retained after the roll. Bit15 is checked above.
        if ((roll.step || roll.pose_base)
            && ((roll.pose_base & 0x3fffU)
                != (content_word(content.roll_poses, 2U * roll.prior_orientation) & 0x3fffU)))
            throw std::invalid_argument("ZOOM ZOO roll base differs from static entry pose");
        if (state.movement.frame == classic_race_scenario(state.track).initialization_frame
            && !std::equal(state.learned_weights[i].begin(), state.learned_weights[i].end(),
                           content.reward_weights.begin() + 1))
            throw std::invalid_argument(
                "ZOOM ZOO initial reward weights differ from static content");
    }
}

} // namespace unirally
