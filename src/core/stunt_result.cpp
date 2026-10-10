// A stunt event's result screen (R-0067, R-0095): `$80:951C`'s type-2 builder `$80:F0EE-F2E9`,
// the trick tally `$80:F669-F813` and the rider's best score; then against the computer the
// qualifying score, or with a second human (2P, VS, a league's pair) rider 1's own tally and best
// below the player's; then the one-run result's waits for a press. The race's return before it
// and the way out after it are the one-run result's (race_result.cpp, R-0057); the records and
// scoring on the way out are the stunt event's (`$80:C948`, `$83:88E1`).
#include "front_end_screens.hpp"
#include "text_printer.hpp"

#include <array>
#include <optional>
#include <stdexcept>

namespace unirally::front_end_screens {

namespace {

// The result's frames from its first (`$83:94D0`'s wait): the objects and the streams, whose
// printing in one-player play runs past the frame's end (R-0067), so the text's upload and the
// decorations come in the third frame without a frame wait (with two riders in the second,
// R-0095); then `$80:9869`'s fade, brightness 2, 4, ..., 14, and the tally's first pass in the
// fade's last frame.
constexpr std::uint32_t build_frame = 1, print_frame = 2, fade_frames = 7;
std::uint32_t first_fade_frame(const FrontEndState& state) {
    return stunt_text_runs_over(state) ? stunt_text_frame + 1 : print_frame + 1;
}
std::uint32_t tally_start_frame(const FrontEndState& state) {
    return first_fade_frame(state) + fade_frames - 1;
}

// The streams of front-end.stunt-result-text, in ROM order.
enum class StuntText : unsigned {
    table,             // $80:F2EA: the title, the columns x1-x4, 1p 2p, the rows and their zeros
    one_player_dashes, // $80:F3E0: dashes over every 2P cell
    total,             // $80:F44F: the running total at row 22
    second_total,      // $80:F456: rider 1's running total at row 24
    qualify,           // $80:F45D: `qualify   :` and the qualifying score at row 24
    record_line,       // $80:F471: the record holder's name and record at row 20
    rider_line,        // $80:F47F: the rider's line at row 22 in palette 0, two riders
    one_player_rider,  // $80:F491: the rider's line at row 22 in palette 7, one player
    second_rider_line, // $80:F4A3: rider 1's line at row 24 in palette 1
};

// The direct-page words the streams print (`F7`, `F8`, `FD`, `F0`).
constexpr std::uint16_t track_word = 0xce, rider_word = 0x17d, second_rider_word = 0x17f,
                        holder_word = 0xbe, record_or_total_word = 0xc0, first_shown_word = 0xb2,
                        qualifying_word = first_shown_word;

// The tally's first cell, row 9 column 7 (`$80:F68F-F6A6`): six columns a tally column, two text
// rows a trick row; rider 1's cells three columns right of the player's (`$80:F69D-F6A3`).
constexpr unsigned first_cell = 9 * 32 + 7, cell_columns = 6, cell_rows = 64,
                   second_rider_cells = 3;
// A pass's wait and a column total's (`$80:F775`, `$80:F7B9`: Y = 6 and 12, one frame more).
constexpr std::uint8_t pass_wait_frames = 7, column_wait_frames = 13;
// $80:F4B5, after rider 1's lines: the printer's palette 7 again.
constexpr std::uint16_t menu_text_attribute = 7U << 10U;

// The objects `$80:F116-F19C` lays out: the markers 96-99 beside the riders' scores, the 1P and
// 2P marks 100-103 beside their lines, the icons 104-106 beside the record, entry 112.
constexpr std::uint8_t marker_column = 0xc4, marker_attributes = 0x17, icon_column = 0x7f;
constexpr std::array<std::uint8_t, 4> marker_lines{0xb0, 0xc0, 0xb0, 0xc0};
constexpr std::array<std::uint8_t, 4> mark_lines{0xaf, 0xc0, 0xaf, 0xc0};
constexpr std::array<std::uint8_t, 4> mark_attributes{0x11, 0x13, 0x11, 0x13};
constexpr std::array<std::uint8_t, 3> icon_lines{0xaf, 0xbf, 0x9f};
constexpr std::uint8_t record_icon_line = 0xa1;
// The high table: 104 and 106 shown, 105 and 107 hidden (0x44); 112 shown, 113-115 hidden
// (0x54). `$80:F209` shows the 1P mark, 100 and 102 (`AND #$CC`); `$80:F23F` the player's
// new-best markers 96 and 98 (`AND #$66`). Rider 1's tally (`$80:F289-F293`) writes 0x40 for
// 104-107 and shows its 2P mark, 101 and 103 (`AND #$33`); its new best shows 97 and 99
// (`$80:F2C4-F2C9`, `AND #$33`).
constexpr std::uint8_t record_icons_high = 0x44, record_icon_high = 0x54, one_player_marks = 0xcc,
                       stunt_best_markers = 0x66, second_rider_icons_high = 0x40,
                       second_rider_marks = 0x33, second_best_markers = 0x33;
// Colours: asset 0x22 at 0xB0 (`$80:F0F3`), the heads (asset 0x26 + rider) at 0 and 0x10, and the
// record holder's colours at 0xA0.
constexpr unsigned marker_palette_asset = 0x22, marker_palette_colour = 0xb0,
                   first_head_asset = 0x26, rider_head_colour = 0x00, opponent_head_colour = 0x10,
                   holder_palette_colour = 0xa0;

std::uint8_t track_of(const FrontEndState& state) {
    return state.tour_menu.track;
}

// The `n`th stream of `table` with its 0xFF, as the printer takes it.
std::span<const std::uint8_t> stream_of(std::span<const std::uint8_t> table, unsigned n) {
    const auto text = nth_string(table, n);
    return {text.data(), text.size() + 1};
}

// The words the result's streams read, as the result sets them before each print. `$00B2` is both
// the first row's shown count, during the tally, and the qualifying score, after it (`$80:F25D`).
struct ResultWords {
    std::uint16_t holder{}, record_or_total{};
    std::array<std::uint16_t, trick_family::count> shown{};
    std::optional<std::uint16_t> qualifying_score; // printed in place of the first row's count
};

void print_stunt(FrontEndState& state, const FrontEndContent& content,
                 std::span<const std::uint8_t> stream, const ResultWords& words) {
    TextVariables variables;
    variables.word = [&](std::uint16_t address) -> std::uint16_t {
        if (address == track_word) return track_of(state);
        if (address == rider_word) return state.rider_menu.rider;
        if (address == second_rider_word) return state.now_playing.opponent;
        if (address == holder_word) return words.holder;
        if (address == record_or_total_word) return words.record_or_total;
        if (address == qualifying_word && words.qualifying_score) return *words.qualifying_score;
        if (address >= first_shown_word) {
            const auto row = static_cast<unsigned>(address - first_shown_word) / 2U;
            if (row < words.shown.size()) return words.shown[row];
        }
        throw std::logic_error("the stunt result prints no such word");
    };
    variables.track_names = content.track_names;
    variables.rider_names = state.records.rider_names;
    print_text(state.text, state.printer, stream, content.character_table, &variables);
}

void print_stunt(FrontEndState& state, const FrontEndContent& content, StuntText text,
                 const ResultWords& words) {
    print_stunt(state, content, stream_of(content.stunt_result_text, static_cast<unsigned>(text)),
                words);
}

bool against_computer(const FrontEndState& state) { // $80:F24F: `$77:0749` from 0x10
    return state.now_playing.opponent >= someone;
}

// $80:B6D3 in the tally: pad 1's twelve buttons, and pad 2's while `$77:0742` bit 10 is clear,
// which it is outside one-player play.
bool tally_press(const FrontEndState& state, FrontEndPads pads) {
    return any_button_pressed(pads, !state.one_player);
}

// $80:F0EE-F0F9 after the one-run result's common part (`$80:951C-955A`).
void start_stunt_result(FrontEndState& state, const FrontEndContent& content) {
    start_result_screen(state, content);
    load_cgram(state, asset(content, marker_palette_asset), marker_palette_colour);
}

// $80:F0FC-F19C: the heads' colours and the result's objects.
void lay_out_stunt_objects(FrontEndState& state, const FrontEndContent& content) {
    load_cgram(state, asset(content, first_head_asset + state.rider_menu.rider), rider_head_colour);
    load_cgram(state, asset(content, first_head_asset + state.now_playing.opponent),
               opponent_head_colour);
    for (unsigned k = 0; k < 4; ++k) {
        const unsigned marker = 96 + k, mark = 100 + k;
        oam_byte(state, marker, 0) = marker_column;
        oam_byte(state, marker, 1) = marker_lines[k];
        oam_byte(state, marker, 3) = marker_attributes;
        oam_byte(state, mark, 1) = mark_lines[k];
        oam_byte(state, mark, 3) = mark_attributes[k];
    }
    high_bits(state, 96) = four_hidden;
    high_bits(state, 100) = four_hidden;
    for (unsigned k = 0; k < icon_lines.size(); ++k) {
        oam_byte(state, 104 + k, 0) = icon_column;
        oam_byte(state, 104 + k, 1) = icon_lines[k];
    }
    high_bits(state, 104) = record_icons_high;
    oam_byte(state, 112, 0) = marker_column;
    oam_byte(state, 112, 1) = record_icon_line;
    oam_byte(state, 112, 3) = marker_attributes;
    high_bits(state, 112) = record_icon_high;
}

// $80:F19F-F1F1: the record holder's colours, the logo held up (`$80:F53F`), and the streams: the
// table, the record line, then in one-player play (`$77:10AD` = 1) the 1P dashes and the rider's
// line in palette 7, otherwise the rider's line in palette 0 (`$80:F1D8-F1F1`).
void print_stunt_screen(FrontEndState& state, const FrontEndContent& content) {
    lay_out_stunt_objects(state, content);
    const auto track = track_of(state);
    ResultWords words;
    words.holder = state.records.record_holders[0][track];
    words.record_or_total = state.records.record_times[0][track];
    load_cgram(state, asset(content, first_rider_palette + words.holder), holder_palette_colour);
    raise_logo(state);
    print_stunt(state, content, StuntText::table, words);
    print_stunt(state, content, StuntText::record_line, words);
    if (state.one_player) {
        print_stunt(state, content, StuntText::one_player_dashes, words);
        print_stunt(state, content, StuntText::one_player_rider, words);
    } else {
        print_stunt(state, content, StuntText::rider_line, words);
    }
}

// $80:F1F4-F202, in the frame the printing ran into: the text shown, the decorations restarted.
void show_stunt_text(FrontEndState& state, const FrontEndContent& content) {
    load_text(state, state.slide.shown_half);
    state.decorations.delay = state.decorations.wave_delay = state.decorations.sway = 0;
    step_decorations(state, content);
}

// The tallies the current tally counts up: the player's (`$77:076B`) or rider 1's (`$77:07D5`).
const StuntTallies& tallies_of(const FrontEndState& state) {
    const auto& times = state.race_result.times;
    return state.race_result.tally.rider == 0 ? times.player_tallies : times.opponent_tallies;
}

// $80:F680-F751 and `$80:F77F-F7A2`: a pass over the column's five rows. A row below its tally
// shows one more; a row still at 0 prints its 0 again. A pass that raised a row plays the move's
// sound unless a button is pressed (`$80:F758-F75D`, `$80:B6D3`). When no row rose the column's
// points go into the total, which is printed (`$80:F44F` or rider 1's `$80:F456`, `$0060`) with
// the choice's sound (`$80:F7A7`), and the wait is the column total's. The cells and totals take
// the printer's palette as the last line printed left it.
void tally_pass(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    auto& tally = state.race_result.tally;
    const auto& tallies = tallies_of(state);
    const unsigned first = first_cell + (tally.rider ? second_rider_cells : 0U);
    bool raised = false;
    ResultWords words;
    for (unsigned row = 0; row < trick_family::count; ++row) {
        auto& shown = tally.shown[row];
        const bool rises = shown < tallies[row][tally.column].shown;
        if (rises) {
            ++shown;
            raised = true;
        }
        if (!rises && shown != 0) continue;
        words.shown = tally.shown;
        state.printer.position = first + cell_columns * tally.column + cell_rows * row;
        print_stunt(state, content, stream_of(content.stunt_tally_cells, row), words);
    }
    tally.frames_waited = 0;
    tally.column_total_shown = !raised;
    if (raised) {
        if (!tally_press(state, pads)) play_menu_sound(state, MenuSound::navigate);
        return;
    }
    for (unsigned row = 0; row < trick_family::count; ++row)
        tally.total = static_cast<std::uint16_t>(tally.total + tallies[row][tally.column].points);
    words.record_or_total = tally.total;
    print_stunt(state, content, tally.rider ? StuntText::second_total : StuntText::total, words);
    play_menu_sound(state, MenuSound::select);
}

// A frame of the tally after a pass: the first shows the text and steps the decorations
// (`$80:F765-F76C`, `$80:F7AD-F7B0`); each copies the OAM and reads the pads (`$80:D1EC`) and then
// either waits on, stepping the decorations again, or ends the wait with the next pass: after its
// frames, or at once on a press (`$80:F7FB`, `$80:B6D3`). The last column's end leaves the tally
// (`$80:F7C4-F7CB`).
void tally_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    auto& tally = state.race_result.tally;
    ++tally.frames_waited;
    if (tally.frames_waited == 1) {
        load_text(state, state.slide.shown_half);
        step_decorations(state, content);
    }
    copy_oam(state);
    const auto wait = tally.column_total_shown ? column_wait_frames : pass_wait_frames;
    if (tally.frames_waited <= wait && !tally_press(state, pads)) { // $80:F7FB
        step_decorations(state, content);
        return;
    }
    if (tally.column_total_shown) {
        ++tally.column;
        tally.shown.fill(0);
        if (tally.column == trick_family::columns) {
            tally.finished_frame = state.script_frame;
            return;
        }
    }
    tally_pass(state, content, pads);
}

// $80:F7CD-F7D7 at a tally's end, a frame after its last wait: the text, the decorations and the
// pads (`$80:D1EC`, kept for rider 1's first pass); then the total printed again and the rider's
// best score `$77:0829 + 2 x (50 x rider + track)` (`$83:9E47`), kept when the score is higher,
// unsigned, with its new-best markers shown: the player's (`$80:F21C-F244`) or rider 1's
// (`$80:F2A1-F2C9`, then `$80:F4B5`'s palette 7).
void end_tally(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    auto& result = state.race_result;
    load_text(state, state.slide.shown_half);
    step_decorations(state, content);
    copy_oam(state);
    result.tally.pads = pads.one;
    result.tally.second_pads = pads.two;
    const bool second = result.tally.rider != 0;
    ResultWords words;
    words.record_or_total = result.tally.total;
    print_stunt(state, content, second ? StuntText::second_total : StuntText::total, words);
    const auto rider = second ? state.now_playing.opponent : state.rider_menu.rider;
    const auto score = second ? result.times.opponent_score : result.times.player_score;
    auto& best = personal_best(state.records, rider, track_of(state));
    if (score > best) {
        best = score;
        high_bits(state, 96) &= second ? second_best_markers : stunt_best_markers;
    }
    if (second) state.printer.attribute = menu_text_attribute;
}

// $80:F283-F29E: rider 1's line in palette 1, its icons and 2P mark, and its tally from x1 with
// its first pass, which reads the pads the player's tally's end read.
void start_second_tally(FrontEndState& state, const FrontEndContent& content) {
    print_stunt(state, content, StuntText::second_rider_line, {});
    high_bits(state, 104) = second_rider_icons_high;
    high_bits(state, 100) &= second_rider_marks;
    const FrontEndPads pads{state.race_result.tally.pads, state.race_result.tally.second_pads};
    state.race_result.tally = {};
    state.race_result.tally.rider = 1;
    tally_pass(state, content, pads);
}

// The frames after a tally (see end_tally). After the player's, a frame wait (`$80:F249`) and the
// text; then against the computer (`$77:0749` from 0x10) the qualifying score (`$80:F24F-F272`,
// not in a league, `$77:10AD` = 4), a frame wait and the text, and `$80:F88D`'s score history
// and checksums, which native does not keep (R-0067); against a second human its tally
// (`$80:F283`). After rider 1's, a frame wait (`$80:F2D4`), the text and `$80:F88D`. Then the
// one-run result's waits (`$80:C24C`, `$80:C206`).
void after_tally_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    auto& result = state.race_result;
    const auto after = state.script_frame - result.tally.finished_frame;
    if (after == 1) {
        end_tally(state, content, pads);
        return;
    }
    const bool second = result.tally.rider != 0;
    const bool qualifying_line = !second && state.mode != FrontEndMode::league;
    if (after == 2) {
        load_text(state, state.slide.shown_half);
        if (!second && !against_computer(state)) {
            start_second_tally(state, content);
        } else if (qualifying_line) {
            ResultWords words;
            words.qualifying_score = tour_qualifying_score(state, content); // $80:F259, `$83:9EEB`
            print_stunt(state, content, StuntText::qualify, words);
        }
        return;
    }
    if (after == 3 && qualifying_line) { // $80:F26F-F272
        load_text(state, state.slide.shown_half);
        return;
    }
    copy_oam(state);
    wait_for_result_press(state, content, pads);
}

} // namespace

void stunt_result_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    const auto frame = state.script_frame;
    if (frame == build_frame) {
        start_stunt_result(state, content);
        return;
    }
    if (frame == print_frame) {
        print_stunt_screen(state, content);
        if (!stunt_text_runs_over(state)) show_stunt_text(state, content);
        return;
    }
    if (frame == stunt_text_frame && stunt_text_runs_over(state)) {
        show_stunt_text(state, content);
        return;
    }
    if (frame <= tally_start_frame(state)) {
        copy_oam(state);
        state.registers.brightness =
            static_cast<std::uint8_t>(2 * (frame - first_fade_frame(state) + 1));
        state.registers.force_blank = false;
        if (frame < tally_start_frame(state)) return;
        high_bits(state, 100) &= one_player_marks; // $80:F209
        state.race_result.tally = {};
        // The pass reads the pads the menus' work RAM brought back from NOW PLAYING's choice.
        tally_pass(state, content, {state.now_playing.choice_pads, 0});
        return;
    }
    if (state.race_result.tally.finished_frame == 0) {
        tally_frame(state, content, pads);
        return;
    }
    after_tally_frame(state, content, pads);
}

} // namespace unirally::front_end_screens
