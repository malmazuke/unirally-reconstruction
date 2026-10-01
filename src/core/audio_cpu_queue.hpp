#pragma once
#include "audio_cpu_clock.hpp"
#include <array>

namespace unirally {
// Ring indices wrap at sixteen; driver-ready initialization sets both to one.
// Parameters occupy the low byte of the original cue word, commands the high.
struct AudioCpuQueueState {
    std::array<std::uint8_t, 16> parameters{}, commands{};
    std::uint8_t read_index = 1, write_index = 1, expected_phase = 128;
    bool operator==(const AudioCpuQueueState&) const = default;
};
bool native_audio_enqueue(AudioCpuWorkClock& clock, AudioCpuQueueState& state, std::uint8_t command,
                          std::uint8_t parameter);
bool native_audio_poll_queue(AudioCpuWorkClock& clock, AudioCpuQueueState& state);
// Cold A119-A169 controls, then B08C's first FADF receiver call. No observed
// enqueue/dispatch clock enters this work; subsequent frame work is open.
void native_audio_bootstrap_queue(AudioCpuWorkClock& clock, AudioCpuQueueState& state);
// Continue FAE3 through the first non-overscan PAL vertical-blank edge.
// Starts after the caller's first receiver invocation, ends before FAF1.
void native_audio_wait_vblank(AudioCpuWorkClock& clock, AudioCpuQueueState& state);
} // namespace unirally
