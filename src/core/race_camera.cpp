// The race camera and what it shows.

#include "race_camera.hpp"

#include "zoom_zoo_movement.hpp"

#include <algorithm>
#include <cstdint>

namespace unirally {

// $819FB0-A16D / $81A520-A53F; PAL one-player camera at 4x horizontal scale.
// Positions are world units, signed velocity and lookahead are whole units.
void update_zoom_camera(ZoomZooState& state, const TrackGeometry& geometry) {
    auto& c = state.race.camera;
    const auto& rider = state.movement.riders[0];
    const auto target = 1 - (static_cast<std::int16_t>(rider.motion.velocity_x) >> 3);
    auto look = static_cast<std::int16_t>(c.lookahead);
    if (look < target)
        ++look;
    else if (look > target)
        --look;
    c.lookahead = static_cast<std::uint16_t>(look);
    const auto center =
        static_cast<std::uint16_t>((c.x + c.lookahead + 95U) & geometry.position_mask);
    const auto delta = static_cast<std::int16_t>(
        static_cast<std::uint16_t>((static_cast<unsigned>(rider.motion.x) << geometry.screen_shift)
                                   - (static_cast<unsigned>(center) << geometry.screen_shift)));
    int vx{};
    // Outside the window the $81:9FF8-A02B half-world test reduces to the sign of the wrapped difference.
    if (delta < geometry.follow_window_low || delta >= geometry.follow_window_high)
        vx = delta < 0 ? -16 : 16;
    else {
        const int distance = delta >> geometry.screen_shift;
        vx = distance < 0 ? std::min(0, distance + 8) : std::max(0, distance - 8);
    }
    c.velocity_x = static_cast<std::uint16_t>(vx);
    int candidate = static_cast<std::int16_t>(c.y) + 30;
    int vy = -16;
    while (vy != 16 && candidate < static_cast<std::int16_t>(rider.motion.y)) {
        candidate += 4;
        ++vy;
        if (vy == 0) candidate += 20;
    }
    c.velocity_y = static_cast<std::uint16_t>(vy);
    c.x = static_cast<std::uint16_t>(c.x + vx) & geometry.position_mask;
    c.y = static_cast<std::uint16_t>(c.y + vy);
}

void update_zoom_visibility(ZoomZooState& state, const TrackGeometry& geometry) {
    auto& c = state.race.camera;
    const auto& rider = state.movement.riders[0];
    // $82ACAE-AD7A. The previous frame's visibility feeds speed damping.
    const int dy = static_cast<std::int16_t>(rider.motion.y - c.y);
    const auto dx = static_cast<std::int16_t>(rider.motion.x - c.x);
    const int scaled = static_cast<std::int16_t>(static_cast<std::uint16_t>(
        static_cast<unsigned>(static_cast<std::uint16_t>(dx)) << geometry.screen_shift));
    const bool outside =
        dy < -41 || dy >= 225 || scaled < geometry.visible_left || scaled >= geometry.visible_right;
    state.race.provisional_1225 = (outside || scaled < 0) ? 1 : 0;
    state.race.provisional_1227 = 0;
    c.screen_xy = outside ? 0x7070U
                          : static_cast<std::uint16_t>((static_cast<unsigned>(dy) & 255U) * 256U
                                                       + (static_cast<unsigned>(dx) & 255U));
}

} // namespace unirally
