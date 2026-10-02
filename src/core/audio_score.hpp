#pragma once

#include "audio_voice.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace unirally {

// One identified sound set: the effect/instrument tables uploaded to 1600 and
// the music score uploaded to 1D00. It is score data, not an uploaded
// executable or APU snapshot (R-0075; the race set is in AUDIO-FIRST-RACE).
// Original pointers are 16-bit data-format values; reads outside these two
// bounded ranges fail. Music instruments 1-36 are copied from the tables.
// The driver's effect tables in the uploaded tables, by effect: pointer low bytes at 1726, high
// bytes and flags after them. Driver 50 has 32 effects; driver 51, which the gold endings load,
// has 29 (SPC 04CE `CMP #$1D`, 04D4/0504/0513 flags at 1760, 0523 high bytes at 1743; R-0077).
struct AudioEffectTables {
    std::uint8_t count = 32;
    std::uint16_t high = 0x1746, flags = 0x1766;
};
constexpr AudioEffectTables ending_driver_effects{29, 0x1743, 0x1760};
struct AudioSoundSet {
    std::vector<std::uint8_t> tables; // title/menu 621 bytes, race 1,583
    std::vector<std::uint8_t> score;  // title 2,200 bytes, first race song 2,457
    AudioEffectTables effects{};      // the driver's (resource 50 unless noted)
};
std::uint8_t audio_sound_set_byte(const AudioSoundSet& data, std::uint16_t pointer);

struct AudioScoreRead {
    std::uint8_t voice;
    std::uint16_t pointer;
    std::uint8_t value;
    bool operator==(const AudioScoreRead&) const = default;
};

// Native work from voice entry 0915 to its next command poll, or to its
// caller when the stop control returns directly. Units: default SMP ticks.
struct AudioScoreUpdateWork {
    struct Timer2Write {
        std::uint32_t ticks;
        std::uint8_t value;
    };
    // A DSP register the score writes itself (control 9A's noise clock, R-0077).
    struct DspWrite {
        std::uint32_t ticks;
        std::uint8_t reg, value;
    };
    std::uint32_t ticks = 0;
    bool polls_commands = true;
    std::vector<Timer2Write> timer2_writes;
    std::vector<DspWrite> dsp_writes;
};
struct AudioScoreRegisterWork {
    std::array<std::uint32_t, 6> write_ticks{};
    std::uint32_t ticks_to_poll = 0;
    bool writes_registers = false;
};

// Score-control parameters and per-voice calculation state. Remaining counts
// are musical/effect updates, independently of the hardware sample clock.
struct AudioScoreVoice {
    std::uint16_t pointer = 0;
    bool enabled = false;
    bool output_enabled = false;
    std::uint8_t effect = 255, priority = 255;
    std::uint8_t remaining = 1; // Unsigned update count; zero wraps to 255.
    bool per_note_volume = false, next_duration_inline = false;
    std::uint8_t fixed_duration = 0, sample = 0, transpose = 0, detune = 0;
    std::uint8_t release_relative = 0, release_absolute = 0;
    std::uint8_t volume = 0, pan = 0, pan_step = 0;
    std::array<std::uint8_t, 7> instrument{};
    std::uint8_t stack_position = 0;
    std::uint8_t pitch_step = 0, pitch_delay = 0, pitch_rate = 0, pitch_period = 0, pitch_glide = 0;
    std::array<std::uint8_t, 3> pitch_alternate{};
    bool suppress_key_on = false, envelope_mode_c9c = false;
    bool restart_envelope = false;
    // Control 8C's table-driven gain (SPC 0B42): the table pointer is score
    // data; the period counts voice updates between table steps.
    std::uint16_t gain_script = 0;
    std::uint8_t gain_script_index = 0, gain_script_loop = 0;
    std::uint8_t gain_script_period = 0, gain_script_countdown = 0;
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
    std::uint8_t key_on_pending = 0, key_off_pending = 0;
    // R-0077, `$D6`: the voices whose output is noise (DSP register 3D, written by the music
    // pass); controls 98/99 set/clear a voice's bit and a voice's reset clears it (08B4-08B9).
    std::uint8_t noise_voices = 0;
    // Sixty-four flags (SPC 033C-0343) that CPU commands 6/11 clear/set and
    // score controls A5-A8 change or test; flag n is bit n & 7 of byte n >> 3.
    std::array<std::uint8_t, 8> flags{};
    bool operator==(const AudioScoreState&) const = default;
};

