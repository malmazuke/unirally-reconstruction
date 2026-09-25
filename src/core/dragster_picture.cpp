#include "picture.hpp"
#include "presentation.hpp"
#include "result_screen.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <stdexcept>
#include <vector>

// The legacy DRAGSTER picture (the v1 pack's renderer).
namespace unirally {

namespace {

struct RiderAtlasGroup {
    std::size_t first_tile;
    std::span<const std::uint16_t> vram_words;
};

RiderAtlasGroup rider_atlas_group(const PresentationSample& sample) {
    static constexpr std::array<std::uint16_t, 27> frame1600{
        24608, 24624, 26800, 26816, 26832, 24864, 24880, 24896, 27056,
        27072, 27088, 25136, 25152, 25168, 27296, 27312, 27328, 27344,
        25392, 25408, 25424, 27552, 27568, 25664, 25680, 27808, 27824};
    static constexpr std::array<std::uint16_t, 20> frame2000{
        24848, 24864, 24880, 27024, 27040, 27056, 25136, 25152, 25168, 25184,
        27312, 27328, 27344, 27360, 25408, 25424, 25440, 27584, 27600, 27616};
    static constexpr std::array<std::uint16_t, 21> frame2400{
        26784, 24848, 24864, 24880, 27024, 27040, 27056, 25136, 25152, 25168, 25184,
        27312, 27328, 27344, 27360, 25408, 25424, 25440, 27584, 27600, 27616};
    static constexpr std::array<std::uint16_t, 21> frame3213{
        24608, 24848, 24864, 24880, 27024, 27040, 27056, 25136, 25152, 25168, 25184,
        27312, 27328, 27344, 27360, 25408, 25424, 25440, 27584, 27600, 27616};
    static constexpr std::array<std::uint16_t, 19> frame3453{
        24624, 24640, 24880, 24896, 25136, 25152, 25376, 25392, 25408, 27552,
        27568, 27584, 25648, 25664, 25680, 27792, 27808, 27824, 27840};
    const auto player = sample.movement.riders[0].pose.pose_index;
    const auto opponent = sample.movement.riders[1].pose.pose_index;
    if (player == 0x04f9 && opponent == 0x0263) return {0, frame1600};
    if (player == 0x0855 && opponent == 0x0895) return {27, frame2000};
    if (player == 0x0895 && opponent == 0x0895) return {47, frame2400};
    if (player == 0x0855 && opponent == 0x08d5) return {68, frame3213};
    if (player == 0x04fe && opponent == 0x037c) return {89, frame3453};
    throw std::invalid_argument("unsupported Classic rider atlas combination");
}

void load_rider_tiles(std::array<std::uint8_t, 65536>& vram, const PresentationSample& sample,
                      std::span<const std::uint8_t> atlas) {
    const auto group = rider_atlas_group(sample);
    for (std::size_t tile = 0; tile < group.vram_words.size(); ++tile) {
        const auto source = (group.first_tile + tile) * 32U;
        const auto destination = static_cast<std::size_t>(group.vram_words[tile]) * 2U;
        std::copy_n(atlas.begin() + static_cast<std::ptrdiff_t>(source), 32,
                    vram.begin() + static_cast<std::ptrdiff_t>(destination));
    }
}

void render_rider(RgbFrame& frame, const std::array<std::uint8_t, 65536>& vram,
                  const std::array<std::uint8_t, 512>& cgram, int x, int y, std::uint16_t base_tile,
                  std::uint8_t attributes) {
    constexpr std::size_t object_tile_base = 0xc000;
    for (int tile_y = 0; tile_y < 8; ++tile_y)
        for (int tile_x = 0; tile_x < 8; ++tile_x) {
            const int source_x = 7 - tile_x;
            const auto tile_offset = static_cast<unsigned>(source_x + (tile_y << 4));
            const auto tile = static_cast<std::uint16_t>(
                (base_tile & 0x100U) | ((static_cast<unsigned>(base_tile) + tile_offset) & 0xffU));
            for (int pixel_y = 0; pixel_y < 8; ++pixel_y)
                for (int pixel_x = 0; pixel_x < 8; ++pixel_x) {
                    const auto value =
                        tile_pixel(vram, object_tile_base, tile, 7 - pixel_x, pixel_y);
                    if (value == 0) continue;
                    const auto palette =
                        static_cast<std::uint8_t>(128U + ((attributes >> 1U) & 7U) * 16U + value);
                    pixel(frame, x + tile_x * 8 + pixel_x, y + tile_y * 8 + pixel_y,
                          colour(cgram, palette));
                }
        }
}

} // namespace

std::array<std::uint16_t, 32 * 32>
build_dragster_result_map(const MovementState& state, std::span<const std::uint8_t> result_assets) {
    std::array<std::uint8_t, 65536> vram{};
    build_result_map(vram, state.finish, state.timer, result_assets);
    std::array<std::uint16_t, 32 * 32> map{};
    for (std::size_t entry = 0; entry < map.size(); ++entry) {
        const auto at = 0x2000U + entry * 2U;
        map[entry] =
            static_cast<std::uint16_t>(vram[at] | (static_cast<unsigned>(vram[at + 1]) << 8U));
    }
    return map;
}

RiderFrameSelection rider_frame_for_pose(std::uint16_t pose, bool reflected) {
    // Every observed pose in the frozen Classic slice carries the reflected
    // semantic orientation. The packed atlas/OAM relationship is not recovered
    // for the contradictory orientation, so fail closed instead of silently
    // drawing the reflected object geometry.
    if (!reflected)
        throw std::invalid_argument("unsupported Classic rider pose/reflection combination");
    RiderFrameId id{};
    switch (pose) {
    case 0x04f9: id = RiderFrameId::LeanForward; break;
    case 0x0263: id = RiderFrameId::CoastForward; break;
    case 0x0855: id = RiderFrameId::RollingForward; break;
    case 0x0895:
        id = reflected ? RiderFrameId::RollingReflected : RiderFrameId::RollingForward;
        break;
    case 0x08d5: id = RiderFrameId::RollingReflected; break;
    case 0x04fe:
        id = reflected ? RiderFrameId::SettledReflected : RiderFrameId::SettledForward;
        break;
    case 0x037c:
        id = reflected ? RiderFrameId::FinishReflected : RiderFrameId::FinishForward;
        break;
    default:
        throw std::invalid_argument("unsupported semantic rider pose for Classic presentation");
    }
    return {id, "presentation.rider.mike.race-tiles.v1", reflected};
}

std::vector<std::uint16_t> gather_dragster_bg1(std::span<const std::uint8_t> track,
                                               std::uint16_t source_x, std::uint16_t stride,
                                               std::size_t count) {
    if (stride == 0 || (stride & 1U))
        throw std::invalid_argument("Dragster BG1 stride must be a positive even byte count");
    std::vector<std::uint16_t> out;
    out.reserve(count);
    std::size_t cursor = 0x0fU + source_x;
    for (std::size_t i = 0; i < count; ++i) {
        out.push_back(word(track, cursor));
        if (i + 1 < count) {
            if (cursor > std::numeric_limits<std::size_t>::max() - stride)
                throw std::invalid_argument("Dragster BG1 gather offset overflow");
            cursor += stride;
        }
    }
    return out;
}

std::array<std::uint16_t, 480> expand_dragster_bg1(std::span<const std::uint8_t> track) {
    constexpr std::size_t base = 0x800f, column_bytes = 32;
    if (track.size() < base + 30 * column_bytes)
        throw std::invalid_argument("decoded track lacks the observed Dragster BG1 map region");
    std::array<std::uint16_t, 480> out{};
    for (std::size_t x = 0; x < 30; ++x)
        for (std::size_t y = 0; y < 16; ++y)
            out[y * 30 + x] = word(track, base + x * column_bytes + y * 2);
    return out;
}

std::array<std::uint16_t, 1024> build_dragster_bg1_map(std::span<const std::uint8_t> track,
                                                       std::int16_t scroll_x,
                                                       std::int16_t scroll_y) {
    constexpr int horizontal_origin_tiles = 16;
    constexpr int vertical_origin_tiles = 10;
    constexpr std::size_t selector_base = 0x5831;
    constexpr std::size_t selector_plane_bytes = 0x800;
    constexpr std::size_t definition_base = 0x800f;

    const int first_tile_x = static_cast<std::uint16_t>(scroll_x) / 16;
    const int first_tile_y = static_cast<std::uint16_t>(scroll_y) / 16;
    std::array<std::uint16_t, 1024> map{};
    for (int ring_y = 0; ring_y < 32; ++ring_y) {
        const int global_y = first_tile_y + ((ring_y - first_tile_y) & 31);
        const int relative_y = global_y - vertical_origin_tiles;
        if (relative_y < 0) continue;
        const auto selector_plane = static_cast<std::size_t>(relative_y / 4);
        const auto definition_y = static_cast<std::size_t>(relative_y & 3);
        if (selector_plane >= 5) continue;
        for (int ring_x = 0; ring_x < 32; ++ring_x) {
            const int global_x = first_tile_x + ((ring_x - first_tile_x) & 31);
            const int relative_x = global_x - horizontal_origin_tiles;
            if (relative_x < 0) continue;
            const auto selector_column = static_cast<std::size_t>(relative_x / 4);
            const auto definition_x = static_cast<std::size_t>(relative_x & 3);
            const auto selector_at =
                selector_base + selector_plane * selector_plane_bytes + selector_column * 2U;
            const auto selector = word(track, selector_at);
            const auto definition_at =
                definition_base + selector * 32U + definition_y * 8U + definition_x * 2U;
            map[static_cast<std::size_t>(ring_y * 32 + ring_x)] = word(track, definition_at);
        }
    }
    return map;
}

// The DRAGSTER v1 pack's entry sizes; a v1 pack carries no window table family.
static void check_dragster_content(const PresentationContent& content) {
    if (content.bg1_tiles.size() != 2560 || content.bg2_tiles.size() != 992
        || content.bg2_map.size() != 8192 || content.palette.size() != 352
        || content.font.size() != 2048 || content.rider_tiles.size() != 3456
        || content.result_assets.size() != 5224 || content.go_window.size() != 898
        || content.winner_window.size() != 898 || content.result_base_vram.size() != 41536
        || content.result_palette.size() != 216 || content.result_palette_tail.size() != 128
        || (!content.window_tables.empty() && content.window_tables.size() != 22475))
        throw std::invalid_argument("Classic presentation entry size is unsupported");
}

// The accepted DRAGSTER picture's timer marks: one bar per digit, its width by the digit.
static void draw_dragster_timer_marks(RgbFrame& f, const RaceTimerDigits& t) {
    const std::array<unsigned, 4> d{{t.minutes, t.tens_seconds, t.seconds, t.tenths}};
    for (std::size_t i = 0; i < 4; ++i)
        rect(f, 8 + static_cast<int>(i) * 9, 8, 3 + static_cast<int>(d[i] % 5), 10,
             {238, 238, 224});
}

// The two riders' objects, from this state's poses or, when given, the rider art's.
// A 64-pixel object wholly outside the 256-pixel screen cannot contribute.
static void draw_dragster_riders(RgbFrame& f, const PresentationSample& s,
                                 const PresentationContent& content,
                                 const std::array<RiderArtPose, 2>* rider_art) {
    std::array<std::uint8_t, 65536> rider_vram{};
    auto art_state = s.movement;
    if (rider_art != nullptr) {
        for (std::size_t rider = 0; rider < rider_art->size(); ++rider) {
            art_state.riders[rider].pose.pose_index = (*rider_art)[rider].pose_index;
            art_state.riders[rider].pose.reflected = (*rider_art)[rider].reflected;
        }
    }
    const PresentationSample art_sample{art_state,      s.camera_x,     s.bg1_scroll_x,
                                        s.bg1_scroll_y, s.bg2_scroll_x, s.bg2_scroll_y};
    load_rider_tiles(rider_vram, art_sample, content.rider_tiles);
    const auto rider_cgram = build_race_cgram(s, content.palette);
    for (std::size_t rider_index = 0; rider_index < 2; ++rider_index) {
        const auto& rider = s.movement.riders[rider_index];
        const auto art_pose = rider_art == nullptr
                                ? RiderArtPose{rider.pose.pose_index, rider.pose.reflected}
                                : (*rider_art)[rider_index];
        (void)rider_frame_for_pose(art_pose.pose_index, art_pose.reflected);
        const std::int64_t wide_x =
            static_cast<std::int16_t>(rider.motion.x) - static_cast<std::int64_t>(s.camera_x) - 832;
        const int y = static_cast<std::int16_t>(rider.motion.y) - 752;
        // Narrow only coordinates in the renderer's small, representable domain.
        if (wide_x > -64 && wide_x < 256)
            render_rider(f, rider_vram, rider_cgram, static_cast<int>(wide_x), y,
                         rider_index == 0 ? 0 : 136, rider_index == 0 ? 0x66 : 0x68);
    }
}

static RgbFrame render_dragster(const PresentationSample& s, const PresentationContent& content,
                                const std::array<RiderArtPose, 2>* rider_art) {
    check_dragster_content(content);
    // The original publishes the completed result at end-of-frame 3678. The
    // accepted gameplay state reaches ResultScreen on the following update, so
    // presentation consumes the observed loading counter without altering the
    // gameplay transition or its serialization.
    const auto& finish = s.movement.finish;
    if (result_screen_visible(finish)) {
        RgbFrame result{};
        render_result_background(result, finish, s.movement.timer,
                                 {content.palette,
                                  content.result_assets,
                                  content.result_base_vram,
                                  content.result_palette,
                                  content.result_palette_tail,
                                  {}});
        return result;
    }
    const auto map = build_dragster_bg1_map(content.track, s.bg1_scroll_x, s.bg1_scroll_y);
    RgbFrame f{};
    render_race_background(f, s, content, map);
    const auto player_pose = s.movement.riders[0].pose.pose_index;
    const auto opponent_pose = s.movement.riders[1].pose.pose_index;
    // The GO and winner windows show colour 0 through colour math. Its race
    // palette cycle is visible only there (R-0037): the accepted white and
    // (98,98,255) are colour 0 at phases 10 and 7, where the frozen frames fall.
    // Without the cycle tables (DRAGSTER v1 packs) keep those accepted colours.
    const auto window_colour =
        [&](std::array<std::uint8_t, 3> accepted) -> std::array<std::uint8_t, 3> {
        if (content.race_palette_cycle.empty()) return accepted;
        auto cgram = build_race_cgram(s, content.palette, false);
        apply_dragster_palette_cycle(cgram, content.race_palette_cycle, s.movement);
        return colour(cgram, 0);
    };
    // R-0040: with the recovered table family the pose pair is replaced by the
    // original's own per-frame selection. A DRAGSTER v1 pack carries only the two
    // frozen tables, so it keeps the accepted pose-keyed placement unchanged.
    const auto window_index = content.window_tables.empty()
                                ? std::optional<unsigned>{}
                                : dragster_window_table_index(s.movement);
    if (content.window_tables.empty()) {
        if (player_pose == 0x04f9 && opponent_pose == 0x0263)
            render_window_xor(f, content.go_window, window_colour({255, 255, 255}));
    } else if (window_index && *window_index <= 6) {
        render_window_xor(f, dragster_window_table(content.window_tables, *window_index),
                          window_colour({255, 255, 255}));
    }
    draw_dragster_timer_marks(f, s.movement.timer);
    draw_dragster_riders(f, s, content, rider_art);
    if (content.window_tables.empty()) {
        if (player_pose == 0x04fe && opponent_pose == 0x037c)
            render_window_xor(f, content.winner_window, window_colour({98, 98, 255}));
    } else if (window_index && *window_index >= 7) {
        render_window_xor(f, dragster_window_table(content.window_tables, *window_index),
                          window_colour({98, 98, 255}));
    }
    return f;
}

RgbFrame render_dragster_headless(const PresentationSample& sample,
                                  const PresentationContent& content) {
    return render_dragster(sample, content, nullptr);
}

RgbFrame render_dragster_headless_with_rider_art(const PresentationSample& sample,
                                                 const PresentationContent& content,
                                                 const std::array<RiderArtPose, 2>& rider_art) {
    return render_dragster(sample, content, &rider_art);
}

} // namespace unirally
