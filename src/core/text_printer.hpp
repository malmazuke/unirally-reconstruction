#pragma once
// The game's text printer, `$80:C3BC` (R-0053, R-0054): a byte stream of characters and control
// codes printed into a 32 x 32 tilemap, the work RAM buffer `$0200` that the menus copy to VRAM.
#include <array>
#include <cstdint>
#include <functional>
#include <span>

namespace unirally {

struct TextMap {
    std::array<std::uint16_t, 1024> words{};
};

// Where the printer writes and with which tilemap attribute: `$009F` (row * 32 + column) and `$00B0`.
struct TextCursor {
    unsigned position{};
    std::uint16_t attribute{};
};

// The codes that read the game's state (R-0056): F7 prints the name of the track a direct-page
// word holds, FD the word as a five-digit number, EF puts object 104 + n at the cursor.
struct TextVariables {
    std::function<std::uint16_t(std::uint16_t address)> word; // the direct-page word at address
    std::span<const std::uint8_t> track_names;                // FF-terminated, by track
    std::function<void(unsigned object, unsigned position)> place_object;
};

// Prints `stream` up to its 0xFF. `character_table` is `$80:C709`, 256 entries: bit 7 clear gives
// a big glyph (2 x 2 tiles from tile 2 x entry), set a small one (one tile wide). Refuses a
// control code the recovered menus do not use, and F7, FD and EF without `variables`.
void print_text(TextMap& map, TextCursor& cursor, std::span<const std::uint8_t> stream,
                std::span<const std::uint8_t> character_table,
                const TextVariables* variables = nullptr);

// `$83:8BE7`: five digits, the leading zeros as blanks (`_`) but the last, then 0xFF.
std::array<std::uint8_t, 6> five_digit_text(std::uint16_t value);

} // namespace unirally
