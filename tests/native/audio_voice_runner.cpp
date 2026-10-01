// Laboratory calculation runner. Each row supplies an observed pre-update
// voice, so this checks arithmetic only, not cold native state or scheduling.
#include "audio_voice.hpp"
#include "audio_voice_timing.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>

static std::vector<std::uint8_t> read(const char* path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("cannot read pitch data");
    return {std::istreambuf_iterator<char>(file), {}};
}
static std::vector<std::uint8_t*> byte_fields(unirally::AudioVoiceArithmetic& v) {
    return {&v.volume, &v.pan, &v.pan_step, &v.volume_decay, &v.left_volume, &v.right_volume,
            &v.remaining, &v.sample, &v.gain, &v.base_note, &v.alternating_note, &v.current_note,
            &v.alternate_interval, &v.alternate_first_period, &v.alternate_second_period,
            &v.alternate_remaining, &v.slide_interval, &v.slide_amount, &v.slide_remaining,
            &v.modulation_direction, &v.modulation_amount, &v.modulation_delay,
            &v.modulation_remaining, &v.modulation_period, &v.convergence_step, &v.detune,
            &v.envelope_phase, &v.envelope_timer, &v.envelope_position, &v.release_gain,
            &v.release_remaining};
}
int main(int argc, char** argv) {
    try {
        if (argc != 5 && argc != 6) throw std::runtime_error("audio_voice_runner PITCH FRACTIONS INPUT OUTPUT [ticks]");
        const bool timing = argc == 6 && std::string(argv[5]) == "ticks";
        if (argc == 6 && !timing) throw std::runtime_error("unknown output mode");
        const auto pitch = read(argv[1]), fractions = read(argv[2]);
        if (pitch.size() != 194 || fractions.size() != 64) throw std::runtime_error("pitch data sizes differ");
        unirally::AudioPitchData data;
        for (std::size_t i = 0; i < data.notes.size(); ++i)
            data.notes[i] = static_cast<std::uint16_t>(pitch[2*i] | unsigned(pitch[2*i+1]) << 8);
        std::copy(fractions.begin(), fractions.end(), data.sample_fraction.begin());
        std::ifstream input(argv[3]); std::ofstream output(argv[4]);
        if (!input || !output) throw std::runtime_error("cannot open arithmetic input/output");
        unsigned counter; std::uint64_t rows = 0;
        while (input >> counter) {
            if (counter > 255 || ++rows > 1000000) throw std::runtime_error("update outside diagnostic bounds");
            unirally::AudioVoiceArithmetic voice;
            const auto fields = byte_fields(voice);
            auto read_byte = [&] {
                unsigned value;
                if (!(input >> value) || value > 255) throw std::runtime_error("invalid voice byte");
                return static_cast<std::uint8_t>(value);
            };
            for (auto* field : fields) *field = read_byte();
            const std::uint16_t* const_words[] = {&voice.modulation_offset, &voice.target_pitch,
                                                 &voice.base_pitch, &voice.output_pitch};
            std::uint16_t* words[] = {&voice.modulation_offset, &voice.target_pitch,
                                      &voice.base_pitch, &voice.output_pitch};
            for (auto* field : words) {
                unsigned value;
                if (!(input >> value) || value > 65535) throw std::runtime_error("invalid voice word");
                *field = static_cast<std::uint16_t>(value);
            }
            for (auto& value : voice.instrument) value = read_byte();
            voice.scripted_envelope = read_byte() != 0;
            if (timing) {
                output << unirally::audio_voice_work_ticks(voice, static_cast<std::uint8_t>(counter), &data) << '\n';
                continue;
            }
            unirally::update_audio_voice(voice, data, static_cast<std::uint8_t>(counter));
            bool first = true;
            auto write = [&](unsigned value) { if (!first) output << ' '; output << value; first = false; };
            for (const auto* field : fields) write(*field);
            for (const auto* field : const_words) write(*field);
            for (const auto value : voice.instrument) write(value);
            write(voice.scripted_envelope); output << '\n';
        }
        if (!input.eof() || !output) throw std::runtime_error("arithmetic input/output failure");
        std::cout << "conditional_voice_updates=" << rows << '\n'; return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
