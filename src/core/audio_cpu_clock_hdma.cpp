#include "audio_cpu_clock.hpp"
#include <algorithm>
#include <stdexcept>

namespace unirally {
// R-0075: native-lifecycle-static bank-80 E2D3-E37A; pinned CPU::step/dmaEdge
// and Channel::hdmaReload/Transfer/Advance. Work-RAM integrity capture freezes
// the cleared tail. No image rendering or original CPU execution occurs here.
void AudioCpuWorkClock::poll_hdma() {
    auto& h = reveal_hdma_;
    if (!h.setup_triggered && ticks_ % 1364 >= h.setup_position) {
        h.setup_triggered = true;
        for (auto& channel : h.channels) {
            channel.completed = false;
            channel.transfer = false;
        }
        if (h.enabled) h.pending = 1;
    }
    if (!h.run_triggered && ticks_ % 1364 >= 1104) {
        h.run_triggered = true;
        if (h.enabled && (!h.channels[0].completed || !h.channels[1].completed)) h.pending = 2;
    }
}
std::uint8_t AudioCpuWorkClock::read_reveal_byte(unsigned offset, unsigned& bus_clocks) {
    if (offset >= reveal_hdma_.ram.size())
        throw std::logic_error("reveal HDMA read outside recovered work RAM");
    step(4);
    const auto value = reveal_hdma_.ram[offset];
    step(4);
    bus_clocks += 8;
    return value;
}
void AudioCpuWorkClock::reload_reveal_channel(unsigned index, unsigned& bus_clocks) {
    auto& channel = reveal_hdma_.channels[index];
    const auto value = read_reveal_byte(channel.cursor, bus_clocks);
    if ((channel.line_counter & 127) == 0) {
        channel.line_counter = value;
        ++channel.cursor;
        channel.completed = value == 0;
        channel.transfer = !channel.completed;
    }
}
unsigned AudioCpuWorkClock::run_reveal_hdma(bool setup) {
    unsigned bus_clocks = 8;
    step(8);
    if (setup) {
        for (unsigned i = 0; i < 2; ++i) {
            auto& channel = reveal_hdma_.channels[i];
            channel.cursor = i == 0 ? 0 : 72;
            channel.line_counter = 0;
            channel.transfer = true;
            reload_reveal_channel(i, bus_clocks);
        }
    } else {
        for (unsigned i = 0; i < 2; ++i) {
            auto& channel = reveal_hdma_.channels[i];
            if (channel.completed || !channel.transfer) continue;
            for (unsigned byte = 0; byte < (i == 0 ? 2U : 1U); ++byte)
                read_reveal_byte(channel.cursor++, bus_clocks);
        }
        for (unsigned i = 0; i < 2; ++i) {
            auto& channel = reveal_hdma_.channels[i];
            if (channel.completed) continue;
            channel.line_counter = static_cast<std::uint8_t>(channel.line_counter - 1U);
            channel.transfer = (channel.line_counter & 128) != 0;
            reload_reveal_channel(i, bus_clocks);
        }
    }
    irq_lock_ = true;
    return bus_clocks;
}
void AudioCpuWorkClock::copy_reveal_tables(std::span<const std::uint8_t, 72> offsets,
                                           std::span<const std::uint8_t, 38> brightness) {
    if (reveal_hdma_.enabled) throw std::logic_error("cannot replace active reveal HDMA tables");
    std::copy(offsets.begin(), offsets.end(), reveal_hdma_.ram.begin());
    std::copy(brightness.begin(), brightness.end(), reveal_hdma_.ram.begin() + 72);
}
void AudioCpuWorkClock::increment_reveal_header(unsigned offset) {
    if (offset != 0 && offset != 3 && offset != 72 && offset != 74)
        throw std::invalid_argument("unidentified reveal header");
    reveal_hdma_.ram[offset] = static_cast<std::uint8_t>(reveal_hdma_.ram[offset] + 3U);
}
void AudioCpuWorkClock::set_reveal_hdma_enabled(bool enabled) {
    store_port();
    reveal_hdma_.enabled = enabled;
}
void AudioCpuWorkClock::clear_reveal_work_ram() {
    if (reveal_hdma_.enabled) throw std::logic_error("cannot clear active reveal work RAM");
    reveal_hdma_.ram.fill(0);
}
} // namespace unirally
