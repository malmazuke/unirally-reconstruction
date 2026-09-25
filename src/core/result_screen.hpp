#pragma once
// The result screen: its title, times, text and backgrounds. Internal to the presentation.

#include "presentation.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace unirally {

struct ClassicResultContent {
    std::span<const std::uint8_t> palette, result_assets, result_base_vram;
    std::span<const std::uint8_t> result_palette, result_palette_tail;
    std::span<const std::uint8_t> track_name; // Empty: the assets' own title.
};

std::uint16_t result_title_tile(char glyph);
void build_result_map(std::array<std::uint8_t, 65536>& vram, const RaceFinishState& finish,
                      const RaceTimerDigits& clock, std::span<const std::uint8_t> result_assets,
                      std::span<const std::uint8_t> track_name = {});
void render_result_background(RgbFrame& frame, const RaceFinishState& finish,
                              const RaceTimerDigits& clock, const ClassicResultContent& content);
void ui_text(RgbFrame& frame, int x, int y, std::string_view text,
             std::array<std::uint8_t, 3> ink = {255, 240, 220});
std::string race_time(unsigned value);
std::string result_time(unsigned value);

} // namespace unirally
