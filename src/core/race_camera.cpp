// The race camera and what it shows.

#include "race_camera.hpp"

#include "zoom_zoo_movement.hpp"

#include <algorithm>
#include <cstdint>

namespace unirally {
namespace {

// The camera centres 95 units ahead of its x plus the lookahead; outside the follow window
// it moves at its top speed of 16, inside it follows the distance past a dead zone of 8.
constexpr unsigned center_offset = 95;
constexpr int top_speed = 16, dead_zone = 8;
// Vertically it searches from 30 below its y in steps of 4 (20 more at the stop) for the
// speed, -16 to 16, that brings the rider into view.
constexpr int vertical_start = 30, vertical_step = 4, vertical_stop_step = 20;
// The rider shows between 41 above and 225 below the camera's y; off screen its published
// position is 0x7070.
constexpr int visible_top = -41, visible_bottom = 225;
constexpr std::uint16_t off_screen = 0x7070;
constexpr int split_visible_bottom = 112;
constexpr std::uint16_t second_off_screen = 0x3030;

// $81:A0D4-A16A and $81:A23C-A2D5: in split mode each camera keeps its own
// rider between 36 and 44 units below its y. Outside that band the requested
// signed velocity is half the distance beyond the band, capped at 16.
int split_vertical_velocity(std::uint16_t rider_y, std::uint16_t camera_y) {
    const int distance = static_cast<std::int16_t>(rider_y - camera_y);
    if (distance > 44) return std::min(16, (distance - 43) / 2);
    if (distance < 36) return -std::min(16, (36 - distance) / 2);
    return 0;
}

void follow_split_rider(ZoomZooCamera& camera, const RiderMovementState& rider,
                        const TrackGeometry& geometry) {
    const auto target = 1 - (static_cast<std::int16_t>(rider.motion.velocity_x) >> 3);
    auto look = static_cast<std::int16_t>(camera.lookahead);
    if (look < target)
        ++look;
    else if (look > target)
        --look;
    camera.lookahead = static_cast<std::uint16_t>(look);
    const auto center = static_cast<std::uint16_t>((camera.x + camera.lookahead + center_offset)
                                                   & geometry.position_mask);
    const auto delta = static_cast<std::int16_t>(
        static_cast<std::uint16_t>((static_cast<unsigned>(rider.motion.x) << geometry.screen_shift)
                                   - (static_cast<unsigned>(center) << geometry.screen_shift)));
    int vx{};
    if (delta < geometry.follow_window_low || delta >= geometry.follow_window_high)
        vx = delta < 0 ? -top_speed : top_speed;
    else {
        const int distance = delta >> geometry.screen_shift;
        vx = distance < 0 ? std::min(0, distance + dead_zone) : std::max(0, distance - dead_zone);
    }
    camera.velocity_x = static_cast<std::uint16_t>(vx);
    const int vy = split_vertical_velocity(rider.motion.y, camera.y);
    camera.velocity_y = static_cast<std::uint16_t>(vy);
    camera.x = static_cast<std::uint16_t>(camera.x + vx) & geometry.position_mask;
    camera.y = static_cast<std::uint16_t>(camera.y + vy);
}

void publish_split_rider(ZoomZooCamera& camera, const RiderMovementState& rider,
                         const TrackGeometry& geometry, std::uint16_t offscreen, unsigned y_offset,
                         std::uint16_t& provisional) {
    const int dy = static_cast<std::int16_t>(rider.motion.y - camera.y);
    const auto dx = static_cast<std::int16_t>(rider.motion.x - camera.x);
    const int scaled = static_cast<std::int16_t>(static_cast<std::uint16_t>(
        static_cast<unsigned>(static_cast<std::uint16_t>(dx)) << geometry.screen_shift));
    const bool outside = dy < visible_top || dy >= split_visible_bottom
                      || scaled < geometry.visible_left || scaled >= geometry.visible_right;
    provisional = (outside || scaled < 0) ? 1 : 0;
    camera.screen_xy =
        outside ? offscreen
                : static_cast<std::uint16_t>(((static_cast<unsigned>(dy) + y_offset) & 255U) * 256U
                                             + (static_cast<unsigned>(dx) & 255U));
}

} // namespace

// $81:9FB0-A16D / $81A520-A53F; PAL one-player camera at 4x horizontal scale.
// Positions are world units, signed velocity and lookahead are whole units.
void update_camera(ZoomZooState& state, const TrackGeometry& geometry) {
    if (state.split_screen) {
        follow_split_rider(state.race.camera, state.movement.riders[0], geometry);
        follow_split_rider(state.race.second_camera, state.movement.riders[1], geometry);
        return;
    }
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
        static_cast<std::uint16_t>((c.x + c.lookahead + center_offset) & geometry.position_mask);
    const auto delta = static_cast<std::int16_t>(
        static_cast<std::uint16_t>((static_cast<unsigned>(rider.motion.x) << geometry.screen_shift)
                                   - (static_cast<unsigned>(center) << geometry.screen_shift)));
    int vx{};
    // Outside the window the $81:9FF8-A02B half-world test reduces to the sign of the wrapped difference.
    if (delta < geometry.follow_window_low || delta >= geometry.follow_window_high)
        vx = delta < 0 ? -top_speed : top_speed;
    else {
        const int distance = delta >> geometry.screen_shift;
        vx = distance < 0 ? std::min(0, distance + dead_zone) : std::max(0, distance - dead_zone);
    }
    c.velocity_x = static_cast<std::uint16_t>(vx);
    int candidate = static_cast<std::int16_t>(c.y) + vertical_start;
    int vy = -top_speed;
    while (vy != top_speed && candidate < static_cast<std::int16_t>(rider.motion.y)) {
        candidate += vertical_step;
        ++vy;
        if (vy == 0) candidate += vertical_stop_step;
    }
    c.velocity_y = static_cast<std::uint16_t>(vy);
    c.x = static_cast<std::uint16_t>(c.x + vx) & geometry.position_mask;
    c.y = static_cast<std::uint16_t>(c.y + vy);
}

// Whether the player is on screen, and where: the published screen position the presentation
// and the next update's speed limit read.
void update_visibility(ZoomZooState& state, const TrackGeometry& geometry) {
    if (state.split_screen) {
        publish_split_rider(state.race.camera, state.movement.riders[0], geometry, off_screen, 0,
                            state.race.provisional_1225);
        publish_split_rider(state.race.second_camera, state.movement.riders[1], geometry,
                            second_off_screen, split_visible_bottom, state.race.provisional_1227);
        state.opponent_retained_oam_x =
            static_cast<std::uint8_t>(state.race.second_camera.screen_xy);
        return;
    }
    auto& c = state.race.camera;
    const auto& rider = state.movement.riders[0];
    // $82:ACAE-AD7A. The previous frame's visibility feeds speed damping.
    const int dy = static_cast<std::int16_t>(rider.motion.y - c.y);
    const auto dx = static_cast<std::int16_t>(rider.motion.x - c.x);
    const int scaled = static_cast<std::int16_t>(static_cast<std::uint16_t>(
        static_cast<unsigned>(static_cast<std::uint16_t>(dx)) << geometry.screen_shift));
    const bool outside = dy < visible_top || dy >= visible_bottom || scaled < geometry.visible_left
                      || scaled >= geometry.visible_right;
    state.race.provisional_1225 = (outside || scaled < 0) ? 1 : 0;
    state.race.provisional_1227 = 0;
    c.screen_xy = outside ? off_screen
                          : static_cast<std::uint16_t>((static_cast<unsigned>(dy) & 255U) * 256U
                                                       + (static_cast<unsigned>(dx) & 255U));
}

void initialize_split_cameras(ZoomZooState& state) {
    constexpr unsigned camera_margin = 256, camera_cell_mask = 0xfff0;
    state.split_screen = true;
    const auto& motion = state.movement.riders[1].motion;
    state.race.second_camera.x =
        static_cast<std::uint16_t>((motion.x - camera_margin) & camera_cell_mask);
    state.race.second_camera.y =
        static_cast<std::uint16_t>((motion.y - camera_margin) & camera_cell_mask);
    state.race.second_camera.screen_xy = 0x2065; // $82:D76D-D774: initial second-rider OAM.
}

} // namespace unirally
