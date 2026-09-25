#pragma once
// Rolls: the rider's roll, bounce and their trick rewards.
// Internal to the race engine; the public interface is zoom_zoo_movement.hpp.

#include "zoom_zoo_movement.hpp"

namespace unirally {

void update_zoom_roll(ZoomZooState& state, unsigned index, bool pressed,
                      const ZoomZooContent& content);

} // namespace unirally
