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
    std::array<std::uint8_t, 1> main_menu{0xff};
    FrontEndContent view{};
    Content() {
        table.fill(0xff);
        view.main_menu_text = main_menu;
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
// R-0099: the podium's sound session on its entry's frame 19 ($82:E06F), then the loop's first
// three passes each send effect gain 255, music gain 127 and the music start with eight polls
// ($82:DEBC-DEFE); `$1365` counts on to 8 and stops. The poses' pictures need ROM content, so the
// later passes run as the entry's last frame does (no picture upload).
AudioCueList podium_entry_cues(FrontEndState& state, const Content& content, std::uint32_t frame,
                               FrontEndPads pads = {}) {
    state.sound_cues.clear();
    state.script_frame = frame;
    league_podium_entry_frame(state, content.view, pads);
    return state.sound_cues;
}
AudioCueList podium_pass_cues(FrontEndState& state, const Content& content, FrontEndPads pads) {
    state.sound_cues.clear();
    state.screen = FrontEndScreen::league_podium_entry;
    ++state.script_frame;
    league_podium_frame(state, content.view, pads);
    return state.sound_cues;
}
// R-0099: the loop leaves on A, X, L, R, B, Y, Select or Start of either pad, or Left or Right held
// on either pad ($82:DE10-DE5B); Up and Down never count (rv-up-pad2).
void podium_exit_test(const Content& content) {
    constexpr std::uint16_t up = 0x0800, down = 0x0400, left = 0x0200, right = 0x0100;
    constexpr std::array<std::uint16_t, 10> leaving{0x8000, 0x4000, 0x2000, 0x1000, left,
                                                    right,  0x0080, 0x0040, 0x0020, 0x0010};
    const auto leaves = [&](FrontEndPads pads) {
        auto state = league(3, 0xe000);
        enter_league_podium(state);
        for (std::uint32_t frame = 1; frame <= 99; ++frame)
            podium_entry_cues(state, content, frame);
        podium_pass_cues(state, content, pads);
        return state.screen == FrontEndScreen::league_podium_exit;
    };
    for (const auto button : leaving)
        require(leaves({button, 0}) && leaves({0, button}), "either pad's exit buttons leave");
    require(!leaves({up, 0}) && !leaves({0, up}) && !leaves({down, 0}) && !leaves({0, down})
                && !leaves({up | down, up}),
            "Up and Down never leave");
    require(!leaves({}), "no press stays");
}
void podium_music(const Content& content) {
    auto state = league(3, 0xe000);
    enter_league_podium(state);
    state.league.podium_music_passes = 5; // a previous podium's count does not carry over
    enter_league_podium(state);
    const auto music = [](AudioDispatchSite site) {
        AudioCueList cues{audio_enqueue(8, 255), audio_enqueue(7, 127), audio_enqueue(1, 2)};
        for (unsigned call = 0; call < 8; ++call) cues.push_back(audio_dispatch(site));
        return cues;
    };
    for (std::uint32_t frame = 1; frame <= 98; ++frame) {
        const auto cues = podium_entry_cues(state, content, frame);
        if (frame == 7)
            require(cues
                        == AudioCueList{audio_enqueue(3, 0x90),
                                        audio_dispatch(AudioDispatchSite::race_choice)},
                    "the menu music fades on the entry's frame 7");
        else if (frame == 19)
            require(cues == AudioCueList{audio_load(AudioSessionLoad::podium)},
                    "the podium's session on the entry's frame 19");
        else
            require(cues.empty(), "no other sound while the podium loads");
    }
    require(podium_entry_cues(state, content, 99) == music(AudioDispatchSite::podium_first_pass),
            "the first pass starts the music");
    require(state.screen == FrontEndScreen::league_podium, "the loop runs from frame 99");
    require(podium_pass_cues(state, content, {}) == music(AudioDispatchSite::podium_second_pass),
            "the second pass too");
    require(podium_pass_cues(state, content, {}) == music(AudioDispatchSite::podium_third_pass),
            "and the third");
    for (unsigned pass = 4; pass <= 12; ++pass)
        require(podium_pass_cues(state, content, {}).empty(), "later passes are silent");
    require(state.league.podium_music_passes == 8, "`$1365` stops at 8");
    // A pass that leaves on a press returns before the music ($82:DE45).
    auto pressed = league(3, 0xe000);
    enter_league_podium(pressed);
    for (std::uint32_t frame = 1; frame <= 99; ++frame) podium_entry_cues(pressed, content, frame);
    require(podium_pass_cues(pressed, content, {pad_start, 0}).empty()
                && pressed.screen == FrontEndScreen::league_podium_exit
                && pressed.league.podium_music_passes == 1,
            "a press leaves without the music");
    // A press held as the podium appears, on the entry's frame 99, leaves before the first pass's
    // music too ($82:DE06-DE45; rv-press1).
    auto at_once = league(3, 0xe000);
    enter_league_podium(at_once);
    for (std::uint32_t frame = 1; frame <= 98; ++frame) podium_entry_cues(at_once, content, frame);
    constexpr std::uint16_t pad_a_word = 0x0080;
    require(podium_entry_cues(at_once, content, 99, {pad_a_word, 0}).empty()
                && at_once.screen == FrontEndScreen::league_podium_exit
                && at_once.league.podium_music_passes == 0,
            "a press on the first pass leaves without the music");
    podium_exit_test(content);
    require(audio_session_name(AudioSessionLoad::podium) == "podium"
                && audio_dispatch_site_name(AudioDispatchSite::podium_third_pass) == "podium-3",
            "the cue logs' names");
}
// R-0105: `$83:958C` clears the title code's flag `$77:10D0` with the pair's word, at a league
// pair's choice and a VS race's; leaving the league slots clears `$77:0742` bit 13, so a podium
// still owed is not shown.
void option_bits(const Content& content) {
    auto pair = league(0, 0x0500);
    pair.records.cheat = true;
    choose_league_pair(pair);
    require(!pair.records.cheat && pair.screen == FrontEndScreen::now_playing_entry,
            "a league pair's choice clears the title code's flag");
    for (const auto mode : {FrontEndMode::versus, FrontEndMode::two_player}) {
        auto race = start_front_end();
        race.mode = mode;
        race.records.cheat = true;
        race.screen = FrontEndScreen::now_playing;
        race.arrow.target_x = 0x0300; // Race
        now_playing_frame(race, content.view, {choose_buttons, 0});
        require(race.screen == FrontEndScreen::race_fade, "NOW PLAYING's race");
        require(race.records.cheat == (mode != FrontEndMode::versus),
                "VS's race clears the flag, 2P's keeps it");
    }
    for (const auto pad : {choose_buttons, back_buttons}) {
        auto slots = league(0, 0x0500);
        slots.records.league_cycle_complete = true;
        slots.screen = FrontEndScreen::league_slots;
        league_slots_frame(slots, content.view, {pad, 0});
        require(!slots.records.league_cycle_complete,
                "leaving the slots either way clears the podium owed");
    }
}
} // namespace
int main() try {
    const Content content;
    eight_members_and_integer_wrap(content);
    odd_pair_and_result_order(content);
    bonus_comparisons();
    stunt_totals_after_the_tallies();
    podium_music(content);
    option_bits(content);
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}
