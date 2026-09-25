#include "track_progress.hpp"
#include <array>
#include <stdexcept>

namespace unirally {
namespace {
constexpr unsigned sample_tile_mask = 0x03FF;    // operand at ROM file 0x008BB6
constexpr unsigned progress_tag_mask = 0x1C00;   // operand at ROM file 0x0197A4
constexpr unsigned progress_tag_shift = 9;       // XBA then LSR, $82:97A6–97A7
constexpr unsigned transition_table_stride = 16; // $80:84CB/84DB/84EB/84FB/850B
constexpr std::array<int, 4> transition_steps = {1, 2, -1, -2};
}

void advance_track_progress(TrackProgress& state, std::span<const std::uint8_t> transition_tables) {
    const unsigned tag = (state.marker_word & progress_tag_mask) >> progress_tag_shift;
    if (state.previous_tag != 0 && state.previous_tag != tag) {
        // Read in original order; a negative table entry stops the search.
        // The fifth table's matching branch also rejects the transition, so
        // reaching it cannot change the outcome and needs no speculative name.
        bool accepted = false;
        for (unsigned table = 0; table < transition_steps.size(); ++table) {
            const unsigned offset = table * transition_table_stride + state.previous_tag;
            if (offset >= transition_tables.size() || transition_tables.size() - offset < 2) {
                throw std::out_of_range("progress transition table is unavailable");
            }
            const unsigned candidate = transition_tables[offset]
                                     | (static_cast<unsigned>(transition_tables[offset + 1]) << 8);
            if ((candidate & 0x8000U) != 0) break;
            if (candidate == tag) {
                const int changed =
                    static_cast<int>(state.transition_count) + transition_steps[table];
                state.transition_count = static_cast<std::uint16_t>(changed);
                accepted = true;
                break;
            }
        }
        if (!accepted) {
            state.transition_rejected = true;
            return; // The original preserves previous_tag on rejection.
        }
    }
    state.previous_tag = static_cast<std::uint16_t>(tag);
    state.transition_rejected = false;
}

void observe_track_markers(TrackProgress& state, const TrackSamples& samples) {
    // Original scan is from sample 9 down to sample 0 ($81:8B93 onward).
    // Several points can write a marker; preserve the last write's priority.
    for (auto sample = samples.rbegin(); sample != samples.rend(); ++sample) {
        if ((*sample & sample_tile_mask) != 0) continue;
        const unsigned tag_bits = *sample & progress_tag_mask;
        if (tag_bits != 0 && tag_bits != progress_tag_mask) state.marker_word = *sample;
    }
}
} // namespace unirally

namespace unirally {
void update_track_progress(ProgressUpdateState& state, const std::array<TrackSamples, 2>& samples,
                           std::span<const std::uint8_t> transition_tables) {
    if (state.phase > 1) throw std::invalid_argument("progress phase is not binary");
    // $83:CCB8–CCBE: SEC; 1 minus the previous $0302 byte. The staging
    // wrappers select player on phase 1, opponent on phase 0. No frame index
    // appears in this recurrence.
    state.phase = static_cast<std::uint8_t>(1U - state.phase);
    const unsigned active_rider = state.phase != 0 ? 0U : 1U;
    advance_track_progress(state.riders[active_rider], transition_tables);
    for (unsigned rider = 0; rider < state.riders.size(); ++rider) {
        observe_track_markers(state.riders[rider], samples[rider]);
    }
}

ProgressBytes serialize_progress(const ProgressUpdateState& state) {
    if (state.phase > 1) throw std::invalid_argument("progress phase is not binary");
    ProgressBytes bytes{};
    std::size_t offset = 0;
    for (const auto& rider : state.riders) {
        for (const auto value : {rider.marker_word, rider.previous_tag, rider.transition_count}) {
            bytes[offset++] = static_cast<std::uint8_t>(value);
            bytes[offset++] = static_cast<std::uint8_t>(value >> 8);
        }
        bytes[offset++] = rider.transition_rejected ? 1 : 0;
    }
    bytes[offset] = state.phase;
    return bytes;
}

ProgressUpdateState deserialize_progress(std::span<const std::uint8_t> bytes) {
    if (bytes.size() != ProgressBytes{}.size())
        throw std::invalid_argument("progress state must contain 15 bytes");
    ProgressUpdateState state{};
    std::size_t offset = 0;
    for (auto& rider : state.riders) {
        for (auto* value : {&rider.marker_word, &rider.previous_tag, &rider.transition_count}) {
            *value = static_cast<std::uint16_t>(bytes[offset]
                                                | (static_cast<unsigned>(bytes[offset + 1]) << 8));
            offset += 2;
        }
        if (bytes[offset] > 1) throw std::invalid_argument("progress rejection flag is not binary");
        rider.transition_rejected = bytes[offset++] != 0;
    }
    if (bytes[offset] > 1) throw std::invalid_argument("progress phase is not binary");
    state.phase = bytes[offset];
    return state;
}
} // namespace unirally
