#pragma once
// The opponent's controller: what the AI presses each update.
// Internal to the race engine; the public interface is zoom_zoo_movement.hpp.

#include "zoom_zoo_movement.hpp"

namespace unirally {

// The opponent's inputs for one update; true when a marker turned the AI off.
bool update_opponent_controller(ZoomZooState& state);

} // namespace unirally
