#pragma once
// The opponent's controller: the AI's inputs and its throttle.
// Internal to the race engine; the public interface is zoom_zoo_movement.hpp.

#include "zoom_zoo_movement.hpp"

namespace unirally {

bool update_zoom_ai(ZoomZooState& state);
void update_zoom_throttle(RiderMovementState& rider, ReflectionTransition& transition,
                          unsigned horizontal, int& animation_override, bool& throttle_target,
                          std::uint16_t& charge_announced, bool leading_support = false,
                          bool bounce_active = false, std::uint16_t drive_step = 24,
                          bool on_mud = false);

} // namespace unirally
