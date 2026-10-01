#include "audio_cpu_scene.hpp"
#include <stdexcept>

namespace unirally {
namespace {
using Clock = AudioCpuWorkClock;
void dispatch_control(Clock& c) {
    c.change_widths();
    c.load_constant(2);
    c.load_constant(2);
    c.update_register();
    c.update_register();
    c.read_rom(2, false, true);
    c.store_direct(2);
    c.jump_indirect();
}
// $83:A9FB-AA20; FE sets column and row from two identified data bytes.
void set_position(Clock& c) {
    c.change_widths();
    c.load_constant();
    c.exchange_accumulator_bytes();
    c.read_rom(1, false, true);
    c.update_register();
    c.store_direct(2);
    c.read_rom(1, false, true);
    c.update_register();
    c.store_direct(2);
    c.change_widths();
    c.read_direct(2);
    for (unsigned i = 0; i < 5; ++i) c.update_register();
    c.update_register();
    c.read_direct(2);
    c.store_direct(2);
    c.change_widths();
    c.update_register();
    c.update_register();
    c.branch_long();
}
void large_glyph(Clock& c) {
    c.change_widths();
    c.save_register(2);
    c.update_register();
    c.read_direct(2);
    c.update_register();
    c.update_register();
    c.load_constant(2);
    c.update_register();
    c.update_register();
    c.update_register();
    c.read_direct(2);
    c.load_constant(2);
    c.store_ram(2, false, true);
    c.update_register();
    c.store_ram(2, false, true);
    c.update_register();
    c.load_constant(2);
    c.store_ram(2, false, true);
    c.update_register();
    c.store_ram(2, false, true);
    c.modify_direct_word();
    c.modify_direct_word();
    c.change_widths();
    c.restore_register(2);
    c.branch_long();
}
// $83:AAC5-AB23; this credits stream uses the ordinary small hyphen.
void small_glyph(Clock& c) {
    c.save_register(2);
    c.change_widths();
    c.update_register();
    c.read_direct(2);
    c.update_register();
    c.update_register();
    c.load_constant(2);
    c.update_register();
    c.update_register();
    c.load_constant(2);
    c.load_constant(2);
    c.branch(false);
    c.load_constant(2);
    c.branch(false);
    c.update_register();
    c.load_constant(2);
    c.read_direct(2);
    c.load_constant(2);
    c.store_ram(2, false, true);
    c.update_register();
    c.load_constant(2);
    c.store_ram(2, false, true);
    c.branch(true);
    c.modify_direct_word();
    c.change_widths();
    c.restore_register(2);
    c.branch_long();
}
void glyph(Clock& c, std::uint8_t byte, std::uint8_t character) {
    c.change_widths();
    c.load_constant();
    c.branch(byte != 0x60);
    if (byte == 0x60) {
        c.load_constant();
        c.branch(true);
    } else {
        c.load_constant();
        c.branch(false);
    }
    c.change_widths();
    c.load_constant(2);
    c.save_register(2);
    c.update_register();
    c.change_widths();
    c.read_rom(1, true, true);
    c.restore_register(2);
    c.load_constant();
    const bool large = (character & 128) == 0;
    c.branch(large);
    if (large)
        large_glyph(c);
    else {
        c.branch_long();
        small_glyph(c);
    }
}
} // namespace
// $83:A93A-AB24; R-0075. Only the identified credits stream's FE, FF and
// ordinary large/small glyphs are in this recovered text work domain.
void native_audio_print_credits(Clock& c, std::span<const std::uint8_t, 17> text,
                                std::span<const std::uint8_t, 256> characters) {
    if (text[0] != 254 || text[1] != 5 || text[2] != 2 || text.back() != 255)
        throw std::invalid_argument("unidentified credits text controls");
    c.change_widths();
    c.save_register(2);
    c.save_register();
    c.save_register();
    c.load_constant();
    c.save_register();
    c.restore_register();
    for (unsigned at = 0; at < text.size(); ++at) {
        c.change_widths();
        c.read_rom(1, false, true);
        c.update_register();
        c.load_constant();
        const auto byte = text[at];
        const bool control = byte >= 238;
        c.branch(!control);
        if (!control)
            glyph(c, byte, characters[byte]);
        else {
            dispatch_control(c);
            if (byte == 254) {
                set_position(c);
                at += 2;
            } else if (byte == 255) {
                c.change_widths();
                c.restore_register();
                c.restore_register();
                c.restore_register(2);
                c.return_local();
                return;
            } else
                throw std::invalid_argument("credits text control outside native domain");
        }
    }
    throw std::invalid_argument("unterminated native credits text");
}
} // namespace unirally
