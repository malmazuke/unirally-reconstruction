// The two five-choice menus under OPTIONS (R-0072). Both print their text into the hidden
// half of BG2 at the choice frame, then slide it into view over 39 more pictures.
#include "front_end_screens.hpp"

#include <algorithm>
#include <array>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

namespace unirally::front_end_screens {
namespace {

constexpr unsigned choice_count = 5;
constexpr std::uint16_t first_row = 0x0580, row_step = 0x0180;

void print_choices(FrontEndState& state, const FrontEndContent& content,
                   std::span<const std::uint8_t> streams) {
    state.text.words.fill(cleared_text);
    for (unsigned row = 0; row < choice_count; ++row) {
        const auto entry = nth_string(streams, row);
        print_text(state.text, state.printer, {entry.data(), entry.size() + 1},
                   content.character_table);
    }
    load_text(state, state.slide.hidden_half);
}

void aim_choice(FrontEndState& state, std::span<const std::uint8_t> columns) {
    const auto row = state.menu.selection;
    // The table byte is signed before conversion to sixteenths of a pixel.
    state.arrow.target_x = static_cast<std::uint16_t>(
        static_cast<std::int16_t>(static_cast<std::int8_t>(columns[row])) * 128);
    state.arrow.target_y = static_cast<std::uint16_t>(first_row + row * row_step);
}

void move_choice(FrontEndState& state, std::span<const std::uint8_t> columns, FrontEndPads pads,
                 unsigned count = choice_count) {
    const auto pad = static_cast<std::uint16_t>(pads.one | pads.two);
    const bool down = (pad & (pad_down | pad_select)) != 0;
    const bool up = !down && (pad & pad_up) != 0;
    if (!down && !up) {
        state.latches.moved = false;
        return;
    }
    if (state.latches.moved) return;
    state.latches.moved = true;
    play_menu_sound(state, MenuSound::navigate); // $80:BA13 (Down), $80:B9B1 (Up)
    auto& row = state.menu.selection;
    row = static_cast<std::uint8_t>(down ? (row + 1U) % count : (row + count - 1U) % count);
    aim_choice(state, columns);
}

void return_to_main(FrontEndState& state, const FrontEndContent& content) {
    print_main_menu(state, content);
    state.screen = FrontEndScreen::main_menu_return;
}

} // namespace

void enter_options_menu(FrontEndState& state, const FrontEndContent& content) {
    // $80:B626-B639, $80:EF4D; R-0072 observation 1.
    state.mode = FrontEndMode::options;
    state.decorations.delay = 0;
    print_choices(state, content, content.options_menu_text);
    state.screen = FrontEndScreen::options_entry;
}

void return_to_options_menu(FrontEndState& state, const FrontEndContent& content) {
    print_choices(state, content, content.options_menu_text);
    state.screen = FrontEndScreen::options_return;
}

void options_entry_frame(FrontEndState& state, const FrontEndContent& content) {
    if (state.script_frame == 1) {
        copy_oam(state);
        start_slide(state, content, false);
        return;
    }
    if (!slide_frame(state, content)) return;
    state.menu.selection = 0;
    aim_choice(state, content.options_arrow_columns);
    state.latches = {};
    state.screen = FrontEndScreen::options_menu;
}

void options_return_frame(FrontEndState& state, const FrontEndContent& content) {
    if (state.script_frame == 1) {
        copy_oam(state);
        start_slide(state, content, true);
        return;
    }
    if (state.script_frame <= 40) {
        if (slide_frame(state, content)) {
            state.menu.selection = 0;
            aim_choice(state, content.options_arrow_columns);
            state.latches = {};
        }
        return;
    }
    if (state.script_frame == 41) {
        reload_menu_palette(state, content);
        if (state.options_palette_saved) {
            std::copy(state.options_upper_palette.begin(), state.options_upper_palette.end(),
                      state.video.cgram.begin() + 0x100);
            state.options_palette_saved = false;
        }
        return;
    }
    if (state.script_frame == 42) {
        reload_menu_text_tiles(state, content);
    }
    state.screen = FrontEndScreen::options_menu;
}

void options_menu_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    copy_oam(state);
    move_choice(state, content.options_arrow_columns, pads);
    const auto pad = static_cast<std::uint16_t>(pads.one | pads.two);
    if (pad & back_buttons || ((pad & choose_buttons) && state.menu.selection == 4)) {
        return_to_main(state, content);
        return;
    }
    if (!(pad & choose_buttons)) return;
    if (state.menu.selection == 0) {
        enter_records_menu(state, content);
        return;
    }
    if (state.menu.selection == 1 || state.menu.selection == 2) {
        std::copy_n(state.video.cgram.begin() + 0x100, 256, state.options_upper_palette.begin());
        state.options_palette_saved = true;
        state.rider_menu.purpose = state.menu.selection == 1 ? RiderMenuPurpose::define_player
                                                             : RiderMenuPurpose::rename_player;
        enter_rider_menu(state);
        return;
    }
    enter_league_slots(state, content);
}

void enter_league_slots(FrontEndState& state, const FrontEndContent& content) {
    // $80:9EF7-9F73, R-0072 observations 4 and 6.
    state.text.words.fill(cleared_text);
    TextVariables variables;
    variables.league_names = state.records.league_names;
    for (unsigned row = 0; row < 6; ++row) {
        const auto entry = nth_string(content.league_slot_text, row);
        print_text(state.text, state.printer, {entry.data(), entry.size() + 1},
                   content.character_table, &variables);
    }
    load_text(state, state.slide.hidden_half);
    state.screen = FrontEndScreen::league_slots_entry;
}

void league_slots_entry_frame(FrontEndState& state, const FrontEndContent& content) {
    if (state.script_frame == 1) {
        copy_oam(state);
        start_slide(state, content, false);
        return;
    }
    if (!slide_frame(state, content)) return;
    state.menu.selection = state.league.slot;
    state.arrow.target_x = 0x0100;
    state.arrow.target_y = static_cast<std::uint16_t>(first_row + state.league.slot * row_step);
    state.latches = {};
    state.screen = FrontEndScreen::league_slots;
}

