#include "audio_cpu_clock.hpp"
#include <stdexcept>

namespace unirally {
void AudioCpuWorkClock::step(unsigned clocks) {
    if (clocks < 2 || clocks > 12 || (clocks & 1))
        throw std::invalid_argument("CPU bus interval is not an even 2..12 clocks");
    for (unsigned i = 0; i < clocks; i += 2) {
        ticks_ += 2;
        const auto line = ticks_ / 1364;
        if (line != scanline_) {
            scanline_ = line;
            refreshed_ = false;
            if (observer_) observer_->scanline(ticks_, completed_step_ticks_);
        }
    }
    completed_step_ticks_ += clocks;
    const auto refresh_position = 538U - unsigned(scanline_ * 1364 % 8);
    if (!refreshed_ && ticks_ % 1364 >= refresh_position) {
        refreshed_ = true;
        for (unsigned i = 0; i < 5; ++i) {
            step(6);
            step(2);
        }
    }
}
void AudioCpuWorkClock::rom_reads(unsigned count) {
    for (unsigned i = 0; i < count; ++i) {
        step(fast_rom_ ? 2U : 4U);
        step(4);
    }
}
void AudioCpuWorkClock::ram_reads(unsigned count) {
    for (unsigned i = 0; i < count; ++i) {
        step(4);
        step(4);
    }
}
void AudioCpuWorkClock::ram_writes(unsigned count) {
    for (unsigned i = 0; i < count; ++i) step(8);
}
void AudioCpuWorkClock::idle(unsigned count) {
    for (unsigned i = 0; i < count; ++i) step(6);
}
void AudioCpuWorkClock::load_constant(unsigned bytes) {
    rom_reads(bytes + 1U);
}
void AudioCpuWorkClock::update_register() {
    rom_reads(1);
    idle();
}
void AudioCpuWorkClock::change_widths() {
    rom_reads(2);
    idle();
}
void AudioCpuWorkClock::save_register(unsigned bytes) {
    rom_reads(1);
    idle();
    ram_writes(bytes);
}
void AudioCpuWorkClock::restore_register(unsigned bytes) {
    rom_reads(1);
    idle(2);
    ram_reads(bytes);
}
void AudioCpuWorkClock::call_local() {
    rom_reads(3);
    idle();
    ram_writes(2);
}
void AudioCpuWorkClock::call_far() {
    rom_reads(3);
    ram_writes();
    idle();
    rom_reads(1);
    ram_writes(2);
}
void AudioCpuWorkClock::return_local() {
    rom_reads(1);
    idle(2);
    ram_reads(2);
    idle();
}
void AudioCpuWorkClock::return_far() {
    rom_reads(1);
    idle(2);
    ram_reads(3);
}
void AudioCpuWorkClock::store_ram(unsigned bytes, bool long_address, bool indexed) {
    rom_reads(long_address ? 4U : 3U);
    if (indexed && !long_address) idle();
    ram_writes(bytes);
}
void AudioCpuWorkClock::store_direct(unsigned bytes) {
    rom_reads(2);
    ram_writes(bytes);
}
void AudioCpuWorkClock::read_direct(unsigned bytes) {
    rom_reads(2);
    ram_reads(bytes);
}
void AudioCpuWorkClock::store_port(unsigned bytes, bool long_address) {
    rom_reads(long_address ? 4U : 3U);
    for (unsigned i = 0; i < bytes; ++i) step(6);
}
void AudioCpuWorkClock::branch(bool taken) {
    rom_reads(2);
    if (taken) idle();
}
void AudioCpuWorkClock::write_audio_port(std::uint8_t port, std::uint8_t value) {
    store_port();
    if (observer_) observer_->write_audio_port(ticks_, port, value);
}
} // namespace unirally
