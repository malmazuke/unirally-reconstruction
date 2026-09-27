#include "presentation.hpp"

#include <cstdint>
#include <span>
#include <stdexcept>

// NEON (R-0068): track 42 in one-player play, whose race sets `$12D1`. Its picture is lit by the
// BG1 palette under the player: each race update $83:D1CA moves a green level towards that
// palette's level, and the NMI writes the level into colour 113 ($80:87C6-87D4), which BG2 (on
// the sub screen only) fills the screen with and some of BG1's tiles use.
namespace unirally {

namespace {

// The eight-bit arithmetic of $83:D1CA reads bit 7 of a difference as its sign.
bool negative_byte(unsigned value) { return (value & 0x80U) != 0; }

// $83:D1FD-D206 and $83:D221-D22D: a quarter of the distance, at least one.
unsigned quarter_step(unsigned distance) {
    const unsigned step = (distance & 0xffU) >> 2U;
    return step ? step : 1U;
}

} // namespace

std::uint8_t neon_green_step(std::uint8_t green, std::uint8_t palette,
                             std::span<const std::uint8_t> levels) {
    if (palette >= levels.size()) throw std::invalid_argument("NEON level table is short");
    const unsigned target = levels[palette];
    if (target == green) return green;
    if (!negative_byte(target - green)) {
        const unsigned next = (green + quarter_step(target - green)) & 0xffU;
        return static_cast<std::uint8_t>(negative_byte(target - next) ? target : next);
    }
    const unsigned next = (green - quarter_step(green - target)) & 0xffU;
    return static_cast<std::uint8_t>(negative_byte(target - next) ? next : target);
}

std::uint16_t neon_colour(std::uint8_t green) {
    // $83:D24A-D271: red 0x1F >> 1 and blue (0x1F >> 1) << 1, green the level's low five bits.
    constexpr unsigned red = 15, blue = 30, green_shift = 5, blue_shift = 10;
    return static_cast<std::uint16_t>(red | ((green & 31U) << green_shift) | (blue << blue_shift));
}

std::uint16_t subtract_colour(std::uint16_t object, std::uint16_t below) {
    std::uint16_t result = 0;
    for (const unsigned shift : {0U, 5U, 10U}) {
        const unsigned a = (unsigned{object} >> shift) & 31U, b = (unsigned{below} >> shift) & 31U;
        result = static_cast<std::uint16_t>(result | ((a > b ? a - b : 0U) << shift));
    }
    return result;
}

} // namespace unirally
