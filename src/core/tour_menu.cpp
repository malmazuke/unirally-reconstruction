// PICK TOUR, the one-player tour menu (R-0056): after a rider is chosen (`$80:BBF7-BC0B`), the
// base palette and the badge tiles again, the tours printed with a badge each and slid in
// (`$80:E550`, `$80:E730`), the medals laid out (`$80:974B`), and the menu's loop (`$80:E5B4`).
// A tour chosen goes on to PICK TRACK; Y or X slides PICK YOUR UNI back in (`$80:BBA3`).
#include "front_end_screens.hpp"

#include <array>
#include <stdexcept>

namespace unirally::front_end_screens {

namespace {

// The medals' palettes (assets 32-34 at colours 0x80, 0x90 and 0xA0, `$80:9764`); the badge
// tiles `$80:A82B` sends again are asset 68's last 1,920 bytes, at VRAM word 0x3D80.
constexpr unsigned first_medal_palette = 32;
constexpr unsigned badge_tiles_asset = 68, badge_tiles_word = 0x3d80;
constexpr std::size_t badge_tiles_bytes = 0x780;

// The text streams in `front-end.tour-menu-text`, in order: the title and the left column
// ($80:E7C4), jumper and bounder ($80:E7FB), runner and sprinter ($80:E80F), hunter ($80:E824).
enum class TourText : std::uint8_t { left_column, level_one, level_two, hunter };

// The tours: 0-7 in two columns, HUNTER 8; 5 tracks each. A cursor past 9 (a move off the
// grid) reads the table's bytes after its ten entries, 0xFF and code, above any level.
constexpr std::uint8_t tours = 10, hunter_right = 9;
constexpr std::uint8_t max_level_needed = 0xff;
// A badge ($83:8D8F) is 5 x 5 tiles, a row of the picture 0x32 tiles below the last; the
// "?" badge is picture 9.
constexpr unsigned badge_size = 5, badge_row_tiles = 0x32, locked_picture = 9;
constexpr std::uint16_t badge_priority = 0x2000;

// The medal objects, entries 0-8 (tour t is entry t); tile 0xAC until the loop turns them.
constexpr std::uint8_t medal_tile = 0xac;
constexpr unsigned first_medal = 0, medal_groups_end = 12;

// The arrow and its shadow: mirrored for an even cursor, not for HUNTER (`$80:C1EB`).
constexpr unsigned shadow_attribute_entry = shadow_entry;
constexpr std::uint8_t mirror = 0x40;

// The stream with its 0xFF, as the printer takes it.
std::span<const std::uint8_t> tour_text(const FrontEndContent& content, TourText which) {
    const auto text = nth_string(content.tour_menu_text, static_cast<unsigned>(which));
    return {text.data(), text.size() + 1};
}

std::uint8_t tour_level(const FrontEndState& state) { // $83:9F14 in 1P
    return state.records.tour_levels[state.rider_menu.rider & 0x0fU];
}

std::uint8_t& level_of(FrontEndState& state) {
    return state.records.tour_levels[state.rider_menu.rider & 0x0fU];
}

// $80:974B's first lines: the medals' entries shown, small; 9-11 hidden.
void show_medal_entries(FrontEndState& state) {
    high_bits(state, 0) = high_bits(state, 4) = four_shown;
    high_bits(state, 8) = static_cast<std::uint8_t>(four_hidden & ~hidden_bit(8));
}

// $80:E553-E560: with a reveal pending, PICK TOUR is drawn at the level below it first.
void begin_reveal(FrontEndState& state) {
    const auto pending = state.records.pending_reveal;
    if (pending != 0) level_of(state) = static_cast<std::uint8_t>(pending - 1);
}

void draw_badge(FrontEndState& state, const FrontEndContent& content, unsigned tour) {
    draw_tour_picture(state, content, tour, word_at(content.tour_badge_places, tour * 2) / 2U);
}

// $80:E730's text: the names of the open tours and a badge for each of 0-7 (and HUNTER).
void print_tour_menu(FrontEndState& state, const FrontEndContent& content) {
    auto& menu = state.tour_menu;
    state.text.words.fill(cleared_text); // $83:8B51
    if (!tour_open(state, content, menu.tour)) {
        // `$83:9983` then also rewrites the race's SRAM words (R-0056).
        menu.track = 0;
        menu.tour = 0;
    }
    const auto print = [&](TourText which) {
        print_text(state.text, state.printer, tour_text(content, which), content.character_table);
    };
    const auto level = tour_level(state);
    if (level >= 2) print(TourText::level_two);
    if (level >= 1) print(TourText::level_one);
    print(TourText::left_column);
    for (unsigned tour = hunter; tour-- > 0;) draw_badge(state, content, tour);
    if (level >= 3) {
        print(TourText::hunter);
        draw_badge(state, content, hunter);
    }
}

// $80:9782-97D2: a medal beside each badge, in its colours; none without a medal. A hidden entry
// keeps the attribute it had.
void lay_out_medals(FrontEndState& state, const FrontEndContent& content) {
    const auto rider = state.rider_menu.rider;
    for (unsigned tour = tours - 1; tour-- > 0;) {
        if (tour == hunter && tour_level(state) < 3) {
            set_oam_x_high(state, tour, true); // $83:961E
            continue;
        }
        oam_byte(state, tour, 0) = content.medal_places[tour * 4];
        oam_byte(state, tour, 1) = content.medal_places[tour * 4 + 1];
        oam_byte(state, tour, 2) = medal_tile;
        const auto medal = state.records.medals[tour * 16U + rider];
        if (medal == 0)
            set_oam_x_high(state, tour, true); // $83:961E
        else
            oam_byte(state, tour, 3) = content.medal_attributes[medal];
    }
}

// $80:E63D, $80:E6C2: the medals' entries, 0-11, hidden.
void hide_medals(FrontEndState& state) {
    for (unsigned entry = first_medal; entry < medal_groups_end; entry += 4)
        high_bits(state, entry) = four_hidden;
}

// $80:E5CC-E5F1, and `$80:C1EB` to clear it: the arrow and its shadow mirrored.
void set_arrow_mirror(FrontEndState& state, bool mirrored) {
    for (const unsigned entry : {arrow_entry, shadow_attribute_entry}) {
        auto& attributes = oam_byte(state, entry, 3);
        attributes =
            static_cast<std::uint8_t>(mirrored ? attributes | mirror : attributes & ~mirror);
    }
}

// $80:E5B4-E5FD, before each frame wait: the arrow's targets and mirror, and the medals' tile
// (turned by the decoration animator's `$0191`).
void aim_tour_arrow(FrontEndState& state, const FrontEndContent& content) {
    const auto cursor = state.tour_menu.cursor;
    state.arrow.target_x = word_at(content.tour_arrow_targets, cursor * 4U);
    state.arrow.target_y = word_at(content.tour_arrow_targets, cursor * 4U + 2);
    set_arrow_mirror(state, cursor % 2 == 0 && cursor != hunter);
    const auto tile = content.decoration_frames[trio_tiles + state.decorations.trio_step];
    for (unsigned tour = 0; tour < tours - 1; ++tour) oam_byte(state, tour, 2) = tile;
}

// $80:E60A-E62A: Left and Right toggle the column (Right only to an open tour); Up and Down move
// a row once a press, to an open tour. None ends the pass.
void move_tour_cursor(FrontEndState& state, const FrontEndContent& content, std::uint16_t pad) {
    auto& cursor = state.tour_menu.cursor;
    auto& latches = state.latches;
    if ((pad & pad_left) && cursor % 2 == 1) --cursor;
    if ((pad & pad_right) && cursor % 2 == 0 && tour_open(state, content, cursor + 1U)) ++cursor;
    if (!(pad & pad_up)) {
        latches.up = false;
    } else if (!latches.up) {
        latches.up = true;
        const auto to = static_cast<std::uint8_t>(cursor - 2);
        if (tour_open(state, content, to)) cursor = to;
    }
    if (!(pad & (pad_down | pad_select))) {
        latches.down = false;
    } else if (!latches.down) {
        latches.down = true;
        const auto to = static_cast<std::uint8_t>(cursor + 2);
        if (tour_open(state, content, to)) cursor = to;
    }
}

} // namespace

std::uint16_t word_at(std::span<const std::uint8_t> table, std::size_t at) {
    return static_cast<std::uint16_t>(table[at] | (static_cast<unsigned>(table[at + 1]) << 8U));
}

bool tour_open(const FrontEndState& state, const FrontEndContent& content, unsigned cursor) {
    const unsigned index = cursor & 0x0fU;
    const auto needed = index < tours ? content.tour_levels[index] : max_level_needed;
    return tour_level(state) >= needed;
}

void draw_tour_picture(FrontEndState& state, const FrontEndContent& content, unsigned tour,
                       unsigned place) {
    const unsigned picture = tour_open(state, content, tour) ? tour : locked_picture;
    const auto palette = static_cast<unsigned>(content.tour_badge_pictures[picture]);
    const unsigned first_tile = word_at(content.tour_badge_pictures, tours + picture * 2);
    for (unsigned row = 0; row < badge_size; ++row)
        for (unsigned column = 0; column < badge_size; ++column)
            state.text.words[(place + row * 32 + column) & 1023U] = static_cast<std::uint16_t>(
                (first_tile + row * badge_row_tiles + column) | (palette << 10U) | badge_priority);
}

void enter_tour_menu(FrontEndState& state) {
    begin_reveal(state);
    state.tour_menu.returning = false;
    state.tour_menu.slides_back = false;
    state.screen = FrontEndScreen::tour_menu_entry;
}

void return_to_tour_menu(FrontEndState& state, bool slides_back) {
    // `$80:BC03`: PICK TOUR from `$80:E550`, without `$80:A858` and `$80:A82B` before it; its
    // first lines round the track now (`$83:893C`).
    begin_reveal(state);
    auto& menu = state.tour_menu;
    menu.returning = true;
    menu.slides_back = slides_back;
    menu.track = static_cast<std::uint8_t>(menu.track / tracks_per_tour * tracks_per_tour);
    state.screen = FrontEndScreen::tour_menu_entry;
}

// A reveal (`$80:E580-E5A2`) redraws the screen after the slide at the new level: the medal
// tiles, `$80:A858`'s two frames with the tours printed, and the text sent to the shown half; the
// rest of the entry follows four frames later (R-0062). False past the four frames.
constexpr std::uint32_t reveal_frames = 4;
bool reveal_frame(FrontEndState& state, const FrontEndContent& content, std::uint32_t step) {
    switch (step) {
    case 0: load_vram(state, content.medal_tiles, swapped_object_tiles_word); return true;
    case 1:
        copy_oam(state);
        load_cgram(state, asset(content, base_palette_low), 0);
        return true;
    case 2:
        copy_oam(state);
        load_cgram(state, asset(content, base_palette_high), 0x40);
        print_tour_menu(state, content);
        return true;
    case 3:
        copy_oam(state);
        load_text(state, state.slide.shown_half);
        show_medal_entries(state);
        return true;
    default: return false;
    }
}

void tour_menu_entry_frame(FrontEndState& state, const FrontEndContent& content) {
    constexpr std::uint32_t first_slide_frame = 8, medal_palettes_frame = 47, medals_frame = 48,
                            loop_frame = 49;
    auto& menu = state.tour_menu;
    // From PICK TRACK the script starts at the medal tiles, its fourth frame.
    constexpr std::uint32_t returning_skips = 3;
    auto frame = state.script_frame + (menu.returning ? returning_skips : 0);
    // A reveal's four frames come after the slide; the rest of the entry follows them.
    if (menu.revealing && frame >= medal_palettes_frame) {
        if (reveal_frame(state, content, frame - medal_palettes_frame)) return;
        frame -= reveal_frames;
    }
    switch (frame) {
    case 1:
    case 5: // $80:A858, twice
        copy_oam(state);
        load_cgram(state, asset(content, base_palette_low), 0);
        return;
    case 2:
        copy_oam(state);
        load_cgram(state, asset(content, base_palette_high), 0x40);
        return;
    case 3: {
        copy_oam(state);
        const auto tiles = asset(content, badge_tiles_asset);
        if (tiles.size() < badge_tiles_bytes)
            throw std::invalid_argument("front-end asset 68 is shorter than its badge tiles");
        load_vram(state, tiles.last(badge_tiles_bytes), badge_tiles_word); // $80:A82B
        menu.track = static_cast<std::uint8_t>(menu.track / tracks_per_tour * tracks_per_tour);
        return;
    }
    case 4: load_vram(state, content.medal_tiles, swapped_object_tiles_word); return; // no OAM copy
    case 6:
        copy_oam(state);
        load_cgram(state, asset(content, base_palette_high), 0x40);
        print_tour_menu(state, content);
        return;
    case 7:
        copy_oam(state);
        load_text(state, state.slide.hidden_half);
        start_slide(state, content, menu.slides_back);
        return;
    case medal_palettes_frame:
        for (unsigned k = 0; k < 3; ++k)
            load_cgram(state, asset(content, first_medal_palette + k), 0x80 + k * 16);
        return;
    case medals_frame: lay_out_medals(state, content); return;
    case loop_frame:
        copy_oam(state);
        menu.revealing = false;
        menu.cursor = menu.tour;
        aim_tour_arrow(state, content);
        state.screen = FrontEndScreen::tour_menu;
        return;
    default:
        if (state.script_frame + (menu.returning ? returning_skips : 0) < first_slide_frame
            || !slide_frame(state, content))
            return;
        if (auto& pending = state.records.pending_reveal; pending != 0) {
            level_of(state) = pending; // $80:E580-E58E: the level revealed, nothing pending
            pending = 0;
            menu.revealing = true;
            return;
        }
        show_medal_entries(state);
        return;
    }
}

void tour_menu_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    auto& menu = state.tour_menu;
    copy_oam(state); // $80:D1EC
    step_decorations(state, content);
    move_tour_cursor(state, content, pads.one);
    const bool choose = (pads.one & choose_buttons) != 0;
    const bool back = (pads.one & back_buttons) != 0;
    if (!choose && !back) {
        aim_tour_arrow(state, content);
        return;
    }
    set_arrow_mirror(state, false);
    hide_medals(state);
    if (choose) { // $80:E6B1-E6DC
        if (menu.cursor == hunter_right) menu.cursor = hunter;
        menu.tour = menu.cursor;
        menu.medal = state.records.medals[menu.tour * 16U + state.rider_menu.rider];
    }
    // From a completion PICK TOUR returns into the scoring (`$83:88D1`), which goes on to PICK
    // TRACK without testing Y or X.
    if (state.award.after_completion) {
        state.award.after_completion = false;
        state.screen = FrontEndScreen::award_return;
        return;
    }
    // The handler tests Y and X again after a choice (`$80:BC12`), so back wins.
    if (back) {
        menu.back = true;
        return_to_rider_menu(state);
        return;
    }
    menu.back = false;
    enter_track_menu(state, false);
}

} // namespace unirally::front_end_screens
