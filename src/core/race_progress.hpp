#pragma once
// Race progress: checkpoints, laps, the finish and the result fields.
// Internal to the race engine; the public interface is zoom_zoo_movement.hpp.

#include "zoom_zoo_movement.hpp"

namespace unirally {

void update_zoom_finish(ZoomZooState& state, const ZoomZooContent& content);
void update_zoom_checkpoint(ZoomZooState& state, unsigned index, const ZoomZooContent& content);
ZoomZooResult zoom_result_fields(const ZoomZooRaceState& race, unsigned updates,
                                 bool lap_graph = true);

} // namespace unirally
