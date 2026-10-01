#include "audio_score.hpp"
#include "audio_voice_timing.hpp"
#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>

namespace unirally {
namespace {
std::uint8_t byte(unsigned value) { return static_cast<std::uint8_t>(value); }
std::uint16_t word(unsigned value) { return static_cast<std::uint16_t>(value); }
}

TitleMenuAudioScore::TitleMenuAudioScore(const TitleMenuAudioData& data, const AudioPitchData* pitch_data)
    : data_(&data), pitch_data_(pitch_data) {
    if (data.menu_tables.size() != 621 || data.title_score.size() != 2200)
        throw std::invalid_argument("title/menu audio data sizes differ");
    std::copy_n(data.menu_tables.begin() + 0x22, 259, state_.instruments.begin());
}
std::uint8_t TitleMenuAudioScore::data_byte(std::uint16_t pointer) const {
    if (pointer >= 0x1600 && pointer < 0x186d) return data_->menu_tables[pointer - 0x1600];
    if (pointer >= 0x1d00 && pointer < 0x2598) return data_->title_score[pointer - 0x1d00];
    throw std::runtime_error("score pointer outside identified data: " + std::to_string(pointer));
}
std::uint8_t TitleMenuAudioScore::read_byte(std::uint8_t voice_index) {
    auto& voice = state_.voices.at(voice_index);
    const auto pointer = voice.pointer;
    const auto value = data_byte(pointer);
    // Read, increment the 16-bit score pointer, recognize its sign and return;
    // the page carry adds six ticks. Include the native call's 16 ticks.
    add_work((pointer & 255) == 255 ? 66U : 60U);
    voice.pointer = word(pointer + 1U);
    reads_.push_back({voice_index, pointer, value});
    return value;
}
void TitleMenuAudioScore::add_work(unsigned ticks) {
    if (measured_work_) measured_work_->ticks += ticks;
}
AudioScoreUpdateWork TitleMenuAudioScore::update_voice_timed(std::uint8_t index,
                                                            bool effect_tick,
                                                            std::uint8_t counter) {
    if (!pitch_data_) throw std::invalid_argument("timed score updates require pitch data");
    AudioScoreUpdateWork result;
    measured_work_ = &result;
    try { update_voice(index, effect_tick, counter); }
    catch (...) { measured_work_ = nullptr; throw; }
    measured_work_ = nullptr;
    return result;
}

// 0C66-0D77. Count semantic note initialization and duration/release work;
// read_byte accounts for each inline parameter independently. No code bytes
// or captured clocks are inputs. Nonzero scripted pitch data is not recovered.
void TitleMenuAudioScore::measure_note_work(const AudioScoreVoice& voice, std::uint8_t note) {
    if (!measured_work_) return;
    if (voice.arithmetic.scripted_envelope)
        throw std::runtime_error("note initialization leaves the recovered timed domain");
    if (!note) add_work(8);
    else {
        // Sample/voice transposition, target lookup, pitch/modulator reset.
        add_work(4 + 8 + 4 + 10 + 10 + 4 + 4 + 10 + 16 + 36);
        add_work(12 + 12 + 4 + 10 + 10);
        add_work(voice.arithmetic.slide_interval ? 8U : 4U + 10 + 16 + 198);
        add_work(16 + 104 + 10 + 8 + 10 + 10 + 16);
        if (!voice.restart_envelope) add_work(28);
        else add_work(voice.envelope_mode_c9c ? 42U : 76U);
        add_work(voice.suppress_key_on ? 18U : 54U);
    }
    add_work(voice.per_note_pan ? 10U + 4 + 16 + 12 + 10 : 18U);
    add_work(16);  // Duration update call.
    if (voice.next_duration_inline) add_work(10 + 8 + 4 + 12 + 10 + 10);
    else if (!voice.fixed_duration) add_work(10 + 4 + 10 + 4 + 4 + 12 + 10 + 10);
    else add_work(10 + 4 + 10 + 8 + 10 + 10);
    add_work(16 + (voice.release_relative ? 64U : 48U) + 10);
}
std::uint16_t TitleMenuAudioScore::read_word(std::uint8_t voice) {
    const auto low = read_byte(voice);
    const auto high = read_byte(voice);
    return word(low | (unsigned(high) << 8));
}
void TitleMenuAudioScore::push_byte(std::uint8_t index, std::uint8_t value) {
    auto& voice = state_.voices.at(index);
    if (voice.stack_position >= state_.stack.size())
        throw std::runtime_error("score stack leaves recovered domain");
    state_.stack[voice.stack_position] = value;
    voice.stack_position = byte(voice.stack_position + 1U);
}
std::uint8_t TitleMenuAudioScore::pop_byte(std::uint8_t index) {
    auto& voice = state_.voices.at(index);
    voice.stack_position = byte(voice.stack_position - 1U);
    if (voice.stack_position >= state_.stack.size()) throw std::runtime_error("score stack underflow");
    return state_.stack[voice.stack_position];
}
void TitleMenuAudioScore::push_pointer(std::uint8_t index) {
    const auto pointer = state_.voices.at(index).pointer;
    push_byte(index, byte(pointer)); push_byte(index, byte(pointer >> 8));
}
void TitleMenuAudioScore::reset_voice(std::uint8_t index, std::uint16_t pointer,
                                     std::uint8_t effect, std::uint8_t priority) {
    // 080D-08C9: clear 35 absolute and 19 direct-page fields, initialize the
    // tag/duration/stack index, clear mode masks and accumulate key-off.
    add_work(4 + 35 * 12 + 19 * 10 + 4 + 12 + 4 + 12 + 10 + 4 + 10 + 4 * 24 + 10);
    auto& voice = state_.voices.at(index);
    voice = AudioScoreVoice{};
    voice.pointer = pointer; voice.enabled = true;
    voice.effect = effect; voice.priority = priority;
    voice.volume_gain = effect == 255 ? state_.music_gain : state_.effect_gain;
    voice.stack_position = byte(16U * index);
    state_.key_off_pending |= byte(1U << index);
}
std::uint8_t TitleMenuAudioScore::take_key_on_pending() {
    const auto mask = state_.key_on_pending; state_.key_on_pending = 0; return mask;
}
std::uint8_t TitleMenuAudioScore::take_key_off_pending() {
    const auto mask = state_.key_off_pending; state_.key_off_pending = 0; return mask;
}
AudioScoreRegisterWork TitleMenuAudioScore::voice_register_work(std::uint8_t index) const {
    const auto& voice = state_.voices.at(index);
    if (!voice.output_enabled) return {{}, 24, false};
    const unsigned left_scale = voice.arithmetic.left_volume & 128 ? 118U : 120U;
    const unsigned right_scale = voice.arithmetic.right_volume & 128 ? 118U : 120U;
    AudioScoreRegisterWork result;
    result.writes_registers = true;
    result.write_ticks[0] = 10 + 4 + 8 + 10 + 16 + left_scale + 8;
    result.write_ticks[1] = result.write_ticks[0] + 4 + 8 + 10 + 16 + right_scale + 8;
    for (unsigned position = 2; position < 5; ++position)
        result.write_ticks[position] = result.write_ticks[position - 1] + 30;
    result.write_ticks[5] = result.write_ticks[4] + 38;
    result.ticks_to_poll = result.write_ticks[5] + 6;
    return result;
}
std::uint32_t TitleMenuAudioScore::start_music_timed(std::uint8_t program) {
    AudioScoreUpdateWork work;
    measured_work_ = &work;
    try { start_music(program); } catch (...) { measured_work_ = nullptr; throw; }
    measured_work_ = nullptr; return work.ticks;
}
std::uint32_t TitleMenuAudioScore::start_effect_timed(std::uint8_t effect) {
    AudioScoreUpdateWork work;
    measured_work_ = &work;
    try { start_effect(effect); } catch (...) { measured_work_ = nullptr; throw; }
    measured_work_ = nullptr; return work.ticks;
}
void TitleMenuAudioScore::set_volume_gain(bool effects, std::uint8_t gain) {
    if (effects) state_.effect_gain = gain;
    else state_.music_gain = gain;
}
// 1312-137C. Positive music volumes use the current music gain. The negative
// branch uses the gain retained in the voice, including for music.
std::array<std::uint8_t, 6> TitleMenuAudioScore::voice_register_values(std::uint8_t index) const {
    const auto& voice = state_.voices.at(index);
    const auto scale = [&](std::uint8_t value) {
        if (value & 128) {
            const auto magnitude = byte(0U - value);
            return byte(0U - (unsigned(magnitude) * voice.volume_gain >> 8));
        }
        const auto gain = voice.priority == 255 ? state_.music_gain : voice.volume_gain;
        return byte(unsigned(value) * gain >> 8);
    };
    return {scale(voice.arithmetic.left_volume), scale(voice.arithmetic.right_volume),
            byte(voice.arithmetic.output_pitch), byte(voice.arithmetic.output_pitch >> 8),
            voice.sample, voice.arithmetic.gain};
}

// R-0075, SPC 074B-07FF: split voice pointers and 36 seven-byte records.
void TitleMenuAudioScore::start_music(std::uint8_t program) {
    if (program > 1) throw std::invalid_argument("music program outside recovered table");
    add_work(40);
    for (std::uint8_t index = 0; index < 8; ++index) {
        const auto base = word(0x1600U + 4U * index + program);
        const auto high = data_byte(word(base + 2U));
        if (high) {
            add_work((index ? 68U : 64U) + 46);  // Pointer selection and music gain/tag setup.
            reset_voice(index, word(data_byte(base) | (unsigned(high) << 8)));
        } else add_work(26);
    }
    const auto root_index = byte(program - 1U);
    const auto root = word(data_byte(word(0x1620U + root_index)) |
                           (unsigned(data_byte(word(0x1621U + root_index))) << 8));
    // Root selection and 252-byte copy, including return. The 251-count loop
    // copies offsets FB through 01; offset zero is copied after that loop.
    add_work(root >> 8 ? 48U + 4 + 4 + 251 * 36 - 4 + 24 + 10 : 66U);
    if (root == 0x0101 && program == 0) {
        // Silent-program instruments are a copy of freshly reset native voice
        // fields: FF effect tags followed by zeros. No captured RAM is used.
        std::fill(state_.instruments.begin() + 7, state_.instruments.end(), 0);
        for (std::uint8_t index = 0; index < 8; ++index)
            state_.instruments[7 + 2U * index] = state_.voices[index].effect;
    } else if (root >> 8) {
        for (unsigned offset = 0; offset < 252; ++offset)
            state_.instruments[7 + offset] = data_byte(word(root + offset));
    }
}

// R-0075, SPC 04CC-052D: reuse matching tags, then free voices, then lowest
// priority. Descending search and equal-priority replacement order matter.
int TitleMenuAudioScore::start_effect(std::uint8_t effect) {
    if (effect >= 32) throw std::invalid_argument("effect outside recovered table");
    const auto flags = data_byte(word(0x1766U + effect));
    add_work(34);
    int selected = -1;
    if ((flags & 128) == 0)
        for (int index = 7; index >= 0; --index) {
            if (state_.voices[static_cast<std::size_t>(index)].effect == effect) {
                add_work(18); selected = index; break;
            }
            add_work(index ? 30U : 26U);
        }
    if (selected < 0) {
        add_work(4);
        for (int index = 7; index >= 0; --index) {
            if (!state_.voices[static_cast<std::size_t>(index)].enabled) {
                add_work(18); selected = index; break;
            }
            add_work(index ? 30U : 26U);
        }
    }
    if (selected < 0) {
        std::uint8_t lowest = 255;
        add_work(8);
        for (int index = 7; index >= 0; --index) {
            const auto priority = state_.voices[static_cast<std::size_t>(index)].priority;
            const bool replaces = lowest >= priority;
            add_work((replaces ? 32U : 18U) + 8 + (index ? 8U : 4U));
            if (replaces) { lowest = priority; selected = index; }
        }
        if ((flags & 127) < lowest) { add_work(52); return -1; }
        add_work(42);
    }
    const auto pointer = word(data_byte(word(0x1726U + effect)) |
                              (unsigned(data_byte(word(0x1746U + effect))) << 8));
    add_work(130);
    reset_voice(byte(static_cast<unsigned>(selected)), pointer, effect, flags & 127);
    return selected;
}
std::uint8_t TitleMenuAudioScore::random_choice(std::uint8_t count) {
    // R-0075, SPC 13DA-1406: EF, EE, ED, EC rotation order makes EC the
    // high byte. Doubled score controls enter with carry=1; ADC never overflows.
    unsigned carry = 1;
    for (unsigned round = 0; round < 8; ++round) {
        const auto feedback = (((state_.random_state >> 24) & 0x48) + 0x38 + carry) >> 6 & 1;
        carry = state_.random_state >> 31;
        state_.random_state = (state_.random_state << 1) | feedback;
    }
    return byte(((state_.random_state >> 24) * count) >> 8);
}
void TitleMenuAudioScore::set_instrument(std::uint8_t index, std::uint8_t instrument) {
    const auto offset = 7U * instrument;
    if (offset + 7 > state_.instruments.size()) throw std::runtime_error("instrument outside recovered table");
    std::copy_n(state_.instruments.begin() + offset, 7, state_.voices.at(index).instrument.begin());
    state_.voices.at(index).envelope_mode_c9c = false;
    state_.voices.at(index).restart_envelope = true;
}
void TitleMenuAudioScore::update_arithmetic(std::uint8_t index, std::uint8_t update_counter) {
    if (!pitch_data_) return;
    auto& voice = state_.voices.at(index);
    auto& arithmetic = voice.arithmetic;
    arithmetic.remaining = voice.remaining;
    arithmetic.volume = voice.volume; arithmetic.pan = voice.pan;
    arithmetic.pan_step = voice.pan_step; arithmetic.sample = voice.sample;
    arithmetic.detune = voice.detune; arithmetic.convergence_step = voice.pitch_glide;
    arithmetic.modulation_amount = voice.pitch_rate;
    arithmetic.modulation_period = voice.pitch_period;
    arithmetic.alternate_interval = voice.pitch_alternate[0];
    arithmetic.alternate_first_period = voice.pitch_alternate[1];
    arithmetic.alternate_second_period = voice.pitch_alternate[2];
    arithmetic.instrument = voice.instrument;
    if (measured_work_) add_work(16 + audio_voice_work_ticks(arithmetic, update_counter, pitch_data_) + 6);
    update_audio_voice(arithmetic, *pitch_data_, update_counter);
    voice.volume = arithmetic.volume; voice.pan = arithmetic.pan; voice.pan_step = arithmetic.pan_step;
}
void TitleMenuAudioScore::update_voice(std::uint8_t index, bool effect_tick, std::uint8_t update_counter) {
    auto& voice = state_.voices.at(index);
    if (!voice.enabled) { add_work(20); return; }
    if (effect_tick != (voice.effect != 255)) { add_work(effect_tick ? 52U : 56U); return; }
    add_work(effect_tick ? 50U : 54U);
    add_work(52 + 10);  // Enable voice output, establish masks, decrement duration.
    voice.output_enabled = true;
    voice.remaining = byte(voice.remaining - 1U);
    if (voice.remaining) { add_work(8); update_arithmetic(index, update_counter); return; }
    add_work(4);
    for (unsigned dispatch = 0; dispatch < 4096; ++dispatch) {
        const auto control = read_byte(index);
        if (control < 128) {
            add_work(8 + 16);  // Recognize a note and call note initialization.
            measure_note_work(voice, control);
            if (pitch_data_) {
                voice.arithmetic.sample = voice.sample;
                voice.arithmetic.instrument = voice.instrument;
                voice.arithmetic.alternate_second_period = voice.pitch_alternate[2];
                initialize_audio_note(voice.arithmetic, *pitch_data_, control, voice.transpose,
                                      voice.pitch_delay, voice.pitch_period, voice.pitch_step,
                                      voice.restart_envelope && !voice.envelope_mode_c9c);
            }
            if (control && !voice.suppress_key_on) {
                const auto mask = byte(1U << index);
                state_.key_on_pending |= mask; state_.key_off_pending |= mask;
            }
            if (voice.per_note_pan) voice.volume = read_byte(index);
            if (voice.fixed_duration == 0 || voice.next_duration_inline) {
                voice.next_duration_inline = false;
                voice.remaining = read_byte(index);
            } else voice.remaining = voice.fixed_duration;
            if (pitch_data_) voice.arithmetic.release_remaining = voice.release_relative
                ? byte(voice.remaining - voice.release_relative) : voice.release_absolute;
            update_arithmetic(index, update_counter); return;
        }
        add_work(66);  // Recognize and dispatch a control through its data table.
        switch (control) {
        case 0x80:
            add_work(86); if (measured_work_) measured_work_->polls_commands = false;
            voice.output_enabled = false;
            state_.key_off_pending |= byte(1U << index);
            voice.enabled = false; voice.effect = 255; voice.priority = 0; return;
        case 0x81: add_work(42); voice.pointer = read_word(index); break;
        case 0x82: { add_work(150); const auto target = read_word(index); push_pointer(index); voice.pointer = target; break; }
        case 0x83: { add_work(98); const auto high = pop_byte(index); voice.pointer = word(pop_byte(index) | (unsigned(high) << 8)); break; }
        case 0x84: { add_work(170); const auto repeats = read_byte(index); push_pointer(index); push_byte(index, repeats); break; }
        case 0x85: {
            const auto position = voice.stack_position;
            if (position < 3) throw std::runtime_error("loop has no stored continuation");
            const auto repeats = byte(state_.stack[position - 1U] - 1U);
            if (repeats) {
                add_work(84);
                state_.stack[position - 1U] = repeats;
                voice.pointer = word(state_.stack[position - 3U] | (unsigned(state_.stack[position - 2U]) << 8));
            } else { add_work(58); voice.stack_position = byte(position - 3U); }
            break;
        }
        case 0x88: add_work(18); voice.transpose = read_byte(index); break;
        case 0x89: add_work(16); voice.sample = read_byte(index); break;
        case 0x8d: add_work(18); voice.detune = read_byte(index); break;
        case 0x8e: case 0x8f:
            add_work(control == 0x8e ? 76U : 68U);
            voice.pitch_step = control == 0x8e ? 1 : 255;
            voice.pitch_delay = read_byte(index); voice.pitch_rate = read_byte(index);
            voice.pitch_period = read_byte(index); break;
        case 0x90: add_work(18); voice.pitch_glide = read_byte(index); break;
        case 0x91: add_work(22); voice.pitch_step = 0; break;
        case 0x92: add_work(34); voice.release_relative = read_byte(index); voice.release_absolute = 0; break;
        case 0x96:
            add_work(52);
            for (auto& parameter : voice.pitch_alternate) parameter = read_byte(index);
            voice.arithmetic.alternate_remaining = voice.pitch_alternate[2]; break;
        case 0x97: add_work(16 + 506 + 6); set_instrument(index, read_byte(index)); break;
        case 0x9b: case 0x9c:
            add_work(control == 0x9b ? 50U : 22U);
            voice.envelope_mode_c9c = control == 0x9c;
            if (control == 0x9b) voice.restart_envelope = true;
            break;
        case 0x9e: case 0x9f: add_work(22); voice.suppress_key_on = control == 0x9f; break;
        case 0xa2:
            add_work(330);
            for (auto& parameter : voice.instrument) parameter = read_byte(index);
            voice.envelope_mode_c9c = false; voice.restart_envelope = true; break;
        case 0xa3: {
            add_work(16 + 682 + 4 + 8 + 4 + 8 + 6 + 10 + 8 + 4 + 10 + 6 + 42);
            const auto count = read_byte(index);
            const auto skip = 2U * random_choice(count);
            voice.pointer = word(voice.pointer + skip);
            voice.pointer = read_word(index); break;
        }
        case 0xb3: add_work(30); voice.volume = read_byte(index); voice.pan = read_byte(index); break;
        case 0xb4: add_work(18); voice.pan_step = read_byte(index); break;
        case 0xb6:
            state_.timer2_target = read_byte(index); add_work(8);
            if (measured_work_) measured_work_->timer2_writes.push_back({measured_work_->ticks, state_.timer2_target});
            add_work(6); break;
        case 0xbc: add_work(18); break;  // Global E9 clear affects later voice updates, not score reads.
        case 0xbd: add_work(18); voice.pan = read_byte(index); break;
        case 0xbe: add_work(18); voice.volume = read_byte(index); break;
        case 0xbf: case 0xc0: add_work(22); voice.per_note_pan = control == 0xbf; break;
        default: throw std::runtime_error("unrecovered score control: " + std::to_string(control));
        }
    }
    throw std::runtime_error("score control loop exceeds diagnostic bound");
}
void TitleMenuAudioScore::restore(const AudioScoreState& state) {
    for (const auto& voice : state.voices)
        if (voice.stack_position > state.stack.size()) throw std::invalid_argument("invalid score stack position");
    state_ = state; reads_.clear();
}
std::vector<AudioScoreRead> TitleMenuAudioScore::take_reads() {
    auto result = std::move(reads_); reads_.clear(); return result;
}
}  // namespace unirally
