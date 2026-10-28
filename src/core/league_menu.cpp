// The saved league's standings and pairing entry. $80:BDD4-BEE4; R-0073.
#include "front_end_screens.hpp"
#include "menu_overruns.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <vector>

namespace unirally::front_end_screens {
namespace {
constexpr unsigned league_rows = 8, first_table_row = 8, row_spacing = 2;
constexpr std::size_t rank_strings = 63, table_row_stream = 99;
constexpr std::uint16_t race_arrow_x = 0x03c0, race_arrow_y = 0x0d00;
using RowOrder = std::array<unsigned, league_rows>;

// Membership bits run from MIKE at bit 15 to STEVE at bit 0. Empty rows use SOMEONE.
// $83:963C-967B; R-0073.
std::array<std::uint8_t, league_rows> league_members(const FrontEndState& state) {
    std::array<std::uint8_t, league_rows> members;
    members.fill(someone);
    const auto mask = state.records.league_members[state.league.slot];
    unsigned row = 0;
    for (unsigned rider = 0; rider < 16; ++rider) {
        if (!(mask & (0x8000U >> rider))) continue;
        if (row == members.size()) throw std::invalid_argument("league exceeds eight members");
        members[row++] = static_cast<std::uint8_t>(rider);
    }
    return members;
}

// Lowest score moves to the end; a later equal value wins that comparison.
// $80:EFD4-F02A; R-0073.
// `stores`, when given, counts `$80:EFE9-EFF4`'s stores of the lowest so far (R-0102: the print's
// time depends on them).
RowOrder standings_order(const FrontEndState& state, unsigned* stores = nullptr) {
    RowOrder rows{0, 1, 2, 3, 4, 5, 6, 7};
    const auto& points = state.records.league_scores[state.league.slot];
    for (std::size_t last = rows.size(); last-- > 0;) {
        std::uint16_t lowest = 0x7fff;
        unsigned chosen = 0;
        for (unsigned index = 0; index <= last; ++index) {
            if (points[rows[index]] <= lowest) {
                lowest = points[rows[index]];
                chosen = index;
                if (stores != nullptr) ++*stores;
            }
        }
        std::swap(rows[chosen], rows[last]);
    }
    return rows;
}

// A tied points value keeps the previous rank, including its skipped ordinal.
// $80:A5B2-A65E; R-0073.
void print_standings(FrontEndState& state, const FrontEndContent& content) {
    const auto members = league_members(state);
    const auto order = standings_order(state);
    TextVariables variables;
    variables.rider_names = state.records.rider_names;
    unsigned row = 0, rank = 0;
    std::uint16_t last_points = 0xffff;
    variables.word = [&](std::uint16_t address) -> std::uint16_t {
        const auto member = order[row];
        if (address == 0x00b2) return members[member];
        if (address == 0x00b4) return state.records.league_played[state.league.slot][member];
        if (address == 0x00b6) return state.records.league_scores[state.league.slot][member];
        throw std::invalid_argument("unknown league standings operand");
    };
    for (; row < league_rows; ++row) {
        if (members[order[row]] == someone) continue;
        const auto points = state.records.league_scores[state.league.slot][order[row]];
        if (points != last_points) rank = row + 1;
        last_points = points;
        const auto y = static_cast<std::uint8_t>(first_table_row + row_spacing * row);
        auto stream = content.league_table_text.subspan(table_row_stream);
        std::vector<std::uint8_t> line(stream.begin(), stream.end());
        for (unsigned at : {4U, 10U, 16U}) line.at(at) = y;
        print_text(state.text, state.printer, line, content.character_table, &variables);
        std::vector<std::uint8_t> ordinal{0xfe, 2, y};
        const auto text = content.league_table_text.subspan(rank_strings + rank * 4, 4);
        ordinal.insert(ordinal.end(), text.begin(), text.end());
        print_text(state.text, state.printer, ordinal, content.character_table);
    }
}

void print_table(FrontEndState& state, const FrontEndContent& content) {
    state.text.words.fill(cleared_text);
    TextVariables variables;
    variables.league_names = state.records.league_names;
    variables.word = [&](std::uint16_t address) -> std::uint16_t {
        if (address == 0x009b) return state.league.slot;
        throw std::invalid_argument("unknown league table operand");
    };
    print_text(state.text, state.printer, content.league_table_text, content.character_table,
               &variables);
    print_standings(state, content);
    load_text(state, state.slide.hidden_half);
}
} // namespace

// $80:AD47-ADE2: census rows retain their played/points words; pairing rank is separate.
void reorder_league(FrontEndState& state) {
    const auto slot = state.league.slot;
    const auto members = league_members(state);
    const auto order = standings_order(state);
    for (unsigned row = 0; row < league_rows; ++row)
        state.records.league_pairings[slot][row] = members[order[row]];
    const auto place = state.tour_menu.track % tracks_per_tour;
    state.records.league_best[slot] = place == 0 || place == 3 ? 0xea62 : 0;
    state.records.league_best_holder[slot] = state.records.league_pairings[slot][0];
}

void reset_league(FrontEndState& state) {
    const auto slot = state.league.slot;
    state.records.league_pair_cursor[slot] = 0;
    state.records.league_tracks[slot] = 0;
    state.records.league_event_totals[slot].fill(0);
    reorder_league(state);
    // DEFINE LEAGUE's caller still has race kind 2; $80:D3F5 initializes its bonus to zero.
    state.records.league_best[slot] = 0;
}

// $83:958C/95CA: races run lower rider ID first; the result restores the saved pairing.
void restore_league_pair(FrontEndState& state) {
    const auto slot = state.league.slot;
    const auto cursor = state.records.league_pair_cursor[slot];
    const auto& pairings = state.records.league_pairings[slot];
    if (state.second_rider >= someone || pairings[cursor] == state.rider_menu.rider) return;
    std::swap(state.rider_menu.rider, state.second_rider);
    state.now_playing.opponent = state.second_rider;
    auto& times = state.race_result.times;
    std::swap(times.player_total, times.opponent_total);
    std::swap(times.player_laps, times.opponent_laps);
    std::swap(times.player_score, times.opponent_score);
    std::swap(times.player_tallies, times.opponent_tallies);
    std::swap(times.league_tricks[0], times.league_tricks[1]);
    std::swap(times.league_wipeouts[0], times.league_wipeouts[1]);
}

namespace {
bool league_race_quit(const RaceTimes& times) {
    return times.player_total == 0xea62 || times.opponent_total == 0xea62;
}

// $80:F88D: the pair's totals at the cursor, in pairing order; the second is not stored when the
// caller passes 0xFFFF ($80:F8A9-F8AF), which only a stunt event against the computer does.
void store_league_totals(FrontEndState& state, std::uint16_t first, std::uint16_t second,
                         bool store_second) {
    const auto slot = state.league.slot;
    const auto cursor = state.records.league_pair_cursor[slot];
    if (cursor >= league_rows) throw std::invalid_argument("invalid league cursor");
    auto& totals = state.records.league_event_totals[slot];
    const bool swapped = state.records.league_pairings[slot][cursor] != state.rider_menu.rider;
    totals[cursor] = first;
    if (store_second && cursor + 1U < league_rows) {
        totals[cursor + 1] = second;
        if (swapped) std::swap(totals[cursor], totals[cursor + 1]);
    }
}
} // namespace

// A stunt event's scores reach the event totals at the end of its result's tallies: after rider
// 1's tally (`$80:F2DC-F2E6`), or against the computer after the player's with no second word
// (`$80:F275-F27F`).
void store_league_stunt_totals(FrontEndState& state, bool against_computer) {
    const auto& times = state.race_result.times;
    if (league_race_quit(times)) return;
    store_league_totals(state, times.player_score, times.opponent_score, !against_computer);
}

// A lap or one-run race's totals reach the event totals with its result's rows: the lap result's
// second frame (`$80:9107`), the one-run result's third (`$80:D15F`) (R-0098).
void store_league_race_totals(FrontEndState& state) {
    const auto& times = state.race_result.times;
    if (league_race_quit(times)) return;
    store_league_totals(state, times.player_total, times.opponent_total, true);
}

// Race totals are held in pairing order ($80:F88D), stored by the result (store_league_race_totals,
// store_league_stunt_totals); bonuses examine player then opponent as the race returns.
void record_league_result(FrontEndState& state) {
    const auto slot = state.league.slot;
    const auto& times = state.race_result.times;
    if (league_race_quit(times)) return;
    auto& best = state.records.league_best[slot];
    auto& holder = state.records.league_best_holder[slot];
    const auto consider = [&](unsigned index, unsigned rider) {
        if (rider >= someone) return;
        if (times.lap_race) {
            const auto& laps = index == 0 ? times.player_laps : times.opponent_laps;
            for (const auto lap : laps)
                if (lap < best) {
                    best = lap;
                    holder = static_cast<std::uint16_t>(rider);
                }
        } else {
            // $83:9757-9782 (one run): the sum beats the bonus strictly; $83:97A4-97BC (stunt
            // event): `$77:07BD`/`$0827`, always 0, takes it on a tie, so the later human holds it.
            const auto value =
                times.stunt_event ? times.league_wipeouts[index] : times.league_tricks[index];
            if (times.stunt_event ? value <= best : value > best) {
                best = value;
                holder = static_cast<std::uint16_t>(rider);
            }
        }
    };
    consider(0, state.rider_menu.rider);
    consider(1, state.now_playing.opponent);
}

namespace {
RowOrder event_order(const FrontEndState& state) {
    RowOrder rows{0, 1, 2, 3, 4, 5, 6, 7};
    const auto& totals = state.records.league_event_totals[state.league.slot];
    const bool stunts = state.race_result.times.stunt_event;
    for (std::size_t last = rows.size(); last-- > 0;) {
        std::uint16_t selected = stunts ? 0 : 0x7fff;
        unsigned chosen = 0;
        for (unsigned index = 0; index <= last; ++index)
            if (stunts ? totals[rows[index]] >= selected : totals[rows[index]] <= selected) {
                selected = totals[rows[index]];
                chosen = index;
            }
        std::swap(rows[chosen], rows[last]);
    }
    std::reverse(rows.begin(), rows.end());
    return rows;
}

void award_points(FrontEndState& state, const FrontEndContent& content) {
    const auto slot = state.league.slot;
    auto& league = state.league;
    league.award_rows = event_order(state);
    league.award_riders = state.records.league_pairings[slot];
    league.award_points.fill(0);
    const auto members = league_members(state);
    unsigned place = 0;
    for (const auto row : league.award_rows) {
        const auto rider = league.award_riders[row];
        if (rider == someone) continue;
        auto points = content.league_awards_text[198 + 7 - place++];
        if (rider == state.records.league_best_holder[slot]) ++points;
        league.award_points[row] = points;
        const auto member = static_cast<unsigned>(std::find(members.begin(), members.end(), rider)
                                                  - members.begin());
        auto& total = state.records.league_scores[slot][member];
        total = static_cast<std::uint16_t>(total + points);
    }
}

// $80:8A70-8B03, one pass of the rows in award order: the rider, the event total and the points,
// with the holder's extra point only on the second pass (`$77:10F3` bit 0, `$80:8AC4-8AD3`).
void print_award_rows(FrontEndState& state, const FrontEndContent& content, bool bonus_pass) {
    const auto slot = state.league.slot;
    const auto holder = state.records.league_best_holder[slot];
    TextVariables variables;
    variables.rider_names = state.records.rider_names;
    variables.time_words = content.time_words;
    unsigned row = 0;
    variables.word = [&](std::uint16_t at) -> std::uint16_t {
        const auto rider = state.league.award_riders[row];
        if (at == 0x00b2) return rider;
        if (at == 0x00b4) return state.records.league_event_totals[slot][row];
        if (at == 0x00b6)
            return static_cast<std::uint16_t>(state.league.award_points[row]
                                              - (!bonus_pass && rider == holder ? 1U : 0U));
        throw std::invalid_argument("unknown league award operand");
    };
    unsigned shown = 0;
    for (const auto ordered : state.league.award_rows) {
        row = ordered;
        const auto rider = state.league.award_riders[row];
        if (rider == someone) continue;
        auto stream = content.league_awards_text.subspan(177, 21);
        std::vector<std::uint8_t> line(stream.begin(), stream.end());
        for (unsigned at : {4U, 10U, 16U}) line[at] = static_cast<std::uint8_t>(9 + 2 * shown);
        if (state.race_result.times.stunt_event) line[11] = 0xfd;
        print_text(state.text, state.printer, line, content.character_table, &variables);
        load_cgram(state, asset(content, first_rider_palette + rider), 0x80 + 0x10 * shown);
        ++shown;
    }
}

// The riders' icons 104-111 shown, one a printed row, once the second pass has ended (R-0098:
// two rows, the frame before the decorations start).
void show_award_icons(FrontEndState& state) {
    unsigned shown = 0;
    for (const auto row : state.league.award_rows) {
        if (state.league.award_riders[row] == someone) continue;
        high_bits(state, 104 + (shown / 4) * 4) &=
            static_cast<std::uint8_t>(~hidden_bit(104 + shown));
        ++shown;
    }
}

// The first pass ($80:8A02): the header and the rows without the bonus point, to the hidden half
// the awards slide in from.
void print_awards(FrontEndState& state, const FrontEndContent& content) {
    TextVariables variables;
    variables.league_names = state.records.league_names;
    variables.word = [&](std::uint16_t at) -> std::uint16_t {
        if (at == 0x00cc) return state.league.slot;
        throw std::invalid_argument("unknown league award operand");
    };
    variables.place_object = [&](unsigned object, unsigned position) {
        place_printed_object(state, object, position);
    };
    state.text.words.fill(cleared_text);
    const auto print = [&](std::span<const std::uint8_t> stream) {
        print_text(state.text, state.printer, stream, content.character_table, &variables);
    };
    print(content.league_awards_text);
    if (state.race_result.times.stunt_event) print(content.league_awards_text.subspan(168));
    high_bits(state, 104) = high_bits(state, 108) = four_hidden;
    print_award_rows(state, content, false);
    load_text(state, state.slide.hidden_half); // slid in by league_awards_frame
}

// After the slide ($80:8B2B-8B6C): the bonus line by the race mode (`$77:074B`: 0 the most stunt
// bonus, 1 the best lap's, 2 the least wipeouts') with its holder, then the rows again with the
// holder's point. The text reaches the screen with the result's sound (league_awards_frame).
void print_award_bonus(FrontEndState& state, const FrontEndContent& content) {
    const auto holder = state.records.league_best_holder[state.league.slot];
    TextVariables variables;
    variables.rider_names = state.records.rider_names;
    variables.word = [&](std::uint16_t) { return holder; };
    const auto& times = state.race_result.times;
    const auto bonus = times.stunt_event ? 141U : times.lap_race ? 119U : 95U;
    print_text(state.text, state.printer, content.league_awards_text.subspan(bonus),
               content.character_table, &variables);
    print_award_rows(state, content, true);
}
} // namespace

namespace {
// `$80:AF54-AFCF`: each rider of the pair at the cursor has played once more.
std::array<std::uint16_t, league_rows> played_after_count(const FrontEndState& state) {
    const auto slot = state.league.slot;
    const auto cursor = state.records.league_pair_cursor[slot];
    const auto members = league_members(state);
    auto played = state.records.league_played[slot];
    for (unsigned at = cursor; at < std::min<unsigned>(cursor + 2, league_rows); ++at) {
        const auto rider = state.records.league_pairings[slot][at];
        if (rider == someone) continue;
        const auto row = static_cast<unsigned>(std::find(members.begin(), members.end(), rider)
                                               - members.begin());
        ++played[row];
    }
    return played;
}

// `$80:AFC3-AFE7`: another pair races after the one at the cursor.
bool another_pair(const FrontEndState& state) {
    const auto slot = state.league.slot;
    const auto cursor = state.records.league_pair_cursor[slot];
    return cursor < 6 && state.records.league_pairings[slot][cursor + 2] != someone;
}

// The standings' rows and the digits of their played and points words, `played` as given:
// `$83:8BE7` counts the tens and above down in one kind of loop and the units in another, then
// blanks the leading zeros; the ranks, a new one wherever the points change; and the sort's
// stores, which depend on the rows' order.
StandingsWork standings_work(const FrontEndState& state,
                             const std::array<std::uint16_t, league_rows>& played) {
    const auto members = league_members(state);
    StandingsWork work;
    constexpr unsigned digits_printed = 5;
    const auto add = [&](unsigned value) {
        work.unit_digit_units += value % 10;
        unsigned digits = 1;
        for (value /= 10; value != 0; value /= 10, ++digits) work.higher_digit_units += value % 10;
        work.leading_zeros += digits_printed - digits;
    };
    std::vector<std::uint16_t> points;
    for (unsigned row = 0; row < league_rows; ++row) {
        if (members[row] == someone) continue;
        ++work.rows;
        add(played[row]);
        const auto row_points = state.records.league_scores[state.league.slot][row];
        add(row_points);
        if (std::find(points.begin(), points.end(), row_points) == points.end())
            points.push_back(row_points);
    }
    // The standings run in points order, so each distinct value after the highest starts a rank.
    work.new_ranks = points.empty() ? 0U : static_cast<unsigned>(points.size() - 1);
    standings_order(state, &work.sort_stores);
    return work;
}

// The palette cycle's delay as this frame's NMI left it, while the cycle runs (R-0102).
PaletteDelay palette_delay(const FrontEndState& state) {
    if (!state.cycle.running) return std::nullopt;
    return static_cast<std::uint8_t>(state.cycle.delay);
}

// `$80:A547` and the standings' entry, after whatever frames the print's cost gives (R-0102).
void show_league_table(FrontEndState& state, const FrontEndContent& content, bool slides_back,
                       std::uint8_t frames_late) {
    // `$80:A547` leaves entries 104-111 as they were: the awards' way out and NOW PLAYING's back
    // hide them themselves, PICK TOUR and the slots leave them shown off screen (R-0098).
    print_table(state, content);
    state.league.table_frames_late = frames_late;
    state.league.table_slides_back = slides_back;
    state.screen = FrontEndScreen::league_table_entry;
    state.latches = {};
    state.league.previous_buttons = 0;
}
} // namespace

// $80:AF54-AFEF in the result's way out, once the records' checksum runs are done (R-0098,
// R-0102): each rider of the pair has played once more, and with another pair to race the cursor
// moves to it.
void count_league_pair(FrontEndState& state) {
    const auto slot = state.league.slot;
    state.records.league_played[slot] = played_after_count(state);
    state.league.more_pairs = another_pair(state);
    if (state.league.more_pairs)
        state.records.league_pair_cursor[slot] =
            static_cast<std::uint8_t>(state.records.league_pair_cursor[slot] + 2);
}

// The way out's cost (R-0102): the count's frame after the records and their checksum runs; with
// another pair, the standings' print after the count, its numbers as the count leaves them. The
// awards keep R-0095's frames, which the same work measures give.
void time_league_exit(FrontEndState& state) {
    LeagueExitWork work;
    work.records_placed = state.race_result.records_placed;
    work.lap_race = state.race_result.times.lap_race;
    work.standings = standings_work(state, played_after_count(state));
    work.palette_delay = palette_delay(state);
    const auto frames = league_exit_frames(work);
    auto& result = state.race_result;
    result.league_count_frame = frames.count;
    result.league_standings_frame =
        another_pair(state) ? frames.standings : league_awards_scoring_frame(result.records_placed);
}

// Then the standings for the next pair, or the awards ($80:8917).
void finish_league_pair(FrontEndState& state, const FrontEndContent& content) {
    if (state.league.more_pairs) {
        show_league_table(state, content, false, 0); // the way out's frames hold the print's
        return;
    }
    award_points(state, content);
    print_awards(state, content);
    state.league.awards_press_seen = state.league.awards_left = false;
    state.screen = FrontEndScreen::league_awards;
}

namespace {

// The awards' frames by the rows printed (N, `award_rows_printed`): the first pass prints a row a
// frame from frame 1, the back slide (`$80:8B28`, `$80:E27E`) starts with its sound on frame N + 1
// and shows the awards on frame N + 40, the second pass prints a row a frame after it and ends on
// frame 2N + 40, the decorations step from frame 2N + 41, and the result's sound (`$80:B051`)
// comes 41 frames after the pass. Measured with two rows (tours-a) and three (odd-tours, R-0098).
struct AwardsFrames {
    std::uint32_t slide_start, passes_end, result;
};
AwardsFrames awards_frames(const FrontEndState& state) {
    unsigned rows = 0;
    for (const auto row : state.league.award_rows)
        if (state.league.award_riders[row] != someone) ++rows;
    constexpr std::uint32_t slide_frames = 39, result_wait = 41;
    return {rows + 1, 2 * rows + slide_frames + 1, 2 * rows + slide_frames + 1 + result_wait};
}

// $80:B05B-B079 (R-0095): after the result's sound `$80:C206` waits for a press of any button of
// either pad on two frames running, each frame copying the OAM and stepping the decorations; then
// the icons 104-111 hidden and, after a frame wait, the text and object palettes again and an OAM
// copy, and the table printed (`$80:A547`) in that frame.
void leave_league_awards(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    constexpr unsigned menu_text_palette = 28;
    auto& league = state.league;
    if (league.awards_left) {
        league.awards_left = false;
        load_cgram(state, asset(content, menu_text_palette), 0xd0);
        load_object_palette(state, content); // $83:91F7
        enter_league_table(state, content);
        return;
    }
    if (state.script_frame <= awards_frames(state).result) {
        league.awards_press_seen = false;
        return;
    }
    const bool press = pads.one != 0 || pads.two != 0;
    if (press && league.awards_press_seen) {
        high_bits(state, 104) = high_bits(state, 108) = four_hidden;
        league.awards_left = true;
    }
    league.awards_press_seen = press;
}

// $80:AFF5-B015 at the second pass's end: the icons shown, the pairing reordered, the cursor back
// to the first pair and the next track (R-0073, R-0098).
void end_award_passes(FrontEndState& state) {
    const auto slot = state.league.slot;
    show_award_icons(state);
    reorder_league(state);
    state.records.league_pair_cursor[slot] = 0;
    state.records.league_tracks[slot] =
        static_cast<std::uint8_t>((state.records.league_tracks[slot] + 1) % 44);
    // $80:B00F-B011: the next track becomes the menus' track (`$00CE`) and `$83:9983` derives its
    // tour (`$00D0`), where a podium's PICK TOUR puts its cursor (R-0098).
    state.tour_menu.track = state.records.league_tracks[slot];
    state.tour_menu.tour = static_cast<std::uint8_t>(state.tour_menu.track / tracks_per_tour);
    if (state.records.league_tracks[slot] % tracks_per_tour == 0)
        state.records.league_cycle_complete = true;
}

} // namespace

// The awards are printed to the hidden half and slide in; the bonus line and the second pass
// follow the slide and reach the screen with the result's sound. The back slide steps no
// decorations: they step from the passes' end on (R-0095, R-0098).
void league_awards_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    const auto frame = state.script_frame;
    const auto frames = awards_frames(state);
    if (state.league.awards_left) { // the frame after the press: no decorations
        copy_oam(state);
        leave_league_awards(state, content, pads);
        return;
    }
    if (frame > frames.slide_start && state.slide.countdown >= 0) {
        if (slide_frame(state, content)) print_award_bonus(state, content); // copies the OAM
    } else {
        copy_oam(state);
    }
    if (frame > frames.passes_end) step_decorations(state, content);
    if (frame == frames.slide_start) start_slide(state, content, true);
    if (frame == frames.result) {
        play_menu_sound(state, MenuSound::result);
        load_text(state, state.slide.shown_half);
    }
    if (frame == frames.passes_end) end_award_passes(state);
    leave_league_awards(state, content, pads);
}

