// NOW PLAYING (R-0056): after a track is chosen, the rider against the opponent the medal sets,
// the race's kind and track, the tour's picture and the track's record, slid in (`$80:B18D`);
// the loop (`$80:B467`) moves the arrow between Race and Exit. Race fades out to the race
// (`$80:9885`); Exit goes back to the main menu; Y or X slides PICK TRACK back in.
#include "front_end_screens.hpp"

#include <array>
#include <stdexcept>
#include <vector>

namespace unirally::front_end_screens {

namespace {

constexpr unsigned base_palette_low = 35, base_palette_high = 36, first_rider_palette = 6;
constexpr std::uint8_t tracks_per_tour = 5, stunt_event = 2, someone = 0x10, first_computer = 0x11,
                       last_medal_opponent = 0x13, anti_uni = 0x14, hunter = 8;
constexpr std::uint8_t end_of_text = 0xff, blank = '_';

// `front-end.now-playing-text` ($80:B4F6-B57D): its streams by offset.
constexpr std::size_t title_at = 0x00, versus_at = 0x0f, record_at = 0x27, hi_score_at = 0x3b,
                      qualifying_at = 0x47, race_exit_at = 0x79, picture_place_at = 0x86;
// `front-end.race-kind-words` ($80:B205): "racing_on", "over", "_laps_on", "doing_stunts_on".
constexpr std::size_t racing_at = 0, over_at = 10, laps_on_at = 15, stunts_at = 24;
// `front-end.time-words` ($80:FC5C): `_quit___` and `_no_time`.
constexpr std::size_t quit_at = 0, no_time_at = 9;
constexpr std::uint16_t quit_time = 0xea61, no_time = 0xea60;

// The rows of the lines: the rider's 6, the opponent's 10, the race's 13.
constexpr std::uint8_t rider_row = 6, opponent_row = 10, race_row = 13;

// The objects: entries 100-103 the 1P and 2P marks, 104-106 the three riders' icons (their tiles
// turned by the decoration animator), 112-115 unused here.
constexpr std::size_t first_icon = 104, icons = 8;

// The arrow's places: Race and Exit on row 24 ($80:B457, $80:B4D4).
constexpr std::uint16_t race_x = 0x0300, exit_x = 0x0680, arrow_y = 0x0c80;

// Pad 1 only: Right, Left and Select ($80:B8F1) move; choose B, Start or A; back Y or X.
constexpr std::uint16_t right = 0x0100, left = 0x0200, select = 0x2000;
constexpr std::uint16_t choose_buttons = 0x9080, back_buttons = 0x4040;

using Text = std::vector<std::uint8_t>;

// The bytes of `table` from `at` up to its 0xFF, without it.
void append_until_end(Text& text, std::span<const std::uint8_t> table, std::size_t at) {
    for (; at < table.size() && table[at] != end_of_text; ++at) text.push_back(table[at]);
    if (at == table.size()) throw std::invalid_argument("front-end text is not terminated");
}

std::span<const std::uint8_t> stream_at(const FrontEndContent& content, std::size_t at) {
    return content.now_playing_text.subspan(at);
}

std::uint8_t track_of(const FrontEndState& state) {
    return state.tour_menu.track;
}

// `$77:074B` as `$83:9983` sets it: 0 a race, 1 laps, 2 a stunt event, by the track's place.
std::uint8_t race_kind(const FrontEndState& state) {
    const auto place = static_cast<std::uint8_t>(track_of(state) % tracks_per_tour);
    return place < 3 ? place : static_cast<std::uint8_t>(place - 3);
}

// `$83:8C7B`: `_m:ss.cc`, the minute from `5` above 0x7FFF (30,000 hundredths less).
Text time_text(const FrontEndContent& content, std::uint16_t time) {
    Text text;
    if (time == quit_time || time == no_time) {
        append_until_end(text, content.time_words, time == quit_time ? quit_at : no_time_at);
        return text;
    }
    unsigned rest = time, first_minute = '0';
    if (time & 0x8000U) {
        rest -= 30000;
        first_minute = '5';
    }
    const auto digit = [&](unsigned place) {
        const auto value = rest / place;
        rest %= place;
        return static_cast<std::uint8_t>('0' + value);
    };
    const auto minutes = static_cast<std::uint8_t>(first_minute + rest / 6000);
    rest %= 6000;
    const auto tens = digit(1000), seconds = digit(100), tenths = digit(10), hundredths = digit(1);
    return {blank, minutes, ':', tens, seconds, '.', tenths, hundredths};
}

// $80:B57E: a rider's line, "FC 09" (the caller sets the row), the name to its first blank, the
// icon (`EF n`) and, for a real rider, the best on this track in brackets.
Text name_line(const FrontEndState& state, const FrontEndContent& content, std::uint8_t rider,
               std::uint8_t icon) {
    Text line{0xfc, 0x09};
    const auto record = content.rider_names.subspan(rider * 16U, 16);
    std::size_t at = 0;
    while (at < record.size() && record[at] != end_of_text && record[at] != ' '
           && record[at] != blank)
        line.push_back(record[at++]); // `$80:933C`, the terminator made a blank
    line.insert(line.end(), {blank, 0xef, icon, blank, blank});
    if (rider < someone) {
        const auto best = state.records.best[rider * 50U + track_of(state)];
        if (race_kind(state) != stunt_event) {
            auto time = time_text(content, best);
            time.front() = '(';
            line.insert(line.end(), time.begin(), time.end());
            line.push_back(')');
        } else {
            const auto score = five_digit_text(best); // `$83:8BE7`, the blanks left out
            line.push_back('(');
            for (std::size_t k = 0; k < 5; ++k)
                if (score[k] != blank) line.push_back(score[k]);
            line.push_back(')');
        }
    }
    line.push_back(end_of_text);
    return line;
}

// The race line on row 13: "racing on", "over N laps on" or "doing stunts on", and the track.
Text race_line(const FrontEndState& state, const FrontEndContent& content) {
    Text line{0xfc, race_row};
    const auto kind = race_kind(state);
    if (kind == 1) {
        append_until_end(line, content.race_kind_words, over_at);
        line.push_back(blank);
        const auto laps = content.laps[track_of(state)];
        if (laps == 10)
            line.insert(line.end(), {'1', '0'});
        else
            line.push_back(static_cast<std::uint8_t>('0' + laps));
        append_until_end(line, content.race_kind_words, laps_on_at);
    } else {
        append_until_end(line, content.race_kind_words, kind == 0 ? racing_at : stunts_at);
    }
    line.push_back(blank);
    std::size_t at = 0;
    for (unsigned k = 0; k < track_of(state); ++k) {
        while (content.track_names[at] != end_of_text) ++at;
        ++at;
    }
    append_until_end(line, content.track_names, at);
    line.push_back(end_of_text);
    return line;
}

// The opponent: in 1P the medal to race for picks BRONSEN, SILVIA or GOLDWYN ($80:B31F-B346);
// on HUNTER's races ANTI-UNI ($80:B361).
std::uint8_t opponent_for(const FrontEndState& state) {
    const auto opponent = static_cast<std::uint8_t>(state.tour_menu.medal + first_computer);
    return opponent > last_medal_opponent ? last_medal_opponent : opponent;
}

// $80:B19A-B3DA: the marks' places, then the text map, all in one frame.
void print_now_playing(FrontEndState& state, const FrontEndContent& content) {
    auto& now = state.now_playing;
    for (const std::size_t k : {24U, 25U, 28U}) state.oam_buffer[oam_high_table + k] = 0x55;
    for (const unsigned entry : {100U, 102U}) {
        oam_byte(state, entry, 1) = 0x2f;
        oam_byte(state, entry, 3) = 0x21;
    }
    for (const unsigned entry : {101U, 103U}) {
        oam_byte(state, entry, 1) = 0x4f;
        oam_byte(state, entry, 3) = 0x23;
    }
    state.text.words.fill(cleared_text);
    std::uint16_t qualifying_score{}; // `$00B6`, the stunt events' FD
    TextVariables variables;
    variables.word = [&](std::uint16_t) { return qualifying_score; };
    variables.place_object = [&](unsigned object, unsigned position) { // `$80:C6D5`
        const auto entry = static_cast<unsigned>(first_icon + object);
        oam_byte(state, entry, 0) = static_cast<std::uint8_t>((position & 31U) * 8 - 1);
        oam_byte(state, entry, 1) = static_cast<std::uint8_t>((position & ~31U) / 4 - 1);
    };
    const auto print = [&](std::span<const std::uint8_t> text) {
        print_text(state.text, state.printer, text, content.character_table, &variables);
    };
    const auto print_line = [&](Text line, std::uint8_t row) {
        line[1] = row;
        print(line);
    };
    print(stream_at(content, title_at));
    print(race_line(state, content));
    draw_tour_picture(state, content, state.tour_menu.tour,
                      word_at(content.now_playing_text, picture_place_at) / 2U);
    print_line(name_line(state, content, state.rider_menu.rider, 0), rider_row);
    print(stream_at(content, versus_at));
    now.opponent = opponent_for(state);
    if (race_kind(state) == stunt_event) {
        // The qualifying score in place of the opponent (`$83:9EEB`: by tour and best medal).
        const auto best = state.records.medals[state.tour_menu.tour * 16U + state.rider_menu.rider];
        const auto level = (best & 3U) == 3 ? 2U : best & 3U;
        qualifying_score =
            word_at(content.qualifying_scores, (state.tour_menu.tour * 3U + level) * 2);
        print(stream_at(content, qualifying_at));
    } else {
        if (state.tour_menu.tour >= hunter) now.opponent = anti_uni;
        print_line(name_line(state, content, now.opponent, 1), opponent_row);
    }
    // The record line: "record:" (or "hi score:"), then the holder's line without its "FC 09".
    now.record_holder = state.records.record_holder[track_of(state)];
    Text record;
    append_until_end(record, content.now_playing_text,
                     race_kind(state) == stunt_event ? hi_score_at : record_at);
    record.push_back(blank);
    const auto holder = name_line(state, content, now.record_holder, 2);
    record.insert(record.end(), holder.begin() + 2, holder.end());
    print(record);
    print(stream_at(content, race_exit_at));
    state.oam_buffer[oam_high_table + 26] = state.oam_buffer[oam_high_table + 27] = 0x55;
}

// $80:D420: the icons back at (1, 1), hidden.
void hide_icons(FrontEndState& state) {
    for (std::size_t k = 0; k < icons; ++k) {
        oam_byte(state, static_cast<unsigned>(first_icon + k), 0) = 1;
        oam_byte(state, static_cast<unsigned>(first_icon + k), 1) = 1;
    }
    state.oam_buffer[oam_high_table + 26] = state.oam_buffer[oam_high_table + 27] = 0x55;
}

// $80:B42F-B465, after the slide: the 2P mark and the opponent's icon hidden for a computer
// opponent (and for a stunt event's qualifying score); the arrow on Race.
void open_now_playing(FrontEndState& state) {
    const bool computer = state.now_playing.opponent >= someone;
    state.oam_buffer[oam_high_table + 25] = computer ? 0x44 : 0x00;
    state.oam_buffer[oam_high_table + 26] =
        race_kind(state) == stunt_event && computer ? 0x44 : 0x40;
    state.arrow.target_x = race_x;
    state.arrow.target_y = arrow_y;
    state.latches = {};
    state.screen = FrontEndScreen::now_playing;
}

// $80:B471-B4F4: Right to Exit, Left to Race, Select to the other, once a press.
void move_now_playing_arrow(FrontEndState& state, std::uint16_t pad) {
    auto& target = state.arrow.target_x;
    std::uint16_t to{};
    if (pad & right)
        to = exit_x;
    else if (pad & left)
        to = race_x;
    else if (pad & select)
        to = target == race_x ? exit_x : race_x;
    else {
        state.latches = {};
        return;
    }
    if (state.latches.moved) return;
    target = to;
    state.latches = {.moved = true};
}

} // namespace

void enter_now_playing(FrontEndState& state) {
    state.now_playing.choice = NowPlayingChoice::none;
    state.screen = FrontEndScreen::now_playing_entry;
}

void now_playing_entry_frame(FrontEndState& state, const FrontEndContent& content) {
    auto& now = state.now_playing;
    switch (state.script_frame) {
    case 1:
        copy_oam(state);
        load_cgram(state, asset(content, base_palette_low), 0);
        return;
    case 2:
        copy_oam(state);
        load_cgram(state, asset(content, base_palette_high), 0x40);
        print_now_playing(state, content);
        return;
    case 3:
        copy_oam(state);
        load_cgram(state, asset(content, first_rider_palette + (state.rider_menu.rider & 15U)),
                   0x80);
        return;
    case 4: // no OAM copy
        load_cgram(state, asset(content, first_rider_palette + now.opponent), 0x90);
        load_cgram(state, asset(content, first_rider_palette + now.record_holder), 0xa0);
        load_text(state, state.slide.hidden_half);
        start_slide(state, content, false);
        return;
    default:
        if (!slide_frame(state, content)) return;
        open_now_playing(state);
        step_decorations(state, content);
        return;
    }
}

void now_playing_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    auto& now = state.now_playing;
    copy_oam(state); // $80:D1EC
    const auto pad = pads.one;
    move_now_playing_arrow(state, pad);
    if (pad & back_buttons)
        now.choice = NowPlayingChoice::back;
    else if (pad & choose_buttons)
        now.choice =
            state.arrow.target_x == race_x ? NowPlayingChoice::race : NowPlayingChoice::exit;
    if (now.choice == NowPlayingChoice::none) {
        step_decorations(state, content);
        return;
    }
    hide_icons(state);
    state.oam_buffer[oam_high_table + 28] = state.oam_buffer[oam_high_table + 25] = 0x55;
    switch (now.choice) {
    case NowPlayingChoice::back: enter_track_menu(state, true); return;
    case NowPlayingChoice::race: state.screen = FrontEndScreen::race_fade; return;
    default: // Exit: the main loop prints the main menu again and slides it back in (`$80:ACD5`)
        print_main_menu(state, content);
        state.menu.selection = 0;
        state.screen = FrontEndScreen::main_menu_return;
        return;
    }
}

void race_fade_frame(FrontEndState& state) {
    constexpr std::uint32_t fade_frames = 7;
    copy_oam(state);
    const auto step = state.script_frame - 1;
    state.registers.brightness = static_cast<std::uint8_t>(13 - 2 * step);
    if (state.script_frame < fade_frames) return;
    state.registers.force_blank = true;
    state.mode_chosen = true;
    state.mode = FrontEndMode::one_player;
}

} // namespace unirally::front_end_screens
