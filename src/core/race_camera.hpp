#pragma once
// The race camera and what it shows.
// Internal to the race engine; the public interface is zoom_zoo_movement.hpp.

#include "zoom_zoo_movement.hpp"

namespace unirally {

void update_zoom_camera(ZoomZooState& state, const TrackGeometry& geometry);
void update_zoom_visibility(ZoomZooState& state, const TrackGeometry& geometry);

} // namespace unirally
