// The two-human result continuation and VS CHAMPIONS (R-0071, R-0095): the five choices, and the
// VS ranking by today's wins from the VS counts the results keep. The menu and table text are ROM
// streams.
#include "front_end_screens.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace unirally::front_end_screens {
namespace {

constexpr std::uint8_t ranking_rows = 8;
constexpr unsigned menu_text_palette = 28;
// The continuation's items, NEXT TRACK to QUIT: `$009D`, the last, 4 ($80:ADFC); the arrow's
// columns (8-pixel units) from `$80:AE81`; y is (24 * item + 0x58) * 16 ($80:B962-B975), each
// move 0x180 from the last target, past the last item from 0x400 (`$80:BA1F`).
constexpr std::uint8_t last_choice = 4;
constexpr std::array<std::uint8_t, last_choice + 1> choice_columns{2, 2, 0, 1, 8};
constexpr std::uint16_t row_spacing = 0x0180, wrap_top_y = 0x0400;

std::uint16_t choice_x(unsigned item) {
    return static_cast<std::uint16_t>(choice_columns[item] * 128U);
}

std::uint16_t choice_y(unsigned item) {
    return static_cast<std::uint16_t>((24 * item + 0x58) * 16);
}

void print_continue(FrontEndState& state, const FrontEndContent& content) {
    state.text.words.fill(cleared_text);
    for (unsigned row = 0; row <= last_choice; ++row) {
        const auto stream = nth_string(content.local_continue_text, row);
        print_text(state.text, state.printer, {stream.data(), stream.size() + 1},
                   content.character_table);
    }
}

// $80:F905-F925 and `$80:F02B`: the sixteen riders with today's VS wins, sorted by moving the
// lowest of the unsorted part to its end, a later equal value taking the place (from 0x7FFF,
// unsigned): the most wins today first.
std::array<std::uint8_t, 16> versus_ranking(const OnePlayerRecords& records) {
    std::array<std::uint8_t, 16> riders{};
    std::array<std::uint16_t, 16> today{};
    for (unsigned rider = 0; rider < riders.size(); ++rider) {
        riders[rider] = static_cast<std::uint8_t>(rider);
        today[rider] = records.versus_today[rider];
    }
    constexpr std::uint16_t first_lowest = 0x7fff;
    for (unsigned last = riders.size(); last-- > 0;) {
        unsigned selected = 0;
        std::uint16_t lowest = first_lowest;
        for (unsigned k = 0; k <= last; ++k)
            if (today[k] <= lowest) {
                lowest = today[k];
                selected = k;
            }
        std::swap(riders[selected], riders[last]);
        std::swap(today[selected], today[last]);
    }
    return riders;
}

// $80:F974-F98F, `$83:9D80`: a rider's VS wins as a percentage of its VS races, 0 without a win.
// The words are left in `$77:10F9` and `$77:10FB`.
std::uint16_t versus_percentage(OnePlayerRecords& records, unsigned rider) {
    records.versus_percentage_races = records.versus_races[rider];
    const auto wins = records.versus_wins[rider];
    if (wins == 0) return 0;
    return cartridge_percentage(records, records.versus_races[rider], wins);
}

// $80:F949-F9BD, one row a frame: the rider's colours at 0x80 + 16 x row (the row's icon, entry
// 104 + row, takes palette `row`), then its name, wins, today's wins and percentage.
void print_champion_row(FrontEndState& state, const FrontEndContent& content, unsigned row) {
    const auto rider = versus_ranking(state.records)[row];
    constexpr unsigned first_row_colour = 0x80;
    load_cgram(state, asset(content, first_rider_palette + rider), first_row_colour + row * 16);
    const auto percentage = versus_percentage(state.records, rider);
    const auto wins = state.records.versus_wins[rider], today = state.records.versus_today[rider];
    TextVariables variables;
    variables.rider_names = state.records.rider_names;
    variables.word = [=](std::uint16_t address) -> std::uint16_t {
        switch (address) {
        case 0xb2: return rider;
        case 0xb4: return wins;
        case 0xb6: return percentage;
        case 0xb8: return today;
        default: return 0;
        }
    };
    // $80:F9A5-F9BA: the template's rows step down two for each row.
    std::vector<std::uint8_t> stream(content.vs_champions_row.begin(),
                                     content.vs_champions_row.end());
    for (std::size_t at = 0; at + 2 < stream.size(); ++at)
        if (stream[at] == 0xfe) stream[at + 2] = static_cast<std::uint8_t>(8 + row * 2);
    print_text(state.text, state.printer, stream, content.character_table, &variables);
}

} // namespace

void enter_local_continue(FrontEndState& state) {
    state.menu.selection = 0;
    state.latches = {};
    state.continuation_back = FrontEndScreen::boot;
    state.continuation_returning = false;
    state.slide_word = 1; // $80:ADF8
    state.screen = FrontEndScreen::local_continue_entry;
}

bool returns_to_continuation(const FrontEndState& state, FrontEndScreen from) {
    return state.mode != FrontEndMode::league && state.continuation_back == from;
}

