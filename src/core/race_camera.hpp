#pragma once
// The race camera and what it shows.
// Internal to the race engine; the public interface is zoom_zoo_movement.hpp.

#include "zoom_zoo_movement.hpp"

namespace unirally {

void update_camera(ZoomZooState& state, const TrackGeometry& geometry);
void update_visibility(ZoomZooState& state, const TrackGeometry& geometry);
// The race setup fills the second camera's initial words even in one-view mode.
void initialize_second_camera(ZoomZooState& state);
// $82:D8F3-D904: turn a freshly initialized race into the two-viewport mode.
void initialize_split_cameras(ZoomZooState& state);

} // namespace unirally
