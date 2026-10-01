#include "audio_cpu_queue.hpp"
#include <stdexcept>

namespace unirally {
namespace {
using Clock = AudioCpuWorkClock;
void read_long_ram(Clock& c) {
    c.read_ram(1, true);
}
void exchange_accumulator_bytes(Clock& c) {
    c.exchange_accumulator_bytes();
}
void restore_queue_caller(Clock& c) {
    c.change_widths();
    c.restore_register(2);
    c.restore_register(2);
    c.restore_register();
}
void validate(const AudioCpuQueueState& state) {
    if (state.read_index > 15 || state.write_index > 15 || (state.expected_phase & 63))
        throw std::invalid_argument("invalid audio command queue state");
}
}
// $82:8000; R-0075. Static bank-82 8000-8034: preserve the word, drop on full, publish last.
bool native_audio_enqueue(Clock& c, AudioCpuQueueState& state, std::uint8_t command,
                          std::uint8_t parameter) {
    validate(state);
    if (command >= 64) throw std::invalid_argument("audio command exceeds phase header");
    c.save_register();
    c.change_widths();
    c.save_register(2);
    c.save_register(2);
    c.save_register(2);
    c.change_widths();
    read_long_ram(c);
    c.update_register();
    c.update_register();
    c.load_constant();
    read_long_ram(c);
    const auto next = static_cast<std::uint8_t>((state.write_index + 1U) & 15);
    const bool full = next == state.read_index;
    c.branch(full);
    if (full) {
        c.change_widths();
        c.restore_register(2);
        c.restore_register(2);
        c.restore_register(2);
        c.restore_register();
        c.return_far();
        return false;
    }
    c.restore_register();
    c.store_ram(1, true, true);
    c.restore_register();
    c.store_ram(1, true, true);
    state.parameters[state.write_index] = parameter;
    state.commands[state.write_index] = command;
    c.update_register();
    c.update_register();
    c.load_constant();
    c.store_ram(1, true);
    state.write_index = next;
    restore_queue_caller(c);
    c.return_far();
    return true;
}
// $82:8035; R-0075. Static bank-82 8035-807D. Ready/empty decisions use native latches and ring
// state. A word store publishes header before parameter, six master clocks apart.
bool native_audio_poll_queue(Clock& c, AudioCpuQueueState& state) {
    validate(state);
    c.save_register();
    c.change_widths();
    c.save_register(2);
    c.save_register(2);
    c.change_widths();
    const auto received = c.read_audio_ports(2);
    read_long_ram(c);
    const bool ready = received == state.expected_phase;
    c.branch(!ready);
    if (!ready) {
        restore_queue_caller(c);
        c.update_register();
        c.return_far();
        return false;
    }
    exchange_accumulator_bytes(c);
    read_long_ram(c);
    read_long_ram(c);
    const bool empty = state.read_index == state.write_index;
    c.branch(empty);
    if (empty) {
        restore_queue_caller(c);
        c.update_register();
        c.return_far();
        return false;
    }
    c.update_register();
    read_long_ram(c);
    c.save_register();
    exchange_accumulator_bytes(c);
    c.load_constant();
    c.store_ram(1, true);
    read_long_ram(c);
    c.save_register();
    c.update_register();
    c.update_register();
    c.load_constant();
    c.store_ram(1, true);
    c.change_widths();
    c.restore_register(2);
    state.expected_phase ^= 192;
    const auto word =
        static_cast<std::uint16_t>(unsigned(state.expected_phase) | state.commands[state.read_index]
                                   | (unsigned(state.parameters[state.read_index]) << 8));
    state.read_index = static_cast<std::uint8_t>((state.read_index + 1U) & 15);
    c.write_audio_word(2, word);
    restore_queue_caller(c);
    c.update_register();
    c.return_far();
    return true;
}
// $80:A119; R-0075. Initial title music/gain cue words.
void native_audio_bootstrap_queue(Clock& c, AudioCpuQueueState& state) {
    constexpr std::array<std::array<std::uint8_t, 2>, 3> commands{{{1, 2}, {8, 255}, {7, 127}}};
    for (const auto& pair : commands) {
        c.load_constant(2);
        c.call_far();
        native_audio_enqueue(c, state, pair[0], pair[1]);
    }
    c.change_widths();
    for (unsigned i = 0; i < 9; ++i) {
        c.load_constant();
        c.store_port();
    }
    for (unsigned i = 0; i < 4; ++i) c.store_port();
    c.return_local();
    c.call_local();
    c.change_widths();
    c.call_local();
    c.call_far();
    native_audio_poll_queue(c, state);
}
// $80:FAE3; R-0075. Static bank-80 FAE3-FAEF. Each loop polls the native command ring.
void native_audio_wait_vblank(Clock& c, AudioCpuQueueState& state) {
    for (;;) {
        const bool blank = c.read_vertical_blank();
        c.branch(blank);
        if (!blank) break;
        c.call_far();
        native_audio_poll_queue(c, state);
    }
    for (;;) {
        c.call_far();
        native_audio_poll_queue(c, state);
        const bool blank = c.read_vertical_blank();
        c.branch(!blank);
        if (blank) break;
    }
}
} // namespace unirally
