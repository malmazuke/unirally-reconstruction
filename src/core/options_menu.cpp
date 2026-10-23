// The two five-choice menus under OPTIONS (R-0072). Both print their text into the hidden
// half of BG2 at the choice frame, then slide it into view over 39 more pictures.
#include "front_end_screens.hpp"

#include <algorithm>
#include <array>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

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

// $80:9F74: the league slots' arrow columns, signed eighths of 16 pixels as the other menus'.
constexpr unsigned league_slot_count = 6;
constexpr std::array<std::uint8_t, league_slot_count> league_slot_columns{2, 2, 0xfe, 0, 0, 2};

void aim_choice(FrontEndState& state, std::span<const std::uint8_t> columns) {
    const auto row = state.menu.selection;
    // The table byte is signed before conversion to sixteenths of a pixel.
    state.arrow.target_x = static_cast<std::uint16_t>(
        static_cast<std::int16_t>(static_cast<std::int8_t>(columns[row])) * 128);
    state.arrow.target_y = static_cast<std::uint16_t>(first_row + row * row_step);
}

void move_choice(FrontEndState& state, std::span<const std::uint8_t> columns, FrontEndPads pads,
                 unsigned count = choice_count) {
    const auto pad = static_cast<std::uint16_t>(pads.one | pads.two);
    const bool down = (pad & (pad_down | pad_select)) != 0;
    const bool up = !down && (pad & pad_up) != 0;
    if (!down && !up) {
        state.latches.moved = false;
        return;
    }
    if (state.latches.moved) return;
    state.latches.moved = true;
    play_menu_sound(state, MenuSound::navigate); // $80:BA13 (Down), $80:B9B1 (Up)
    auto& row = state.menu.selection;
    row = static_cast<std::uint8_t>(down ? (row + 1U) % count : (row + count - 1U) % count);
    aim_choice(state, columns);
}

// The two warnings' answer ($80:C17B-C197, $80:9C07-9C23), from either pad's whole word: B, Start,
// a direction, X, L or R leaves; only Select, Y and A together, with nothing else, confirm.
enum class WarningAnswer : std::uint8_t { none, leave, confirm };

WarningAnswer warning_answer(FrontEndPads pads) {
    constexpr std::uint16_t leave = 0x9f70, confirm = pad_select | 0x4000U | pad_a;
    if ((pads.one & leave) || (pads.two & leave)) return WarningAnswer::leave;
    if (pads.one == confirm || pads.two == confirm) return WarningAnswer::confirm;
    return WarningAnswer::none;
}

void return_to_main(FrontEndState& state, const FrontEndContent& content) {
    print_main_menu(state, content);
    state.screen = FrontEndScreen::main_menu_return;
}

} // namespace

void enter_options_menu(FrontEndState& state, const FrontEndContent& content) {
    // $80:B626-B639, $80:EF4D; R-0072 observation 1.
    state.mode = FrontEndMode::options;
    // $80:B626-B639; R-0072 observation 1. `$0089`, the main menu's idle count, is the decorations'
    // delay from here: its low byte counts down until it goes negative, then from 2 (R-0095).
    state.decorations.delay = static_cast<std::uint8_t>(state.menu.idle);
    print_choices(state, content, content.options_menu_text);
    state.screen = FrontEndScreen::options_entry;
}

void return_to_options_menu(FrontEndState& state, const FrontEndContent& content) {
    print_choices(state, content, content.options_menu_text);
    state.screen = FrontEndScreen::options_return;
}

namespace {

// $80:B63C: the slide ends into the generic list loop (`$80:B93C`), which turns the PICK TRACK
// markers (`$80:EB23`) in that frame (R-0095).
void open_options_choices(FrontEndState& state, const FrontEndContent& content) {
    state.menu.selection = 0;
    aim_choice(state, content.options_arrow_columns);
    state.latches = {};
    turn_markers(state, content);
    state.screen = FrontEndScreen::options_menu;
}

} // namespace

void options_entry_frame(FrontEndState& state, const FrontEndContent& content) {
    if (state.script_frame == 1) {
        // `$80:EF4D` prints and waits without an OAM copy; the slide's passes copy it (R-0095).
        start_slide(state, content, false);
        return;
    }
    if (!slide_frame(state, content)) return;
    open_options_choices(state, content);
}

