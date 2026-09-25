// Race progress: checkpoints, laps, the finish and the result fields.

#include "race_progress.hpp"

#include "reward_queue.hpp"
#include "rider_motion.hpp"
#include "word_arithmetic.hpp"
#include "zoom_zoo_movement.hpp"

#include <algorithm>
#include <cstdint>
#include <stdexcept>

namespace unirally {

namespace {

// $0304 during the update that produces frame whole.frame + 1: the update's
// number counted from the race's initialization boundary, mod 3.
unsigned race_update_phase(const ZoomZooState& state) {
    const auto boundary = classic_race_scenario(state.track).initialization_frame;
    return (state.movement.frame + 1U - boundary) % 3U;
}

} // namespace

// $83E8E0-EC13 and $828953-89C2. Finish animation is a collision-pose input.
void update_zoom_finish(ZoomZooState& state, const ZoomZooContent& content) {
    auto& whole = state.movement;
    if (state.race.riders[0].finished) {
        if (state.race.finish_delay == 240)
            throw std::invalid_argument("race result loading outside frozen finish display");
        ++state.race.finish_delay;
    }
    for (unsigned index = 0; index < 2; ++index) {
        if (!state.race.riders[index].finished) continue;
        auto& input = state.reflection[index];
        input.brake_input = 1;
        input.jump_input = input.rotate_negative_input = input.rotate_positive_input = 0;
        if (index == 0)
            whole.player_input.horizontal = 1;
        else
            state.opponent_horizontal = 1;
        // $83:E90D-E915 (and $83:EA9F for the opponent) skip the rest while
        // bit 0 of $0304 is set. $0304 counts race updates 0, 1, 2
        // ($83:CCAB-CCB5) and is 0 at every captured boundary, so it is the
        // update's number from initialization mod 3 (R-0049); DRAGSTER and ZOOM
        // ZOO, whose boundaries are 2 mod 3, once read it from the frame.
        if (race_update_phase(state) == 1U) continue;
        apply_finish_slowdown(whole.riders[index]);
        const auto own = state.race.total_times[index], other = state.race.total_times[1 - index];
        const bool won = own != 0xea60U && (other == 0xea60U || own < other);
        const bool tied = own == other;
        auto& pose = state.race.finish_pose[index];
        if (pose.active) {
            if (index == 1)
                enqueue_zoom_opponent(whole, tied ? 38U : won ? 37U : 39U);
            else
                enqueue_zoom_player(state, tied ? 38U : won ? 37U : 39U);
        }
        const auto& queue = index == 1 ? whole.rewards : state.player_announcements.queue;
        if (!pose.active && (index == 1 || state.native_initialization)
            && ((queue.write_cursor - queue.read_cursor - 1U) & 31U) != 0)
            continue;
        // $828959-8965 waits for pending announcements before first activation.
        pose.active = 1;
        const unsigned kind = won || tied ? 1U : 2U;
        if (pose.kind != kind && !pose.locked) {
            pose.kind = static_cast<std::uint16_t>(kind);
            pose.selector = 0;
            pose.locked = 1;
        }
        const auto table = content_word(content.finish_poses, 2U * pose.kind) - 0xc7c8U;
        auto offset = table + 2U * pose.selector;
        ++pose.selector;
        auto value = content_word(content.finish_poses, offset);
        if (value & 0x8000U) {
            pose.selector =
                static_cast<std::uint16_t>(content_word(content.finish_poses, offset + 2));
            value = content_word(content.finish_poses, table + 2U * pose.selector);
        }
        input.pose_override = static_cast<std::uint16_t>(value);
    }
}

// $818050-82B6: ordered checkpoint tiles and three completed laps after the
// initial start-line crossing. SRAM slots retain original 0xEA60 sentinels.
void update_zoom_checkpoint(ZoomZooState& state, unsigned index, const ZoomZooContent& content) {
    auto& lap = state.race.riders[index];
    const auto& rider = state.movement.riders[index];
    if (lap.checkpoint_display_countdown) --lap.checkpoint_display_countdown;
    const auto descriptor = rider.contact.selected_word;
    const auto tile = ((descriptor & 0x3f0U) >> 2U) + ((descriptor & 15U) >> 1U);
    if (tile >= content.movement.flat_contact.flags.size())
        throw std::invalid_argument("ZOOM ZOO checkpoint tile flag is missing");
    if (rider.contact.auxiliary_flag || (content.movement.flat_contact.flags[tile] & 0xfeU) != 20
        || lap.finished)
        return;
    const unsigned tag = (descriptor & 0x1c00U) >> 10U;
    const unsigned checkpoint = tag <= 5 ? tag : 0;
    if (checkpoint == 0) {
        if (lap.start_line_latch) return;
        lap.start_line_latch = 1;
        const auto& timer = state.movement.timer;
        const auto hundredths =
            static_cast<std::uint16_t>(timer.subframe * 2U + state.movement.contact_phase);
        lap.time_digits = {timer.minutes, timer.tens_seconds, timer.seconds, timer.tenths,
                           hundredths};
        const auto total =
            static_cast<std::uint16_t>(timer.minutes * 6000U + timer.tens_seconds * 1000U
                                       + timer.seconds * 100U + timer.tenths * 10U + hundredths);
        std::uint16_t previous = 0;
        for (auto value : state.race.lap_times[index])
            if (value != 0xea60U) previous = add_word(previous, value);
        // $0D15 holds the initial laps + 1 ($81:D673); $81:8139-814A skips the
        // slot of the initial crossing and $81:8197-81A3 its display and
        // final-lap announcement.
        const int initial_laps_remaining = classic_race_scenario(state.track).laps + 1;
        const int slot = initial_laps_remaining - static_cast<int>(lap.laps_remaining) - 1;
        if (slot >= 0 && slot < 10)
            state.race.lap_times[index][static_cast<unsigned>(slot)] =
                static_cast<std::uint16_t>(total - previous);
        --lap.laps_remaining;
        const bool initial_crossing =
            initial_laps_remaining - static_cast<int>(lap.laps_remaining) == 1;
        if (!initial_crossing && lap.laps_remaining == 1
            && classic_race_scenario(state.track).tour_race) {
            if (index == 1)
                enqueue_zoom_opponent(state.movement, 15);
            else
                enqueue_zoom_player(state, 15);
        }
        if (!initial_crossing) {
            if (lap.laps_remaining == 0) {
                lap.finished = 1;
                state.race.total_times[index] = total;
            }
            lap.checkpoint_display_countdown = 120;
        }
        lap.checkpoint = 0;
        lap.next_checkpoint = 1;
    } else {
        if (checkpoint == 2) {
            if (!lap.start_line_latch) return;
            --lap.start_line_latch;
        } else if (checkpoint != lap.next_checkpoint)
            return;
        lap.checkpoint = static_cast<std::uint16_t>(checkpoint);
        lap.next_checkpoint = static_cast<std::uint16_t>((checkpoint + 1) & 3U);
        lap.checkpoint_display_countdown = 120;
        const auto seen_index = lap.laps_remaining * 4U + checkpoint;
        if (seen_index >= state.race.checkpoint_seen.size())
            throw std::invalid_argument("race checkpoint index invalid");
        auto& seen = state.race.checkpoint_seen[seen_index];
        if (seen & 0x80U) {
            seen = 0;
            if (index == 1) lap.checkpoint_display_countdown = 2;
        }
    }
}

// $83904A-90F0 publishes graph extrema at result update106. $80F88D
// publishes the two total times on update107. Prior track records are the
// authenticated fresh-scenario 60000 sentinel; they do not expand this range.
ZoomZooResult zoom_result_fields(const ZoomZooRaceState& race, unsigned updates, bool lap_graph) {
    ZoomZooResult result{};
    if (lap_graph && updates >= 106) {
        std::uint16_t minimum = 60000, maximum = 0;
        for (const auto& laps : race.lap_times)
            for (auto lap : laps)
                if (lap < 60000) {
                    minimum = std::min(minimum, lap);
                    maximum = std::max(maximum, lap);
                }
        if (static_cast<std::uint16_t>(maximum - minimum) < 200U)
            minimum = static_cast<std::uint16_t>(maximum - 200U);
        result.graph_minimum = minimum;
        result.graph_maximum = maximum;
    }
    // $80:F88D-F8A5 publishes the totals one load update later on the mode-0
    // (DRAGSTER) result screen: 108, observed at DRAGSTER load 3580 -> 3687.
    if (updates >= (lap_graph ? 107U : 108U)) result.published_totals = race.total_times;
    return result;
}

bool classic_race_player_won(const ZoomZooState& state) {
    // Finish order, as $83:E8E0-EC13 selects the finish pose: an equal time
    // means both crossed on one update, and the player is processed first.
    const auto own = state.race.total_times[0], other = state.race.total_times[1];
    return own != 0xea60U && (other == 0xea60U || own <= other);
}

std::uint16_t stable_result_updates(const ZoomZooState& state) {
    const auto scenario = classic_race_scenario(state.track);
    return classic_race_player_won(state) ? scenario.stable_result_won
                                          : scenario.stable_result_lost;
}

} // namespace unirally
