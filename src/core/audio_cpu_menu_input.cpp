#include "audio_cpu_menu_input.hpp"
#include <stdexcept>

namespace unirally {
namespace {
using Clock = AudioCpuWorkClock;
using Cartridge = std::array<std::uint8_t, 8192>;
// $80:B71D/$80:B76F/$80:B794; R-0075. Cartridge bits9/10 exclude pad1/pad2.
bool test_menu_input(Clock& c, const Cartridge& cartridge, const AudioCpuMenuInputState& state,
                     std::uint16_t mask) {
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
        pressed = (state.controllers[0] & mask) != 0;
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
            pressed = (state.controllers[1] & mask) != 0;
            c.branch(pressed);
            if (!pressed) c.branch(true);
        }
    }
    c.restore_register();
    c.change_widths();
    c.return_local();
    return pressed;
}
// $80:ABEB/$80:ABFC; R-0075. Exact whole-pad words, before ordinary menu tests.
bool test_menu_code(Clock& c, const AudioCpuMenuInputState& state, std::uint16_t code) {
    c.read_direct(2);
    c.load_constant(2);
    const bool first = state.controllers[0] == code;
    c.branch(first);
    if (first) return true;
    c.read_direct(2);
    c.load_constant(2);
    const bool second = state.controllers[1] == code;
    c.branch(!second);
    return second;
}
// $80:B178; R-0075. Navigation changes effect volume and starts effect3 over music.
void enqueue_navigation(Clock& c, AudioCpuQueueState& queue) {
    c.call_local();
    c.save_register();
    c.change_widths();
    c.save_register(2);
    c.load_constant(2);
    c.call_far();
    native_audio_enqueue(c, queue, 8, 127);
    c.load_constant(2);
    c.call_far();
    native_audio_enqueue(c, queue, 2, 3);
    c.restore_register(2);
    c.restore_register();
    c.return_local();
}
// $80:AC50/$80:AC9D; R-0075. Targets are fixed-point words, wrapping modulo65536.
void update_menu_targets(Clock& c, AudioCpuSceneWorkState& scene, AudioCpuMenuInputState& state,
                         std::span<const std::uint8_t, 5> positions, bool up) {
    c.load_constant();
    c.store_direct();
    state.direction_latched = true;
    c.load_constant(2);
    c.store_direct(2);
    state.idle_remaining = 1500;
    c.read_direct();
    c.change_widths();
    c.load_constant(2);
    c.update_register();
    c.read_direct(2);
    c.update_register();
    c.read_rom(2, false, true);
    c.load_constant(2);
    for (unsigned bit = 0; bit < 7; ++bit) c.update_register();
    c.store_ram(2);
    scene.horizontal_target = static_cast<std::uint16_t>(unsigned(positions[state.selection]) << 7);
    c.read_ram(2);
    c.update_register();
    c.load_constant(2);
    c.store_ram(2);
    scene.vertical_target = static_cast<std::uint16_t>(scene.vertical_target + (up ? -384 : 384));
    c.change_widths();
    c.branch_long();
}
// $80:AC39/$80:AC83; R-0075. One cue per directional hold; five choices wrap.
void move_menu_selection(Clock& c, AudioCpuQueueState& queue, AudioCpuSceneWorkState& scene,
                         AudioCpuMenuInputState& state, std::span<const std::uint8_t, 5> positions,
                         bool up) {
    c.read_direct();
    c.load_constant();
    c.branch(state.direction_latched);
    if (state.direction_latched) {
        if (!up) c.branch_long();
        return;
    }
    enqueue_navigation(c, queue);
    bool wrapped;
    if (up) {
        c.modify_direct_byte();
        state.selection = static_cast<std::uint8_t>(state.selection - 1U);
        wrapped = (state.selection & 128) != 0;
        c.branch(!wrapped);
    } else {
        c.read_direct();
        c.update_register();
        ++state.selection;
        c.load_constant();
        wrapped = state.selection >= 5;
        c.branch(!wrapped);
    }
    if (wrapped) {
        c.load_constant(2);
        c.store_ram(2);
        scene.vertical_target = up ? 0x0d00 : 0x0400;
        c.load_constant();
        state.selection = up ? 4 : 0;
    }
    if (!up || wrapped) c.store_direct();
    update_menu_targets(c, scene, state, positions, up);
}
} // namespace
// $80:ABC8/$83:9558; R-0075. Begin with both pads enabled and first-choice targets.
void native_audio_begin_menu_input(Clock& c, AudioCpuSceneWorkState& scene, Cartridge& cartridge,
                                   AudioCpuMenuInputState& state) {
    c.call_local();
    c.call_far();
    c.save_register();
    c.change_widths();
    c.save_register(2);
    c.read_ram(2, true);
    c.load_constant(2);
    c.load_constant(2);
    c.store_ram(2, true);
    cartridge[0x0743] &= 0xf9;
    c.restore_register(2);
    c.restore_register();
    c.return_far();
    for (unsigned i = 0; i < 3; ++i) c.store_direct();
    c.load_constant(2);
    c.store_direct(2);
    c.load_constant(2);
    c.store_ram(2);
    c.load_constant(2);
    c.store_ram(2);
    scene.horizontal_target = 0x04fd;
    scene.vertical_target = 0x0580;
    state = {};
}
// $80:ABE3-$80:AC2C; R-0075. One waited/polled iteration, retaining the release latch.
AudioCpuMenuAction native_audio_menu_input_frame(Clock& c, AudioCpuQueueState& queue,
                                                 AudioCpuSceneWorkState& scene,
                                                 Cartridge& cartridge,
                                                 AudioCpuMenuInputState& state,
                                                 std::span<const std::uint8_t, 5> positions) {
    if (state.selection > 4) throw std::invalid_argument("invalid native menu selection");
    c.call_local();
    native_audio_frame_wait(c, queue, scene);
    c.call_local();
    state.controllers = native_audio_poll_controllers(c);
    c.change_widths();
    if (test_menu_code(c, state, 0x02b0)) {
        c.branch_long();
        return AudioCpuMenuAction::wipe_ram;
    }
    if (test_menu_code(c, state, 0x8430)) {
        c.branch_long();
        return AudioCpuMenuAction::hunter;
    }
    c.change_widths();
    c.read_direct(2);
    c.update_register();
    state.idle_remaining = static_cast<std::uint16_t>(state.idle_remaining - 1U);
    const bool expired = (state.idle_remaining & 0x8000) != 0;
    c.branch(expired);
    if (expired) {
        c.change_widths();
        c.load_constant();
        c.store_direct();
        state.selection = 5;
        c.load_constant();
        c.store_direct();
        c.return_local();
        return AudioCpuMenuAction::attract;
    }
    c.store_direct(2);
    c.change_widths();
    const bool confirmed = test_menu_input(c, cartridge, state, 0x9080);
    c.branch(!confirmed);
    if (confirmed) {
        c.branch_long();
        c.load_constant();
        c.store_direct();
        c.return_local();
        return AudioCpuMenuAction::selected;
    }
    const bool down = test_menu_input(c, cartridge, state, 0x2400);
    c.branch(down);
    if (down) {
        move_menu_selection(c, queue, scene, state, positions, false);
        return AudioCpuMenuAction::waiting;
    }
    const bool up = test_menu_input(c, cartridge, state, 0x0800);
    c.branch(up);
    if (up) {
        move_menu_selection(c, queue, scene, state, positions, true);
        return AudioCpuMenuAction::waiting;
    }
    c.store_direct();
    state.direction_latched = false;
    c.branch(true);
    return AudioCpuMenuAction::waiting;
}
} // namespace unirally
