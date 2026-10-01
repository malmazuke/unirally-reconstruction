#pragma once
#include "audio_voice.hpp"
#include <vector>

inline std::vector<std::uint8_t*> audio_voice_byte_fields(unirally::AudioVoiceArithmetic& v) {
    return {&v.volume, &v.pan, &v.pan_step, &v.volume_decay, &v.left_volume, &v.right_volume,
            &v.remaining, &v.sample, &v.gain, &v.base_note, &v.alternating_note, &v.current_note,
            &v.alternate_interval, &v.alternate_first_period, &v.alternate_second_period,
            &v.alternate_remaining, &v.slide_interval, &v.slide_amount, &v.slide_remaining,
            &v.modulation_direction, &v.modulation_amount, &v.modulation_delay,
            &v.modulation_remaining, &v.modulation_period, &v.convergence_step, &v.detune,
            &v.envelope_phase, &v.envelope_timer, &v.envelope_position, &v.release_gain,
            &v.release_remaining};
}
inline std::vector<std::uint16_t*> audio_voice_word_fields(unirally::AudioVoiceArithmetic& v) {
    return {&v.modulation_offset, &v.target_pitch, &v.base_pitch, &v.output_pitch};
}
