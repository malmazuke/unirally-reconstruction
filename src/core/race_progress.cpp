// Race progress: checkpoints, laps, the finish and the result fields.
//
// A lap is the start line (checkpoint 0) then checkpoints 1-3 in order; the first crossing
// of the start line only starts the race. Each lap's time goes to its slot, and the last
// lap finishes the rider. A finished rider brakes and, once the announcements before it have
// shown, cycles its finish pose while its result (winner, draw or loser) is announced.

#include "race_progress.hpp"

#include "announcements.hpp"
#include "reward_queue.hpp"
#include "rider_motion.hpp"
#include "word_arithmetic.hpp"
#include "zoom_zoo_movement.hpp"

#include <algorithm>
#include <cstdint>
#include <stdexcept>

namespace unirally {
namespace {

// A lap slot or total not run holds 60000 (0xEA60), as the cartridge's records do.
constexpr std::uint16_t no_time = 0xea60;
constexpr std::uint16_t finish_display_updates = 240, checkpoint_display_updates = 120;
constexpr unsigned checkpoint_tile_pair = 20, checkpoints_per_lap = 4, lap_slots = 10;
// The checkpoint tile's number is the descriptor's bits 10-12; 0 is the start line and
// numbers above 5 count as it too.
constexpr std::uint16_t checkpoint_bits = 0x1c00;
constexpr unsigned checkpoint_shift = 10, highest_checkpoint_tag = 5, halfway_checkpoint = 2;
// The finish pose tables: two word pointers into the 0xC7C8-based table, entries with bit
// 15 set jump back to an earlier selector.
constexpr unsigned finish_pose_table_base = 0xc7c8;
constexpr std::uint16_t finish_pose_jump = 0x8000;
constexpr unsigned winner_pose = 1, loser_pose = 2;
// $83:904A-90F0 publishes the lap graph's extrema at result update 106, over at least 200
// hundredths; $80:F88D publishes the totals at 107 (108 on the mode-0 DRAGSTER screen).
constexpr unsigned graph_update = 106, totals_update = 107, dragster_totals_update = 108;
constexpr std::uint16_t smallest_graph_range = 200;

// $0304 during the update that produces frame whole.frame + 1: the update's number counted
// from the race's initialization boundary, mod 3.
unsigned race_update_phase(const ZoomZooState& state) {
    const auto boundary = classic_race_scenario(state.track).initialization_frame;
    return (state.movement.frame + 1U - boundary) % 3U;
}

bool finished_first(std::uint16_t own, std::uint16_t other) {
    return own != no_time && (other == no_time || own < other);
}

void announce(ZoomZooState& state, unsigned rider, unsigned event) {
    if (rider == 1)
        queue_opponent_announcement(state.movement, event);
    else
        queue_player_announcement(state, event);
}

// $82:8953-89C2: the finish pose steps through its table each update; an entry with bit 15
// set holds the selector to jump back to.
void step_finish_pose(ZoomZooFinishPose& pose, ReflectionTransition& input,
                      const ZoomZooContent& content) {
    const auto table = content_word(content.finish_poses, 2U * pose.kind) - finish_pose_table_base;
    const auto offset = table + 2U * pose.selector;
    ++pose.selector;
    auto value = content_word(content.finish_poses, offset);
    if (value & finish_pose_jump) {
        pose.selector = static_cast<std::uint16_t>(content_word(content.finish_poses, offset + 2));
        value = content_word(content.finish_poses, table + 2U * pose.selector);
    }
    input.pose_override = static_cast<std::uint16_t>(value);
}

// One finished rider's update: it brakes with the D-pad centred; on two updates in three
// it slows and, once the announcements queued before it have shown ($82:8959-8965), takes
// its finish pose (winner's or loser's, chosen once) and announces its result each update.
void finish_rider(ZoomZooState& state, unsigned index, const ZoomZooContent& content) {
    auto& whole = state.movement;
    auto& input = state.reflection[index];
    input.brake_input = 1;
    input.jump_input = input.rotate_negative_input = input.rotate_positive_input = 0;
    if (index == 0)
        whole.player_input.horizontal = direction::neutral;
    else
        state.opponent_horizontal = direction::neutral;
    // $83:E90D-E915 (and $83:EA9F for the opponent) skip the rest while bit 0 of $0304 is
    // set. $0304 counts race updates 0, 1, 2 ($83:CCAB-CCB5) and is 0 at every captured
    // boundary, so it is the update's number from initialization mod 3 (R-0049); DRAGSTER
    // and ZOOM ZOO, whose boundaries are 2 mod 3, once read it from the frame.
    if (race_update_phase(state) == 1U) return;
    apply_finish_slowdown(whole.riders[index]);
    const auto own = state.race.total_times[index], other = state.race.total_times[1 - index];
    const bool won = finished_first(own, other);
    const bool tied = own == other;
    auto& pose = state.race.finish_pose[index];
    if (pose.active)
        announce(state, index,
                 tied  ? announcement::draw
                 : won ? announcement::winner
                       : announcement::loser);
    const auto& queue = index == 1 ? whole.rewards : state.player_announcements.queue;
    if (!pose.active && (index == 1 || state.native_initialization)
        && ((queue.write_cursor - queue.read_cursor - 1U) & 31U) != 0)
        return;
    pose.active = 1;
    const unsigned kind = won || tied ? winner_pose : loser_pose;
    if (pose.kind != kind && !pose.locked) {
        pose.kind = static_cast<std::uint16_t>(kind);
        pose.selector = 0;
        pose.locked = 1;
    }
    step_finish_pose(pose, input, content);
}

// The start line: its first crossing starts the race; each later one records the lap's time
// in its slot (the crossing's hundredths come from the contact phase), announces the last
// lap on a tour race ($81:8197-81A3), and on the last lap finishes the rider. $0D15 holds
// the initial laps + 1 ($81:D673); $81:8139-814A skips the initial crossing's slot.
void cross_start_line(ZoomZooState& state, unsigned index) {
    auto& lap = state.race.riders[index];
    if (lap.start_line_latch) return;
    lap.start_line_latch = 1;
    const auto& timer = state.movement.timer;
    const auto hundredths =
        static_cast<std::uint16_t>(timer.subframe * 2U + state.movement.contact_phase);
    lap.time_digits = {timer.minutes, timer.tens_seconds, timer.seconds, timer.tenths, hundredths};
    const auto total =
        static_cast<std::uint16_t>(timer.minutes * 6000U + timer.tens_seconds * 1000U
                                   + timer.seconds * 100U + timer.tenths * 10U + hundredths);
    std::uint16_t previous = 0;
    for (auto value : state.race.lap_times[index])
        if (value != no_time) previous = add_word(previous, value);
    const auto scenario = classic_race_scenario(state.track);
    const int initial_laps_remaining = scenario.laps + 1;
    const int slot = initial_laps_remaining - static_cast<int>(lap.laps_remaining) - 1;
    if (slot >= 0 && slot < static_cast<int>(lap_slots))
        state.race.lap_times[index][static_cast<unsigned>(slot)] =
            static_cast<std::uint16_t>(total - previous);
    --lap.laps_remaining;
    const bool initial_crossing =
        initial_laps_remaining - static_cast<int>(lap.laps_remaining) == 1;
    if (!initial_crossing && lap.laps_remaining == 1 && scenario.tour_race)
        announce(state, index, announcement::last_lap);
    if (!initial_crossing) {
        if (lap.laps_remaining == 0) {
            lap.finished = 1;
            state.race.total_times[index] = total;
        }
        lap.checkpoint_display_countdown = checkpoint_display_updates;
    }
    lap.checkpoint = 0;
    lap.next_checkpoint = 1;
}

// A checkpoint counts only in order; checkpoint 2 also releases the start line. The first
// time a lap's checkpoint is seen clears its flag (0xFF at the start), which shortens the
// opponent's display.
void pass_checkpoint(ZoomZooState& state, unsigned index, unsigned checkpoint) {
    auto& lap = state.race.riders[index];
    if (checkpoint == halfway_checkpoint) {
        if (!lap.start_line_latch) return;
        --lap.start_line_latch;
    } else if (checkpoint != lap.next_checkpoint) {
        return;
    }
    lap.checkpoint = static_cast<std::uint16_t>(checkpoint);
    lap.next_checkpoint = static_cast<std::uint16_t>((checkpoint + 1) & 3U);
    lap.checkpoint_display_countdown = checkpoint_display_updates;
    const auto seen_index = lap.laps_remaining * checkpoints_per_lap + checkpoint;
    if (seen_index >= state.race.checkpoint_seen.size())
        throw std::invalid_argument("race checkpoint index invalid");
    auto& seen = state.race.checkpoint_seen[seen_index];
    if (seen & 0x80U) {
        seen = 0;
        if (index == 1) lap.checkpoint_display_countdown = 2;
    }
}

} // namespace

// $83:E8E0-EC13 and $82:8953-89C2: the finish display counts to 240 once the player has
// finished, and each finished rider brakes and poses. The finish pose feeds the collision
// sample.
void update_finish(ZoomZooState& state, const ZoomZooContent& content) {
    if (state.race.riders[0].finished) {
        if (state.race.finish_delay == finish_display_updates)
            throw std::invalid_argument("race result loading outside frozen finish display");
        ++state.race.finish_delay;
    }
    for (unsigned index = 0; index < 2; ++index)
        if (state.race.riders[index].finished) finish_rider(state, index, content);
}

// $81:8050-82B6: the checkpoint tile under a rider (flag pair 20), unless the rider has
// finished.
void update_checkpoints(ZoomZooState& state, unsigned index, const ZoomZooContent& content) {
    auto& lap = state.race.riders[index];
    const auto& rider = state.movement.riders[index];
    if (lap.checkpoint_display_countdown) --lap.checkpoint_display_countdown;
    const auto descriptor = rider.contact.selected_word;
    const auto tile = ((descriptor & 0x3f0U) >> 2U) + ((descriptor & 15U) >> 1U);
    if (tile >= content.movement.flat_contact.flags.size())
        throw std::invalid_argument("ZOOM ZOO checkpoint tile flag is missing");
    if (rider.contact.auxiliary_flag
        || (content.movement.flat_contact.flags[tile] & 0xfeU) != checkpoint_tile_pair
        || lap.finished)
        return;
    const unsigned tag = (descriptor & checkpoint_bits) >> checkpoint_shift;
    const unsigned checkpoint = tag <= highest_checkpoint_tag ? tag : 0;
    if (checkpoint == 0)
        cross_start_line(state, index);
    else
        pass_checkpoint(state, index, checkpoint);
}

// The result screen's fields at load update `updates`. Prior track records are the fresh
// scenario's 60000; they do not widen the graph.
ZoomZooResult result_fields(const ZoomZooRaceState& race, unsigned updates, bool lap_graph) {
    ZoomZooResult result{};
    if (lap_graph && updates >= graph_update) {
        std::uint16_t minimum = no_time, maximum = 0;
        for (const auto& laps : race.lap_times)
            for (auto lap : laps)
                if (lap < no_time) {
                    minimum = std::min(minimum, lap);
                    maximum = std::max(maximum, lap);
                }
        if (static_cast<std::uint16_t>(maximum - minimum) < smallest_graph_range)
            minimum = static_cast<std::uint16_t>(maximum - smallest_graph_range);
        result.graph_minimum = minimum;
        result.graph_maximum = maximum;
    }
    // $80:F88D-F8A5: observed at DRAGSTER load 3580 -> 3687.
    if (updates >= (lap_graph ? totals_update : dragster_totals_update))
        result.published_totals = race.total_times;
    return result;
}

bool classic_race_player_won(const ZoomZooState& state) {
    // Finish order, as $83:E8E0-EC13 selects the finish pose: an equal time means both
    // crossed on one update, and the player is processed first.
    const auto own = state.race.total_times[0], other = state.race.total_times[1];
    return own != no_time && (other == no_time || own <= other);
}

std::uint16_t stable_result_updates(const ZoomZooState& state) {
    const auto scenario = classic_race_scenario(state.track);
    return classic_race_player_won(state) ? scenario.stable_result_won
                                          : scenario.stable_result_lost;
}

} // namespace unirally