void league_slots_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    copy_oam(state);
    constexpr std::array<std::uint8_t, 6> columns{2, 2, 2, 2, 2, 2};
    move_choice(state, columns, pads, 6);
    const auto pad = static_cast<std::uint16_t>(pads.one | pads.two);
    if (pad & back_buttons) {
        if (state.mode == FrontEndMode::league)
            return_to_main(state, content);
        else
            return_to_options_menu(state, content);
        return;
    }
    if (!(pad & choose_buttons)) return;
    play_menu_sound(state, MenuSound::select); // $80:9F15
    state.league.slot = state.menu.selection;
    if (state.mode == FrontEndMode::league) {
        choose_league_slot(state, content);
        return;
    }
    state.text.words.fill(cleared_text);
    TextVariables variables;
    variables.word = [&](std::uint16_t address) -> std::uint16_t {
        if (address != 0x00cc) throw std::invalid_argument("unknown league warning operand");
        return state.league.slot;
    };
    variables.league_names = state.records.league_names;
    print_text(state.text, state.printer, content.league_warning, content.character_table,
               &variables);
    print_text(state.text, state.printer, content.define_player_confirm_prompt,
               content.character_table);
    load_text(state, state.slide.hidden_half);
    state.screen = FrontEndScreen::league_warning_entry;
}

void league_warning_entry_frame(FrontEndState& state, const FrontEndContent& content) {
    if (state.script_frame == 1) {
        copy_oam(state);
        start_slide(state, content, false);
        return;
    }
    if (!slide_frame(state, content)) return;
    state.arrow.target_x = 0x0100;
    state.arrow.target_y = first_row;
    state.screen = FrontEndScreen::league_warning;
}

void league_warning_frame(FrontEndState& state, FrontEndPads pads) {
    copy_oam(state);
    constexpr std::uint16_t confirm = pad_select | 0x4000U | pad_a;
    if ((pads.one & confirm) != confirm) return;
    state.league.members = 0;
    state.league.previous_buttons = 0;
    state.rider_menu.purpose = RiderMenuPurpose::league_members;
    state.rider_menu.rider = 7; // $80:9D0B: COLIN is the first arrow position
    enter_rider_menu(state);
}

void enter_records_menu(FrontEndState& state, const FrontEndContent& content) {
    // $80:D525-D53C; R-0072 observation 5.
    state.decorations.delay = 0;
    state.records_detail.returning = false;
    print_choices(state, content, content.records_menu_text);
    state.screen = FrontEndScreen::records_entry;
}

void records_entry_frame(FrontEndState& state, const FrontEndContent& content) {
    if (state.script_frame == 1) {
        copy_oam(state);
        start_slide(state, content, state.records_detail.returning);
        return;
    }
    if (!slide_frame(state, content)) return;
    state.menu.selection = 0;
    aim_choice(state, content.records_arrow_columns);
    state.latches = {};
    state.records_detail.returning = false;
    state.screen = FrontEndScreen::records_menu;
}

void print_high_scores(FrontEndState& state, const FrontEndContent& content);
void print_player_scores(FrontEndState& state, const FrontEndContent& content);
void print_group_scores(FrontEndState& state, const FrontEndContent& content);
void print_track_records(FrontEndState& state, const FrontEndContent& content);

void records_menu_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    copy_oam(state);
    move_choice(state, content.records_arrow_columns, pads);
    const auto pad = static_cast<std::uint16_t>(pads.one | pads.two);
    if ((pad & choose_buttons) && state.menu.selection == 4) {
        return_to_main(state, content);
        return;
    }
    if (pad & back_buttons) {
        return_to_options_menu(state, content);
        return;
    }
    if (!(pad & choose_buttons)) return;
    state.records_detail.category = state.menu.selection;
    state.records_detail.rider = 0;
    state.records_detail.tour = 0;
    raise_logo(state);
    state.screen = FrontEndScreen::records_detail_entry;
}

