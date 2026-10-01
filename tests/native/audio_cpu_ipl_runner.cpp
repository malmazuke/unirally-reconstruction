#include "audio_cpu_boot.hpp"
#include "audio_cpu_clock.hpp"
#include "audio_ipl.hpp"
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
struct CpuYield {};
class ColdBus final : public unirally::AudioDriverBus, public unirally::AudioCpuWorkObserver {
public:
    explicit ColdBus(std::ostream& output) : output_(output), ipl_(*this) {}
    std::uint8_t read_port(std::uint64_t ticks, std::uint8_t port) override {
        if (ticks * cpu_frequency >= cpu_completed_ * smp_frequency) throw CpuYield{};
        const auto value = incoming_.at(port);
        emit('R', ticks, port, value);
        return value;
    }
    void write_port(std::uint64_t ticks, std::uint8_t port, std::uint8_t value) override {
        emit('P', ticks, port, value);
    }
    void write_ram(std::uint64_t, std::uint16_t, std::uint8_t) override {}
    void write_dsp(std::uint64_t, std::uint8_t, std::uint8_t) override {
        throw std::logic_error("IPL wrote DSP");
    }
    void clear_ports(std::uint64_t, std::uint8_t) override {
        throw std::logic_error("IPL cleared IO");
    }
    void scanline(std::uint64_t ticks, std::uint64_t completed_ticks) override {
        cpu_master_ = ticks;
        cpu_completed_ = completed_ticks;
        synchronize();
    }
    void write_audio_port(std::uint64_t ticks, std::uint8_t port, std::uint8_t value) override {
        cpu_master_ = cpu_completed_ = ticks;
        synchronize();
        incoming_.at(port) = value;
        emit('C', ipl_.state().ticks, port, value);
    }

private:
    static constexpr std::uint64_t cpu_frequency = 21281370, smp_frequency = 2050560;
    std::ostream& output_;
    unirally::AudioIplHandshake ipl_;
    std::array<std::uint8_t, 4> incoming_{};
    std::uint64_t cpu_master_ = 0, cpu_completed_ = 0;
    void emit(char kind, std::uint64_t ticks, std::uint8_t port, std::uint8_t value) {
        output_ << kind << ' ' << ticks << ' ' << cpu_master_ << ' ' << unsigned(port) << ' '
                << unsigned(value) << '\n';
    }
    void synchronize() {
        if (ipl_.state().ticks * cpu_frequency >= cpu_completed_ * smp_frequency) return;
        try {
            ipl_.run_until(std::numeric_limits<std::uint64_t>::max());
        } catch (const CpuYield&) {}
    }
};
}
int main(int argc, char** argv) {
    try {
        if (argc != 2) throw std::invalid_argument("audio_cpu_ipl_runner OUTPUT");
        std::ofstream output(argv[1]);
        if (!output) throw std::runtime_error("cannot open cold transport output");
        ColdBus bus(output);
        const auto clocks = unirally::native_audio_cpu_boot_prefix({}, &bus);
        if (!output) throw std::runtime_error("cold transport write failed");
        std::cout << "computed_first_cpu_audio_write=" << clocks.back() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
