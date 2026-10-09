// The main menu's WIPE RAM code (R-0092): Left+A+L+R alone on either pad opens a two-entry menu,
// WIPE RAM and MAIN MENU (`$80:A9B4`). WIPE RAM raises the logo and slides in a warning; Select+Y+A
// alone on either pad then puts the cartridge RAM back to a cold start's, and most other buttons
// leave it as it was. Either answer prints its message, waits for a press and slides the menu
// back in; MAIN MENU, Y or X go back to the main menu.
#include "front_end_screens.hpp"

#include <cstdint>
#include <span>

namespace unirally::front_end_screens {
namespace {

// The streams in `front-end.wipe-ram-menu-text` and `front-end.wipe-ram-messages`.
constexpr unsigned wipe_ram_entry = 0, main_menu_entry = 1, last_entry = main_menu_entry;
constexpr unsigned warning_message = 0, reset_message = 1, nothing_done_message = 2;
// $80:B93C: the arrow's first row, a row's height, both in sixteenths of a pixel.
constexpr std::uint16_t first_row_y = 0x0580, row_height = 0x0180;
// $80:AA42-AA62: a button of the mask on either pad cancels; exactly Select+Y+A wipes.
constexpr std::uint16_t cancel_buttons = 0x9f70, reset_buttons = 0x6080;
// The wipe and the defaults (`$83:FB41` to `$83:90F4`) run on past two frame boundaries
// (`confirm`: the press on 1100, the message printed on 1102), with no frame wait.
constexpr std::uint32_t reset_busy_frames = 2;

void print_stream(FrontEndState& state, const FrontEndContent& content,
                  std::span<const std::uint8_t> table, unsigned n) {
    const auto stream = nth_string(table, n);
    print_text(state.text, state.printer, {stream.data(), stream.size() + 1},
               content.character_table);
}

// $80:B93C-B975 and $80:B9D9-BA5C: the arrow's x target is the entry's signed column (in units of
// 8 pixels), its y target the entry's row.
void aim_at_entry(FrontEndState& state, const FrontEndContent& content) {
    const auto entry = state.menu.selection;
    const auto column = static_cast<std::int8_t>(content.wipe_ram_arrow_columns[entry]);
    state.arrow.target_x = static_cast<std::uint16_t>(column * 128);
    state.arrow.target_y = static_cast<std::uint16_t>(first_row_y + entry * row_height);
}

// $80:B9A9-BA5C: Down (or Select) before Up, each once per press, with the move's sound; past
// either end the arrow wraps.
void move_entry(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    const auto pad = static_cast<std::uint16_t>(pads.one | pads.two);
    const bool down = (pad & (pad_down | pad_select)) != 0;
    const bool up = !down && (pad & pad_up) != 0;
    if (!down && !up) {
        state.latches.moved = false; // $80:B99C
        return;
    }
    if (state.latches.moved) return;
    state.latches.moved = true;
    play_menu_sound(state, MenuSound::navigate); // $80:BA13 (Down), $80:B9B1 (Up)
    auto& entry = state.menu.selection;
    if (down)
        entry = entry == last_entry ? 0 : static_cast<std::uint8_t>(entry + 1);
    else
        entry = entry == 0 ? last_entry : static_cast<std::uint8_t>(entry - 1);
    aim_at_entry(state, content);
}

// $80:AAD7-AAEF: "nothing done" over the warning, the logo lowered.
void cancel_reset(FrontEndState& state, const FrontEndContent& content) {
    state.text.words.fill(cleared_text);
    print_stream(state, content, content.wipe_ram_messages, nothing_done_message);
    state.logo.raised = false; // $80:F52B
    state.wipe_ram = {.reset = false};
    state.screen = FrontEndScreen::wipe_ram_answer;
}

// $80:C236-C244, after the press: entries 32-35 hidden, entries 30 and 31 hidden and small.
void hide_entries_30_to_35(FrontEndState& state) {
    high_bits(state, 32) = four_hidden;
    auto& bits = high_bits(state, 28);
    const auto entries_30_31 =
        static_cast<std::uint8_t>(hidden_bit(30) | large_bit(30) | hidden_bit(31) | large_bit(31));
    bits = static_cast<std::uint8_t>((bits & ~entries_30_31) | hidden_bit(30) | hidden_bit(31));
}

// $80:C24C, then `$80:C206`: both pads released, then a press on two frames running.
bool waited_for_press(FrontEndState& state, FrontEndPads pads) {
    auto& menu = state.wipe_ram;
    const bool press = pads.one != 0 || pads.two != 0;
    if (!menu.released) {
        menu.released = !press;
        return false;
    }
    if (!menu.press_seen || !press) {
        menu.press_seen = press;
        return false;
    }
    return true;
}

} // namespace

bool enter_wipe_ram_menu(FrontEndState& state, const FrontEndContent& content) {
    if (content.wipe_ram_menu_text.empty()) return false; // an older pack: not native
    state.mode = FrontEndMode::wipe_ram;
    // The forward slide steps the decorations from the main menu's idle count (the shared `$0089`).
    state.decorations.delay = static_cast<std::uint8_t>(state.menu.idle);
    state.wipe_ram = {};
    print_wipe_ram_menu(state, content);
    return true;
}

// $80:EF4D: the entries into the hidden half, from the last (`$80:EF5D-EF71`): MAIN MENU, then
// WIPE RAM.
void print_wipe_ram_menu(FrontEndState& state, const FrontEndContent& content) {
    state.text.words.fill(cleared_text);
    for (unsigned entry = last_entry + 1; entry-- > 0;)
        print_stream(state, content, content.wipe_ram_menu_text, entry);
    load_text(state, state.slide.hidden_half);
    state.screen = FrontEndScreen::wipe_ram_menu_entry;
}

// $80:EF73-EF88: the slide, forward from the main menu, back after an answer (`$00AC` is then
// 0x80, the DMA start `$80:93A5` left in A; `$80:A9E7` stores it). Then `$80:B93C`'s set-up on
// WIPE RAM, and its first pass's `$80:EB23`.
void wipe_ram_menu_entry_frame(FrontEndState& state, const FrontEndContent& content) {
    // No OAM copy before the slide's first pass (`$80:9318` runs after each of its frame waits):
    // the picture keeps the arrow where the last pass left it.
    if (state.script_frame == 1) {
        start_slide(state, content, state.wipe_ram.returning);
        return;
    }
    if (!slide_frame(state, content)) return;
    state.menu.selection = wipe_ram_entry;
    state.latches = {};
    aim_at_entry(state, content);
    turn_markers(state, content);
    state.screen = FrontEndScreen::wipe_ram_menu;
}

// One pass of `$80:B93C`: Y or X (`$80:B74A`), then B, Start or A (`$80:B71D`), then the moves;
// a pass that stays turns the markers (`$80:EB23`). MAIN MENU, Y and X go back to the main menu
// (`$80:A9CD-A9F7`); WIPE RAM goes on to the warning (`$80:AA1B-AA2C`).
void wipe_ram_menu_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    copy_oam(state); // $80:D1EC
    const auto pad = static_cast<std::uint16_t>(pads.one | pads.two);
    const bool back = (pad & back_buttons) != 0;
    if (back || (pad & choose_buttons) != 0) {
        if (back || state.menu.selection == main_menu_entry) {
            print_main_menu(state, content);
            state.screen = FrontEndScreen::main_menu_return;
            return;
        }
        state.logo.raised = true; // $80:F51B
        send_arrow_off(state);
        state.text.words.fill(cleared_text);
        print_stream(state, content, content.wipe_ram_messages, warning_message);
        load_text(state, state.slide.hidden_half);
        state.screen = FrontEndScreen::wipe_ram_warning_entry;
        return;
    }
    move_entry(state, content, pads);
    turn_markers(state, content);
}

