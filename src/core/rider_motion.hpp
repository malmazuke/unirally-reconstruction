#pragma once
// A rider's horizontal drive, jump, gravity, damping and position integration.
// Internal to the race engine; the public interface is zoom_zoo_movement.hpp.

#include "zoom_zoo_movement.hpp"

namespace unirally {

void update_horizontal(RiderMovementState& rider, bool brake, bool accelerate, bool opponent,
                       const MovementState& whole, const MovementContent& content,
                       int& animation_override, bool& use_throttle_target);
void update_jump(RiderMovementState& rider, bool jump_input);
void update_active_low_speed_damping(RiderMovementState& rider);
void apply_finish_slowdown(RiderMovementState& rider);
void update_gravity(RiderMovementState& rider);
void integrate_motion(RiderMovementState& rider);

} // namespace unirally
