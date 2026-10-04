#pragma once
#include <array>
#include <cstdint>
#include <string_view>
#include <utility>
#include <vector>

namespace unirally {
// The sound queue operations the native game performs in one frame, in the original's
// program order (AUDIO-FIRST-RACE, R-0076). Game code names where it calls the dispatcher
// (`$82:8035`); the audio side gives each site a frame-anchored CPU clock (D-0010).
enum class AudioDispatchSite : std::uint8_t {
    frame_wait,     // $80:FADF, $83:A923: a front-end frame polls until vertical blank
    race_early,     // $83:CD6E: after the countdown and the finish fade
    race_late,      // $83:CD9F: after the riders, checkpoints and announcements
    countdown,      // $83:E739, $83:E74F, $83:E785: each countdown update
    finish_fade,    // $83:E82C, $83:E830: right after the finish fade's cue
    race_choice,    // $80:99BE: NOW PLAYING's Race fades the menu music at once
    pause,          // $83:F65E-F67A: a paused update's eight calls
    pause_fade,     // $83:F68A-F68E: the fade-out when the pause opens
    pause_continue, // $83:F937-F93B: CONTINUE's fade-in
};
// Sound program sessions the game starts from a running driver ($82:807E).
enum class AudioSessionLoad : std::uint8_t {
    race,         // $83:CA2D-CBEE: the race set with the song the cartridge counter names
                  // (`command` the track, `parameter` the counter 0-5), then its music start
                  // and four polls (R-0076, R-0077)
    title_return, // $80:A0F7-A11C after a race: title set, then the title music start
    award,        // $83:A614-A66B: the medal award's set, its music start and five polls
    ending,       // $83:A507-A55F: a gold ending's set (driver 51), likewise (R-0077)
    award_return, // $83:A721-A770 after either: the title set, then the title music start
    reset_boot,   // $80:A09A after a soft reset: the title set from a running driver, as after a
                  // race (R-0077)
};
// The laboratory cue logs' names of the sessions other than a race's ("race-RESOURCE").
constexpr std::array<std::pair<AudioSessionLoad, std::string_view>, 5> audio_session_names{{
    {AudioSessionLoad::title_return, "title"},
    {AudioSessionLoad::award, "award"},
    {AudioSessionLoad::ending, "ending"},
    {AudioSessionLoad::award_return, "title-award"},
    {AudioSessionLoad::reset_boot, "title-reset"},
}};
inline std::string_view audio_session_name(AudioSessionLoad load) {
    for (const auto& [session, name] : audio_session_names)
        if (session == load) return name;
    return "race";
}
// $83:CA08-CA1E: the race song by the cartridge counter `$77:10B1` after its increment
// modulo 6: resources 64, 62, 63, 64, 65, 66 (R-0076 observation 2).
constexpr std::uint8_t race_song_count = 6;
inline std::uint8_t race_song_resource(std::uint8_t song_counter) {
    constexpr std::uint8_t resources[race_song_count] = {64, 62, 63, 64, 65, 66};
    return resources[song_counter % race_song_count];
}
// `rotation`: a rider's rotation state (`command` the rider, `parameter` 1 rotating); the
// audio side keeps the original's sound latches and enqueues only on a change.
enum class AudioCueKind : std::uint8_t { enqueue, dispatch, load, rotation };
struct AudioCue {
    AudioCueKind kind = AudioCueKind::enqueue;
    std::uint8_t command = 0, parameter = 0; // enqueue: `$82:8000`'s cue word
    AudioDispatchSite site = AudioDispatchSite::frame_wait;
    AudioSessionLoad load = AudioSessionLoad::race;
    bool operator==(const AudioCue&) const = default;
};
inline AudioCue audio_enqueue(std::uint8_t command, std::uint8_t parameter) {
    return {AudioCueKind::enqueue, command, parameter};
}
inline AudioCue audio_dispatch(AudioDispatchSite site) {
    return {AudioCueKind::dispatch, 0, 0, site};
}
inline AudioCue audio_rotation(unsigned rider, bool rotating) {
    return {AudioCueKind::rotation, static_cast<std::uint8_t>(rider),
            static_cast<std::uint8_t>(rotating ? 1 : 0)};
}
inline AudioCue audio_load(AudioSessionLoad load) {
    return {AudioCueKind::load, 0, 0, AudioDispatchSite::frame_wait, load};
}
inline AudioCue audio_race_load(std::uint8_t track, std::uint8_t song_counter) {
    return {AudioCueKind::load, track, song_counter, AudioDispatchSite::frame_wait,
            AudioSessionLoad::race};
}
using AudioCueList = std::vector<AudioCue>;
// A race's sound load timing on the menus' path (R-0077; `race_sound_load_timing`): the frames
// from the sound session's first request to the race's first update, and the frames the
// start's countdown cue ($82:D84A) runs before that request.
struct RaceSoundLoadTiming {
    std::uint32_t upload_frames = 0; // 0: the track's loading is not measured
    std::uint32_t start_cue_lead = 0;
};
} // namespace unirally