namespace {

struct ScoreLeader {
    std::uint16_t value{};
    std::uint16_t rider{someone};
};

struct ScoreLeaders {
    ScoreLeader races, wins, losses, best_win_average, worst_win_average, total_score,
        best_score_average, worst_score_average;
};

void take_larger(ScoreLeader& leader, std::uint16_t value, unsigned rider) {
    if (value <= leader.value) return;
    leader = {value, static_cast<std::uint16_t>(rider)};
}

ScoreLeaders high_score_leaders(const OnePlayerRecords& records) {
    ScoreLeaders leaders;
    leaders.worst_win_average.value = leaders.worst_score_average.value = 0xffff;
    bool any_played = false;
    for (unsigned rider = 0; rider < records.statistics.size(); ++rider) {
        const auto& stats = records.statistics[rider];
        const auto races = stats[0], wins = stats[1];
        const auto losses = static_cast<std::uint16_t>(races - wins);
        const auto score = stats[3];
        take_larger(leaders.races, races, rider);
        take_larger(leaders.wins, wins, rider);
        take_larger(leaders.losses, losses, rider);
        if (score >= leaders.total_score.value)
            leaders.total_score = {score, static_cast<std::uint16_t>(rider)};
        // $80:D8C1-D8D8 assigns an unplayed rider the 7000 score-average
        // sentinel. In the measured one-played-rider case, equal zero best
        // averages and equal 7000 worst averages end on rider 15.
        const auto score_average = static_cast<std::uint16_t>(races == 0 ? 0 : score / races);
        if (score_average >= leaders.best_score_average.value)
            leaders.best_score_average = {score_average, static_cast<std::uint16_t>(rider)};
        const auto worst_score = static_cast<std::uint16_t>(races == 0 ? 7000 : score_average);
        if (leaders.worst_score_average.rider == someone
            || worst_score >= leaders.worst_score_average.value)
            leaders.worst_score_average = {worst_score, static_cast<std::uint16_t>(rider)};
        if (races == 0) continue;
        any_played = true;
        const auto win_average = static_cast<std::uint16_t>(100U * wins / races);
        take_larger(leaders.best_win_average, win_average, rider);
        if (win_average < leaders.worst_win_average.value)
            leaders.worst_win_average = {win_average, static_cast<std::uint16_t>(rider)};
    }
    if (leaders.worst_win_average.rider == someone) leaders.worst_win_average.value = 0;
    if (!any_played) {
        leaders.total_score = {};
        leaders.best_score_average = leaders.worst_score_average = {};
    }
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

} // namespace

void print_high_scores(FrontEndState& state, const FrontEndContent& content) {
    // $80:D7CD-DACC; the text stream's operands are the computed leaders and their values.
    const auto leaders = high_score_leaders(state.records);
    TextVariables variables;
    variables.rider_names = state.records.rider_names;
    variables.word = [&](std::uint16_t address) { return high_score_word(leaders, address); };
    variables.place_object = [&](unsigned object, unsigned position) {
        place_printed_object(state, object, position);
    };
    print_text(state.text, state.printer, content.high_scores_text, content.character_table,
               &variables);
}

void print_player_scores(FrontEndState& state, const FrontEndContent& content) {
    // $80:DACF-DDE3: base bars, then the selected rider's overprinted values.
    const auto rider = state.records_detail.rider;
    const auto& stats = state.records.statistics[rider];
    const auto races = stats[0], wins = stats[1], failed = stats[2];
    const auto losses = static_cast<std::uint16_t>(races - wins - failed);
    TextVariables variables;
    variables.rider_names = state.records.rider_names;
    variables.word = [&](std::uint16_t address) -> std::uint16_t {
        switch (address) {
        case 0x00ca: return rider;
        case 0x00b2: return races;
        case 0x00b4: return static_cast<std::uint16_t>(races == 0 ? 0U : 100U * wins / races);
        case 0x00b6: return static_cast<std::uint16_t>(races == 0 ? 0U : 100U * losses / races);
        case 0x00b8: return static_cast<std::uint16_t>(races == 0 ? 0U : 100U * failed / races);
        case 0x00ba: return stats[3];
        default: throw std::invalid_argument("unknown PLAYER SCORES text operand");
        }
    };
    variables.place_object = [&](unsigned object, unsigned position) {
        place_printed_object(state, object, position);
    };
    print_text(state.text, state.printer, content.player_scores_text, content.character_table,
               &variables);
    print_text(state.text, state.printer, content.player_scores_values, content.character_table,
               &variables);
    // $80:DB9E-DC11 rewrites the template's five 16-cell bars. The first
    // and last cells are fixed; the middle fourteen show a rounded fraction
    // of the largest rider value (races/score) or 100 (percentages). R-0072's
    // cold, 1/2/3/4/5/6/10-race and 20/40/60/80/100-percent probes fix the
    // integer rule and the zero baseline on this screen.
    const auto maximum = [&](unsigned statistic) {
        std::uint16_t result = 0;
        for (const auto& other : state.records.statistics)
            result = std::max(result, other[statistic]);
        return result;
    };
    const std::array<unsigned, 5> values{races, races == 0 ? 0U : 100U * wins / races,
                                         races == 0 ? 0U : 100U * losses / races,
                                         races == 0 ? 0U : 100U * failed / races, stats[3]};
    const std::array<unsigned, 5> maxima{maximum(0), 100, 100, 100, maximum(3)};
    for (unsigned row = 0; row < values.size(); ++row) {
        const unsigned count =
            maxima[row] == 0
                ? 1U
                : std::min(15U, 1U + (14U * values[row] + maxima[row] / 2U) / maxima[row]);
        for (unsigned cell = 0; cell < 16; ++cell) {
            const unsigned x = 8 + cell, y = 6 + 2 * row;
            const std::uint16_t tile = static_cast<std::uint16_t>(cell < count ? 0x0cf : 0x0d0);
            state.text.words[y * 32 + x] = static_cast<std::uint16_t>(0x3c00 | tile);
            state.text.words[(y + 1) * 32 + x] = static_cast<std::uint16_t>(0x3c00 | (tile + 0x3c));
        }
    }
    load_cgram(state, asset(content, first_rider_palette + rider), 0x80);
    load_cgram(state, asset(content, 5), 0xe0); // $80:DB4E: bar gradient
}

void print_group_scores(FrontEndState& state, const FrontEndContent& content) {
    // $80:EF89-EFD3 copies the selected slot's eight (row, score) pairs from
    // $77:02C0, sorts them by score, and resolves row numbers to league members.
    // $80:DE2E-E008 prints the result and its bar lengths.
    for (unsigned x = 2; x < 32; ++x) {
        state.text.words[4 * 32 + x] = 0x3cce;
        state.text.words[5 * 32 + x] = 0x3d0a;
    }
    print_text(state.text, state.printer, content.group_scores_text, content.character_table);
    TextVariables variables;
    variables.league_names = state.records.league_names;
    const std::array<std::uint8_t, 5> name{0xf3, state.records_detail.tour, 0xfc, 0x04, 0xff};
    print_text(state.text, state.printer, name, content.character_table, &variables);
    const auto& scores = state.records.league_scores[state.records_detail.tour];
    std::array<unsigned, 8> order{};
    std::iota(order.begin(), order.end(), 0U);
    std::stable_sort(order.begin(), order.end(),
                     [&](unsigned a, unsigned b) { return scores[a] > scores[b]; });
    const unsigned top_score = scores[order[0]];
    const auto digits = std::to_string(top_score);
    std::vector<std::uint8_t> total{0xfe, static_cast<std::uint8_t>(29 - digits.size()), 0x07};
    total.insert(total.end(), digits.begin(), digits.end());
    total.push_back(0xff);
    print_text(state.text, state.printer, total, content.character_table);
    print_text(state.text, state.printer, content.group_scores_empty, content.character_table);
    std::array<unsigned, 8> members{};
    members.fill(16);
    unsigned member_count = 0;
    for (unsigned rider = 0; rider < 16 && member_count < 8; ++rider)
        if (state.records.league_members[state.records_detail.tour] & (0x8000U >> rider))
            members[member_count++] = rider;
    for (unsigned shown = 0; shown < member_count; ++shown) {
        const unsigned rider = members[order[shown]];
        if (rider >= 16) continue;
        variables.rider_names = state.records.rider_names;
        variables.word = [rider](std::uint16_t address) -> std::uint16_t {
            if (address != 0x00ca) throw std::invalid_argument("unknown GROUP SCORES name operand");
            return static_cast<std::uint16_t>(rider);
        };
        const std::array<std::uint8_t, 8> row_name{
            0xfe, 0x02, static_cast<std::uint8_t>(0x09 + 2 * shown), 0xf8, 0xca, 0x00, 0xff, 0xff};
        print_text(state.text, state.printer, row_name, content.character_table, &variables);
    }
    for (unsigned row = 0; row < 8; ++row) {
        const unsigned y = 9 + 2 * row;
        const unsigned score = scores[order[row]];
        const unsigned count =
            top_score == 0 ? 1U : std::min(15U, 1U + (14U * score + top_score / 2U) / top_score);
        for (unsigned cell = 0; cell < 16; ++cell) {
            const std::uint16_t tile = static_cast<std::uint16_t>(cell < count ? 0x0cf : 0x0d0);
            state.text.words[y * 32 + 13 + cell] = static_cast<std::uint16_t>(0x3c00 | tile);
            state.text.words[(y + 1) * 32 + 13 + cell] =
                static_cast<std::uint16_t>(0x3c00 | (tile + 0x3c));
        }
    }
    load_cgram(state, asset(content, 5), 0xe0);
}

namespace {

void track_grid_line(FrontEndState& state, const FrontEndContent& content, unsigned y,
                     const char* word, unsigned length) {
    std::array<std::uint8_t, 16> stream{};
    stream[0] = 0xfe;
    stream[1] = 9;
    stream[2] = static_cast<std::uint8_t>(y);
    for (unsigned k = 0; k < length; ++k) stream[3 + k] = static_cast<std::uint8_t>(word[k]);
    stream[3 + length] = 0xff;
    print_text(state.text, state.printer, std::span<const std::uint8_t>(stream.data(), 4 + length),
               content.character_table);
}

void track_grid_half(FrontEndState& state, const FrontEndContent& content, unsigned first_tour,
                     unsigned count, unsigned starting_row, unsigned map_word) {
    state.text.words.fill(cleared_text);
    for (unsigned i = 0; i < count; ++i) {
        const unsigned tour = first_tour + i;
        const unsigned row = starting_row + i * 6;
        draw_tour_picture(state, content, tour, row * 32 + 2);
        for (unsigned medal = 0; medal < 3; ++medal) {
            const unsigned y = row + 2 * medal;
            if (medal == 0) track_grid_line(state, content, y, "gold", 4);
            if (medal == 1) track_grid_line(state, content, y, "silver", 6);
            if (medal == 2) track_grid_line(state, content, y, "bronze", 6);
            for (unsigned track = 0; track < 5; ++track) {
                const unsigned x = 16 + 3 * track;
                const unsigned index = tour * 5 + track;
                const bool placed =
                    state.records.record_times[medal][index] != (track == 2 ? 0 : 0xea60);
                // $80:EEF2-EF35 maps the holder to an eight-glyph icon and
                // its palette. The same rank changes from blue to red when
                // only its holder changes (track 0, R-0072).
                constexpr std::array<std::uint8_t, 16> holder_palettes{4, 3, 0, 2, 3, 3, 4, 0,
                                                                       3, 0, 4, 0, 2, 1, 0, 5};
                const unsigned holder = state.records.record_holders[medal][index];
                const std::uint16_t top =
                    holder < holder_palettes.size()
                        ? static_cast<std::uint16_t>(
                              0x20d1U + (holder & 7U)
                              + (static_cast<unsigned>(holder_palettes[holder]) << 10U))
                        : 0x3cd9;
                const std::uint16_t dot_top = placed ? top : 0x3cd9;
                const std::uint16_t dot_bottom =
                    placed ? static_cast<std::uint16_t>(top + 0x3cU) : 0x3d15;
                state.text.words[y * 32 + x] = dot_top;
                state.text.words[y * 32 + x + 1] = dot_top | 0x4000;
                state.text.words[(y + 1) * 32 + x] = dot_bottom;
                state.text.words[(y + 1) * 32 + x + 1] = dot_bottom | 0x4000;
            }
        }
    }
    load_text(state, map_word);
}

void track_records_hdma(FrontEndState& state) {
    // $80:EB76 copies the two mode-2 HDMA tables at $80:ECD1/ECF7. Channel 6
    // switches BG2 to the grid for lines 40-167; channel 5 supplies its
    // scrolling vertical offset. The last writes remain in the PPU registers.
    auto add = [&](unsigned row, SnesLineRegisterName name, unsigned value) {
        state.line_registers.push_back(
            {static_cast<std::uint8_t>(row), name, static_cast<std::uint16_t>(value)});
    };
    add(0, SnesLineRegisterName::bg2_horizontal_offset, 0);
    add(0, SnesLineRegisterName::bg2_vertical_offset, 0x100);
    add(39, SnesLineRegisterName::bg2_vertical_offset, 0x104);
    add(40, SnesLineRegisterName::bg2_horizontal_offset, 0x100);
    add(40, SnesLineRegisterName::bg2_vertical_offset, state.records_detail.track_scroll);
    add(168, SnesLineRegisterName::bg2_horizontal_offset, 0);
    add(168, SnesLineRegisterName::bg2_vertical_offset, 0x08d);
    add(169, SnesLineRegisterName::bg2_vertical_offset, 0x105);
}

void print_track_selected_times(FrontEndState& state, const FrontEndContent& content) {
    // $80:D699-D6B1 derives the selected tour from the BG2 grid scroll:
    // a half-pixel shift, three 16-pixel rows per tour, and a five-track
    // stride. The 5-bit row wraps while the scroll itself keeps moving.
    const auto delta = static_cast<std::uint16_t>(state.records_detail.track_scroll - 0x01a7U);
    const unsigned row = (((delta >> 1U) + 1U) >> 3U) & 31U;
    const unsigned tour = row / 3U;
    if (tour >= 9U) return; // $80:D6AE's bounds branch retains the last lines.
    state.text.words.fill(cleared_text);
    const unsigned track = tour * 5U + state.records_detail.rider;
    for (unsigned rank = 0; rank < 3; ++rank) {
        const auto time = state.records.record_times[rank][track];
        const auto holder = state.records.record_holders[rank][track];
        // $80:D6D5-D6FF: each selected holder colours one of the three
        // accompanying object rows, including SOMEONE at palette asset 22.
        load_cgram(state, asset(content, first_rider_palette + holder), 0x80 + 16 * rank);
        std::vector<std::uint8_t> stream{0xfe, 0x06, static_cast<std::uint8_t>(22 + 2 * rank)};
        const auto display = race_time_text(time, content.time_words);
        stream.insert(stream.end(), display.begin(), display.end());
        constexpr std::array<std::uint8_t, 4> by{'_', 'b', 'y', '_'};
        stream.insert(stream.end(), by.begin(), by.end());
        stream.insert(stream.end(), {0xf8, 0xbc, 0x00, 0xff});
        TextVariables variables;
        variables.rider_names = state.records.rider_names;
        variables.word = [holder](std::uint16_t address) -> std::uint16_t {
            if (address != 0x00bc) throw std::invalid_argument("unknown TRACK RECORDS holder");
            return holder;
        };
        print_text(state.text, state.printer, stream, content.character_table, &variables);
    }
    for (unsigned y = 22; y < 28; ++y)
        for (unsigned x = 0; x < 32; ++x) {
            const unsigned at = (0x1800 + y * 32 + x) * 2;
            const auto word = state.text.words[y * 32 + x];
            state.video.vram[at] = static_cast<std::uint8_t>(word);
            state.video.vram[at + 1] = static_cast<std::uint8_t>(word >> 8);
        }
}

} // namespace

void print_track_records(FrontEndState& state, const FrontEndContent& content) {
    // $80:D5E6-D7C0: the fixed title and three selected times occupy the
    // lower-left BG2 map; the five-tour grid wraps across its right halves.
    state.text.words.fill(cleared_text);
    load_cgram(state, asset(content, 35), 0);
    load_cgram(state, asset(content, 36), 0x40);
    for (unsigned slot = 0; slot < 3; ++slot)
        load_cgram(state, asset(content, 22), 0x80 + 16 * slot);
    load_vram(state, asset(content, 68).last(0x780), 0x3d80);
    print_text(state.text, state.printer, content.track_records_text, content.character_table);
    TextVariables variables;
    variables.place_object = [&](unsigned object, unsigned position) {
        place_printed_object(state, object, position);
    };
    print_text(state.text, state.printer, content.track_records_objects, content.character_table,
               &variables);
    for (unsigned rank = 0; rank < 3; ++rank) {
        // $80:D641-D660 raises the printed icons five pixels and places the
        // three medal objects beside the selected record lines.
        oam_byte(state, 104 + rank, 1) =
            static_cast<std::uint8_t>(oam_byte(state, 104 + rank, 1) - 5);
        oam_byte(state, 112 + rank, 0) = 32;
        oam_byte(state, 112 + rank, 1) = static_cast<std::uint8_t>(174 + 16 * rank);
        oam_byte(state, 112 + rank, 2) = static_cast<std::uint8_t>(0xa6 + 2 * rank);
        oam_byte(state, 112 + rank, 3) =
            static_cast<std::uint8_t>(rank == 2 ? 0x1d : 0x17 + 2 * rank);
    }
    high_bits(state, 104) = 0x40;
    high_bits(state, 108) = 0x00;
    high_bits(state, 112) = 0x40;
    load_text(state, 0x1800);
    track_grid_half(state, content, 0, 5, 2, 0x1400);
    track_grid_half(state, content, 5, 4, 0, 0x1c00);
    state.records_detail.track_scroll = 0x01e7;
    print_track_selected_times(state, content);
    state.registers.bg[1].hofs = 0;
    state.registers.bg[1].vofs = 0x105;
    load_cgram(state, asset(content, 34), 0xb0);
    load_cgram(state, asset(content, 33), 0xc0);
    load_cgram(state, asset(content, 32), 0xe0);
    state.arrow.target_x = 0x0600;
    state.arrow.target_y = 0x0740;
    state.records_detail.track_scroll_step = 0;
    track_records_hdma(state);
}

void set_bar_objects(FrontEndState& state) {
    // $80:DDE3 and $80:E043: five and eight large 32-pixel bar ends. Window
    // 1 masks OBJ outside the red bar, leaving a moving coloured edge.
    const bool group = state.records_detail.category == 3;
    const unsigned count = group ? 8U : 5U;
    const std::uint8_t first_x = static_cast<std::uint8_t>(group ? 164 : 118);
    const std::uint8_t first_y = static_cast<std::uint8_t>(group ? 72 : 48);
    for (unsigned k = 0; k < count; ++k) {
        oam_byte(state, 4 + k, 0) = first_x;
        oam_byte(state, 4 + k, 1) = static_cast<std::uint8_t>(first_y + 16 * k);
        oam_byte(state, 4 + k, 2) = 72;
        oam_byte(state, 4 + k, 3) = 0x3d;
    }
    high_bits(state, 4) = 0xaa;
    high_bits(state, 8) = group ? 0xaa : 0x56;
    state.registers.object_window_one_inverted = true;
    state.registers.object_window_left = static_cast<std::uint8_t>(group ? 104 : 64);
    state.registers.object_window_right = static_cast<std::uint8_t>(group ? 231 : 191);
}

void load_high_score_palettes(FrontEndState& state, const FrontEndContent& content) {
    // $80:D917-D9A8: the eight leaders supply the eight object palettes.
    const auto leaders = high_score_leaders(state.records);
    const std::array<std::uint16_t, 8> riders{leaders.wins.rider,
                                              leaders.losses.rider,
                                              leaders.races.rider,
                                              leaders.best_win_average.rider,
                                              leaders.worst_win_average.rider,
                                              leaders.total_score.rider,
                                              leaders.best_score_average.rider,
                                              leaders.worst_score_average.rider};
    for (unsigned k = 0; k < riders.size(); ++k)
        load_cgram(state, asset(content, first_rider_palette + riders[k]), 0x80 + 16 * k);
}

void records_detail_entry_frame(FrontEndState& state, const FrontEndContent& content) {
    if (state.records_detail.category == 0) {
        copy_oam(state);
        if (state.script_frame == 1) {
            print_track_records(state, content);
            return;
        }
        track_records_hdma(state);
        if (state.script_frame >= 82) {
            // The PAL frame-1200 wave and arrow snapshots are one animation
            // step ahead of the shared menu loop at this entry boundary.
            // Keep the medal objects on their previously sampled step.
            for (unsigned k = 0; k < 8; ++k) {
                state.decorations.wave[k] =
                    static_cast<std::uint8_t>((state.decorations.wave[k] + 1U) % 20U);
                oam_byte(state, 104 + k, 2) =
                    content.decoration_frames[wave_tiles + state.decorations.wave[k]];
            }
            state.arrow.spin = static_cast<std::uint8_t>((state.arrow.spin + 1U) & 31U);
            state.screen = FrontEndScreen::records_detail;
        }
        return;
    }
    if (state.script_frame == 1) {
        copy_oam(state);
        state.text.words.fill(cleared_text);
        if (state.records_detail.category == 1)
            print_high_scores(state, content);
        else if (state.records_detail.category == 2)
            print_player_scores(state, content);
        else if (state.records_detail.category == 3)
            print_group_scores(state, content);
        else
            throw std::runtime_error("RECORDS detail category is not yet recovered");
        load_text(state, state.slide.hidden_half);
        if (state.records_detail.category == 2 || state.records_detail.category == 3)
            set_bar_objects(state);
        high_bits(state, 104) = high_bits(state, 108) = four_hidden;
        return;
    }
    if (state.script_frame == 2) {
        copy_oam(state);
        send_arrow_off(state);
        start_slide(state, content, false);
        return;
    }
    if (!slide_frame(state, content)) return;
    high_bits(state, 104) = high_bits(state, 108) = four_shown;
    if (state.records_detail.category == 1) load_high_score_palettes(state, content);
    if (state.records_detail.category == 3)
        for (unsigned k = 0; k < 8; ++k)
            oam_byte(state, 4 + k, 0) = static_cast<std::uint8_t>(oam_byte(state, 4 + k, 0) - 8);
    state.screen = FrontEndScreen::records_detail;
}

void update_track_records(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    // The printer consumes the grid offset from the preceding picture;
    // the current input then advances the HDMA scroll for the next one.
    print_track_selected_times(state, content);
    if (state.script_frame > 3) step_decorations(state, content);
    constexpr std::array<std::uint8_t, 12> scroll_steps{1, 2, 2, 3, 4, 5, 4, 4, 2, 2, 2, 1};
    const auto pad = pads.one;
    const bool down = (pad & (pad_down | pad_select)) != 0;
    const bool up = !down && (pad & pad_up) != 0;
    if (!down && !up) state.latches.moved = false;
    if ((down || up) && !state.latches.moved) {
        state.latches.moved = true;
        state.records_detail.track_scroll_up = up;
        state.records_detail.track_scroll_step = 1;
    }
    const bool right = (pad & pad_right) != 0;
    const bool left = !right && (pad & pad_left) != 0;
    if (!right && !left) state.latches.up = false;
    if ((right || left) && !state.latches.up) {
        state.latches.up = true;
        auto& column = state.records_detail.rider;
        column = static_cast<std::uint8_t>(right ? (column + 1U) % 5U : (column + 4U) % 5U);
        state.arrow.target_x = static_cast<std::uint16_t>(0x0600 + column * 0x180);
    }
    if (state.records_detail.track_scroll_step != 0) {
        const auto step = state.records_detail.track_scroll_step - 1U;
        const auto pixels = scroll_steps[step];
        auto& offset = state.records_detail.track_scroll;
        offset = static_cast<std::uint16_t>(state.records_detail.track_scroll_up ? offset - pixels
                                                                                 : offset + pixels);
        state.records_detail.track_scroll_step =
            static_cast<std::uint8_t>(step + 1U == scroll_steps.size() ? 0U : step + 2U);
    }
    track_records_hdma(state);
    if (pad & (back_buttons | choose_buttons)) state.screen = FrontEndScreen::records_detail_exit;
}

std::array<unsigned, 8> score_bar_targets(const FrontEndState& state) {
    std::array<unsigned, 8> targets{};
    if (state.records_detail.category == 2) {
        const auto& stats = state.records.statistics[state.records_detail.rider];
        const unsigned races = stats[0];
        const unsigned wins = stats[1];
        const unsigned failed = stats[2];
        const std::array<unsigned, 5> values{races, races == 0 ? 0U : 100U * wins / races, 0U,
                                             races == 0 ? 0U : 100U * failed / races, stats[3]};
        unsigned max_races = 0, max_score = 0;
        for (const auto& other : state.records.statistics) {
            max_races = std::max<unsigned>(max_races, other[0]);
            max_score = std::max<unsigned>(max_score, other[3]);
        }
        const std::array<unsigned, 5> maxima{max_races, 100, 100, 100, max_score};
        for (unsigned k = 0; k < values.size(); ++k)
            targets[k] = 48 + (maxima[k] == 0 ? 0U : 128U * values[k] / maxima[k]);
        // $83:8FDE-8FE8 gives the remaining pixels to LOST after
        // truncating WON and FAILED separately. At 3/1/1 in five races
        // the endpoints are 124, 75, 73, rather than three independent
        // rounded percentages.
        if (races != 0) targets[2] = 48 + 128 - (targets[1] - 48) - (targets[3] - 48);
    } else {
        const auto& scores = state.records.league_scores[state.records_detail.tour];
        std::array<unsigned, 8> order{};
        std::iota(order.begin(), order.end(), 0U);
        std::stable_sort(order.begin(), order.end(),
                         [&](unsigned a, unsigned b) { return scores[a] > scores[b]; });
        const unsigned top = scores[order[0]];
        for (unsigned k = 0; k < order.size(); ++k)
            targets[k] = 88 + (top == 0 ? 0U : 128U * scores[order[k]] / top);
    }
    return targets;
}

void update_score_bar_objects(FrontEndState& state) {
    const auto targets = score_bar_targets(state);
    for (unsigned k = 0; k < (state.records_detail.category == 3 ? 8U : 5U); ++k) {
        auto& x = oam_byte(state, 4 + k, 0);
        if (state.records_detail.category == 3 && state.script_frame == 1)
            --x; // PAL entry-to-loop boundary retains one downward step.
        else if (x < targets[k])
            ++x;
        else if (x > targets[k])
            --x;
    }
}

void records_detail_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    copy_oam(state);
    if (state.records_detail.category == 0) {
        update_track_records(state, content, pads);
        return;
    }
    if (state.records_detail.category == 2 || state.records_detail.category == 3)
        update_score_bar_objects(state);
    // $80:D9AC-D9B7 runs three waits after the slide before the record loop.
    if (state.script_frame > 3) step_decorations(state, content);
    if (state.records_detail.category == 2) {
        const bool down = (pads.one & pad_down) != 0;
        const bool up = !down && (pads.one & pad_up) != 0;
        if (!down && !up) state.latches.moved = false;
        if ((down || up) && !state.latches.moved) {
            state.latches.moved = true;
            auto& rider = state.records_detail.rider;
            rider = static_cast<std::uint8_t>(down ? (rider + 15U) & 15U : (rider + 1U) & 15U);
            state.text.words.fill(cleared_text);
            print_player_scores(state, content);
            load_text(state, state.slide.shown_half);
        }
    }
    if (!(pads.one & (back_buttons | choose_buttons))
        && !(state.records_detail.category == 2 && (pads.one & pad_right)))
        return;
    state.screen = FrontEndScreen::records_detail_exit;
}

