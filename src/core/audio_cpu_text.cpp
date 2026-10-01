#include "audio_cpu_text.hpp"
#include <stdexcept>

namespace unirally {
namespace {
using Clock = AudioCpuWorkClock;
std::uint8_t byte_at(std::span<const std::uint8_t> text, std::size_t at) {
    if (at >= text.size()) throw std::invalid_argument("menu text runs beyond its identified data");
    return text[at];
}
// $80:8C41; R-0075. Test the raw conversion-table small-glyph flag for centering.
bool small_glyph_work(Clock& c, std::uint8_t character,
                      std::span<const std::uint8_t, 256> characters) {
    c.call_local();
    c.save_register(2);
    c.change_widths();
    c.update_register();
    c.change_widths();
    c.read_rom(1, false, true);
    c.restore_register(2);
    c.load_constant();
    c.return_local();
    return (characters[character] & 128) != 0;
}
// $80:C4B8; R-0075. The 33-tile byte budget wraps on subtraction before division by2.
void center_text_work(Clock& c, AudioCpuTextWorkState& state, std::span<const std::uint8_t> text,
                      std::size_t& at, std::span<const std::uint8_t, 256> characters) {
    c.change_widths();
    c.load_constant();
    c.exchange_accumulator_bytes();
    c.read_rom(1, false, true);
    const auto row = byte_at(text, at);
    c.update_register();
    c.store_direct(2);
    c.load_constant(2);
    c.store_direct(2);
    c.update_register();
    std::uint8_t budget = 33;
    auto scan = at + 1;
    for (;;) {
        c.read_rom(1, false, true);
        const auto character = byte_at(text, scan++);
        c.update_register();
        c.load_constant();
        c.branch(character == 255);
        if (character == 255) break;
        c.load_constant();
        c.branch(character == 251);
        if (character == 251) break;
        c.load_constant();
        c.branch(character != 239);
        if (character == 239) {
            c.update_register();
            byte_at(text, scan++);
            c.branch(true);
            continue;
        }
        const bool small = small_glyph_work(c, character, characters);
        c.branch(small);
        if (!small) {
            c.modify_direct_byte();
            --budget;
        }
        c.modify_direct_byte();
        --budget;
        c.branch(true);
    }
    c.read_direct();
    c.update_register();
    c.store_direct();
    c.change_widths();
    c.read_direct(2);
    for (unsigned shift = 0; shift < 6; ++shift) c.update_register();
    c.read_direct(2);
    c.store_direct(2);
    state.cursor = static_cast<std::uint16_t>(unsigned(row) * 32 + (budget >> 1));
    c.change_widths();
    c.update_register();
    ++at;
    c.branch_long();
}
// $80:C41E; R-0075. Big glyphs write two tiles on each of two rows.
void big_glyph_work(Clock& c, AudioCpuTextWorkState& state) {
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
    state.cursor = static_cast<std::uint16_t>(state.cursor + 2);
    c.change_widths();
    c.restore_register(2);
    c.branch_long();
}
// $80:C56C; R-0075. Small glyphs use one tile per row, with two special conversion entries.
void small_glyph_print_work(Clock& c, AudioCpuTextWorkState& state, std::uint8_t entry) {
    c.branch_long();
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
    const auto index = entry & 127;
    c.load_constant(2);
    c.branch(index == 100);
    if (index != 100) {
        c.load_constant(2);
        c.branch(index == 101);
    }
    if (index == 100 || index == 101)
        c.load_constant(2);
    else
        c.update_register();
    c.load_constant(2);
    c.read_direct(2);
    c.load_constant(2);
    c.store_ram(2, false, true);
    if (index != 101) {
        c.update_register();
        c.load_constant(2);
    }
    c.store_ram(2, false, true);
    if (index != 101) c.branch(true);
    c.modify_direct_word();
    state.cursor = static_cast<std::uint16_t>(state.cursor + 1);
    c.change_widths();
    c.restore_register(2);
    c.branch_long();
}
// $80:C3FE; R-0075. Normalize the printed character, then select the recovered glyph width.
void print_character_work(Clock& c, AudioCpuTextWorkState& state, std::uint8_t character,
                          std::span<const std::uint8_t, 256> characters) {
    c.change_widths();
    c.load_constant();
    c.branch(character != 96);
    if (character == 96) {
        c.load_constant();
        c.branch(true);
        character = 95;
    } else {
        c.load_constant();
        c.branch(character >= 140);
        if (character >= 140) {
            c.load_constant();
            c.branch(true);
            character = 63;
        }
    }
    c.change_widths();
    c.load_constant(2);
    c.update_register();
    c.change_widths();
    c.read_rom(1, false, true);
    const auto entry = characters[character];
    c.load_constant();
    c.branch((entry & 128) == 0);
    if (entry & 128)
        small_glyph_print_work(c, state, entry);
    else
        big_glyph_work(c, state);
}
void control_dispatch_work(Clock& c) {
    c.change_widths();
    c.load_constant(2);
    c.load_constant(2);
    c.update_register();
    c.update_register();
    c.read_rom(2, false, true);
    c.store_direct(2);
    c.jump_indirect();
}
// $80:C557; R-0075. The byte attribute becomes a word shifted left10, with uint16 truncation.
void attribute_work(Clock& c, AudioCpuTextWorkState& state, std::uint8_t attribute) {
    c.change_widths();
    c.read_rom(1, false, true);
    c.change_widths();
    c.load_constant(2);
    c.exchange_accumulator_bytes();
    c.update_register();
    c.update_register();
    c.store_direct(2);
    state.attribute = static_cast<std::uint16_t>(unsigned(attribute) << 10);
    c.change_widths();
    c.update_register();
    c.branch_long();
}
// $80:C3BC; R-0075. Only the identified initial menu's FF/FB/FC/F9 controls are recovered here.
void print_menu_text_work(Clock& c, AudioCpuTextWorkState& state,
                          std::span<const std::uint8_t> text,
                          std::span<const std::uint8_t, 256> characters) {
    c.call_local();
    c.change_widths();
    c.save_register(2);
    c.save_register();
    std::size_t at = 0;
    for (;;) {
        c.change_widths();
        c.read_rom(1, false, true);
        const auto character = byte_at(text, at++);
        c.update_register();
        c.load_constant();
        c.branch(character < 238);
        if (character < 238) {
            print_character_work(c, state, character, characters);
            continue;
        }
        control_dispatch_work(c);
        if (character == 255) break;
        if (character == 251) continue;
        if (character == 252) {
            center_text_work(c, state, text, at, characters);
        } else if (character == 249) {
            attribute_work(c, state, byte_at(text, at++));
        } else {
            throw std::invalid_argument("menu text control outside recovered cold domain");
        }
    }
    c.change_widths();
    c.restore_register();
    c.restore_register(2);
    c.return_local();
}
} // namespace
// $80:ACD5; R-0075. Cold text setup through the printer return, before ACF7's frame wait.
void native_audio_begin_menu_text(Clock& c, AudioCpuTextWorkState& state,
                                  std::span<const std::uint8_t> text,
                                  std::span<const std::uint8_t, 256> characters) {
    if (state.prepared || state.drawn) throw std::invalid_argument("menu text already prepared");
    c.call_local();
    c.change_widths();
    c.read_direct();
    c.branch(false);
    c.read_direct();
    c.branch(true);
    c.read_direct(2);
    c.save_register(2);
    c.read_direct(2);
    c.store_direct(2);
    c.call_local();
    c.save_register();
    c.change_widths();
    c.load_constant(2);
    c.store_direct(2);
    c.load_constant();
    c.call_far();
    native_audio_clear_menu_text_work(c);
    c.restore_register();
    c.return_local();
    c.load_constant(2);
    print_menu_text_work(c, state, text, characters);
    state.prepared = true;
}
} // namespace unirally
