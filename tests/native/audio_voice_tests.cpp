#include "audio_voice.hpp"
#include <iostream>
#include <stdexcept>

static void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
int main() {
    try {
        unirally::AudioPitchData data;
        data.notes[12] = 0x1000; data.sample_fraction[0] = 128;
        unirally::AudioVoiceArithmetic voice;
        voice.alternating_note = 12; voice.envelope_phase = 4;
        voice.volume = 64; voice.pan_step = 255;
        unirally::update_audio_voice(voice, data, 1);
        require(voice.target_pitch == 0x1800 && voice.output_pitch == 0x1800,
                "fractional sample pitch differs");
        require(voice.left_volume == 0 && voice.right_volume == 64 && voice.pan == 0 && voice.pan_step == 1,
                "negative pan boundary did not reflect");
        voice.modulation_direction = 255; voice.modulation_amount = 0;
        voice.modulation_remaining = 1; voice.modulation_period = 0;
        unirally::update_audio_voice(voice, data, 2);
        require(voice.modulation_offset == 0xff00 && voice.output_pitch == 0x1700,
                "negative zero modulation lost bytewise high carry");
        voice.volume = 1; voice.volume_decay = 1;
        unirally::update_audio_voice(voice, data, 16);
        require(voice.volume == 0 && voice.left_volume == 0 && voice.right_volume == 0 && voice.volume_decay == 0,
                "volume decay did not clear output at zero");

        unirally::AudioVoiceArithmetic release;
        release.alternating_note = 12; release.remaining = release.release_remaining = 8;
        release.gain = 120; release.envelope_phase = 2; release.envelope_timer = 1;
        release.instrument = {1, 0, 3, 120, 2, 60, 4};
        for (const auto gain : {90, 60, 30, 0}) {
            unirally::update_audio_voice(release, data, 1);
            require(release.gain == gain, "release envelope gain differs");
            --release.remaining;
        }
        require(release.envelope_phase == 4, "release did not complete");

        unirally::AudioVoiceArithmetic single;
        single.alternating_note = 12; single.envelope_timer = 1;
        single.instrument = {1, 0, 1, 64, 2, 32, 4};
        unirally::update_audio_voice(single, data, 1);
        require(single.gain == 255 && single.envelope_phase == 1,
                "single-count attack lost the original divisor-zero quotient");
        single.current_note = single.alternating_note = 85;
        bool rejected = false;
        try { unirally::update_audio_voice(single, data, 2); }
        catch (const std::runtime_error&) { rejected = true; }
        require(rejected, "unidentified note lookup accepted");
        std::cout << "native pitch, pan, modulation, envelope and data bounds pass\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
