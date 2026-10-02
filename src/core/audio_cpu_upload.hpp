#pragma once
#include "audio_cpu_clock.hpp"
#include <array>
#include <stdexcept>
#include <vector>

namespace unirally {
// The sound sets a session can upload: the title/menu set (R-0075) and the race set with one
// of its five songs (R-0076, R-0077: resource 54's tables, the song's resource 62-66 and the
// `$83:FC75` sample slots). The race ids follow the song resources in order.
enum class AudioSoundSetId : std::uint8_t {
    title,
    race_song_62,
    race_song_63,
    race_song_64,
    race_song_65,
    race_song_66,
    award, // $83:A614: driver 50, tables 52, score 58 (R-0077)
    ending // $83:A507: driver 51, tables 55, score 60
};
constexpr unsigned first_race_song_resource = 62, race_song_resources = 5, sound_set_count = 8;
inline bool is_race_sound_set(AudioSoundSetId id) {
    return id >= AudioSoundSetId::race_song_62 && id <= AudioSoundSetId::race_song_66;
}
inline AudioSoundSetId race_song_sound_set(unsigned resource) {
    if (resource < first_race_song_resource
        || resource >= first_race_song_resource + race_song_resources)
        throw std::invalid_argument("no race song has this resource");
    return static_cast<AudioSoundSetId>(1 + resource - first_race_song_resource);
}
inline unsigned race_song_resource_of(AudioSoundSetId id) {
    return first_race_song_resource + static_cast<unsigned>(id) - 1;
}
// Identified resource lengths and score bytes only. No original executable
// transfer payload is an input: its bytes are discarded by native IPL. R-0075.
struct AudioCpuUploadData {
    std::array<std::uint16_t, 58> resource_lengths{};
    // Resources 58-66: 58-62 from pack v32, 63-66 from v33 (0 before).
    std::array<std::uint16_t, 9> race_resource_lengths{};
    std::vector<std::uint8_t> menu_transfer, title_transfer;
    std::vector<std::uint8_t> race_tables_transfer; // empty before v32
    // The songs' transfers by resource 62-66: 62 from v32, the rest from v33 (empty before).
    std::array<std::vector<std::uint8_t>, race_song_resources> race_song_transfers;
    std::array<std::uint8_t, 64> sample_slots{}, race_sample_slots{};
    // The award's and the endings' tables and score transfers and slots (pack v34; empty before).
    struct ScreenSet {
        std::vector<std::uint8_t> tables_transfer, score_transfer;
        std::array<std::uint8_t, 64> sample_slots{};
    };
    ScreenSet award, ending;
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
