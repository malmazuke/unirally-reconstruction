#include "audio_score.hpp"
#include <stdexcept>
#include <string>
namespace unirally {
namespace {
std::uint8_t byte(unsigned value) {
    return static_cast<std::uint8_t>(value);
}
std::uint16_t word(unsigned value) {
    return static_cast<std::uint16_t>(value);
}
[[noreturn]] void reject_score_control(std::uint8_t control) {
    throw std::runtime_error("unrecovered score control: " + std::to_string(control));
}
}
// R-0075, 0C66-0D77. A note initializes pitch/envelope state, then consumes
// its optional volume and wrapping byte duration before one arithmetic update.
void TitleMenuAudioScore::initialize_score_note(std::uint8_t index, std::uint8_t note,
                                                std::uint8_t update_counter) {
    auto& voice = state_.voices.at(index);
    add_work(8 + 16); // Recognize a note and call note initialization.
    measure_note_work(voice, note);
    if (pitch_data_) {
        voice.arithmetic.sample = voice.sample;
        voice.arithmetic.instrument = voice.instrument;
        voice.arithmetic.alternate_second_period = voice.pitch_alternate[2];
        initialize_audio_note(voice.arithmetic, *pitch_data_, note, voice.transpose,
                              voice.pitch_delay, voice.pitch_period, voice.pitch_step,
                              voice.restart_envelope && !voice.envelope_mode_c9c);
    }
    // 0C8F-0CA0: a note restarts an active gain script unless held by C9C.
    if (note && voice.arithmetic.scripted_envelope && !voice.envelope_mode_c9c) {
        voice.gain_script_index = voice.gain_script_loop = 0;
        voice.gain_script_countdown = voice.gain_script_period;
    }
    if (note && !voice.suppress_key_on) {
        const auto mask = byte(1U << index);
        state_.key_on_pending |= mask;
        state_.key_off_pending |= mask;
    }
    if (voice.per_note_volume) voice.volume = read_byte(index);
    if (voice.fixed_duration == 0 || voice.next_duration_inline) {
        voice.next_duration_inline = false;
        voice.remaining = read_byte(index);
    } else
        voice.remaining = voice.fixed_duration;
    if (pitch_data_)
        voice.arithmetic.release_remaining = voice.release_relative
                                               ? byte(voice.remaining - voice.release_relative)
                                               : voice.release_absolute;
    update_arithmetic(index, update_counter);
}
// R-0075, 0E03 control table. Control values are the identified score format.
void TitleMenuAudioScore::apply_score_control(std::uint8_t index, std::uint8_t control) {
    if (control <= 0x86 || control == 0xa3)
        apply_sequence_control(index, control);
    else if (control >= 0xa5 && control <= 0xa8)
        apply_flag_control(index, control);
    else if (control == 0x8c || (control >= 0x97 && control <= 0xa2))
        apply_instrument_control(index, control);
    else if (control <= 0x96)
        apply_pitch_control(index, control);
    else
        apply_mix_control(index, control);
}

// AUDIO-FIRST-RACE, 1142-11A4: set/clear a flag, or jump (as control 81) when
// a flag is set (A7) or clear (A8), otherwise skip the two-byte target.
void TitleMenuAudioScore::apply_flag_control(std::uint8_t index, std::uint8_t control) {
    constexpr unsigned flag_lookup = 84; // 1151-115F and its call
    auto& voice = state_.voices.at(index);
    const auto selected = read_byte(index);
    switch (control) {
    case 0xa5:
        add_work(flag_lookup + 10 + 12 + 6);
        set_flag(selected, true);
        return;
    case 0xa6:
        add_work(flag_lookup + 4 + 10 + 12 + 6);
        set_flag(selected, false);
        return;
    default: break;
    }
    const bool jumps = flag(selected) == (control == 0xa7);
    if (jumps) {
        add_work(flag_lookup + 10 + 4 + 6 + 42);
        voice.pointer = read_word(index);
    } else {
        add_work(flag_lookup + 10 + 8 + 54);
        voice.pointer = word(voice.pointer + 2U);
    }
}

void TitleMenuAudioScore::apply_sequence_control(std::uint8_t index, std::uint8_t control) {
    auto& voice = state_.voices.at(index);
    switch (control) {
    case 0x80:
        add_work(86);
        if (measured_work_) measured_work_->polls_commands = false;
        voice.output_enabled = false;
        state_.key_off_pending |= byte(1U << index);
        voice.enabled = false;
        voice.effect = 255;
        voice.priority = 0;
        return;
    case 0x81:
        add_work(42);
        voice.pointer = read_word(index);
        break;
    case 0x82: {
        add_work(150);
        const auto target = read_word(index);
        push_pointer(index);
        voice.pointer = target;
        break;
    }
    case 0x83: {
        add_work(98);
        const auto high = pop_byte(index);
        voice.pointer = word(pop_byte(index) | (unsigned(high) << 8));
        break;
    }
    case 0x84: {
        add_work(170);
        const auto repeats = read_byte(index);
        push_pointer(index);
        push_byte(index, repeats);
        break;
    }
    case 0x85: {
        const auto position = voice.stack_position;
        if (position < 3) throw std::runtime_error("loop has no stored continuation");
        const auto repeats = byte(state_.stack[position - 1U] - 1U);
        if (repeats) {
            add_work(84);
            state_.stack[position - 1U] = repeats;
            voice.pointer =
                word(state_.stack[position - 3U] | (unsigned(state_.stack[position - 2U]) << 8));
        } else {
            add_work(58);
            voice.stack_position = byte(position - 3U);
        }
        break;
    }
    case 0x86:
        add_work(18);
        voice.fixed_duration = read_byte(index);
        break;
    case 0xa3: {
        add_work(16 + 682 + 4 + 8 + 4 + 8 + 6 + 10 + 8 + 4 + 10 + 6 + 42);
        const auto count = read_byte(index);
        const auto skip = 2U * random_choice(count);
        voice.pointer = word(voice.pointer + skip);
        voice.pointer = read_word(index);
        break;
    }

    default: reject_score_control(control);
    }
}

void TitleMenuAudioScore::apply_pitch_control(std::uint8_t index, std::uint8_t control) {
    auto& voice = state_.voices.at(index);
    switch (control) {
    case 0x88:
        add_work(18);
        voice.transpose = read_byte(index);
        break;
    case 0x89:
        add_work(16);
        voice.sample = read_byte(index);
        break;
    case 0x8d:
        add_work(18);
        voice.detune = read_byte(index);
        break;
    case 0x8e:
    case 0x8f:
        add_work(control == 0x8e ? 76U : 68U);
        voice.pitch_step = control == 0x8e ? 1 : 255;
        voice.pitch_delay = read_byte(index);
        voice.pitch_rate = read_byte(index);
        voice.pitch_period = read_byte(index);
        break;
    case 0x90:
        add_work(18);
        voice.pitch_glide = read_byte(index);
        break;
    case 0x91:
        add_work(22);
        voice.pitch_step = 0;
        break;
    case 0x92:
        add_work(34);
        voice.release_relative = read_byte(index);
        voice.release_absolute = 0;
        break;
    case 0x96:
        add_work(52);
        for (auto& parameter : voice.pitch_alternate) parameter = read_byte(index);
        voice.arithmetic.alternate_remaining = voice.pitch_alternate[2];
        break;

    default: reject_score_control(control);
    }
}

void TitleMenuAudioScore::apply_instrument_control(std::uint8_t index, std::uint8_t control) {
    auto& voice = state_.voices.at(index);
    switch (control) {
    case 0x8c: start_gain_script(index); break;
    case 0x97:
        add_work(16 + 506 + 6);
        set_instrument(index, read_byte(index));
        break;
    case 0x9b:
    case 0x9c:
        add_work(control == 0x9b ? 50U : 22U);
        voice.envelope_mode_c9c = control == 0x9c;
        if (control == 0x9b) {
            voice.restart_envelope = true;
            voice.arithmetic.scripted_envelope = false; // 107E-1083 clears 02D0
        }
        break;
    case 0x9e:
    case 0x9f:
        add_work(22);
        voice.suppress_key_on = control == 0x9f;
        break;
    case 0xa2:
        add_work(330);
        for (auto& parameter : voice.instrument) parameter = read_byte(index);
        voice.envelope_mode_c9c = false; // 10E7 continues at control 9B's 107E
        voice.restart_envelope = true;
        voice.arithmetic.scripted_envelope = false;
        break;

    default: reject_score_control(control);
    }
}

void TitleMenuAudioScore::apply_mix_control(std::uint8_t index, std::uint8_t control) {
    auto& voice = state_.voices.at(index);
    switch (control) {
    case 0xb3:
        add_work(30);
        voice.volume = read_byte(index);
        voice.pan = read_byte(index);
        break;
    case 0xb4:
        add_work(18);
        voice.pan_step = read_byte(index);
        break;
    case 0xb6:
        state_.timer2_target = read_byte(index);
        add_work(8);
        if (measured_work_)
            measured_work_->timer2_writes.push_back({measured_work_->ticks, state_.timer2_target});
        add_work(6);
        break;
    case 0xbc: add_work(18); break; // Global E9 clear affects later voice updates, not score reads.
    case 0xbd:
        add_work(18);
        voice.pan = read_byte(index);
        break;
    case 0xbe:
        add_work(18);
        voice.volume = read_byte(index);
        break;
    case 0xbf:
    case 0xc0:
        add_work(22);
        voice.per_note_volume = control == 0xbf;
        break;

    default: reject_score_control(control);
    }
}
} // namespace unirally
