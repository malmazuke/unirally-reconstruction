// PLAYER SCORES (`$80:DACF`) and GROUP TABLES (`$80:DDFC`) under RECORDS (R-0072, R-0097): the
// screen printed and slid in, each bar's end object moving a pixel a frame to its target and
// redrawing the bar's text as it goes, Up and Down (or Select) changing the rider or the league
// shown, and the way back to the RECORDS menu.
#include "front_end_screens.hpp"

#include <algorithm>
#include <array>
#include <numeric>
#include <stdexcept>
#include <vector>

namespace unirally::front_end_screens {
namespace {

constexpr unsigned first_bar_object = 4, bar_colours_asset = 5, last_rider = 15;
constexpr std::uint8_t league_slots = 6;

// What tells the two screens' bars apart.
struct BarLayout {
    unsigned rows;
    // The end objects' first place, tile and attributes ($80:DDE4, $80:E04B).
    std::uint8_t first_x, first_y, tile, attributes;
    // The forward slide hides the objects when BG2's offset reaches this (`$80:DC74`, `$80:DFC2`).
    std::uint8_t hide_at_scroll;
    // The objects' window, `$007A` and `$007B` ($80:DB36-DB3D, $80:DEBB-DEC7).
    std::uint8_t window_left, window_right;
    // A bar's text row is first_text_row + 2 x row. Its edge is redrawn while the object's x / 8
    // is in [first_column, last_column): PLAYER SCORES on a multiple of 8 (`$80:DBC1`), GROUP
    // TABLES off one (`$80:DF04`).
    unsigned first_text_row;
    std::uint8_t first_column, last_column;
    bool redraws_on_tile;
};
constexpr BarLayout player_bars{5, 0x76, 0x30, 0x48, 0x3d, 0x74, 0x40, 0xbf, 6, 7, 22, true};
constexpr BarLayout group_bars{8, 0xa4, 0x48, 0x48, 0x3d, 0x9c, 0x68, 0xe7, 9, 12, 27, false};

const BarLayout& bar_layout(const FrontEndState& state) {
    return state.records_detail.category == player_scores_category ? player_bars : group_bars;
}

// $80:DAF0-DAFA, $80:DE16-DE24: the end objects' first place (the low table only).
void place_bar_objects(FrontEndState& state, const BarLayout& layout) {
    for (unsigned row = 0; row < layout.rows; ++row) {
        const auto entry = first_bar_object + row;
        oam_byte(state, entry, 0) = layout.first_x;
        oam_byte(state, entry, 1) = static_cast<std::uint8_t>(layout.first_y + 16 * row);
        oam_byte(state, entry, 2) = layout.tile;
        oam_byte(state, entry, 3) = layout.attributes;
    }
}

// The forward slide's pass ($80:DC74-DC97, $80:DFC2-DFE5): at one offset the objects go off
// screen ($80:DC9A, $80:DFE8; GROUP TABLES also turns the colour math back on), then each moves
// left with the text.
void slide_bars_in(FrontEndState& state, const BarLayout& layout) {
    if ((state.slide.scroll & 0xffU) == layout.hide_at_scroll) {
        for (unsigned row = 0; row < layout.rows; ++row)
            oam_byte(state, first_bar_object + row, 0) = 0xff;
        high_bits(state, 4) = 0xaa;
        high_bits(state, 8) = layout.rows == group_bars.rows ? 0xaa : 0x56;
        if (layout.rows == group_bars.rows) state.registers.colour_math = 0x7f;
    }
    for (unsigned row = 0; row < layout.rows; ++row) {
        auto& x = oam_byte(state, first_bar_object + row, 0);
        x = static_cast<std::uint8_t>(x - state.slide.speed);
    }
}

// The back slide's pass ($80:DCB6-DCEE, $80:E010-E048): the objects and the window move right
// with the text, stopping at the right edge.
void slide_bars_out(FrontEndState& state, const BarLayout& layout) {
    const auto add = [&](std::uint8_t& value) {
        const unsigned sum = value + state.slide.speed;
        value = static_cast<std::uint8_t>(sum > 0xff ? 0xff : sum);
        return sum > 0xff;
    };
    for (unsigned row = 0; row < layout.rows; ++row)
        add(oam_byte(state, first_bar_object + row, 0));
    auto& detail = state.records_detail;
    add(detail.window_right);
    if (add(detail.window_left)) detail.window_right = 0xfe;
}

// $80:DB29-DB47, $80:DEB3-DED2: no colour math, OBJ window 1 inverted around the bars.
void open_bar_window(FrontEndState& state, const BarLayout& layout) {
    auto& registers = state.registers;
    registers.colour_math = 0;
    registers.object_window_one_inverted = true;
    state.records_detail.window_left = registers.object_window_left = layout.window_left;
    state.records_detail.window_right = registers.object_window_right = layout.window_right;
}

// $80:DBAC-DC71, $80:DEEF-DF36: a bar's end object a pixel toward its target, and the bar's
// edge (a filled cell, then an empty one) printed at the object's column.
void step_bar(FrontEndState& state, const FrontEndContent& content, const BarLayout& layout,
              unsigned row) {
    auto& x = oam_byte(state, first_bar_object + row, 0);
    const auto target = state.records_detail.bar_targets[row];
    if (x > target)
        --x;
    else if (x < target)
        ++x;
    if (((x & 7U) == 0) != layout.redraws_on_tile) return;
    const auto column = static_cast<std::uint8_t>(x >> 3U);
    if (column < layout.first_column || column >= layout.last_column) return;
    constexpr std::uint8_t filled = 0x7b, empty = 0x7c;
    const std::array<std::uint8_t, 6> edge{
        print_code::place,
        static_cast<std::uint8_t>(column + 1U),
        static_cast<std::uint8_t>(layout.first_text_row + 2 * row),
        filled,
        empty,
        print_code::end};
    print_text(state.text, state.printer, edge, content.character_table);
}

// $80:C206: a press of any button of either pad on two frames running, each frame copying the
// OAM and stepping the decorations; then `$80:C236` hides entries 30-35.
bool press_seen_twice(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    copy_oam(state);
    step_decorations(state, content);
    const bool press = (pads.one | pads.two) != 0;
    auto& seen = state.records_detail.press_seen;
    if (!seen || !press) {
        seen = press;
        return false;
    }
    high_bits(state, 32) = four_hidden;
    high_bits(state, 28) = static_cast<std::uint8_t>((high_bits(state, 28) & 0x0fU) | 0x50U);
    return true;
}

void restore_hidden_half(FrontEndState& state) {
    state.slide.hidden_half = state.records_detail.hidden_half;
}

void begin_phase(FrontEndState& state, ScoresPhase phase) {
    state.records_detail.phase = phase;
    state.records_detail.phase_start = state.script_frame;
}

// The loop's screen from the next frame, its script counted from 0. The text goes to the half
// on screen through `$00A8`, the hidden half set aside.
void enter_scores_loop(FrontEndState& state, ScoresPhase phase) {
    state.records_detail.hidden_half = state.slide.hidden_half;
    state.slide.hidden_half = state.slide.shown_half;
    state.screen = FrontEndScreen::records_detail;
    state.records_detail.phase = phase;
    state.records_detail.phase_start = 0;
}

std::uint32_t phase_frame(const FrontEndState& state) {
    return state.script_frame - state.records_detail.phase_start;
}

// ---- PLAYER SCORES ----

// $83:8F2F-8F5D, $80:9AE2-9B0B: a bar's share of 128 pixels, the value's of the largest. Both
// are shifted left until one has bit 15, and the value's high byte (times 256) is divided by the
// largest's (`$80:9719`) and halved: exact below 256, not above.
std::uint8_t bar_share(std::uint16_t value, std::uint16_t largest) {
    constexpr std::uint8_t whole_bar = 0x80;
    if (value == 0) return 0;
    if (value == largest) return whole_bar;
    while (!(largest & 0x8000U) && !(value & 0x8000U)) {
        largest = static_cast<std::uint16_t>(largest << 1U);
        value = static_cast<std::uint16_t>(value << 1U);
    }
    const auto quotient = divide_word(static_cast<std::uint16_t>(value & 0xff00U),
                                      static_cast<std::uint16_t>(largest >> 8U));
    return static_cast<std::uint8_t>(quotient >> 1U);
}

// The rider's figures as the printer reads them (`$00B2-$00BA`) and the bars' shares (`$000A-$000E`),
// as `$83:8EFE` computes them.
struct PlayerFigures {
    std::uint16_t races{}, won{}, lost{}, failed{}, score{};
    std::array<std::uint8_t, 5> shares{};
};

// $83:8F72-8FDC: WON and FAILED as percentages of the races (`$83:91D0`), all of them 100 and the
// whole bar, none 0.
void part_of_races(OnePlayerRecords& records, std::uint16_t races, std::uint16_t part,
                   std::uint16_t& percent, std::uint8_t& share) {
    constexpr std::uint16_t all = 100;
    if (part == 0) return;
    if (part == races) {
        percent = all;
        share = 0x80;
        return;
    }
    percent = cartridge_percentage(records, races, part);
    share = static_cast<std::uint8_t>(divide_word(static_cast<std::uint16_t>(percent << 7U), all));
}

// $83:8EFE-9045. LOST is what WON and FAILED leave: 100 less both percentages (`$83:8FEC-8FF5`)
// and 128 less both shares (`$83:8FDE-8FE8`); with no race every figure but the score is 0. The
// largest races and score are `$83:97BD`'s, from RECORDS' entry.
PlayerFigures player_figures(OnePlayerRecords& records, unsigned rider) {
    const auto& stats = records.statistics[rider];
    std::uint16_t most_races = 0, best_score = 0;
    for (const auto& other : records.statistics) {
        most_races = std::max(most_races, other[0]);
        best_score = std::max(best_score, other[3]);
    }
    PlayerFigures figures;
    figures.races = stats[0];
    figures.score = stats[3];
    figures.shares[0] = bar_share(stats[0], most_races);
    figures.shares[4] = bar_share(stats[3], best_score);
    if (stats[0] == 0) return figures;
    part_of_races(records, stats[0], stats[1], figures.won, figures.shares[1]);
    part_of_races(records, stats[0], stats[2], figures.failed, figures.shares[3]);
    figures.shares[2] = static_cast<std::uint8_t>(0x80 - figures.shares[1] - figures.shares[3]);
    figures.lost = static_cast<std::uint16_t>(100 - figures.won - figures.failed);
    return figures;
}

TextVariables player_variables(FrontEndState& state, const PlayerFigures& figures) {
    TextVariables variables;
    variables.rider_names = state.records.rider_names;
    variables.word = [&state, figures](std::uint16_t address) -> std::uint16_t {
        switch (address) {
        case 0x00ca: return state.options_rider;
        case 0x00b2: return figures.races;
        case 0x00b4: return figures.won;
        case 0x00b6: return figures.lost;
        case 0x00b8: return figures.failed;
        case 0x00ba: return figures.score;
        default: throw std::invalid_argument("unknown PLAYER SCORES text operand");
        }
    };
    variables.place_object = [&state](unsigned object, unsigned position) {
        place_printed_object(state, object, position);
    };
    return variables;
}

// $80:DB8F-DB9E: the rider's colours and the text's upload, then `$80:C24C`.
void show_player_figures(FrontEndState& state, const FrontEndContent& content) {
    load_cgram(state, asset(content, first_rider_palette + state.options_rider), 0x80);
    load_text(state, state.slide.shown_half);
    begin_phase(state, ScoresPhase::releasing);
}

// $80:DB59-DB8C: the figures, each bar's end at 48 plus its share (`$80:DB5D-DB66`), the
// figures printed, and the decorations' step.
void print_player_figures(FrontEndState& state, const FrontEndContent& content) {
    const auto figures = player_figures(state.records, state.options_rider);
    constexpr std::uint8_t left = 48;
    for (unsigned row = 0; row < figures.shares.size(); ++row)
        state.records_detail.bar_targets[row] =
            static_cast<std::uint8_t>(left + figures.shares[row]);
    const auto variables = player_variables(state, figures);
    print_text(state.text, state.printer, content.player_scores_values, content.character_table,
               &variables);
    step_decorations(state, content);
}

// $80:DACF-DB0C: the colours for the bars, their objects, and the screen's text.
void print_player_screen(FrontEndState& state, const FrontEndContent& content) {
    state.options_rider = static_cast<std::uint8_t>(state.options_rider & last_rider);
    load_cgram(state, asset(content, bar_colours_asset), 0xe0);
    place_bar_objects(state, player_bars);
    state.text.words.fill(cleared_text);
    const auto variables = player_variables(state, PlayerFigures{});
    print_text(state.text, state.printer, content.player_scores_text, content.character_table,
               &variables);
}

// The frames after the slide: the text to the half on screen, the window and the rider's
// icon ($80:DB26-DB53); the figures ($80:DB56-DB8C); then the rider's colours and the text's
// upload, as after every change of rider ($80:DB8F-DB9E).
void player_scores_slid_in(FrontEndState& state, const FrontEndContent& content) {
    switch (phase_frame(state)) {
    case 1:
        load_text(state, state.slide.shown_half);
        open_bar_window(state, player_bars);
        high_bits(state, 104) = 0x54;
        step_decorations(state, content);
        return;
    case 2:
        copy_oam(state);
        print_player_figures(state, content);
        return;
    default: show_player_figures(state, content); return;
    }
}

// $80:DBE7-DC12: Up first (the next rider), then Down or Select (the one before), each with the
// result's sound; then the buttons that leave (`$80:B6F8`: B, Y, Select, Start, Right, A, X, L
// and R). No latch: the figures wait for both pads' release (`$80:C24C`).
void player_scores_running(FrontEndState& state, const FrontEndContent& content,
                           std::uint16_t pad) {
    constexpr std::uint16_t up = pad_up, down = pad_down | pad_select, leave = 0xf1f0;
    if (pad & (up | down)) {
        play_menu_sound(state, MenuSound::result); // $80:DC05, $80:DBF8
        auto& rider = state.options_rider;
        rider = static_cast<std::uint8_t>((pad & up) ? (rider + 1U) & last_rider
                                                     : (rider + last_rider) & last_rider);
        print_player_figures(state, content);
        begin_phase(state, ScoresPhase::reprinted);
        return;
    }
    if (pad & leave) {
        restore_hidden_half(state);
        state.records_detail.press_seen = false;
        begin_phase(state, ScoresPhase::pressing);
        return;
    }
    for (unsigned row = 0; row < player_bars.rows; ++row)
        step_bar(state, content, player_bars, row);
    step_decorations(state, content);
}

// ---- GROUP TABLES ----

// $80:EF89-EFD3: the league's eight rows (index, score), sorted, and the rider each row's member
// is (16 for none; its members in rider order, `$83:96BE`).
struct GroupStanding {
    std::array<std::uint8_t, 8> order{}, riders{};
    std::array<std::uint16_t, 8> scores{};
};

// $80:EFD4-F024: a selection sort, highest score first. Each pass takes the last row of the
// lowest score among rows 0 to n (from 0x7FFF, so a tie goes to the later row) and swaps it with
// row n; tied rows therefore do not keep their members' order.
void sort_group_rows(GroupStanding& standing) {
    for (auto last = static_cast<unsigned>(standing.scores.size()); last-- > 0;) {
        std::uint16_t lowest = 0x7fff;
        unsigned at = 0;
        for (unsigned row = 0; row <= last; ++row)
            if (standing.scores[row] <= lowest) {
                lowest = standing.scores[row];
                at = row;
            }
        std::swap(standing.scores[at], standing.scores[last]);
        std::swap(standing.order[at], standing.order[last]);
    }
}

GroupStanding group_standing(const FrontEndState& state) {
    const auto slot = state.league.slot;
    GroupStanding standing;
    for (unsigned row = 0; row < standing.order.size(); ++row) {
        standing.order[row] = static_cast<std::uint8_t>(row);
        standing.scores[row] = state.records.league_scores[slot][row];
    }
    sort_group_rows(standing);
    std::array<std::uint8_t, 8> members{};
    members.fill(someone);
    unsigned count = 0;
    for (unsigned rider = 0; rider <= last_rider && count < members.size(); ++rider)
        if (state.records.league_members[slot] & (0x8000U >> rider))
            members[count++] = static_cast<std::uint8_t>(rider);
    for (unsigned row = 0; row < standing.riders.size(); ++row)
        standing.riders[row] = members[standing.order[row]];
    return standing;
}

// $80:DE38-DE96: the league's name line and highest score (`$80:E184`), the rows' dashes
// (`$80:E12B`) and each member's name (`$80:E1B1`), and the bars' targets: 88 plus 128 pixels
// times the row's share of the highest score. Nothing is cleared first.
void print_group_table(FrontEndState& state, const FrontEndContent& content) {
    const auto standing = group_standing(state);
    const auto top_score = standing.scores[0];
    constexpr std::uint8_t left = 88; // $80:9B20-9B23
    for (unsigned row = 0; row < standing.scores.size(); ++row)
        state.records_detail.bar_targets[row] =
            static_cast<std::uint8_t>(left + bar_share(standing.scores[row], top_score));
    // `$00B2`: the highest score for the name line ($80:DE3F-DE42), then each row's rider
    // ($80:DE7E-DE80).
    std::uint16_t shown_value = top_score;
    TextVariables variables;
    variables.rider_names = state.records.rider_names;
    variables.league_names = state.records.league_names;
    variables.word = [&](std::uint16_t address) -> std::uint16_t {
        if (address == 0x00cc) return state.league.slot;
        if (address == 0x00b2) return shown_value;
        throw std::invalid_argument("unknown GROUP TABLES text operand");
    };
    namespace code = print_code;
    constexpr unsigned name_line_blanks = 30;
    constexpr std::uint8_t league_word = 0xcc, value_word = 0xb2, name_row = 4, score_column = 24,
                           score_row = 7;
    std::vector<std::uint8_t> name_line{code::place, 2, name_row};
    name_line.insert(name_line.end(), name_line_blanks, '_');
    name_line.insert(name_line.end(),
                     {code::league_name, league_word, 0, code::centre, name_row, code::place,
                      score_column, score_row, code::number, value_word, 0, code::end});
    print_text(state.text, state.printer, name_line, content.character_table, &variables);
    print_text(state.text, state.printer, content.group_scores_empty, content.character_table);
    std::uint8_t text_row = 9;
    for (const auto rider : standing.riders) {
        if (rider >= someone) continue;
        shown_value = rider;
        const std::array<std::uint8_t, 9> name{
            code::attribute,  7,          code::place, 2,        text_row,
            code::rider_name, value_word, 0,           code::end};
        print_text(state.text, state.printer, name, content.character_table, &variables);
        text_row = static_cast<std::uint8_t>(text_row + 2);
    }
}

// $80:DDFC-DE35: the colours, no latches, the objects, no colour math, the arrow away, the
// screen's text and the table. Its printing runs past the next frame's start (R-0097).
void print_group_screen(FrontEndState& state, const FrontEndContent& content) {
    load_cgram(state, asset(content, bar_colours_asset), 0xe0);
    state.latches = {};
    place_bar_objects(state, group_bars);
    state.registers.colour_math = 0;
    send_arrow_off(state);
    state.text.words.fill(cleared_text);
    print_text(state.text, state.printer, content.group_scores_text, content.character_table);
    print_group_table(state, content);
}

// $80:DF37-DF98, on the first row's pass: Up (the next league, latched in `$008F` bit 3), then
// Down or Select (the one before, bit 2), each with the result's sound and the table printed
// again; then Y or X or a choice leaves. Returns true when the loop's pass ends there.
bool group_tables_input(FrontEndState& state, const FrontEndContent& content, std::uint16_t pad) {
    auto& latches = state.latches;
    auto& slot = state.league.slot;
    const auto reprint = [&](std::uint8_t next) {
        play_menu_sound(state, MenuSound::result); // $80:DF46, $80:DF6C
        slot = next;
        print_group_table(state, content);
        state.records_detail.group_reprinted = true;
        return true;
    };
    if (!(pad & pad_up))
        latches.up = false;
    else if (!latches.up) {
        latches.up = true;
        return reprint(static_cast<std::uint8_t>(slot + 1U == league_slots ? 0U : slot + 1U));
    }
    if (!(pad & (pad_down | pad_select)))
        latches.down = false;
    else if (!latches.down) {
        latches.down = true;
        return reprint(static_cast<std::uint8_t>(slot == 0 ? league_slots - 1U : slot - 1U));
    }
    if (!(pad & (back_buttons | choose_buttons))) return false;
    // $80:DF99-DFA6: leaving, with the back slide.
    state.records.group_table_shown = false;
    restore_hidden_half(state);
    start_slide(state, content, true);
    slide_bars_out(state, group_bars);
    begin_phase(state, ScoresPhase::leaving);
    return true;
}

// $80:DEDB-DF96: a pass of the bars, the pad tests on the first row's.
void group_tables_running(FrontEndState& state, const FrontEndContent& content, std::uint16_t pad) {
    copy_oam(state);
    load_text(state, state.slide.shown_half);
    step_bar(state, content, group_bars, 0);
    if (group_tables_input(state, content, pad)) return;
    for (unsigned row = 1; row < group_bars.rows; ++row) step_bar(state, content, group_bars, row);
}

// ---- the way back to RECORDS ----

// $80:D503 and the window closed ($80:DC27-DC3C, $80:DFAC-DFBE), the logo down (`$80:F52B`), and
// the RECORDS loop again from the back slide's last frame (`$80:D53F`), without a new print.
void close_scores(FrontEndState& state, const FrontEndContent& content) {
    auto& registers = state.registers;
    registers.object_window_one_inverted = false;
    registers.object_window_left = registers.object_window_right = 0;
    state.records_detail.window_left = state.records_detail.window_right = 0;
    for (unsigned entry = first_bar_object; entry < first_bar_object + group_bars.rows; ++entry)
        for (unsigned field = 0; field < 4; ++field) oam_byte(state, entry, field) = 1;
    high_bits(state, 4) = high_bits(state, 8) = four_hidden;
    registers.colour_math = 0x7f;
    state.logo.raised = false;
    reopen_records_menu(state, content);
}

// A back slide's pass: the window's registers from the last pass's `$007A`/`$007B`
// ($80:E2A2-E2A9), then the objects and the window move on.
void leave_scores_pass(FrontEndState& state, const FrontEndContent& content,
                       const BarLayout& layout) {
    const bool done = slide_frame(state, content);
    state.registers.object_window_left = state.records_detail.window_left;
    state.registers.object_window_right = state.records_detail.window_right;
    if (!done) {
        slide_bars_out(state, layout);
        return;
    }
    close_scores(state, content);
}

// PLAYER SCORES' way out after `$80:C206`: a frame wait, then `$80:A8A8` (an OAM copy and the
// base colours), `$80:D420` (the printed objects hidden) and the back slide ($80:DC1B-DC24).
void player_scores_leaving(FrontEndState& state, const FrontEndContent& content) {
    const auto frame = phase_frame(state);
    if (frame == 1) return;
    if (frame == 2) {
        copy_oam(state);
        load_cgram(state, content.base_palette, 0);
        for (unsigned entry = 104; entry < 112; ++entry)
            oam_byte(state, entry, 0) = oam_byte(state, entry, 1) = 1;
        high_bits(state, 104) = high_bits(state, 108) = four_hidden;
        start_slide(state, content, true);
        slide_bars_out(state, player_bars);
        return;
    }
    leave_scores_pass(state, content, player_bars);
}

} // namespace

bool shows_scores(const FrontEndState& state) {
    return state.records_detail.category == player_scores_category
        || state.records_detail.category == group_tables_category;
}

bool scores_frame_waits(const FrontEndState& state, std::uint32_t next) {
    // GROUP TABLES' first print ends in the frame after its own, which has no frame wait.
    return !(state.records_detail.category == group_tables_category && next == 2);
}

void scores_entry_frame(FrontEndState& state, const FrontEndContent& content) {
    const bool player = state.records_detail.category == player_scores_category;
    const auto& layout = bar_layout(state);
    // PLAYER SCORES prints in its first frame and slides from its second; GROUP TABLES' print
    // takes two and its slide starts in the third ($80:DE96-DEA6), after the text's upload.
    const std::uint32_t slide_start = player ? 2 : 3;
    if (state.script_frame == 1) {
        if (player)
            print_player_screen(state, content);
        else
            print_group_screen(state, content);
        return;
    }
    if (state.script_frame < slide_start) return;
    if (state.script_frame == slide_start) {
        load_text(state, state.slide.hidden_half);
        if (player) send_arrow_off(state); // $80:DB12
        start_slide(state, content, false);
        slide_bars_in(state, layout);
        return;
    }
    if (!slide_frame(state, content)) {
        slide_bars_in(state, layout);
        return;
    }
    if (player) {
        enter_scores_loop(state, ScoresPhase::slid_in);
        return;
    }
    // $80:DEA9-DED9: the table marked shown, then the loop.
    state.records.group_table_shown = true;
    open_bar_window(state, group_bars);
    enter_scores_loop(state, ScoresPhase::running);
}

void scores_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    auto& detail = state.records_detail;
    const auto pad = gated_pads(state, pads);
    if (detail.category == group_tables_category) {
        if (detail.phase == ScoresPhase::leaving) {
            leave_scores_pass(state, content, group_bars);
            return;
        }
        if (detail.group_reprinted) { // $80:DE9B, then the loop's frame wait
            detail.group_reprinted = false;
            load_text(state, state.slide.shown_half);
            return;
        }
        group_tables_running(state, content, pad);
        return;
    }
    switch (detail.phase) {
    case ScoresPhase::slid_in: player_scores_slid_in(state, content); return;
    case ScoresPhase::reprinted: show_player_figures(state, content); return;
    case ScoresPhase::releasing: // `$80:C24C`, then the loop's first pass
        copy_oam(state);
        step_decorations(state, content);
        if ((pads.one | pads.two) != 0) return;
        begin_phase(state, ScoresPhase::running);
        for (unsigned row = 0; row < player_bars.rows; ++row)
            step_bar(state, content, player_bars, row);
        step_decorations(state, content);
        return;
    case ScoresPhase::running:
        load_text(state, state.slide.shown_half); // $80:DBD9
        copy_oam(state);
        player_scores_running(state, content, pad);
        return;
    case ScoresPhase::pressing:
        if (press_seen_twice(state, content, pads)) begin_phase(state, ScoresPhase::leaving);
        return;
    case ScoresPhase::leaving: player_scores_leaving(state, content); return;
    case ScoresPhase::entering: break;
    }
}

} // namespace unirally::front_end_screens
