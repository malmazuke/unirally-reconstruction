#include "picture.hpp"

#include "presentation.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>

// The SNES picture's parts: colours and CGRAM, tiles, backgrounds and the window.
namespace unirally {

std::uint16_t snes_direct_colour(std::uint8_t palette_colour, std::uint8_t palette_group) {
    // bsnes sfc/ppu/screen.cpp:152-158. In mode 3 with CGWSEL bit 1 set,
    // BG1's 8-bit pixel and the map entry's three-bit group directly form BGR555.
    return static_cast<std::uint16_t>(((static_cast<unsigned>(palette_colour) << 7U) & 0x6000U)
                                      | ((static_cast<unsigned>(palette_group) << 10U) & 0x1000U)
                                      | ((static_cast<unsigned>(palette_colour) << 4U) & 0x0380U)
                                      | ((static_cast<unsigned>(palette_group) << 5U) & 0x0040U)
                                      | ((static_cast<unsigned>(palette_colour) << 2U) & 0x001cU)
                                      | ((static_cast<unsigned>(palette_group) << 1U) & 0x0002U));
}

std::uint16_t snes_add_colour(std::uint16_t main_colour, std::uint16_t sub_colour, bool halve) {
    // bsnes sfc/ppu/screen.cpp:130-137. The masks preserve independent carries
    // for the three five-bit colour channels.
    if (halve) {
        return static_cast<std::uint16_t>(
            (main_colour + sub_colour - ((main_colour ^ sub_colour) & 0x0421U)) >> 1U);
    }
    const auto sum = static_cast<unsigned>(main_colour) + sub_colour;
    const auto carry = (sum - ((main_colour ^ sub_colour) & 0x0421U)) & 0x8420U;
    return static_cast<std::uint16_t>((sum - carry) | (carry - (carry >> 5U)));
}

std::uint16_t word(std::span<const std::uint8_t> b, std::size_t at) {
    if (at > b.size() || b.size() - at < 2)
        throw std::invalid_argument("Dragster BG1 gather exceeds decoded track data");
    return static_cast<std::uint16_t>(b[at] | (static_cast<unsigned>(b[at + 1]) << 8U));
}

void pixel(RgbFrame& f, int x, int y, std::array<std::uint8_t, 3> c) {
    if (x < 0 || y < 0 || x >= 256 || y >= 224) return;
    const auto at = (static_cast<std::size_t>(y) * 256 + static_cast<std::size_t>(x)) * 3;
    std::copy(c.begin(), c.end(), f.pixels.begin() + static_cast<std::ptrdiff_t>(at));
}

void rect(RgbFrame& f, int x, int y, int w, int h, std::array<std::uint8_t, 3> c) {
    for (int py = y; py < y + h; ++py)
        for (int px = x; px < x + w; ++px) pixel(f, px, py, c);
}

void render_window_xor(RgbFrame& frame, std::span<const std::uint8_t> table,
                       std::array<std::uint8_t, 3> fixed_colour) {
    // Channel 6 uses HDMA mode 4: each active line writes WH0..WH3 ($2126-$2129).
    // The race setup combines its two inclusive horizontal windows with XOR.
    std::size_t source = 0;
    int screen_y = 0;
    while (screen_y < 224) {
        if (source >= table.size())
            throw std::invalid_argument("Classic window HDMA table is truncated");
        const auto line_control = table[source++];
        const int line_count = line_control & 0x7fU;
        if (line_count == 0 || (line_control & 0x80U) == 0)
            throw std::invalid_argument("unsupported Classic window HDMA state");
        for (int line = 0; line < line_count && screen_y < 224; ++line, ++screen_y) {
            if (table.size() - source < 4)
                throw std::invalid_argument("Classic window HDMA row is truncated");
            const int window1_left = table[source++];
            const int window1_right = table[source++];
            const int window2_left = table[source++];
            const int window2_right = table[source++];
            for (int screen_x = 0; screen_x < 256; ++screen_x) {
                const bool in_window1 = window1_left <= window1_right && screen_x >= window1_left
                                     && screen_x <= window1_right;
                const bool in_window2 = window2_left <= window2_right && screen_x >= window2_left
                                     && screen_x <= window2_right;
                if (in_window1 != in_window2) pixel(frame, screen_x, screen_y, fixed_colour);
            }
        }
    }
    if (source != table.size())
        throw std::invalid_argument("Classic window HDMA table has trailing bytes");
}

std::uint8_t channel8(std::uint16_t value) {
    const auto expanded = static_cast<unsigned>((value << 3U) | (value >> 2U));
    const auto wide = expanded * 257U;
    if (wide > 32767U) return static_cast<std::uint8_t>(wide >> 8U);
    const auto corrected = 32767.0 * std::pow(wide / 32767.0, 1.5);
    return static_cast<std::uint8_t>(static_cast<unsigned>(corrected) >> 8U);
}

std::array<std::uint8_t, 3> colour(const std::array<std::uint8_t, 512>& cgram, std::uint8_t index) {
    const auto at = static_cast<std::size_t>(index) * 2;
    const auto value =
        static_cast<std::uint16_t>(cgram[at] | (static_cast<unsigned>(cgram[at + 1]) << 8U));
    return {channel8(value & 31U), channel8((value >> 5U) & 31U), channel8((value >> 10U) & 31U)};
}

std::uint16_t colour_word(const std::array<std::uint8_t, 512>& cgram, std::uint8_t index) {
    const auto at = static_cast<std::size_t>(index) * 2;
    return static_cast<std::uint16_t>(cgram[at] | (static_cast<unsigned>(cgram[at + 1]) << 8U));
}

std::array<std::uint8_t, 3> colour_word_rgb(std::uint16_t value) {
    return {channel8(value & 31U), channel8((value >> 5U) & 31U), channel8((value >> 10U) & 31U)};
}

std::uint16_t apply_snes_brightness(std::uint16_t colour_value, unsigned brightness) {
    const auto scale = [brightness](unsigned channel) { return (brightness * channel + 7U) / 15U; };
    return static_cast<std::uint16_t>(scale(colour_value & 31U)
                                      | (scale((colour_value >> 5U) & 31U) << 5U)
                                      | (scale((colour_value >> 10U) & 31U) << 10U));
}

std::array<std::uint8_t, 512> build_race_cgram(std::span<const std::uint8_t> packed_palette,
                                               bool late_finish) {
    std::array<std::uint8_t, 512> cgram{};
    constexpr std::array<std::size_t, 6> targets{{0, 224, 256, 352, 480, 384}};
    constexpr std::array<std::size_t, 6> lengths{{192, 32, 32, 32, 32, 32}};
    std::size_t source{};
    for (std::size_t piece = 0; piece < targets.size(); ++piece) {
        std::copy_n(packed_palette.begin() + static_cast<std::ptrdiff_t>(source), lengths[piece],
                    cgram.begin() + static_cast<std::ptrdiff_t>(targets[piece]));
        source += lengths[piece];
    }
    static constexpr std::array<std::uint8_t, 32> racing_cycle{
        16,  66, 181, 86, 107, 45, 0,   0,  255, 127, 0,  0,   148, 82,  255, 127,
        106, 73, 164, 48, 65,  20, 164, 48, 106, 73,  81, 102, 122, 127, 81,  102};
    static constexpr std::array<std::uint8_t, 32> finish_cycle{
        16, 66,  0,   0,   255, 127, 181, 86, 107, 45, 0,  0,  148, 82, 255, 127,
        81, 102, 122, 127, 81,  102, 106, 73, 164, 48, 65, 20, 164, 48, 106, 73};
    const auto& cycle = late_finish ? finish_cycle : racing_cycle;
    std::copy(cycle.begin(), cycle.end(), cgram.begin() + 192);
    if (late_finish) {
        cgram[0] = 173;
        cgram[1] = 125;
    } else {
        cgram[0] = 255;
        cgram[1] = 127;
    }
    return cgram;
}

// The accepted M3 DRAGSTER palette: its late-finish colours are keyed to its
// own finish poses. The shared renderer never uses this keying; the race NMI
// cycle (R-0037) supplies those colours from the pack instead.
std::array<std::uint8_t, 512> build_race_cgram(const PresentationSample& sample,
                                               std::span<const std::uint8_t> packed_palette,
                                               bool dragster_late_finish) {
    const bool late_finish = dragster_late_finish
                          && (sample.movement.riders[1].pose.pose_index == 0x08d5
                              || sample.movement.riders[0].pose.pose_index == 0x04fe);
    return build_race_cgram(packed_palette, late_finish);
}

std::uint8_t tile_pixel(const std::array<std::uint8_t, 65536>& vram, std::size_t tile_byte,
                        std::uint16_t tile, int x, int y) {
    const auto at = (tile_byte + (tile & 0x3ffU) * 32U) & 0xffffU;
    const auto bit = static_cast<unsigned>(7 - x);
    const auto plane01 = at + static_cast<std::size_t>(y) * 2;
    const auto plane23 = plane01 + 16;
    return static_cast<std::uint8_t>(
        ((static_cast<unsigned>(vram[plane01]) >> bit) & 1U)
        | (((static_cast<unsigned>(vram[plane01 + 1]) >> bit) & 1U) << 1U)
        | (((static_cast<unsigned>(vram[plane23]) >> bit) & 1U) << 2U)
        | (((static_cast<unsigned>(vram[plane23 + 1]) >> bit) & 1U) << 3U));
}

std::uint8_t tile_pixel_8bpp(const std::array<std::uint8_t, 65536>& vram, std::size_t tile_byte,
                             std::uint16_t tile, int x, int y) {
    const auto at = (tile_byte + (tile & 0x3ffU) * 64U) & 0xffffU;
    const auto bit = static_cast<unsigned>(7 - x);
    std::uint8_t value{};
    for (unsigned plane = 0; plane < 8; ++plane) {
        const auto plane_byte =
            at + static_cast<std::size_t>(y) * 2U + (plane / 2U) * 16U + (plane & 1U);
        value |= static_cast<std::uint8_t>(
            ((static_cast<unsigned>(vram[plane_byte & 0xffffU]) >> bit) & 1U) << plane);
    }
    return value;
}

std::uint8_t background_pixel(const std::array<std::uint8_t, 65536>& vram, std::size_t map_base,
                              bool wide, bool tall, std::size_t tile_base, bool tiles16, int hofs,
                              int vofs, int screen_x, int screen_y) {
    const int tile_size = tiles16 ? 16 : 8;
    const int map_width = wide ? 64 : 32;
    const int map_height = tall ? 64 : 32;
    const int px = (screen_x + hofs) & (map_width * tile_size - 1);
    const int py = (screen_y + vofs + 1) & (map_height * tile_size - 1);
    int map_x = px / tile_size, map_y = py / tile_size, screen = 0;
    if (map_x >= 32) {
        ++screen;
        map_x -= 32;
    }
    if (map_y >= 32) {
        screen += wide ? 2 : 1;
        map_y -= 32;
    }
    const auto entry_at = (map_base + static_cast<std::size_t>(screen) * 0x800U
                           + static_cast<std::size_t>(map_y * 32 + map_x) * 2U)
                        & 0xffffU;
    const auto entry = static_cast<std::uint16_t>(
        vram[entry_at] | (static_cast<unsigned>(vram[entry_at + 1]) << 8U));
    int tile_x = px % tile_size, tile_y = py % tile_size;
    if (entry & 0x4000U) tile_x = tile_size - 1 - tile_x;
    if (entry & 0x8000U) tile_y = tile_size - 1 - tile_y;
    auto tile = static_cast<std::uint16_t>(entry & 0x3ffU);
    if (tiles16) {
        const auto subtile = static_cast<unsigned>((tile_x >> 3) + ((tile_y >> 3) << 4));
        tile = static_cast<std::uint16_t>((tile + subtile) & 0x3ffU);
        tile_x &= 7;
        tile_y &= 7;
    }
    const auto value = tile_pixel(vram, tile_base, tile, tile_x, tile_y);
    return value == 0 ? 0 : static_cast<std::uint8_t>(((entry >> 10U) & 7U) * 16U + value);
}

void render_race_background(RgbFrame& frame, const PresentationSample& sample,
                            const PresentationContent& content,
                            const std::array<std::uint16_t, 1024>& map) {
    std::array<std::uint8_t, 65536> vram{};
    std::copy(content.bg2_tiles.begin(), content.bg2_tiles.end(), vram.begin() + 0x2000);
    std::copy(content.bg2_map.begin(), content.bg2_map.end(), vram.begin() + 0xe000);
    constexpr std::array<std::uint16_t, 40> bg1_words{
        {8192, 8448, 8224, 8480, 8256, 8512, 8288, 8544, 8320, 8576, 8352, 8608, 8384, 8640,
         8416, 8672, 8704, 8960, 8736, 8992, 8768, 9024, 8800, 9056, 8832, 9088, 8864, 9120,
         8896, 9152, 8928, 9184, 9216, 9472, 9248, 9504, 9280, 9536, 9312, 9568}};
    for (std::size_t piece = 0; piece < bg1_words.size(); ++piece)
        std::copy_n(content.bg1_tiles.begin() + static_cast<std::ptrdiff_t>(piece * 64), 64,
                    vram.begin() + bg1_words[piece] * 2);
    for (std::size_t y = 0; y < 32; ++y)
        for (std::size_t x = 0; x < 32; ++x) {
            const auto at = 0x1800U + (y * 32U + x) * 2U;
            const auto entry = map[y * 32 + x];
            vram[at] = static_cast<std::uint8_t>(entry);
            vram[at + 1] = static_cast<std::uint8_t>(entry >> 8U);
        }
    auto cgram = build_race_cgram(sample, content.palette, content.race_palette_cycle.empty());
    if (!content.race_palette_cycle.empty())
        apply_dragster_palette_cycle(cgram, content.race_palette_cycle, sample.movement);
    rect(frame, 0, 0, 256, 224, colour(cgram, 0));
    for (int y = 0; y < 224; ++y)
        for (int x = 0; x < 256; ++x) {
            const auto bg2 = background_pixel(vram, 0xe000, true, true, 0x2000, false,
                                              sample.bg2_scroll_x, sample.bg2_scroll_y, x, y);
            if (bg2) pixel(frame, x, y, colour(cgram, bg2));
            const auto bg1 = background_pixel(vram, 0x1800, false, false, 0x4000, true,
                                              sample.bg1_scroll_x, sample.bg1_scroll_y, x, y);
            if (bg1) pixel(frame, x, y, colour(cgram, bg1));
        }
}

void copy_wrapping(std::array<std::uint8_t, 65536>& destination, std::size_t destination_byte,
                   std::span<const std::uint8_t> source) {
    for (std::size_t index = 0; index < source.size(); ++index)
        destination[(destination_byte + index) & 0xffffU] = source[index];
}

void set_map_word(std::array<std::uint8_t, 65536>& vram, int x, int y, std::uint16_t value) {
    const auto at = 0x2000U + static_cast<std::size_t>(y * 32 + x) * 2U;
    vram[at] = static_cast<std::uint8_t>(value);
    vram[at + 1] = static_cast<std::uint8_t>(value >> 8U);
}

} // namespace unirally
