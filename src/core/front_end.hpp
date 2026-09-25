#pragma once
// The front end from power-on to the choice of a mode (R-0054): the Nintendo screen, the title
// and the main menu, frame by frame as the original shows them. The state mirrors the original's
// where it is observable (the arrow, the palette cycle, the menu selection and idle count, the
// OAM buffer) so it can be compared with captures.
#include "presentation.hpp"
#include "snes_screen.hpp"

#include <array>
#include <cstdint>
#include <span>

namespace unirally {

class ClassicContentPack;

// Pack content of the front end (profile v15).
struct FrontEndContent {
    std::array<std::span<const std::uint8_t>, 256> assets{}; // by asset id; empty if not packed
    std::span<const std::uint8_t> base_palette, character_table, main_menu_text, arrow_frames;
    std::span<const std::uint8_t> menu_arrow_columns, cycle_colours;
};
FrontEndContent front_end_content(const ClassicContentPack& pack);

// The main menu's entries, in `$9B` order (COVERAGE-ROADMAP), and the demo it starts when idle.
enum class FrontEndMode : std::uint8_t { one_player, two_player, versus, league, options, demo };

// The menu arrow ($80:FAF5): position and target in sixteenths of a pixel, and its spin.
struct MenuArrow {
    std::uint16_t x{}, target_x{}, y{}, target_y{}; // $0C60, $0C62, $0C68, $0C6A
    std::uint8_t spin{};                            // $C6, 31 down to 0
};

// The palette cycle the NMI runs from the title on ($80:FA60).
struct PaletteCycle {
    std::int8_t delay{}; // $C8: frames to the next step, 6 down to 0
    std::int8_t phase{}; // $C9: 3 down to 0
    bool running{};      // the NMI hook is installed and NMIs are enabled
};

struct MainMenu {
    std::uint8_t selection{}; // $9B
    std::int16_t idle{};      // $89: frames left before the demo
    bool move_latched{};      // $8F: Up or Down still held since the last move
};

struct FrontEndState {
    std::uint32_t frame{}; // frames since power-on; the next update is this frame
    SnesVideoMemory video{};
    SnesVideoRegisters registers{};
    std::array<std::uint8_t, 544> oam_buffer{}; // $0A00, copied to OAM by DMA
    MenuArrow arrow{};
    PaletteCycle cycle{};
    MainMenu menu{};
    bool in_main_menu{};
    bool mode_chosen{};
    FrontEndMode mode{};
};

// Controller words as the auto-joypad read gives them (`$4218`, `$421A`): B 0x8000, Y 0x4000,
// Select 0x2000, Start 0x1000, Up 0x0800, Down 0x0400, Left 0x0200, Right 0x0100, A 0x0080,
// X 0x0040, L 0x0020, R 0x0010.
struct FrontEndPads {
    std::uint16_t one{}, two{};
};

FrontEndState start_front_end();
// One frame: its vblank's work, in the original's order. Once a mode is chosen the state stops.
void update_front_end(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads);
// The frame the last update produced.
RgbFrame render_front_end(const FrontEndState& state);

} // namespace unirally
