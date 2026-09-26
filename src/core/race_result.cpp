// After a one-player race (R-0057): the race returns (`$80:99A4` after `$83:C8E0`), the menu
// machine is reloaded as at the boot, the one-run result screen is built (`$80:951C`,
// `$80:CE90`) and faded in, and a press leaves it (`$80:C24C`, `$80:C206`). Leaving it updates
// the records (`$80:C786`) and scores the race (`$83:879A`), then PICK TRACK comes again.
#include "front_end_screens.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace unirally::front_end_screens {

namespace {

// The race's return, frames from the race's last frame (r, `$80:A09A`, NMI off): the sound
// program's upload takes 74 frames; `$80:D20E` then as at the boot (377-403, from
// `menu_screen_frame`); the text tiles, the palettes, and the result's build (R-0057's
// timeline). `restore_frame` is in front_end_screens.hpp.
constexpr std::uint32_t objects_frame = 100, tiles_frame = 102, palette_low_frame = 103,
                        palette_high_frame = 104;
constexpr unsigned early_palette = 5, early_objects = 88; // $80:A09A
constexpr unsigned badge_tiles_asset = 68, badge_tiles_word = 0x3d80;
constexpr std::size_t badge_tiles_bytes = 0x780;

// The result screen's frames from its first, `$83:94D0`'s wait: the rows, the icons and the
// text, then the fade (`$80:9869`, brightness 2, 4, ..., 14).
constexpr std::uint32_t rows_frame = 2, icons_frame = result_tail_frame, first_fade_frame = 4,
                        fade_frames = 7;
constexpr unsigned first_rider_palette_colour = 0x80, opponent_palette_colour = 0x90;
constexpr unsigned rows_palettes = 0x80;         // `$80:D163`: a row's rider at 0x80, 0x90, ...
constexpr unsigned trophy_palettes_first = 0x22; // assets 0x22, 0x21, 0x20 at 0xD0, 0xE0, 0xF0

// The rows `$80:CE90` sorts: the player, the opponent, the three records, three blanks.
enum class RowKind : std::uint8_t { blank, player, opponent, record_1, record_2, record_3 };
struct Row {
    RowKind kind;
    std::uint16_t time;
};
constexpr std::uint16_t no_row = 0xea62; // sorts after every time
// A total the race's pause menu writes (`$83:F8DA-F90B`): quit after the countdown, restart during
// it (R-0060). `$80:88DD` also tests the opponent's total, which only pad 2's pause writes: not in
// one-player play.
constexpr std::uint16_t restarted = 0xea62;

// The result's objects (`$80:951C`, `$80:CE90`): the two riders' large marks (entries 30 and 31);
// the row icons (104-108); the new-best markers (96-99); the 1P mark (100, 102) and the 2P mark
// (101, 103), which moves to its row: 24 lines a row from line 0x58.
constexpr std::uint8_t rider_mark_tile = 0xc0, rider_mark_attributes = 0x21,
                       opponent_mark_attributes = 0x23, rider_marks_high = 0xf5;
constexpr std::uint8_t icon_column = 0x78, marker_column = 0xdb, marker_attributes = 0x1b;
constexpr std::uint8_t first_row_line = 0x58, row_lines = 24, player_mark_attributes = 0x11;
constexpr std::uint8_t off_screen_line = 0xef;

// Pads that leave the one-run result: any button on either pad (`$80:C206`).
bool pressed(FrontEndPads pads) {
    return pads.one != 0 || pads.two != 0;
}

// Pads that leave the lap result: any of pad 1's twelve buttons (`$80:B6D3`); pad 2 is ignored in
// one-player play, where `$77:0742` bit 10 is set.
bool lap_graph_left(FrontEndPads pads) {
    constexpr std::uint16_t buttons = 0xfff0;
    return (pads.one & buttons) != 0;
}

std::uint8_t track_of(const FrontEndState& state) {
    return state.tour_menu.track;
}

void restore_menus(FrontEndState& state) { // $83:987D
    const auto& saved = state.saved;
    state.menu = saved.menu;
    state.cycle.delay = saved.cycle.delay;
    state.cycle.phase = saved.cycle.phase;
    state.logo.offset = saved.logo_offset;
    state.slide = saved.slide;
    state.decorations = saved.decorations;
    state.latches = saved.latches;
    state.rider_menu = saved.rider_menu;
    state.tour_menu = saved.tour_menu;
    state.track_menu = saved.track_menu;
    state.now_playing = saved.now_playing;
    state.printer = saved.printer;
    state.arrow.spin = saved.arrow_spin;
}

// $80:A09A: forced blank, the logo's slide flag cleared, NMI off, the early loads.
void early_loads(FrontEndState& state, const FrontEndContent& content) {
    state.registers.force_blank = true;
    state.logo.raised = false;
    state.cycle.running = false;
    clear_oam_buffer(state);
    load_cgram(state, asset(content, early_palette), 0xe0);
    load_vram(state, asset(content, early_objects), 0x7000);
    set_early_registers(state);
}

// $80:CE90-CF51 and `$80:F082`: the rows sorted by time, a later equal time last.
std::array<Row, 8> sorted_rows(const FrontEndState& state) {
    const auto& records = state.records;
    const auto track = track_of(state);
    const auto& times = state.race_result.times;
    const bool computer = state.now_playing.opponent >= someone;
    std::array<Row, 8> rows{{{RowKind::player, times.player_total},
                             {RowKind::opponent, computer ? no_row : times.opponent_total},
                             {RowKind::record_1, records.record_times[0][track]},
                             {RowKind::record_2, records.record_times[1][track]},
                             {RowKind::record_3, records.record_times[2][track]},
                             {RowKind::blank, no_row},
                             {RowKind::blank, no_row},
                             {RowKind::blank, no_row}}};
    for (std::size_t last = rows.size(); last-- > 0;) {
        std::size_t latest = 0;
        std::uint16_t longest = 0;
        for (std::size_t k = 0; k <= last; ++k)
            if (rows[k].time >= longest) {
                longest = rows[k].time;
                latest = k;
            }
        std::swap(rows[latest], rows[last]);
    }
    return rows;
}

// $80:CF5B-D0E6: each of the first five rows' rider (or holder) and time into `$00B2 + 4n`,
// its colours at 0x80 + 16n; the player's personal best and its markers; the 1P mark.
void print_rows(FrontEndState& state, const FrontEndContent& content,
                std::array<std::uint16_t, 16>& words) {
    auto& records = state.records;
    const auto track = track_of(state);
    const auto rows = sorted_rows(state);
    unsigned printed = 0;
    for (std::size_t k = 0; k < 5; ++k) {
        const auto& row = rows[k];
        std::uint8_t rider{};
        switch (row.kind) {
        case RowKind::blank: continue;
        case RowKind::opponent: continue; // a human opponent's row is 2P's (not recovered)
        case RowKind::player: {
            rider = state.rider_menu.rider;
            auto& best = personal_best(records, rider, track);
            if (row.time < best) {
                best = row.time;
                high_bits(state, 96) &= player_best_markers;
            }
            const auto y = static_cast<std::uint8_t>(first_row_line + row_lines * printed);
            for (const unsigned entry : {96U, 98U, 100U, 102U}) oam_byte(state, entry, 1) = y;
            for (const unsigned entry : {100U, 102U})
                oam_byte(state, entry, 3) =
                    static_cast<std::uint8_t>(player_mark_attributes | (printed * 2));
            break;
        }
        default:
            rider = records.record_holders[static_cast<unsigned>(row.kind)
                                           - static_cast<unsigned>(RowKind::record_1)][track];
            break;
        }
        words[printed * 2] = rider;
        words[printed * 2 + 1] = row.time;
        load_cgram(state, asset(content, first_rider_palette + rider),
                   rows_palettes + printed * 16);
        ++printed;
    }
}

// $80:951C-9555 and $80:CE90-CF4E, on the result's first frame. In one-player play the opponent
// is a computer's (0x11 on); a rider opponent's row and 2P mark (`$80:D007`) are not recovered.
void start_result(FrontEndState& state, const FrontEndContent& content) {
    if (state.now_playing.opponent < someone)
        throw std::logic_error("a rider opponent's result row ($80:D007) is not recovered");
    load_vram(state, content.medal_tiles, swapped_object_tiles_word); // $83:94D0
    load_cgram(state, asset(content, first_rider_palette + state.rider_menu.rider),
               first_rider_palette_colour);
    load_cgram(state, asset(content, first_rider_palette + state.now_playing.opponent),
               opponent_palette_colour);
    for (const unsigned entry : {30U, 31U}) oam_byte(state, entry, 2) = rider_mark_tile;
    oam_byte(state, 30, 3) = rider_mark_attributes;
    oam_byte(state, 31, 3) = opponent_mark_attributes;
    high_bits(state, 28) = rider_marks_high;
    // $80:F53F: the logo held up.
    state.logo.raised = true;
    state.logo.offset = 0x52;
    state.registers.bg[0].vofs = 0x52;
    state.text.words.fill(cleared_text);
    if (state.race_result.times.lap_race) {
        build_lap_result(state, content); // $80:8D6E
        return;
    }
    // The icons beside the rows, entries 104-108, and the new-best markers 96-99 (hidden).
    constexpr std::array<std::uint8_t, 5> icon_rows{0x57, 0x6f, 0x87, 0x9f, 0xb7};
    for (unsigned k = 0; k < icon_rows.size(); ++k) {
        oam_byte(state, 104 + k, 0) = icon_column;
        oam_byte(state, 104 + k, 1) = icon_rows[k];
    }
    high_bits(state, 104) = four_shown;
    high_bits(state, 108) = static_cast<std::uint8_t>(four_hidden & ~hidden_bit(108));
    for (unsigned entry = 96; entry < 100; ++entry) {
        oam_byte(state, entry, 0) = marker_column;
        oam_byte(state, entry, 3) = marker_attributes;
    }
    high_bits(state, 96) = four_hidden;
}

void print_result(FrontEndState& state, const FrontEndContent& content) {
    std::array<std::uint16_t, 16> words{};
    print_rows(state, content, words);
    TextVariables variables;
    variables.word = [&](std::uint16_t address) -> std::uint16_t {
        constexpr std::uint16_t track_word = 0xce, first_row_word = 0xb2;
        if (address == track_word) return track_of(state);
        return words[(address - first_row_word) / 2U];
    };
    variables.track_names = content.track_names;
    variables.rider_names = content.rider_names;
    variables.time_words = content.time_words;
    print_text(state.text, state.printer, content.result_text, content.character_table, &variables);
    // A computer opponent has no row: the 2P mark and entries 108-111 go (`$80:D0FE`).
    high_bits(state, 108) = four_hidden;
    for (const unsigned entry : {101U, 103U}) oam_byte(state, entry, 1) = off_screen_line;
    high_bits(state, 100) = four_shown;
}

// $80:D113-D13C for a one-run race, then the tail `$80:9579-958F`.
void show_icons(FrontEndState& state, const FrontEndContent& content) {
    if (!state.race_result.times.lap_race) {
        for (unsigned k = 0; k < 3; ++k)
            load_cgram(state, asset(content, trophy_palettes_first - k), 0xd0 + k * 16);
        for (std::size_t k = 0; k < content.result_icons.size(); ++k)
            state.oam_buffer[112 * 4 + k] = content.result_icons[k];
        high_bits(state, 112) = hidden_bit(115);
    }
    load_text(state, state.slide.shown_half); // $80:93A5 to $00AA
    state.decorations.delay = state.decorations.wave_delay = state.decorations.sway = 0;
    step_decorations(state, content);
}

// $80:C236-C245, `$80:9805` and `$80:F4B8`'s first half, on the press's second frame.
void hide_result_objects(FrontEndState& state) {
    high_bits(state, 32) = four_hidden;
    // `$80:C236`: entries 30 and 31 hidden and small. `$80:9805` then parks 0-29 and hides 0-31,
    // but only while `$77:0742` bit 8 is clear, and sets it: the first result after a cold start
    // parks, later one-run results skip it until a lap result's graph (`$80:98CB`) clears the
    // bit. Native keeps no bit 8 and always parks: `$80:A09A` has already parked and hidden
    // those entries, so the OAM buffer is the same either way.
    high_bits(state, 28) = static_cast<std::uint8_t>((high_bits(state, 28) & 0x0fU) | 0x50U);
    for (unsigned entry = 0; entry < 30; ++entry)
        oam_byte(state, entry, 0) = oam_byte(state, entry, 1) = 1;
    for (unsigned entry = 0; entry < 32; entry += 4) high_bits(state, entry) = four_hidden;
    for (const unsigned group : {96U, 100U, 104U, 108U, 112U, 120U})
        high_bits(state, group) = four_hidden;
}

// $80:C9D7: a time into the track's top three, shorter strictly first; true when it is placed
// (and the original then recomputes the checksums, `$83:90F4`).
bool insert_record(OnePlayerRecords& records, unsigned track, std::uint16_t time,
                   std::uint8_t holder) {
    auto& times = records.record_times;
    auto& holders = records.record_holders;
    for (unsigned place = 0; place < 3; ++place) {
        if (time >= times[place][track]) continue;
        for (unsigned later = 2; later > place; --later) {
            times[later][track] = times[later - 1][track];
            holders[later][track] = holders[later - 1][track];
        }
        times[place][track] = time;
        holders[place][track] = holder;
        return true;
    }
    return false;
}

// $80:C786: races and wins by rider, and the times into the records: for a one-run race
// (`$80:C7C4`) the totals, for a lap race (`$80:C868`) the best laps; a win is still the lower
// total. A computer opponent (16 and up) keeps no counts and no records (`$80:CAAA`, `$80:C82F`).
void update_records(FrontEndState& state) {
    auto& records = state.records;
    const auto rider = state.rider_menu.rider, opponent = state.now_playing.opponent;
    const bool rider_opponent = opponent < records.statistics.size();
    const auto& times = state.race_result.times;
    const auto player = times.player_total, other = times.opponent_total;
    // A lap race's records take the best laps (`$80:C868`), a one-run race's the totals.
    const auto player_record = times.lap_race ? record_lap(times.player_laps) : player;
    const auto other_record = times.lap_race ? record_lap(times.opponent_laps) : other;
    constexpr std::size_t races = 0, wins = 1, no_time_losses = 2;
    const auto track = track_of(state);
    // A time goes into the records unless it is no time, which counts a loss without one.
    bool placed = false;
    const auto insert = [&](std::uint16_t time, std::uint8_t who) {
        placed = insert_record(records, track, time, who) || placed;
    };
    const auto record = [&](std::uint8_t who, std::uint16_t time) {
        if (time >= no_time)
            ++records.statistics[who][no_time_losses];
        else
            insert(time, who);
    };
    ++records.statistics[rider][races]; // $80:CA74
    if (rider_opponent) ++records.statistics[opponent][races];
    if (player <= other) { // $80:CA83: a win or a tie
        ++records.statistics[rider][wins];
        ++records.player_wins;
    }
    if (player >= other && rider_opponent) { // $80:CAC5
        ++records.statistics[opponent][wins];
        ++records.opponent_wins;
    }
    if (player < other) {
        insert(player_record, rider);                       // $80:C81C, $80:C902
        if (rider_opponent) record(opponent, other_record); // $80:C82F, $80:C913
    } else if (player > other) {
        if (rider_opponent) insert(other_record, opponent); // $80:C850
        record(rider, player_record);                       // $80:C800, $80:C8E8
    } else {                                                // $80:C7EE, $80:C8D0
        insert(player_record, rider);
        if (rider_opponent) insert(other_record, opponent);
    }
    // A placed time's checksums run `$80:C786` past its frame's end, so the scoring waits a frame
    // more (R-0060).
    state.race_result.record_placed = placed;
}

// $83:879A on its pad read (`$80:D1E8`, the exit's third frame): pad 1 exactly Select + X + R
// forces the tour's completion (`$83:87B6`); otherwise a win (a total strictly under the
// opponent's) marks the track done, and the tour's fifth done track completes it; a loss sets
// `$77:0742` bit 12.
void score_race(FrontEndState& state, FrontEndPads pads) {
    constexpr std::uint16_t forced_completion = 0x2050;
    if (pads.one == forced_completion) {
        complete_tour(state);
        return;
    }
    const auto& times = state.race_result.times;
    auto& records = state.records;
    if (times.player_total >= times.opponent_total) {
        records.race_lost = true;
        return;
    }
    const auto track = track_of(state);
    constexpr unsigned track_bits = 0x3f; // $83:9EC8
    records.tracks_done[track & track_bits] = 1;
    const auto first = track / tracks_per_tour * tracks_per_tour;
    if (std::all_of(records.tracks_done.begin() + first,
                    records.tracks_done.begin() + first + tracks_per_tour,
                    [](std::uint8_t done) { return done != 0; }))
        complete_tour(state);
}

// $80:C24C and `$80:C206`: after a one-run result's fade, both pads released, then a press seen
// on two frames running.
void one_run_wait_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    auto& result = state.race_result;
    step_decorations(state, content);
    if (!result.released) { // $80:C24C
        result.released = !pressed(pads);
        if (result.released) state.decorations.delay = 3; // $80:98B3
        return;
    }
    const bool press = pressed(pads); // $80:C206: a press on two frames running
    if (!result.press_seen || !press) {
        result.press_seen = press;
        return;
    }
    hide_result_objects(state);
    state.screen = FrontEndScreen::race_result_exit;
}