// Back into OPTIONS from RECORDS, the keyboard, a picker or the league slots: `$80:EF4D`'s frame,
// the back slide, and the loop from the slide's last frame, as on entry (R-0097).
void options_return_frame(FrontEndState& state, const FrontEndContent& content) {
    if (state.script_frame == 1) {
        // As on entry, no OAM copy before the slide's first pass (`$80:E29F`).
        start_slide(state, content, true);
        return;
    }
    if (!slide_frame(state, content)) return;
    // The colours a rider picker's HDMA wrote stay (`scores-carry`'s PLAYER SCORES icon, R-0097).
    open_options_choices(state, content);
}

void options_menu_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    copy_oam(state);
    move_choice(state, content.options_arrow_columns, pads);
    const auto pad = static_cast<std::uint16_t>(pads.one | pads.two);
    if (pad & back_buttons || ((pad & choose_buttons) && state.menu.selection == 4)) {
        return_to_main(state, content);
        return;
    }
    if (!(pad & choose_buttons)) {
        turn_markers(state, content); // $80:B97A's `$80:EB23` (R-0095)
        return;
    }
    if (state.menu.selection == 0) {
        enter_records_menu(state, content);
        return;
    }
    if (state.menu.selection == 1 || state.menu.selection == 2) {
        state.rider_menu.purpose = state.menu.selection == 1 ? RiderMenuPurpose::define_player
                                                             : RiderMenuPurpose::rename_player;
        enter_rider_menu(state);
        return;
    }
    enter_league_slots(state, content);
}

void enter_league_slots(FrontEndState& state, const FrontEndContent& content, bool slides_back) {
    // $80:9EF7-9F73, R-0072 observations 4 and 6.
    state.league.slots_slide_back = slides_back;
    state.text.words.fill(cleared_text);
    TextVariables variables;
    variables.league_names = state.records.league_names;
    for (unsigned row = 0; row < 6; ++row) {
        const auto entry = nth_string(content.league_slot_text, row);
        print_text(state.text, state.printer, {entry.data(), entry.size() + 1},
                   content.character_table, &variables);
    }
    load_text(state, state.slide.hidden_half);
    state.screen = FrontEndScreen::league_slots_entry;
}

void league_slots_entry_frame(FrontEndState& state, const FrontEndContent& content) {
    if (state.script_frame == 1) {
        // `$80:EF4D`: no OAM copy before the first pass.
        start_slide(state, content, state.league.slots_slide_back);
        return;
    }
    if (!slide_frame(state, content)) return;
    state.menu.selection = state.league.slot;
    aim_choice(state, league_slot_columns); // $80:B93E keeps `$009B`
    state.latches = {};
    turn_markers(state, content); // $80:9F09 JSR $B93E (R-0097)
    state.screen = FrontEndScreen::league_slots;
}

void league_slots_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    copy_oam(state);
    move_choice(state, league_slot_columns, pads, league_slot_count);
    const auto pad = static_cast<std::uint16_t>(pads.one | pads.two);
    if (pad & back_buttons) {
        if (state.mode == FrontEndMode::league)
            return_to_main(state, content);
        else
            return_to_options_menu(state, content);
        return;
    }
    if (!(pad & choose_buttons)) {
        turn_markers(state, content); // $80:B97A
        return;
    }
    play_menu_sound(state, MenuSound::select); // $80:9F15
    state.league.slot = state.menu.selection;
    if (state.mode == FrontEndMode::league) {
        choose_league_slot(state, content);
        return;
    }
    state.text.words.fill(cleared_text);
    TextVariables variables;
    variables.word = [&](std::uint16_t address) -> std::uint16_t {
        if (address != 0x00cc) throw std::invalid_argument("unknown league warning operand");
        return state.league.slot;
    };
    variables.league_names = state.records.league_names;
    print_text(state.text, state.printer, content.league_warning, content.character_table,
               &variables);
    print_text(state.text, state.printer, content.define_player_confirm_prompt,
               content.character_table);
    load_text(state, state.slide.hidden_half);
    state.screen = FrontEndScreen::league_warning_entry;
}

void league_warning_entry_frame(FrontEndState& state, const FrontEndContent& content) {
    if (state.script_frame == 1) {
        start_slide(state, content, false); // `$80:9BE8-9BEE`: no OAM copy before the first pass
        return;
    }
    if (!slide_frame(state, content)) return;
    state.arrow.target_x = 0x0100;
    state.arrow.target_y = first_row;
    state.screen = FrontEndScreen::league_warning;
}

