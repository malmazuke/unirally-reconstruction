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
    if (!slide_frame(state, content)) return;
    state.menu.selection = 0;
    aim_choice(state, content.options_arrow_columns);
    state.latches = {};
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
        print_choices(state, content, content.options_menu_text);
        state.screen = FrontEndScreen::options_return;
        return;
    }
    if (pad & choose_buttons)
        throw std::runtime_error("RECORDS detail path is not yet recovered");
}

} // namespace unirally::front_end_screens
