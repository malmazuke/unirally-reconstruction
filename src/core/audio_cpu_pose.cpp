#include "audio_cpu_scene.hpp"
#include <stdexcept>

namespace unirally {
namespace {
using Clock = AudioCpuWorkClock;
// $83:8484-84D8. Native tile-source arithmetic, then a 33-byte block copy.
// A=32 decrements through0 toFFFF. The RAM block-move/return bus work is
// charged explicitly; original instructions are never loaded or executed.
void copy_pose_tile(Clock& c) {
    c.save_register();
    c.change_widths();
    c.update_register();
    c.exchange_accumulator_bytes();
    c.load_constant(2);
    c.update_register();
    c.update_register();
    c.update_register();
    c.load_constant(2);
    c.change_widths();
    c.store_direct();
    c.change_widths();
    c.update_register();
    c.load_constant(2);
    for (unsigned i = 0; i < 6; ++i) c.update_register();
    c.load_constant(2);
    c.store_direct(2);
    c.read_direct(2);
    c.exchange_accumulator_bytes();
    c.update_register();
    c.store_direct(2);
    c.read_direct(2);
    for (unsigned i = 0; i < 6; ++i) c.update_register();
    c.read_direct(2);
    c.update_register();
    c.read_direct(2);
    c.store_direct(2);
    c.change_widths();
    c.read_direct();
    c.store_ram();
    c.load_constant();
    c.store_ram();
    c.change_widths();
    c.load_constant(2);
    c.read_direct(2);
    c.read_direct(2);
    c.call_far();
    for (unsigned byte = 0; byte < 33; ++byte) c.move_ram_byte();
    c.return_ram_far();
    c.restore_register();
    c.change_widths();
    c.return_far();
}
void pose_prefix(Clock& c) {
    c.save_register();
    c.save_register(2);
    for (unsigned i = 0; i < 8; ++i) {
        c.read_direct(2);
        c.save_register(2);
    }
    c.change_widths();
    c.store_direct(2);
    c.update_register();
    c.update_register();
    c.update_register();
    c.read_direct(2);
    c.update_register();
    c.read_slow_rom(2, true, true);
    c.update_register();
    c.change_widths();
    c.read_slow_rom(1, true, true);
    c.update_register();
    c.load_constant();
    c.save_register();
    c.restore_register();
    c.change_widths();
    for (unsigned i = 0; i < 2; ++i) {
        c.read_slow_rom(2, false, true);
        c.store_direct(2);
        c.update_register();
        c.update_register();
    }
    c.change_widths();
    c.load_constant();
    c.store_direct();
    c.load_constant(2);
    c.store_direct(2);
    c.store_direct(2);
}
void mask_group(Clock& c) {
    c.load_constant();
    c.store_direct();
    c.read_direct();
    c.load_constant();
    c.update_register();
    c.load_constant();
    c.change_widths();
    c.load_constant(2);
    c.update_register();
    c.change_widths();
    c.read_ram(1, true, true);
    c.store_direct();
}
void pose_cell(Clock& c, bool present, unsigned index) {
    c.modify_direct_byte();
    c.branch(!present);
    if (present) {
        c.read_slow_rom(1, false, true);
        c.exchange_accumulator_bytes();
        c.read_slow_rom(1, false, true);
        c.update_register();
        c.update_register();
        c.branch(true);
    } else {
        c.change_widths();
        c.load_constant(2);
        c.change_widths();
        c.branch(true);
    }
    c.save_register(2);
    c.call_far();
    copy_pose_tile(c);
    c.restore_register(2);
    c.read_direct(2);
    c.update_register();
    c.store_direct(2);
    c.load_constant(2);
    const bool next_column = index % 6 < 5;
    c.branch(next_column);
    if (!next_column) {
        c.load_constant(2);
        c.store_direct(2);
        c.read_direct(2);
        c.update_register();
        c.store_direct(2);
        c.load_constant(2);
        c.branch(index < 29);
    }
}
} // namespace
// $83:8E3A-8EFD; R-0075. Four header bytes supply30 presence bits. The
// identified pose directory and frame data choose branches, never observations.
void native_audio_build_pose_work(Clock& c, std::uint16_t pose,
                                  std::span<const std::uint8_t> pointers,
                                  std::span<const std::uint8_t> frames) {
    const auto at = std::size_t(pose) * 3;
    if (at + 3 > pointers.size()) throw std::invalid_argument("pose pointer outside native data");
    const auto address = unsigned(pointers[at]) | unsigned(pointers[at + 1]) << 8;
    if (address < 32768) throw std::invalid_argument("pose address outside native data");
    const auto frame = std::size_t(pointers[at + 2]) * 32768 + address - 32768;
    if (frame + 4 > frames.size()) throw std::invalid_argument("pose header outside native data");
    unsigned words = 0;
    for (unsigned i = 0; i < 30; ++i) words += (unsigned(frames[frame + i / 8]) >> (7 - i % 8)) & 1;
    if (frame + 4 + words * 2 > frames.size())
        throw std::invalid_argument("pose references outside native data");
    pose_prefix(c);
    for (unsigned cell = 0; cell < 30; ++cell) {
        if (cell % 8 == 0) mask_group(c);
        pose_cell(c, (frames[frame + cell / 8] & (128U >> (cell % 8))) != 0, cell);
        if (cell < 29) {
            c.modify_direct_byte();
            const bool more_bits = cell % 8 != 7;
            c.branch(more_bits);
            if (!more_bits) {
                c.modify_direct_byte();
                c.branch(true);
            }
        }
    }
    for (unsigned i = 0; i < 8; ++i) {
        c.restore_register(2);
        c.store_direct(2);
    }
    c.restore_register(2);
    c.restore_register();
    c.return_far();
}
} // namespace unirally