// $80:98B3 on the lap result's last fade frame: `$0089` = 3, then the graph's first pass.
void start_lap_graph(FrontEndState& state, const FrontEndContent& content) {
    state.decorations.delay = 3;
    step_decorations(state, content);
    step_lap_graph(state);
}

// $80:98C9-997B: the lap graph until a press on pad 1 (`$80:B6D3`); then `$80:9805` (the graph
// cleared bit 8, so it parks) and `$80:F4B8`. `hide_result_objects` also makes `$80:C236`'s
// writes, which the lap result does not, but `$80:9805`'s cover them: the OAM buffer is the same.
void lap_graph_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    if (lap_graph_left(pads)) {
        hide_result_objects(state);
        state.screen = FrontEndScreen::race_result_exit;
        return;
    }
    step_decorations(state, content);
    step_lap_graph(state);
}

} // namespace

void race_return_frame(FrontEndState& state, const FrontEndContent& content) {
    switch (state.script_frame) {
    case menu_screen_frame: load_main_menu_screen(state, content); return;
    case objects_frame: lay_out_menu_objects(state); return;
    case restore_frame:
        copy_oam(state); // $80:D372; NMI on (`$80:D377`)
        state.cycle.running = true;
        restore_menus(state);
        send_arrow_off(state);
        return;
    case tiles_frame: {
        copy_oam(state);
        const auto tiles = asset(content, badge_tiles_asset);
        load_vram(state, tiles.last(badge_tiles_bytes), badge_tiles_word); // $80:A82B
        return;
    }
    case palette_low_frame:
        copy_oam(state);
        load_cgram(state, asset(content, base_palette_low), 0);
        return;
    case palette_high_frame:
        copy_oam(state);
        load_cgram(state, asset(content, base_palette_high), 0x40);
        // $83:89BA: the text halves and BG2's offsets back to the start. `$80:9AAC` restores
        // `$77:0742`, whose bit 1 (the logo held up, `$80:F53F`) was set before the race.
        state.slide.hidden_half = 0x1400;
        state.slide.shown_half = 0x1000;
        state.slide.scroll = 0;
        state.registers.bg[1].hofs = state.registers.bg[1].vofs = 0;
        state.logo.raised = true;
        // $80:88DD: a restart from the race's pause menu (0xEA62) skips the result.
        if (state.race_result.times.player_total == restarted) {
            state.text.words.fill(cleared_text); // $80:88F8
            state.screen = FrontEndScreen::race_restart;
            return;
        }
        state.screen = FrontEndScreen::race_result;
        return;
    default: return; // the sound program's upload and `$80:D20E`'s frames
    }
}