void league_warning_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    copy_oam(state);
    const auto answer = warning_answer(pads);
    if (answer == WarningAnswer::leave) { // $80:9C46: back to OPTIONS
        return_to_options_menu(state, content);
        return;
    }
    if (answer != WarningAnswer::confirm) return;
    state.league.members = 0;
    state.rider_menu.purpose = RiderMenuPurpose::league_members;
    state.rider_menu.choice_latched = true; // $80:9D11
    enter_rider_menu(state);
}

void enter_records_menu(FrontEndState& state, const FrontEndContent& content) {
    // $80:D525-D53C; R-0072 observation 5. `$0089`, the decorations' delay, goes on (R-0097).
    print_choices(state, content, content.records_menu_text);
    state.screen = FrontEndScreen::records_entry;
}

void records_entry_frame(FrontEndState& state, const FrontEndContent& content) {
    if (state.script_frame == 1) {
        start_slide(state, content, false); // `$80:EF4D`: no OAM copy before the first pass
        return;
    }
    if (!slide_frame(state, content)) return;
    state.menu.selection = 0;
    aim_choice(state, content.records_arrow_columns);
    state.latches = {};
    turn_markers(state, content); // $80:D53F JSR $B93C (R-0097)
    state.screen = FrontEndScreen::records_menu;
}

void reopen_records_menu(FrontEndState& state, const FrontEndContent& content) {
    // $80:B93C: the first choice, no latches, the markers turned.
    state.menu.selection = 0;
    aim_choice(state, content.records_arrow_columns);
    state.latches = {};
    turn_markers(state, content);
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
    if (!(pad & choose_buttons)) {
        turn_markers(state, content); // $80:B97A
        return;
    }
    auto& detail = state.records_detail;
    detail.category = state.menu.selection;
    state.screen = FrontEndScreen::records_detail_entry;
    // $80:D54B: the logo slides up for every choice (R-0097, R-0100). PLAYER SCORES and GROUP
    // TABLES start on the last rider and league.
    state.logo.raised = true;
    detail.phase = ScoresPhase::entering;
    detail.high_scores = HighScoresPhase::entering;
    detail.press_seen = detail.group_reprinted = false;
    // $80:DACF-DAD6: PLAYER SCORES masks its rider to 0-15 before its first frame wait.
    constexpr std::uint8_t rider_bits = 15;
    if (detail.category == player_scores_category)
        state.options_rider = static_cast<std::uint8_t>(state.options_rider & rider_bits);
}

void records_detail_entry_frame(FrontEndState& state, const FrontEndContent& content) {
    if (state.records_detail.category == track_records_category) {
        track_records_entry_frame(state, content);
        return;
    }
    if (shows_scores(state)) {
        scores_entry_frame(state, content);
        return;
    }
    high_scores_entry_frame(state, content);
}

void records_detail_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    if (state.records_detail.category == track_records_category) {
        track_records_frame(state, content, pads);
        return;
    }
    if (shows_scores(state)) {
        scores_frame(state, content, pads);
        return;
    }
    high_scores_frame(state, content, pads);
}

void records_detail_exit_frame(FrontEndState& state, const FrontEndContent& content) {
    // Only TRACK RECORDS has a screen of its own on the way out; the other three slide back from
    // their loop's screen.
    track_records_exit_frame(state, content);
}

void enter_define_player_warning(FrontEndState& state, const FrontEndContent& content) {
    // $80:C12A-C1EA: show the warning for the chosen rider. Its F8 operand reads
    // the selected rider from $00CA and that rider's mutable SRAM name.
    state.text.words.fill(cleared_text);
    TextVariables variables;
    variables.word = [&](std::uint16_t address) -> std::uint16_t {
        if (address != 0x00ca) throw std::invalid_argument("unknown warning text operand");
        return state.options_rider;
    };
    variables.rider_names = state.records.rider_names;
    print_text(state.text, state.printer, content.define_player_warning, content.character_table,
               &variables);
    print_text(state.text, state.printer, content.define_player_confirm_prompt,
               content.character_table);
    load_text(state, state.slide.hidden_half);
    state.screen = FrontEndScreen::define_player_warning_entry;
}

