#include "race_sound.hpp"
#include "zoom_zoo_movement.hpp"

namespace unirally::race_sound {
namespace {
// Sound driver commands (R-0076).
constexpr std::uint8_t start_effect = 2, music_fade = 3, clear_flag = 6, set_flag = 11;
constexpr std::uint8_t countdown_beep = 15, countdown_go = 16, checkpoint_chime = 14,
                       skid_effect = 13, voice_effect = 17;
constexpr std::uint8_t checkpoint_flag = 20, first_skid_flag = 21;
// $83:E7F2: the finish fade's rate, -96 * 8 per music update.
constexpr std::uint8_t finish_fade_rate = 0xa0;
constexpr std::uint16_t finish_fade_from = 180; // $83:E820 `CMP #$B4`
// $83:E5E7/E639/E68B/E6ED: a race beeps as the countdown first falls below these and sounds
// GO below the last; $83:E5BA/E5C2/E5CA: a stunt event (`$77:074B` 2) beeps as each range
// starts (above 220, then 220, 160 and 100) and has no GO.
constexpr std::uint16_t first_beep = 249, second_beep = 189, third_beep = 129, go_beep = 69;
constexpr std::uint16_t stunt_second_beep = 220, stunt_third_beep = 160, stunt_fourth_beep = 100;
// Every start begins its countdown at 270 ($82:D841), so a stunt event's first range starts
// on that value's update. No stunt event is compared in R-0076: a static reading only.
constexpr std::uint16_t countdown_start = 270;
// $81:828E-829D: a speed within (-256, 256) clears the checkpoint flag.
constexpr std::uint16_t fast_checkpoint = 0x0100;

void enqueue(ZoomZooState& next, std::uint8_t command, std::uint8_t parameter) {
    next.sound_cues.push_back(audio_enqueue(command, parameter));
}
bool beeps(std::uint16_t countdown, bool stunt_event) {
    if (!stunt_event)
        return countdown == first_beep || countdown == second_beep || countdown == third_beep;
    return countdown == countdown_start || countdown == stunt_second_beep
        || countdown == stunt_third_beep || countdown == stunt_fourth_beep;
}
} // namespace

void dispatch(ZoomZooState& next, AudioDispatchSite site) {
    next.sound_cues.push_back(audio_dispatch(site));
}
void pause_frame(ZoomZooState& next, bool opening) {
    constexpr unsigned paused_calls = 8;
    constexpr std::uint8_t pause_fade_out = 0x80;
    for (unsigned call = 0; call < paused_calls; ++call) dispatch(next, AudioDispatchSite::pause);
    if (!opening) return;
    enqueue(next, music_fade, pause_fade_out);
    dispatch(next, AudioDispatchSite::pause);
    dispatch(next, AudioDispatchSite::pause);
}
void pause_continue(ZoomZooState& next) {
    constexpr std::uint8_t pause_fade_in = 0x7f;
    enqueue(next, music_fade, pause_fade_in);
    dispatch(next, AudioDispatchSite::pause);
    dispatch(next, AudioDispatchSite::pause);
}
void countdown(ZoomZooState& next, std::uint16_t countdown, bool stunt_event) {
    if (beeps(countdown, stunt_event)) enqueue(next, start_effect, countdown_beep);
    if (!stunt_event && countdown == go_beep) enqueue(next, start_effect, countdown_go);
    dispatch(next, AudioDispatchSite::countdown);
}
void finish_fade(ZoomZooState& next) {
    if (next.race.finish_delay < finish_fade_from) return;
    enqueue(next, music_fade, finish_fade_rate);
    dispatch(next, AudioDispatchSite::finish_fade);
    dispatch(next, AudioDispatchSite::finish_fade);
}
void checkpoint(ZoomZooState& next, std::uint16_t velocity_x) {
    const auto speed = static_cast<std::int16_t>(velocity_x);
    const bool fast = speed >= fast_checkpoint || speed <= -static_cast<int>(fast_checkpoint);
    enqueue(next, fast ? set_flag : clear_flag, checkpoint_flag);
    enqueue(next, start_effect, checkpoint_chime);
}
void brake_skid(ZoomZooState& next, unsigned rider, bool skidding) {
    enqueue(next, skidding ? set_flag : clear_flag,
            static_cast<std::uint8_t>(first_skid_flag + rider));
    enqueue(next, start_effect, skid_effect);
}
void rotation(ZoomZooState& next, unsigned rider, bool rotating) {
    next.sound_cues.push_back(audio_rotation(rider, rotating));
}
// $81:C198-C216 / $81:C2D7-C357: FF is silent; bit 7 names a plain effect; otherwise the
// voice effect plays with flags from the entry: bits 0 and 1, and its high bits as an index.
void announcement_voice(ZoomZooState& next, unsigned rider, std::uint8_t event,
                        std::span<const std::uint8_t> voices) {
    constexpr std::uint8_t first_voice_event = 72, silent = 0xff;
    if (!event || event >= first_voice_event || event >= voices.size()) return;
    const auto entry = voices[event];
    if (entry == silent) return;
    if (entry & 0x80U) {
        enqueue(next, start_effect, static_cast<std::uint8_t>(entry & 0x7fU));
        return;
    }
    // Player: 30, 36 (bit 0), 35 (bit 1), 32 + entry / 4; opponent: 31, 41, 40, 37 + entry / 4.
    const std::uint8_t base = rider ? 31 : 30, bit0 = rider ? 41 : 36, bit1 = rider ? 40 : 35,
                       index = rider ? 37 : 32;
    enqueue(next, set_flag, base);
    enqueue(next, (entry & 1U) ? set_flag : clear_flag, bit0);
    enqueue(next, (entry & 2U) ? set_flag : clear_flag, bit1);
    enqueue(next, set_flag, static_cast<std::uint8_t>(index + (entry >> 2U)));
    enqueue(next, start_effect, voice_effect);
}
AudioCueList loading(std::uint32_t frames_to_initialization) {
    constexpr std::uint32_t start_setup = 85, sound_upload = 79;
    if (frames_to_initialization == start_setup)
        return {audio_enqueue(start_effect, countdown_beep)};
    if (frames_to_initialization == sound_upload) return {audio_load(AudioSessionLoad::first_race)};
    return {};
}
} // namespace unirally::race_sound