void race_restart_frame(FrontEndState& state) {
    const auto frame = state.script_frame;
    if (frame == 1) { // $80:8903-890A: the logo up, the blank text shown
        state.logo.raised = true;
        state.logo.offset = 0x52;
        state.registers.bg[0].vofs = 0x52;
        load_text(state, state.slide.shown_half);
        return;
    }
    copy_oam(state); // $80:9869: seven frames, brightness 2 to 14
    constexpr std::uint32_t last_fade_frame = 8;
    state.registers.brightness = static_cast<std::uint8_t>(2 * (frame - 1));
    state.registers.force_blank = false;
    if (frame == last_fade_frame) enter_now_playing(state); // $80:BC36-BC45
}

void race_result_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    auto& result = state.race_result;
    const auto frame = state.script_frame;
    if (frame == 1) {
        start_result(state, content);
        return;
    }
    if (frame == rows_frame) {
        if (result.times.lap_race)
            print_lap_result(state, content);
        else
            print_result(state, content);
        return;
    }
    if (frame == icons_frame) {
        show_icons(state, content);
        return;
    }
    copy_oam(state);
    const auto last_fade_frame = first_fade_frame + fade_frames - 1;
    if (frame <= last_fade_frame) {
        state.registers.brightness = static_cast<std::uint8_t>(2 * (frame - first_fade_frame + 1));
        state.registers.force_blank = false;
        if (frame == last_fade_frame && result.times.lap_race) start_lap_graph(state, content);
        return;
    }
    if (result.times.lap_race)
        lap_graph_frame(state, content, pads);
    else
        one_run_wait_frame(state, content, pads);
}