// $80:ADFA: back from a choice, in the frame the choice's screen returns, the menu printed again
// without `$80:ADE5-ADF4`'s palettes, then slid back in (`$80:EF81`) with the arrow left where it
// was (R-0095).
void return_to_local_continue(FrontEndState& state, const FrontEndContent& content) {
    enter_local_continue(state);
    state.continuation_returning = true;
    state.slide_word = 2; // $80:ADFA from a choice's Y
    print_continue(state, content);
}

void local_continue_entry_frame(FrontEndState& state, const FrontEndContent& content) {
    // Back from a choice the menu is already printed: the slide starts a frame earlier.
    const auto frame = state.script_frame + (state.continuation_returning ? 1 : 0);
    switch (frame) {
    case 1: // $80:ADE5-ADF4: the object and text palettes again (VS CHAMPIONS' rows took them)
        copy_oam(state);
        load_object_palette(state, content); // $83:91F7
        load_cgram(state, asset(content, menu_text_palette), 0xd0);
        print_continue(state, content);
        return;
    case 2:
        // Back from a choice the OAM is copied next in the slide's first pass (R-0095).
        if (!state.continuation_returning) copy_oam(state);
        load_text(state, state.slide.hidden_half);
        if (!state.continuation_returning) send_arrow_off(state);
        start_slide(state, content, state.continuation_returning);
        return;
    default:
        if (slide_frame(state, content)) state.screen = FrontEndScreen::local_continue;
    }
}

namespace {

// $80:AE96-AECC: the next track, after the last (44) the first, on until its tour is open
// (`$83:9F14` against `$80:E842`), with the select sound for each track tried ($80:AEAD;
// R-0095).
void choose_next_track(FrontEndState& state, const FrontEndContent& content) {
    constexpr std::uint8_t tracks = 45;
    auto& track = state.tour_menu.track;
    do {
        track = static_cast<std::uint8_t>(track + 1 >= tracks ? 0 : track + 1);
        play_menu_sound(state, MenuSound::select);
        state.tour_menu.tour = static_cast<std::uint8_t>(track / tracks_per_tour); // $83:99A8
    } while (!tour_open(state, content, state.tour_menu.tour));
}

// $80:B93C: the cursor on NEXT TRACK and the arrow sent to it.
void open_local_continue(FrontEndState& state) {
    state.menu.selection = 0;
    state.latches = {};
    state.arrow.target_x = choice_x(0);
    state.arrow.target_y = choice_y(0);
}

// $80:B983-BA5E, the generic list menu's moves (R-0095): Down (or Select) before Up, once a
// press, wrapping past either end, each with the navigation sound (`$80:B178`).
void move_continue_cursor(FrontEndState& state, std::uint16_t pad) {
    auto& selection = state.menu.selection;
    auto& target_y = state.arrow.target_y;
    const bool down = (pad & (pad_down | pad_select)) != 0, is_up = !down && (pad & pad_up) != 0;
    if (!down && !is_up) {
        state.latches.moved = false;
        return;
    }
    if (state.latches.moved) return;
    state.latches.moved = true;
    play_menu_sound(state, MenuSound::navigate);
    if (down) {
        if (++selection > last_choice) {
            selection = 0;
            target_y = wrap_top_y;
        }
        target_y = static_cast<std::uint16_t>(target_y + row_spacing);
    } else {
        if (selection-- == 0) {
            selection = last_choice;
            target_y = choice_y(last_choice + 1U);
        }
        target_y = static_cast<std::uint16_t>(target_y - row_spacing);
    }
    state.arrow.target_x = choice_x(selection);
}

} // namespace

// After the slide `$80:A858`'s two frames (the base palette's halves), then the generic list
// menu's loop (`$80:B93C`, from $80:AE0F), which turns the PICK TRACK markers (`$80:EB23`) and does
// not step the decorations (R-0095).
void local_continue_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    if (state.script_frame <= 2) {
        copy_oam(state);
        if (state.script_frame == 1) {
            load_cgram(state, asset(content, base_palette_low), 0);
            return;
        }
        load_cgram(state, asset(content, base_palette_high), 0x40);
        open_local_continue(state);
        turn_markers(state, content);
        return;
    }
    copy_oam(state); // $80:D1EC
    // The menu's tests take either pad the gate lets in (`$80:B6D3-B82B`, R-0095).
    const auto pad = gated_pads(state, pads);
    // $80:B74A, then $80:AE12-AE1A: Y or X leaves the loop and comes straight back, no move.
    if (pad & back_buttons) {
        turn_markers(state, content);
        return;
    }
    if (!(pad & choose_buttons)) { // $80:B71D
        move_continue_cursor(state, pad);
        turn_markers(state, content);
        return;
    }
    // The pad gate stays as the challenger's pick left it while the continuation (`$80:C0DD JSR
    // $ADE3`) runs its choice's screens; `$80:C0E0` opens both pads once it returns, on NOW
    // PLAYING's Race or on QUIT (R-0095).
    switch (state.menu.selection) {
    case 0: // NEXT TRACK
        choose_next_track(state, content);
        enter_now_playing(state);
        state.continuation_back = FrontEndScreen::now_playing;
        return;
    case 1: // SAME TRACK
        enter_now_playing(state);
        state.continuation_back = FrontEndScreen::now_playing;
        return;
    case 2:                   // SELECT TRACK
        state.slide_word = 1; // $80:AEF5
        enter_track_menu(state, false);
        state.continuation_back = FrontEndScreen::track_menu;
        return;
    case 3: // SELECT TOUR: `$00AC` = 2, `$80:E550` with neither `$80:A858` nor `$80:A82B` before it
        state.slide_word = 2; // $80:AF1D
        return_to_tour_menu(state, false);
        state.continuation_back = FrontEndScreen::tour_menu;
        return;
    default:                          // QUIT
        gate_pads(state, true, true); // $80:C0E0
        print_main_menu(state, content);
        state.screen = FrontEndScreen::main_menu_return;
        return;
    }
}

