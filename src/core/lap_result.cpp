// The lap race's result screen (R-0058): `$80:951C`'s type-1 builder `$80:8D6E-910E`, its
// streams, and the lap graph `$80:98B3` animates until a press. The race's return before it and
// the way out after it are the one-run result's (race_result.cpp, R-0057).
#include "front_end_screens.hpp"
#include "text_printer.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace unirally::front_end_screens {

namespace {

constexpr std::size_t lap_slots = 10;

// The graph ($80:8DD9-8F49): ten dots a rider, entries 0-9 the player's laps and 10-19 the
// opponent's, and entries 20-29 the track record's line. The dots start together at (0x80, 0x6E)
// and fly to their laps' columns, 16 pixels apart from 0x3B, at heights from the graph's floor.
constexpr unsigned opponent_dots = 10, record_line = 20, record_line_entries = 10;
constexpr std::uint16_t dot_start_x = 0x0800, dot_start_y = 0x06e0, unrun_lap_y = 0x0f00;
constexpr std::uint8_t first_dot_column = 0x2b, first_line_column = 0x27, column_step = 16;
constexpr std::uint8_t player_graph_base = 0xb8, opponent_graph_base = 0xb9; // a pixel lower
constexpr std::uint16_t graph_height = 128, minimum_range = 200;             // $83:904A
constexpr std::uint8_t dot_start_oam_x = 0x80, dot_start_oam_y = 0x6e;
constexpr std::uint8_t player_dot_tile = 0x08, computer_dot_tile = 0x0a, line_tile = 0x8e;
constexpr std::uint8_t player_dot_attributes = 0x11, opponent_dot_attributes = 0x13,
                       line_attributes = 0x15;
// The new-best markers, entries 96-99 beside the best laps, and entry 112 beside the record.
constexpr std::uint8_t lap_marker_column = 0xe5, player_marker_line = 0x16,
                       opponent_marker_line = 0x26, lap_marker_attributes = 0x17;
constexpr std::uint8_t record_icon_x = 0xe7, record_icon_y = 0xab;
constexpr unsigned holder_palette_colour = 0xa0, marker_palette_colour = 0xb0,
                   marker_palette_asset = 0x22;
// The high-table bits `$80:90E5` clears for the opponent's new-best markers, entries 97 and 99.
constexpr std::uint8_t opponent_best_markers = 0xbb;
// The high table's groups of four entries (two bits each: bit 0 the ninth x bit, set hides;
// bit 1 large). 0xFF: the markers 96-99 large and hidden. 0x5A: 104-105 large and shown, 106-107
// hidden. With a record: 0x6A shows 106 too, 0x56 shows 112 large, 0xAA shows 20-27 large, and
// 0x5A shows 28-29 large and hides 30-31.
constexpr std::uint8_t markers_hidden_large = 0xff, two_icons = 0x5a, three_icons = 0x6a,
                       record_icon_shown = 0x56, record_line_shown = 0xaa, record_line_end = 0x5a;
// $80:8D98: 8 x 8 and 16 x 16 objects, name base 3, until `$80:F4B8`.
constexpr std::uint8_t graph_object_sizes = 0x03;
// $80:9980: a dot's desired velocity is its distance over 8, at most 40 (2.5 pixels) either way.
constexpr std::int32_t fastest = 0x28;

// $83:904A: the graph's floor and range. The range is the laps' spread and the record's, at
// least 200 hundredths; the floor is the fastest of them.
struct GraphScale {
    std::uint16_t floor{}, range{}, top{};
};

GraphScale graph_scale(const RaceTimes& times, std::uint16_t record) {
    std::uint16_t top = record < no_time ? record : 0;
    std::uint16_t floor = record < no_time ? record : no_time;
    for (std::size_t k = 0; k < lap_slots; ++k)
        for (const auto lap : {times.player_laps[k], times.opponent_laps[k]}) {
            if (lap >= no_time) continue;
            top = std::max(top, lap);
            floor = std::min(floor, lap);
        }
    const auto spread = static_cast<std::uint16_t>(top - floor);
    if (spread < minimum_range)
        return {static_cast<std::uint16_t>(top - minimum_range), minimum_range, top};
    return {floor, spread, top};
}

// $80:8E0D-8E2F and $83:8D5D: a time's height on the graph, as a pixel line (the division's
// quotient taken in eight bits, as the original's store does).
std::uint8_t graph_line(std::uint16_t time, const GraphScale& scale, std::uint8_t base) {
    const auto above_floor =
        static_cast<std::uint32_t>(static_cast<std::uint16_t>(time - scale.floor));
    const auto height = above_floor * graph_height / scale.range;
    return static_cast<std::uint8_t>(base - height);
}

std::uint16_t to_twelve_four(std::uint8_t pixel) {
    return static_cast<std::uint16_t>(pixel << 4U);
}

// $80:9980: the distance over 8, arithmetically, clamped to 40 either way.
std::uint16_t desired_velocity(std::uint16_t target, std::uint16_t position) {
    const auto distance = static_cast<std::int16_t>(static_cast<std::uint16_t>(target - position));
    const std::int32_t desired = std::clamp<std::int32_t>(distance >> 3, -fastest, fastest);
    return static_cast<std::uint16_t>(desired);
}

// $80:9937-9974: the velocity two nearer the desired one.
void steer(std::uint16_t& velocity, std::uint16_t desired) {
    const auto difference = static_cast<std::uint16_t>(velocity - desired);
    if (difference == 0) return;
    velocity = static_cast<std::uint16_t>(velocity + ((difference & 0x8000U) ? 2 : -2));
}

std::uint16_t record_of(const FrontEndState& state) {
    return state.records.record_times[0][state.tour_menu.track];
}

// The words the lap result's streams read, as the direct page holds them after `$80:8FFD`.
struct LapWords {
    std::uint16_t best{}, holder{}, record{}, total{}, floor{}, top{};
};

void print_stream(FrontEndState& state, const FrontEndContent& content,
                  std::span<const std::uint8_t> stream, const LapWords& words) {
    TextVariables variables;
    variables.word = [&](std::uint16_t address) -> std::uint16_t {
        switch (address) {
        case 0x00b2: return words.best;
        case 0x00b4: return words.holder; // `$80:9B2F` masks it to the holder's byte
        case 0x00b6: return words.record;
        case 0x00ba: return words.total;
        case 0x00bc: return words.floor;
        case 0x00be: return words.top;
        case 0x00ce: return state.tour_menu.track;
        case 0x017d: return state.rider_menu.rider;
        case 0x017f: return state.now_playing.opponent;
        default: throw std::invalid_argument("the lap result's streams read no such word");
        }
    };
    variables.track_names = content.track_names;
    variables.rider_names = content.rider_names;
    variables.time_words = content.time_words;
    variables.place_object = [&](unsigned object, unsigned position) {
        place_printed_object(state, object, position);
    };
    print_text(state.text, state.printer, stream, content.character_table, &variables);
}

// $80:90AF-90F2 for a computer opponent: the best lap goes to `$77:0829 + 100 x opponent + 2 x
// track`, past the 16 riders' table. For 0x11-0x13 that is inside `$77:0E6B-1008`, the pre-race
// save of work RAM `$0000-$019D` (`$83:9894`), so the markers show when the best lap is under
// the saved word there. Native keeps the words the boot leaves under BRONSEN's CRAWLER tracks
// (`$0062-$006B`, unchanged in every capture); elsewhere it takes 0, no markers, which is wrong
// where the word is not 0 (R-0058 lists the cases).
std::uint16_t saved_word_under_opponent_best(const FrontEndState& state) {
    constexpr std::uint8_t bronsen = 0x11;
    constexpr std::array<std::uint16_t, 5> bronsen_crawler{0x0000, 0x1300, 0x0000, 0x9e00, 0x0000};
    const auto track = state.tour_menu.track;
    if (state.now_playing.opponent == bronsen && track < bronsen_crawler.size())
        return bronsen_crawler[track];
    return 0;
}

} // namespace

