#include "menu_overruns.hpp"

#include <array>
#include <bit>

namespace unirally::front_end_screens {

namespace {

// A PAL frame is 312 lines of 1,364 master clocks from vblank's start, 40 of each line the work
// RAM's refresh, which the CPU waits through. A frame wait (`$80:FADF`) polls the sound queue
// (`JSL $82:8035`) before its first `$4212` read; a read after the frame's end, in the next
// vblank, waits out the next frame too. The models below time that first read.
constexpr std::uint32_t line_clocks = 1364, refresh_clocks = 40, frame_lines = 312;
constexpr std::uint32_t frame_clocks = line_clocks * frame_lines;
constexpr std::uint32_t nmi_clocks = 1060; // the menus' NMI handler, when the work runs into vblank

// CPU clocks to elapsed clocks: the refresh's 40 in every line.
constexpr std::uint32_t with_refresh(std::uint32_t cpu) {
    return cpu * line_clocks / (line_clocks - refresh_clocks);
}

// --- RECORDS' statistics ---

// `$83:97BD`'s CPU time in master clocks, step by step as the core runs it (R-0102,
// cycles_records.py): the routine and its sixteen riders' fixed path, each larger value stored
// (the eight `CMP`/`BCC`/`STA`s), each percentage `$83:986A` hands to `$83:9D80` (a rider with
// races), each halving of the part and the whole, and each quotient bit `$80:971D` sets.
constexpr std::uint32_t statistics_clocks = 32362;
constexpr std::uint32_t store_clocks = 34;
constexpr std::uint32_t percentage_clocks = 7704;  // $83:9878-9DB2 and back, with 16 division steps
constexpr std::uint32_t halving_clocks = 330;      // $83:9D85-9DA0
constexpr std::uint32_t quotient_bit_clocks = 112; // $80:973B-973F
// The JSL at `$80:D527` comes this long after vblank's start in OPTIONS' choice frame (8,994 to
// 9,596 over the seven entries measured); RECORDS' print and the way to the frame wait's first
// `$4212` read (`$80:FAE3`, the poll included) take the rest.
constexpr std::uint32_t statistics_start_clocks = 9300, records_print_clocks = 229474;

struct StatisticsCounts {
    std::uint32_t stores{}, percentages{}, halvings{}, quotient_bits{};
};

// `$83:9D80`: the part as a percentage of the whole, both halved until the part is below 655;
// `$80:971D` divides bit by bit (a whole of 0 gives 0xFFFF).
std::uint16_t count_percentage(std::uint16_t part, std::uint16_t whole, StatisticsCounts& counts) {
    constexpr std::uint16_t largest_part = 0x028e, percent = 100;
    ++counts.percentages;
    while (part > largest_part) {
        part = static_cast<std::uint16_t>(part >> 1U);
        whole = static_cast<std::uint16_t>(whole >> 1U);
        ++counts.halvings;
    }
    const auto scaled = static_cast<std::uint16_t>(part * percent);
    const auto quotient =
        whole == 0 ? std::uint16_t{0xffff} : static_cast<std::uint16_t>(scaled / whole);
    counts.quotient_bits += static_cast<std::uint32_t>(std::popcount(quotient));
    return quotient;
}

// `CMP`, `BCC` past the `STA` when the value is the smaller.
void keep_largest(std::uint16_t value, std::uint16_t& largest, StatisticsCounts& counts) {
    if (value < largest) return;
    largest = value;
    ++counts.stores;
}

// $83:97BD-9869: per rider (`$77:0230 + 8 x rider`: races, wins, then two more words) the
// largest races, losses (races less wins), wins and the two other words, then three percentages
// of the races (`$83:986A`: wins, losses and the third word; none without races, which leaves
// 0) against largest values that start at 0 and go back to 100 after each rider (`$83:984F`).
StatisticsCounts count_statistics(const OnePlayerRecords& records) {
    StatisticsCounts counts;
    std::array<std::uint16_t, 5> largest{};
    std::array<std::uint16_t, 3> largest_percentage{};
    for (const auto& stats : records.statistics) {
        const auto races = stats[0], wins = stats[1];
        const auto losses = static_cast<std::uint16_t>(races - wins);
        const std::array<std::uint16_t, 5> values{races, losses, wins, stats[2], stats[3]};
        for (std::size_t at = 0; at < values.size(); ++at)
            keep_largest(values[at], largest[at], counts);
        const std::array<std::uint16_t, 3> parts{wins, losses, stats[2]};
        for (std::size_t at = 0; at < parts.size(); ++at) {
            // `$83:986A` hands back the races, 0, for a rider without races.
            const auto percentage =
                races == 0 ? std::uint16_t{0} : count_percentage(parts[at], races, counts);
            keep_largest(percentage, largest_percentage[at], counts);
        }
        largest_percentage.fill(100);
    }
    return counts;
}

// --- A league pair's way out ---

// Measured over 27 way outs of three leagues and 13 of one with edited points (R-0102,
// trace_pairs.sh, the reviews' band): `$80:C786` is called this long after vblank (24,918-26,660),
// runs one checksum and one more for each time placed, and a lap race's best laps add to it; the
// count with its checksum. The standings' print to its frame wait: the title, the rows and each
// new rank (a row whose points differ from the row above's; its ordinal), fitted over 44 way outs
// (the band's 17 decide the edge, the leagues' largest error 1,490 clocks; they include the names
// of the measured leagues, the default riders 0, 2, 4, 6 and 8, so other names move them a
// little), the digit loops and the leading zeros' pass at their CPU cost a pass (from the
// instruction trace, `digit_steps.py`; the hundreds and above are taken as the tens' loop, which
// they repeat), the NMI when the print runs past a frame's end, and the wait's poll of the sound
// queue before its first `$4212` read (576-616 clocks traced after the print, review round 3).
constexpr std::uint32_t records_start_clocks = 25200;
constexpr std::uint32_t records_clocks = 227224, placing_clocks = 224765, lap_race_clocks = 4480;
constexpr std::uint32_t count_clocks = 226738;
constexpr std::uint32_t standings_title_clocks = 221930, standings_row_clocks = 39650;
constexpr std::uint32_t new_rank_clocks = 100;
constexpr std::uint32_t higher_digit_unit_clocks = 122; // $83:8C31-8C36, a pass
constexpr std::uint32_t unit_digit_unit_clocks = 90;    // $83:8C3B-8C3E, a pass
constexpr std::uint32_t leading_zero_clocks = 200;      // $83:8C64-8C76, a pass
constexpr std::uint32_t frame_wait_poll_clocks = 596;
// Elsewhere the print's JSR (`$80:BE57`) comes 15,484-25,010 clocks into its frame (`five`'s).
constexpr std::uint32_t standings_start_clocks = 25000;

// The clocks from vblank's start in the frame of `start` to the print's frame wait's first
// `$4212` read.
std::uint32_t standings_wait_clocks(std::uint32_t start, const StandingsWork& standings) {
    const auto digits = higher_digit_unit_clocks * standings.higher_digit_units
                      + unit_digit_unit_clocks * standings.unit_digit_units
                      + leading_zero_clocks * standings.leading_zeros;
    const auto wait = start + standings_title_clocks + standings_row_clocks * standings.rows
                    + new_rank_clocks * standings.new_ranks + with_refresh(digits);
    return wait + nmi_clocks * (wait / frame_clocks - start / frame_clocks)
         + frame_wait_poll_clocks;
}

} // namespace

std::uint8_t records_statistics_frames_late(const OnePlayerRecords& records) {
    const auto counts = count_statistics(records);
    const auto cpu = statistics_clocks + store_clocks * counts.stores
                   + percentage_clocks * counts.percentages + halving_clocks * counts.halvings
                   + quotient_bit_clocks * counts.quotient_bits;
    auto wait = statistics_start_clocks + with_refresh(cpu) + records_print_clocks;
    if (wait >= frame_clocks) wait += nmi_clocks;
    return static_cast<std::uint8_t>(wait / frame_clocks);
}

LeagueExitFrames league_exit_frames(const LeagueExitWork& work) {
    const auto counted = records_start_clocks + records_clocks
                       + placing_clocks * work.records_placed
                       + (work.lap_race ? lap_race_clocks : 0);
    const auto printed = standings_wait_clocks(counted + count_clocks, work.standings);
    // Exit frame 1 is `$80:C786`'s; each frame boundary the work passes moves what follows on.
    return {1 + counted / frame_clocks, 1 + printed / frame_clocks};
}

std::uint8_t standings_print_frames_late(const StandingsWork& standings) {
    return static_cast<std::uint8_t>(standings_wait_clocks(standings_start_clocks, standings)
                                     / frame_clocks);
}

} // namespace unirally::front_end_screens
