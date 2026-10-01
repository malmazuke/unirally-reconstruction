// Conditional driver-loop experiment: original CPU write and loader boundaries
// remain laboratory inputs. No driver update timestamps or DSP values are
// inputs.
#include "audio_driver.hpp"
#include "audio_driver_dsp.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>

namespace {
struct PortWrite {
    std::uint64_t ticks;
    std::uint8_t port, value;
};
class Bus final : public unirally::AudioDriverBus {
public:
    std::vector<PortWrite> inputs;
    std::array<std::uint8_t, 4> ports{};
    std::size_t next = 0;
    std::ofstream output;
    std::uint64_t horizon = 0;
    std::unique_ptr<DriverDspDiagnostic> dsp;
    void synchronize(std::uint64_t ticks) {
        while (next < inputs.size() && inputs[next].ticks <= ticks) {
            ports.at(inputs[next].port) = inputs[next].value;
            ++next;
        }
    }
    std::uint8_t read_port(std::uint64_t ticks, std::uint8_t port) override {
        if (ticks > horizon + 1000000)
            throw std::runtime_error("conditional port inputs exhausted");
        synchronize(ticks);
        const auto value = ports.at(port);
        if (ticks < horizon)
            output << "R " << ticks << ' ' << unsigned(port) << ' ' << unsigned(value) << '\n';
        return value;
    }
    void clear_ports(std::uint64_t ticks, std::uint8_t first_port) override {
        synchronize(ticks);
        ports.at(first_port) = ports.at(first_port + 1U) = 0;
    }
    void write_ram(std::uint64_t ticks, std::uint16_t address, std::uint8_t value) override {
        if (dsp) dsp->write_ram(ticks, address, value);
        if (ticks < horizon)
            output << "N " << ticks << ' ' << unsigned(address) << ' ' << unsigned(value) << '\n';
    }
    void write_port(std::uint64_t ticks, std::uint8_t port, std::uint8_t value) override {
        if (ticks < horizon)
            output << "P " << ticks << ' ' << unsigned(port) << ' ' << unsigned(value) << '\n';
    }
    void write_dsp(std::uint64_t ticks, std::uint8_t reg, std::uint8_t value) override {
        if (dsp) dsp->write_register(ticks, reg, value);
        if (ticks < horizon)
            output << "D " << ticks << ' ' << unsigned(reg) << ' ' << unsigned(value) << '\n';
    }
};
std::vector<std::uint8_t> read(const char* path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("cannot read audio data");
    return {std::istreambuf_iterator<char>(file), {}};
}
} // namespace
int main(int argc, char** argv) {
    try {
        if (argc != 8 && argc != 9 && argc != 10)
            throw std::runtime_error("audio_driver_runner TABLES TITLE PITCH "
                                     "FRACTIONS TRANSPOSE INPUT OUTPUT [boot [PCM]]");
        const bool boot = argc >= 9 && std::string(argv[8]) == "boot";
        if (argc >= 9 && !boot) throw std::runtime_error("unknown entry mode");
        const unirally::TitleMenuAudioData score_data{read(argv[1]), read(argv[2])};
        const auto pitch = read(argv[3]), fractions = read(argv[4]), transpose = read(argv[5]);
        if (pitch.size() != 194 || fractions.size() != 64 || transpose.size() != 64)
            throw std::runtime_error("pitch data sizes differ");
        unirally::AudioPitchData data;
        for (std::size_t i = 0; i < data.notes.size(); ++i)
            data.notes[i] =
                static_cast<std::uint16_t>(pitch[2 * i] | unsigned(pitch[2 * i + 1]) << 8);
        std::copy(fractions.begin(), fractions.end(), data.sample_fraction.begin());
        std::copy(transpose.begin(), transpose.end(), data.sample_transpose.begin());
        std::ifstream input(argv[6]);
        Bus bus;
        bus.output.open(argv[7]);
        if (!input || !bus.output) throw std::runtime_error("cannot open driver input/output");
        std::uint64_t entry;
        input >> entry >> bus.horizon;
        if (argc == 10) {
            if (bus.horizon % 64)
                throw std::runtime_error("PCM diagnostic needs a complete DSP batch horizon");
            bus.dsp = std::make_unique<DriverDspDiagnostic>(argv[9], bus.horizon / 2);
        }
        unirally::AudioTimers timers;
        char kind;
        std::uint64_t ticks;
        unsigned address, value;
        while (input >> kind >> ticks >> address >> value) {
            if (value > 255) throw std::runtime_error("invalid register value");
            const auto byte = static_cast<std::uint8_t>(value);
            if (kind == 'C') {
                if (address > 3) throw std::runtime_error("invalid port index");
                bus.inputs.push_back({ticks, static_cast<std::uint8_t>(address), byte});
            } else if (kind == 'T') {
                if (ticks > entry)
                    throw std::runtime_error("timer condition exceeds loader boundary");
                timers.advance_to(ticks);
                if (address == 0xf0) {
                    if (value & 0xf0) throw std::runtime_error("nondefault clock divider");
                    timers.set_gate((value & 8) != 0, (value & 1) != 0);
                } else if (address == 0xf1)
                    timers.write_control(byte);
                else if (address >= 0xfa && address <= 0xfc)
                    timers.write_target(static_cast<std::uint8_t>(address - 0xfa), byte);
                else
                    throw std::runtime_error("invalid timer register");
            } else
                throw std::runtime_error("invalid diagnostic operation");
        }
        if (!input.eof()) throw std::runtime_error("incomplete driver input");
        if (!std::is_sorted(bus.inputs.begin(), bus.inputs.end(),
                            [](const auto& a, const auto& b) { return a.ticks < b.ticks; }))
            throw std::runtime_error("CPU writes move backwards");
        unirally::TitleMenuAudioDriver driver(score_data, data, bus, timers.state(), entry, boot);
        driver.run_until(bus.horizon);
        if (bus.dsp) bus.dsp->finish();
        if (!bus.output) throw std::runtime_error("driver output failed");
        std::cout << "computed_end_ticks=" << driver.ticks() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
