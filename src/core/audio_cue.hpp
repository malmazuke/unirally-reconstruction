#pragma once
#include <cstdint>
#include <vector>

namespace unirally {
// The sound queue operations the native game performs in one frame, in the original's
// program order (AUDIO-FIRST-RACE, R-0076). Game code names where it calls the dispatcher
// (`$82:8035`); the audio side gives each site a frame-anchored CPU clock (D-0010).
enum class AudioDispatchSite : std::uint8_t {
    frame_wait,  // $80:FADF, $83:A923: a front-end frame polls until vertical blank
    race_early,  // $83:CD6E: after the countdown and the finish fade
    race_late,   // $83:CD9F: after the riders, checkpoints and announcements
    countdown,   // $83:E739, $83:E74F, $83:E785: each countdown update
    finish_fade, // $83:E82C, $83:E830: right after the finish fade's cue
    race_choice, // $80:99BE: NOW PLAYING's Race fades the menu music at once
};
// Sound program sessions the game starts from a running driver ($82:807E).
enum class AudioSessionLoad : std::uint8_t {
    first_race,   // $83:CA72-CBEE: race set, then its music start and four polls
    title_return, // $80:A0F7-A11C after a race: title set, then the title music start
};
enum class AudioCueKind : std::uint8_t { enqueue, dispatch, load };
struct AudioCue {
    AudioCueKind kind = AudioCueKind::enqueue;
    std::uint8_t command = 0, parameter = 0; // enqueue: `$82:8000`'s cue word
    AudioDispatchSite site = AudioDispatchSite::frame_wait;
    AudioSessionLoad load = AudioSessionLoad::first_race;
    bool operator==(const AudioCue&) const = default;
};
inline AudioCue audio_enqueue(std::uint8_t command, std::uint8_t parameter) {
    return {AudioCueKind::enqueue, command, parameter};
}
inline AudioCue audio_dispatch(AudioDispatchSite site) {
    return {AudioCueKind::dispatch, 0, 0, site};
}
inline AudioCue audio_load(AudioSessionLoad load) {
    return {AudioCueKind::load, 0, 0, AudioDispatchSite::frame_wait, load};
}
using AudioCueList = std::vector<AudioCue>;
} // namespace unirally