void race_result_exit_frame(FrontEndState& state, const FrontEndContent& content,
                            FrontEndPads pads) {
    constexpr unsigned menu_text_palette = 28;
    const auto frame = state.script_frame;
    const auto scoring = result_scoring_frame(state);
    if (frame == 1) { // $80:F4B8's wait, then `$80:C786` and `$77:1073`
        copy_oam(state);
        load_object_palette(state, content);
        load_cgram(state, asset(content, menu_text_palette), 0xd0);
        state.registers.obsel = 0x63;
        update_records(state);
        state.records.tries = 3;
        return;
    }
    if (frame < scoring) return; // `$80:C786`'s overrun: no wait, no OAM copy
    copy_oam(state);
    if (frame == scoring) { // $83:879A after `$83:A923`'s wait: `$80:D1E8` (the pads), the scoring
        score_race(state, pads);
        return;
    }
    if (frame == scoring + 1) { // $80:A858, twice
        load_cgram(state, asset(content, base_palette_low), 0);
        return;
    }
    load_cgram(state, asset(content, base_palette_high), 0x40);
    enter_track_menu(state, state.track_menu.returning);
}

void begin_race_return(FrontEndState& state, const FrontEndContent& content, std::uint32_t frame,
                       const RaceTimes& times) {
    if (state.screen != FrontEndScreen::race)
        throw std::logic_error("the front end is not waiting for a race");
    state.mode_chosen = false;
    state.frame = frame;
    state.race_result = {};
    state.race_result.times = times;
    // $83:CE2C set the rider's bit when the race's hints ended.
    if (times.tutorial_hints_over)
        state.records.tutorial_bits = static_cast<std::uint16_t>(state.records.tutorial_bits
                                                                 | (1U << state.rider_menu.rider));
    state.screen = FrontEndScreen::race_return;
    state.script_frame = 0;
    early_loads(state, content);
    ++state.frame;
}

} // namespace unirally::front_end_screens