// $80:F8E7-F925, in the frame the result's records end: the arrow sent off, the icons 104-111
// hidden (`$0C1A`, `$0C1B`), the text cleared and the header printed with the icons placed; the
// rows follow a frame each.
void enter_vs_champions(FrontEndState& state, const FrontEndContent& content) {
    send_arrow_off(state);
    high_bits(state, 104) = high_bits(state, 108) = four_hidden;
    state.text.words.fill(cleared_text);
    TextVariables variables;
    variables.rider_names = state.records.rider_names;
    variables.place_object = [&](unsigned object, unsigned position) {
        place_printed_object(state, object, position);
    };
    print_text(state.text, state.printer, content.vs_champions_header, content.character_table,
               &variables);
    state.screen = FrontEndScreen::vs_champions_entry;
}

// The header's printing and the ranking, which may run past the frame's end into the next
// (`champions_overrun_frames`); then the rows, one a frame, then `$80:937B` and the slide back in
// (`$80:E27E`), after which the icons show (`$80:F9DB-F9DE`).
void vs_champions_entry_frame(FrontEndState& state, const FrontEndContent& content) {
    const auto overrun = champions_overrun_frames(state);
    const auto frame = state.script_frame;
    if (frame <= overrun) return;
    if (frame <= overrun + ranking_rows) {
        print_champion_row(state, content, frame - overrun - 1);
        return;
    }
    if (frame == overrun + ranking_rows + 1) {
        load_text(state, state.slide.hidden_half);
        start_slide(state, content, true);
        return;
    }
    if (!slide_frame(state, content)) return;
    high_bits(state, 104) = high_bits(state, 108) = four_shown;
    state.race_result.press_seen = false;
    state.screen = FrontEndScreen::vs_champions;
}

// $80:C206: a press on two frames running, of any button of either pad, each frame copying the
// OAM, reading the pads and stepping the decorations; then `$80:C236` hides entries 30-35 and
// `$80:F9E4` the icons, and the race's loser picks a challenger.
void vs_champions_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    copy_oam(state);
    step_decorations(state, content);
    auto& press_seen = state.race_result.press_seen;
    const bool press = pads.one != 0 || pads.two != 0;
    if (!press_seen || !press) {
        press_seen = press;
        return;
    }
    high_bits(state, 32) = four_hidden;
    high_bits(state, 28) = static_cast<std::uint8_t>((high_bits(state, 28) & 0x0fU) | 0x50U);
    high_bits(state, 104) = high_bits(state, 108) = four_hidden;
    enter_vs_challenger(state);
}

bool versus_tie(const RaceTimes& times) {
    return times.stunt_event ? times.player_score == times.opponent_score
                             : times.player_total == times.opponent_total;
}

// $80:C02C-C03F, in the result exit's first frame: the text cleared and REMATCH printed.
void enter_vs_rematch(FrontEndState& state, const FrontEndContent& content) {
    if (content.vs_rematch_text.empty())
        throw std::invalid_argument("a VS tie's REMATCH is not in the pack (profile v38)");
    state.text.words.fill(cleared_text);
    print_text(state.text, state.printer, content.vs_rematch_text, content.character_table);
    state.race_result.released = state.race_result.press_seen = false;
    state.screen = FrontEndScreen::vs_rematch;
}

// After a frame wait the text is shown; then the one-run result's waits (`$80:C24C`, `$80:C206`:
// both pads released, then a press on two frames running, each frame copying the OAM and
// stepping the decorations), `$80:C236`'s hidden entries, and NOW PLAYING again (`$80:C048` to
// `$80:BFE0`), the same pairing, nothing counted.
void vs_rematch_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    if (state.script_frame == 1) {
        load_text(state, state.slide.shown_half);
        return;
    }
    copy_oam(state);
    step_decorations(state, content);
    auto& result = state.race_result;
    const bool press = pads.one != 0 || pads.two != 0;
    if (!result.released) {
        result.released = !press;
        return;
    }
    if (!result.press_seen || !press) {
        result.press_seen = press;
        return;
    }
    high_bits(state, 32) = four_hidden;
    high_bits(state, 28) = static_cast<std::uint8_t>((high_bits(state, 28) & 0x0fU) | 0x50U);
    enter_now_playing(state);
}

} // namespace unirally::front_end_screens
