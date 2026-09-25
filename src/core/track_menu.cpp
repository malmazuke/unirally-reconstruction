// PICK TRACK, the one-player track menu (R-0056): after a tour is chosen, its five tracks and
// (in 1P) the medal line printed with twelve pictures of the tour and slid in (`$80:E84E`), the
// done-track markers laid out, and the menu's loop (`$80:BAA4`), the generic list menu's. A
// track chosen goes on to NOW PLAYING; Y or X slides PICK TOUR back in.
#include "front_end_screens.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <vector>

namespace unirally::front_end_screens {

namespace {

constexpr unsigned marker_palette_asset = 37, first_rider_palette = 6;
constexpr unsigned object_tiles_word = 0x7a00; // `$83:94FF`, `$83:94D0`
constexpr std::uint8_t tracks_per_tour = 5, medal_line = 5;

// `front-end.track-menu-layout`: the items' arrow columns (8-pixel units), the markers' rows
// (from the last track's up) and the twelve pictures' places (byte offsets in the text map).
constexpr std::size_t item_columns = 0, marker_rows = 6, picture_places = 11, pictures = 12;
// The track rows' variables for the text's F7 codes: `$00B2`, `$00B4`, ... `$00BA`.
constexpr std::uint16_t first_track_word = 0xb2;
// `front-end.medal-words`: GOLD, SILVER, BRONZE, each "FC 17 ... FF".
constexpr std::array<std::size_t, 3> medal_word_at{20, 11, 0}; // by medal: bronze, silver, gold
constexpr std::uint8_t gold = 2;

// The done-track markers, entries 64-68 beside the tracks ($80:E948-E995).
constexpr unsigned first_marker = 64;
constexpr std::uint8_t marker_x = 0xc6, marker_tile = 0xe0, marker_attributes = 0x17;
constexpr std::uint8_t marker_steps = 14, marker_half_cycle = 7;

// The arrow: x the item's column * 128, y (24 * item + 0x48) * 16 ($80:BA6A).
constexpr std::uint16_t row_spacing = 0x0180, wrap_top_y = 0x0300;

// Pad 1 only. Down counts Select ($80:B794); choose: B, Start or A; back: Y or X.
constexpr std::uint16_t up = 0x0800, down_or_select = 0x2400;
constexpr std::uint16_t choose_buttons = 0x9080, back_buttons = 0x4040;

std::uint8_t last_item(const FrontEndState&) {
    return medal_line; // `$009D`: 5 in 1P, where the medal line follows the tracks
}

std::uint16_t item_x(const FrontEndContent& content, unsigned item) {
    const auto column = static_cast<std::int8_t>(content.track_menu_layout[item_columns + item]);
    return static_cast<std::uint16_t>(column * 128);
}

std::uint16_t item_y(unsigned item) {
    return static_cast<std::uint16_t>((24 * item + 0x48) * 16);
}

std::uint8_t tour_of(const FrontEndState& state) {
    return state.tour_menu.tour;
}

// The medal line's word for the medal to race for ($80:E906-E923).
void print_medal(FrontEndState& state, const FrontEndContent& content) {
    const auto medal = state.tour_menu.medal;
    const auto word = content.medal_words.subspan(medal_word_at[medal >= gold ? 2 : medal]);
    print_text(state.text, state.printer, word, content.character_table);
}

// $80:E878-E923: the text map, all in one frame.
void print_track_menu(FrontEndState& state, const FrontEndContent& content) {
    const auto tour = tour_of(state);
    for (unsigned k = pictures; k-- > 0;)
        draw_tour_picture(state, content, tour,
                          word_at(content.track_menu_layout, picture_places + k * 2) / 2U);
    // The tour's name, centred on row 4: `FC 04` and the name from `$83:A1B4` ($80:9BA0).
    std::vector<std::uint8_t> name{0xfc, 0x04};
    std::size_t at = 0;
    for (unsigned k = 0; k < tour; ++k)
        at = static_cast<std::size_t>(
                 std::find(content.tour_names.begin() + static_cast<std::ptrdiff_t>(at),
                           content.tour_names.end(), std::uint8_t{0xff})
                 - content.tour_names.begin())
           + 1;
    for (; at < content.tour_names.size() && content.tour_names[at] != 0xff; ++at)
        name.push_back(content.tour_names[at]);
    name.push_back(0xff);
    print_text(state.text, state.printer, name, content.character_table);
    // The title and the five tracks: `$00B2 + 2i` holds track 5 * tour + i.
    TextVariables variables;
    variables.word = [&](std::uint16_t address) {
        return static_cast<std::uint16_t>(tour * tracks_per_tour
                                          + (address - first_track_word) / 2U);
    };
    variables.track_names = content.track_names;
    print_text(state.text, state.printer, content.track_menu_text, content.character_table,
               &variables);
    print_medal(state, content);
}

// $80:E948-E995: a marker beside each track won in this run.
void lay_out_markers(FrontEndState& state, const FrontEndContent& content) {
    for (unsigned k = 0; k < tracks_per_tour; ++k) {
        const auto entry = first_marker + k;
        oam_byte(state, entry, 0) = marker_x;
        oam_byte(state, entry, 1) = content.track_menu_layout[marker_rows + 4 - k];
        oam_byte(state, entry, 2) = marker_tile;
        oam_byte(state, entry, 3) = marker_attributes;
    }
    state.oam_buffer[oam_high_table + 16] = 0x00;
    state.oam_buffer[oam_high_table + 17] = 0x54;
    const unsigned first = tour_of(state) * tracks_per_tour;
    for (unsigned k = tracks_per_tour; k-- > 0;)
        if (!state.records.tracks_done[first + k])
            state.oam_buffer[oam_high_table + (first_marker + k) / 4] |=
                static_cast<std::uint8_t>(1U << (((first_marker + k) % 4) * 2));
}

// $80:EB23, before each frame wait: the markers turn, entries 64, 66 and 68 half a cycle from
// 65 and 67.
void turn_markers(FrontEndState& state, const FrontEndContent& content) {
    auto& step = state.track_menu.marker_step;
    step = step == 0 ? marker_steps - 1 : static_cast<std::uint8_t>(step - 1);
    for (const unsigned entry : {64U, 66U, 68U})
        oam_byte(state, entry, 2) = content.marker_tiles[step];
    for (const unsigned entry : {65U, 67U})
        oam_byte(state, entry, 2) = content.marker_tiles[step + marker_half_cycle];
}

// $80:E9AA-E9CC and $80:BA6A: the arrow on the chosen track, or the next one not yet won.
void open_track_menu(FrontEndState& state, const FrontEndContent& content) {
    auto& menu = state.track_menu;
    unsigned track = state.tour_menu.track, item = track % tracks_per_tour;
    while (state.records.tracks_done[track]) {
        ++track;
        if (++item == tracks_per_tour) {
            track = tour_of(state) * tracks_per_tour;
            item = 0;
        }
    }
    menu.cursor = static_cast<std::uint8_t>(item);
    state.latches = {};
    state.arrow.target_x = item_x(content, item);
    state.arrow.target_y = item_y(item);
    turn_markers(state, content);
    state.screen = FrontEndScreen::track_menu;
}

// $80:EA8F: a choice on the medal line steps the medal to race for, up to the rider's best on
// the tour, once a press; not on HUNTER or in a run already begun. True when it stepped.
bool step_medal(FrontEndState& state, const FrontEndContent& content) {
    auto& tour = state.tour_menu;
    constexpr std::uint8_t hunter = 8;
    if (tour.tour == hunter || state.track_menu.medal_latched) return false;
    for (unsigned k = 0; k < tracks_per_tour; ++k)
        if (state.records.tracks_done[tour.tour * tracks_per_tour + k]) return false;
    const auto best = state.records.medals[tour.tour * 16U + state.rider_menu.rider];
    auto next = static_cast<std::uint8_t>(tour.medal + 1);
    if (next > gold || next > best) next = 0;
    tour.medal = next;
    print_medal(state, content);
    return true;
}

// $80:BAC4-BB90: Down (or Select) and Up move once a press, wrapping past either end.
void move_track_cursor(FrontEndState& state, const FrontEndContent& content, std::uint16_t pad) {
    auto& cursor = state.track_menu.cursor;
    const bool down = (pad & down_or_select) != 0, is_up = !down && (pad & up) != 0;
    if (!down && !is_up) {
        state.latches = {};
        return;
    }
    if (state.latches.moved) return;
    state.latches = {.moved = true};
    if (down) {
        if (++cursor > last_item(state)) {
            cursor = 0;
            state.arrow.target_y = wrap_top_y;
        }
        state.arrow.target_y = static_cast<std::uint16_t>(state.arrow.target_y + row_spacing);
    } else {
        if (cursor-- == 0) {
            cursor = last_item(state);
            state.arrow.target_y = item_y(cursor + 1U);
        }
        state.arrow.target_y = static_cast<std::uint16_t>(state.arrow.target_y - row_spacing);
    }
    state.arrow.target_x = item_x(content, cursor);
}

// $80:9983: the race the track sets (laps and kind are read from the track by the screens).
void choose_track(FrontEndState& state) {
    state.tour_menu.track =
        static_cast<std::uint8_t>(tour_of(state) * tracks_per_tour + state.track_menu.cursor);
}

} // namespace

void enter_track_menu(FrontEndState& state, bool returning) {
    // $80:E84F-E876.
    auto& menu = state.track_menu;
    menu.returning = returning;
    menu.medal_latched = false;
    menu.marker_step = 0;
    for (std::size_t k = 0; k < 6; ++k) state.oam_buffer[oam_high_table + k] = 0x55;
    state.screen = FrontEndScreen::track_menu_entry;
}

void track_menu_entry_frame(FrontEndState& state, const FrontEndContent& content) {
    switch (state.script_frame) {
    case 1:
        copy_oam(state);
        state.text.words.fill(cleared_text); // $83:8B51
        return;
    case 2: // `$83:94FF`'s frame wait copies no OAM
        load_vram(state, content.track_menu_tiles, object_tiles_word);
        load_cgram(state, asset(content, marker_palette_asset), 0xb0);
        load_cgram(state, asset(content, first_rider_palette + state.rider_menu.rider), 0x80);
        print_track_menu(state, content);
        return;
    case 3:
        copy_oam(state);
        load_text(state, state.slide.hidden_half);
        start_slide(state, content, state.track_menu.returning);
        return;
    default:
        if (!slide_frame(state, content)) return;
        lay_out_markers(state, content);
        open_track_menu(state, content);
        return;
    }
}

void track_menu_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    auto& menu = state.track_menu;
    if (menu.medal_stepped) { // the rest of $80:EA8F
        menu.medal_stepped = false;
        copy_oam(state);
        load_text(state, state.slide.shown_half);
        menu.medal_latched = true;
        turn_markers(state, content);
        return;
    }
    copy_oam(state); // $80:D1EC
    const auto pad = pads.one;
    menu.back = (pad & back_buttons) != 0;
    if (menu.back || (pad & choose_buttons)) {
        if (!menu.back && menu.cursor == medal_line) {
            if (step_medal(state, content)) {
                menu.medal_stepped = true;
                return;
            }
            turn_markers(state, content);
            return;
        }
        if (!menu.back) choose_track(state);
        // $80:E9FC: the markers hidden.
        for (std::size_t k = 16; k < 20; ++k) state.oam_buffer[oam_high_table + k] = 0x55;
        state.screen = FrontEndScreen::track_menu_exit;
        return;
    }
    menu.medal_latched = false;
    move_track_cursor(state, content, pad);
    turn_markers(state, content);
}

void track_menu_exit_frame(FrontEndState& state, const FrontEndContent& content) {
    if (state.script_frame == 1) {
        copy_oam(state);
        return;
    }
    // `$83:94D0`: the usual object tiles back, without an OAM copy.
    constexpr std::size_t medal_tiles_word = 0x7a00;
    load_vram(state, content.medal_tiles, medal_tiles_word);
    if (state.track_menu.back) {
        // `$80:BC22-BC34` would clear the run's tracks when the medal to race for is not the
        // best; a cold start's are both 0.
        return_to_tour_menu(state);
        return;
    }
    enter_now_playing(state);
}

} // namespace unirally::front_end_screens
