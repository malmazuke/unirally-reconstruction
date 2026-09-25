#include "rider_object.hpp"

#include "content_pack.hpp"

#include <bit>
#include <stdexcept>

namespace unirally {
namespace {

constexpr std::uint8_t first_frame_bank = 0x23;
constexpr std::uint8_t first_tile_bank = 0x27;
constexpr std::size_t bank_bytes = 0x8000;
constexpr std::size_t tile_bytes = 32;
constexpr std::size_t header_bytes = 4;
constexpr std::size_t object_rows = 5;
constexpr std::size_t row_columns = 6;

// Offset of a LoROM bank:address inside a pack entry that starts at
// `first_bank`:8000 and concatenates whole banks.
std::size_t banked_offset(std::uint8_t bank, std::uint16_t address, std::uint8_t first_bank) {
    if (bank < first_bank || address < 0x8000U)
        throw std::invalid_argument("rider object reference precedes its packed banks");
    return static_cast<std::size_t>(bank - first_bank) * bank_bytes + (address - 0x8000U);
}

// One frame stream: its five row masks and the cursor of its next tile word.
struct FrameStream {
    std::array<std::uint8_t, object_rows> row_masks{};
    std::size_t cursor{};
};

FrameStream open_frame(const RiderObjectContent& content, std::uint16_t pose, RiderRowClip clip) {
    const auto pointer = static_cast<std::size_t>(pose) * 3U;
    if (pointer + 3U > content.pose_pointers.size())
        throw std::invalid_argument("rider pose index is outside the packed pose table");
    const auto address = static_cast<std::uint16_t>(
        content.pose_pointers[pointer]
        | (static_cast<unsigned>(content.pose_pointers[pointer + 1]) << 8U));
    // $83:F2DA: the stored bank byte is relative to $23.
    const auto bank =
        static_cast<std::uint8_t>(content.pose_pointers[pointer + 2] + first_frame_bank);
    const auto start = banked_offset(bank, address, first_frame_bank);
    if (start + header_bytes > content.pose_frames.size())
        throw std::invalid_argument("rider pose frame is outside the packed frames");
    const auto header = content.pose_frames.subspan(start, header_bytes);
    // $83:F2FF-$83:F3F8: six-bit row masks in bits 7..2.
    FrameStream frame;
    frame.row_masks = {
        static_cast<std::uint8_t>(header[0] & 0xfcU),
        static_cast<std::uint8_t>(((header[0] & 0x03U) << 6U) | ((header[1] & 0xf0U) >> 2U)),
        static_cast<std::uint8_t>(((header[1] & 0x0fU) << 4U)
                                  | ((static_cast<unsigned>(header[2]) >> 6U) << 2U)),
        static_cast<std::uint8_t>((header[2] & 0x3fU) << 2U),
        static_cast<std::uint8_t>(header[3] & 0xfcU)};
    frame.cursor = start + header_bytes;
    if (clip == RiderRowClip::top_row) {
        // $83:F311/$83:F412: skip row 0's words, then drop its mask.
        frame.cursor += 2U * static_cast<std::size_t>(std::popcount(frame.row_masks[0]));
        frame.row_masks[0] = 0;
    } else if (clip == RiderRowClip::bottom_row) {
        frame.row_masks[4] = 0; // $83:F3E9/$83:F4F2
    }
    const std::size_t words = static_cast<std::size_t>(
        std::popcount(frame.row_masks[0]) + std::popcount(frame.row_masks[1])
        + std::popcount(frame.row_masks[2]) + std::popcount(frame.row_masks[3])
        + std::popcount(frame.row_masks[4]));
    if (frame.cursor + 2U * words > content.pose_frames.size())
        throw std::invalid_argument("rider pose frame tile words are truncated");
    return frame;
}

std::uint16_t next_word(const RiderObjectContent& content, FrameStream& frame) {
    const auto value = static_cast<std::uint16_t>(
        content.pose_frames[frame.cursor]
        | (static_cast<unsigned>(content.pose_frames[frame.cursor + 1]) << 8U));
    frame.cursor += 2;
    return value;
}

void copy_tile(const RiderObjectContent& content, std::uint16_t word, RiderObjectPixels& pixels,
               std::size_t row, std::size_t column) {
    const auto reference = decode_rider_tile_reference(word);
    const auto start = banked_offset(reference.bank, reference.address, first_tile_bank);
    if (start + tile_bytes > content.object_tiles.size())
        throw std::invalid_argument("rider tile reference is outside the packed tiles");
    const auto tile = content.object_tiles.subspan(start, tile_bytes);
    for (std::size_t y = 0; y < 8; ++y)
        for (std::size_t x = 0; x < 8; ++x) {
            const auto bit = 7U - x;
            const auto plane = [&](std::size_t at) {
                return (static_cast<unsigned>(tile[at]) >> bit) & 1U;
            };
            const auto value = plane(y * 2) | (plane(y * 2 + 1) << 1U) | (plane(16 + y * 2) << 2U)
                             | (plane(16 + y * 2 + 1) << 3U);
            pixels[(row * 8 + y) * rider_object_size + column * 8 + x] =
                static_cast<std::uint8_t>(value);
        }
}

bool bit_below(std::uint16_t value, std::uint16_t limit) {
    // CMP/BMI and CMP/BPL test bit 15 of the wrapped 16-bit difference.
    return (static_cast<std::uint16_t>(value - limit) & 0x8000U) != 0;
}

} // namespace

RiderObjectContent rider_object_content(const ClassicContentPack& pack) {
    return {pack.entry("presentation.rider.pose-pointers.v1"),
            pack.entry("presentation.rider.pose-frames.v1"),
            pack.entry("presentation.rider.object-tiles.v1")};
}

RiderTileReference decode_rider_tile_reference(std::uint16_t word) {
    const auto low = static_cast<unsigned>(word & 0xffU);
    const auto high = static_cast<unsigned>(word >> 8U);
    return {static_cast<std::uint8_t>(((low & 0xfcU) >> 2U) + first_tile_bank),
            static_cast<std::uint16_t>(0x8000U | ((((low << 8U) | high) << 5U) & 0xffffU))};
}

RiderObjectPixels compose_rider_object(const RiderObjectContent& content, std::uint16_t pose_index,
                                       std::optional<std::uint16_t> overlay_pose,
                                       RiderRowClip clip) {
    auto pose = open_frame(content, pose_index, clip);
    FrameStream overlay;
    if (overlay_pose) overlay = open_frame(content, *overlay_pose, clip);
    RiderObjectPixels pixels{};
    // $83:F1F2-$83:F2C3 visits rows 0-4, columns 1-6, in this order.
    for (std::size_t row = 0; row < object_rows; ++row)
        for (std::size_t slot = 0; slot < row_columns; ++slot) {
            const auto bit = static_cast<std::uint8_t>(0x80U >> slot);
            const std::size_t column = slot + 1;
            if (overlay.row_masks[row] & bit) {
                if (pose.row_masks[row] & bit)
                    pose.cursor += 2; // the covered pose word is consumed unused
                copy_tile(content, next_word(content, overlay), pixels, row, column);
            } else if (pose.row_masks[row] & bit) {
                copy_tile(content, next_word(content, pose), pixels, row, column);
            }
            // An unused slot receives the all-zero $27:8000 tile when it was used
            // by the previous update, so it is always transparent.
        }
    return pixels;
}

RiderOam project_rider_oam(std::uint16_t world_x, std::uint16_t world_y, std::uint16_t camera_x,
                           std::uint16_t camera_y, bool reflected, const TrackGeometry& geometry) {
    const unsigned zoom_shift = geometry.screen_shift;                           // $03F1
    const auto left_limit = static_cast<std::uint16_t>(geometry.visible_left);   // $0425
    const auto right_limit = static_cast<std::uint16_t>(geometry.visible_right); // $0427
    const std::uint16_t world_mask = geometry.position_mask;                     // $0D4F
    RiderOam oam;
    oam.horizontal_flip = reflected; // $0BA7/$0BA9 sets attribute bit 6
    const auto screen_y = static_cast<std::uint16_t>(world_y - camera_y);
    if (bit_below(screen_y, 0xffd7) || !bit_below(screen_y, 0x00e1))
        return oam; // hidden at X/Y 0x70 with the ninth X bit set
    // $82:AD91-$82:ADAB, evaluated only for a published Y.
    if (bit_below(screen_y, 0xffe0))
        oam.clip = RiderRowClip::top_row;
    else if (!bit_below(screen_y, 0x00d0))
        oam.clip = RiderRowClip::bottom_row;
    const auto difference = static_cast<std::uint16_t>(world_x - camera_x);
    const auto scaled = static_cast<std::uint16_t>(difference << zoom_shift);
    if (bit_below(scaled, left_limit) || !bit_below(scaled, right_limit)) return oam;
    oam.visible = true;
    const int low_x = (difference & world_mask) & 0xff;
    oam.x = (scaled & 0x8000U) ? low_x - 256 : low_x;
    oam.y = static_cast<std::uint8_t>(screen_y);
    return oam;
}

} // namespace unirally
