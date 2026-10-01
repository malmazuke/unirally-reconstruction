#include "audio_cpu_boot.hpp"
#include "audio_cpu_upload.hpp"
#include "audio_cpu_queue.hpp"
#include "audio_ipl.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <algorithm>
#include <stdexcept>

namespace {
struct CpuYield {};
class Bus final : public unirally::AudioDriverBus, public unirally::AudioCpuWorkObserver {
public:
    Bus(std::ostream& output, const unirally::TitleMenuAudioData& score,
        const unirally::AudioPitchData& pitch, bool ready_mode)
        : output_(output), ipl_(*this), score_(&score), pitch_(&pitch), ready_mode_(ready_mode) {}
    std::uint8_t read_port(std::uint64_t ticks, std::uint8_t port) override {
        if (ticks * cpu_frequency >= cpu_completed_ * smp_frequency) throw CpuYield{};
        const auto value = incoming_.at(port);
        emit('R', ticks, port, value); return value;
    }
    void write_port(std::uint64_t ticks, std::uint8_t port, std::uint8_t value) override {
        if (ticks * cpu_frequency >= cpu_completed_ * smp_frequency) throw CpuYield{};
        outgoing_.at(port) = value;
        emit('P', ticks, port, value);
    }
    void write_ram(std::uint64_t ticks, std::uint16_t address, std::uint8_t value) override {
        emit('N', ticks, address, value);
    }
    void write_dsp(std::uint64_t ticks, std::uint8_t reg, std::uint8_t value) override {
        emit('D', ticks, reg, value);
    }
    void clear_ports(std::uint64_t ticks, std::uint8_t port) override {
        if (ticks * cpu_frequency >= cpu_completed_ * smp_frequency) throw CpuYield{};
        incoming_.at(port) = incoming_.at(port + 1U) = 0;
        emit('Z', ticks, port, 0);
    }
    unsigned clock_sync_step() const override { return 2; }
    void advance_clock(std::uint64_t ticks) override {
        constexpr std::uint64_t force_lead = 768ULL * 24 * 24000000;
        if (ticks * cpu_frequency > cpu_completed_ * smp_frequency + force_lead) throw CpuYield{};
    }
    void scanline(std::uint64_t ticks, std::uint64_t completed_ticks) override {
        cpu_master_ = ticks; cpu_completed_ = completed_ticks; synchronize();
    }
    std::uint8_t read_audio_port(std::uint64_t ticks, std::uint8_t port) override {
        cpu_master_ = cpu_completed_ = ticks; synchronize();
        const auto value = outgoing_.at(port);
        emit('Q', smp_ticks(), port, value); return value;
    }
    void write_audio_port(std::uint64_t ticks, std::uint8_t port, std::uint8_t value) override {
        cpu_master_ = cpu_completed_ = ticks; synchronize();
        incoming_.at(port) = value;
        emit('C', smp_ticks(), port, value);
    }
private:
    static constexpr std::uint64_t cpu_frequency = 21281370, smp_frequency = 2050560;
    std::ostream& output_; unirally::AudioIplHandshake ipl_;
    const unirally::TitleMenuAudioData* score_; const unirally::AudioPitchData* pitch_;
    bool ready_mode_;
    std::unique_ptr<unirally::TitleMenuAudioDriver> driver_;
    std::array<std::uint8_t, 4> incoming_{}, outgoing_{};
    std::uint64_t smp_ticks() const { return driver_ ? driver_->ticks() : ipl_.state().ticks; }
    std::uint64_t cpu_master_ = 0, cpu_completed_ = 0;
    void emit(char kind, std::uint64_t ticks, std::uint16_t address, std::uint8_t value) {
        output_ << kind << ' ' << ticks << ' ' << cpu_master_ << ' ' << address << ' ';
        output_ << unsigned(value) << '\n';
    }
    void synchronize() {
        if (smp_ticks() * cpu_frequency >= cpu_completed_ * smp_frequency) return;
        try {
            if (!driver_) {
                ipl_.run_until(std::numeric_limits<std::uint64_t>::max());
                if (ipl_.driver_ready()) {
                    if (!ready_mode_) throw std::logic_error("upload crossed its domain");
                    driver_ = std::make_unique<unirally::TitleMenuAudioDriver>(
                        *score_, *pitch_, *this, unirally::AudioTimersState{}, ipl_.state().ticks,
                        true, true);
                }
            }
            if (driver_) driver_->run_until(std::numeric_limits<std::uint64_t>::max());
        } catch (const CpuYield&) {}
    }
};
std::vector<std::uint8_t> read(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("cannot read identified upload data");
    return {std::istreambuf_iterator<char>(file), {}};
}
}
int main(int argc, char** argv) {
    try {
        if (argc != 3 && argc != 4)
            throw std::invalid_argument("audio_cpu_upload_runner DATA_DIRECTORY OUTPUT [ready|samples|queue|vblank]");
        const bool vblank = argc == 4 && std::string(argv[3]) == "vblank";
        const bool queue = argc == 4 && (std::string(argv[3]) == "queue" || vblank);
        const bool samples = argc == 4 && (std::string(argv[3]) == "samples" || queue);
        const bool ready = argc == 4 && (std::string(argv[3]) == "ready" || samples);
        if (argc == 4 && !ready) throw std::invalid_argument("unknown upload mode");
        const std::string root = argv[1]; unirally::AudioCpuUploadData data;
        std::ifstream lengths(root + "/cpu-resource-lengths.txt");
        for (auto& length : data.resource_lengths) {
            unsigned value; if (!(lengths >> value) || value > 65535)
                throw std::runtime_error("invalid resource length");
            length = static_cast<std::uint16_t>(value);
        }
        data.menu_transfer = read(root + "/cpu-menu-upload.bin");
        data.title_transfer = read(root + "/cpu-title-upload.bin");
        if (samples) {
            const auto slots = read(root + "/cpu-sample-slots.bin");
            if (slots.size() != 64) throw std::invalid_argument("invalid sample slots");
            std::copy(slots.begin(), slots.end(), data.sample_slots.begin());
            for (const auto sample : slots) {
                if (sample == 255) continue;
                if (sample >= data.sample_resources.size()) throw std::invalid_argument("sample index");
                const auto name = std::to_string(sample);
                data.sample_resources[sample] = read(root + "/cpu-sample-resource-"
                    + (sample < 10 ? "0" : "") + name + ".bin");
            }
        }
        std::ofstream output(argv[2]);
        if (!output) throw std::runtime_error("cannot write native upload events");
        const unirally::TitleMenuAudioData score{read(root + "/menu-tables.bin"),
                                                 read(root + "/title-score.bin")};
        unirally::AudioPitchData pitch;
        const auto notes = read(root + "/pitch-table.bin");
        const auto fraction = read(root + "/sample-fractions.bin");
        const auto transpose = read(root + "/sample-transpose.bin");
        if (notes.size() != 194 || fraction.size() != 64 || transpose.size() != 64)
            throw std::invalid_argument("invalid pitch data");
        for (std::size_t i = 0; i < pitch.notes.size(); ++i)
            pitch.notes[i] = static_cast<std::uint16_t>(notes[2*i] | unsigned(notes[2*i+1]) << 8);
        std::copy(fraction.begin(), fraction.end(), pitch.sample_fraction.begin());
        std::copy(transpose.begin(), transpose.end(), pitch.sample_transpose.begin());
        Bus bus(output, score, pitch, ready); unirally::AudioCpuWorkClock clock(&bus);
        unirally::native_audio_cpu_boot_prefix(clock);
        unirally::native_audio_cpu_uploads(clock, data);
        if (ready) unirally::native_audio_cpu_finish_driver_entry(clock);
        if (samples) unirally::native_audio_cpu_upload_samples(clock, data);
        if (queue) {
            unirally::AudioCpuQueueState state;
            unirally::native_audio_bootstrap_queue(clock, state);
            if (vblank) unirally::native_audio_wait_vblank(clock, state);
            std::cout << "queue_read=" << unsigned(state.read_index)
                      << " queue_write=" << unsigned(state.write_index)
                      << " expected_phase=" << unsigned(state.expected_phase) << '\n';
        }
        std::cout << "computed_final_upload_cpu_clock=" << clock.ticks() << '\n';
        if (!output) throw std::runtime_error("native upload events failed");
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