void define_player_warning_entry_frame(FrontEndState& state, const FrontEndContent& content) {
    if (state.script_frame == 1) {
        start_slide(state, content, false); // `$80:C15C-C162`: no OAM copy before the first pass
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
    const auto rider = state.options_rider;
    auto& records = state.records;
    std::fill_n(records.rider_names.begin() + static_cast<std::size_t>(rider) * 16, 8,
                std::uint8_t{'_'});
    records.statistics[rider].fill(0);
    records.versus_today[rider] = records.versus_races[rider] = records.versus_wins[rider] = 0;
    records.tour_levels[rider] = 0;
    // $83:9BB5: the best times' loop starts from track 0 in `$00CE`, the chosen track (R-0095).
    state.tour_menu.track = 0;
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

void define_player_warning_frame(FrontEndState& state, const FrontEndContent& content,
                                 FrontEndPads pads) {
    copy_oam(state);
    const auto answer = warning_answer(pads);
    if (answer == WarningAnswer::leave) { // $80:C1A8-C1AD: the logo down, back to OPTIONS
        state.logo.raised = false;
        return_to_options_menu(state, content);
        return;
    }
    if (answer != WarningAnswer::confirm) return;
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
    std::copy_n(state.records.rider_names.begin() + 15 * 16, 8, state.keyboard.scratch.begin() + 2);
    state.keyboard.scratch[10] = state.keyboard.scratch[11] = 0xff;
    state.text.words.fill(cleared_text);
    print_text(state.text, state.printer, content.keyboard_text, content.character_table);
    print_text(state.text, state.printer, content.rename_prompt, content.character_table);
    load_text(state, state.slide.hidden_half);
    state.screen = FrontEndScreen::rename_entry;
}

void enter_league_editor(FrontEndState& state, const FrontEndContent& content) {
    // $80:9E57-9E7C shares the keyboard but starts with $80:9E0F's scratch.
    state.keyboard = {};
    state.keyboard.scratch = {0xf9, 0x07, 0xfe, 0x1b, 0x0d, 0x58,
                              0xfb, 0xff, 0xfe, 0x5f, 0xff, 0xff};
    // $80:9E3C-9E54; R-0073: retain the last marker's column/row, including bytes after FF FF.
    state.keyboard.scratch[3] = state.league.marker_column;
    state.keyboard.scratch[4] = state.league.marker_row;
    // $00E8-$00EE held underscores in every capture of a league's naming (R-0097; their writer is
    // not identified): a long name's commit copies them after its end.
    std::fill(state.keyboard.scratch.begin() + 12, state.keyboard.scratch.end(), std::uint8_t{'_'});
    constexpr std::uint8_t league_length_limit = 18; // $80:9E78
    state.keyboard.length_limit = league_length_limit;
    state.league.editor_active = true;
    state.menu.selection = 0;
    state.text.words.fill(cleared_text);
    print_text(state.text, state.printer, content.keyboard_text, content.character_table);
    print_text(state.text, state.printer, content.league_prompt, content.character_table);
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

std::uint8_t keyboard_key(const KeyboardEditor& editor, std::span<const std::uint8_t> table) {
    unsigned base = 0;
    switch (editor.offset_y) {
    case 0: base = 2; break;    // A-M at $80:A4DD
    case -24: base = 22; break; // N-Z at $80:A4F1
    case -48: base = 38; break; // backspace and digits at $80:A501
    case -72: base = 54; break; // punctuation and OK at $80:A511
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

// $80:A416-A445: the last character erased with the result's sound, or the refusal's when the
// name is empty.
void erase_keyboard_character(FrontEndState& state, const FrontEndContent& content) {
    if (state.keyboard.length == 0) {
        play_menu_sound(state, MenuSound::refused); // $80:A41A
        return;
    }
    play_menu_sound(state, MenuSound::result); // $80:A420
    --state.keyboard.length;
    --state.printer.position;
    constexpr std::array<std::uint8_t, 2> dot{'.', 0xff};
    print_text(state.text, state.printer, dot, content.character_table);
    --state.printer.position;
}

// The keyboard's inputs in the order `$80:A25F-A294` tests them, each on either pad the gate
// lets in: the first one held is the frame's only input, so a held direction hides every button.
enum class KeyboardInput : std::uint8_t { none, left, right, up, down, ok, erase, choose };

KeyboardInput keyboard_input(std::uint16_t pad) {
    if (pad & pad_left) return KeyboardInput::left;         // $80:B7B9
    if (pad & pad_right) return KeyboardInput::right;       // $80:B7DE
    if (pad & pad_up) return KeyboardInput::up;             // $80:B803
    if (pad & pad_down) return KeyboardInput::down;         // $80:B830
    if (pad & pad_start) return KeyboardInput::ok;          // $80:B916: Start goes straight to OK
    if (pad & back_buttons) return KeyboardInput::erase;    // $80:B74A
    if (pad & choose_buttons) return KeyboardInput::choose; // $80:B71D
    return KeyboardInput::none;
}

// The bits of `$008F` the keyboard keeps (KeyboardEditor::latches).
constexpr std::uint8_t latch_left = 0x01, latch_right = 0x02, latch_down = 0x04, latch_up = 0x08,
                       latch_choice = 0x10, latch_erase = 0x20;

// $80:A2A0-A2B8 and $80:A2DF-A2F7: Left and Right act on their press, and while held again each
// time the byte `$0076` counts below zero: 19 frames after the press, then every 7.
bool held_key_repeats(KeyboardEditor& editor, std::uint8_t latch) {
    constexpr std::uint8_t first_wait = 18, next_wait = 6;
    if (!(editor.latches & latch)) {
        editor.latches = static_cast<std::uint8_t>(editor.latches | latch);
        editor.repeat_wait = first_wait;
        return true;
    }
    --editor.repeat_wait;
    if (!(editor.repeat_wait & 0x80U)) return false;
    editor.repeat_wait = next_wait;
    return true;
}

// Up, Down, a choice and Y or X act once a press ($80:A31C, $80:A340, $80:A39A, $80:A364).
bool first_press(KeyboardEditor& editor, std::uint8_t latch) {
    if (editor.latches & latch) return false;
    editor.latches = static_cast<std::uint8_t>(editor.latches | latch);
    return true;
}

// $80:A2BA-A2DC (Left) and $80:A2F9-A319 (Right): a column along the row, wrapping, with the
// navigation sound.
void step_key_column(FrontEndState& state, bool left) {
    auto& editor = state.keyboard;
    play_menu_sound(state, MenuSound::navigate);
    if (left) {
        editor.offset_x =
            editor.offset_x == 0 ? -192 : static_cast<std::int16_t>(editor.offset_x + 16);
        state.menu.selection = static_cast<std::uint8_t>((state.menu.selection + 1) % 39);
    } else {
        editor.offset_x =
            editor.offset_x == -192 ? 0 : static_cast<std::int16_t>(editor.offset_x - 16);
        state.menu.selection = state.menu.selection == 0 ? 39 : state.menu.selection - 1;
    }
}

// $80:A34A-A35F (Up) and $80:A326-A33B (Down): a row, wrapping, with the navigation sound.
void step_key_row(FrontEndState& state, bool up) {
    auto& editor = state.keyboard;
    play_menu_sound(state, MenuSound::navigate);
    if (up)
        editor.offset_y =
            editor.offset_y == 0 ? -72 : static_cast<std::int16_t>(editor.offset_y + 24);
    else
        editor.offset_y =
            editor.offset_y == -72 ? 0 : static_cast<std::int16_t>(editor.offset_y - 24);
}

// $80:A491-A4AC: OK, from Start or the OK key. An empty name is refused ($80:A493 to $80:A41A);
// otherwise the select sound, the name's end, and the keyboard's last frame.
void press_keyboard_ok(FrontEndState& state) {
    auto& editor = state.keyboard;
    if (editor.length == 0) {
        play_menu_sound(state, MenuSound::refused);
        return;
    }
    play_menu_sound(state, MenuSound::select); // $80:A495
    editor.scratch[editor.length] = editor.scratch[editor.length + 1] = 0xff;
    state.screen = FrontEndScreen::rename_commit;
}

// $80:A3BF-A3C3: the choice's latch is stored from the accumulator that still holds the arrow's
// new y target, so the target's low byte (0x70 or 0xF0) comes with it, the Y-or-X bit among it.
std::uint8_t latches_after_choice(std::uint16_t target_y) {
    return static_cast<std::uint8_t>((target_y & 0xffU) | latch_choice);
}

// A choice ($80:A39A-A48E): the arrow sent to the key, then OK, a deletion or a character.
void choose_key(FrontEndState& state, const FrontEndContent& content) {
    auto& editor = state.keyboard;
    // $80:A3A0-A3BC aims at the selected key, in sixteenths of a pixel.
    state.arrow.target_x = static_cast<std::uint16_t>(-editor.offset_x * 16);
    state.arrow.target_y = static_cast<std::uint16_t>((31 - editor.offset_y) * 16);
    editor.latches = latches_after_choice(state.arrow.target_y);
    const auto key = keyboard_key(editor, content.keyboard_text);
    if (key == 0x40) { // OK at $80:A52A
        press_keyboard_ok(state);
        return;
    }
    if (key == 0x3c) { // backspace
        erase_keyboard_character(state, content);
        return;
    }
    play_menu_sound(state, MenuSound::result); // $80:A449, before the length test
    if (editor.length + 1U == editor.length_limit) return;
    editor.scratch[editor.length] = stored_character(key);
    const std::array<std::uint8_t, 2> shown{editor.scratch[editor.length], 0xff};
    print_text(state.text, state.printer, shown, content.character_table);
    ++editor.length;
}

void read_keyboard_pad(FrontEndState& state, const FrontEndContent& content, std::uint16_t pad) {
    auto& editor = state.keyboard;
    const auto input = keyboard_input(pad);
    switch (input) {
    case KeyboardInput::none: // $80:A297-A29C
        editor.repeat_wait = 0;
        editor.latches = 0;
        return;
    case KeyboardInput::left:
    case KeyboardInput::right: {
        const bool left = input == KeyboardInput::left;
        if (held_key_repeats(editor, left ? latch_left : latch_right)) step_key_column(state, left);
        return;
    }
    case KeyboardInput::up:
    case KeyboardInput::down: {
        const bool up = input == KeyboardInput::up;
        if (first_press(editor, up ? latch_up : latch_down)) step_key_row(state, up);
        return;
    }
    case KeyboardInput::ok: press_keyboard_ok(state); return; // no latch: every frame held
    case KeyboardInput::erase:
        if (!first_press(editor, latch_erase)) return;
        // $80:A36E-A37E: the arrow to the backspace key, then the deletion.
        state.arrow.target_x = 0x0100;
        state.arrow.target_y = 0x04e0;
        erase_keyboard_character(state, content);
        return;
    case KeyboardInput::choose:
        if (editor.latches & latch_choice) return;
        choose_key(state, content);
        return;
    }
}

} // namespace

void rename_keyboard_frame(FrontEndState& state, const FrontEndContent& content,
                           FrontEndPads pads) {
    // $80:A207-A22E: the cursor's first place, then the OAM copy; the first poll is the next
    // frame's.
    if (state.script_frame == 1) {
        initialize_keyboard_cursor(state);
        copy_oam(state);
        load_text(state, state.slide.shown_half);
        return;
    }
    copy_oam(state);
    load_text(state, state.slide.shown_half);
    // $80:A231 positions the cursor before polling, so a direction affects OAM one
    // frame after it changes the key offsets.
    position_keyboard_cursor(state);
    read_keyboard_pad(state, content, gated_pads(state, pads));
}

void rename_commit_frame(FrontEndState& state, const FrontEndContent& content) {
    // The save path makes one final OAM upload after the arrow's frame update.
    // The keyboard's preceding frame retained the older sprite position.
    copy_oam(state);
    if (state.league.editor_active) {
        const auto first = static_cast<std::size_t>(state.league.slot) * 32U;
        std::copy(state.keyboard.scratch.begin(), state.keyboard.scratch.end(),
                  state.records.league_names.begin() + first);
        state.records.active_league_members = state.league.members;
        state.records.league_naming = false;
        state.menu.selection = 0;
        high_bits(state, 123) |= hidden_bit(123);
        state.logo.raised = false;
        state.screen = FrontEndScreen::rename_return;
        return;
    }
    const auto first = static_cast<std::size_t>(state.options_rider) * 16U;
    std::copy_n(state.keyboard.scratch.begin(), 8, state.records.rider_names.begin() + first);
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
    state.league.editor_active = false;
    return_to_options_menu(state, content);
}

} // namespace unirally::front_end_screens
