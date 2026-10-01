// Laboratory timer reads use observed IO boundaries. No driver/CPU scheduling
// or original timer-state seed is supplied, and this is not product acceptance.
#include "audio_timers.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>

int main(int argc, char** argv) {
    try {
        if (argc != 3) throw std::runtime_error("audio_timer_runner COMMANDS OUTPUT");
        std::ifstream input(argv[1]); std::ofstream output(argv[2]);
        if (!input || !output) throw std::runtime_error("cannot open timer input/output");
        unirally::AudioTimers timers;
        char kind; std::uint64_t ticks; unsigned index, value;
        while (input >> kind >> ticks >> index) {
            timers.advance_to(ticks);
            if (kind == 'R') {
                if (index > 2) throw std::runtime_error("invalid timer read index");
                output << unsigned(timers.read_output(static_cast<std::uint8_t>(index))) << '\n';
            } else if (kind == 'W') {
                if (!(input >> value) || value > 255) throw std::runtime_error("invalid timer register value");
                const auto byte = static_cast<std::uint8_t>(value);
                if (index == 0xf0) {
                    if (value & 0xf0) throw std::runtime_error("nondefault timer clock divider is not in this diagnostic");
                    timers.set_gate((value & 8) != 0, (value & 1) != 0);
                } else if (index == 0xf1) timers.write_control(byte);
                else if (index >= 0xfa && index <= 0xfc) timers.write_target(static_cast<std::uint8_t>(index - 0xfa), byte);
                else throw std::runtime_error("unknown timer register");
            } else throw std::runtime_error("unknown timer operation");
        }
        if (!input.eof() || !output) throw std::runtime_error("timer input/output failure");
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
