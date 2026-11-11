#include "menu_overruns.hpp"

#include <array>
#include <bit>
#include <utility>

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
// new rank (its ordinal, about 100 clocks fitted on the band), fitted over 51 way outs (the band's
// 24 decide the edge, the leagues' error -528 to +510 clocks; they include the names of the
// measured leagues, the default riders 0, 2, 4, 6 and 8, so other names move them a little); the
// sort's stores (55 clocks each, fitted on the band: the listing gives 58 for a lower word and 48
// for an equal one, which the band does not separate); the digit loops and the leading zeros' pass
// at their CPU cost a pass (from the instruction trace, `digit_steps.py`; the hundreds and above
// are taken as the tens' loop, which they repeat); the NMI when the print runs past a frame's end;
// and the wait's poll of the sound queue before its first `$4212` read (576-616 clocks traced after
// the print, review round 3); and the palette cycle's steps in the NMIs the work runs through.
constexpr std::uint32_t records_start_clocks = 25200;
constexpr std::uint32_t records_clocks = 227224, placing_clocks = 224765, lap_race_clocks = 4480;
constexpr std::uint32_t count_clocks = 226738;
constexpr std::uint32_t standings_title_clocks = 220340, standings_row_clocks = 39500;
constexpr std::uint32_t new_rank_clocks = 100;
constexpr std::uint32_t sort_store_clocks = 55;         // $80:EFE9-EFF4, a store
constexpr std::uint32_t higher_digit_unit_clocks = 122; // $83:8C31-8C36, a pass
constexpr std::uint32_t unit_digit_unit_clocks = 90;    // $83:8C3B-8C3E, a pass
constexpr std::uint32_t leading_zero_clocks = 200;      // $83:8C64-8C76, a pass
constexpr std::uint32_t frame_wait_poll_clocks = 596;
// The NMI that steps the palette cycle (`$00C8` run out), over an ordinary one: fitted on the 13
// league way outs that cross one (review round 5; their errors -744 to -1,384 before it).
constexpr std::uint32_t palette_step_clocks = 1100;
// Elsewhere the print's JSR (`$80:BE57`) comes 15,484-25,010 clocks into its frame (`five`'s).
constexpr std::uint32_t standings_start_clocks = 25000;

// Work from `start` that takes `clocks`, with every palette step in the NMIs it runs through: the
// work's first frame's NMI left the delay at `delay`, so the NMI k frame ends later steps when
// k = delay + 1, 8, ... (frame ends counted from the first frame's start).
std::uint32_t with_palette_steps(std::uint32_t start, std::uint32_t clocks, PaletteDelay delay) {
    const auto end = start + clocks;
    if (!delay) return end;
    constexpr std::uint32_t cycle_frames = 7; // the delay runs 6 down to 0, then steps
    std::uint32_t steps = 0;
    for (auto frame_end = start / frame_clocks + 1; frame_end <= end / frame_clocks; ++frame_end)
        if ((frame_end + cycle_frames - 1 - *delay) % cycle_frames == 0) ++steps;
    return end + palette_step_clocks * steps;
}

// The work's first frame starts after its NMI: a step there (the delay just set to 6) moves the
// start on.
std::uint32_t after_first_nmi(std::uint32_t start, PaletteDelay delay) {
    constexpr std::uint8_t just_stepped = 6;
    return start + (delay == just_stepped ? palette_step_clocks : 0);
}

// The clocks from vblank's start in the frame of `start` to the print's frame wait's entry.
std::uint32_t standings_wait_clocks(std::uint32_t start, const StandingsWork& standings) {
    const auto digits = higher_digit_unit_clocks * standings.higher_digit_units
                      + unit_digit_unit_clocks * standings.unit_digit_units
                      + leading_zero_clocks * standings.leading_zeros
                      + sort_store_clocks * standings.sort_stores;
    const auto wait = start + standings_title_clocks + standings_row_clocks * standings.rows
                    + new_rank_clocks * standings.new_ranks + with_refresh(digits);
    return wait + nmi_clocks * (wait / frame_clocks - start / frame_clocks);
}

// To the frame wait's first `$4212` read: the print, its palette steps, then the poll.
std::uint32_t standings_read_clocks(std::uint32_t start, const StandingsWork& standings,
                                    PaletteDelay delay) {
    const auto wait = standings_wait_clocks(start, standings);
    return with_palette_steps(start, wait - start, delay) + frame_wait_poll_clocks;
}

// --- TRACK RECORDS' grid halves ---

// The elapsed clock after `cpu` master clocks of CPU work from `clock` (both from vblank's start):
// each line the work RAM's refresh stops the CPU for 40 clocks at dot 538 and, on the visible
// lines (0-224, vblank starting at line 225), the grid's two HDMA channels for about 24 at dot
// 1,104 (fitted on the traces, R-0115).
constexpr std::uint32_t refresh_dot = 538, hdma_dot = 1104, grid_hdma_clocks = 24;
constexpr std::uint32_t vblank_line = 225;

