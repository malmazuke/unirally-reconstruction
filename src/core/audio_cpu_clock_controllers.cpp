#include "audio_cpu_clock.hpp"
#include <stdexcept>

namespace unirally {
std::uint16_t AudioCpuWorkObserver::controller_input(std::uint64_t, unsigned) {
    return 0;
}
// R-0075; pinned CPU::joypadEdge. Start on a 256-clock edge in blank's
// 130..384-clock window; sixteen serial bits take 33 subsequent 128-clock edges.
void AudioCpuWorkClock::poll_controllers() {
    const auto horizontal = ticks_ % 1364;
    if ((ticks_ / 1364) % 312 == 225 && (ticks_ & 255) == 0 && horizontal >= 130
        && horizontal <= 384) {
        auto_joypad_counter_ = 0;
    } else {
        if (auto_joypad_counter_ >= 33) return;
        ++auto_joypad_counter_;
    }
    if (auto_joypad_counter_ == 0 && auto_joypad_enabled_) {
        controller_words_ = {};
        for (unsigned port = 0; port < 2; ++port)
            latched_controllers_[port] = observer_ ? observer_->controller_input(ticks_, port) : 0;
    }
    if (auto_joypad_counter_ != 1 && !auto_joypad_enabled_) {
        auto_joypad_counter_ = 33;
        return;
    }
    if (auto_joypad_counter_ >= 3 && (auto_joypad_counter_ & 1)) {
        const auto bit = 16U - auto_joypad_counter_ / 2;
        for (unsigned port = 0; port < 2; ++port)
            controller_words_[port] =
                static_cast<std::uint16_t>((unsigned(controller_words_[port]) << 1)
                                           | ((latched_controllers_[port] >> bit) & 1));
    }
}
// $80:D1EF; $80:D1F4; R-0075. Word reads of auto-poll result registers
// sample their two bytes separately, six master clocks apart.
std::uint16_t AudioCpuWorkClock::read_controller(unsigned port) {
    if (port >= 2) throw std::invalid_argument("controller port is outside native domain");
    begin_instruction();
    rom_reads(3);
    std::uint16_t value = 0;
    for (unsigned byte = 0; byte < 2; ++byte) {
        if (byte == 1) last_cycle();
        begin_bus(6);
        step(2);
        irq_lock_ = false;
        value |= static_cast<std::uint16_t>(controller_words_[port] & (255U << (8 * byte)));
        step(4);
    }
    return value;
}
} // namespace unirally
