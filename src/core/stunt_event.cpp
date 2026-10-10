// The stunt event (race mode 2, R-0066): a 45-second solo run for points.
//
// The race engine runs it as a race with the opponent switched off (race_update.cpp) and these
// rules added: the clock counts down from the track header's time, and when it runs out each
// rider finishes on the first update it is supported; the tricks the player's queue shows are
// tallied by family and count for the stunt result; the finish's caption compares the score
// with the qualifying score instead of the times.

#include "stunt_event.hpp"

#include "announcements.hpp"
#include "race_sound.hpp"
#include "word_arithmetic.hpp"

#include <cstdint>
#include <stdexcept>

namespace unirally {
namespace {

// A tenth every five updates, as the race clock counts up ($81:C7DE).
constexpr std::uint16_t updates_per_tenth = 5;
constexpr std::uint16_t highest_tenth = 9, highest_second = 9, highest_ten_seconds = 5;
// $81:C838-C846: supported means fewer than two updates unsupported; falling fast means a
// vertical velocity below -256 (0xFF00).
constexpr std::uint16_t unsupported_limit = 2, fastest_finishing_fall = 0xff00;
// $83:E85C-E85F: a rider stands on the ground with no update unsupported; a settled rider's
// vertical velocity is held at 128 ($83:E856-E859).
constexpr std::uint16_t settled_fall = 0x80;
// A reward class is twice the tally's column index: eight classes to a family's row of four
// columns (classes 0-34 occur; the class table's 255 is no class).
constexpr unsigned classes_per_family = 8, classes_per_column = 2;

// One digit counts down; below 0 it takes its highest value and the next digit borrows
// ($81:C7F4-C82B, DEY and BMI). Returns whether it borrowed.
bool count_digit_down(std::uint16_t& digit, std::uint16_t highest) {
    if (digit != 0) {
        --digit;
        return false;
    }
    digit = highest;
    return true;
}

// $81:C7D7-C833: a tenth off the clock every five updates. Returns true on the tick after
// 0:00.0, which clamps the minutes at 0 ($81:C830) and leaves the other digits at 5, 9 and 9;
// the HUD keeps showing 0:00.0.
bool count_down_tenth(RaceTimerDigits& clock) {
    if (++clock.subframe != updates_per_tenth) return false;
    clock.subframe = 0;
    if (!count_digit_down(clock.tenths, highest_tenth)
        || !count_digit_down(clock.seconds, highest_second)
        || !count_digit_down(clock.tens_seconds, highest_ten_seconds))
        return false;
    if (clock.minutes != 0) {
        --clock.minutes;
        return false;
    }
    return true;
}

// $81:C8D3-C8ED: a tick that leaves 0:05.0, 0:04.0 ... 0:00.0 on the clock sounds its warning
// (`CMP #6`, BPL: whole seconds 0-5 with no minutes, tens or tenths).
bool warns_last_seconds(const RaceTimerDigits& clock) {
    constexpr std::uint16_t warned_seconds = 6;
    return !clock.minutes && !clock.tens_seconds && !clock.tenths && clock.seconds < warned_seconds;
}

// $83:E85C-E862: CMP #1 and BPL, so a count of 0 (or one past 0x8000).
bool stands_on_ground(const RiderMovementState& rider) {
    return negative(static_cast<std::uint16_t>(rider.contact.unsupported_count - 1U));
}

bool queue_empty(const RewardQueueState& queue) {
    return ((queue.read_cursor + 1U) & 31U) == queue.write_cursor;
}

} // namespace

unsigned rider_passes(const ClassicRaceScenario& scenario, bool two_human) {
    return scenario.stunt_event && !two_human ? 1U : 2U;
}

bool update_stunt_finish(ZoomZooState& state) {
    auto& stunt = state.stunt;
    const auto& riders = state.movement.riders;
    for (unsigned index = 0; index < 2; ++index) {
        if (!state.race.riders[index].finished) continue;
        if (stunt.settled[index]) state.movement.riders[index].motion.velocity_y = settled_fall;
        if (stands_on_ground(riders[index])) stunt.settled[index] = 1;
    }
    if (stunt.finish_display) return true;
    if (!state.race.riders[0].finished || !state.race.riders[1].finished
        || !stands_on_ground(riders[0]) || !stands_on_ground(riders[1]))
        return false;
    // $83:E8B6 clears $12E3 and $12E5; $12E5, a second human's hints, is only ever set with one
    // ($82:D96F), so one player has only $12E3.
    state.player_announcements.hints_active = 0;
    state.opponent_hints.active = false;
    if (queue_empty(state.player_announcements.queue) && queue_empty(state.movement.rewards))
        stunt.finish_display = 1; // $83:E8DA
    return false;
}

bool stunt_rider_can_finish(const RiderMovementState& rider) {
    const auto unsupported = rider.contact.unsupported_count;
    return negative(static_cast<std::uint16_t>(unsupported - unsupported_limit))
        && !negative(static_cast<std::uint16_t>(rider.motion.velocity_y - fastest_finishing_fall));
}

void update_stunt_clock(ZoomZooState& state, bool running) {
    auto& stunt = state.stunt;
    auto& clock = state.movement.timer;
    // $81:C7D2-C7D5: once stopped, the clock only tests the riders.
    if (!stunt.clock_stopped) {
        if (!running) return;
        if (!count_down_tenth(clock)) {
            // A tick leaves the update counter at 0 ($81:C7E9).
            if (clock.subframe == 0 && warns_last_seconds(clock))
                race_sound::effect(state, race_sound::clock_warning);
            return;
        }
    }
    for (unsigned index = 0; index < 2; ++index)
        if (stunt_rider_can_finish(state.movement.riders[index]))
            state.race.riders[index].finished = 1; // $0EFF/$0F01; the times stay 60000
    stunt.clock_stopped = 1;                       // $81:C864-C867
}

void tally_stunt_trick(StuntTallies& tallies, std::uint8_t trick_class, std::uint8_t paid) {
    const unsigned family = trick_class / classes_per_family;
    const unsigned column = trick_class % classes_per_family / classes_per_column;
    if (family >= trick_family::count || trick_class % classes_per_column != 0)
        throw std::invalid_argument("a trick's reward class lies outside the stunt tallies");
    auto& tally = tallies[family][column];
    tally.shown = static_cast<std::uint8_t>(tally.shown + 1U); // one byte ($81:C111-C116)
    tally.points = add_word(tally.points, paid);
}

std::uint8_t stunt_finish_announcement(std::uint16_t score, std::uint16_t qualifying_score) {
    if (score == qualifying_score) return announcement::draw;
    return negative(static_cast<std::uint16_t>(score - qualifying_score)) ? announcement::loser
                                                                          : announcement::winner;
}

} // namespace unirally
