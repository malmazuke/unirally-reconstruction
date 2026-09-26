// The special tiles: boost, mud, corkscrew and loop (R-0047, R-0051).

#include "word_arithmetic.hpp"
#include "zoom_zoo_movement.hpp"

#include <cstdint>
#include <span>
#include <stdexcept>

namespace unirally {
namespace {

// The corkscrew: poses 0x600-0x60F over 48 steps (step 0x30 leaves it, 0x31 is done), the
// drive and gravity held 8 updates a step, a sideways speed of 0x1CE, and the object
// priority toggled on four steps. An ejection pushes 0x40 more than the boost's 0x80.
constexpr std::uint16_t corkscrew_poses = 0x600, corkscrew_pose_end = 0x610;
constexpr std::uint16_t corkscrew_exit_step = 0x30, corkscrew_done = 0x31;
constexpr std::uint16_t corkscrew_hold = 8, corkscrew_speed = 0x01ce, ejection_extra = 0x40;
constexpr std::uint16_t ejected = 0xffff, ejection_latch = 0xfffc; // -1 and -4
constexpr std::size_t corkscrew_heights_size = 96, rolling_heights = 48;
constexpr std::uint16_t exit_orientation_right = 0x2b, exit_orientation_reflected = 0x15;
// The boost: 0x80 along the descriptor's facing, and a launch override of 80.
constexpr std::uint16_t boost_push = 0x80, tile_launch_override = 80;
// Mud: both counters held at 4, a drive step of 4, and a brake of 5 unless velocity x is
// within 48 of zero.
constexpr std::uint16_t mud_hold = 4, mud_drive_step = 4, mud_brake = 5;
constexpr std::uint16_t mud_still_speed = 0x30, mud_still_speed_back = 0xffd0;
// The loop: poses 0x610-0x61F, 10 units in at entry, velocity y 0x1CE (down), 17 steps
// from a 34-byte offset table, a 3-update cooldown; step 8 is the top, 16 the end. A
// refused entry counts up from -2.
constexpr std::uint16_t loop_poses = 0x610, loop_pose_end = 0x620, loop_entry = 10;
constexpr std::uint16_t loop_speed = 0x01ce, loop_top = 8, loop_end = 0x10;
constexpr std::uint16_t loop_refused = 0xfffe, loop_cooldown_updates = 3;
constexpr std::size_t loop_offsets_size = 34;
constexpr std::uint16_t loop_exit_orientation = 0x14, loop_exit_orientation_back = 0x24;
constexpr std::uint16_t loop_top_step = 9;
// Both tiles lock the reflection for 6 updates when they let the rider go.
constexpr std::uint16_t reflection_lock_updates = 6;
constexpr std::uint16_t mirrored_tile = 0x4000, inverted_tile = 0x8000;
constexpr std::uint8_t high_tile = 0x80;

} // namespace

// $81:8690-86FE, the special-tile part of the per-update reset that runs
// before the tile dispatch ($81:858E from $82:8C3A / $82:9124).
void update_special_tile_counters(SpecialTileRider& tiles, ReflectionTransition& transition,
                                  std::uint8_t selected_high) {
    if (tiles.mud_cooldown)
        --tiles.mud_cooldown;
    else if (tiles.mud_exit_pending)
        tiles.mud_exit_pending = 0; // and sound 0x213
    // With no corkscrew in the previous update the step, the corkscrew's
    // poses 0x600-0x60F and, off an inverted contact, the float all end.
    if (!tiles.corkscrew_latch) {
        tiles.corkscrew_step = 0;
        if (transition.pose_override >= corkscrew_poses
            && transition.pose_override < corkscrew_pose_end)
            transition.pose_override = 0;
        if (!(selected_high & high_tile)) tiles.corkscrew_float = 0;
    }
    tiles.corkscrew_latch = negative(tiles.corkscrew_latch)
                              ? static_cast<std::uint16_t>(tiles.corkscrew_latch + 1U)
                              : 0;
    if (tiles.physics_hold) --tiles.physics_hold;
}

bool special_tiles_skipped_contact(const SpecialTileRider& tiles) {
    if (tiles.physics_hold == corkscrew_hold) return true;
    return tiles.loop_cooldown == loop_cooldown_updates && tiles.loop_step >= 2
        && tiles.loop_step <= loop_end && tiles.loop_step != loop_top + 1U;
}

// $81:871C-875B, the boost tile (flag pair 2); the corkscrew's ejection adds
// `extra` ($0DED) to the push. Leading support bypasses the push and the pose flag.
void apply_boost_tile(RiderMovementState& rider, SurfaceTransition& surface, std::uint16_t extra) {
    rider.motion.velocity_y = 0;
    if (!surface.leading_support) {
        rider.launch_override = tile_launch_override;
        const auto push = static_cast<std::uint16_t>(extra + boost_push);
        rider.motion.velocity_x =
            add_word(rider.motion.velocity_x, (rider.contact.selected_word & mirrored_tile)
                                                  ? static_cast<std::uint16_t>(0U - push)
                                                  : push);
        surface.tile_pose = 1;
    }
    surface.tile_pose_enabled = 1;
}

// $81:8999-89F6, flag pair 14 (mud). Entering halves velocity x with an
// arithmetic shift and stops vertical motion (the original also queues sound
// 0x212); every update on it holds both counters at 4 and brakes by 5 unless
// velocity x is already within 48 of zero (0xFFD0-0x002F, N-flag compares).
void update_mud_tile(RiderMovementState& rider, SpecialTileRider& tiles, SurfaceTransition& surface,
                     SpecialTileUpdate& special) {
    auto& velocity = rider.motion.velocity_x;
    if (!tiles.mud_cooldown) {
        velocity = static_cast<std::uint16_t>((velocity >> 1U) | (velocity & 0x8000U));
        rider.motion.velocity_y = 0;
    }
    tiles.mud_cooldown = mud_hold;
    tiles.mud_exit_pending = mud_hold;
    surface.tile_mode = 1;
    if (!negative(velocity)) {
        if (negative(static_cast<std::uint16_t>(velocity - mud_still_speed))) return;
        velocity = static_cast<std::uint16_t>(velocity - mud_brake);
    } else {
        if (!negative(static_cast<std::uint16_t>(velocity - mud_still_speed_back))) return;
        velocity = static_cast<std::uint16_t>(velocity + mud_brake);
    }
    special.mud_velocity = velocity;
    special.drive_step = mud_drive_step;
}

// $81:87C2-894F, flag pair 10 (corkscrew). A rider that enters it facing the
// descriptor's way, on the ground and not mid-reflection is carried through
// 48 steps at a fixed speed: poses 0x600-0x60F, y from the height table, the
// object priority toggled four times, gravity and the drive suspended. Step
// 0x30 leaves it inverted and reflected; any other entry ejects it with a boost.
void update_corkscrew_tile(RiderMovementState& rider, SpecialTileRider& tiles,
                           SurfaceTransition& surface, ReflectionTransition& transition,
                           SpecialTileUpdate& special, std::span<const std::uint8_t> heights) {
    const auto eject = [&] {
        // $81:87D1-87F5; a first ejection also queues sound 0x21B.
        tiles.corkscrew_latch = ejection_latch;
        apply_boost_tile(rider, surface, ejection_extra);
        tiles.corkscrew_step = ejected;
    };
    if (negative(tiles.corkscrew_latch)) {
        eject();
        return;
    }
    tiles.corkscrew_latch = 1;
    const auto word = rider.contact.selected_word;
    const auto velocity = rider.motion.velocity_x;
    // $81:87F8-882E: the entry conditions, all against the descriptor's facing.
    if (transition.step || (word & inverted_tile)
        || static_cast<std::int16_t>(rider.contact.previous_unsupported_count) >= 3
        || (rider.pose.reflected ? (negative(velocity) || !(word & mirrored_tile))
                                 : ((velocity && !negative(velocity)) || (word & mirrored_tile)))) {
        eject();
        return;
    }
    if (!tiles.corkscrew_step) {
        if (transition.pose_override) return;
        tiles.raised_priority = 0; // $0FAF = 0x26
        rider.motion.response_a = 0;
        rider.motion.response_b = 0;
    }
    rider.launch_override = tile_launch_override;
    if (tiles.corkscrew_step == corkscrew_done) return;
    if (tiles.corkscrew_step == corkscrew_exit_step) {
        // $81:885F-88B2: leave facing the other way, upside down.
        rider.pose.orientation =
            rider.pose.reflected ? exit_orientation_reflected : exit_orientation_right;
        rider.pose.reflected = !rider.pose.reflected;
        tiles.corkscrew_step = corkscrew_done;
        transition.pose_override = 0;
        surface.tile_pose_enabled = 0;
        if (special.contact_skip) --special.contact_skip;
        rider.contact.selected_high = high_tile;
        tiles.reflection_lock = reflection_lock_updates;
        surface.mode = 1;
        return;
    }
    const auto step = tiles.corkscrew_step;
    if (step == 1 || step == 0x12 || step == 0x20 || step == 0x2f) tiles.raised_priority ^= 1U;
    tiles.corkscrew_float = 1;
    special.contact_skip = 1;
    surface.tile_pose_enabled = 1;
    rider.motion.velocity_x =
        rider.pose.reflected ? corkscrew_speed : static_cast<std::uint16_t>(0U - corkscrew_speed);
    rider.motion.velocity_y = 0;
    tiles.physics_hold = corkscrew_hold;
    tiles.corkscrew_step = static_cast<std::uint16_t>(step + 1U);
    auto pose = static_cast<unsigned>(tiles.corkscrew_step >> 1U);
    if (pose >= 0x10) pose -= 0x10;
    transition.pose_override = static_cast<std::uint16_t>(corkscrew_poses + pose);
    if (heights.size() != corkscrew_heights_size)
        throw std::invalid_argument("corkscrew heights are missing");
    const auto height = heights[(rider.pose.rolling ? rolling_heights : 0U)
                                + ((tiles.corkscrew_step - 1U) & 0xffU)];
    rider.motion.y =
        static_cast<std::uint16_t>(rider.motion.y + (height < 128 ? height : height - 256));
    special.corkscrew_stepped = true;
}

// $81:85AB-8622, after the reflection lock: a refused loop entry counts back
// up to 0; otherwise the loop cooldown runs down, and on the update it reaches
// 1 the loop's pose override, float and angle sentinel end and the rider is
// posed rolling (level 2), upright, turned back the loop's way from step 9.
// The same reset clears $0F41 (tile_pose_enabled) at $81:8687 anyway.
void update_loop_cooldown(RiderMovementState& rider, SpecialTileRider& tiles,
                          ReflectionTransition& transition) {
    if (negative(tiles.loop_step)) {
        ++tiles.loop_step;
        return;
    }
    if (!tiles.loop_cooldown) {
        tiles.loop_step = 0;
        return;
    }
    if (--tiles.loop_cooldown != 1) return;
    tiles.corkscrew_float = 0;
    transition.pose_override = 0;
    rider.contact.angle_unspecified = false;
    rider.pose.orientation = (tiles.loop_step && !rider.pose.reflected)
                               ? loop_exit_orientation
                               : loop_exit_orientation_back;
    rider.pose.rolling = true;
    rider.pose.rolling_level = 2;
    if (tiles.loop_step == loop_top_step) rider.pose.reflected = tiles.loop_direction != 0;
}

// $81:837E-84AB, flag pair 26 (the loop). A rider on the ground, falling or
// level, not already posed by another tile and facing the descriptor's way
// enters it: 10 units along, velocity x stopped, pose 0x610. Each later
// update is one step: poses 0x611-0x61F, velocity y 0x1CE with gravity and
// contact suspended, x moved by the step's offset along the entry direction,
// the reflection locked and surface mode set; step 8 (the top) restores
// gravity and contact. Step 16 ends the loop.
void update_loop_tile(RiderMovementState& rider, SpecialTileRider& tiles,
                      SurfaceTransition& surface, ReflectionTransition& transition,
                      SpecialTileUpdate& special, std::span<const std::uint8_t> offsets) {
    if (offsets.size() != loop_offsets_size)
        throw std::invalid_argument("loop offsets are missing");
    const auto step = tiles.loop_step;
    const auto end_step = [&] {
        // $81:8490-84A3: step 0 (entry) and step 8 restore contact and gravity.
        if (!step || step == loop_top) {
            special.contact_skip = 0;
            tiles.corkscrew_float = 0;
            surface.tile_pose_enabled = 0;
        }
        tiles.loop_step = static_cast<std::uint16_t>(step + 1U);
        tiles.loop_cooldown = loop_cooldown_updates;
    };
    if (!step) {
        const auto pose = transition.pose_override;
        if (negative(rider.motion.velocity_y) || surface.leading_support
            || (pose
                && (negative(static_cast<std::uint16_t>(pose - loop_poses))
                    || !negative(static_cast<std::uint16_t>(pose - loop_pose_end))))) {
            tiles.loop_step = loop_refused;
            return;
        }
        tiles.loop_cooldown = loop_cooldown_updates;
        const bool mirrored = (rider.contact.selected_word & mirrored_tile) != 0;
        if (mirrored != rider.pose.reflected) return;
        tiles.loop_direction = mirrored ? 0 : 1;
        transition.pose_override = loop_poses;
        rider.motion.x =
            static_cast<std::uint16_t>(rider.motion.x + (mirrored ? 0U - loop_entry : loop_entry));
        rider.motion.velocity_x = 0;
        end_step();
        return;
    }
    if (negative(step)) {
        tiles.loop_step = loop_refused;
        return;
    }
    if (step == loop_end) {
        tiles.loop_step = 0;
        return;
    }
    transition.pose_override = static_cast<std::uint16_t>(loop_poses + (step & 15U));
    rider.motion.response_a = 0;
    rider.motion.response_b = 0;
    rider.motion.velocity_x = 0;
    rider.motion.velocity_y = loop_speed;
    tiles.reflection_lock = reflection_lock_updates;
    surface.mode = 1;
    tiles.corkscrew_float = 1;
    special.contact_skip = 1;
    surface.tile_pose_enabled = 1;
    rider.pose.reflected = tiles.loop_direction == 0;
    const auto offset =
        static_cast<std::uint16_t>(offsets[step * 2U] | (offsets[step * 2U + 1U] << 8U));
    rider.motion.x = static_cast<std::uint16_t>(tiles.loop_direction ? rider.motion.x + offset
                                                                     : rider.motion.x - offset);
    end_step();
}

} // namespace unirally