void records_detail_exit_frame(FrontEndState& state, const FrontEndContent& content) {
    // $80:D9B7-D9D7 restores the menu palettes and hides score-row objects.
    if (state.records_detail.category == 0) {
        // TRACK RECORDS replaced the high BG1 logo tiles with its tour badges
        // and used BG2's vertical offset for the scrolling grid. Its return
        // restores both before the records menu slides back into view.
        state.registers.bg[1].vofs = 0;
        reload_menu_palette(state, content);
        reload_menu_text_tiles(state, content);
        high_bits(state, 112) = four_hidden;
    }
    if (state.records_detail.category == 2 || state.records_detail.category == 3) {
        for (unsigned k = 4; k < 12; ++k) {
            oam_byte(state, k, 0) = 1;
            oam_byte(state, k, 1) = 1;
            oam_byte(state, k, 2) = 1;
            oam_byte(state, k, 3) = 1;
        }
        high_bits(state, 4) = high_bits(state, 8) = four_hidden;
    }
    high_bits(state, 104) = high_bits(state, 108) = four_hidden;
    load_object_palette(state, content);
    load_cgram(state, asset(content, 28), 0xd0);
    load_cgram(state, asset(content, 5), 0xe0);
    state.registers.object_window_one_inverted = false;
    state.logo.raised = false;
    enter_records_menu(state, content);
    state.records_detail.returning = true;
}

