// HIGH SCORES, RECORDS' second choice (`$80:D7CD`; R-0072, R-0100): the leaders of eight
// categories over the sixteen riders printed and slid in, their colours over three frames, then
// `$80:C206`'s wait for a press and the back slide to the RECORDS menu.
#include "front_end_screens.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace unirally::front_end_screens {
namespace {

struct ScoreLeader {
    std::uint16_t value{};
    std::uint16_t rider{someone};
};

struct ScoreLeaders {
    ScoreLeader races, wins, losses, best_win_average, worst_win_average, total_score,
        best_score_average, worst_score_average;
};

constexpr unsigned rider_count = 16, last_rider = rider_count - 1;

// $83:97BD (RECORDS' entry): the largest races, wins and losses (races less wins) and score.
void find_largest(const OnePlayerRecords& records, ScoreLeaders& leaders) {
    for (const auto& stats : records.statistics) {
        leaders.races.value = std::max(leaders.races.value, stats[0]);
        leaders.losses.value =
            std::max(leaders.losses.value, static_cast<std::uint16_t>(stats[0] - stats[1]));
        leaders.wins.value = std::max(leaders.wins.value, stats[1]);
    }
}

// $80:D7F2-D833: the last rider with the most races and the last with the most losses; the first
// with the most wins.
void find_most(const OnePlayerRecords& records, ScoreLeaders& leaders) {
    for (unsigned rider = 0; rider < rider_count; ++rider) {
        const auto& stats = records.statistics[rider];
        if (stats[0] == leaders.races.value)
            leaders.races.rider = static_cast<std::uint16_t>(rider);
        if (static_cast<std::uint16_t>(stats[0] - stats[1]) == leaders.losses.value)
            leaders.losses.rider = static_cast<std::uint16_t>(rider);
    }
    for (unsigned rider = 0; rider < rider_count; ++rider) {
        if (records.statistics[rider][1] != leaders.wins.value) continue;
        leaders.wins.rider = static_cast<std::uint16_t>(rider);
        return;
    }
}

// The best (the last of the highest) and the worst (the first of the lowest below 7000) of a
// rider's average.
void rank_average(ScoreLeader& best, ScoreLeader& worst, std::uint16_t value, unsigned rider) {
    if (value >= best.value) best = {value, static_cast<std::uint16_t>(rider)};
    if (value < worst.value) worst = {value, static_cast<std::uint16_t>(rider)};
}

// $80:D7CD-D8F8. Every leader starts on rider 15 (`$80:D8C3` turns the loops' counts into riders);
// the worst averages start at 7000. The win averages are `$83:9D80`'s percentages of the riders
// with races (`$77:10F9` keeps the last one's races, halved; `$77:10FB` rider 15's wins, halved
// only if it has races); the score averages the score over the
// races (`$80:9719`) of the riders with a score. The total score is the last of the highest. With
// no race at all, both worst averages are 0 and every leader SOMEONE.
ScoreLeaders high_score_leaders(OnePlayerRecords& records) {
    constexpr std::uint16_t no_average = 7000;
    ScoreLeaders leaders;
    for (auto* leader : {&leaders.races, &leaders.wins, &leaders.losses, &leaders.best_win_average,
                         &leaders.worst_win_average, &leaders.total_score,
                         &leaders.best_score_average, &leaders.worst_score_average})
        leader->rider = last_rider;
    leaders.worst_win_average.value = leaders.worst_score_average.value = no_average;
    find_largest(records, leaders);
    find_most(records, leaders);
    for (unsigned rider = 0; rider < rider_count; ++rider) {
        const auto& stats = records.statistics[rider];
        // $80:D83D-D849: every rider's wins go to `$77:10FB` before the test for races.
        if (stats[0] == 0)
            records.versus_percentage_wins = stats[1];
        else
            rank_average(leaders.best_win_average, leaders.worst_win_average,
                         cartridge_percentage(records, stats[0], stats[1]), rider);
        if (stats[3] >= leaders.total_score.value)
            leaders.total_score = {stats[3], static_cast<std::uint16_t>(rider)};
    }
    for (unsigned rider = 0; rider < rider_count; ++rider) {
        const auto& stats = records.statistics[rider];
        if (stats[3] != 0)
            rank_average(leaders.best_score_average, leaders.worst_score_average,
                         divide_word(stats[3], stats[0]), rider);
    }
    if (leaders.worst_win_average.value != no_average) return leaders;
    leaders.worst_win_average.value = leaders.worst_score_average.value = 0;
    for (auto* leader : {&leaders.races, &leaders.wins, &leaders.losses, &leaders.best_win_average,
                         &leaders.worst_win_average, &leaders.total_score,
                         &leaders.best_score_average, &leaders.worst_score_average})
        leader->rider = someone;
    return leaders;
}

std::uint16_t high_score_word(const ScoreLeaders& s, std::uint16_t address) {
    switch (address) {
    case 0x013f: return s.races.value;
    case 0x0016: return s.races.rider;
    case 0x0141: return s.wins.value;
    case 0x001a: return s.wins.rider;
    case 0x0143: return s.losses.value;
    case 0x001e: return s.losses.rider;
    case 0x0020: return s.best_win_average.value;
    case 0x0022: return s.best_win_average.rider;
    case 0x0024: return s.worst_win_average.value;
    case 0x0026: return s.worst_win_average.rider;
    case 0x0147: return s.total_score.value;
    case 0x002a: return s.total_score.rider;
    case 0x002c: return s.best_score_average.value;
    case 0x002e: return s.best_score_average.rider;
    case 0x0030: return s.worst_score_average.value;
    case 0x0032: return s.worst_score_average.rider;
    default: throw std::invalid_argument("unknown HIGH SCORES text operand");
    }
}

// $80:D7CD-D905: the leaders and the text, their objects hidden. The text stream's operands are
// the computed leaders and their values.
void print_high_scores(FrontEndState& state, const FrontEndContent& content) {
    const auto leaders = high_score_leaders(state.records);
    TextVariables variables;
    variables.rider_names = state.records.rider_names;
    variables.word = [&](std::uint16_t address) { return high_score_word(leaders, address); };
    variables.place_object = [&](unsigned object, unsigned position) {
        place_printed_object(state, object, position);
    };
    state.text.words.fill(cleared_text); // $83:8B51
    print_text(state.text, state.printer, content.high_scores_text, content.character_table,
               &variables);
    high_bits(state, 104) = high_bits(state, 108) = four_hidden;
}

// $80:D917-D9AF: after the slide, one frame wait each for the leaders' colours (wins, losses and
// races; the best and worst win averages and the total score; the best and worst score
// averages), then the objects shown.
void load_leader_colours(FrontEndState& state, const FrontEndContent& content, unsigned frame) {
    const auto leaders = high_score_leaders(state.records);
    const std::array<std::uint16_t, 8> riders{leaders.wins.rider,
                                              leaders.losses.rider,
                                              leaders.races.rider,
                                              leaders.best_win_average.rider,
                                              leaders.worst_win_average.rider,
                                              leaders.total_score.rider,
                                              leaders.best_score_average.rider,
                                              leaders.worst_score_average.rider};
    const unsigned first = 3 * frame, last = frame == 2 ? 8 : first + 3;
    for (unsigned k = first; k < last; ++k)
        load_cgram(state, asset(content, first_rider_palette + riders[k]), 0x80 + 16 * k);
    if (frame == 2) high_bits(state, 104) = high_bits(state, 108) = four_shown;
}

void begin(FrontEndState& state, HighScoresPhase phase) {
    state.records_detail.high_scores = phase;
    state.records_detail.phase_start = state.script_frame;
}

// $80:D9B5-D9D7: the printed objects hidden (`$80:D420`), then after a frame wait the OAM, the
// object colours (`$83:91F7`, assets 28 and 5) and the back slide.
void leave_high_scores(FrontEndState& state) {
    for (unsigned entry = 104; entry < 112; ++entry)
        oam_byte(state, entry, 0) = oam_byte(state, entry, 1) = 1;
    high_bits(state, 104) = high_bits(state, 108) = four_hidden;
    begin(state, HighScoresPhase::leaving);
}

void start_back_slide(FrontEndState& state, const FrontEndContent& content) {
    copy_oam(state);
    load_object_palette(state, content);
    load_cgram(state, asset(content, 28), 0xd0);
    load_cgram(state, asset(content, 5), 0xe0);
    start_slide(state, content, true);
}

} // namespace

