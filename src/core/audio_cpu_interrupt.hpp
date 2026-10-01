#pragma once
#include "audio_cpu_clock.hpp"

namespace unirally {
struct AudioCpuInterruptWorkState {
    // $80:A09F-A0A5 clears bit1 of erased FF cartridge flags. A16A clears
    // scroll/palette counters. Values below belong to native cold state.
    std::uint8_t cartridge_flags = 253, scroll = 0, palette_delay = 0, palette_index = 0;
    bool operator==(const AudioCpuInterruptWorkState&) const = default;
};
// Semantic hardware entry, mirrored ROM wrapper and title callback. R-0075.
// Recurring raster/last-cycle dispatch is not provided by this body.
void native_audio_title_interrupt(AudioCpuWorkClock& clock, AudioCpuInterruptWorkState& state);
// First-enable experiment only: starts before F5B8, ends at 9869. Raising
// NMI during blank defers it through the next JSR under the pinned IRQ lock.
void native_audio_first_title_interrupt(AudioCpuWorkClock& clock);
} // namespace unirally