std::uint32_t grid_clock_after(std::uint32_t clock, std::uint32_t cpu) {
    while (true) {
        const auto line_start = clock / line_clocks * line_clocks;
        const bool visible = (vblank_line + clock / line_clocks) % frame_lines < vblank_line;
        const std::array<std::pair<std::uint32_t, std::uint32_t>, 2> stops{
            {{refresh_dot, refresh_clocks}, {hdma_dot, visible ? grid_hdma_clocks : 0U}}};
        for (const auto& [dot, stopped] : stops) {
            if (clock >= line_start + dot) continue;
            if (clock + cpu <= line_start + dot) return clock + cpu;
            cpu -= line_start + dot - clock;
            clock = line_start + dot + stopped;
        }
        const auto next_line = line_start + line_clocks;
        if (clock + cpu <= next_line) return clock + cpu;
        cpu -= next_line - clock;
        clock = next_line;
    }
}

// CPU master clocks between the print's writes, each the mean of the traced prints' steps with
// the refresh and HDMA taken out (`grid_model.py --fit`). The half starts (`$80:ED1D`)
// 25,856-26,358 clocks into frame 63 (seven traced prints) and 8,826-9,386 into frame 86 (eight)
// when that frame's NMI does not step the palette cycle: the spread is how much of the frame
// wait's sound poll (`JSL $82:8035`) is left when vblank comes, which the model cannot know, so it
// takes the mean.
constexpr std::uint32_t first_half_start = 26110, second_half_start = 9164;
constexpr std::uint32_t grid_setup = 452;          // $80:ED1D-ED46
constexpr std::uint32_t tour_head = 3900;          // $80:ED46 to the picture's first row
constexpr std::uint32_t second_half_test = 60;     // $80:ED70-ED7C
constexpr std::uint32_t hunter_level_check = 2140; // $83:8DA4 JSL $83:9F14, tour 8's picture
constexpr unsigned hunter_tour = 8;
constexpr std::uint32_t picture_row = 554;        // $83:8DE5-8DF5, a row of five words
constexpr std::uint32_t picture_to_track = 1412;  // to `$80:EDB1` STA $CE
constexpr std::uint32_t track_to_medal = 1341;    // to the medal's `JSR $C3BC`
constexpr std::uint32_t printer_to_letter = 1669; // `$80:C3BC` to a letter's store (`$80:C58F`)
constexpr std::uint32_t next_letter = 1059;       // a medal's next letter
constexpr std::uint32_t letter_to_return = 703;   // the store to the printer's `RTS`
constexpr std::uint32_t medal_to_dot = 244;
constexpr std::uint32_t dot_without_icon = 241, dot_with_icon = 576; // $80:EEF2 to the printer
constexpr std::uint32_t between_columns = 298;                       // $80:EF1F-EF2F
constexpr std::uint32_t next_dot = 315, next_rank = 1886, next_tour = 413;

} // namespace

unsigned grid_half_steps_in_frame(const GridHalfWork& work) {
    const unsigned tours = work.second_half ? 4 : 5, first_tour = work.second_half ? 5 : 0;
    auto clock = after_first_nmi(work.second_half ? second_half_start : first_half_start,
                                 work.palette_delay);
    unsigned steps = 0;
    // Each write: the CPU work before it, then the write if the frame has not ended.
    const auto write_after = [&clock, &steps](std::uint32_t cpu) {
        clock = grid_clock_after(clock, cpu);
        if (clock >= frame_clocks) return false;
        ++steps;
        return true;
    };
    clock = grid_clock_after(clock, grid_setup);
    for (unsigned i = 0; i < tours; ++i) {
        const unsigned tour = first_tour + i;
        auto before = tour_head + (work.second_half ? second_half_test : 0U)
                    + (tour == hunter_tour ? hunter_level_check : 0U);
        for (unsigned row = 0; row < grid_picture_rows; ++row, before = picture_row)
            if (!write_after(before)) return steps;
        if (!write_after(picture_to_track)) return steps;
        before = track_to_medal;
        for (unsigned rank = 0; rank < grid_ranks; ++rank) {
            before += printer_to_letter;
            for (unsigned letter = 0; letter < grid_medal_letters[rank];
                 ++letter, before = next_letter)
                if (!write_after(before)) return steps;
            before = letter_to_return + medal_to_dot;
            for (unsigned track = 0; track < grid_tracks; ++track) {
                before += (work.icons[i][rank][track] ? dot_with_icon : dot_without_icon)
                        + printer_to_letter;
                if (!write_after(before)) return steps;
                if (!write_after(letter_to_return + between_columns + printer_to_letter))
                    return steps;
                before = letter_to_return + (track + 1 < grid_tracks ? next_dot : 0U);
            }
            before += next_rank;
        }
        clock = grid_clock_after(clock, before - next_rank + next_tour);
    }
    return steps;
}

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
    const auto delay = work.palette_delay;
    const auto counted = with_palette_steps(after_first_nmi(records_start_clocks, delay),
                                            records_clocks + placing_clocks * work.records_placed
                                                + (work.lap_race ? lap_race_clocks : 0),
                                            delay);
    const auto printed = standings_read_clocks(with_palette_steps(counted, count_clocks, delay),
                                               work.standings, delay);
    // Exit frame 1 is `$80:C786`'s; each frame boundary the work passes moves what follows on.
    return {1 + counted / frame_clocks, 1 + printed / frame_clocks};
}

std::uint8_t standings_print_frames_late(const StandingsWork& standings,
                                         PaletteDelay palette_delay) {
    const auto start = after_first_nmi(standings_start_clocks, palette_delay);
    return static_cast<std::uint8_t>(standings_read_clocks(start, standings, palette_delay)
                                     / frame_clocks);
}

} // namespace unirally::front_end_screens
