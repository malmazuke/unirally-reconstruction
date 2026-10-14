// Authored boundary checks for recovered league arithmetic (R-0073), without
// ROM content.
#include "front_end.hpp"
#include "front_end_screens.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace unirally;
using namespace unirally::front_end_screens;
void require(bool value, const char* description) {
    if (!value) throw std::runtime_error(std::string("league: ") + description);
}
struct Content {
    std::array<std::uint8_t, 256> characters{};
    std::array<std::uint8_t, 120> table{};
    std::array<std::uint8_t, 206> awards{};
    std::array<std::uint8_t, 32> palette{};
    std::array<std::uint8_t, 256> decoration_frames{};
    FrontEndContent view{};
    Content() {
        table.fill(0xff);
        awards.fill(0xff);
        constexpr std::array<std::uint8_t, 8> points{2, 3, 4, 5, 6, 7, 8, 10};
        for (unsigned k = 0; k < points.size(); ++k) awards[198 + k] = points[k];
        view.decoration_frames = decoration_frames;
        view.character_table = characters;
        view.league_table_text = table;
        view.league_awards_text = awards;
        for (auto& asset : view.assets) asset = palette;
    }
};
FrontEndState league(unsigned slot, std::uint16_t membership) {
    auto state = start_front_end();
    state.mode = FrontEndMode::league;
    state.league.slot = static_cast<std::uint8_t>(slot);
    state.records.league_members[slot] = membership;
    reset_league(state);
    return state;
}
void eight_members_and_integer_wrap(const Content& content) {
    auto state = league(5, 0xff00);
    auto& records = state.records;
    for (unsigned row = 0; row < 8; ++row) {
        require(records.league_pairings[5][row] == row, "equal-score census order");
        records.league_event_totals[5][row] = static_cast<std::uint16_t>(100 + row);
    }
    records.league_best_holder[5] = 3;
    records.league_played[5][0] = 0xffff;
    for (unsigned pair = 0; pair < 4; ++pair) {
        require(records.league_pair_cursor[5] == 2 * pair, "advance each disjoint pair");
        count_league_pair(state);
        finish_league_pair(state, content.view);
        if (pair < 3)
            require(state.screen == FrontEndScreen::league_table_entry,
                    "pending pair returns to table");
    }
    constexpr std::array<std::uint16_t, 8> expected{10, 8, 7, 7, 5, 4, 3, 2};
    require(records.league_scores[5] == expected, "eight-place awards and one bonus");
    require(records.league_played[5][0] == 0, "played wraps at 16 bits");
    for (unsigned row = 1; row < 8; ++row)
        require(records.league_played[5][row] == 1, "one race each");
    require(records.league_scores[0][0] == 0, "slot isolation");
    // R-0098: eight rows printed, so the second pass ends on frame 2 x 8 + 40.
    state.script_frame = 56;
    records.league_tracks[5] = 43;
    league_awards_frame(state, content.view, {});
    require(records.league_tracks[5] == 0 && records.league_pair_cursor[5] == 0,
            "track 43 wraps and cursor returns to first pair");
    require(records.league_cycle_complete, "wrapped tour has podium pending");
}
void odd_pair_and_result_order(const Content& content) {
    auto state = league(1, (0x8000U >> 7) | (0x8000U >> 9) | (0x8000U >> 11));
    state.rider_menu.rider = 7;
    state.second_rider = state.now_playing.opponent = 9;
    state.records.league_pairings[1][0] = 9;
    state.records.league_pairings[1][1] = 7;
    state.race_result.times = {3293, 3290, false};
    state.race_result.times.league_tricks = {5, 3};
    record_league_result(state);
    store_league_race_totals(state); // with the result's rows (R-0098)
    require(state.records.league_event_totals[1][0] == 3290
                && state.records.league_event_totals[1][1] == 3293,
            "totals follow pairing order");
    require(state.records.league_best_holder[1] == 7, "bonus follows physical player");
    restore_league_pair(state);
    require(state.rider_menu.rider == 9 && state.second_rider == 7
                && state.race_result.times.player_total == 3290,
            "result swaps whole pairing");
    count_league_pair(state);
    finish_league_pair(state, content.view);
    require(state.records.league_pair_cursor[1] == 2 && state.records.league_scores[1][0] == 0,
            "odd event awaits final human");
    state.rider_menu.rider = 11;
    state.second_rider = state.now_playing.opponent = someone;
    state.race_result.times = {5277, 3358, false};
    record_league_result(state);
    store_league_race_totals(state);
    count_league_pair(state);
    finish_league_pair(state, content.view);
    require(state.records.league_scores[1][0] == 9 && state.records.league_scores[1][1] == 10
                && state.records.league_scores[1][2] == 7,
            "CPU time does not take human award place");
    require(state.records.league_played[1][2] == 1, "odd human counted once");
}
void bonus_comparisons() {
    auto state = league(0, 0x0500);
    state.rider_menu.rider = 7;
    state.second_rider = state.now_playing.opponent = 9;
    state.race_result.times = {1, 1, true};
    state.race_result.times.player_laps[0] = state.race_result.times.opponent_laps[0] = 200;
    state.records.league_best[0] = 0xea62;
    record_league_result(state);
    require(state.records.league_best[0] == 200 && state.records.league_best_holder[0] == 7,
            "best-lap equality keeps first holder");
    state.race_result.times.stunt_event = true;
    state.race_result.times.lap_race = false;
    state.race_result.times.league_wipeouts = {2, 2};
    state.records.league_best[0] = 10;
    record_league_result(state);
    require(state.records.league_best[0] == 2 && state.records.league_best_holder[0] == 9,
            "fewest-wipeout equality replaces with later human");
}
// R-0098: a stunt event's totals wait for the end of its tallies; against the computer only the
// player's word is stored (`$80:F275-F27F`, Y = 0xFFFF).
void stunt_totals_after_the_tallies() {
    auto state = league(2, 0x0500);
    state.rider_menu.rider = 7;
    state.second_rider = state.now_playing.opponent = 9;
    state.records.league_pairings[2][0] = 7;
    state.records.league_pairings[2][1] = 9;
    state.records.league_event_totals[2] = {11, 12, 13, 0, 0, 0, 0, 0};
    state.race_result.times = {60000, 60000, false};
    state.race_result.times.stunt_event = true;
    state.race_result.times.player_score = 21;
    state.race_result.times.opponent_score = 22;
    record_league_result(state);
    require(state.records.league_event_totals[2][0] == 11, "no totals as the result starts");
    store_league_stunt_totals(state, false);
    require(state.records.league_event_totals[2][0] == 21
                && state.records.league_event_totals[2][1] == 22,
            "both scores after rider 1's tally");
    state.records.league_pair_cursor[2] = 2;
    state.race_result.times.player_score = 31;
    store_league_stunt_totals(state, true);
    require(state.records.league_event_totals[2][2] == 31
                && state.records.league_event_totals[2][3] == 0,
            "against the computer only the player's word");
}
} // namespace
int main() try {
    const Content content;
    eight_members_and_integer_wrap(content);
    odd_pair_and_result_order(content);
    bonus_comparisons();
    stunt_totals_after_the_tallies();
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}