void enter_define_player_warning(FrontEndState& state, const FrontEndContent& content) {
    // $80:C12A-C1EA: show the warning for the chosen rider. Its F8 operand reads
    // the selected rider from $00CA and that rider's mutable SRAM name.
    state.text.words.fill(cleared_text);
    TextVariables variables;
    variables.word = [&](std::uint16_t address) -> std::uint16_t {
        if (address != 0x00ca) throw std::invalid_argument("unknown warning text operand");
        return state.rider_menu.rider;
    };
    variables.rider_names = state.records.rider_names;
    print_text(state.text, state.printer, content.define_player_warning, content.character_table,
               &variables);
    print_text(state.text, state.printer, content.define_player_confirm_prompt,
               content.character_table);
    load_text(state, state.slide.hidden_half);
    state.screen = FrontEndScreen::define_player_warning_entry;
}

void define_player_warning_entry_frame(FrontEndState& state, const FrontEndContent& content) {
    if (state.script_frame == 1) {
        copy_oam(state);
        start_slide(state, content, false);
        return;
    }
    if (!slide_frame(state, content)) return;
    state.arrow.target_x = 0x0100;
    state.arrow.target_y = 0x0580;
    state.screen = FrontEndScreen::define_player_warning;
}

namespace {

void clear_defined_player(FrontEndState& state) {
    // The SRAM removal is visible on the same picture as SELECT+Y+A. A name
    // record's first eight characters clear; its remaining bytes stay intact.
    const auto rider = state.rider_menu.rider;
    auto& records = state.records;
    std::fill_n(records.rider_names.begin() + static_cast<std::size_t>(rider) * 16, 8,
                std::uint8_t{'_'});
    records.statistics[rider].fill(0);
    records.tour_levels[rider] = 0;
    const auto cold = cold_start_records();
    for (unsigned track = 0; track < 50; ++track)
        records.best[static_cast<std::size_t>(rider) * 50 + track] =
            cold.best[static_cast<std::size_t>(rider) * 50 + track];
    for (unsigned tour = 0; tour < 10; ++tour)
        records.medals[tour * 16 + rider] = cold.medals[tour * 16 + rider];
    for (unsigned rank = 0; rank < 3; ++rank)
        for (unsigned track = 0; track < 50; ++track)
            if (records.record_holders[rank][track] == rider) {
                records.record_holders[rank][track] = someone;
            }
}

} // namespace

