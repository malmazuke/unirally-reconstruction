// PICK YOUR UNI, the one-player rider menu (R-0055): 1P chosen on the main menu (`$80:BB9C`),
// the names printed and slid in over the main menu's text (`$80:CB04`, `$80:E233`), the menu's
// loop (`$80:CBC8`), and the way out (`$80:F4E9`): a rider chosen, or Y back to the main menu
// (`$80:BC9B`, `$80:ACD5`, `$80:E27E`). Like the boot, the steps between screens take fixed
// frames, counted from the frame each script starts.
#include "front_end_screens.hpp"

#include <array>
#include <utility>

namespace unirally::front_end_screens {

namespace {

// The riders' sprite palettes: rider r's is asset 6 + r (R-0055).
constexpr unsigned first_rider_palette = 6;
// What `$80:F4E9` loads on the way out ($83:91F7, $80:F502-F510): the chosen rider's palette, or
// asset 2 after Y, at colour 0xF0; assets 28 and 5 at 0xD0 and 0xE0.
constexpr unsigned no_rider_palette = 2, menu_text_palette = 28, early_palette = 5;

// The rider menu's text ($80:95AB): each name at column 5 (even riders) or 19 (odd), text row
// 4 + 3 * row; then the title.
constexpr std::uint8_t left_name_column = 5, right_name_column = 19, first_name_row = 4,
                       name_rows = 3;
constexpr std::size_t name_record = 16, riders = 16;
constexpr std::uint8_t text_position = 0xfe, end_of_text = 0xff;

// The uni picture ($83:8E3A, $80:F818): five rows of six tiles at VRAM word 0x7000, a row every
// 0x100 words. It builds up over pictures 0x12EE-0x130A (the intro counter from 0x25DC, a picture
// every two frames), then loops through 0x1320 down to 0x130D (`$80:CD5C`, by idle step / 2).
constexpr unsigned uni_tiles_word = 0x7000, uni_row_words = 0x100, tile_words = 0x10;
constexpr std::uint16_t intro_start = 0x25dc, intro_end = 0x2616, first_loop_picture = 0x130d;
constexpr std::uint8_t last_idle_step = 0x27;

// The arrow's targets: x by column, y (24 * row + 41) * 16 ($80:CB62-CBBB).
constexpr std::uint16_t left_column_x = 0x0680, right_column_x = 0x0780;
constexpr std::uint16_t row_spacing = 0x0180, first_row_y = 0x0290;
constexpr std::uint16_t off_screen_x = 0xfd00, off_screen_y = 0x0700; // $80:98A4
// The arrow's attributes ($0BDF): priority 3, palette rider & 7 (bits 3-1), mirrored in the
// left column so that it points at the name.
constexpr std::uint8_t arrow_priority = 0x30, arrow_mirror = 0x40;
// The high table byte of entries 124-127, set every pass: the arrow's shadow stays hidden.
constexpr std::size_t last_high_byte = oam_high_table + 31;
constexpr std::uint8_t shadow_hidden = 0x55;

// Pad 1 only ($83:9543). Choose: B, Start or A ($80:B71D); back: Y or X ($80:B74A).
constexpr std::uint16_t choose_buttons = 0x9080, back_buttons = 0x4040;
constexpr std::uint16_t up = 0x0800, down = 0x0400, left = 0x0200, right = 0x0100;

// The HDMA of $80:CC1A-CC47: colour 0 black from the top; from row 64 one colour a row through
// colours 0x80-0xFF, so that palette p holds rider p + 8's colours on rows 64 + 16p on.
constexpr std::uint8_t split_row = 64;

// $80:95AB, then the title: the names of the records in the text map, in the printer's current
// attribute (the main menu's).
void print_rider_menu(FrontEndState& state, const FrontEndContent& content) {
    state.text.words.fill(cleared_text); // $83:8B51
    for (const bool right_column : {false, true})
        for (std::size_t row = 0; row < riders / 2; ++row) {
            const auto rider = row * 2 + (right_column ? 1 : 0);
            const std::array<std::uint8_t, 4> place{
                text_position, right_column ? right_name_column : left_name_column,
                static_cast<std::uint8_t>(first_name_row + name_rows * row), end_of_text};
            print_text(state.text, state.printer, place, content.character_table);
            const auto name = content.rider_names.subspan(rider * name_record, name_record);
            print_text(state.text, state.printer, name, content.character_table);
        }
    print_text(state.text, state.printer, content.rider_menu_title, content.character_table);
}

// $80:F818: the picture built on the last pass into the object tiles.
void upload_uni(FrontEndState& state, const FrontEndContent& content) {
    const auto cells = pose_frame_cells(content.uni_pictures, state.rider_menu.picture);
    for (std::size_t row = 0; row < pose_frame_rows; ++row)
        for (std::size_t column = 0; column < pose_frame_columns; ++column) {
            const auto word = cells[row * pose_frame_columns + column];
            load_vram(
                state, rider_tile_bytes(content.uni_pictures, word),
                static_cast<unsigned>(uni_tiles_word + row * uni_row_words + column * tile_words));
        }
}

// $80:CBC8-CBFB: the next picture, the intro's while it runs, then the loop's.
void choose_next_picture(RiderMenu& menu) {
    if (menu.intro != 0) {
        if (++menu.intro < intro_end) {
            menu.picture = static_cast<std::uint16_t>(menu.intro >> 1U);
            return;
        }
        menu.intro = 0;
        menu.idle_step = 0;
    }
    step_down(menu.idle_step, last_idle_step);
    menu.picture = static_cast<std::uint16_t>(first_loop_picture + (menu.idle_step >> 1U));
}

// $80:9625: rider r's icon is six objects, the same uni picture in its own palette (r & 7),
// mirrored in the left column. Rider 15 takes entries 0-5, rider 0 entries 90-95.
void lay_out_icons(FrontEndState& state) {
    struct Piece {
        std::uint8_t tile, left_x, right_x, y;
    };
    constexpr std::array<Piece, 6> pieces{{{0x00, 0x10, 0xd0, 16},
                                           {0x04, 0x00, 0xf0, 16},
                                           {0x24, 0x00, 0xf0, 32},
                                           {0x40, 0x20, 0xd0, 48},
                                           {0x42, 0x10, 0xe0, 48},
                                           {0x44, 0x00, 0xf0, 48}}};
    constexpr std::uint8_t icon_attributes = 0x31; // priority 3, the second name table
    constexpr std::array<std::uint8_t, 3> high_bits{0x02, 0x20, 0x00}; // the first piece large
    for (std::size_t rider = 0; rider < riders; ++rider) {
        const auto first = static_cast<unsigned>((riders - 1 - rider) * pieces.size());
        const bool left_column = rider % 2 == 0;
        const auto row = static_cast<unsigned>(rider / 2);
        for (std::size_t k = 0; k < pieces.size(); ++k) {
            const auto& piece = pieces[k];
            const auto entry = first + static_cast<unsigned>(k);
            oam_byte(state, entry, 0) = left_column ? piece.left_x : piece.right_x;
            oam_byte(state, entry, 1) = static_cast<std::uint8_t>(24 * row + piece.y);
            oam_byte(state, entry, 2) = piece.tile;
            oam_byte(state, entry, 3) = static_cast<std::uint8_t>(
                icon_attributes | ((rider & 7U) << 1U) | (left_column ? arrow_mirror : 0));
        }
    }
    for (std::size_t k = 0; k < 24; ++k) state.oam_buffer[oam_high_table + k] = high_bits[k % 3];
    state.registers.obsel = 0x63;
}

std::uint8_t& arrow_attribute(FrontEndState& state) {
    return oam_byte(state, arrow_entry, 3);
}

void set_arrow_row_palette(FrontEndState& state) {
    auto& attributes = arrow_attribute(state);
    attributes =
        static_cast<std::uint8_t>((attributes & 0xf3U) | ((state.rider_menu.row & 3U) << 2U));
}

// $80:CB50-CBC6, at the end of the slide: the icons, and the arrow on the last rider chosen.
void open_rider_menu(FrontEndState& state) {
    auto& menu = state.rider_menu;
    state.oam_buffer[last_high_byte] = shadow_hidden;
    lay_out_icons(state);
    const bool right_column = (menu.rider & 1U) != 0;
    // The attribute is written as a word here, so entry 120's x becomes 0.
    arrow_attribute(state) = static_cast<std::uint8_t>(
        arrow_priority | (right_column ? 0 : arrow_mirror) | ((menu.rider & 7U) << 1U));
    oam_byte(state, arrow_entry + 1, 0) = 0;
    state.arrow.target_x = right_column ? right_column_x : left_column_x;
    menu.row = static_cast<std::uint8_t>(menu.rider >> 1U);
    state.arrow.target_y = static_cast<std::uint16_t>(first_row_y + menu.row * row_spacing);
    menu.intro = intro_start;
    state.screen = FrontEndScreen::rider_menu;
}

// $80:A705 and the HDMA: riders 0-7's palettes in colours 0x80-0xFE before the picture (the
// 255-byte copy leaves colour 0xFF alone), riders 8-15's written row by row during it.
void load_rider_palettes(FrontEndState& state, const FrontEndContent& content) {
    for (unsigned rider = 0; rider < 8; ++rider) {
        auto palette = asset(content, first_rider_palette + rider);
        if (rider == 7) palette = palette.first(palette.size() - 2);
        load_cgram(state, palette, 0x80 + rider * 16);
    }
    state.line_colours.clear();
    state.line_colours.push_back({0, 0, 0});
    for (unsigned line = 0; line < 128; ++line) {
        const auto palette = asset(content, first_rider_palette + 8 + line / 16);
        const auto colour = static_cast<std::uint16_t>(
            palette[(line % 16) * 2] | (static_cast<unsigned>(palette[(line % 16) * 2 + 1]) << 8U));
        state.line_colours.push_back({static_cast<std::uint8_t>(split_row + line),
                                      static_cast<std::uint8_t>(0x80 + line), colour});
    }
}

void move_arrow_row(FrontEndState& state, int step) {
    state.rider_menu.row = static_cast<std::uint8_t>(state.rider_menu.row + step);
    set_arrow_row_palette(state);
    state.arrow.target_y =
        static_cast<std::uint16_t>(state.arrow.target_y + step * static_cast<int>(row_spacing));
}

// $80:CC52-CD46. Left and Right only act on a change of column; Up and Down move once a press
// and stop at the first and last rows. A move down ends the pass; a move up skips Down.
// Returns true when a rider is chosen or Y or X pressed.
bool read_rider_menu_pad(FrontEndState& state, std::uint16_t pad) {
    auto& menu = state.rider_menu;
    auto& latches = state.latches;
    auto& attributes = arrow_attribute(state);
    if ((pad & left) && !(attributes & arrow_mirror)) {
        attributes = static_cast<std::uint8_t>((attributes | arrow_mirror) & 0xfdU);
        state.arrow.target_x = left_column_x;
    }
    if ((pad & right) && (attributes & arrow_mirror)) {
        attributes = static_cast<std::uint8_t>((attributes & ~arrow_mirror) | 0x02U);
        state.arrow.target_x = right_column_x;
    }
    bool moved_up = false;
    if (!(pad & up)) {
        latches.up = false;
    } else if (!latches.up) {
        latches.up = true;
        if (menu.row > 0) {
            move_arrow_row(state, -1);
            moved_up = true;
        }
    }
    if (!moved_up) {
        if (!(pad & down)) {
            latches.down = false;
        } else if (!latches.down) {
            latches.down = true;
            if (menu.row < riders / 2 - 1) {
                move_arrow_row(state, 1);
                return false;
            }
        }
    }
    // A choice or Y needs no release first: the latch at $0010 bit 0 is never set on this way.
    if (!(pad & (choose_buttons | back_buttons))) return false;
    const bool right_column = state.arrow.target_x == right_column_x;
    menu.back = (pad & back_buttons) != 0;
    state.menu.selection =
        static_cast<std::uint8_t>((menu.row * 2 + (right_column ? 1 : 0)) | (menu.back ? 0x80 : 0));
    return true;
}

// $80:CB04's first lines, before its first frame wait.
void open_rider_menu_entry(FrontEndState& state) {
    state.menu.selection = state.rider_menu.rider; // $80:CB07
    state.latches = {}; // $80:CB0C
    state.screen = FrontEndScreen::rider_menu_entry;
}

} // namespace

void enter_rider_menu(FrontEndState& state) {
    state.logo.raised = true;                                             // $80:F51B
    state.decorations.delay = static_cast<std::uint8_t>(state.menu.idle); // the shared $0089
    state.rider_menu.returning = false;
    open_rider_menu_entry(state);
}

void return_to_rider_menu(FrontEndState& state) {
    state.rider_menu.returning = true;
    open_rider_menu_entry(state);
}

void rider_menu_entry_frame(FrontEndState& state, const FrontEndContent& content) {
    constexpr std::uint16_t first_picture = intro_start >> 1U;
    switch (state.script_frame) {
    case 1:
        copy_oam(state);
        print_rider_menu(state, content);
        return;
    case 2:
        copy_oam(state);
        load_text(state, state.slide.hidden_half);
        state.arrow.target_x = off_screen_x;
        state.arrow.target_y = off_screen_y;
        state.rider_menu.picture = first_picture;
        return;
    case 3:
        upload_uni(state, content);
        start_slide(state, content, state.rider_menu.returning);
        return;
    default:
        if (!slide_frame(state, content)) return;
        open_rider_menu(state);
        choose_next_picture(state.rider_menu);
        return;
    }
}

void rider_menu_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    state.oam_buffer[last_high_byte] = shadow_hidden;
    copy_oam(state); // $80:D1EC, which also reads the pads
    load_rider_palettes(state, content);
    // $80:93A5: the text again, to the half it was first shown in (`$005A`), now hidden.
    load_text(state, state.slide.hidden_half);
    upload_uni(state, content);
    if (!read_rider_menu_pad(state, pads.one)) {
        choose_next_picture(state.rider_menu);
        return;
    }
    // $80:BBB8-BBC1 for a choice ($80:BC9B for Y), then $80:F4E9, which stops the HDMA at once.
    if (!state.rider_menu.back) {
        state.arrow.target_x = off_screen_x;
        state.arrow.target_y = off_screen_y;
        state.rider_menu.rider = state.menu.selection;
    }
    state.line_colours.clear();
    state.screen = FrontEndScreen::rider_menu_exit;
}

