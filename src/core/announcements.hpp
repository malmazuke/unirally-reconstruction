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

// The HUNTER tour's effects (R-0052), 27-34, and the blank caption of an effect's end.
inline constexpr std::uint8_t screen_flip_on = 27;
inline constexpr std::uint8_t hedgehog_speed = 28;
inline constexpr std::uint8_t slow_motion_on = 29;
inline constexpr std::uint8_t power_bounce_on = 30;
inline constexpr std::uint8_t barf_mode_on = 31;
inline constexpr std::uint8_t invisible_track = 32;
inline constexpr std::uint8_t wobble_mode_on = 33;
inline constexpr std::uint8_t control_reversed = 34;
inline constexpr std::uint8_t effect_over = 35;

// 40-71: the tutorial hints, shown four at a time.
inline constexpr std::uint8_t first_hint = 40;
inline constexpr unsigned hints_per_group = 4;

// 72 and up: rider voices ("rockin", "righteous", ...), sixteen per character pair
// ($82:9D47-9D5E). The riders' (characters 0-15) are 72-199; the computer opponents' are
// 200-247: BRONSEN's (17) 200-215, SILVIA's and GOLDWYN's (18 and 19, one pair) 216-231 and
// ANTI-UNI's (20) 232-247.
inline constexpr std::uint8_t first_voice = 72;
// A queue found dry waits this many cooldown units (two an update) before looking again.
inline constexpr std::uint16_t empty_queue_wait = 10;
inline constexpr unsigned voices_per_character_pair = 16;
inline constexpr std::uint8_t first_opponent_voice = 200, last_opponent_voice = 247;
// The first of `character`'s sixteen voices.
constexpr unsigned first_voice_of(unsigned character) {
    return first_voice + (character >> 1U) * voices_per_character_pair;
}

} // namespace unirally::announcement
