#include "audio_timers.hpp"
#include <stdexcept>

namespace unirally {
namespace {
constexpr std::array<unsigned, 3> divider_periods{128, 128, 16};
std::uint8_t byte(unsigned value) { return static_cast<std::uint8_t>(value); }
}
// Pinned bsnes sfc/smp/timing.cpp and io.cpp, commit 7d5aa1e. R-0075.
// The target counter sees only a falling edge, including a gate transition.
void AudioTimers::synchronize_line(AudioTimerState& timer) {
    const bool level = timer.divider_level && state_.gate_enabled && !state_.gate_disabled;
    const bool falling = timer.line && !level;
    timer.line = level;
    if (!falling || !timer.enabled) return;
    timer.target_counter = byte(timer.target_counter + 1U);
    if (timer.target_counter != timer.target) return;
    timer.target_counter = 0;
    timer.output = byte((timer.output + 1U) & 15);
}
void AudioTimers::advance_to(std::uint64_t ticks) {
    if (ticks < state_.ticks) throw std::invalid_argument("audio timer clock moved backwards");
    const auto elapsed = ticks - state_.ticks;
    for (std::size_t index = 0; index < state_.timers.size(); ++index) {
        auto& timer = state_.timers[index];
        const auto period = divider_periods[index];
        // Split division avoids overflow when elapsed approaches UINT64_MAX.
        auto toggles = elapsed / period;
        const auto remainder = elapsed % period + timer.divider;
        toggles += remainder / period;
        timer.divider = byte(static_cast<unsigned>(remainder % period));
        while (toggles) {
            --toggles;
            timer.divider_level = !timer.divider_level;
            synchronize_line(timer);
        }
    }
    state_.ticks = ticks;
}
void AudioTimers::write_target(std::uint8_t index, std::uint8_t target) {
    state_.timers.at(index).target = target;
}
void AudioTimers::write_control(std::uint8_t control) {
    for (unsigned index = 0; index < state_.timers.size(); ++index) {
        auto& timer = state_.timers[index];
        const bool enabled = (control & (1U << index)) != 0;
        const bool rising = !timer.enabled && enabled;
        timer.enabled = enabled;
        // Preserve the reference implementation's inverted timer-2 condition.
        // It resets stage 2/3 on every CONTROL write except a rising enable.
        if (index == 2 ? !rising : rising) timer.target_counter = timer.output = 0;
    }
}
void AudioTimers::set_gate(bool enabled, bool disabled) {
    state_.gate_enabled = enabled; state_.gate_disabled = disabled;
    for (auto& timer : state_.timers) synchronize_line(timer);
}
std::uint8_t AudioTimers::read_output(std::uint8_t index) {
    auto& timer = state_.timers.at(index);
    const auto value = timer.output;
    timer.output = 0;
    return value;
}
void AudioTimers::restore(const AudioTimersState& state) {
    for (std::size_t index = 0; index < state.timers.size(); ++index) {
        const auto& timer = state.timers[index];
        const bool line = timer.divider_level && state.gate_enabled && !state.gate_disabled;
        if (timer.divider >= divider_periods[index] || timer.output > 15 || timer.line != line)
            throw std::invalid_argument("invalid audio timer snapshot");
    }
    state_ = state;
}
}  // namespace unirally
