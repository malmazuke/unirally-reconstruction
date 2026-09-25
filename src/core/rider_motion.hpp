#pragma once
// A rider's drive, brake and throttle, jump, gravity, damping and position integration.
// Internal to the race engine; the public interface is zoom_zoo_movement.hpp.

#include "zoom_zoo_movement.hpp"

namespace unirally {

void update_horizontal(RiderMovementState& rider, bool brake, bool accelerate, bool opponent,
                       const MovementState& whole, const MovementContent& content,
                       int& animation_override, bool& use_throttle_target);
// A race rider's drive for one update; `horizontal` is the held direction, and
// `brake_drops_throttle` is flag pair 12 or a mud cooldown.
void update_drive(RiderMovementState& rider, ReflectionTransition& transition, unsigned horizontal,
                  int& animation_override, bool& throttle_target, std::uint16_t& charge_announced,
                  bool leading_support = false, bool bounce_active = false,
                  std::uint16_t drive_step = 24, bool brake_drops_throttle = false);
void update_jump(RiderMovementState& rider, bool jump_input);
void update_active_low_speed_damping(RiderMovementState& rider);
void apply_finish_slowdown(RiderMovementState& rider);
void update_gravity(RiderMovementState& rider);
void integrate_motion(RiderMovementState& rider);

} // namespace unirally