void choose_league_slot(FrontEndState& state, const FrontEndContent& content) {
    // $80:BE00-BE14; R-0073.
    if (!state.records.league_members[state.league.slot]) return;
    if (state.records.league_pair_cursor[state.league.slot]) {
        enter_league_table(state, content);
        return;
    }
    // `$017D` keeps its rider: only the pair's choice writes it (`$83:98EE`; R-0098, `backs`).
    enter_tour_menu(state);
}

void enter_league_table(FrontEndState& state, const FrontEndContent& content, bool slides_back) {
    const auto standings = standings_work(state, state.records.league_played[state.league.slot]);
    show_league_table(state, content, slides_back,
                      standings_print_frames_late(standings, palette_delay(state)));
}

void league_table_entry_frame(FrontEndState& state, const FrontEndContent& content) {
    if (state.league.table_frames_late != 0) { // the print runs on: only the NMI
        --state.league.table_frames_late;
        state.script_frame = 0;
        return;
    }
    if (state.script_frame == 1) {
        copy_oam(state);
        start_slide(state, content, state.league.table_slides_back);
        return;
    }
    if (!slide_frame(state, content)) return;
    state.arrow.target_x = race_arrow_x;
    state.arrow.target_y = race_arrow_y;
    state.screen = FrontEndScreen::league_table;
}