// This component reads controls and updates voices. Supplying pitch data enables
// arithmetic; omitting it runs the conditional score-reading diagnostic only.
// It does not yet own cold producer timing, timers, DSP writes or device audio.
// The laboratory runner supplies update modes and consumed command boundaries.
class TitleMenuAudioScore {
public:
    explicit TitleMenuAudioScore(const AudioSoundSet& data,
                                 const AudioPitchData* pitch_data = nullptr);
    void start_music(std::uint8_t program);
    int start_effect(std::uint8_t effect); // Selected voice, or -1 if rejected.
    std::uint32_t start_music_timed(std::uint8_t program);
    std::uint32_t start_effect_timed(std::uint8_t effect);
    void set_volume_gain(bool effects, std::uint8_t gain);
    void set_flag(std::uint8_t flag, bool value);
    bool flag(std::uint8_t flag) const;
    std::array<std::uint8_t, 6> voice_register_values(std::uint8_t voice) const;
    AudioScoreRegisterWork voice_register_work(std::uint8_t voice) const;
    std::uint8_t take_key_on_pending();
    std::uint8_t noise_voices() const { return state_.noise_voices; }
    std::uint8_t take_key_off_pending();
    void update_voice(std::uint8_t voice, bool effect_tick, std::uint8_t update_counter = 0);
    AudioScoreUpdateWork update_voice_timed(std::uint8_t voice, bool effect_tick,
                                            std::uint8_t update_counter);
    const AudioScoreState& state() const { return state_; }
    void restore(const AudioScoreState& state);
    std::vector<AudioScoreRead> take_reads();

private:
    const AudioSoundSet* data_;
    const AudioPitchData* pitch_data_;
    AudioScoreState state_{};
    std::vector<AudioScoreRead> reads_;
    AudioScoreUpdateWork* measured_work_ = nullptr;
    void add_work(unsigned ticks);
    void measure_note_work(const AudioScoreVoice& voice, std::uint8_t note);
    std::uint8_t data_byte(std::uint16_t pointer) const;
    std::uint8_t read_byte(std::uint8_t voice);
    std::uint16_t read_word(std::uint8_t voice);
    void reset_voice(std::uint8_t voice, std::uint16_t pointer, std::uint8_t effect = 255,
                     std::uint8_t priority = 255);
    void push_byte(std::uint8_t voice, std::uint8_t value);
    std::uint8_t pop_byte(std::uint8_t voice);
    void push_pointer(std::uint8_t voice);
    std::uint8_t random_choice(std::uint8_t count);
    void set_instrument(std::uint8_t voice, std::uint8_t instrument);
    void initialize_score_note(std::uint8_t voice, std::uint8_t note, std::uint8_t counter);
    void apply_score_control(std::uint8_t voice, std::uint8_t control);
    void apply_sequence_control(std::uint8_t voice, std::uint8_t control);
    void apply_random_control(std::uint8_t voice, std::uint8_t control);
    void apply_pitch_control(std::uint8_t voice, std::uint8_t control);
    void apply_instrument_control(std::uint8_t voice, std::uint8_t control);
    void apply_mix_control(std::uint8_t voice, std::uint8_t control);
    void update_arithmetic(std::uint8_t voice, std::uint8_t update_counter);
    void start_gain_script(std::uint8_t voice);
    void step_gain_script(std::uint8_t voice);
    void apply_flag_control(std::uint8_t voice, std::uint8_t control);
};

} // namespace unirally
