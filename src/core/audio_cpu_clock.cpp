#include "audio_cpu_clock.hpp"
#include <stdexcept>

namespace unirally {
std::uint8_t AudioCpuWorkObserver::read_audio_port(std::uint64_t, std::uint8_t) {
    throw std::logic_error("CPU observer has no audio receiver");
}

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
        begin_bus(fast_rom_ ? 6U : 8U);
        step(fast_rom_ ? 2U : 4U);
        step(4);
    }
}
void AudioCpuWorkClock::ram_reads(unsigned count) {
    for (unsigned i = 0; i < count; ++i) {
        begin_bus(8);
        step(4);
        step(4);
    }
}
void AudioCpuWorkClock::ram_writes(unsigned count) {
    for (unsigned i = 0; i < count; ++i) {
        begin_bus(8);
        step(8);
    }
}
void AudioCpuWorkClock::idle(unsigned count) {
    for (unsigned i = 0; i < count; ++i) {
        begin_bus(6);
        step(6);
    }
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
    for (unsigned i = 0; i < bytes; ++i) {
        begin_bus(6);
        step(6);
    }
}
void AudioCpuWorkClock::branch(bool taken) {
    rom_reads(2);
    if (taken) idle();
}
void AudioCpuWorkClock::write_audio_word(std::uint8_t first_port, std::uint16_t value) {
    if (first_port > 2) throw std::invalid_argument("audio word exceeds ports");
    rom_reads(3);
    for (unsigned i = 0; i < 2; ++i) {
        begin_bus(6);
        step(6);
        if (observer_)
            observer_->write_audio_port(ticks_, static_cast<std::uint8_t>(first_port + i),
                                        static_cast<std::uint8_t>(value >> (8 * i)));
    }
}
std::uint16_t AudioCpuWorkClock::read_audio_ports(std::uint8_t first_port, unsigned bytes) {
    if (!observer_ || bytes < 1 || bytes > 2 || unsigned(first_port) + bytes > 4)
        throw std::invalid_argument("invalid CPU audio read");
    rom_reads(3);
    std::uint16_t value = 0;
    for (unsigned i = 0; i < bytes; ++i) {
        begin_bus(6);
        step(2);
        value |= static_cast<std::uint16_t>(
            unsigned(observer_->read_audio_port(ticks_, static_cast<std::uint8_t>(first_port + i)))
            << (8 * i));
        step(4);
    }
    return value;
}
void AudioCpuWorkClock::write_audio_port(std::uint8_t port, std::uint8_t value) {
    store_port();
    if (observer_) observer_->write_audio_port(ticks_, port, value);
}
void AudioCpuWorkClock::request_dma(unsigned bytes) {
    if (bytes == 0 || bytes > 65536 || pending_dma_bytes_)
        throw std::invalid_argument("invalid single-channel DMA request");
    pending_dma_bytes_ = bytes;
    dma_active_ = false;
}
// Pinned CPU::dmaEdge/dmaRun/Channel::dmaRun. Enable waits one full CPU
// bus cycle; then alignment, global/channel setup, split byte reads and
// final alignment run before the next CPU bus interval.
void AudioCpuWorkClock::begin_bus(unsigned clocks) {
    if (!pending_dma_bytes_) return;
    if (!dma_active_) {
        dma_active_ = true;
        return;
    }
    const auto bytes = pending_dma_bytes_;
    pending_dma_bytes_ = 0;
    const auto alignment = 8U - unsigned(ticks_ & 7);
    step(alignment);
    step(8);
    step(8);
    for (unsigned i = 0; i < bytes; ++i) {
        step(4);
        step(4);
    }
    const auto dma_bus_clocks = alignment + 16U + bytes * 8U;
    step(clocks - dma_bus_clocks % clocks);
    dma_active_ = false;
}
bool AudioCpuWorkClock::read_vertical_blank() {
    rom_reads(3);
    begin_bus(6);
    step(2);
    // HVBJOY uses the raster directly, without NMI transition delay.
    const bool blank = (ticks_ / 1364) % 312 >= 225;
    step(4);
    return blank;
}
} // namespace unirally
