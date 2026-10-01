#pragma once
#include "audio_cpu_clock.hpp"
#include <array>
#include <vector>

namespace unirally {
// Identified resource lengths and score bytes only. No original executable
// transfer payload is an input: its bytes are discarded by native IPL. R-0075.
struct AudioCpuUploadData {
    std::array<std::uint16_t, 58> resource_lengths{};
    std::vector<std::uint8_t> menu_transfer, title_transfer;
};
// Cold transport after the first native FF write. Ending at the third final
// jump request is a research boundary, not complete CPU/product acceptance.
void native_audio_cpu_uploads(AudioCpuWorkClock& clock, const AudioCpuUploadData& data);
void native_audio_cpu_finish_driver_entry(AudioCpuWorkClock& clock);
} // namespace unirally
