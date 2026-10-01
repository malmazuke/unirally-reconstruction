#pragma once

#include <array>
#include <cstdint>

namespace unirally {

// Identified lookup values from 1407/145C and sample-header fractional pitch.
// The table is data; no executable upload or captured voice state is required.
struct AudioPitchData {
    std::array<std::uint16_t, 85> notes{};
    std::array<std::uint8_t, 64> sample_fraction{};
    std::array<std::uint8_t, 64> sample_transpose{};
};

// All byte fields retain unsigned eight-bit storage. Direction/step sign tests
// inspect bit 7; intermediate products and pitch additions are explicit below.
// Gain is the DSP GAIN register value, not a floating-point amplitude.
struct AudioVoiceArithmetic {
    std::uint8_t volume = 0, pan = 0, pan_step = 0, volume_decay = 0;
    std::uint8_t left_volume = 0, right_volume = 0, remaining = 0;
    std::uint8_t sample = 0, gain = 0;
    std::uint8_t base_note = 0, alternating_note = 0, current_note = 0;
    std::uint8_t alternate_interval = 0, alternate_first_period = 0, alternate_second_period = 0;
    std::uint8_t alternate_remaining = 0;
    std::uint8_t slide_interval = 0, slide_amount = 0, slide_remaining = 0;
    std::uint8_t modulation_direction = 0, modulation_amount = 0, modulation_delay = 0;
    std::uint8_t modulation_remaining = 0, modulation_period = 0;
    std::uint16_t modulation_offset = 0, target_pitch = 0, base_pitch = 0, output_pitch = 0;
    std::uint8_t convergence_step = 0, detune = 0;
    std::uint8_t envelope_phase = 0, envelope_timer = 0, envelope_position = 0;
    std::uint8_t release_gain = 0, release_remaining = 0;
    std::array<std::uint8_t, 7> instrument{};
    bool scripted_envelope = false;
    bool operator==(const AudioVoiceArithmetic&) const = default;
};

// R-0075: semantic readings 09B4-0C65 and conditional update comparisons.
// Clock scheduling and note initialization are separate from these updates.
void update_audio_voice(AudioVoiceArithmetic& voice, const AudioPitchData& pitch_data,
                        std::uint8_t update_counter);

void initialize_audio_note(AudioVoiceArithmetic& voice, const AudioPitchData& pitch_data,
                           std::uint8_t note, std::uint8_t transpose,
                           std::uint8_t modulation_delay, std::uint8_t modulation_period,
                           std::uint8_t modulation_direction, bool restart_envelope);

}  // namespace unirally
