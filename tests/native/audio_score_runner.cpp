// Conditional laboratory runner. Input commands/modes are observations, not a
// cold producer, and this executable is never linked into the desktop product.
#include "audio_score.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>

static std::vector<std::uint8_t> read(const char* path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("cannot read score data");
    return {std::istreambuf_iterator<char>(file), {}};
}
int main(int argc, char** argv) {
    try {
        if (argc != 5) throw std::runtime_error("audio_score_runner TABLES TITLE COMMANDS OUT_READS");
        const unirally::AudioSoundSet data{read(argv[1]), read(argv[2])};
        unirally::TitleMenuAudioScore score(data);
        std::ifstream input(argv[3]); std::ofstream output(argv[4], std::ios::binary);
        if (!input || !output) throw std::runtime_error("cannot open command/output file");
        char kind; unsigned index;
        std::uint64_t rows = 0, reads = 0;
        while (input >> kind >> index) {
            if (index > 255 || ++rows > 1000000) throw std::runtime_error("input outside diagnostic bounds");
            const auto parameter = static_cast<std::uint8_t>(index);
            if (kind == 'M') score.start_music(parameter);
            else if (kind == 'E') score.start_effect(parameter);
            else if (kind == 'V') {
                unsigned mode;
                if (!(input >> mode) || mode > 1) throw std::runtime_error("invalid update mode");
                score.update_voice(parameter, mode != 0);
            } else throw std::runtime_error("unknown diagnostic command");
            for (const auto& event : score.take_reads()) {
                const char bytes[] = {static_cast<char>(event.voice), static_cast<char>(event.pointer),
                                      static_cast<char>(event.pointer >> 8), static_cast<char>(event.value)};
                output.write(bytes, 4); ++reads;
            }
        }
        if (!input.eof() || !output) throw std::runtime_error("input or output failure");
        std::cout << "conditional_commands=" << rows << " score_reads=" << reads << '\n';
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
