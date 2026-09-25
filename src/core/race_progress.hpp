#pragma once
// Race progress: checkpoints, laps, the finish and the result fields.
// Internal to the race engine; the public interface is zoom_zoo_movement.hpp.

#include "zoom_zoo_movement.hpp"

namespace unirally {

// The finish display and each finished rider's braking, pose and result announcement.
void update_finish(ZoomZooState& state, const ZoomZooContent& content);
// The checkpoint tile under rider `index`: laps, lap times and the finish.
void update_checkpoints(ZoomZooState& state, unsigned index, const ZoomZooContent& content);
// The result screen's fields at load update `updates` (the lap graph on a tour race).
ZoomZooResult result_fields(const ZoomZooRaceState& race, unsigned updates, bool lap_graph = true);

} // namespace unirally