std::uint16_t best_lap(const std::array<std::uint16_t, 10>& laps) {
    return *std::min_element(laps.begin(), laps.end());
}

std::uint16_t record_lap(const std::array<std::uint16_t, 10>& laps) {
    std::uint16_t best = 0xea62;
    for (const auto lap : laps)
        if (lap != 0 && lap < best) best = lap;
    return best;
}

void build_lap_result(FrontEndState& state, const FrontEndContent& content) {
    auto& result = state.race_result;
    const auto& times = result.times;
    // $80:8D70-8D9A: the markers large and pushed off, entries 0-19 shown and small.
    high_bits(state, 96) = markers_hidden_large;
    high_bits(state, 100) = four_hidden;
    for (const unsigned group : {0U, 4U, 8U, 12U, 16U}) high_bits(state, group) = four_shown;
    state.registers.obsel = graph_object_sizes;
    const auto record = record_of(state);
    const auto scale = graph_scale(times, record);
    const auto line = graph_line(record, scale, player_graph_base); // $80:8DA1-8DD7
    for (unsigned lap = 0; lap < lap_slots; ++lap) {
        const auto column = static_cast<std::uint8_t>(first_dot_column + column_step * (lap + 1));
        const auto height = [&](std::uint16_t time, std::uint8_t base) {
            return time < no_time ? to_twelve_four(graph_line(time, scale, base)) : unrun_lap_y;
        };
        result.dots[lap] = {dot_start_x,
                            dot_start_y,
                            to_twelve_four(column),
                            height(times.player_laps[lap], player_graph_base),
                            0,
                            0};
        result.dots[opponent_dots + lap] = {dot_start_x,
                                            dot_start_y,
                                            to_twelve_four(column),
                                            height(times.opponent_laps[lap], opponent_graph_base),
                                            0,
                                            0};
    }
    const bool computer = state.now_playing.opponent >= someone;
    for (unsigned k = 0; k < opponent_dots; ++k) {
        for (const unsigned entry : {k, opponent_dots + k}) {
            oam_byte(state, entry, 0) = dot_start_oam_x;
            oam_byte(state, entry, 1) = dot_start_oam_y;
        }
        oam_byte(state, k, 2) = player_dot_tile;
        oam_byte(state, k, 3) = player_dot_attributes;
        oam_byte(state, opponent_dots + k, 2) = computer ? computer_dot_tile : player_dot_tile;
        oam_byte(state, opponent_dots + k, 3) = opponent_dot_attributes;
        oam_byte(state, record_line + k, 2) = line_tile;
        oam_byte(state, record_line + k, 3) = line_attributes;
    }
    for (unsigned entry = 96; entry < 100; ++entry) {
        oam_byte(state, entry, 0) = lap_marker_column;
        oam_byte(state, entry, 1) = entry % 2 == 0 ? player_marker_line : opponent_marker_line;
        oam_byte(state, entry, 3) = lap_marker_attributes;
    }
    for (unsigned k = 0; k < record_line_entries; ++k) { // $80:8F4C-8FA6
        oam_byte(state, record_line + k, 0) =
            static_cast<std::uint8_t>(first_line_column + column_step * (k + 1));
        oam_byte(state, record_line + k, 1) = line;
    }
    // $80:8FA8-8FF8: the holder's colours at 0xA0; the record line, or entries 106-107 hidden.
    const auto holder = state.records.record_holders[0][state.tour_menu.track];
    load_cgram(state, asset(content, first_rider_palette + holder), holder_palette_colour);
    if (record >= no_time) {
        high_bits(state, 104) = two_icons;
        return;
    }
    print_stream(state, content, content.lap_result_record, {0, holder, record, 0, 0, 0});
    high_bits(state, 112) = record_icon_shown;
    high_bits(state, 104) = three_icons;
    high_bits(state, 20) = high_bits(state, 24) = record_line_shown;
    high_bits(state, 28) = record_line_end;
}

