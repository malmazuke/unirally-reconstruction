#pragma once
#include <array>
#include <cstdint>

namespace unirally {
struct AudioTimerState {
    std::uint8_t divider = 0, target_counter = 0, output = 0, target = 0;
    bool divider_level = false, line = false, enabled = false;
    bool operator==(const AudioTimerState&) const = default;
};
struct AudioTimersState {
    std::array<AudioTimerState, 3> timers{};
    std::uint64_t ticks = 0;
    bool gate_enabled = true, gate_disabled = false;
    bool operator==(const AudioTimersState&) const = default;
};

// Timer hardware only, independent of any CPU instruction interpreter. SMP
// ticks use the pinned default wait-state clock. Outputs wrap at four bits;
// target zero means 256 falling divider edges, not an immediate pulse.
class AudioTimers {
public:
    void advance_to(std::uint64_t ticks);
    void write_target(std::uint8_t timer, std::uint8_t target);
    void write_control(std::uint8_t control);
    void set_gate(bool enabled, bool disabled);
    std::uint8_t read_output(std::uint8_t timer);
    const AudioTimersState& state() const { return state_; }
    void restore(const AudioTimersState& state);
private:
    AudioTimersState state_{};
    void synchronize_line(AudioTimerState& timer);
};
}  // namespace unirally
