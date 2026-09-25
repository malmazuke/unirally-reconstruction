#pragma once
// The SNES picture's parts: colours and CGRAM, tiles, backgrounds and the window. Internal to the presentation.

#include "presentation.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace unirally {

std::uint16_t word(std::span<const std::uint8_t> b, std::size_t at);
void pixel(RgbFrame& f, int x, int y, std::array<std::uint8_t, 3> c);
void rect(RgbFrame& f, int x, int y, int w, int h, std::array<std::uint8_t, 3> c);
void render_window_xor(RgbFrame& frame, std::span<const std::uint8_t> table,
                       std::array<std::uint8_t, 3> fixed_colour);
std::uint8_t channel8(std::uint16_t value);
std::array<std::uint8_t, 3> colour(const std::array<std::uint8_t, 512>& cgram, std::uint8_t index);
std::uint16_t colour_word(const std::array<std::uint8_t, 512>& cgram, std::uint8_t index);
std::array<std::uint8_t, 3> colour_word_rgb(std::uint16_t value);
std::uint16_t apply_snes_brightness(std::uint16_t colour_value, unsigned brightness);
std::array<std::uint8_t, 512> build_race_cgram(std::span<const std::uint8_t> packed_palette,
                                               bool late_finish);
std::array<std::uint8_t, 512> build_race_cgram(const PresentationSample& sample,
                                               std::span<const std::uint8_t> packed_palette,
                                               bool dragster_late_finish = true);
std::uint8_t tile_pixel(const std::array<std::uint8_t, 65536>& vram, std::size_t tile_byte,
                        std::uint16_t tile, int x, int y);
std::uint8_t tile_pixel_8bpp(const std::array<std::uint8_t, 65536>& vram, std::size_t tile_byte,
                             std::uint16_t tile, int x, int y);
std::uint8_t background_pixel(const std::array<std::uint8_t, 65536>& vram, std::size_t map_base,
                              bool wide, bool tall, std::size_t tile_base, bool tiles16, int hofs,
                              int vofs, int screen_x, int screen_y);
void render_race_background(RgbFrame& frame, const PresentationSample& sample,
                            const PresentationContent& content,
                            const std::array<std::uint16_t, 1024>& map);
void copy_wrapping(std::array<std::uint8_t, 65536>& destination, std::size_t destination_byte,
                   std::span<const std::uint8_t> source);
void set_map_word(std::array<std::uint8_t, 65536>& vram, int x, int y, std::uint16_t value);

} // namespace unirally
