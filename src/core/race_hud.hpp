#pragma once
// The race HUD: the lap and clock fields, their text queue, and the captions. Internal to the presentation.

#include "presentation.hpp"

#include <array>
#include <bitset>
#include <cstdint>
#include <optional>

namespace unirally {

void draw_classic_caption(RgbFrame& frame, const ZoomZooState& published,
                          const ClassicRacePresentationContent& content,
                          const std::optional<ClassicHudPublished>& hud,
                          std::array<std::uint8_t, 3> ink, std::bitset<256 * 224>& inked);
void draw_classic_hud(RgbFrame& frame, const ZoomZooState& state,
                      const ClassicRacePresentationContent& content,
                      std::optional<std::uint32_t> opponent_finish_frame,
                      const std::optional<ClassicHudPublished>& published,
                      std::array<std::uint8_t, 3> ink, std::bitset<256 * 224>& inked);

} // namespace unirally