void print_lap_result(FrontEndState& state, const FrontEndContent& content) {
    auto& records = state.records;
    const auto& times = state.race_result.times;
    const auto track = state.tour_menu.track;
    load_cgram(state, asset(content, marker_palette_asset), marker_palette_colour);
    oam_byte(state, 112, 0) = record_icon_x;
    oam_byte(state, 112, 1) = record_icon_y;
    oam_byte(state, 112, 3) = lap_marker_attributes;
    const auto scale = graph_scale(times, record_of(state));
    // $80:9017-905A: the player's best lap, a personal best when under the rider's.
    const auto player_best = best_lap(times.player_laps);
    auto& best = personal_best(records, state.rider_menu.rider, track);
    if (player_best < best) {
        best = player_best;
        high_bits(state, 96) &= player_best_markers;
    }
    print_stream(state, content, content.lap_result_text,
                 {0, 0, 0, 0, scale.floor, scale.top}); // $80:910F
    print_stream(state, content, content.lap_result_player,
                 {player_best, 0, 0, times.player_total, 0, 0}); // $80:91A1
    // A rider opponent (below 0x10) would keep a personal best like the player's (`$80:90AF`);
    // one-player play has none, and `start_result` refuses one.
    const auto opponent_best = best_lap(times.opponent_laps);
    if (opponent_best < saved_word_under_opponent_best(state))
        high_bits(state, 96) &= opponent_best_markers;
    print_stream(state, content, content.lap_result_opponent,
                 {opponent_best, 0, 0, times.opponent_total, 0, 0}); // $80:91B9
}

void step_lap_graph(FrontEndState& state) {
    auto& dots = state.race_result.dots;
    for (unsigned k = 0; k < dots.size(); ++k) {
        auto& dot = dots[k];
        oam_byte(state, k, 0) = static_cast<std::uint8_t>(dot.x >> 4U);
        oam_byte(state, k, 1) = static_cast<std::uint8_t>(dot.y >> 4U);
        dot.x = static_cast<std::uint16_t>(dot.x + dot.velocity_x);
        dot.y = static_cast<std::uint16_t>(dot.y + dot.velocity_y);
        steer(dot.velocity_x, desired_velocity(dot.target_x, dot.x));
        steer(dot.velocity_y, desired_velocity(dot.target_y, dot.y));
    }
}

} // namespace unirally::front_end_screens
