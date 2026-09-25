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
// program's upload takes 74 frames; `$80:D20E` then as at the boot (377-403); the text tiles,
// the palettes, and the result's build (R-0057's timeline).
constexpr std::uint32_t menu_screen_frame = 75, objects_frame = 100, restore_frame = 101,
                        tiles_frame = 102, palette_low_frame = 103, palette_high_frame = 104;
constexpr unsigned early_palette = 5, early_objects = 88; // $80:A09A
constexpr unsigned badge_tiles_asset = 68, badge_tiles_word = 0x3d80;
constexpr std::size_t badge_tiles_bytes = 0x780;

// The result screen's frames from its first, `$83:94D0`'s wait: the rows, the icons and the
// text, then the fade (`$80:9869`, brightness 2, 4, ..., 14).
constexpr std::uint32_t rows_frame = 2, icons_frame = 3, first_fade_frame = 4, fade_frames = 7;
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
constexpr std::uint8_t someone = 0x10;

// Pads that leave the result: any button on either pad (`$80:C206`).
bool pressed(FrontEndPads pads) {
    return pads.one != 0 || pads.two != 0;
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
    const auto& totals = state.race_result.totals;
    const bool computer = state.now_playing.opponent >= someone;
    std::array<Row, 8> rows{{{RowKind::player, totals.player_total},
                             {RowKind::opponent, computer ? no_row : totals.opponent_total},
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
            auto& best = records.best[rider * 50U + track];
            if (row.time < best) {
                best = row.time;
                high_bits(state, 96) &= 0xee; // the new-best markers, entries 96 and 98
            }
            const auto y = static_cast<std::uint8_t>(6 * printed * 4 + 0x58);
            for (const unsigned entry : {96U, 98U, 100U, 102U}) oam_byte(state, entry, 1) = y;
            for (const unsigned entry : {100U, 102U})
                oam_byte(state, entry, 3) = static_cast<std::uint8_t>(0x11 | (printed * 2));
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

// $80:951C-9555 and $80:CE90-CF4E, on the result's first frame.
void start_result(FrontEndState& state, const FrontEndContent& content) {
    load_vram(state, content.medal_tiles, swapped_object_tiles_word); // $83:94D0
    load_cgram(state, asset(content, first_rider_palette + state.rider_menu.rider),
               first_rider_palette_colour);
    load_cgram(state, asset(content, first_rider_palette + state.now_playing.opponent),
               opponent_palette_colour);
    for (const unsigned entry : {30U, 31U}) oam_byte(state, entry, 2) = 0xc0;
    oam_byte(state, 30, 3) = 0x21;
    oam_byte(state, 31, 3) = 0x23;
    high_bits(state, 28) = 0xf5;
    // $80:F53F: the logo held up.
    state.logo.raised = true;
    state.logo.offset = 0x52;
    state.registers.bg[0].vofs = 0x52;
    state.text.words.fill(cleared_text);
    // The icons beside the rows, entries 104-108, and the new-best markers 96-99 (hidden).
    constexpr std::array<std::uint8_t, 5> icon_rows{0x57, 0x6f, 0x87, 0x9f, 0xb7};
    for (unsigned k = 0; k < icon_rows.size(); ++k) {
        oam_byte(state, 104 + k, 0) = 0x78;
        oam_byte(state, 104 + k, 1) = icon_rows[k];
    }
    high_bits(state, 104) = four_shown;
    high_bits(state, 108) = static_cast<std::uint8_t>(four_hidden & ~hidden_bit(108));
    for (unsigned entry = 96; entry < 100; ++entry) {
        oam_byte(state, entry, 0) = 0xdb;
        oam_byte(state, entry, 3) = 0x1b;
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
    for (const unsigned entry : {101U, 103U}) oam_byte(state, entry, 1) = 0xef;
    high_bits(state, 100) = four_shown;
}

// $80:D113-D13C and the tail `$80:9579-958F`.
void show_icons(FrontEndState& state, const FrontEndContent& content) {
    for (unsigned k = 0; k < 3; ++k)
        load_cgram(state, asset(content, trophy_palettes_first - k), 0xd0 + k * 16);
    for (std::size_t k = 0; k < content.result_icons.size(); ++k)
        state.oam_buffer[112 * 4 + k] = content.result_icons[k];
    high_bits(state, 112) = hidden_bit(115);
    load_text(state, state.slide.shown_half); // $80:93A5 to $00AA
    state.decorations.delay = state.decorations.wave_delay = state.decorations.sway = 0;
    step_decorations(state, content);
}

// $80:C236-C245, `$80:9805` and `$80:F4B8`'s first half, on the press's second frame.
void hide_result_objects(FrontEndState& state) {
    high_bits(state, 32) = four_hidden;
    high_bits(state, 28) = static_cast<std::uint8_t>((high_bits(state, 28) & 0x0fU) | 0x50U);
    for (unsigned entry = 0; entry < 30; ++entry)
        oam_byte(state, entry, 0) = oam_byte(state, entry, 1) = 1;
    for (unsigned entry = 0; entry < 32; entry += 4) high_bits(state, entry) = four_hidden;
    for (const unsigned group : {96U, 100U, 104U, 108U, 112U, 120U})
        high_bits(state, group) = four_hidden;
}

// $80:C9D7: a time into the track's top three, shorter strictly first.
void insert_record(OnePlayerRecords& records, unsigned track, std::uint16_t time,
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
        return;
    }
}

// $80:C786-C866 for a one-run race: races and wins by rider, and the times into the records. A
// computer opponent (16 and up) keeps no counts and no records (`$80:CAAA`, `$80:C82F`).
void update_records(FrontEndState& state) {
    auto& records = state.records;
    const auto rider = state.rider_menu.rider, opponent = state.now_playing.opponent;
    const bool rider_opponent = opponent < records.statistics.size();
    const auto& totals = state.race_result.totals;
    const auto player = totals.player_total, other = totals.opponent_total;
    constexpr std::size_t races = 0, wins = 1, no_time_losses = 2;
    constexpr std::uint16_t no_time = 0xea60;
    const auto track = track_of(state);
    // A time goes into the records unless it is no time, which counts a loss without one.
    const auto record = [&](std::uint8_t who, std::uint16_t time) {
        if (time >= no_time)
            ++records.statistics[who][no_time_losses];
        else
            insert_record(records, track, time, who);
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
        insert_record(records, track, player, rider); // $80:C81C
        if (rider_opponent) record(opponent, other);  // $80:C82F
    } else if (player > other) {
        if (rider_opponent) insert_record(records, track, other, opponent); // $80:C850
        record(rider, player);                                              // $80:C800
    } else {
        insert_record(records, track, player, rider); // $80:C7EE's $80:C81C and $80:C850
        if (rider_opponent) insert_record(records, track, other, opponent);
    }
}

// $83:879A for a one-run race: a win (a total strictly under the opponent's) marks the track
// won; a loss sets `$77:0742` bit 12.
void score_race(FrontEndState& state) {
    const auto& totals = state.race_result.totals;
    auto& records = state.records;
    if (totals.player_total >= totals.opponent_total) {
        records.race_lost = true;
        return;
    }
    const auto track = track_of(state);
    records.tracks_done[track & 0x3fU] = 1;
    const auto first = track / tracks_per_tour * tracks_per_tour;
    if (std::all_of(records.tracks_done.begin() + first,
                    records.tracks_done.begin() + first + tracks_per_tour,
                    [](std::uint8_t done) { return done != 0; }))
        throw std::logic_error("a tour's completion (its award and PICK TOUR's reveal) is not "
                               "recovered yet");
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
        state.arrow.target_x = 0xfd00; // $80:98A4
        state.arrow.target_y = 0x0700;
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
        // $83:89BA: the text halves and BG2's offsets back to the start; `$77:0742` restored.
        state.slide.hidden_half = 0x1400;
        state.slide.shown_half = 0x1000;
        state.slide.scroll = 0;
        state.registers.bg[1].hofs = state.registers.bg[1].vofs = 0;
        state.logo.raised = true;
        state.screen = FrontEndScreen::race_result;
        return;
    default: return; // the sound program's upload and `$80:D20E`'s frames
    }
}

void race_result_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    auto& result = state.race_result;
    const auto frame = state.script_frame;
    if (frame == 1) {
        start_result(state, content);
        return;
    }
    if (frame == rows_frame) {
        print_result(state, content);
        return;
    }
    if (frame == icons_frame) {
        show_icons(state, content);
        return;
    }
    copy_oam(state);
    if (frame < first_fade_frame + fade_frames) {
        state.registers.brightness = static_cast<std::uint8_t>(2 * (frame - first_fade_frame + 1));
        state.registers.force_blank = false;
        return;
    }
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

void race_result_exit_frame(FrontEndState& state, const FrontEndContent& content) {
    constexpr std::uint32_t leave_frame = 1, scoring_frame = 2, palette_low = 4, palette_high = 5;
    constexpr unsigned menu_text_palette = 28;
    switch (state.script_frame) {
    case leave_frame: // $80:F4B8's wait, then `$80:C786` and `$77:1073`
        copy_oam(state);
        load_object_palette(state, content);
        load_cgram(state, asset(content, menu_text_palette), 0xd0);
        state.registers.obsel = 0x63;
        update_records(state);
        state.records.tries = 3;
        return;
    case scoring_frame: // $83:879A: a wait without the arrow, then the scoring
        score_race(state);
        return;
    case palette_low:
        copy_oam(state);
        load_cgram(state, asset(content, base_palette_low), 0);
        return;
    case palette_high:
        copy_oam(state);
        load_cgram(state, asset(content, base_palette_high), 0x40);
        enter_track_menu(state, state.track_menu.returning);
        return;
    default: copy_oam(state); return;
    }
}

void begin_race_return(FrontEndState& state, const FrontEndContent& content, std::uint32_t frame,
                       const RaceTotals& totals) {
    if (state.screen != FrontEndScreen::race)
        throw std::logic_error("the front end is not waiting for a race");
    state.mode_chosen = false;
    state.frame = frame;
    state.race_result = {};
    state.race_result.totals = totals;
    state.screen = FrontEndScreen::race_return;
    state.script_frame = 0;
    early_loads(state, content);
    ++state.frame;
}

} // namespace unirally::front_end_screens

namespace unirally {

void return_from_race(FrontEndState& state, const FrontEndContent& content, std::uint32_t frame,
                      const RaceTotals& totals) {
    front_end_screens::begin_race_return(state, content, frame, totals);
}

} // namespace unirally
