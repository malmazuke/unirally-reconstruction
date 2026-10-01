#include "audio_score.hpp"

namespace unirally {
namespace {
std::uint8_t byte(unsigned value) {
    return static_cast<std::uint8_t>(value);
}
std::uint16_t word(unsigned value) {
    return static_cast<std::uint16_t>(value);
}
constexpr std::uint8_t gain_script_loop_marker = 0x80;
// 12D9-12E7, including its call: read the table at the current index, then
// advance the wrapping eight-bit index.
constexpr unsigned gain_script_read_ticks = 16 + 72;
}

// AUDIO-FIRST-RACE, control 8C (0F62-0F89): period, table pointer; restart at
// index zero, disable the software envelope's restart/hold flags, enable the
// script and apply its first byte immediately as the DSP GAIN value.
void TitleMenuAudioScore::start_gain_script(std::uint8_t index) {
    auto& voice = state_.voices.at(index);
    voice.gain_script_period = read_byte(index);
    const auto low = read_byte(index);
    voice.gain_script = word(low | (unsigned(read_byte(index)) << 8));
    voice.gain_script_index = 0;
    voice.restart_envelope = false;
    voice.envelope_mode_c9c = false;
    voice.arithmetic.scripted_envelope = true;
    voice.gain_script_countdown = 1;
    voice.arithmetic.gain = data_byte(word(voice.gain_script + voice.gain_script_index));
    voice.gain_script_index = byte(voice.gain_script_index + 1U);
    add_work(12 + 10 + 10 + 4 + 10 + 12 + 12 + 4 + 12 + 4 + 10 + gain_script_read_ticks + 10 + 6);
}

// AUDIO-FIRST-RACE, 0B42-0B89. Every `period` updates the next table byte is
// applied: a positive byte is the GAIN value, 80 jumps back to the loop index
// and rereads, any other negative byte holds its position. At the release
// point (remaining == release, loop index still zero) the position and loop
// both move past the table's first negative byte. Units: voice updates.
void TitleMenuAudioScore::step_gain_script(std::uint8_t index) {
    auto& voice = state_.voices.at(index);
    auto& arithmetic = voice.arithmetic;
    add_work(10 + 4 + 8);
    if (!voice.gain_script_countdown)
        add_work(8);
    else {
        add_work(4 + 10);
        voice.gain_script_countdown = byte(voice.gain_script_countdown - 1U);
        if (voice.gain_script_countdown)
            add_work(8);
        else {
            add_work(4);
            while (true) {
                add_work(gain_script_read_ticks);
                const auto value = data_byte(word(voice.gain_script + voice.gain_script_index));
                voice.gain_script_index = byte(voice.gain_script_index + 1U);
                if (!(value & 128)) {
                    add_work(8 + 10);
                    arithmetic.gain = value;
                    break;
                }
                add_work(4 + 4);
                if (value == gain_script_loop_marker) {
                    add_work(4 + 8 + 10 + 8);
                    voice.gain_script_index = voice.gain_script_loop;
                    continue;
                }
                add_work(8 + 10 + 8);
                voice.gain_script_index = byte(voice.gain_script_index - 1U);
                break;
            }
            add_work(10 + 10);
            voice.gain_script_countdown = voice.gain_script_period;
        }
    }
    add_work(8);
    if (!arithmetic.release_remaining || arithmetic.remaining != arithmetic.release_remaining
        || voice.gain_script_loop) {
        add_work(!arithmetic.release_remaining                          ? 8U + 10
                 : arithmetic.remaining != arithmetic.release_remaining ? 4U + 8 + 8 + 10
                                                                        : 4U + 8 + 4 + 8 + 8 + 10);
        return;
    }
    add_work(4 + 8 + 4 + 8 + 4 + 8 + 8 + 8 + 8 + 4);
    std::uint8_t position = 255;
    while (true) {
        position = byte(position + 1U);
        add_work(4 + 12);
        if (data_byte(word(voice.gain_script + position)) & 128) {
            add_work(4);
            break;
        }
        add_work(8);
    }
    add_work(4 + 10 + 10 + 10);
    voice.gain_script_index = voice.gain_script_loop = byte(position + 1U);
}
} // namespace unirally
