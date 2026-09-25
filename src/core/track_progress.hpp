#pragma once
#include "track_sampling.hpp"

namespace unirally {

// Names describe the observed transition mechanism. One count is one accepted
// marker transition, not a distance or a lap. Original fields: $0FB5/$0FB7/
// $0FB9/$0FBB in the shared rider workspace; R-0010.
struct TrackProgress {
    std::uint16_t marker_word{};
    std::uint16_t previous_tag{};
    std::uint16_t transition_count{};
    bool transition_rejected{};
};

void advance_track_progress(TrackProgress& state, std::span<const std::uint8_t> transition_tables);
void observe_track_markers(TrackProgress& state, const TrackSamples& samples);

} // namespace unirally

namespace unirally {
struct ProgressUpdateState {
    std::array<TrackProgress, 2> riders{};
    std::uint8_t phase{}; // $0302; required to be 0 or 1
};
using ProgressBytes = std::array<std::uint8_t, 15>;
ProgressBytes serialize_progress(const ProgressUpdateState& state);
ProgressUpdateState deserialize_progress(std::span<const std::uint8_t> bytes);
void update_track_progress(ProgressUpdateState& state, const std::array<TrackSamples, 2>& samples,
                           std::span<const std::uint8_t> transition_tables);
} // namespace unirally
