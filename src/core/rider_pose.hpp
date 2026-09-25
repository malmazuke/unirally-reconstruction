#pragma once
// A rider's pose: the idle wobble, the pose and animation update, rolling and quarter turns.
// Internal to the race engine; the public interface is zoom_zoo_movement.hpp.

#include "zoom_zoo_movement.hpp"

namespace unirally {

void decay_idle_wobble(RiderMovementState& rider, bool surface_mode = false);
void update_idle_pose(RiderMovementState& rider, bool race_active, bool opponent,
                      std::uint8_t animation_counter, std::span<const std::uint8_t> table);
void update_rolling_mode(RiderMovementState& rider, bool surface_mode = false);
void update_pose(RiderMovementState& rider, std::uint8_t counter, std::uint8_t contact_phase,
                 const MovementContent& content, int animation_override, bool use_throttle_target,
                 int surface_angle_offset = 0, std::uint16_t mud_velocity = 0,
                 bool slow_tile = false);
int stationary_animation_override(const RiderMovementState& rider, std::uint8_t horizontal);
unsigned update_quarter_turns(RiderMovementState& rider, bool leading_support = false,
                              unsigned air_turns = 0, bool rolling = false,
                              unsigned held_rotations = 0);

} // namespace unirally
