#include "audio_cpu_scene.hpp"
#include <stdexcept>

namespace unirally {
namespace {
using Clock = AudioCpuWorkClock;
// $80:B124; R-0075. Code acknowledgment uses the same native command ring.
void acknowledge_title_code(Clock& c, AudioCpuQueueState& queue) {
    c.save_register();
    c.change_widths();
    c.save_register(2);
    c.load_constant(2);
    c.call_far();
    native_audio_enqueue(c, queue, 8, 63);
    c.load_constant(2);
    c.call_far();
    native_audio_enqueue(c, queue, 4, 2);
    c.restore_register(2);
    c.restore_register();
    c.return_local();
}
// $80:F5E3; R-0075. Restore/save level loops each traverse sixteen bytes.
void apply_title_code(Clock& c, AudioCpuQueueState& queue, AudioCpuSceneWorkState& scene) {
    c.change_widths();
    c.load_constant(2);
    c.load_constant(2);
    for (unsigned i = 0; i < 16; ++i) {
        c.read_ram(1, true, true);
        c.store_ram(1, true, true);
        c.update_register();
        c.update_register();
        c.branch(i < 15);
    }
    c.load_constant(2);
    c.load_constant(2);
    for (unsigned i = 0; i < 16; ++i) {
        c.load_constant();
        c.store_ram(1, true, true);
        c.update_register();
        c.update_register();
        c.branch(i < 15);
    }
    c.load_constant();
    c.store_ram(1, true);
    scene.title_levels_pending = 1;
    c.call_local();
    acknowledge_title_code(c, queue);
}
}
// $80:D1EC; R-0075. OAM DMA completes before the two automatic pad reads.
std::array<std::uint16_t, 2> native_audio_poll_controllers(Clock& c) {
    c.call_local();
    native_audio_upload_oam(c);
    std::array<std::uint16_t, 2> controllers;
    for (unsigned port = 0; port < 2; ++port) {
        controllers[port] = c.read_controller(port);
        c.store_direct(2);
    }
    c.return_local();
    return controllers;
}
// $80:F5C0; R-0075. Hold counter is a wrapping 16-bit Y value.
void native_audio_begin_title_hold(Clock& c) {
    c.load_constant(2);
    c.store_direct(2);
    c.load_constant(2);
}
// $80:F5C8; R-0075. Input code index advances only on an exact word match.
bool native_audio_title_hold_frame(Clock& c, AudioCpuQueueState& queue,
                                   AudioCpuSceneWorkState& scene, AudioCpuTitleHoldState& state) {
    if (state.remaining > 110 || state.code_index > 4)
        throw std::invalid_argument("invalid native title hold state");
    constexpr std::array<std::uint16_t, 5> code{0x0800, 0x0200, 0x0800, 0x0010, 0x0080};
    c.save_register(2);
    c.save_register(2);
    c.call_local();
    native_audio_frame_wait(c, queue, scene);
    c.call_local();
    state.controllers = native_audio_poll_controllers(c);
    c.restore_register(2);
    c.change_widths();
    c.read_direct(2);
    c.read_rom(2, false, true);
    c.change_widths();
    const bool matched = state.controllers[0] == code[state.code_index];
    c.branch(!matched);
    if (matched) {
        c.update_register();
        c.update_register();
        c.load_constant(2);
        ++state.code_index;
        c.branch(state.code_index != 5);
        if (state.code_index == 5) {
            apply_title_code(c, queue, scene);
            state.code_index = 0;
        }
    }
    c.restore_register(2);
    c.update_register();
    state.remaining = static_cast<std::uint16_t>(state.remaining - 1U);
    const bool more = (state.remaining & 0x8000) == 0;
    c.branch(more);
    return more;
}
} // namespace unirally
