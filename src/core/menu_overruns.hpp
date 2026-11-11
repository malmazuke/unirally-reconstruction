#pragma once
// Menu passes whose CPU work can run past the frame they start in, so the frame wait after them
// comes a frame later (R-0102), or the work RAM at that frame's end holds only part of a print
// (R-0115). Each is a cost model of the original's routines (master clocks per step counted from
// the reference core's instruction trace), not a CPU model.
#include "front_end.hpp"

#include <array>
#include <cstdint>
#include <optional>

namespace unirally::front_end_screens {

// RECORDS' entry (`$80:D525-D53C`): `$83:97BD` takes every rider's largest statistics and 48
// percentages (`$83:9D80`, a software division each) before RECORDS' choices are printed. The
// frames it and the print run past OPTIONS' choice frame before the frame wait: 0 for sparse
// statistics, 1 for dense ones (review capture `wr-stats2`).
std::uint8_t records_statistics_frames_late(const OnePlayerRecords& records);

// A league pair's way out after `$80:F4B8`'s wait (exit frame 1): the records (`$80:C786`, a
// checksum run, `$83:90F4`, and another for each time placed), the pair's count (`$80:AF54`,
// with its own checksum run when another pair races) and, with another pair to race, the
// standings' print (`$80:A547`), all without a frame wait.
struct LeagueExitFrames {
    std::uint32_t count{};     // the exit frame the count (the played words) is made in
    std::uint32_t standings{}; // the exit frame of the standings' frame wait's first `$4212` read
};

// What the standings' print (`$80:A547`) costs by: its rows and the digits of every played and
// points word it prints. `$83:8BE7` turns a word into decimal by subtracting each power of ten
// once per unit of its digit (`$83:8BFE-8C36`), counts the units down (`$83:8C3B-8C3E`) and blanks
// each leading zero of the five digits but the last (`$83:8C64-8C76`).
struct StandingsWork {
    unsigned rows{};
    unsigned higher_digit_units{}; // the tens and above, summed
    unsigned unit_digit_units{};   // the units, summed
    unsigned leading_zeros{};      // 4 for a one-digit word, 3 for two digits, ...
    // The distinct points less one: as the rows print in points order, the rows whose points
    // differ from the row above's, each a new rank (`$80:A627`).
    unsigned new_ranks{};
    // The selection sort's stores of the lowest so far (`$80:EFE9-EFF4`), over all eight rows: 36
    // when the rows already stand in points order, fewer in another order.
    unsigned sort_stores{};
};

// The palette cycle's delay `$00C8` (6 down to 0) as the work's first frame's NMI leaves it, or
// no value when the cycle is not running: the NMI after which the delay was 0 steps the cycle and
// runs about 1,100 clocks longer, which pushes everything after it on (review round 5).
using PaletteDelay = std::optional<std::uint8_t>;

// The palette cycle's delay as this frame's NMI left it, while the cycle runs (R-0102).
inline PaletteDelay palette_delay(const FrontEndState& state) {
    if (!state.cycle.running) return std::nullopt;
    return static_cast<std::uint8_t>(state.cycle.delay);
}

struct LeagueExitWork {
    unsigned records_placed{}; // times placed in the track's top three
    bool lap_race{};           // `$80:C868`'s best laps take longer than `$80:C7C4`'s totals
    StandingsWork standings{}; // as the count leaves them
    PaletteDelay palette_delay{};
};

LeagueExitFrames league_exit_frames(const LeagueExitWork& work);

// The standings' print (`$80:A547`) where it starts a frame's work (from PICK TOUR, the slots,
// NOW PLAYING's Y and the awards' way out): the frames it runs past that frame before the frame
// wait, 1 for five or more rows (`five`: every such print), 0 for fewer.
std::uint8_t standings_print_frames_late(const StandingsWork& standings,
                                         PaletteDelay palette_delay);

// TRACK RECORDS' grid halves (`$80:ED1D`, R-0100, R-0115): each half runs on past the frame it
// starts in (entry frames 63 and 86), so the work RAM at that frame's end holds the grid only as
// far as the print had come. The print's writes, in the original's order, are its steps: for each
// tour of the half its picture's five rows (`$83:8DE5-8DF5`), the chosen track `$00CE` (`$80:EDB1`),
// then for each rank the medal's name a letter at a time and the five tracks' record dots a column
// at a time (`$80:EEF2`, two calls of the printer `$80:C3BC` each).
inline constexpr unsigned grid_picture_rows = 5, grid_ranks = 3, grid_tracks = 5;
// The letters of the medals' names: gold, silver, bronze.
inline constexpr std::array<unsigned, grid_ranks> grid_medal_letters{4, 6, 6};

struct GridHalfWork {
    bool second_half{}; // tours 5-8 (frame 86), not 0-4 (frame 63)
    // By tour of the half, rank and track: the record's holder has an icon (below 16), whose dot
    // takes `$80:EEFA-EF13`'s longer path (the colour from `$80:EF3D`).
    std::array<std::array<std::array<bool, grid_tracks>, grid_ranks>, 5> icons{};
    PaletteDelay palette_delay{};
};

// The steps of the half's print the original has done when its frame ends (of 52 a tour): a cost
// model in master clocks fitted on four traced prints and checked on sixteen (R-0115), not a CPU
// model.
unsigned grid_half_steps_in_frame(const GridHalfWork& work);

} // namespace unirally::front_end_screens
