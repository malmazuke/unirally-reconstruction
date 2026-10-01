#include "audio_voice.hpp"
#include <stdexcept>

namespace unirally {
namespace {
std::uint8_t byte(unsigned value) {
    return static_cast<std::uint8_t>(value);
}
std::uint16_t word(unsigned value) {
    return static_cast<std::uint16_t>(value);
}

// SPC arithmetic's eight-bit quotient, including overflow and divisor zero.
// Only integer division semantics are represented; this is not a CPU model.
std::uint8_t envelope_quotient(unsigned numerator, unsigned divisor) {
    if ((numerator >> 8) < (divisor << 1)) return byte(numerator / divisor);
    return byte(255U - (numerator - (divisor << 9)) / (256U - divisor));
}

// 09B4-0A33. The multiplication's high byte and each subtraction/shift wrap
// before the next operation. Pan is updated after this tick's output volumes.
void update_pan(AudioVoiceArithmetic& voice, std::uint8_t update_counter) {
    if (!voice.volume) return;
    const auto panned = byte(unsigned(voice.volume) * voice.pan >> 8);
    voice.right_volume = byte(2U * byte(voice.volume - panned) - 1U);
    voice.left_volume = byte(2U * panned);
    if (voice.pan & 128)
        voice.left_volume = voice.volume;
    else
        voice.right_volume = voice.volume;
    if ((update_counter & 15) == 0 && voice.volume_decay) {
        voice.volume = byte(unsigned(voice.volume) * voice.volume_decay >> 8);
        if (!voice.volume) {
            voice.left_volume = voice.right_volume = voice.volume_decay = 0;
        }
    }
    const unsigned sum = unsigned(voice.pan) + voice.pan_step;
    const bool boundary = (voice.pan_step & 128) ? sum < 256 : sum >= 256;
    if (boundary) {
        voice.pan = (voice.pan_step & 128) ? 0 : 255;
        voice.pan_step = byte(0U - voice.pan_step);
    } else
        voice.pan = byte(sum);
}

// 0A3F-0A64: alternate between the base note and an unsigned interval.
void update_alternation(AudioVoiceArithmetic& voice) {
    if (!voice.alternate_interval) return;
    voice.alternate_remaining = byte(voice.alternate_remaining - 1U);
    if (voice.alternate_remaining) return;
    if (voice.base_note != voice.alternating_note) {
        voice.alternating_note = voice.base_note;
        voice.alternate_remaining = voice.alternate_second_period;
    } else {
        voice.alternating_note = byte(voice.base_note + voice.alternate_interval);
        voice.alternate_remaining = voice.alternate_first_period;
    }
}

// 0CF0-0D18: base + floor(base * fractional_header_byte / 256), modulo 65536.
void calculate_target_pitch(AudioVoiceArithmetic& voice, const AudioPitchData& data) {
    if (voice.current_note >= data.notes.size() || voice.sample >= data.sample_fraction.size())
        throw std::runtime_error("pitch lookup leaves identified data");
    const auto base = data.notes[voice.current_note];
    voice.target_pitch = word(base + (unsigned(base) * data.sample_fraction[voice.sample] >> 8));
}

// 0A65-0AA8: note-space slide. Bounds use wrapped eight-bit results, as the
// original does, rather than clamping a signed mathematical intermediate.
void update_note_slide(AudioVoiceArithmetic& voice, const AudioPitchData& data) {
    if (!voice.slide_remaining)
        voice.current_note = voice.alternating_note;
    else {
        voice.slide_remaining = byte(voice.slide_remaining - 1U);
        if (voice.slide_remaining) return;
        voice.slide_remaining = voice.slide_interval;
        if (voice.current_note == voice.alternating_note) return;
        if (voice.current_note > voice.alternating_note) {
            voice.current_note = byte(voice.current_note - voice.slide_amount);
            if (voice.current_note < voice.alternating_note)
                voice.current_note = voice.alternating_note;
        } else {
            voice.current_note = byte(voice.current_note + voice.slide_amount);
            if (voice.current_note >= voice.alternating_note)
                voice.current_note = voice.alternating_note;
        }
    }
    calculate_target_pitch(voice, data);
}

// 0AA9-0AF6: delayed fractional modulation. Negative amount zero deliberately
// adds FF00; replacing the bytewise negation with signed -0 changes behavior.
void update_modulation(AudioVoiceArithmetic& voice) {
    if (voice.modulation_delay) {
        --voice.modulation_delay;
        return;
    }
    if (!voice.modulation_direction) return;
    const unsigned delta = (voice.modulation_direction & 128)
                             ? 0xff00U | byte(0U - voice.modulation_amount)
                             : voice.modulation_amount;
    voice.modulation_offset = word(voice.modulation_offset + delta);
    voice.modulation_remaining = byte(voice.modulation_remaining - 1U);
    if (voice.modulation_remaining) return;
    voice.modulation_remaining = voice.modulation_period;
    if (voice.modulation_period) voice.modulation_direction = byte(0U - voice.modulation_direction);
}

// 0AF7-0B41: converge the base pitch towards the lookup result, preserving the
// wrapped add/subtract then unsigned comparison used at 0995-09A6.
void update_pitch_convergence(AudioVoiceArithmetic& voice) {
    if (!voice.convergence_step) {
        voice.base_pitch = voice.target_pitch;
        return;
    }
    if (voice.base_pitch == voice.target_pitch) return;
    if (voice.base_pitch > voice.target_pitch) {
        const unsigned delta = 0xff00U | byte(0U - voice.convergence_step);
        voice.base_pitch = word(voice.base_pitch + delta);
        if (voice.base_pitch < voice.target_pitch) voice.base_pitch = voice.target_pitch;
    } else {
        voice.base_pitch = word(voice.base_pitch + voice.convergence_step);
        if (voice.base_pitch >= voice.target_pitch) voice.base_pitch = voice.target_pitch;
    }
}

// 0B8A-0C65: seven-byte software envelope. Instrument units are update counts
// and raw gain values. Phase 2 sustains, phase 4 has completed release.
void update_envelope(AudioVoiceArithmetic& voice) {
    // 0B8A-0B8D: a gain script (control 8C, stepped by the score) owns GAIN.
    if (voice.scripted_envelope) return;
    if (voice.remaining && voice.remaining == voice.release_remaining) {
        voice.envelope_phase = 3;
        voice.release_gain = voice.gain;
        voice.envelope_position = 0;
    }
    voice.envelope_timer = byte(voice.envelope_timer - 1U);
    if (voice.envelope_timer) return;
    auto& instrument = voice.instrument;
    switch (voice.envelope_phase) {
    case 0:
        if (instrument[2]) {
            const unsigned difference = byte(instrument[3] - instrument[1]);
            const auto quotient =
                envelope_quotient(difference * voice.envelope_position, byte(instrument[2] - 1U));
            voice.gain = byte(quotient + instrument[1]);
            voice.envelope_position = byte(voice.envelope_position + 1U);
            if (voice.envelope_position == instrument[2]) {
                ++voice.envelope_phase;
                voice.envelope_position = 0;
            }
        }
        break;
    // Phases 1 and 3: 0C13/0C4A branch on the flags of MOV Y,A (the nonzero
    // remaining step), not on the popped length (POP sets no flags), so a zero
    // length still divides, with the hardware's divisor-zero quotient.
    case 1:
        if (byte(instrument[4] - voice.envelope_position - 1U) == 0) {
            voice.gain = instrument[5];
            ++voice.envelope_phase;
            voice.envelope_position = 0;
        } else {
            const unsigned difference = byte(instrument[3] - instrument[5]);
            const unsigned position = byte(instrument[4] - voice.envelope_position - 1U);
            voice.gain =
                byte(envelope_quotient(difference * position, instrument[4]) + instrument[5]);
            voice.envelope_position = byte(voice.envelope_position + 1U);
        }
        break;
    case 2:
    case 4: break;
    case 3:
        if (byte(instrument[6] - voice.envelope_position - 1U) == 0) {
            voice.gain = 0;
            ++voice.envelope_phase;
        } else {
            const unsigned position = byte(instrument[6] - voice.envelope_position - 1U);
            voice.gain = envelope_quotient(unsigned(voice.release_gain) * position, instrument[6]);
            voice.envelope_position = byte(voice.envelope_position + 1U);
        }
        break;
    default: throw std::runtime_error("software envelope phase outside recovered table");
    }
    voice.envelope_timer = instrument[0];
}
} // namespace

void update_audio_voice(AudioVoiceArithmetic& voice, const AudioPitchData& pitch_data,
                        std::uint8_t update_counter) {
    update_pan(voice, update_counter);
    update_alternation(voice);
    update_note_slide(voice, pitch_data);
    update_modulation(voice);
    update_pitch_convergence(voice);
    update_envelope(voice);
    voice.output_pitch = word(unsigned(voice.base_pitch) + voice.modulation_offset + voice.detune);
}

// 0C66-0CB9 and 0D19-0D5B. A zero note retains pitch/envelope state. Duration
// and inline volume follow this initialization in the score format.
void initialize_audio_note(AudioVoiceArithmetic& voice, const AudioPitchData& pitch_data,
                           std::uint8_t note, std::uint8_t transpose, std::uint8_t modulation_delay,
                           std::uint8_t modulation_period, std::uint8_t modulation_direction,
                           bool restart_envelope) {
    if (!note) return;
    if (voice.sample >= pitch_data.sample_transpose.size())
        throw std::runtime_error("sample transpose leaves identified data");
    voice.base_note = byte(unsigned(note) + pitch_data.sample_transpose[voice.sample] + transpose);
    voice.alternating_note = voice.base_note;
    voice.slide_remaining = voice.slide_interval;
    if (!voice.slide_remaining) {
        voice.current_note = voice.base_note;
        calculate_target_pitch(voice, pitch_data);
    }
    voice.modulation_delay = modulation_delay;
    voice.modulation_remaining = byte(modulation_period >> 1);
    voice.modulation_direction = modulation_direction;
    voice.modulation_offset = 0;
    voice.alternate_remaining = voice.alternate_second_period;
    if (restart_envelope) {
        voice.envelope_phase = voice.envelope_position = 0;
        voice.envelope_timer = 1;
    }
}

} // namespace unirally
