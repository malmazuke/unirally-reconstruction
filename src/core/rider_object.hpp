#pragma once
// Original rider sprite composition, recovered in R-0036.
//
// Each rider is one 64-by-64 OBJ. Every race update the original rebuilds the
// object's tiles from the rider's pose index and republishes its OAM entry.
// These functions reproduce those two steps from static pack content and the
// semantic rider fields; they own no state.
#include "zoom_zoo_movement.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace unirally {

class ClassicContentPack;

struct RiderObjectContent {
    // $20:8000: three bytes per pose, a frame address word and bank - $23.
    std::span<const std::uint8_t> pose_pointers;
    // $23:8000-$26:E47B: frame headers and tile reference words, bank by bank.
    std::span<const std::uint8_t> pose_frames;
    // $27:8000-$3F:FFFF: 32-byte SNES 4bpp tiles, bank by bank.
    std::span<const std::uint8_t> object_tiles;
};
RiderObjectContent rider_object_content(const ClassicContentPack& pack);

// $0C89/$0C8B. The builder drops a row that would wrap to the opposite
// screen edge: the top row above OAM Y -32, the bottom row from Y 208.
enum class RiderRowClip : std::uint8_t { none = 0, top_row = 1, bottom_row = 2 };

inline constexpr std::size_t rider_object_size = 64;
// Colour index 0..15 for each object pixel, row-major, before any flip.
// Index 0 is transparent.
using RiderObjectPixels = std::array<std::uint8_t, rider_object_size * rider_object_size>;

// $83:F0FF-$83:F2D9 for one rider; the NMI queue at $82:B8AA uploads the
// result. A pose frame is a four-byte header of five
// six-bit row masks (object tile rows 0-4, columns 1-6; bits 7..2 of each
// masked byte are columns 1..6) followed by one tile reference word per set
// bit in row-major order. An overlay frame uses the same layout; where its
// mask is set its tile replaces the pose's, and the pose word is skipped.
// Throws std::invalid_argument for a pose, overlay or tile reference outside
// the packed tables.
RiderObjectPixels compose_rider_object(const RiderObjectContent& content, std::uint16_t pose_index,
                                       std::optional<std::uint16_t> overlay_pose,
                                       RiderRowClip clip);

// A pose frame's cells as `$83:8E3A` walks them for the front end's pictures (R-0055): rows 0-4
// by columns 0-5, each its tile reference word, or 0 (the blank tile $27:8000) where the frame's
// mask is clear. Throws std::invalid_argument like `compose_rider_object`.
inline constexpr std::size_t pose_frame_rows = 5, pose_frame_columns = 6;
using PoseFrameCells = std::array<std::uint16_t, pose_frame_rows * pose_frame_columns>;
PoseFrameCells pose_frame_cells(const RiderObjectContent& content, std::uint16_t pose_index);

// The 32 bytes of the SNES 4bpp tile a reference word names.
std::span<const std::uint8_t> rider_tile_bytes(const RiderObjectContent& content,
                                               std::uint16_t word);

// Decoded tile reference word ($83:F253-$83:F26B): the low byte's bits 7..2
// select bank 0x27 + n, and bits 1..0 with the high byte select one of 1024
// 32-byte tiles in that bank.
struct RiderTileReference {
    std::uint8_t bank{};
    std::uint16_t address{};
};
RiderTileReference decode_rider_tile_reference(std::uint16_t word);

// One rider OAM entry as $82:ACAC-$82:AE5E publishes it for the one-player
// race. `x` is the signed screen X formed from the ninth X bit.
struct RiderOam {
    bool visible{};
    int x{};
    std::uint8_t y{};
    bool horizontal_flip{};
    RiderRowClip clip{};
    // R-0052: HUNTER effect 3 sets the OAM vertical-flip bit ($83:E04F).
    bool vertical_flip{};
};
// Positions and camera are the original 16-bit words. The projection limits
// are the track's playfield set ($81:A304-A51B, `track_geometry`): the screen
// shift $03F1, the visible span $0425/$0427 and the x mask $0D4F ($81:A445 for
// ZOOM ZOO, $81:A4C1 for DRAGSTER).
RiderOam project_rider_oam(std::uint16_t world_x, std::uint16_t world_y, std::uint16_t camera_x,
                           std::uint16_t camera_y, bool reflected, const TrackGeometry& geometry);

// Draws one OBJ into an RGB-indexed target: `plot(screen_x, screen_y,
// colour_index)` receives only opaque pixels that fall on the 256-by-224
// picture. The OBJ wraps vertically at 256 like the PPU.
template <typename Plot>
void draw_rider_object(const RiderObjectPixels& pixels, const RiderOam& oam, Plot&& plot) {
    if (!oam.visible) return;
    for (int screen_y = 0; screen_y < 224; ++screen_y) {
        const int object_y = (screen_y - static_cast<int>(oam.y)) & 0xff;
        if (object_y >= static_cast<int>(rider_object_size)) continue;
        for (int object_x = 0; object_x < static_cast<int>(rider_object_size); ++object_x) {
            const int screen_x = oam.x + object_x;
            if (screen_x < 0 || screen_x >= 256) continue;
            const int source_x =
                oam.horizontal_flip ? static_cast<int>(rider_object_size) - 1 - object_x : object_x;
            const int source_y =
                oam.vertical_flip ? static_cast<int>(rider_object_size) - 1 - object_y : object_y;
            const auto value = pixels[static_cast<std::size_t>(source_y) * rider_object_size
                                      + static_cast<std::size_t>(source_x)];
            if (value != 0) plot(screen_x, screen_y, value);
        }
    }
}

} // namespace unirally
