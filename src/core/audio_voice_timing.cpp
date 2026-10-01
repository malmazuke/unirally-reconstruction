#include "audio_voice_timing.hpp"
#include <stdexcept>

namespace unirally {
namespace {
std::uint8_t byte(unsigned value) {
    return static_cast<std::uint8_t>(value);
}

// Costs are groups of native semantic work, in SMP ticks at two ticks per
// default bus/internal cycle. Branch recognition and returns are included.
unsigned pan_work(const AudioVoiceArithmetic& voice, std::uint8_t counter) {
    if (!voice.volume) return 46;
    unsigned ticks = 28 + 4 + 112 + ((voice.pan & 128) ? 32U : 28U);
    if (counter & 15)
        ticks += 18;
    else if (!voice.volume_decay)
        ticks += 32;
    else {
        const auto after = unsigned(voice.volume) * voice.volume_decay >> 8;
        ticks += 14 + 10 + 4 + 4 + 10 + 18 + 4 + 12 + (after ? 8U : 36U);
    }
    const auto total = unsigned(voice.pan) + voice.pan_step;
    const bool negative = (voice.pan_step & 128) != 0;
    const bool reflects = negative ? total < 256 : total >= 256;
    ticks += 10 + (negative ? 8U : 4U) + 4 + 10;
    ticks += reflects ? 4 + (negative ? 4U : 12U) + 38 : 8;
    return ticks + 12 + 10;
}
unsigned alternation_work(const AudioVoiceArithmetic& voice) {
    if (!voice.alternate_interval) return 28;
    if (byte(voice.alternate_remaining - 1U)) return 42;
    return voice.base_note == voice.alternating_note ? 112 : 102;
}
unsigned note_slide_work(const AudioVoiceArithmetic& voice) {
    if (!voice.slide_remaining) return 272;
    if (byte(voice.slide_remaining - 1U)) return 44;
    auto target = voice.alternating_note;
    if (voice.alternate_interval && !byte(voice.alternate_remaining - 1U))
        target = voice.base_note != target ? voice.base_note
                                           : byte(voice.base_note + voice.alternate_interval);
    if (voice.current_note == target) return 86;
    if (voice.current_note > target) {
        const auto after = byte(voice.current_note - voice.slide_amount);
        return after >= target ? 358U : 382U;
    }
    const auto after = byte(unsigned(voice.current_note) + voice.slide_amount);
    return after < target ? 362U : 378U;
}
unsigned convergence_work(const AudioVoiceArithmetic& voice, std::uint16_t target) {
    if (!voice.convergence_step) return 102;
    if (voice.base_pitch == target) return 120;
    if (voice.base_pitch > target) {
        const unsigned delta = 0xff00U | byte(0U - voice.convergence_step);
        const auto after = static_cast<std::uint16_t>(voice.base_pitch + delta);
        return after >= target ? 282U : 356U;
    }
    const auto after =
        static_cast<std::uint16_t>(unsigned(voice.base_pitch) + voice.convergence_step);
    return after < target ? 278U : 344U;
}
unsigned modulation_work(const AudioVoiceArithmetic& voice) {
    if (voice.modulation_delay) return 40;
    if (!voice.modulation_direction) return 44;
    unsigned ticks = (voice.modulation_direction & 128) ? 108U : 102U;
    if (byte(voice.modulation_remaining - 1U)) return ticks + 28;
    return ticks + (voice.modulation_period ? 78U : 52U);
}
unsigned envelope_work(const AudioVoiceArithmetic& voice) {
    unsigned ticks;
    auto phase = voice.envelope_phase;
    auto position = voice.envelope_position;
    if (!voice.remaining)
        ticks = 30;
    else if (voice.remaining != voice.release_remaining)
        ticks = 42;
    else {
        ticks = 84;
        phase = 3;
        position = 0;
    }
    ticks += 10;
    if (byte(voice.envelope_timer - 1U)) return ticks + 8 + 10;
    ticks += 4 + 116; // Dispatch work, including the phase's indirect return.
    switch (phase) {
    case 0:
        if (!voice.instrument[2]) return ticks + 100;
        return ticks + (byte(position + 1U) == voice.instrument[2] ? 234U : 214U);
    case 1:
        if (!byte(voice.instrument[4] - position - 1U)) return ticks + 172;
        return ticks + (voice.instrument[4] ? 212U : 136U);
    case 2:
    case 4: return ticks + 10;
    case 3:
        if (!byte(voice.instrument[6] - position - 1U)) return ticks + 136;
        return ticks + (voice.instrument[6] ? 182U : 116U);
    default: throw std::runtime_error("software envelope phase outside timed domain");
    }
}
} // namespace

std::uint32_t audio_voice_work_ticks(const AudioVoiceArithmetic& voice, std::uint8_t update_counter,
                                     const AudioPitchData* pitch_data) {
    if (voice.scripted_envelope)
        throw std::runtime_error("voice arithmetic leaves the recovered timed domain");
    auto target = voice.target_pitch;
    if (voice.convergence_step) {
        if (!pitch_data) throw std::runtime_error("timed convergence requires pitch data");
        auto advanced = voice;
        update_audio_voice(advanced, *pitch_data, update_counter);
        target = advanced.target_pitch;
    }
    // Scripted-envelope bypass: 28; six calls between phases: 96; final
    // output-pitch sum: 98. The value update remains independently callable.
    return pan_work(voice, update_counter) + alternation_work(voice) + note_slide_work(voice)
         + modulation_work(voice) + convergence_work(voice, target) + 28 + envelope_work(voice) + 96
         + 98;
}
} // namespace unirally
