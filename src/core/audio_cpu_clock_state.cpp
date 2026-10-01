#include "audio_cpu_clock.hpp"
#include <limits>
#include <stdexcept>

namespace unirally {
AudioCpuClockState AudioCpuWorkClock::snapshot() const {
    if (in_interrupt_ || dma_active_)
        throw std::logic_error("CPU snapshot is inside native bus work");
    AudioCpuClockState state;
    state.ticks = ticks_;
    state.completed_step_ticks = completed_step_ticks_;
    state.scanline = scanline_;
    state.refreshed = refreshed_;
    state.fast_rom = fast_rom_;
    state.pending_dma_bytes = pending_dma_bytes_;
    state.dma_active = dma_active_;
    state.nmi_enabled = nmi_enabled_;
    state.nmi_valid = nmi_valid_;
    state.nmi_line = nmi_line_;
    state.nmi_hold = nmi_hold_;
    state.nmi_transition = nmi_transition_;
    state.nmi_pending = nmi_pending_;
    state.irq_lock = irq_lock_;
    state.in_interrupt = in_interrupt_;
    state.auto_joypad_enabled = auto_joypad_enabled_;
    state.auto_joypad_counter = auto_joypad_counter_;
    state.latched_controllers = latched_controllers_;
    state.controller_words = controller_words_;
    return state;
}
void AudioCpuWorkClock::restore(const AudioCpuClockState& state) {
    if ((state.ticks & 1) || state.ticks != state.completed_step_ticks
        || state.scanline != state.ticks / 1364 || state.pending_dma_bytes > 65536
        || state.dma_active || state.in_interrupt || state.auto_joypad_counter > 33
        || state.ticks > std::numeric_limits<std::uint64_t>::max() / 2050560)
        throw std::invalid_argument("invalid native CPU clock continuation");
    ticks_ = state.ticks;
    completed_step_ticks_ = state.completed_step_ticks;
    scanline_ = state.scanline;
    refreshed_ = state.refreshed;
    fast_rom_ = state.fast_rom;
    pending_dma_bytes_ = state.pending_dma_bytes;
    dma_active_ = state.dma_active;
    nmi_enabled_ = state.nmi_enabled;
    nmi_valid_ = state.nmi_valid;
    nmi_line_ = state.nmi_line;
    nmi_hold_ = state.nmi_hold;
    nmi_transition_ = state.nmi_transition;
    nmi_pending_ = state.nmi_pending;
    irq_lock_ = state.irq_lock;
    in_interrupt_ = state.in_interrupt;
    auto_joypad_enabled_ = state.auto_joypad_enabled;
    auto_joypad_counter_ = state.auto_joypad_counter;
    latched_controllers_ = state.latched_controllers;
    controller_words_ = state.controller_words;
}
} // namespace unirally
