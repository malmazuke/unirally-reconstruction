#include "audio_cpu_scene.hpp"
#include <stdexcept>

namespace unirally {
namespace {
using Clock = AudioCpuWorkClock;
// $83:8AF7; R-0075. Verify the original 8 KiB mirror by writing across its last byte.
void verify_cartridge_mirror(Clock& c, std::array<std::uint8_t, 8192>& cartridge) {
    c.call_far();
    c.save_register();
    c.change_widths();
    c.read_ram(1, true);
    const auto saved = cartridge[0];
    c.save_register();
    c.load_constant();
    c.store_ram(1, true);
    cartridge[0] = 0x12;
    c.change_widths();
    c.load_constant(2);
    c.store_ram(2, true);
    cartridge[0x1fff] = 0x56;
    cartridge[0x2000 & 0x1fff] = 0x34;
    c.change_widths();
    c.read_ram(1, true);
    c.load_constant();
    const bool mirrored = cartridge[0] == 0x34;
    c.branch(mirrored);
    if (!mirrored) throw std::logic_error("cartridge mirror failed");
    c.restore_register();
    c.store_ram(1, true);
    cartridge[0] = saved;
    c.restore_register();
    c.return_far();
}
// $83:8B51; R-0075. Bit2 requests the 1024-word menu text workspace reset.
void clear_menu_text_work(Clock& c) {
    for (unsigned bit = 0; bit < 3; ++bit) {
        c.update_register();
        c.branch(bit == 2);
    }
    c.save_register();
    c.change_widths();
    for (unsigned value = 0; value < 3; ++value) c.load_constant(2);
    for (unsigned word = 0; word < 1024; ++word) {
        c.store_ram(2, false, true);
        for (unsigned step = 0; step < 3; ++step) c.update_register();
        c.branch(word < 1023);
    }
    c.change_widths();
    c.restore_register();
    c.branch_long();
    for (unsigned bit = 0; bit < 3; ++bit) c.update_register();
    c.branch(true);
    c.return_far();
}
// $83:FB41; R-0075. Clear each cartridge word in descending address order.
void clear_cartridge(Clock& c, std::array<std::uint8_t, 8192>& cartridge) {
    c.call_far();
    c.call_local();
    c.change_widths();
    c.load_constant(2);
    c.load_constant(2);
    for (unsigned remaining = 4096; remaining > 0; --remaining) {
        c.store_ram(2, true, true);
        cartridge[2 * (remaining - 1)] = 0;
        cartridge[2 * (remaining - 1) + 1] = 0;
        c.update_register();
        c.update_register();
        c.branch(remaining > 1);
    }
    c.return_local();
    c.return_far();
}
} // namespace
// $80:D36F; $80:8C4E; R-0075. Finish layout, check cartridge signature, and on a
// mismatch stop before the default-record calls at8C80. A matching header returns normally.
bool native_audio_begin_menu_records(Clock& c, AudioCpuQueueState& queue,
                                     AudioCpuSceneWorkState& scene,
                                     std::array<std::uint8_t, 8192>& cartridge,
                                     std::span<const std::uint8_t, 12> signature) {
    c.call_local();
    native_audio_frame_wait(c, queue, scene);
    c.call_local();
    native_audio_upload_oam(c);
    c.load_constant();
    c.set_nmi_enabled(true);
    c.return_local();
    verify_cartridge_mirror(c, cartridge);
    c.call_local();
    c.save_register();
    c.change_widths();
    c.load_constant();
    c.call_far();
    clear_menu_text_work(c);
    c.read_direct(2);
    c.store_direct(2);
    c.change_widths();
    c.load_constant(2);
    c.load_constant(2);
    for (unsigned word = 0; word < 6; ++word) {
        c.read_ram(2, true, true);
        c.read_rom(2, true, true);
        const bool mismatch = cartridge[2 * word] != signature[2 * word]
                           || cartridge[2 * word + 1] != signature[2 * word + 1];
        c.branch(mismatch);
        if (mismatch) {
            clear_cartridge(c, cartridge);
            for (unsigned field = 0; field < 4; ++field) c.store_direct(2);
            return true;
        }
        for (unsigned step = 0; step < 3; ++step) c.update_register();
        c.branch(word < 5);
    }
    c.restore_register();
    c.return_local();
    return false;
}
} // namespace unirally
