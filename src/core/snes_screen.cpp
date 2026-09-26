#include "snes_screen.hpp"

#include "picture.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

// A port of the reference emulator's fast PPU line renderer (bsnes `sfc/ppu-fast/line.cpp`,
// `background.cpp`, `object.cpp`), limited to what the front end draws.
namespace unirally {

namespace {

enum class Source : std::uint8_t {
    bg1,
    bg2,
    bg3,
    bg4,
    obj_low_palettes,
    obj_high_palettes,
    colour
};

struct Pixel {
    Source source = Source::colour;
    std::uint8_t priority{};
    std::uint16_t colour{};
};

enum class TileDepth : std::uint8_t { bits2, bits4, bits8, inactive };

struct ModeLayout {
    std::array<TileDepth, 4> depth;
    std::array<std::array<std::uint8_t, 2>, 4> bg_priority; // tile priority bit clear, set
    std::array<std::uint8_t, 4> obj_priority;               // by OAM priority 0-3
};

// bsnes `PPU::updateVideoMode`: each mode's tile depths and the priority of every layer.
ModeLayout mode_layout(const SnesVideoRegisters& registers) {
    using enum TileDepth;
    switch (registers.mode) {
    case 0:
        return {{bits2, bits2, bits2, bits2}, {{{8, 11}, {7, 10}, {2, 5}, {1, 4}}}, {3, 6, 9, 12}};
    case 1:
        if (registers.bg3_priority)
            return {
                {bits4, bits4, bits2, inactive}, {{{5, 8}, {4, 7}, {1, 10}, {0, 0}}}, {2, 3, 6, 9}};
        return {{bits4, bits4, bits2, inactive}, {{{6, 9}, {5, 8}, {1, 3}, {0, 0}}}, {2, 4, 7, 10}};
    case 2: // offset-per-tile from BG3's map; `render_snes_screen` refuses any it would apply
        return {
            {bits4, bits4, inactive, inactive}, {{{3, 7}, {1, 5}, {0, 0}, {0, 0}}}, {2, 4, 6, 8}};
    case 3:
        return {
            {bits8, bits4, inactive, inactive}, {{{3, 7}, {1, 5}, {0, 0}, {0, 0}}}, {2, 4, 6, 8}};
    default: throw std::invalid_argument("SNES screen mode is not modelled");
    }
}

std::uint16_t vram_word(const SnesVideoMemory& memory, unsigned word) {
    const auto at = static_cast<std::size_t>(word & 0x7fffU) * 2;
    return static_cast<std::uint16_t>(memory.vram[at]
                                      | (static_cast<unsigned>(memory.vram[at + 1]) << 8U));
}

std::uint16_t cgram_colour(const SnesVideoMemory& memory, unsigned index) {
    const auto at = static_cast<std::size_t>(index & 0xffU) * 2;
    return static_cast<std::uint16_t>(memory.cgram[at]
                                      | (static_cast<unsigned>(memory.cgram[at + 1]) << 8U));
}

// bsnes `directColor`: an 8bpp colour and the tile's palette field as BGR555.
std::uint16_t direct_colour(unsigned palette_number, unsigned colour) {
    return static_cast<std::uint16_t>(
        ((colour << 2U) & 0x001cU) + ((palette_number << 1U) & 0x0002U) + ((colour << 4U) & 0x0380U)
        + ((palette_number << 5U) & 0x0040U) + ((colour << 7U) & 0x6000U)
        + ((palette_number << 10U) & 0x1000U));
}

struct Line {
    std::array<Pixel, 256> above{}, below{};
};

void plot(std::array<Pixel, 256>& layer, int x, Source source, std::uint8_t priority,
          std::uint16_t colour) {
    if (priority > layer[static_cast<std::size_t>(x)].priority)
        layer[static_cast<std::size_t>(x)] = {source, priority, colour};
}

// bsnes `getTile`: the map word under a background position.
std::uint16_t map_entry(const SnesVideoMemory& memory, const SnesBackground& bg, unsigned hoffset,
                        unsigned voffset) {
    const unsigned tile_shift = bg.large_tiles ? 4 : 3;
    const unsigned screen_x = (bg.map_size & 1U) ? 32U << 5U : 0U;
    const unsigned screen_y = (bg.map_size & 2U) ? 32U << (5U + (bg.map_size & 1U)) : 0U;
    const unsigned tile_x = hoffset >> tile_shift;
    const unsigned tile_y = voffset >> tile_shift;
    unsigned offset = ((tile_y & 31U) << 5U) | (tile_x & 31U);
    if (tile_x & 32U) offset += screen_x;
    if (tile_y & 32U) offset += screen_y;
    return vram_word(memory, bg.map_word + offset);
}

// bsnes `renderBackground` for one line, without mosaic, hires or offset-per-tile.
void draw_background(Line& line, const SnesVideoMemory& memory, const SnesVideoRegisters& registers,
                     const ModeLayout& layout, unsigned index, unsigned y) {
    const auto& bg = registers.bg[index];
    const bool main = (static_cast<unsigned>(registers.main_screen) >> index) & 1U;
    const bool sub = (static_cast<unsigned>(registers.sub_screen) >> index) & 1U;
    const auto depth = layout.depth[index];
    if ((!main && !sub) || depth == TileDepth::inactive) return;
    const auto source = static_cast<Source>(index);
    const unsigned depth_index = static_cast<unsigned>(depth);
    const bool direct = (registers.colour_select & 1U) && index == 0
                     && (registers.mode == 3 || registers.mode == 4);
    const unsigned tile_shift = bg.large_tiles ? 4 : 3;
    const unsigned tile_mask = 0x0fffU >> depth_index;
    const unsigned tile_base = bg.tile_word >> (3U + depth_index);
    const unsigned palette_base = registers.mode == 0 ? index << 5U : 0U;
    const unsigned palette_shift = 2U << depth_index;
    const unsigned hmask = (256U << (bg.large_tiles ? 1 : 0) << ((bg.map_size & 1U) ? 1 : 0)) - 1;
    const unsigned vmask = (256U << (bg.large_tiles ? 1 : 0) << ((bg.map_size & 2U) ? 1 : 0)) - 1;
    int x = -static_cast<int>(bg.hofs & 7U);
    while (x < 256) {
        const unsigned hoffset = (static_cast<unsigned>(x) + bg.hofs) & hmask;
        const unsigned voffset = (y + bg.vofs) & vmask;
        unsigned tile = map_entry(memory, bg, hoffset, voffset);
        const unsigned mirror_y = (tile & 0x8000U) ? 7 : 0;
        const unsigned mirror_x = (tile & 0x4000U) ? 7 : 0;
        const auto priority = layout.bg_priority[index][(tile & 0x2000U) ? 1 : 0];
        const unsigned palette_number = (tile >> 10U) & 7U;
        const unsigned palette_index = (palette_base + (palette_number << palette_shift)) & 0xffU;
        if (tile_shift == 4 && (((hoffset & 8U) != 0) != (mirror_x != 0))) tile += 1;
        if (tile_shift == 4 && (((voffset & 8U) != 0) != (mirror_y != 0))) tile += 16;
        tile = ((tile & 0x03ffU) + tile_base) & tile_mask;
        const unsigned address =
            ((tile << (3U + depth_index)) + ((voffset & 7U) ^ mirror_y)) & 0x7fffU;
        std::uint64_t data = vram_word(memory, address);
        data |= static_cast<std::uint64_t>(vram_word(memory, address + 8)) << 16U;
        data |= static_cast<std::uint64_t>(vram_word(memory, address + 16)) << 32U;
        data |= static_cast<std::uint64_t>(vram_word(memory, address + 24)) << 48U;
        for (unsigned tile_x = 0; tile_x < 8; ++tile_x, ++x) {
            if (x < 0 || x >= 256) continue;
            const unsigned shift = mirror_x ? tile_x : 7 - tile_x;
            unsigned colour = static_cast<unsigned>((data >> shift) & 1U);
            colour += static_cast<unsigned>((data >> (shift + 7)) & 2U);
            if (depth != TileDepth::bits2) {
                colour += static_cast<unsigned>((data >> (shift + 14)) & 4U);
                colour += static_cast<unsigned>((data >> (shift + 21)) & 8U);
            }
            if (depth == TileDepth::bits8) {
                colour += static_cast<unsigned>((data >> (shift + 28)) & 16U);
                colour += static_cast<unsigned>((data >> (shift + 35)) & 32U);
                colour += static_cast<unsigned>((data >> (shift + 42)) & 64U);
                colour += static_cast<unsigned>((data >> (shift + 49)) & 128U);
            }
            if (colour == 0) continue;
            const auto value = direct ? direct_colour(palette_number, colour)
                                      : cgram_colour(memory, palette_index + colour);
            if (main) plot(line.above, x, source, priority, value);
            if (sub) plot(line.below, x, source, priority, value);
        }
    }
}

struct ObjectEntry {
    unsigned x{}, y{}, character{};
    bool name_select{}, hflip{}, vflip{}, large{};
    unsigned palette{}, priority{};
};

// OAM entry `n` as bsnes keeps it: y one line later than written ("rendering happens one
// scanline late"), x with its ninth bit from the high table.
ObjectEntry object_entry(const SnesVideoMemory& memory, unsigned n) {
    const auto* entry = &memory.oam[n * 4];
    const unsigned high = (static_cast<unsigned>(memory.oam[512 + n / 4]) >> ((n % 4) * 2)) & 3U;
    const unsigned attributes = entry[3];
    return {static_cast<unsigned>(entry[0]) | ((high & 1U) << 8U),
            (entry[1] + 1U) & 0xffU,
            entry[2],
            (attributes & 1U) != 0,
            (attributes & 0x40U) != 0,
            (attributes & 0x80U) != 0,
            (high & 2U) != 0,
            (attributes >> 1U) & 7U,
            (attributes >> 4U) & 3U};
}

struct ObjectItem {
    unsigned index, width, height;
};

struct ObjectTile {
    unsigned x, palette, priority;
    bool hflip;
    std::uint32_t data;
};

// bsnes `renderObject`, first pass: the objects on this line, 32 at most, in OAM order.
unsigned line_objects(std::array<ObjectItem, 32>& items, const SnesVideoMemory& memory,
                      const SnesVideoRegisters& registers, unsigned y) {
    constexpr std::array<unsigned, 8> small_width{8, 8, 8, 16, 16, 32, 16, 16};
    constexpr std::array<unsigned, 8> small_height{8, 8, 8, 16, 16, 32, 32, 32};
    constexpr std::array<unsigned, 8> large_width{16, 32, 64, 32, 64, 64, 32, 32};
    constexpr std::array<unsigned, 8> large_height{16, 32, 64, 32, 64, 64, 64, 32};
    const unsigned base_size = registers.obsel >> 5U;
    unsigned count = 0;
    for (unsigned n = 0; n < 128; ++n) {
        const auto object = object_entry(memory, n);
        const unsigned width = object.large ? large_width[base_size] : small_width[base_size];
        const unsigned height = object.large ? large_height[base_size] : small_height[base_size];
        if (object.x > 256 && object.x + width - 1 < 512) continue;
        if ((y >= object.y && y < object.y + height)
            || (object.y + height >= 256 && y < ((object.y + height) & 255U))) {
            if (count >= items.size()) break;
            items[count++] = {n, width, height};
        }
    }
    return count;
}

// Second pass: their tiles on this line, last object first, 34 tiles at most.
unsigned line_object_tiles(std::array<ObjectTile, 34>& tiles,
                           const std::array<ObjectItem, 32>& items, unsigned item_count,
                           const SnesVideoMemory& memory, const SnesVideoRegisters& registers,
                           unsigned y) {
    const unsigned tile_base = (registers.obsel & 7U) << 13U;
    const unsigned name_select = (registers.obsel >> 3U) & 3U;
    unsigned count = 0;
    for (unsigned k = item_count; k-- > 0;) {
        const auto& item = items[k];
        const auto object = object_entry(memory, item.index);
        const unsigned tile_width = item.width >> 3U;
        unsigned row = (y - object.y) & 0xffU;
        if (object.vflip) {
            if (item.width == item.height)
                row = item.height - 1 - row;
            else if (row < item.width)
                row = item.width - 1 - row;
            else
                row = item.width + (item.width - 1) - (row - item.width);
        }
        row &= 255U;
        unsigned base = tile_base;
        if (object.name_select) base += (1U + name_select) << 12U;
        const unsigned character_x = object.character & 15U;
        const unsigned character_y = (((object.character >> 4U) + (row >> 3U)) & 15U) << 4U;
        for (unsigned tile_x = 0; tile_x < tile_width; ++tile_x) {
            const unsigned object_x = (object.x + (tile_x << 3U)) & 511U;
            if (object.x != 256 && object_x >= 256 && object_x + 7 < 512) continue;
            const unsigned mirror = object.hflip ? tile_width - 1 - tile_x : tile_x;
            unsigned address = base + ((character_y + ((character_x + mirror) & 15U)) << 4U);
            address = (address & 0x7ff0U) + (row & 7U);
            const std::uint32_t data =
                vram_word(memory, address)
                | (static_cast<std::uint32_t>(vram_word(memory, address + 8)) << 16U);
            if (count >= tiles.size()) return count;
            tiles[count++] = {object_x, 128U + (object.palette << 4U), object.priority,
                              object.hflip, data};
        }
    }
    return count;
}

// bsnes `renderObject` for one line: later tiles (earlier objects) are in front.
void draw_objects(Line& line, const SnesVideoMemory& memory, const SnesVideoRegisters& registers,
                  const ModeLayout& layout, unsigned y) {
    const bool main = (registers.main_screen >> 4U) & 1U;
    const bool sub = (registers.sub_screen >> 4U) & 1U;
    if (!main && !sub) return;
    std::array<ObjectItem, 32> items{};
    const auto item_count = line_objects(items, memory, registers, y);
    std::array<ObjectTile, 34> tiles{};
    const auto tile_count = line_object_tiles(tiles, items, item_count, memory, registers, y);
    std::array<std::uint8_t, 256> palette{}, priority{};
    for (unsigned n = 0; n < tile_count; ++n) {
        const auto& tile = tiles[n];
        unsigned tile_x = tile.x;
        for (unsigned x = 0; x < 8; ++x, ++tile_x) {
            tile_x &= 511U;
            if (tile_x >= 256) continue;
            const unsigned shift = tile.hflip ? x : 7 - x;
            unsigned colour = (tile.data >> shift) & 1U;
            colour += (tile.data >> (shift + 7)) & 2U;
            colour += (tile.data >> (shift + 14)) & 4U;
            colour += (tile.data >> (shift + 21)) & 8U;
            if (colour == 0) continue;
            palette[tile_x] = static_cast<std::uint8_t>(tile.palette + colour);
            priority[tile_x] = layout.obj_priority[tile.priority];
        }
    }
    for (int x = 0; x < 256; ++x) {
        const auto at = static_cast<std::size_t>(x);
        if (!priority[at]) continue;
        const auto source =
            palette[at] < 192 ? Source::obj_low_palettes : Source::obj_high_palettes;
        const auto value = cgram_colour(memory, palette[at]);
        if (main) plot(line.above, x, source, priority[at], value);
        if (sub) plot(line.below, x, source, priority[at], value);
    }
}

// bsnes `blend`: colour addition or subtraction of two BGR555 colours, optionally halved.
std::uint16_t blend(unsigned a, unsigned b, bool halve, bool subtract) {
    if (!subtract) {
        if (!halve) {
            const unsigned sum = a + b;
            const unsigned carry = (sum - ((a ^ b) & 0x0421U)) & 0x8420U;
            return static_cast<std::uint16_t>((sum - carry) | (carry - (carry >> 5U)));
        }
        return static_cast<std::uint16_t>((a + b - ((a ^ b) & 0x0421U)) >> 1U);
    }
    const unsigned diff = a - b + 0x8420U;
    const unsigned borrow = (diff - ((a ^ b) & 0x8420U)) & 0x8420U;
    if (!halve) return static_cast<std::uint16_t>((diff - borrow) & (borrow - (borrow >> 5U)));
    return static_cast<std::uint16_t>((((diff - borrow) & (borrow - (borrow >> 5U))) & 0x7bdeU)
                                      >> 1U);
}

// bsnes `col.enable` (CGADSUB bits 0-5): objects of palettes 0-3 never take part.
bool colour_math_enabled(std::uint8_t colour_math, Source source) {
    switch (source) {
    case Source::bg1:
    case Source::bg2:
    case Source::bg3:
    case Source::bg4:
        return (static_cast<unsigned>(colour_math) >> static_cast<unsigned>(source)) & 1U;
    case Source::obj_low_palettes: return false;
    case Source::obj_high_palettes: return (static_cast<unsigned>(colour_math) >> 4U) & 1U;
    case Source::colour: return (static_cast<unsigned>(colour_math) >> 5U) & 1U;
    }
    return false;
}

// bsnes `pixel`: the colour math of one screen position. The colour window's two masks are
// either "never" (0) or "always" (3); the region masks need windows, which are not modelled.
std::uint16_t combine(const SnesVideoRegisters& registers, Pixel above, const Pixel& below) {
    const unsigned clip_mask = (registers.colour_select >> 6U) & 3U;
    const unsigned prevent_mask = (registers.colour_select >> 4U) & 3U;
    if (clip_mask == 3) above.colour = 0;
    if (prevent_mask == 3) return above.colour;
    if (!colour_math_enabled(registers.colour_math, above.source)) return above.colour;
    // bsnes halves only where the colour is not clipped to black (`halve && windowAbove[x]`).
    const bool halve = (registers.colour_math & 0x40U) != 0 && clip_mask != 3;
    const bool subtract = (registers.colour_math & 0x80U) != 0;
    if (!(registers.colour_select & 2U))
        return blend(above.colour, registers.fixed_colour, halve, subtract);
    return blend(above.colour, below.colour, halve && below.source != Source::colour, subtract);
}

} // namespace

RgbFrame render_snes_screen(const SnesVideoMemory& screen_memory,
                            const SnesVideoRegisters& registers,
                            std::span<const SnesLineColour> line_colours) {
    RgbFrame frame{};
    if (registers.force_blank) return frame;
    if (registers.unmodelled_features)
        throw std::invalid_argument("SNES windows, mosaic, HDMA and OAM rotation are not modelled");
    const unsigned clip_mask = (registers.colour_select >> 6U) & 3U;
    const unsigned prevent_mask = (registers.colour_select >> 4U) & 3U;
    if ((clip_mask == 1 || clip_mask == 2) || (prevent_mask == 1 || prevent_mask == 2))
        throw std::invalid_argument("SNES colour window regions are not modelled");
    const auto layout = mode_layout(registers);
    // Mode 2's offset-per-tile entries are BG3's map words; bits 13 and 14 apply one to BG1 and
    // BG2. Native draws mode 2 without them, so it refuses a map where any is set.
    if (registers.mode == 2) {
        constexpr unsigned map_words = 32 * 32;
        constexpr std::uint16_t applies = 0x6000;
        for (unsigned k = 0; k < map_words; ++k)
            if (vram_word(screen_memory, registers.bg[2].map_word + k) & applies)
                throw std::invalid_argument("SNES mode 2's offset-per-tile is not modelled");
    }
    // bsnes caches CGRAM per line (`PPU::Line::cache`), so a colour changed during the picture
    // shows from the next line on: the picture is drawn from a copy whose CGRAM changes by row.
    SnesVideoMemory memory = screen_memory;
    auto next_colour = line_colours.begin();
    for (int row = 0; row < 224; ++row) {
        for (; next_colour != line_colours.end() && next_colour->row <= row; ++next_colour) {
            memory.cgram[next_colour->index * 2U] = static_cast<std::uint8_t>(next_colour->colour);
            memory.cgram[next_colour->index * 2U + 1] =
                static_cast<std::uint8_t>(next_colour->colour >> 8U);
        }
        // Screen row 0 is scanline 1, as in `background_pixel`.
        const auto y = static_cast<unsigned>(row + 1);
        Line line;
        line.above.fill({Source::colour, 0, cgram_colour(memory, 0)});
        line.below.fill({Source::colour, 0, registers.fixed_colour});
        for (unsigned index = 0; index < 4; ++index)
            draw_background(line, memory, registers, layout, index, y);
        draw_objects(line, memory, registers, layout, y);
        for (int x = 0; x < 256; ++x) {
            const auto at = static_cast<std::size_t>(x);
            const auto value = combine(registers, line.above[at], line.below[at]);
            pixel(frame, x, row,
                  colour_word_rgb(apply_snes_brightness(value, registers.brightness)));
        }
    }
    return frame;
}

} // namespace unirally