void wipe_ram_warning_entry_frame(FrontEndState& state, const FrontEndContent& content) {
    if (state.script_frame == 1) {
        start_slide(state, content, false); // $80:AA35
        return;
    }
    if (slide_frame(state, content)) state.screen = FrontEndScreen::wipe_ram_warning;
}

// $80:AA38-AA62, each frame: a button of the cancel mask on pad 1, then on pad 2, cancels; else
// exactly Select+Y+A on pad 1 or pad 2 puts the cartridge RAM back to a cold start's
// (`$80:AA64-AAB4`: the boot's wipe and defaults, `$80:8C74-8CC2`).
void wipe_ram_warning_frame(FrontEndState& state, const FrontEndContent& content,
                            FrontEndPads pads) {
    copy_oam(state); // $80:D1EC
    if (((pads.one | pads.two) & cancel_buttons) != 0) {
        cancel_reset(state, content);
        return;
    }
    if (pads.one != reset_buttons && pads.two != reset_buttons) return;
    wipe_cartridge(state, content);
    state.wipe_ram = {.reset = true};
    state.screen = FrontEndScreen::wipe_ram_answer;
}

bool wipe_ram_answer_waits(const FrontEndState& state) {
    return !state.wipe_ram.reset || state.script_frame + 1 > reset_busy_frames;
}

// After the answer: the wipe's busy frames and its message (`$80:AABA-AAC8`; the wipe cleared the
// logo's bit, so the logo slides down; "nothing done" is already printed), the message to the
// shown half (`$80:93A5`), then the press waits, with the decorations (`$83:9A1E`). The second
// press of the wait brings the menu back (`$80:A9E7`, to `$80:A9B8`).
void wipe_ram_answer_frame(FrontEndState& state, const FrontEndContent& content,
                           FrontEndPads pads) {
    // The message's frame: "nothing done" was printed on the press's (script frame 0).
    const auto printed = state.wipe_ram.reset ? reset_busy_frames : 0;
    const auto frame = state.script_frame;
    if (frame < printed) return;
    if (frame == printed) {
        state.text.words.fill(cleared_text);
        print_stream(state, content, content.wipe_ram_messages, reset_message);
        return;
    }
    if (frame == printed + 1) {
        load_text(state, state.slide.shown_half);
        return;
    }
    copy_oam(state); // $80:D1EC
    step_decorations(state, content);
    if (!waited_for_press(state, pads)) return;
    hide_entries_30_to_35(state);
    state.wipe_ram = {.returning = true};
    print_wipe_ram_menu(state, content);
}

} // namespace unirally::front_end_screens