void define_player_warning_frame(FrontEndState& state, FrontEndPads pads) {
    copy_oam(state);
    constexpr std::uint16_t confirm = pad_select | 0x4000U | pad_a;
    if ((pads.one & confirm) != confirm) return;
    clear_defined_player(state);
    state.text.words.fill(cleared_text);
    state.screen = FrontEndScreen::define_player_after_confirm;
}

void define_player_after_confirm_frame(FrontEndState& state, const FrontEndContent& content) {
    enter_rename_editor(state, content);
}

namespace {

void initialize_keyboard_cursor(FrontEndState& state) {
    // $80:A212-A22B before the first $80:A231 poll.
    oam_byte(state, 123, 0) = 95;
    oam_byte(state, 123, 1) = 3;
    oam_byte(state, 123, 2) = 12;
    oam_byte(state, 123, 3) = 11;
    high_bits(state, 123) &= static_cast<std::uint8_t>(~hidden_bit(123));
}

} // namespace

void enter_rename_editor(FrontEndState& state, const FrontEndContent& content) {
    // $80:D468-D47D prints the shared keyboard and the RENAME PLAYER prompt.
    state.keyboard = {};
    state.menu.selection = 0; // $009B in $80:A207
    // $00DC was the text printer's scratch for the last displayed rider, STEVE on this
    // picker. The keyboard overwrites from its first byte and deliberately keeps the tail.
    state.keyboard.scratch[0] = state.keyboard.scratch[1] = 0xfb;
    std::copy_n(state.records.rider_names.begin() + 15 * 16, 8, state.keyboard.scratch.begin() + 2);
    state.keyboard.scratch[10] = state.keyboard.scratch[11] = 0xff;
    state.text.words.fill(cleared_text);
    print_text(state.text, state.printer, content.keyboard_text, content.character_table);
    print_text(state.text, state.printer, content.rename_prompt, content.character_table);
    load_text(state, state.slide.hidden_half);
    state.screen = FrontEndScreen::rename_entry;
}

