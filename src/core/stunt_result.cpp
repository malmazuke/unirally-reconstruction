// A stunt event's result screen (R-0067): `$80:951C`'s type-2 builder `$80:F0EE-F2E9`, the trick
// tally `$80:F669-F813`, the rider's best score, then the one-run result's waits for a press. The
// race's return before it and the way out after it are the one-run result's (race_result.cpp,
// R-0057); the records and scoring on the way out are the stunt event's (`$80:C948`, `$83:88E1`).
#include "front_end_screens.hpp"
#include "text_printer.hpp"

#include <array>
#include <optional>
#include <stdexcept>

namespace unirally::front_end_screens {

namespace {

// The result's frames from its first (`$83:94D0`'s wait): the objects and the streams, whose
// printing runs past the frame's end (R-0067), so the text's upload and the decorations come in
// the third frame without a frame wait; then `$80:9869`'s fade, brightness 2, 4, ..., 14, and the
// tally's first pass in the fade's last frame.
constexpr std::uint32_t build_frame = 1, print_frame = 2, first_fade_frame = 4, fade_frames = 7;
constexpr std::uint32_t tally_start_frame = first_fade_frame + fade_frames - 1;

// The streams of front-end.stunt-result-text, in ROM order.
enum class StuntText : unsigned {
    table,             // $80:F2EA: the title, the columns x1-x4, 1p 2p, the rows and their zeros
    one_player_dashes, // $80:F3E0: dashes over every 2P cell
    total,             // $80:F44F: the running total at row 22
    second_total,      // $80:F456: the 2P rider's total at row 24 (not recovered)
    qualify,           // $80:F45D: `qualify   :` and the qualifying score at row 24
    record_line,       // $80:F471: the record holder's name and record at row 20
    rider_line,        // $80:F47F: the rider's line at row 22, two players
    one_player_rider,  // $80:F491: the rider's line at row 22 in palette 7, one player
    second_rider_line, // $80:F4A3: the 2P rider's line at row 24 (not recovered)
};

// The direct-page words the streams print (`F7`, `F8`, `FD`, `F0`).
constexpr std::uint16_t track_word = 0xce, rider_word = 0x17d, holder_word = 0xbe,
                        record_or_total_word = 0xc0, first_shown_word = 0xb2,
                        qualifying_word = first_shown_word;

// The tally's first cell, row 9 column 7 (`$80:F68F-F6A6`): six columns a tally column, two text
// rows a trick row.
constexpr unsigned first_cell = 9 * 32 + 7, cell_columns = 6, cell_rows = 64;
// A pass's wait and a column total's (`$80:F775`, `$80:F7B9`: Y = 6 and 12, one frame more).
constexpr std::uint8_t pass_wait_frames = 7, column_wait_frames = 13;

// The objects `$80:F116-F19C` lays out: the markers 96-99 beside the rider's score, the 1P and 2P
// marks 100-103 beside its line, the icons 104-106 beside the record, entry 112.
constexpr std::uint8_t marker_column = 0xc4, marker_attributes = 0x17, icon_column = 0x7f;
constexpr std::array<std::uint8_t, 4> marker_lines{0xb0, 0xc0, 0xb0, 0xc0};
constexpr std::array<std::uint8_t, 4> mark_lines{0xaf, 0xc0, 0xaf, 0xc0};
constexpr std::array<std::uint8_t, 4> mark_attributes{0x11, 0x13, 0x11, 0x13};
constexpr std::array<std::uint8_t, 3> icon_lines{0xaf, 0xbf, 0x9f};
constexpr std::uint8_t record_icon_line = 0xa1;
// The high table: 104 and 106 shown, 105 and 107 hidden (0x44); 112 shown, 113-115 hidden
// (0x54). `$80:F209` shows the 1P mark, 100 and 102 (0xCC); `$80:F23F` the new-best markers 96
// and 98 (0x66).
constexpr std::uint8_t record_icons_high = 0x44, record_icon_high = 0x54, one_player_marks = 0xcc,
                       stunt_best_markers = 0x66;
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
        if (address == 0x017f) return state.now_playing.opponent;
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

// $80:F0EE-F0F9 after the one-run result's common part (`$80:951C-955A`).
void start_stunt_result(FrontEndState& state, const FrontEndContent& content) {
    if (state.mode != FrontEndMode::league
        && (!state.one_player || state.now_playing.opponent < someone))
        throw std::logic_error("a stunt result for two players ($80:F283) is not recovered");
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
    high_bits(state, 104) =
        state.now_playing.opponent < someone ? hidden_bit(107) : record_icons_high;
    oam_byte(state, 112, 0) = marker_column;
    oam_byte(state, 112, 1) = record_icon_line;
    oam_byte(state, 112, 3) = marker_attributes;
    high_bits(state, 112) = record_icon_high;
}

// $80:F19F-F1F1: the record holder's colours, the logo held up (`$80:F53F`), and the streams: the
// table, the record line, the 1P dashes and the rider's line.
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
    if (state.now_playing.opponent < someone) {
        print_stunt(state, content, StuntText::rider_line, words);
        print_stunt(state, content, StuntText::second_rider_line, words);
    } else {
        print_stunt(state, content, StuntText::one_player_dashes, words);
        print_stunt(state, content, StuntText::one_player_rider, words);
    }
}

// $80:F1F4-F202, in the frame the printing ran into: the text shown, the decorations restarted.
void show_stunt_text(FrontEndState& state, const FrontEndContent& content) {
    load_text(state, state.slide.shown_half);
    state.decorations.delay = state.decorations.wave_delay = state.decorations.sway = 0;
    step_decorations(state, content);
}

const TrickTally& tally_of(const FrontEndState& state, unsigned row, unsigned column) {
    return state.race_result.times.player_tallies[row][column];
}

// $80:F669-F813: the second human's blue cells share the player's pass and wait.
bool second_tally_pass(FrontEndState& state, const FrontEndContent& content) {
    if (state.now_playing.opponent >= someone) return false;
    auto& tally = state.race_result.tally;
    bool raised = false;
    ResultWords words;
    for (unsigned row = 0; row < trick_family::count; ++row) {
        auto& shown = tally.second_shown[row];
        const bool rises =
            shown < state.race_result.times.opponent_tallies[row][tally.column].shown;
        if (rises) {
            ++shown;
            raised = true;
        }
        if (!rises && shown != 0) continue;
        words.shown = tally.second_shown;
        state.printer.position = first_cell + 3 + cell_columns * tally.column + cell_rows * row;
        state.printer.attribute = 0x0400;
        print_stunt(state, content, stream_of(content.stunt_tally_cells, row), words);
    }
    return raised;
}

// $80:F680-F751 and `$80:F77F-F7A2`: a pass over the column's five rows. A row below its tally
// shows one more; a row still at 0 prints its 0 again. When no row rose the column's points go
// into the total, which is printed, and the wait is the column total's.
void tally_pass(FrontEndState& state, const FrontEndContent& content) {
    auto& tally = state.race_result.tally;
    bool raised = false;
    ResultWords words;
    for (unsigned row = 0; row < trick_family::count; ++row) {
        auto& shown = tally.shown[row];
        const bool rises = shown < tally_of(state, row, tally.column).shown;
        if (rises) {
            ++shown;
            raised = true;
        }
        if (!rises && shown != 0) continue;
        words.shown = tally.shown;
        state.printer.position = first_cell + cell_columns * tally.column + cell_rows * row;
        state.printer.attribute = 0;
        print_stunt(state, content, stream_of(content.stunt_tally_cells, row), words);
    }
    raised = second_tally_pass(state, content) || raised;
    tally.frames_waited = 0;
    tally.column_total_shown = !raised;
    if (raised) return;
    for (unsigned row = 0; row < trick_family::count; ++row) {
        const auto points = tally_of(state, row, tally.column).points;
        tally.total = static_cast<std::uint16_t>(tally.total + points);
    }
    words.record_or_total = tally.total;
    state.printer.attribute = 0;
    print_stunt(state, content, StuntText::total, words);
    if (state.now_playing.opponent < someone) {
        for (unsigned row = 0; row < trick_family::count; ++row)
            tally.second_total = static_cast<std::uint16_t>(
                tally.second_total
                + state.race_result.times.opponent_tallies[row][tally.column].points);
        words.record_or_total = tally.second_total;
        state.printer.attribute = 0x0400;
        print_stunt(state, content, StuntText::second_total, words);
    }
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
    if (tally.frames_waited <= wait && !any_button_pressed(pads)) { // $80:F7FB
        step_decorations(state, content);
        return;
    }
    if (tally.column_total_shown) {
        ++tally.column;
        tally.shown.fill(0);
        tally.second_shown.fill(0);
        if (tally.column == trick_family::columns) {
            tally.finished_frame = state.script_frame;
            return;
        }
    }
    tally_pass(state, content);
}

// The frames after the tally: `$80:F7CD-F7D7` and the total printed again, the rider's best score
// (`$80:F222-F244`: kept when higher, the new-best markers shown); then the qualifying score
// (`$80:F24F-F272`, a computer opponent), then `$80:F88D`'s score history and checksums, which
// native does not keep (R-0067); then the one-run result's waits (`$80:C24C`, `$80:C206`).
void after_tally_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    auto& result = state.race_result;
    const auto after = state.script_frame - result.tally.finished_frame;
    if (after == 1) {
        load_text(state, state.slide.shown_half);
        step_decorations(state, content);
        copy_oam(state);
        ResultWords words;
        words.record_or_total = result.tally.total;
        state.printer.attribute = 0;
        print_stunt(state, content, StuntText::total, words);
        auto& best = personal_best(state.records, state.rider_menu.rider, track_of(state));
        if (result.times.player_score > best) {
            best = result.times.player_score;
            high_bits(state, 96) &= stunt_best_markers;
        }
        return;
    }
    if (after == 2) {
        load_text(state, state.slide.shown_half);
        ResultWords words;
        if (state.now_playing.opponent < someone) {
            words.record_or_total = result.times.opponent_score;
            state.printer.attribute = 0x0400;
            print_stunt(state, content, StuntText::second_total, words);
            auto& best = personal_best(state.records, state.now_playing.opponent, track_of(state));
            if (result.times.opponent_score > best) {
                best = result.times.opponent_score;
                high_bits(state, 96) &= 0x99;
            }
        } else {
            words.qualifying_score = tour_qualifying_score(state, content); // $80:F259, `$83:9EEB`
            print_stunt(state, content, StuntText::qualify, words);
        }
        return;
    }
    if (after == 3) {
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
        return;
    }
    if (frame == stunt_text_frame) {
        show_stunt_text(state, content);
        return;
    }
    if (frame <= tally_start_frame) {
        copy_oam(state);
        state.registers.brightness = static_cast<std::uint8_t>(2 * (frame - first_fade_frame + 1));
        state.registers.force_blank = false;
        if (frame < tally_start_frame) return;
        high_bits(state, 100) =
            state.now_playing.opponent < someone ? four_shown : one_player_marks;
        state.race_result.tally = {};
        tally_pass(state, content);
        return;
    }
    if (state.race_result.tally.finished_frame == 0) {
        tally_frame(state, content, pads);
        return;
    }
    after_tally_frame(state, content, pads);
}

} // namespace unirally::front_end_screens
