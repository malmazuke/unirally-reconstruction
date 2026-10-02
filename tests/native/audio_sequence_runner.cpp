// Data-only native state continuation, with observed command/update boundaries.
#include "audio_score.hpp"
#include "audio_voice_fields.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>

static std::vector<std::uint8_t> read(const char* path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("cannot read audio data");
    return {std::istreambuf_iterator<char>(file), {}};
}
int main(int argc, char** argv) {
    try {
        if (argc != 8 && argc != 9) throw std::runtime_error("audio_sequence_runner TABLES TITLE PITCH FRACTIONS TRANSPOSE COMMANDS OUTPUT [registers|ticks|command-ticks|register-ticks|masks]");
        const bool registers = argc == 9 && std::string(argv[8]) == "registers";
        const bool ticks = argc == 9 && std::string(argv[8]) == "ticks";
        const bool command_ticks = argc == 9 && std::string(argv[8]) == "command-ticks";
        const bool register_ticks = argc == 9 && std::string(argv[8]) == "register-ticks";
        const bool masks = argc == 9 && std::string(argv[8]) == "masks";
        if (argc == 9 && !registers && !ticks && !command_ticks && !register_ticks && !masks)
            throw std::runtime_error("unknown output mode");
        const unirally::AudioSoundSet score_data{read(argv[1]), read(argv[2])};
        const auto pitch = read(argv[3]), fractions = read(argv[4]), transpose = read(argv[5]);
        if (pitch.size() != 194 || fractions.size() != 64 || transpose.size() != 64)
            throw std::runtime_error("pitch data sizes differ");
        unirally::AudioPitchData data;
        for (std::size_t i = 0; i < data.notes.size(); ++i)
            data.notes[i] = static_cast<std::uint16_t>(pitch[2*i] | unsigned(pitch[2*i+1]) << 8);
        std::copy(fractions.begin(), fractions.end(), data.sample_fraction.begin());
        std::copy(transpose.begin(), transpose.end(), data.sample_transpose.begin());
        unirally::TitleMenuAudioScore score(score_data, &data);
        std::ifstream input(argv[6]); std::ofstream output(argv[7]);
        if (!input || !output) throw std::runtime_error("cannot open command/output file");
        char kind; unsigned index; std::uint64_t rows = 0, updates = 0;
        while (input >> kind >> index) {
            if (index > 255 || ++rows > 1000000) throw std::runtime_error("input outside diagnostic bounds");
            const auto parameter = static_cast<std::uint8_t>(index);
            if (kind == 'M') {
                if (command_ticks) output << score.start_music_timed(parameter) << '\n';
                else score.start_music(parameter);
            }
            else if (kind == 'E') {
                if (command_ticks) output << score.start_effect_timed(parameter) + 50 << '\n';
                else score.start_effect(parameter);
            }
            else if (kind == 'G') {
                unsigned gain;
                if (index > 1 || !(input >> gain) || gain > 255) throw std::runtime_error("invalid gain command");
                score.set_volume_gain(index != 0, static_cast<std::uint8_t>(gain));
            } else if (kind == 'W') {
                if (!registers || index > 7) throw std::runtime_error("invalid register output request");
                const auto values = score.voice_register_values(parameter);
                for (std::size_t i = 0; i < values.size(); ++i) {
                    if (i) output << ' ';
                    output << unsigned(values[i]);
                }
                output << '\n'; ++updates;
            } else if (kind == 'Q') {
                if (!register_ticks || index > 7) throw std::runtime_error("invalid register work request");
                const auto work = score.voice_register_work(parameter);
                for (const auto offset : work.write_ticks) output << offset << ' ';
                output << work.ticks_to_poll << '\n'; ++updates;
            } else if (kind == 'K') {
                if (index > 1) throw std::runtime_error("invalid key-mask request");
                const auto value = index ? score.take_key_off_pending() : score.take_key_on_pending();
                if (masks) { output << unsigned(value) << '\n'; ++updates; }
            }
            else if (kind == 'V') {
                unsigned mode, counter;
                if (!(input >> mode >> counter) || mode > 1 || counter > 255 || index > 7)
                    throw std::runtime_error("invalid voice update");
                const auto& before = score.state().voices[index];
                const bool active = before.enabled && (mode != 0) == (before.effect != 255);
                if (ticks) {
                    const auto work = score.update_voice_timed(parameter, mode != 0, static_cast<std::uint8_t>(counter));
                    output << work.ticks << ' ' << work.polls_commands << '\n'; ++updates;
                } else score.update_voice(parameter, mode != 0, static_cast<std::uint8_t>(counter));
                if (argc == 8 && active && score.state().voices[index].enabled) {
                    auto voice = score.state().voices[index].arithmetic;
                    bool first = true;
                    auto write = [&](unsigned value) { if (!first) output << ' '; output << value; first = false; };
                    for (const auto* field : audio_voice_byte_fields(voice)) write(*field);
                    for (const auto* field : audio_voice_word_fields(voice)) write(*field);
                    for (const auto value : voice.instrument) write(value);
                    write(voice.scripted_envelope); output << '\n'; ++updates;
                }
            } else throw std::runtime_error("unknown diagnostic command");
            score.take_reads();
        }
        if (!input.eof() || !output) throw std::runtime_error("input or output failure");
        std::cout << "conditional_commands=" << rows << " native_voice_updates=" << updates << '\n';
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
