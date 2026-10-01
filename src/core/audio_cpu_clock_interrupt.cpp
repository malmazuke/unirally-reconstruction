#include "audio_cpu_clock.hpp"
#include <stdexcept>

namespace unirally {
// R-0075; pinned CPU::nmiPoll, PPUcounter::vcounter(offset). PAL noninterlace
// polls every four master clocks using the raster two clocks earlier.
void AudioCpuWorkClock::poll_nmi() {
    if (nmi_hold_ && nmi_enabled_) nmi_transition_ = true;
    nmi_hold_ = false;
    const bool blank = ((ticks_ - 2) / 1364) % 312 >= 225;
    if (blank != nmi_valid_) {
        nmi_valid_ = blank;
        nmi_line_ = blank;
        if (blank) nmi_hold_ = true;
    }
}
void AudioCpuWorkClock::last_cycle() {
    if (!irq_lock_ && nmi_transition_) {
        nmi_transition_ = false;
        nmi_pending_ = true;
    }
}
void AudioCpuWorkClock::begin_instruction() {
    if (!nmi_pending_ || in_interrupt_) return;
    if (!observer_) throw std::logic_error("native NMI requires an observer");
    nmi_pending_ = false;
    in_interrupt_ = true;
    try {
        observer_->nonmaskable_interrupt(*this);
    } catch (...) {
        in_interrupt_ = false;
        throw;
    }
    in_interrupt_ = false;
}
// R-0075; pinned CPU::nmitimenUpdate sets IRQ lock after the MMIO write.
void AudioCpuWorkClock::set_nmi_enabled(bool enabled) {
    store_port();
    if (enabled && !nmi_enabled_ && nmi_line_) nmi_transition_ = true;
    nmi_enabled_ = enabled;
    auto_joypad_enabled_ = enabled;
    irq_lock_ = true;
}
void AudioCpuWorkClock::read_ram(unsigned bytes, bool long_address, bool indexed) {
    begin_instruction();
    rom_reads(long_address ? 4U : 3U);
    if (indexed && !long_address) idle();
    if (bytes > 1) ram_reads(bytes - 1);
    last_cycle();
    ram_reads();
}
void AudioCpuWorkClock::read_rom(unsigned bytes, bool long_address, bool indexed) {
    begin_instruction();
    rom_reads(long_address ? 4U : 3U);
    if (indexed && !long_address) idle();
    if (bytes > 1) rom_reads(bytes - 1);
    last_cycle();
    rom_reads(1);
}
void AudioCpuWorkClock::read_stack(unsigned bytes) {
    begin_instruction();
    rom_reads(2);
    idle();
    if (bytes > 1) ram_reads(bytes - 1);
    last_cycle();
    ram_reads();
}
void AudioCpuWorkClock::modify_direct_byte() {
    begin_instruction();
    rom_reads(2);
    ram_reads();
    idle();
    last_cycle();
    ram_writes();
}
void AudioCpuWorkClock::exchange_accumulator_bytes() {
    begin_instruction();
    rom_reads(1);
    idle();
    last_cycle();
    idle();
}
void AudioCpuWorkClock::jump_far() {
    begin_instruction();
    rom_reads(3);
    last_cycle();
    rom_reads(1);
}
} // namespace unirally
