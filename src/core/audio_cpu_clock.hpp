#pragma once
#include <cstdint>

namespace unirally {
class AudioCpuWorkObserver {
public:
    virtual ~AudioCpuWorkObserver() = default;
    // CPU::scanline synchronizes SMP before the enclosing bus step subtracts
    // its clocks from the CPU/SMP balance. Keep that completed-step clock.
    virtual void scanline(std::uint64_t master_ticks, std::uint64_t completed_step_ticks) = 0;
    virtual std::uint8_t read_audio_port(std::uint64_t master_ticks, std::uint8_t port);
    virtual void write_audio_port(std::uint64_t master_ticks, std::uint8_t port,
                                  std::uint8_t value) = 0;
};
// Semantic CPU work, in master clocks, for the pinned PAL/version-2 bus.
// ROM reads account for bus work only: this class reads no ROM/opcode bytes.
// The recovered cold domain has no enabled DMA, HDMA or NMI. R-0075.
class AudioCpuWorkClock {
public:
    explicit AudioCpuWorkClock(AudioCpuWorkObserver* observer = nullptr) : observer_(observer) {}
    std::uint64_t ticks() const { return ticks_; }
    void set_fast_rom(bool enabled) { fast_rom_ = enabled; }
    void step(unsigned clocks);
    void rom_reads(unsigned count);
    void ram_reads(unsigned count = 1);
    void ram_writes(unsigned count = 1);
    void idle(unsigned count = 1);
    void load_constant(unsigned bytes = 1);
    void update_register();
    void change_widths();
    void save_register(unsigned bytes = 1);
    void restore_register(unsigned bytes = 1);
    void call_local();
    void call_far();
    void return_local();
    void return_far();
    void store_ram(unsigned bytes = 1, bool long_address = false, bool indexed = false);
    void store_direct(unsigned bytes = 1);
    void read_direct(unsigned bytes = 1);
    void store_port(unsigned bytes = 1, bool long_address = false);
    void branch(bool taken);
    void write_audio_port(std::uint8_t port, std::uint8_t value);
    void write_audio_word(std::uint8_t first_port, std::uint16_t value);
    std::uint16_t read_audio_ports(std::uint8_t first_port, unsigned bytes = 1);

private:
    std::uint64_t ticks_ = 0, completed_step_ticks_ = 0, scanline_ = 0;
    bool refreshed_ = false, fast_rom_ = false;
    AudioCpuWorkObserver* observer_;
};
} // namespace unirally
