#pragma once
#include "audio_cpu_clock.hpp"
#include <array>
#include <vector>

namespace unirally {
// The sound sets a session can upload: the title/menu set (R-0075) and the
// first race's set (R-0076; resources 54/62 and the `$83:FC75` sample slots).
enum class AudioSoundSetId : std::uint8_t { title, first_race };
// Identified resource lengths and score bytes only. No original executable
// transfer payload is an input: its bytes are discarded by native IPL. R-0075.
struct AudioCpuUploadData {
    std::array<std::uint16_t, 58> resource_lengths{};
    std::array<std::uint16_t, 5> race_resource_lengths{}; // resources 58-62 (pack v32)
    std::vector<std::uint8_t> menu_transfer, title_transfer;
    std::vector<std::uint8_t> race_tables_transfer, race_song_transfer; // empty before v32
    std::array<std::uint8_t, 64> sample_slots{}, race_sample_slots{};
    std::array<std::vector<std::uint8_t>, 58> sample_resources;
};
// Transport after the session's first native FF write. Ending at the third final
// jump request is a research boundary, not complete CPU/product acceptance.
void native_audio_cpu_uploads(AudioCpuWorkClock& clock, const AudioCpuUploadData& data,
                              AudioSoundSetId set = AudioSoundSetId::title);
void native_audio_cpu_finish_driver_entry(AudioCpuWorkClock& clock);
void native_audio_cpu_upload_samples(AudioCpuWorkClock& clock, const AudioCpuUploadData& data,
                                     AudioSoundSetId set = AudioSoundSetId::title);
// $82:807E entered from a running driver (`JSL` after `LDX`): its FF request starts
// the driver's stop, then the session's three transfers follow. AUDIO-FIRST-RACE.
void native_audio_cpu_begin_session(AudioCpuWorkClock& clock);
} // namespace unirally
