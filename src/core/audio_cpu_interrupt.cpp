#include "audio_cpu_interrupt.hpp"
#include <stdexcept>

namespace unirally {
namespace {
using Clock = AudioCpuWorkClock;
void modify_direct_byte(Clock& c) {
    c.rom_reads(2);
    c.ram_reads();
    c.idle();
    c.ram_writes();
}
// $80:8587; R-0075. The native-mode vector points to the bank00 ROM mirror.
void save_interrupt_caller(Clock& c) {
    c.rom_reads(1);
    c.idle();
    c.ram_writes(4);
    c.rom_reads(2);
    c.save_register();
    c.save_register(2);
    c.change_widths();
    for (unsigned i = 0; i < 3; ++i) c.save_register(2);
    c.change_widths();
    c.load_constant();
    c.save_register();
    c.restore_register();
    c.rom_reads(3);
    c.ram_writes(2);
    c.restore_register(2);
    c.rom_reads(3);
    c.ram_reads(3);
}
// $80:F622; R-0075. Arithmetic wraps as bytes before sign/threshold tests.
void update_title_scroll(Clock& c, AudioCpuInterruptWorkState& state) {
    c.change_widths();
    c.rom_reads(4);
    c.ram_reads();
    c.load_constant();
    const bool increasing = (state.cartridge_flags & 2) != 0;
    c.branch(increasing);
    c.read_direct();
    c.update_register();
    c.update_register();
    const auto next = static_cast<std::uint8_t>(state.scroll + (increasing ? 2 : -2));
    const bool skip = increasing ? next >= 83 : (next & 128) != 0;
    if (increasing) c.load_constant();
    c.branch(skip);
    if (!increasing && !skip) c.branch(true);
    if (!skip) {
        state.scroll = next;
        c.store_direct();
        c.store_port();
        c.store_port();
    }
    c.rom_reads(4);
}
void write_palette_colour(Clock& c) {
    c.rom_reads(3);
    c.idle();
    c.rom_reads(2);
    c.change_widths();
    c.store_port();
    c.rom_reads(1);
    c.idle(2);
    c.store_port();
}
// $80:FA60; R-0075. Four colours rotate once per seven interrupt calls.
void update_palette_cycle(Clock& c, AudioCpuInterruptWorkState& state) {
    c.change_widths();
    modify_direct_byte(c);
    state.palette_delay = static_cast<std::uint8_t>(state.palette_delay - 1U);
    const bool waiting = (state.palette_delay & 128) == 0;
    c.branch(waiting);
    if (waiting) {
        c.rom_reads(4);
        return;
    }
    c.load_constant();
    c.store_direct();
    state.palette_delay = 6;
    modify_direct_byte(c);
    state.palette_index = static_cast<std::uint8_t>(state.palette_index - 1U);
    const bool wrapped = (state.palette_index & 128) != 0;
    c.branch(!wrapped);
    if (wrapped) {
        c.load_constant();
        c.store_direct();
        state.palette_index = 3;
    }
    c.load_constant();
    c.store_port();
    c.change_widths();
    c.read_direct(2);
    c.load_constant(2);
    c.update_register();
    c.update_register();
    c.change_widths();
    write_palette_colour(c);
    for (unsigned i = 0; i < 3; ++i) {
        if (i == 2) c.change_widths();
        c.load_constant();
        c.store_port();
        c.change_widths();
        write_palette_colour(c);
    }
    c.rom_reads(4);
}
// $80:B0F0; R-0075. Restore words and return through the hardware frame.
void restore_interrupt_caller(Clock& c) {
    c.change_widths();
    for (unsigned i = 0; i < 4; ++i) c.restore_register(2);
    c.restore_register();
    c.change_widths();
    c.rom_reads(1);
    c.idle(2);
    c.ram_reads(4);
}
}
void native_audio_title_interrupt(Clock& c, AudioCpuInterruptWorkState& state) {
    if (state.palette_delay > 6 || state.palette_index > 3)
        throw std::invalid_argument("invalid native palette cycle");
    save_interrupt_caller(c);
    update_title_scroll(c, state);
    update_palette_cycle(c, state);
    restore_interrupt_caller(c);
}
void native_audio_first_title_interrupt(Clock& c) {
    c.load_constant();
    c.set_nmi_enabled(true);
    c.call_local();
    c.begin_instruction();
}
} // namespace unirally
