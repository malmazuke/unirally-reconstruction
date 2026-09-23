#pragma once
#include "content_pack.hpp"
#include "zoom_zoo_movement.hpp"
namespace unirally {
// The shared race engine's content for the ZOOM ZOO scenario. Track content
// is ZOOM ZOO's own (`zoom.*`). Track-independent engine tables are read under
// the neutral `physics.*` names the DRAGSTER v1 pack introduced; the two-track
// pack also stores the same bytes under `zoom.*` aliases (eight entries,
// 50,828 bytes), which nothing reads any more and a later profile can drop.
// The remaining `zoom.*` engine tables have no neutral entry yet and keep the
// name they were first extracted with; a rename would need a new profile.
inline ZoomZooContent zoom_zoo_content(const ClassicContentPack& pack) {
    const auto zoom=[&](const char* name){return pack.entry(std::string("zoom.")+name);};
    const auto engine=[&](const char* name){return pack.entry(std::string("physics.")+name);};
    return {{{zoom("track-data"),engine("rider.collision-poses"),engine("rider.collision-templates")},
             {zoom("tile-tables"),zoom("tile-flags")},engine("track.progress-transitions"),
             engine("rider.pose-slopes"),engine("rider.displacement-table"),engine("rider.idle-pose-table"),
             zoom("race-finish-reward-values"),zoom("race-finish-reward-classes"),
             {engine("speed.masks"),engine("speed.decrements")}},
             zoom("sustained-slope-coefficients"),zoom("reflection-pose-table"),
             zoom("landing-response-matrices"),zoom("race-finish-poses"),zoom("roll-pose-table"),zoom("roll-direction-table"),zoom("roll-reward-weights"),zoom("trick-combinations")};
}
// DRAGSTER on the shared race engine (R-0038): the shared content with the
// track, its tile columns and tile flags replaced by DRAGSTER's own entries.
// The 25-entry DRAGSTER pack lacks the engine tables that only have `zoom.*`
// entries, so this requires the two-track pack.
inline ZoomZooContent dragster_race_content(const ClassicContentPack& pack) {
    auto content=zoom_zoo_content(pack);
    content.movement.sampling.track=pack.entry("physics.track.dragster.data");
    content.movement.flat_contact={pack.entry("physics.track.dragster.tile-columns"),
                                   pack.entry("physics.track.dragster.tile-flags")};
    return content;
}
// Any race track's engine content (TRACK-BREADTH part 3): the shared content
// with the track's own decoded data, tile columns and tile flags, which pack
// profile v10 carries as `track.NN.*` for the tracks beyond the first two.
inline std::string classic_track_entry(ClassicRaceTrack track,const char* part) {
    return std::string("track.")+char('0'+track.index/10U)+char('0'+track.index%10U)+'.'+part;
}
inline ZoomZooContent classic_race_content(const ClassicContentPack& pack,ClassicRaceTrack track) {
    if(track==ClassicRaceTrack::ZoomZoo)return zoom_zoo_content(pack);
    if(track==ClassicRaceTrack::Dragster)return dragster_race_content(pack);
    auto content=zoom_zoo_content(pack);
    content.movement.sampling.track=pack.entry(classic_track_entry(track,"data"));
    content.movement.flat_contact={pack.entry(classic_track_entry(track,"tile-columns")),
                                   pack.entry(classic_track_entry(track,"tile-flags"))};
    return content;
}
}
