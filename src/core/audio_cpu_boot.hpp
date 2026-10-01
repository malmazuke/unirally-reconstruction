#pragma once
#include <array>
#include <cstdint>

namespace unirally {
class AudioCpuWorkObserver;
class AudioCpuWorkClock;
// CPU milestones from hardware power-on through the first sound-upload write.
// Counts are recovered content-domain values, not observed clock seeds.
// Subsequent upload/producer work and resumable CPU phases remain open. R-0075.
struct AudioBootAssetSizes {
    unsigned palette_bytes = 32, tile_bytes = 8192;
};
std::array<std::uint64_t, 12>
native_audio_cpu_boot_prefix(const AudioBootAssetSizes& sizes = {},
                             AudioCpuWorkObserver* observer = nullptr);
std::array<std::uint64_t, 12>
native_audio_cpu_boot_prefix(AudioCpuWorkClock& clock, const AudioBootAssetSizes& sizes = {});
} // namespace unirally