void rider_menu_exit_frame(FrontEndState& state, const FrontEndContent& content) {
    switch (state.script_frame) {
    case 1: lay_out_menu_objects(state); return;
    case 2: copy_oam(state); return;
    default: break;
    }
    copy_oam(state);
    const bool back = state.rider_menu.back;
    load_cgram(
        state,
        asset(content, back ? no_rider_palette : first_rider_palette + state.rider_menu.rider),
        0xf0);
    load_cgram(state, asset(content, menu_text_palette), 0xd0);
    load_cgram(state, asset(content, early_palette), 0xe0);
    state.registers.obsel = 0x63;
    if (!back) {
        enter_tour_menu(state);
        return;
    }
    print_main_menu(state, content);
    state.screen = FrontEndScreen::main_menu_return;
}

void main_menu_return_frame(FrontEndState& state, const FrontEndContent& content) {
    constexpr std::uint32_t palette_frame = 41, tiles_frame = 42;
    switch (state.script_frame) {
    case 1:
        load_text(state, state.slide.hidden_half);
        state.menu.selection = 0;
        start_slide(state, content, true);
        return;
    case palette_frame: reload_menu_palette(state, content); return;
    case tiles_frame:
        reload_menu_text_tiles(state, content);
        start_main_menu(state);
        return;
    default:
        if (slide_frame(state, content)) state.logo.raised = false; // $80:F52B
        return;
    }
}

} // namespace unirally::front_end_screens
