#pragma once
// Two menu passes whose CPU work can run past the frame they start in, so the frame wait after
// them comes a frame later (R-0102). Each is a cost model of the original's routines (master
// clocks per step counted from the reference core's instruction trace), not a CPU model.
#include "front_end.hpp"

#include <cstdint>

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
    std::uint32_t standings{}; // the exit frame the standings' print ends with its frame wait
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
};

struct LeagueExitWork {
    unsigned records_placed{}; // times placed in the track's top three
    bool lap_race{};           // `$80:C868`'s best laps take longer than `$80:C7C4`'s totals
    StandingsWork standings{}; // as the count leaves them
};

LeagueExitFrames league_exit_frames(const LeagueExitWork& work);

// The standings' print (`$80:A547`) where it starts a frame's work (from PICK TOUR, the slots,
// NOW PLAYING's Y and the awards' way out): the frames it runs past that frame before the frame
// wait, 1 for five or more rows (`five`: every such print), 0 for fewer.
std::uint8_t standings_print_frames_late(const StandingsWork& standings);

} // namespace unirally::front_end_screens
