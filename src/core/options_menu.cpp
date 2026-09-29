// The two five-choice menus under OPTIONS (R-0072). Both print their text into the hidden
// half of BG2 at the choice frame, then slide it into view over 39 more pictures.
#include "front_end_screens.hpp"

#include <stdexcept>

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

void move_choice(FrontEndState& state, std::span<const std::uint8_t> columns,
                 FrontEndPads pads) {
    const auto pad = static_cast<std::uint16_t>(pads.one | pads.two);
    const bool down = (pad & (pad_down | pad_select)) != 0;
    const bool up = !down && (pad & pad_up) != 0;
    if (!down && !up) {
        state.latches.moved = false;
        return;
    }
    if (state.latches.moved) return;
    state.latches.moved = true;
    auto& row = state.menu.selection;
    row = static_cast<std::uint8_t>(down ? (row + 1U) % choice_count
                                         : (row + choice_count - 1U) % choice_count);
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
        std::copy_n(state.video.cgram.begin() + 0x100, 256,
                    state.options_upper_palette.begin());
        state.options_palette_saved = true;
        state.rider_menu.purpose = state.menu.selection == 1
            ? RiderMenuPurpose::define_player : RiderMenuPurpose::rename_player;
        enter_rider_menu(state);
        return;
    }
    throw std::runtime_error("OPTIONS editor path is not yet recovered");
}

void enter_records_menu(FrontEndState& state, const FrontEndContent& content) {
    // $80:D525-D53C; R-0072 observation 5.
    state.decorations.delay = 0;
    print_choices(state, content, content.records_menu_text);
    state.screen = FrontEndScreen::records_entry;
}

void records_entry_frame(FrontEndState& state, const FrontEndContent& content) {
    if (state.script_frame == 1) {
        copy_oam(state);
        start_slide(state, content, false);
        return;
    }
    if (!slide_frame(state, content)) return;
    state.menu.selection = 0;
    aim_choice(state, content.records_arrow_columns);
    state.latches = {};
    state.screen = FrontEndScreen::records_menu;
}

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
    if (pad & choose_buttons)
        throw std::runtime_error("RECORDS detail path is not yet recovered");
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
    print_text(state.text, state.printer, content.define_player_warning,
               content.character_table, &variables);
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
    std::copy_n(state.records.rider_names.begin() + 15 * 16, 8,
                state.keyboard.scratch.begin() + 2);
    state.keyboard.scratch[10] = state.keyboard.scratch[11] = 0xff;
    state.text.words.fill(cleared_text);
    print_text(state.text, state.printer, content.keyboard_text, content.character_table);
    print_text(state.text, state.printer, content.rename_prompt, content.character_table);
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

std::uint8_t keyboard_key(const KeyboardEditor& editor,
                          std::span<const std::uint8_t> table) {
    unsigned base = 0;
    switch (editor.offset_y) {
    case 0: base = 2; break;     // A-M at $80:A4DD
    case -24: base = 22; break;  // N-Z at $80:A4F1
    case -48: base = 38; break;  // backspace and digits at $80:A501
    case -72: base = 54; break;  // punctuation and OK at $80:A511
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

} // namespace

void rename_keyboard_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    copy_oam(state);
    load_text(state, state.slide.shown_half);
    auto& editor = state.keyboard;
    if (state.script_frame == 1)
        initialize_keyboard_cursor(state);
    // $80:A231 positions the cursor before polling, so a direction affects OAM one
    // frame after it changes the key offsets.
    if (state.script_frame > 1) position_keyboard_cursor(state);
    const auto pad = pads.one;
    const auto pressed = static_cast<std::uint16_t>(pad & ~editor.previous_buttons);
    editor.previous_buttons = pad;
    if (pressed & pad_left) {
        editor.offset_x = editor.offset_x == 0 ? -192
                                               : static_cast<std::int16_t>(editor.offset_x + 16);
        state.menu.selection = static_cast<std::uint8_t>((state.menu.selection + 1) % 39);
    } else if (pressed & pad_right) {
        editor.offset_x = editor.offset_x == -192 ? 0
                                                   : static_cast<std::int16_t>(editor.offset_x - 16);
        state.menu.selection = state.menu.selection == 0 ? 39 : state.menu.selection - 1;
    } else if (pressed & pad_up) {
        editor.offset_y = editor.offset_y == 0 ? -72
                                               : static_cast<std::int16_t>(editor.offset_y + 24);
    } else if (pressed & pad_down) {
        editor.offset_y = editor.offset_y == -72 ? 0
                                                 : static_cast<std::int16_t>(editor.offset_y - 24);
    }
    if (pressed & back_buttons)
        throw std::runtime_error("OPTIONS keyboard cancellation is not yet recovered");
    if (!(pressed & choose_buttons)) return;
    // $80:A3A0-A3BC aims at the selected key, in sixteenths of a pixel.
    state.arrow.target_x = static_cast<std::uint16_t>(-editor.offset_x * 16);
    state.arrow.target_y = static_cast<std::uint16_t>((31 - editor.offset_y) * 16);
    const auto key = keyboard_key(editor, content.keyboard_text);
    if (key == 0x40) { // OK at $80:A52A
        if (editor.length == 0) return;
        editor.scratch[editor.length] = editor.scratch[editor.length + 1] = 0xff;
        state.screen = FrontEndScreen::rename_commit;
        return;
    }
    if (key == 0x3c) { // backspace
        if (editor.length != 0) {
            --editor.length;
            --state.printer.position;
            constexpr std::array<std::uint8_t, 2> dot{'.', 0xff};
            print_text(state.text, state.printer, dot, content.character_table);
            --state.printer.position;
        }
        return;
    }
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
    const auto first = static_cast<std::size_t>(state.rider_menu.rider) * 16U;
    std::copy_n(state.keyboard.scratch.begin(), 8,
                state.records.rider_names.begin() + first);
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
    return_to_options_menu(state, content);
}

} // namespace unirally::front_end_screens
