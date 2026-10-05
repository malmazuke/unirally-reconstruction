#pragma once
// The race HUD: the lap and clock fields, their text queue, and the captions. Internal to the presentation.

#include "presentation.hpp"

#include <array>
#include <bitset>
#include <cstdint>
#include <optional>
#include <string_view>

namespace unirally {

void draw_classic_caption(RgbFrame& frame, const ZoomZooState& published,
                          const ClassicRacePresentationContent& content,
                          const std::optional<ClassicHudPublished>& hud,
                          std::array<std::uint8_t, 3> ink, std::bitset<256 * 224>& inked);
void draw_classic_hud(RgbFrame& frame, const ZoomZooState& state,
                      const ClassicRacePresentationContent& content,
                      std::optional<std::uint32_t> opponent_finish_frame,
                      const std::optional<ClassicHudPublished>& published,
                      std::array<std::uint8_t, 3> ink, std::array<std::uint8_t, 3> opponent_ink,
                      unsigned opponent_caption_event, std::bitset<256 * 224>& inked);

// The BG3 tilemap's cells, 32 columns by 28 rows, as one bit each.
using Bg3Cells = std::bitset<32 * 28>;
// The cells the paused race's menu writes (`$83:F794-F8A7`, R-0078): rows 5-6 from column 8
// (" continue game "), rows 7-8 from column 13 (the second choice) and column 24 of rows 5-8
// (the cursor and its blank); in the lower view of a split race the same 14 rows further down
// (R-0079). Its words replace whatever the HUD had there.
Bg3Cells classic_pause_menu_cells(bool lower_view);
// $83:F915-F95F: whether this update ran the menu and closed it (CONTINUE GAME, or a
// reopening while Start is still held): the menu's own count of updates moved on and no choice
// is left. A HUNTER effect's skipped update (R-0052) runs no menu, and the standalone race's
// restart starts the count again.
bool classic_pause_menu_closed(const ZoomZooState& previous, const ZoomZooState& updated);
// The paused race's menu over the picture: the two choices and the "<" beside `selection` (1 the
// first, 0xFFFF the second), in the caption's font and the view's ink. `second_choice` is the
// original's "quit", or the standalone race's "restart".
void draw_classic_pause_menu(RgbFrame& frame, const ClassicRacePresentationContent& content,
                             std::uint16_t selection, std::string_view second_choice,
                             bool lower_view, std::array<std::uint8_t, 3> ink,
                             std::bitset<256 * 224>& inked);

} // namespace unirally