bool high_scores_entry_waits(std::uint32_t next) {
    // The leaders' print runs past the choice's frame and ends in the next (`$80:D908`).
    return next != 1;
}

void high_scores_entry_frame(FrontEndState& state, const FrontEndContent& content) {
    switch (state.script_frame) {
    case 1: print_high_scores(state, content); return;
    case 2: // $80:D90B-D914
        copy_oam(state);
        load_text(state, state.slide.hidden_half);
        send_arrow_off(state);
        start_slide(state, content, false);
        return;
    default:
        if (!slide_frame(state, content)) return;
        // The loop's screen from the next frame, its script counted from 0.
        state.records_detail.high_scores = HighScoresPhase::colours;
        state.records_detail.phase_start = 0;
        state.screen = FrontEndScreen::records_detail;
        return;
    }
}

void high_scores_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    auto& detail = state.records_detail;
    const auto frame = state.script_frame - detail.phase_start;
    switch (detail.high_scores) {
    case HighScoresPhase::colours:
        load_leader_colours(state, content, frame - 1);
        if (frame != 3) return;
        begin(state, HighScoresPhase::pressing);
        detail.press_seen = false;
        return;
    case HighScoresPhase::pressing:
        if (press_seen_twice(state, content, pads)) leave_high_scores(state);
        return;
    case HighScoresPhase::leaving:
        if (frame == 1) {
            start_back_slide(state, content);
            return;
        }
        if (!slide_frame(state, content)) return;
        state.logo.raised = false; // $80:D55F `JSR $F52B`
        reopen_records_menu(state, content);
        return;
    case HighScoresPhase::entering: return;
    }
}

} // namespace unirally::front_end_screens
