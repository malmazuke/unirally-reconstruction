// The two-human result continuation and the VS ranking (R-0071). The menu and table text are
// ROM streams; all counts below are race results, not a separate progression system.
#include "front_end_screens.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

namespace unirally::front_end_screens {
namespace {

constexpr std::uint8_t choices = 5, ranking_rows = 8;
constexpr std::uint16_t continue_x = 0x0100, continue_first_y = 0x0580, continue_row_y = 0x0180;

void print_continue(FrontEndState& state, const FrontEndContent& content) {
    state.text.words.fill(cleared_text);
    for (unsigned row = 0; row < choices; ++row) {
        const auto stream = nth_string(content.local_continue_text, row);
        print_text(state.text, state.printer, {stream.data(), stream.size() + 1},
                   content.character_table);
    }
}

void print_champions(FrontEndState& state, const FrontEndContent& content) {
    state.text.words.fill(cleared_text);
    TextVariables variables;
    variables.rider_names = state.records.rider_names;
    variables.place_object = [&](unsigned object, unsigned position) {
        place_printed_object(state, object, position);
    };
    print_text(state.text, state.printer, content.vs_champions_header, content.character_table,
               &variables);
    std::array<std::uint8_t, 16> ranked{};
    for (unsigned rider = 0; rider < ranked.size(); ++rider)
        ranked[rider] = static_cast<std::uint8_t>(rider);
    std::stable_sort(ranked.begin(), ranked.end(), [&](std::uint8_t a, std::uint8_t b) {
        return state.records.statistics[a][1] > state.records.statistics[b][1];
    });
    for (unsigned row = 0; row < ranking_rows; ++row) {
        const auto rider = ranked[row];
        load_cgram(state, asset(content, first_rider_palette + rider), 0xf0 - row * 16);
        oam_byte(state, 104 + row, 3) = static_cast<std::uint8_t>(0x1f - row * 2);
        const auto& statistics = state.records.statistics[rider];
        const auto races = statistics[0], wins = statistics[1];
        // The cold-start, first-match domain gives today's count equal to the wins count.
        // Later-day reset behavior has not been recovered.
        const auto percentage = races == 0 ? 0U : 100U * wins / races;
        variables.word = [=](std::uint16_t address) -> std::uint16_t {
            switch (address) {
            case 0xb2: return rider;
            case 0xb4: return wins;
            case 0xb6: return static_cast<std::uint16_t>(percentage);
            case 0xb8: return wins;
            default: return 0;
            }
        };
        std::vector<std::uint8_t> stream(content.vs_champions_row.begin(),
                                         content.vs_champions_row.end());
        for (std::size_t at = 0; at + 2 < stream.size(); ++at)
            if (stream[at] == 0xfe) stream[at + 2] = static_cast<std::uint8_t>(8 + row * 2);
        print_text(state.text, state.printer, stream, content.character_table, &variables);
    }
    high_bits(state, 104) = high_bits(state, 108) = four_shown;
}

} // namespace

void enter_local_continue(FrontEndState& state) {
    state.menu.selection = 0;
    state.latches = {};
    state.screen = FrontEndScreen::local_continue_entry;
}

void local_continue_entry_frame(FrontEndState& state, const FrontEndContent& content) {
    switch (state.script_frame) {
    case 1:
        copy_oam(state);
        print_continue(state, content);
        return;
    case 2:
        copy_oam(state);
        load_text(state, state.slide.hidden_half);
        send_arrow_off(state);
        start_slide(state, content, false);
        return;
    default:
        if (!slide_frame(state, content)) return;
        state.arrow.target_x = continue_x;
        state.arrow.target_y = continue_first_y;
        state.latches = {};
        state.screen = FrontEndScreen::local_continue;
    }
}

void local_continue_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    copy_oam(state);
    step_decorations(state, content);
    const auto pad = pads.one;
    if (!(pad & pad_up)) state.latches.up = false;
    if (!(pad & pad_down)) state.latches.down = false;
    if ((pad & pad_up) && !state.latches.up) {
        state.latches.up = true;
        if (state.menu.selection > 0) --state.menu.selection;
    } else if ((pad & pad_down) && !state.latches.down) {
        state.latches.down = true;
        if (state.menu.selection + 1 < choices) ++state.menu.selection;
    }
    state.arrow.target_y =
        static_cast<std::uint16_t>(continue_first_y + state.menu.selection * continue_row_y);
    if (!(pad & choose_buttons)) return;
    switch (state.menu.selection) {
    case 0: // NEXT TRACK
        state.tour_menu.track = static_cast<std::uint8_t>(state.tour_menu.track + 1);
        enter_now_playing(state);
        return;
    case 1: // SAME TRACK
        enter_now_playing(state);
        return;
    case 2: // SELECT TRACK
        enter_track_menu(state, false);
        return;
    case 3: // SELECT TOUR
        enter_tour_menu(state);
        return;
    default: // QUIT
        print_main_menu(state, content);
        state.screen = FrontEndScreen::main_menu_return;
        return;
    }
}

void enter_vs_champions(FrontEndState& state) {
    state.screen = FrontEndScreen::vs_champions_entry;
}

void vs_champions_entry_frame(FrontEndState& state, const FrontEndContent& content) {
    switch (state.script_frame) {
    case 1:
        copy_oam(state);
        print_champions(state, content);
        return;
    case 2:
        copy_oam(state);
        load_text(state, state.slide.hidden_half);
        send_arrow_off(state);
        start_slide(state, content, true);
        return;
    default:
        if (!slide_frame(state, content)) return;
        state.latches = {};
        state.screen = FrontEndScreen::vs_champions;
    }
}

void vs_champions_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    copy_oam(state);
    step_decorations(state, content);
    if (!(pads.one & choose_buttons)) {
        state.latches.moved = false;
        return;
    }
    if (state.latches.moved) return;
    state.latches.moved = true;
    enter_vs_challenger(state);
}

} // namespace unirally::front_end_screens
