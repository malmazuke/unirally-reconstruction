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
// Wins of 655 and more are halved with the races until below it, so that 100 times them fits a
// word (`$83:956D`); the division is `$80:9719`'s, unsigned, 0xFFFF for no races. The words are
// left in `$77:10F9` and `$77:10FB`.
std::uint16_t versus_percentage(OnePlayerRecords& records, unsigned rider) {
    constexpr std::uint16_t largest_wins = 0x028e, percent = 100, no_races = 0xffff;
    records.versus_percentage_races = records.versus_races[rider];
    const auto wins = records.versus_wins[rider];
    if (wins == 0) return 0;
    records.versus_percentage_wins = wins;
    while (records.versus_percentage_wins > largest_wins) {
        records.versus_percentage_wins =
            static_cast<std::uint16_t>(records.versus_percentage_wins >> 1U);
        records.versus_percentage_races =
            static_cast<std::uint16_t>(records.versus_percentage_races >> 1U);
    }
    const auto scaled = static_cast<std::uint16_t>(records.versus_percentage_wins * percent);
    return records.versus_percentage_races == 0
             ? no_races
             : static_cast<std::uint16_t>(scaled / records.versus_percentage_races);
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
    // The menu's tests take either pad the gate lets in (`$80:B6D3-B82B`, R-0095).
    const auto pad = static_cast<std::uint16_t>((state.pad_one_ignored ? 0U : pads.one)
                                                | (state.pad_two_ignored ? 0U : pads.two));
    if (!(pad & pad_up)) state.latches.up = false;
    if (!(pad & pad_down)) state.latches.down = false;
    // The original's menu is `$80:B93C`'s (from $80:ADE3), whose every move plays the navigation
    // sound ($80:B9B1 Up, $80:BA13 Down).
    if ((pad & pad_up) && !state.latches.up) {
        state.latches.up = true;
        play_menu_sound(state, MenuSound::navigate);
        if (state.menu.selection > 0) --state.menu.selection;
    } else if ((pad & pad_down) && !state.latches.down) {
        state.latches.down = true;
        play_menu_sound(state, MenuSound::navigate);
        if (state.menu.selection + 1 < choices) ++state.menu.selection;
    }
    state.arrow.target_y =
        static_cast<std::uint16_t>(continue_first_y + state.menu.selection * continue_row_y);
    if (!(pad & choose_buttons)) return;
    state.pad_one_ignored = state.pad_two_ignored = false; // $80:C0E0, `$83:9558`
    switch (state.menu.selection) {
    case 0: // NEXT TRACK, with the select sound ($80:AEAD)
        play_menu_sound(state, MenuSound::select);
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

// The header's printing and the ranking run past the frame's end into the next (R-0095: the
// captured results placed a record, so `$80:C786` also ran long); then the rows, one a frame,
// then `$80:937B` and the slide back in (`$80:E27E`), after which the icons show
// (`$80:F9DB-F9DE`).
void vs_champions_entry_frame(FrontEndState& state, const FrontEndContent& content) {
    const auto frame = state.script_frame;
    if (frame == champions_overrun_frame) return;
    if (frame <= champions_overrun_frame + ranking_rows) {
        print_champion_row(state, content, frame - champions_overrun_frame - 1);
        return;
    }
    if (frame == champions_overrun_frame + ranking_rows + 1) {
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
