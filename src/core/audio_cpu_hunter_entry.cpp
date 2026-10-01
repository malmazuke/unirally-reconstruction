#include "audio_cpu_scene.hpp"
#include <stdexcept>

namespace unirally {
namespace {
// $83:A923; R-0075. Poll before each HVBJOY read, without arrow work or OAM.
void ending_wait(AudioCpuWorkClock& c, AudioCpuQueueState& queue) {
    c.save_register();
    c.change_widths();
    for (;;) {
        c.call_far();
        native_audio_poll_queue(c, queue);
        const bool blank = c.read_vertical_blank();
        c.branch(blank);
        if (!blank) break;
    }
    for (;;) {
        c.call_far();
        native_audio_poll_queue(c, queue);
        const bool blank = c.read_vertical_blank();
        c.branch(!blank);
        if (blank) break;
    }
    c.restore_register();
    c.return_local();
}
}
// R-0075. Static bank-80 F0D6/F51B/98A4, with fresh HUNTER endpoint watches.
// The logo flag is a cartridge word; targets and the held Y count wrap at16 bits.
void native_audio_begin_hunter_code(AudioCpuWorkClock& c, AudioCpuSceneWorkState& scene,
                                    AudioCpuInterruptWorkState& interrupt,
                                    std::array<std::uint8_t, 8192>& cartridge) {
    c.change_widths();
    c.call_local();
    c.save_register();
    c.change_widths();
    c.read_ram(2, true);
    c.load_constant(2);
    c.store_ram(2, true);
    cartridge[0x0742] |= 2;
    interrupt.cartridge_flags = cartridge[0x0742];
    c.restore_register();
    c.return_local();
    c.call_local();
    c.save_register(2);
    c.load_constant(2);
    c.store_ram(2);
    scene.horizontal_target = 0xfd00;
    c.load_constant(2);
    c.store_ram(2);
    scene.vertical_target = 0x0700;
    c.restore_register(2);
    c.return_local();
    c.load_constant(2);
}
// F0E1-F0EA. Thirty-one waited iterations, then a far jump to the ending.
bool native_audio_hunter_code_frame(AudioCpuWorkClock& c, AudioCpuQueueState& queue,
                                    AudioCpuSceneWorkState& scene, std::uint16_t& remaining) {
    if (remaining > 30) throw std::invalid_argument("invalid HUNTER entry count");
    c.call_local();
    native_audio_frame_wait(c, queue, scene);
    c.call_local();
    native_audio_upload_oam(c);
    c.update_register();
    remaining = static_cast<std::uint16_t>(remaining - 1U);
    const bool more = (remaining & 0x8000) == 0;
    c.branch(more);
    if (!more) c.jump_far();
    return more;
}
// $83:AB9A/$83:A4E9; R-0075. Sixteen blank waits with byte brightness decrement.
void native_audio_hunter_first_fade(AudioCpuWorkClock& c, AudioCpuQueueState& queue) {
    c.change_widths();
    c.call_local();
    c.save_register();
    c.change_widths();
    c.load_constant();
    c.store_direct();
    c.load_constant(2);
    for (unsigned frame = 0; frame < 16; ++frame) {
        c.call_local();
        ending_wait(c, queue);
        c.modify_direct_byte();
        c.read_direct();
        c.store_port();
        c.update_register();
        c.branch(frame < 15);
    }
    c.load_constant();
    c.store_port();
    c.restore_register();
    c.return_local();
}
} // namespace unirally