void enter_league_editor(FrontEndState& state, const FrontEndContent& content) {
    // $80:9E57-9E7C shares the keyboard but starts with $80:9E0F's scratch.
    state.keyboard = {};
    state.keyboard.scratch = {0xf9, 0x07, 0xfe, 0x1b, 0x0d, 0x58,
                              0xfb, 0xff, 0xfe, 0x5f, 0xff, 0xff};
    // $80:9E3C-9E54; R-0073: retain the last marker's column/row, including bytes after FF FF.
    state.keyboard.scratch[3] = state.league.marker_column;
    state.keyboard.scratch[4] = state.league.marker_row;
    state.league.editor_active = true;
    state.rider_menu.rider = 0xff;
    state.menu.selection = 0;
    state.text.words.fill(cleared_text);
    print_text(state.text, state.printer, content.keyboard_text, content.character_table);
    print_text(state.text, state.printer, content.league_prompt, content.character_table);
    load_text(state, state.slide.hidden_half);
    state.screen = FrontEndScreen::rename_entry;
}

void rename_entry_frame(FrontEndState& state, const FrontEndContent& content) {
    if (state.script_frame == 1) {
        copy_oam(state);
        start_slide(state, content, false);
        return;
    }
    if (!slide_frame(state, content)) return;
    state.screen = FrontEndScreen::rename_keyboard;
}

