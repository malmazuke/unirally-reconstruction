#pragma once
// The X trick: a z flip, held as a tabletop, and the head bounce it can charge.
// Internal to the race engine; the public interface is zoom_zoo_movement.hpp.

#include "zoom_zoo_movement.hpp"

namespace unirally {

// The X trick for rider `index` (0 the player, 1 the opponent) and one update.
void update_z_flip(ZoomZooState& state, unsigned index, bool pressed,
                   const ZoomZooContent& content);

} // namespace unirally
