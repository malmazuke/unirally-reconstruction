#pragma once

#include "audio_voice.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace unirally {

// Identified score data, not an uploaded executable or APU snapshot. R-0075.
// Original pointers are 16-bit data-format values; reads outside these two
// bounded ranges fail. Music instruments 1-36 are copied from the title data.
struct TitleMenuAudioData {
    std::vector<std::uint8_t> menu_tables;  // 621 bytes, origin 1600
    std::vector<std::uint8_t> title_score;  // 2200 bytes, origin 1D00
};

struct AudioScoreRead {
    std::uint8_t voice;
    std::uint16_t pointer;
    std::uint8_t value;
    bool operator==(const AudioScoreRead&) const = default;
};

// Score-control parameters and per-voice calculation state. Remaining counts
// are musical/effect updates, independently of the hardware sample clock.
struct AudioScoreVoice {
    std::uint16_t pointer = 0;
    bool enabled = false;
    std::uint8_t effect = 255, priority = 255;
    std::uint8_t remaining = 1;  // Unsigned update count; zero wraps to 255.
    bool per_note_pan = false, next_duration_inline = false;
    std::uint8_t fixed_duration = 0, sample = 0, transpose = 0, detune = 0;
    std::uint8_t release_relative = 0, release_absolute = 0;
    std::uint8_t volume = 0, pan = 0, pan_step = 0;
    std::array<std::uint8_t, 7> instrument{};
    std::uint8_t stack_position = 0;
    std::uint8_t pitch_step = 0, pitch_delay = 0, pitch_rate = 0, pitch_period = 0, pitch_glide = 0;
    std::array<std::uint8_t, 3> pitch_alternate{};
    bool suppress_key_on = false, envelope_mode_c9c = false;
    bool restart_envelope = false;
    std::uint8_t volume_gain = 0;
    AudioVoiceArithmetic arithmetic{};
    bool operator==(const AudioScoreVoice&) const = default;
};

struct AudioScoreState {
    std::array<AudioScoreVoice, 8> voices{};
    std::array<std::uint8_t, 128> stack{};
    std::array<std::uint8_t, 259> instruments{};
    std::uint32_t random_state = 0x1b09133a;
    std::uint8_t timer2_target = 133;
    std::uint8_t music_gain = 48, effect_gain = 64;
    bool operator==(const AudioScoreState&) const = default;
};

// This component reads controls and updates voices. Supplying pitch data enables
// arithmetic; omitting it runs the conditional score-reading diagnostic only.
// It does not yet own cold producer timing, timers, DSP writes or device audio.
// The laboratory runner supplies update modes and consumed command boundaries.
class TitleMenuAudioScore {
public:
    explicit TitleMenuAudioScore(const TitleMenuAudioData& data, const AudioPitchData* pitch_data = nullptr);
    void start_music(std::uint8_t program);
    int start_effect(std::uint8_t effect);  // Selected voice, or -1 if rejected.
    void set_volume_gain(bool effects, std::uint8_t gain);
    std::array<std::uint8_t, 6> voice_register_values(std::uint8_t voice) const;
    void update_voice(std::uint8_t voice, bool effect_tick, std::uint8_t update_counter = 0);
    const AudioScoreState& state() const { return state_; }
    void restore(const AudioScoreState& state);
    std::vector<AudioScoreRead> take_reads();

private:
    const TitleMenuAudioData* data_;
    const AudioPitchData* pitch_data_;
    AudioScoreState state_{};
    std::vector<AudioScoreRead> reads_;
    std::uint8_t data_byte(std::uint16_t pointer) const;
    std::uint8_t read_byte(std::uint8_t voice);
    std::uint16_t read_word(std::uint8_t voice);
    void reset_voice(std::uint8_t voice, std::uint16_t pointer,
                     std::uint8_t effect = 255, std::uint8_t priority = 255);
    void push_byte(std::uint8_t voice, std::uint8_t value);
    std::uint8_t pop_byte(std::uint8_t voice);
    void push_pointer(std::uint8_t voice);
    std::uint8_t random_choice(std::uint8_t count);
    void set_instrument(std::uint8_t voice, std::uint8_t instrument);
    void update_arithmetic(std::uint8_t voice, std::uint8_t update_counter);
};

}  // namespace unirally
