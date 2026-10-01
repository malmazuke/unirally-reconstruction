#include "audio_cpu_scene.hpp"

namespace unirally {
namespace {
using Clock = AudioCpuWorkClock;
bool whole_pad_press(Clock& c, const std::array<std::uint16_t, 2>& controllers) {
    c.read_direct(2);
    const bool first = controllers[0] != 0;
    c.branch(first);
    if (first) return true;
    c.read_direct(2);
    const bool second = controllers[1] != 0;
    c.branch(!second);
    return second;
}
} // namespace
// $80:B6CF/B6D3; R-0075. Cartridge disable bits mask each pad, except low4.
bool native_audio_ending_press(Clock& c, const std::array<std::uint16_t, 2>& controllers,
                               const std::array<std::uint8_t, 8192>& cartridge) {
    c.call_local();
    c.save_register();
    c.change_widths();
    c.read_ram(2, true);
    c.load_constant(2);
    const bool skip_first = (cartridge[0x0743] & 2) != 0;
    c.branch(skip_first);
    bool pressed = false;
    if (!skip_first) {
        c.read_direct(2);
        c.load_constant(2);
        pressed = (controllers[0] & 0xfff0) != 0;
        c.branch(pressed);
        if (!pressed) c.read_ram(2, true);
    }
    if (!pressed) {
        c.load_constant(2);
        const bool skip_second = (cartridge[0x0743] & 4) != 0;
        c.branch(skip_second);
        if (!skip_second) {
            c.read_direct(2);
            c.load_constant(2);
            pressed = (controllers[1] & 0xfff0) != 0;
            c.branch(pressed);
            if (!pressed) c.branch(true);
        }
    }
    c.restore_register();
    c.change_widths();
    c.return_local();
    c.return_far();
    return pressed;
}
// $80:C202-C24B; R-0075. Two consecutive whole-pad presses, with native work
// between them. A release after the first returns to the initial wait.
bool native_audio_hunter_page_wait_frame(Clock& c, AudioCpuQueueState& queue,
                                         AudioCpuSceneWorkState& scene,
                                         AudioCpuHunterWorkState& state) {
    if (!state.wait_started) {
        c.call_far();
        c.call_local();
        c.save_register();
        c.change_widths();
        c.save_register(2);
        state.wait_started = true;
    }
    c.change_widths();
    c.call_local();
    native_audio_frame_wait(c, queue, scene);
    c.call_local();
    state.controllers = native_audio_poll_controllers(c);
    c.call_far();
    native_audio_step_decorations(c, state.decorations);
    c.change_widths();
    const bool pressed = whole_pad_press(c, state.controllers);
    const bool done = state.first_press && pressed;
    state.first_press = pressed;
    if (!done) return true;
    c.change_widths();
    c.load_constant();
    c.store_ram();
    c.read_ram();
    c.load_constant();
    c.load_constant();
    c.store_ram();
    c.change_widths();
    c.restore_register(2);
    c.restore_register();
    c.return_local();
    c.return_far();
    return false;
}
// $83:AC1F-AC32; R-0075. Up to1201 blank waits, ending early on an enabled pad.
bool native_audio_hunter_timed_frame(Clock& c, AudioCpuQueueState& queue,
                                     AudioCpuHunterWorkState& state,
                                     const std::array<std::uint8_t, 8192>& cartridge) {
    if (!state.timed_started) {
        c.load_constant(2);
        state.timed_remaining = 1200;
        state.timed_started = true;
    }
    c.call_local();
    native_audio_hunter_ending_wait(c, queue);
    c.call_far();
    c.call_local();
    state.controllers = native_audio_poll_controllers(c);
    c.return_far();
    c.call_far();
    const bool pressed = native_audio_ending_press(c, state.controllers, cartridge);
    c.branch(pressed);
    if (pressed) return false;
    c.update_register();
    state.timed_remaining = static_cast<std::uint16_t>(state.timed_remaining - 1U);
    const bool more = (state.timed_remaining & 0x8000) == 0;
    c.branch(more);
    return more;
}
} // namespace unirally
