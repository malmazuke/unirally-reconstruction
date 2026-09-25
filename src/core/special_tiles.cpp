// The special tiles: boost, mud, corkscrew and loop (R-0047, R-0051).

#include "word_arithmetic.hpp"
#include "zoom_zoo_movement.hpp"

#include <cstdint>
#include <span>
#include <stdexcept>

namespace unirally {

// $81:8690-86FE, the special-tile part of the per-update reset that runs
// before the tile dispatch ($81:858E from $82:8C3A / $82:9119).
void update_special_tile_counters(SpecialTileRider& tiles, ReflectionTransition& transition,
                                  std::uint8_t selected_high) {
    if (tiles.mud_cooldown)
        --tiles.mud_cooldown;
    else if (tiles.mud_exit_pending)
        tiles.mud_exit_pending = 0; // and sound $0213
    // With no corkscrew in the previous update the step, the corkscrew's
    // $0600-$060F poses and, off an inverted contact, the float all end.
    if (!tiles.corkscrew_latch) {
        tiles.corkscrew_step = 0;
        if (transition.pose_override >= 0x600 && transition.pose_override < 0x610)
            transition.pose_override = 0;
        if (!(selected_high & 0x80U)) tiles.corkscrew_float = 0;
    }
    tiles.corkscrew_latch = negative(tiles.corkscrew_latch)
                              ? static_cast<std::uint16_t>(tiles.corkscrew_latch + 1U)
                              : 0;
    if (tiles.physics_hold) --tiles.physics_hold;
}

// $81:871C-875B, the boost tile (flag pair 2); the corkscrew's ejection adds
// `extra` ($0DED) to the push. Leading support bypasses the push and the pose flag.
void apply_boost_tile(RiderMovementState& rider, SurfaceTransition& surface, std::uint16_t extra) {
    rider.motion.velocity_y = 0;
    if (!surface.leading_support) {
        rider.launch_override = 80;
        const auto push = static_cast<std::uint16_t>(extra + 0x80U);
        rider.motion.velocity_x = add_word(
            rider.motion.velocity_x,
            (rider.contact.selected_word & 0x4000U) ? static_cast<std::uint16_t>(0U - push) : push);
        surface.tile_pose = 1;
    }
    surface.tile_pose_enabled = 1;
}

// $81:8999-89F6, flag pair 14 (mud). Entering halves velocity x with an
// arithmetic shift and stops vertical motion (the original also queues sound
// $0212); every update on it holds both counters at 4 and brakes by 5 unless
// velocity x is already within 48 of zero ($FFD0-$002F, N-flag compares).
void update_mud_tile(RiderMovementState& rider, SpecialTileRider& tiles, SurfaceTransition& surface,
                     SpecialTileUpdate& special) {
    auto& velocity = rider.motion.velocity_x;
    if (!tiles.mud_cooldown) {
        velocity = static_cast<std::uint16_t>((velocity >> 1U) | (velocity & 0x8000U));
        rider.motion.velocity_y = 0;
    }
    tiles.mud_cooldown = 4;
    tiles.mud_exit_pending = 4;
    surface.tile_mode = 1;
    if (!negative(velocity)) {
        if (negative(static_cast<std::uint16_t>(velocity - 0x30U))) return;
        velocity = static_cast<std::uint16_t>(velocity - 5U);
    } else {
        if (!negative(static_cast<std::uint16_t>(velocity - 0xffd0U))) return;
        velocity = static_cast<std::uint16_t>(velocity + 5U);
    }
    special.mud_velocity = velocity;
    special.drive_step = 4;
}

// $81:87C2-894F, flag pair 10 (corkscrew). A rider that enters it facing the
// descriptor's way, on the ground and not mid-reflection is carried through
// 48 steps at a fixed speed: poses $0600-$060F, y from the height table, the
// object priority toggled four times, gravity and the drive suspended. Step
// $30 leaves it inverted and reflected; any other entry ejects it with a boost.
void update_corkscrew_tile(RiderMovementState& rider, SpecialTileRider& tiles,
                           SurfaceTransition& surface, ReflectionTransition& transition,
                           SpecialTileUpdate& special, std::span<const std::uint8_t> heights) {
    const auto eject = [&] {
        // $81:87D1-87F5; a first ejection also queues sound $021B.
        tiles.corkscrew_latch = 0xfffc;
        apply_boost_tile(rider, surface, 0x40);
        tiles.corkscrew_step = 0xffff;
    };
    if (negative(tiles.corkscrew_latch)) {
        eject();
        return;
    }
    tiles.corkscrew_latch = 1;
    const auto word = rider.contact.selected_word;
    const auto velocity = rider.motion.velocity_x;
    // $81:87F8-882E: the entry conditions, all against the descriptor's facing.
    if (transition.step || (word & 0x8000U)
        || static_cast<std::int16_t>(rider.contact.previous_unsupported_count) >= 3
        || (rider.pose.reflected ? (negative(velocity) || !(word & 0x4000U))
                                 : ((velocity && !negative(velocity)) || (word & 0x4000U)))) {
        eject();
        return;
    }
    if (!tiles.corkscrew_step) {
        if (transition.pose_override) return;
        tiles.raised_priority = 0; // $0FAF = $26
        rider.motion.response_a = 0;
        rider.motion.response_b = 0;
    }
    rider.launch_override = 80;
    if (tiles.corkscrew_step == 0x31) return;
    if (tiles.corkscrew_step == 0x30) {
        // $81:885F-88B2: leave facing the other way, upside down.
        rider.pose.orientation = rider.pose.reflected ? 0x15 : 0x2b;
        rider.pose.reflected = !rider.pose.reflected;
        tiles.corkscrew_step = 0x31;
        transition.pose_override = 0;
        surface.tile_pose_enabled = 0;
        if (special.contact_skip) --special.contact_skip;
        rider.contact.selected_high = 0x80;
        tiles.reflection_lock = 6;
        surface.mode = 1;
        return;
    }
    const auto step = tiles.corkscrew_step;
    if (step == 1 || step == 0x12 || step == 0x20 || step == 0x2f) tiles.raised_priority ^= 1U;
    tiles.corkscrew_float = 1;
    special.contact_skip = 1;
    surface.tile_pose_enabled = 1;
    rider.motion.velocity_x = rider.pose.reflected ? 0x01ce : 0xfe32;
    rider.motion.velocity_y = 0;
    tiles.physics_hold = 8;
    tiles.corkscrew_step = static_cast<std::uint16_t>(step + 1U);
    auto pose = static_cast<unsigned>(tiles.corkscrew_step >> 1U);
    if (pose >= 0x10) pose -= 0x10;
    transition.pose_override = static_cast<std::uint16_t>(0x600U + pose);
    if (heights.size() != 96) throw std::invalid_argument("corkscrew heights are missing");
    const auto height =
        heights[(rider.pose.rolling ? 48U : 0U) + ((tiles.corkscrew_step - 1U) & 0xffU)];
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
    rider.pose.orientation = (tiles.loop_step && !rider.pose.reflected) ? 0x14 : 0x24;
    rider.pose.rolling = true;
    rider.pose.rolling_level = 2;
    if (tiles.loop_step == 9) rider.pose.reflected = tiles.loop_direction != 0;
}

// $81:837E-84AB, flag pair 26 (the loop). A rider on the ground, falling or
// level, not already posed by another tile and facing the descriptor's way
// enters it: 10 units along, velocity x stopped, pose $0610. Each later
// update is one step: poses $0611-$061F, velocity y $1CE with gravity and
// contact suspended, x moved by the step's offset along the entry direction,
// the reflection locked and surface mode set; step 8 (the top) restores
// gravity and contact. Step $10 ends the loop.
void update_loop_tile(RiderMovementState& rider, SpecialTileRider& tiles,
                      SurfaceTransition& surface, ReflectionTransition& transition,
                      SpecialTileUpdate& special, std::span<const std::uint8_t> offsets) {
    if (offsets.size() != 34) throw std::invalid_argument("loop offsets are missing");
    const auto step = tiles.loop_step;
    const auto end_step = [&] {
        // $81:8490-84A3: step 0 (entry) and step 8 restore contact and gravity.
        if (!step || step == 8) {
            special.contact_skip = 0;
            tiles.corkscrew_float = 0;
            surface.tile_pose_enabled = 0;
        }
        tiles.loop_step = static_cast<std::uint16_t>(step + 1U);
        tiles.loop_cooldown = 3;
    };
    if (!step) {
        const auto pose = transition.pose_override;
        if (negative(rider.motion.velocity_y) || surface.leading_support
            || (pose
                && (negative(static_cast<std::uint16_t>(pose - 0x610U))
                    || !negative(static_cast<std::uint16_t>(pose - 0x620U))))) {
            tiles.loop_step = 0xfffe;
            return;
        }
        tiles.loop_cooldown = 3;
        const bool mirrored = (rider.contact.selected_word & 0x4000U) != 0;
        if (mirrored != rider.pose.reflected) return;
        tiles.loop_direction = mirrored ? 0 : 1;
        transition.pose_override = 0x610;
        rider.motion.x = static_cast<std::uint16_t>(rider.motion.x + (mirrored ? 0xfff6U : 10U));
        rider.motion.velocity_x = 0;
        end_step();
        return;
    }
    if (negative(step)) {
        tiles.loop_step = 0xfffe;
        return;
    }
    if (step == 0x10) {
        tiles.loop_step = 0;
        return;
    }
    transition.pose_override = static_cast<std::uint16_t>(0x610U + (step & 15U));
    rider.motion.response_a = 0;
    rider.motion.response_b = 0;
    rider.motion.velocity_x = 0;
    rider.motion.velocity_y = 0x1ce;
    tiles.reflection_lock = 6;
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
