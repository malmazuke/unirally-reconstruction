#include "track_sampling.hpp"

#include <cstddef>
#include <stdexcept>

namespace unirally {
namespace {

// Instruction/operand locations in the identified PAL ROM (R-0010).
constexpr unsigned pose_record_bytes = 8;        // $81:9E20–9E22 (three ASLs)
constexpr unsigned reflected_x_origin = 47;      // operand at file 0x009F14
constexpr unsigned sampling_x_bias = 8;          // operand at file 0x009F5D
constexpr unsigned coarse_cell_shift = 6;        // $81:8A3F–8A44
constexpr unsigned fine_cell_shift = 4;          // masks at $81:8B4B / 8B58
constexpr unsigned coarse_cell_units = 1U << coarse_cell_shift;
constexpr unsigned fine_cell_mask = coarse_cell_units - (1U << fine_cell_shift);
constexpr unsigned block_bytes = 32;             // $81:8AEA–8AEE (five ASLs)
constexpr unsigned coarse_map_offset = 0x000F;   // operand at file 0x008AA2
constexpr unsigned sample_blocks_offset = 0x800F; // operand at file 0x008B67

std::uint16_t word(std::span<const std::uint8_t> bytes, std::size_t offset) {
    if (offset >= bytes.size() || bytes.size() - offset < 2) {
        throw std::out_of_range("sampling content address is unavailable");
    }
    return static_cast<std::uint16_t>(static_cast<unsigned>(bytes[offset]) |
                                     (static_cast<unsigned>(bytes[offset + 1]) << 8));
}

std::uint8_t byte(std::span<const std::uint8_t> bytes, std::size_t offset) {
    if (offset >= bytes.size()) {
        throw std::out_of_range("sampling template address is unavailable");
    }
    return bytes[offset];
}

bool negative_difference(std::uint8_t left, unsigned right) {
    // CMP followed by BMI tests bit 7 of the wrapped subtraction, not a
    // host signed comparison. Preserve it even outside the usual 0..63 range.
    return ((static_cast<unsigned>(left) - right) & 0x80U) != 0;
}

} // namespace

CollisionPoints collision_points(const SamplingContent& content,
                                 std::uint16_t pose_index, bool reflected) {
    // $21:8000 contains eight-byte records; cross-bank poses are outside
    // this recovered subset and rejected by the bounds-checked read.
    const std::size_t pose = static_cast<std::size_t>(pose_index) * pose_record_bytes;
    CollisionPoints points{};
    for (std::size_t i = 0; i < 2; ++i) {
        points[i] = {byte(content.poses, pose + i * 2),
                     byte(content.poses, pose + i * 2 + 1)};
    }
    const unsigned origin_x = byte(content.poses, pose + 4);
    const unsigned origin_y = byte(content.poses, pose + 5);
    const auto selector = word(content.poses, pose + 6);
    // XBA followed by four ASLs ($81:9E6F–9E76), all at width 16.
    const unsigned template_offset =
        ((((static_cast<unsigned>(selector) & 0xFFU) << 8) |
          (static_cast<unsigned>(selector) >> 8)) << 4) & 0xFFFFU;
    for (std::size_t i = 0; i < 8; ++i) {
        points[i + 2] = {
            static_cast<std::uint8_t>(origin_x + byte(content.templates, template_offset + i * 2)),
            static_cast<std::uint8_t>(origin_y + byte(content.templates, template_offset + i * 2 + 1))};
    }
    for (auto& point : points) {
        // Constants 47 and 8: ROM file offsets 0x009F14 and 0x009F5D.
        if (reflected) point.x = static_cast<std::uint8_t>(reflected_x_origin - point.x);
        point.x = static_cast<std::uint8_t>(static_cast<unsigned>(point.x) + sampling_x_bias);
    }
    return points;
}

TrackSamples sample_track(const SamplingContent& content,
                          const CollisionPoints& points,
                          std::uint16_t position_x, std::uint16_t position_y,
                          std::uint16_t coarse_width) {
    unsigned column = static_cast<unsigned>(position_x) >> coarse_cell_shift;
    unsigned row = static_cast<unsigned>(position_y) >> coarse_cell_shift;
    // $81:8A2C-8A3B: a negative y ($A7 bit 15) with $0FF7 clear, as every
    // shape track_geometry accepts leaves it, samples coarse cell (0, 0); the
    // fine offsets below still use the position (TRACK-BREADTH part 3).
    if (position_y >= 0x8000U) column = row = 0;
    if (coarse_width == 0 || column >= coarse_width) {
        throw std::out_of_range("coarse column outside the playfield");
    }
    // $81:8A53–8AC4. Multiplication and shifts wrap at 16 bits.
    const unsigned row_words = (row * coarse_width) & 0xFFFFU;
    const unsigned row_start = (row_words * 2) & 0xFFFFU;
    const unsigned upper_left = ((row_words + column) * 2) & 0xFFFFU;
    const unsigned stride = (static_cast<unsigned>(coarse_width) * 2) & 0xFFFFU;
    // $81:8A60-8A99: in the last column the right-hand neighbours are column 0
    // of the same two rows, so the playfield wraps horizontally.
    const unsigned right = column + 1 == coarse_width ? row_start : (upper_left + 2) & 0xFFFFU;
    std::array<unsigned, 4> blocks{};
    const std::array<unsigned, 4> addresses = {
        upper_left, right,
        (upper_left + stride) & 0xFFFFU, (right + stride) & 0xFFFFU};
    for (std::size_t i = 0; i < blocks.size(); ++i) {
        // The header is 15 bytes ($7F:000F); each fine-cell block is 32 bytes.
        blocks[i] = (static_cast<unsigned>(word(content.track, coarse_map_offset + addresses[i])) * block_bytes) & 0xFFFFU;
    }
    const unsigned boundary_x = coarse_cell_units - (position_x & (coarse_cell_units - 1U));
    const unsigned boundary_y = coarse_cell_units - (position_y & (coarse_cell_units - 1U));
    TrackSamples samples{};
    for (std::size_t i = 0; i < points.size(); ++i) {
        const auto point = points[i];
        const unsigned quadrant = (negative_difference(point.x, boundary_x) ? 0U : 1U) +
                                  (negative_difference(point.y, boundary_y) ? 0U : 2U);
        const unsigned fine_x = ((static_cast<unsigned>(point.x) + position_x) & fine_cell_mask) >> 2;
        const unsigned fine_y = (static_cast<unsigned>(point.y) + position_y) & fine_cell_mask;
        const unsigned offset = (blocks[quadrant] + ((fine_x + fine_y) >> 1)) & 0xFFFFU;
        samples[i] = word(content.track, sample_blocks_offset + offset);
    }
    return samples;
}

} // namespace unirally
