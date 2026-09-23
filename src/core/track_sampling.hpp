#pragma once

#include <array>
#include <cstdint>
#include <span>

namespace unirally {

// Coordinates are raw 8-bit offsets produced by the pose record. They are
// added to u16 track positions with the original 8-bit arithmetic in the
// spatial query. Evidence: R-0010; $81:9E1B–9FAF.
struct SamplePoint { std::uint8_t x; std::uint8_t y; };
using CollisionPoints = std::array<SamplePoint, 10>;
using TrackSamples = std::array<std::uint16_t, 10>;

struct SamplingContent {
    std::span<const std::uint8_t> track;
    std::span<const std::uint8_t> poses;
    std::span<const std::uint8_t> templates;
};

CollisionPoints collision_points(const SamplingContent& content,
                                 std::uint16_t pose_index, bool reflected);

// Coarse cells are 64x64 position units, with four 16x16 cells per axis.
// A negative y samples coarse cell (0, 0), and the last column's right-hand
// neighbours are column 0 of the same rows ($81:8A2A-8AC4, TRACK-BREADTH
// part 3). A zero width or a column outside the playfield is rejected.
TrackSamples sample_track(const SamplingContent& content,
                          const CollisionPoints& points,
                          std::uint16_t position_x, std::uint16_t position_y,
                          std::uint16_t coarse_width);

} // namespace unirally
