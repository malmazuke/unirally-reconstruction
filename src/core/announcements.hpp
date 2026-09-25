#pragma once
// Announcement events: the one-byte entries of the announcement queues, named by the
// caption each shows (presentation.classic.captions.v1, sixteen characters per event at
// $17:CA04). Internal to the race engine; the public interface is zoom_zoo_movement.hpp.

#include <cstdint>

namespace unirally::announcement {

// Tricks come in fours: the trick, its double, its treble and its "city" (four or more).
inline constexpr std::uint8_t roll = 1;    // 1-4: roll, double roll, treble roll, roll city
inline constexpr std::uint8_t flip = 5;    // 5-8
inline constexpr std::uint8_t twist = 9;   // 9-12: twist ... twister city
inline constexpr std::uint8_t z_flip = 18; // 18-21
inline constexpr unsigned tricks_per_kind = 4;
// The event of `count` (1-4) of the trick whose first event is `first`.
constexpr unsigned trick(std::uint8_t first, unsigned count) {
    return first + count - 1U;
}

inline constexpr std::uint8_t wipeout = 14;
inline constexpr std::uint8_t last_lap = 15;
inline constexpr std::uint8_t head_bounce = 16;
inline constexpr std::uint8_t tabletop = 17;
inline constexpr std::uint8_t wrong_way = 22;
inline constexpr std::uint8_t winner = 37;
inline constexpr std::uint8_t draw = 38;
inline constexpr std::uint8_t loser = 39;

// The HUNTER tour's effects (R-0052), 27-34 ("screen flip on" ... "control reversed"),
// and the blank caption that announces an effect's end.
inline constexpr std::uint8_t effect_over = 35;

// 40-71: the tutorial hints, shown four at a time.
inline constexpr std::uint8_t first_hint = 40;
inline constexpr unsigned hints_per_group = 4;

// 72 and up: rider voices ("rockin", "righteous", ...), sixteen per character pair. The
// player's are 72-87, BRONSEN's (character 17) 200-215 and the HUNTER opponent's
// (character 20) 232-247.
inline constexpr std::uint8_t first_voice = 72;
inline constexpr unsigned voices_per_character_pair = 16;
inline constexpr std::uint8_t bronsen_first_voice = 200, bronsen_last_voice = 215;
inline constexpr std::uint8_t hunter_first_voice = 232, hunter_last_voice = 247;

} // namespace unirally::announcement