namespace unirally {

void return_from_race(FrontEndState& state, const FrontEndContent& content, std::uint32_t frame,
                      const RaceTimes& times) {
    front_end_screens::begin_race_return(state, content, frame, times);
}

ClassicRaceScenario one_player_race_scenario(const FrontEndState& state) {
    const auto rider = state.rider_menu.rider;
    return classic_race_scenario(ClassicRaceTrack{state.tour_menu.track},
                                 {rider, state.now_playing.opponent},
                                 ((state.records.tutorial_bits >> rider) & 1U) == 0);
}

std::uint32_t race_loading_frames(ClassicRaceTrack track) {
    if (track == ClassicRaceTrack::Dragster) return 121;
    if (track == ClassicRaceTrack::ZoomZoo) return 169;
    return 0;
}

RaceTimes race_times(const ZoomZooState& race) {
    const auto& times = race.race;
    RaceTimes result{times.total_times[0], times.total_times[1],
                     classic_race_scenario(race.track).tour_race};
    if (result.lap_race) {
        result.player_laps = times.lap_times[0];
        result.opponent_laps = times.lap_times[1];
    }
    result.tutorial_hints_over = race.player_announcements.hints_active == 0;
    return result;
}

std::optional<RaceTimes> update_race_for_menus(ZoomZooState& race, const ControllerButtons& buttons,
                                               const ZoomZooContent& content) {
    constexpr std::uint16_t quit = 0xea61, restart = 0xea62;
    auto next = race;
    update_zoom_zoo(next, buttons, content);
    // `restart_zoom_zoo` from the pause menu: a paused race whose count of paused updates, which
    // only a restart clears, is back to 0.
    if (race.pause.suspended_updates != 0 && next.pause.suspended_updates == 0) {
        auto times = race_times(race);
        times.player_total = race.movement.countdown != 0 ? restart : quit; // $83:F8DA-F90B
        return times;
    }
    race = next;
    if (race.result_updates != 1) return std::nullopt;
    return race_times(race);
}

} // namespace unirally