namespace {

std::uint8_t keyboard_key(const KeyboardEditor& editor, std::span<const std::uint8_t> table) {
    unsigned base = 0;
    switch (editor.offset_y) {
    case 0: base = 2; break;    // A-M at $80:A4DD
    case -24: base = 22; break; // N-Z at $80:A4F1
    case -48: base = 38; break; // backspace and digits at $80:A501
    case -72: base = 54; break; // punctuation and OK at $80:A511
    default: throw std::logic_error("keyboard row outside the original table");
    }
    const auto column = static_cast<unsigned>(-editor.offset_x)
                      / static_cast<unsigned>(editor.offset_y == -72 ? 8 : 16);
    const auto index = base + column + (editor.offset_y == -72 ? 1 : 0);
    if (index >= table.size()) throw std::logic_error("keyboard column outside the original table");
    return table[index];
}

void position_keyboard_cursor(FrontEndState& state) {
    constexpr unsigned cursor = 123; // $0BEC, the selected character's brown overlay
    oam_byte(state, cursor, 0) = static_cast<std::uint8_t>(24 - state.keyboard.offset_x);
    oam_byte(state, cursor, 1) = static_cast<std::uint8_t>(23 - state.keyboard.offset_y);
}

std::uint8_t stored_character(std::uint8_t key) {
    if (key == ' ') return 0x60;
    if (key >= 'A' && key <= 'Z') return static_cast<std::uint8_t>(key | 0x20U);
    if (key >= 0x16 && key < 0x20) return static_cast<std::uint8_t>(key + 0x1aU);
    return key;
}

// $80:A416-A445: the last character erased with the result's sound, or the refusal's when the
// name is empty.
void erase_keyboard_character(FrontEndState& state, const FrontEndContent& content) {
    if (state.keyboard.length == 0) {
        play_menu_sound(state, MenuSound::refused); // $80:A41A
        return;
    }
    play_menu_sound(state, MenuSound::result); // $80:A420
    --state.keyboard.length;
    --state.printer.position;
    constexpr std::array<std::uint8_t, 2> dot{'.', 0xff};
    print_text(state.text, state.printer, dot, content.character_table);
    --state.printer.position;
}

} // namespace

void rename_keyboard_frame(FrontEndState& state, const FrontEndContent& content,
                           FrontEndPads pads) {
    copy_oam(state);
    load_text(state, state.slide.shown_half);
    auto& editor = state.keyboard;
    if (state.script_frame == 1) initialize_keyboard_cursor(state);
    // $80:A231 positions the cursor before polling, so a direction affects OAM one
    // frame after it changes the key offsets.
    if (state.script_frame > 1) position_keyboard_cursor(state);
    const auto pad = pads.one;
    const auto pressed = static_cast<std::uint16_t>(pad & ~editor.previous_buttons);
    editor.previous_buttons = pad;
    // Each move plays the navigation sound: $80:A2BA (Left), $80:A2F9 (Right), $80:A34A (Up),
    // $80:A326 (Down).
    if (pressed & (pad_left | pad_right | pad_up | pad_down))
        play_menu_sound(state, MenuSound::navigate);
    if (pressed & pad_left) {
        editor.offset_x =
            editor.offset_x == 0 ? -192 : static_cast<std::int16_t>(editor.offset_x + 16);
        state.menu.selection = static_cast<std::uint8_t>((state.menu.selection + 1) % 39);
    } else if (pressed & pad_right) {
        editor.offset_x =
            editor.offset_x == -192 ? 0 : static_cast<std::int16_t>(editor.offset_x - 16);
        state.menu.selection = state.menu.selection == 0 ? 39 : state.menu.selection - 1;
    } else if (pressed & pad_up) {
        editor.offset_y =
            editor.offset_y == 0 ? -72 : static_cast<std::int16_t>(editor.offset_y + 24);
    } else if (pressed & pad_down) {
        editor.offset_y =
            editor.offset_y == -72 ? 0 : static_cast<std::int16_t>(editor.offset_y - 24);
    }
    if (pressed & back_buttons) { // Y or X: direct backspace, $80:A29B
        state.arrow.target_x = 0x0100;
        state.arrow.target_y = 0x04e0;
        erase_keyboard_character(state, content);
        return;
    }
    if (!(pressed & choose_buttons)) return;
    // $80:A3A0-A3BC aims at the selected key, in sixteenths of a pixel.
    state.arrow.target_x = static_cast<std::uint16_t>(-editor.offset_x * 16);
    state.arrow.target_y = static_cast<std::uint16_t>((31 - editor.offset_y) * 16);
    const auto key = keyboard_key(editor, content.keyboard_text);
    if (key == 0x40) { // OK at $80:A52A
        if (editor.length == 0) {
            play_menu_sound(state, MenuSound::refused); // $80:A493 to $80:A41A
            return;
        }
        play_menu_sound(state, MenuSound::select); // $80:A495
        editor.scratch[editor.length] = editor.scratch[editor.length + 1] = 0xff;
        state.screen = FrontEndScreen::rename_commit;
        return;
    }
    if (key == 0x3c) { // backspace
        erase_keyboard_character(state, content);
        return;
    }
    play_menu_sound(state, MenuSound::result); // $80:A449, before the length test
    if (editor.length >= 8) return;
    editor.scratch[editor.length] = stored_character(key);
    const std::array<std::uint8_t, 2> shown{editor.scratch[editor.length], 0xff};
    print_text(state.text, state.printer, shown, content.character_table);
    ++editor.length;
}

void rename_commit_frame(FrontEndState& state, const FrontEndContent& content) {
    // The save path makes one final OAM upload after the arrow's frame update.
    // The keyboard's preceding frame retained the older sprite position.
    copy_oam(state);
    if (state.league.editor_active) {
        const auto first = static_cast<std::size_t>(state.league.slot) * 32U;
        std::copy(state.keyboard.scratch.begin(), state.keyboard.scratch.end(),
                  state.records.league_names.begin() + first);
        state.records.active_league_members = state.league.members;
        state.records.league_naming = false;
        state.menu.selection = 0;
        high_bits(state, 123) |= hidden_bit(123);
        state.logo.raised = false;
        state.screen = FrontEndScreen::rename_return;
        return;
    }
    const auto first = static_cast<std::size_t>(state.rider_menu.rider) * 16U;
    std::copy_n(state.keyboard.scratch.begin(), 8, state.records.rider_names.begin() + first);
    // $80:D1FA clears the map. The reference's save frame has the last two
    // options and the first five characters of RENAME PLAYER; printing resumes
    // on the next frame ($0200-$09FF at frames 2001-2002, R-0072).
    state.text.words.fill(cleared_text);
    for (unsigned row : {4U, 3U}) {
        const auto entry = nth_string(content.options_menu_text, row);
        print_text(state.text, state.printer, {entry.data(), entry.size() + 1},
                   content.character_table);
    }
    const auto rename = nth_string(content.options_menu_text, 2);
    std::array<std::uint8_t, 9> partial{0xfe, 3, 16};
    std::copy_n(rename.begin() + 2, 5, partial.begin() + 3);
    partial[8] = 0xff;
    print_text(state.text, state.printer, partial, content.character_table);
    state.text.words[17 * 32 + 12] = cleared_text; // the fifth lower tile lands next frame
    high_bits(state, 123) |= hidden_bit(123);
    state.logo.raised = false; // $80:D4BF calls $80:F52B as the editor ends
    state.screen = FrontEndScreen::rename_return;
}

void rename_return_frame(FrontEndState& state, const FrontEndContent& content) {
    state.slide.countdown = 0;
    state.league.editor_active = false;
    return_to_options_menu(state, content);
}

} // namespace unirally::front_end_screens
