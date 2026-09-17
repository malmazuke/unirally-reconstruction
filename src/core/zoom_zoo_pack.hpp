#pragma once
#include "content_pack.hpp"
#include "zoom_zoo_movement.hpp"
namespace unirally {
inline ZoomZooContent zoom_zoo_content(const ClassicContentPack& pack) {
    const auto entry=[&](const char* name){return pack.entry(std::string("zoom.")+name);};
    return {{{entry("track-data"),entry("collision-poses"),entry("collision-templates")},
             {entry("tile-tables"),entry("tile-flags")},entry("progress-transitions"),
             entry("pose-slopes"),entry("displacement-table"),entry("idle-pose-table"),
             entry("race-finish-reward-values"),entry("race-finish-reward-classes"),
             {entry("speed-masks"),entry("speed-decrements")}},
             entry("sustained-slope-coefficients"),entry("reflection-pose-table"),
             entry("landing-response-matrices"),entry("race-finish-poses"),entry("roll-pose-table"),entry("roll-direction-table"),entry("roll-reward-weights"),entry("trick-combinations")};
}
// DRAGSTER on the shared race engine (R-0038). The track, its tile columns and
// tile flags are DRAGSTER's own entries; every other table is a track-independent
// ROM table (or the pre-race landing matrices, identical before both races) that
// the two-track pack stores under the zoom.* names it was first extracted with.
// The 25-entry DRAGSTER pack lacks those tables, so this requires the two-track pack.
inline ZoomZooContent dragster_race_content(const ClassicContentPack& pack) {
    auto content=zoom_zoo_content(pack);
    content.movement.sampling.track=pack.entry("physics.track.dragster.data");
    content.movement.flat_contact={pack.entry("physics.track.dragster.tile-columns"),
                                   pack.entry("physics.track.dragster.tile-flags")};
    return content;
}
}
