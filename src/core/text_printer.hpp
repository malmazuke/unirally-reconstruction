#pragma once
// The game's text printer, `$80:C3BC` (R-0053, R-0054): a byte stream of characters and control
// codes printed into a 32 x 32 tilemap, the work RAM buffer `$0200` that the menus copy to VRAM.
#include <array>
#include <cstdint>
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

// Prints `stream` up to its 0xFF. `character_table` is `$80:C709`, 256 entries: bit 7 clear gives
// a big glyph (2 x 2 tiles from tile 2 x entry), set a small one (one tile wide). Refuses a
// control code the recovered menus do not use.
void print_text(TextMap& map, TextCursor& cursor, std::span<const std::uint8_t> stream,
                std::span<const std::uint8_t> character_table);

} // namespace unirally
