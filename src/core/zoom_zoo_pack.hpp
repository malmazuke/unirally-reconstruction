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
}
