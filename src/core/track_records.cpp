// TRACK RECORDS, RECORDS' first choice (`$80:D5E6`; R-0072, R-0100): the grid of every tour's
// three record holders, wiped in from the right through two HDMA tables on BG2 (`$80:EB76`), the
// grid scrolled up and down a row at a time and the column moved across the tours' tracks, the
// three records of the track under the arrow printed below it, and the wipe back out
// (`$80:9F7A`) to the RECORDS menu.
#include "front_end_screens.hpp"

#include <array>
#include <stdexcept>
#include <vector>

namespace unirally::front_end_screens {
namespace {

// ---- The HDMA tables ($0C70-$0C95) ----

// Each table is six direct entries of a line count and BG2's offset (mode 2: the low byte, then
// the high byte), then an end byte. The bands from the top: the title (its count grows as the
// wipe opens), an edge line, the grid's upper and lower parts, and two lines below them.
namespace grid_table {
// BG2VOFS's table (channel 5, `$0C70`).
inline constexpr std::size_t title_lines = 0x00, title_offset = 0x01, edge_offset = 0x04,
                             upper_lines = 0x06, upper_scroll = 0x07, lower_lines = 0x09,
                             lower_scroll = 0x0a, bottom_offset = 0x0d, last_offset = 0x10;
// BG2HOFS's table (channel 6, `$0C83`).
inline constexpr std::size_t hofs_table = 0x13, hofs_title_lines = 0x13, hofs_edge_high = 0x18,
                             hofs_upper_lines = 0x19, grid_offset = 0x1a, hofs_lower_lines = 0x1c,
                             lower_grid_offset = 0x1d;
} // namespace grid_table

std::uint16_t table_word(const TrackRecordsGrid& grid, std::size_t at) {
    return static_cast<std::uint16_t>(grid.tables[at] | (grid.tables[at + 1] << 8U));
}

void set_table_word(TrackRecordsGrid& grid, std::size_t at, unsigned value) {
    grid.tables[at] = static_cast<std::uint8_t>(value);
    grid.tables[at + 1] = static_cast<std::uint8_t>(value >> 8U);
}

// $80:EB7B-EBA2: `$80:ECD1` or, with the shown half at word 0x1400, `$80:ECF7`, which differs only
// in BG2HOFS's 0x100 that shows the grid from the other half's maps. The title's band starts with
// no lines, so the tables write nothing until the wipe opens it.
void copy_grid_tables(FrontEndState& state) {
    struct Band {
        std::uint8_t lines;
        std::uint16_t vertical;
    };
    constexpr std::array<Band, 6> bands{
        {{0, 0x127}, {1, 0x12b}, {0x67, 0}, {0x78, 0}, {1, 0x55}, {1, 0xcd}}};
    constexpr std::uint16_t second_maps = 0x1400, other_half = 0x100;
    auto& grid = state.records_detail.grid;
    const std::uint16_t horizontal = state.slide.shown_half == second_maps ? other_half : 0;
    for (std::size_t band = 0; band < bands.size(); ++band) {
        grid.tables[band * 3] = grid.tables[grid_table::hofs_table + band * 3] = bands[band].lines;
        set_table_word(grid, band * 3 + 1, bands[band].vertical);
        set_table_word(grid, grid_table::hofs_table + band * 3 + 1, horizontal);
    }
    grid.tables[grid_table::hofs_table - 1] = grid.tables[grid.tables.size() - 1] = 0;
}

// The picture's HDMA writes from the two tables while they are on.
void write_grid_lines(FrontEndState& state) {
    const auto& grid = state.records_detail.grid;
    if (!grid.hdma_on) return;
    const std::span<const std::uint8_t> tables = grid.tables;
    state.line_registers.clear();
    add_hdma_writes(state.line_registers, tables.first(grid_table::hofs_table), 2,
                    SnesLineRegisterName::bg2_vertical_offset);
    add_hdma_writes(state.line_registers, tables.subspan(grid_table::hofs_table), 2,
                    SnesLineRegisterName::bg2_horizontal_offset);
    sort_line_registers(state.line_registers);
}

// ---- The wipes ($80:A031, $80:9FBF) ----

constexpr std::uint8_t wipe_passes = 0x39, speeding_from = 0x31, slowing_below = 0x19;

// $80:A039-A047, $80:9FC7-9FD5: the grid's step rises by 1 a pass for the first 8 passes and
// falls by 1 for the last 25, so the passes add up to 256 pixels (a negative step adds nothing).
void set_wipe_speed(TrackRecordsGrid& grid) {
    if (grid.wipe_left >= speeding_from)
        ++grid.wipe_speed;
    else if (grid.wipe_left < slowing_below)
        --grid.wipe_speed;
}

void start_wipe(TrackRecordsGrid& grid) {
    grid.wipe_left = wipe_passes - 1;
    grid.wipe_speed = 0;
    set_wipe_speed(grid);
}

// `ADC $A1` and `SBC $A1` read the step as a word: its high byte, `$00A2`, holds 0x4C from the
// boot's `$80:A1A1-A1A4` (`STX $A2` of 0x044C) in every capture, which BG2HOFS's ten bits do not
// show (40 passes of 0x4C00 leave the bits above them).
void move_grid(TrackRecordsGrid& grid, bool in) {
    constexpr unsigned step_high_byte = 0x4c00;
    if (grid.wipe_speed < 0) return;
    const auto step = static_cast<unsigned>(grid.wipe_speed) | step_high_byte;
    const auto offset = table_word(grid, grid_table::grid_offset);
    const auto moved = in ? offset + step : offset - step;
    set_table_word(grid, grid_table::grid_offset, moved);
    set_table_word(grid, grid_table::lower_grid_offset, moved);
}

// A pass of the wipe in ($80:A04C-A091): the grid moves left, the title's band takes a line from
// the grid's upper part until its offset's low byte reaches 0, and the lower part gives a line
// to the bottom until it is 0x40 lines.
void wipe_in_pass(TrackRecordsGrid& grid) {
    using namespace grid_table;
    move_grid(grid, true);
    auto& t = grid.tables;
    if (t[title_offset] != 0) {
        ++t[title_lines];
        ++t[hofs_title_lines];
        --t[hofs_upper_lines];
        --t[upper_lines];
        --t[edge_offset];
        --t[title_offset];
    }
    constexpr std::uint8_t lower_open = 0x40;
    if (t[lower_lines] != lower_open) {
        --t[hofs_lower_lines];
        --t[lower_lines];
        set_table_word(grid, bottom_offset, table_word(grid, bottom_offset) + 1U);
        set_table_word(grid, last_offset, table_word(grid, last_offset) + 1U);
    }
}

// A pass of the wipe out ($80:9FDA-A01D): the reverse, without the lower part's limit; once the
// title's band is down to one line, the edge line's horizontal high byte is cleared instead.
void wipe_out_pass(TrackRecordsGrid& grid) {
    using namespace grid_table;
    move_grid(grid, false);
    auto& t = grid.tables;
    if (t[title_lines] == 1) {
        t[hofs_edge_high] = 0;
    } else {
        --t[title_lines];
        --t[hofs_title_lines];
        ++t[hofs_upper_lines];
        ++t[upper_lines];
        ++t[edge_offset];
        ++t[title_offset];
    }
    ++t[hofs_lower_lines];
    ++t[lower_lines];
    set_table_word(grid, bottom_offset, table_word(grid, bottom_offset) - 1U);
    set_table_word(grid, last_offset, table_word(grid, last_offset) - 1U);
}

// The pass's work after its frame wait, then the next pass's step; false after the last.
bool wipe_frame(FrontEndState& state, bool in) {
    auto& grid = state.records_detail.grid;
    copy_oam(state);
    if (in)
        wipe_in_pass(grid);
    else
        wipe_out_pass(grid);
    if (grid.wipe_left-- == 0) return false;
    set_wipe_speed(grid);
    return true;
}

void set_grid_scroll(TrackRecordsGrid& grid, std::uint16_t scroll) {
    grid.scroll = scroll;
    set_table_word(grid, grid_table::upper_scroll, scroll);
    set_table_word(grid, grid_table::lower_scroll, scroll);
}

// ---- The grid ($80:ED1D) ----

constexpr unsigned stunt_column = 2, record_ranks = 3;
constexpr unsigned first_records_row = 22, grid_column = 16, tour_rows = 6;

void grid_line(FrontEndState& state, const FrontEndContent& content, unsigned y, const char* word,
               unsigned length) {
    std::array<std::uint8_t, 16> stream{};
    stream[0] = print_code::place;
    stream[1] = 9;
    stream[2] = static_cast<std::uint8_t>(y);
    for (unsigned k = 0; k < length; ++k) stream[3 + k] = static_cast<std::uint8_t>(word[k]);
    stream[3 + length] = print_code::end;
    print_text(state.text, state.printer, std::span<const std::uint8_t>(stream.data(), 4 + length),
               content.character_table);
}

// $80:EEF2-EF35: a record's dot, the holder's eight-glyph icon in its palette, or an empty dot
// for no record. The same rank changes from blue to red when only its holder changes (R-0072).
void record_dot(FrontEndState& state, unsigned y, unsigned x, unsigned holder, bool placed) {
    constexpr std::array<std::uint8_t, 16> holder_palettes{4, 3, 0, 2, 3, 3, 4, 0,
                                                           3, 0, 4, 0, 2, 1, 0, 5};
    constexpr std::uint16_t empty_top = 0x3cd9, empty_bottom = 0x3d15, flipped = 0x4000;
    const std::uint16_t top =
        holder < holder_palettes.size()
            ? static_cast<std::uint16_t>(0x20d1U + (holder & 7U)
                                         + (static_cast<unsigned>(holder_palettes[holder]) << 10U))
            : empty_top;
    const std::uint16_t dot_top = placed ? top : empty_top;
    const std::uint16_t dot_bottom =
        placed ? static_cast<std::uint16_t>(top + 0x3cU) : empty_bottom;
    auto& words = state.text.words;
    words[y * 32 + x] = dot_top;
    words[y * 32 + x + 1] = dot_top | flipped;
    words[(y + 1) * 32 + x] = dot_bottom;
    words[(y + 1) * 32 + x + 1] = dot_bottom | flipped;
}

// One half of the grid into the text map: tours 0-4 from row 2, or 5-8 from row 0, each a badge,
// its medals' names and a dot for each track's three records. `$00CE`, the chosen track, is left
// on the half's last tour's track 5 (25 or 45).
void print_grid_half(FrontEndState& state, const FrontEndContent& content, bool second) {
    constexpr unsigned first_half_tours = 5, second_half_tours = 4;
    constexpr std::array<const char*, record_ranks> medals{"gold", "silver", "bronze"};
    constexpr std::array<unsigned, record_ranks> medal_lengths{4, 6, 6};
    const unsigned first_tour = second ? first_half_tours : 0;
    const unsigned count = second ? second_half_tours : first_half_tours;
    const unsigned starting_row = second ? 0 : 2;
    for (unsigned i = 0; i < count; ++i) {
        const unsigned tour = first_tour + i, row = starting_row + i * tour_rows;
        draw_tour_picture(state, content, tour, row * 32 + 2);
        for (unsigned rank = 0; rank < record_ranks; ++rank) {
            const unsigned y = row + 2 * rank;
            grid_line(state, content, y, medals[rank], medal_lengths[rank]);
            for (unsigned track = 0; track < tracks_per_tour; ++track) {
                const unsigned index = tour * tracks_per_tour + track;
                const bool placed =
                    state.records.record_times[rank][index] != (track == stunt_column ? 0 : 0xea60);
                record_dot(state, y, grid_column + 3 * track,
                           state.records.record_holders[rank][index], placed);
            }
        }
    }
    state.tour_menu.track = static_cast<std::uint8_t>((first_tour + count) * tracks_per_tour);
}

// ---- The records under the grid ($80:D682-D6D3) ----

// `$80:D777`, the three lines' template inside `front-end.track-records-text`.
constexpr std::size_t template_offset = 0x4e;

// $80:D699-D6B1: the tour under the arrow from the grid's scroll: a half-pixel shift, three
// 16-pixel rows a tour; the row's 5 bits wrap while the scroll goes on. Past tour 8 nothing new
// is printed (`$80:D6AE`).
unsigned tour_under_arrow(const TrackRecordsGrid& grid) {
    constexpr std::uint16_t first_row_scroll = 0x01a7;
    const auto delta = static_cast<std::uint16_t>(grid.scroll - first_row_scroll);
    const unsigned row = (((delta >> 1U) + 1U) >> 3U) & 31U;
    return row / 3U;
}

// $80:C30E: a race's time (`$83:8C7B`) at column 6; `$80:C269`: a stunt event's score as five
// digits after two blanks at column 7. Then "_by_" and the holder's name.
std::vector<std::uint8_t> record_line(const FrontEndState& state, const FrontEndContent& content,
                                      unsigned rank, unsigned track) {
    const auto row = static_cast<std::uint8_t>(first_records_row + 2 * rank);
    const auto record = state.records.record_times[rank][track];
    std::vector<std::uint8_t> stream;
    if (state.records_detail.grid.column == stunt_column) {
        const auto score = static_cast<std::uint8_t>(0xb2 + 2 * rank); // `$00B2`, `$B4`, `$B6`
        stream = {print_code::place, 7, row, '_', '_', print_code::number, score, 0};
    } else {
        stream = {print_code::place, 6, row};
        const auto time = race_time_text(record, content.time_words);
        stream.insert(stream.end(), time.begin(), time.end());
    }
    const auto holder = static_cast<std::uint8_t>(0xbc + 2 * rank); // `$00BC`, `$BE`, `$C0`
    stream.insert(stream.end(),
                  {'_', 'b', 'y', '_', print_code::rider_name, holder, 0, print_code::end});
    return stream;
}

void print_selected_records(FrontEndState& state, const FrontEndContent& content) {
    auto& grid = state.records_detail.grid;
    state.text.words.fill(cleared_text); // $83:8B51
    const auto text = content.track_records_text;
    if (text.size() <= template_offset || text[template_offset] != print_code::place)
        throw std::invalid_argument("front-end.track-records-text has no records template");
    print_text(state.text, state.printer, text.subspan(template_offset), content.character_table);
    const unsigned tour = tour_under_arrow(grid);
    constexpr unsigned tours_in_grid = 9;
    if (tour >= tours_in_grid) return;
    const unsigned track = tour * tracks_per_tour + grid.column;
    for (unsigned rank = 0; rank < record_ranks; ++rank) {
        grid.holders[rank] = state.records.record_holders[rank][track];
        TextVariables variables;
        variables.rider_names = state.records.rider_names;
        variables.word = [&state, &grid, rank, track](std::uint16_t address) -> std::uint16_t {
            if (address == 0xbc + 2 * rank) return grid.holders[rank];
            if (address == 0xb2 + 2 * rank) return state.records.record_times[rank][track];
            throw std::invalid_argument("unknown TRACK RECORDS text operand");
        };
        print_text(state.text, state.printer, record_line(state, content, rank, track),
                   content.character_table, &variables);
    }
}

// The loop's pass before its frame wait: the records printed, the decorations stepped.
void print_loop_pass(FrontEndState& state, const FrontEndContent& content) {
    print_selected_records(state, content);
    step_decorations(state, content); // $80:D6D5
}

// $80:D6DC-D703 after the wait: the last print's holders' colours, the text's rows 22 on to the
// grid's half (`$80:93CB`: 0x1FF bytes, so the last word's high byte stays), the OAM.
void show_selected_records(FrontEndState& state, const FrontEndContent& content) {
    const auto& grid = state.records_detail.grid;
    for (unsigned rank = 0; rank < record_ranks; ++rank)
        load_cgram(state, asset(content, first_rider_palette + grid.holders[rank]),
                   0x80 + 16 * rank);
    constexpr unsigned first_word = first_records_row * 32, bytes = 0x1ff;
    const unsigned to = (state.slide.shown_half + 0x800U + first_word) * 2U;
    for (unsigned k = 0; k < bytes; ++k) {
        const auto word = state.text.words[first_word + k / 2];
        state.video.vram[(to + k) & 0xffffU] =
            static_cast<std::uint8_t>(k % 2 == 0 ? word : word >> 8U);
    }
    copy_oam(state); // $80:D1EC
}

// ---- The loop's input ($80:E3E9) ----

// $80:E536: the scroll's pixels a frame for each speed step.
constexpr std::array<std::int8_t, 11> scroll_speeds{-4, -3, -2, -2, -1, 0, 1, 2, 2, 3, 4};
constexpr std::uint8_t still = 5, fastest_down = 10, at_rest = 7, at_row_end = 0xf;
constexpr std::uint8_t faster_down = 0, easing = 1, faster_up = 2;

// $80:E3EC-E435: a scroll off its rows' rhythm is put back on it by a pixel.
void align_scroll(TrackRecordsGrid& grid) {
    switch (grid.scroll & 0xfU) {
    case 0x0:
    case 0xb:
    case 0xd: --grid.scroll; return;
    case 0x1:
    case 0x3:
    case 0x5:
    case 0x9:
    case 0xe: ++grid.scroll; return;
    default: return;
    }
}

// $80:E437-E481: at rest Down speeds the grid down and Up up (Up winning); at a row's end with
// neither held it eases to the next rest from its fastest speed in its direction.
void steer_scroll(TrackRecordsGrid& grid, std::uint16_t pad) {
    const bool up = (pad & pad_up) != 0, down = (pad & pad_down) != 0;
    const auto phase = static_cast<std::uint8_t>(grid.scroll & 0xfU);
    if (phase == at_row_end) {
        if (up || down) return;
        grid.drive = easing;
        if (grid.speed_step > still) grid.speed_step = fastest_down;
        if (grid.speed_step >= still) return;
        grid.speed_step = 0;
    } else if (phase != at_rest) {
        return;
    }
    if (down) grid.drive = faster_down;
    if (up) grid.drive = faster_up;
}

// $80:E483-E4E4: Left and Right each latched in `$008F`, each a move's sound on its press and a
// column's move within 0-4 (no wrap); Left is tested first and Right after it.
void move_column(FrontEndState& state, std::uint16_t pad) {
    auto& grid = state.records_detail.grid;
    const auto press = [&](bool held, bool& latched, int step) {
        if (!held) {
            latched = false;
            return;
        }
        if (latched) return;
        latched = true;
        play_menu_sound(state, MenuSound::navigate); // $80:E492, $80:E4C6
        const int column = grid.column + step;
        if (column < 0 || column >= static_cast<int>(tracks_per_tour)) return;
        grid.column = static_cast<std::uint8_t>(column);
        constexpr std::uint16_t first_column_x = 0x0600, column_step = 0x0180;
        state.arrow.target_x = static_cast<std::uint16_t>(first_column_x + column * column_step);
    };
    press((pad & pad_left) != 0, grid.left_latched, -1);
    press((pad & pad_right) != 0, grid.right_latched, 1);
}

// $80:E4EC-E52F: the speed steps toward the drive's end, and the scroll moves by its speed.
void step_scroll(TrackRecordsGrid& grid) {
    if (grid.drive == easing) {
        if (grid.speed_step < still) ++grid.speed_step;
        if (grid.speed_step > still) --grid.speed_step;
    } else if (grid.drive == faster_down) {
        if (grid.speed_step < fastest_down) ++grid.speed_step;
    } else if (grid.speed_step > 0) {
        --grid.speed_step;
    }
    set_grid_scroll(grid, static_cast<std::uint16_t>(grid.scroll + scroll_speeds[grid.speed_step]));
}

void steer_grid(FrontEndState& state, std::uint16_t pad) {
    auto& grid = state.records_detail.grid;
    align_scroll(grid);
    steer_scroll(grid, pad);
    move_column(state, pad);
    step_scroll(grid);
}

// ---- The entry ($80:D5E6, $80:EB76) ----

// The entry's script frames: its loads, the wipe's 57 passes, the grid's two halves (each runs
// past its frame's end: the frame after has no frame wait) with the badge tiles and the base
// colours between them, the grid's 18 passes into place, then the records and the loop.
namespace entry_frame {
inline constexpr std::uint32_t tables = 1, title = 2, wipe_start = 3, wipe_end = 60,
                               grid_reset = 61, grid_colours = 62, first_half = 63,
                               first_half_upload = 65, badge_tiles = 66, palette_low = 67,
                               palette_high = 68, scroll_last = 86, second_half_upload = 88;
} // namespace entry_frame
constexpr std::uint16_t grid_start_scroll = 0x0157, grid_scroll_step = 8, grid_rest = 0x01e7;

// $80:D5E8-D60B, $80:EB78-EBDB: the medals' colours, the forward slide's sound, the tables and
// HDMA on, the latches cleared and the text.
void open_grid(FrontEndState& state, const FrontEndContent& content) {
    load_cgram(state, asset(content, 34), 0xb0);
    load_cgram(state, asset(content, 33), 0xc0);
    load_cgram(state, asset(content, 32), 0xe0);
    play_menu_sound(state, MenuSound::forward_slide);
    copy_grid_tables(state);
    state.records_detail.grid.hdma_on = true;
    state.latches = {};
    state.records_detail.grid.right_latched = state.records_detail.grid.left_latched = false;
    state.text.words.fill(cleared_text);
}

// $80:EBE2-EC1A.
void entry_title_frames(FrontEndState& state, const FrontEndContent& content, std::uint32_t frame) {
    auto& slide = state.slide;
    if (frame == entry_frame::title) {
        load_text(state, slide.hidden_half);
        load_text(state, slide.hidden_half + 0x800U);
        print_text(state.text, state.printer, content.track_records_text, content.character_table);
        return;
    }
    load_text(state, slide.shown_half + 0x800U);
    copy_oam(state);
    state.arrow.target_x = 0x0600;
    state.arrow.target_y = 0x0740;
    start_wipe(state.records_detail.grid);
}

// $80:EC20-EC85: after the wipe, the grid's offset and scroll reset, its two colours, the first
// half printed and sent, the badge tiles (`$80:A82B`) and the base colours (`$80:A858`).
void entry_grid_frames(FrontEndState& state, const FrontEndContent& content, std::uint32_t frame) {
    auto& grid = state.records_detail.grid;
    const auto hidden = state.slide.hidden_half;
    switch (frame) {
    case entry_frame::grid_reset:
        copy_oam(state);
        grid.tables[grid_table::grid_offset] = grid.tables[grid_table::lower_grid_offset] = 0;
        set_grid_scroll(grid, grid_start_scroll);
        return;
    case entry_frame::grid_colours:
        if (!content.assets[29].empty()) load_cgram(state, asset(content, 29), 0x00);
        if (!content.assets[30].empty()) load_cgram(state, asset(content, 30), 0x10);
        copy_oam(state);
        state.text.words.fill(cleared_text);
        return;
    case entry_frame::first_half:
        load_text(state, hidden + 0x800U);
        copy_oam(state);
        print_grid_half(state, content, false);
        return;
    case entry_frame::first_half_upload:
        load_text(state, hidden);
        copy_oam(state);
        state.text.words.fill(cleared_text);
        return;
    case entry_frame::badge_tiles: {
        constexpr unsigned badge_tiles_asset = 68, badge_tiles_word = 0x3d80;
        constexpr std::size_t badge_tiles_bytes = 0x780;
        copy_oam(state);
        load_vram(state, asset(content, badge_tiles_asset).last(badge_tiles_bytes),
                  badge_tiles_word);
        return;
    }
    case entry_frame::palette_low:
        copy_oam(state);
        load_cgram(state, asset(content, base_palette_low), 0);
        return;
    case entry_frame::palette_high:
        copy_oam(state);
        load_cgram(state, asset(content, base_palette_high), 0x40);
        grid.scroll = grid_start_scroll;
        play_menu_sound(state, MenuSound::forward_slide); // $80:EC8A
        return;
    default: return;
    }
}

// $80:D60E-D681: the medal objects (entries 112-114) beside the records, the printed icons
// (entries 104-106, `$80:D7BD`) raised five pixels, the grid at rest on tour 0's first track.
void place_record_objects(FrontEndState& state, const FrontEndContent& content) {
    constexpr std::array<std::uint8_t, record_ranks> medal_y{0xae, 0xbe, 0xce},
        medal_attributes{0x17, 0x19, 0x1d};
    for (unsigned rank = 0; rank < record_ranks; ++rank) {
        oam_byte(state, 112 + rank, 0) = 0x20;
        oam_byte(state, 112 + rank, 1) = medal_y[rank];
        oam_byte(state, 112 + rank, 3) = medal_attributes[rank];
    }
    high_bits(state, 112) = 0x40;
    TextVariables variables;
    variables.place_object = [&](unsigned object, unsigned position) {
        place_printed_object(state, object, position);
    };
    print_text(state.text, state.printer, content.track_records_objects, content.character_table,
               &variables);
    constexpr std::uint8_t icon_raise = 5;
    for (unsigned rank = 0; rank < record_ranks; ++rank)
        oam_byte(state, 104 + rank, 1) =
            static_cast<std::uint8_t>(oam_byte(state, 104 + rank, 1) - icon_raise);
    high_bits(state, 104) = 0x40;
    auto& grid = state.records_detail.grid;
    grid.scroll = grid_rest;
    grid.speed_step = still;
    grid.column = 0;
    grid.drive = easing;
}

// $80:ECAE-ECD0, then `$80:D60E` and the loop's first pass: the second half sent, the result's
// sound.
void show_records(FrontEndState& state, const FrontEndContent& content) {
    play_menu_sound(state, MenuSound::result); // $80:ECC7
    load_text(state, state.slide.hidden_half + 0x800U);
    copy_oam(state);
    place_record_objects(state, content);
    print_loop_pass(state, content);
    state.screen = FrontEndScreen::records_detail;
}

// ---- The way out ($80:D71F, $80:9F7A) ----

namespace exit_frame {
inline constexpr std::uint32_t text_tiles = 2, base_colours = 3, wipe_start = 4, wipe_end = 61;
} // namespace exit_frame

// $80:D71F-D725, `$80:9F7A-9F88`: the logo down, the printed icons and the medals hidden, the
// forward slide's sound, the text cleared.
void leave_grid(FrontEndState& state) {
    state.logo.raised = false;                       // $80:F52B
    for (unsigned entry = 104; entry < 112; ++entry) // $80:D420
        oam_byte(state, entry, 0) = oam_byte(state, entry, 1) = 1;
    high_bits(state, 104) = high_bits(state, 108) = high_bits(state, 112) = four_hidden;
    play_menu_sound(state, MenuSound::forward_slide);
    state.text.words.fill(cleared_text);
    state.screen = FrontEndScreen::records_detail_exit;
}

} // namespace

bool track_records_entry_waits(std::uint32_t next) {
    // Each half of the grid runs past the frame it starts in ($80:ED1D, R-0100).
    return next != entry_frame::first_half + 1 && next != entry_frame::scroll_last + 1;
}

void track_records_entry_frame(FrontEndState& state, const FrontEndContent& content) {
    const auto frame = state.script_frame;
    auto& grid = state.records_detail.grid;
    if (frame == entry_frame::tables) {
        open_grid(state, content);
    } else if (frame <= entry_frame::wipe_start) {
        entry_title_frames(state, content, frame);
    } else if (frame <= entry_frame::wipe_end) {
        wipe_frame(state, true);
    } else if (frame <= entry_frame::palette_high) {
        entry_grid_frames(state, content, frame);
    } else if (frame <= entry_frame::scroll_last) {
        copy_oam(state);
        set_grid_scroll(grid, static_cast<std::uint16_t>(grid.scroll + grid_scroll_step));
        if (frame == entry_frame::scroll_last) print_grid_half(state, content, true);
    } else if (frame == entry_frame::second_half_upload) {
        show_records(state, content);
    }
    write_grid_lines(state);
}

void track_records_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    show_selected_records(state, content);
    // The tests read either pad the gate lets in ($80:D70F-D714, `$80:E449-E4B7`; R-0097).
    const auto pad = gated_pads(state, pads);
    if (pad & (choose_buttons | back_buttons)) {
        leave_grid(state);
    } else {
        steer_grid(state, pad);
        print_loop_pass(state, content);
    }
    write_grid_lines(state);
}

void track_records_exit_frame(FrontEndState& state, const FrontEndContent& content) {
    const auto frame = state.script_frame;
    auto& grid = state.records_detail.grid;
    const auto hidden = state.slide.hidden_half;
    if (frame == 1) { // $80:9F8B-9FA1: the cleared text to both of the hidden half's maps
        load_text(state, hidden);
        load_text(state, hidden + 0x800U);
    } else if (frame == exit_frame::text_tiles) {
        reload_menu_text_tiles(state, content); // $80:A877
    } else if (frame == exit_frame::base_colours) {
        reload_menu_palette(state, content); // $80:A8A8
        set_grid_scroll(grid, 0);
        load_text(state, hidden + 0x800U);
    } else if (frame == exit_frame::wipe_start) {
        start_wipe(grid);
    } else if (frame <= exit_frame::wipe_end) {
        wipe_frame(state, false);
    } else { // the frame after the last pass
        // $80:A029: HDMA off; back through `$80:D55F` to the RECORDS menu's loop.
        grid.hdma_on = false;
        state.registers.bg[1].vofs = 0;
        state.logo.raised = false;
        reopen_records_menu(state, content);
        return;
    }
    write_grid_lines(state);
}

} // namespace unirally::front_end_screens