void league_table_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    copy_oam(state);
    step_decorations(state, content);
    const auto buttons = static_cast<std::uint16_t>(pads.one | pads.two);
    const auto previous = state.league.previous_buttons;
    state.league.previous_buttons = buttons;
    if (!buttons || !previous) return;
    if (buttons & back_buttons) {
        // $80:BE5F-BE73: `$00AC` = 1, so the slots slide forward (`$80:EF7B`) and PICK TOUR, from
        // `$80:E550` without `$80:BE16`'s loads, back (`$80:E570`; R-0098).
        if (state.records.league_pair_cursor[state.league.slot])
            enter_league_slots(state, content);
        else
            return_to_tour_menu(state, true);
        return;
    }
    if (!(buttons & choose_buttons)) return;
    if (state.records.league_cycle_complete) {
        enter_league_podium(state);
        return;
    }
    choose_league_pair(state);
}

// $83:958C/95CA; R-0073. Both standings and post-podium tour choice launch this pairing.
void choose_league_pair(FrontEndState& state) {
    const auto slot = state.league.slot;
    const auto& members = state.records.league_pairings[slot];
    const auto cursor = state.records.league_pair_cursor[slot];
    if (cursor >= league_rows || members[cursor] == someone)
        throw std::invalid_argument("league pairing continuation is not recovered");
    state.rider_menu.rider = members[cursor];
    state.second_rider = cursor + 1U < league_rows ? members[cursor + 1U] : someone;
    if (state.second_rider < someone && state.rider_menu.rider > state.second_rider)
        std::swap(state.rider_menu.rider, state.second_rider);
    state.tour_menu.track = state.records.league_tracks[slot];
    state.tour_menu.tour = state.tour_menu.track / tracks_per_tour;
    state.now_playing.opponent = state.second_rider;
    // `$83:958F-9592` stores 0 to `$77:10CF-10D0`, so the pair's choice also clears the title
    // code's flag, before NOW PLAYING (`$80:BEF9`; R-0105).
    state.records.cheat = false;
    enter_now_playing(state);
}
} // namespace unirally::front_end_screens
