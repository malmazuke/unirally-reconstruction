#include "audio_cpu_scene.hpp"
#include <stdexcept>

namespace unirally {
namespace {
using Clock = AudioCpuWorkClock;
// $83:AD9B-ADCD; R-0075. One pose index serves two consecutive frames;
// the second tile buffer uses the same index plus25, with word arithmetic.
void build_credits_pose(Clock& c, std::uint16_t pose, bool lower,
                        std::span<const std::uint8_t> pointers,
                        std::span<const std::uint8_t> frames) {
    c.change_widths();
    c.read_direct(2);
    for (unsigned i = 0; i < 3; ++i) c.update_register();
    c.read_rom(2, true, true);
    if (lower) {
        c.update_register();
        c.load_constant(2);
    }
    c.update_register();
    c.load_constant(2);
    c.store_direct(2);
    c.change_widths();
    c.call_far();
    native_audio_build_pose_work(c, pose, pointers, frames);
}
} // namespace
// $83:AD8A-AD9B and $83:A4D2; R-0075. Upload the second initial pose,
// then fifteen brightness increments, each after the ending's blank wait.
void native_audio_hunter_finish_credits_setup(Clock& c, AudioCpuQueueState& queue) {
    c.load_constant(2);
    c.store_direct(2);
    c.call_far();
    native_audio_upload_pose_work(c);
    c.call_local();
    c.save_register();
    c.change_widths();
    c.store_direct();
    c.load_constant(2);
    for (unsigned frame = 0; frame < 15; ++frame) {
        c.call_local();
        native_audio_hunter_ending_wait(c, queue);
        c.modify_direct_byte();
        c.read_direct();
        c.store_port();
        c.update_register();
        c.branch(frame < 14);
    }
    c.restore_register();
    c.return_local();
    c.load_constant(2);
    c.store_direct(2);
}
// $83:AD9B-ADFA; R-0075.96-frame animation cycle, with two native pose
// builds and five-row uploads. An enabled pad ends before the counter update.
bool native_audio_hunter_credits_frame(Clock& c, AudioCpuQueueState& queue,
                                       AudioCpuHunterWorkState& state,
                                       const std::array<std::uint8_t, 8192>& cartridge,
                                       std::span<const std::uint8_t, 96> poses,
                                       std::span<const std::uint8_t> pointers,
                                       std::span<const std::uint8_t> frames) {
    if (state.credits_frame >= 96) throw std::invalid_argument("invalid credits frame");
    const auto offset = state.credits_frame / 2U * 2U;
    const auto pose = static_cast<std::uint16_t>(poses[offset] | unsigned(poses[offset + 1]) << 8);
    build_credits_pose(c, pose, false, pointers, frames);
    build_credits_pose(c, static_cast<std::uint16_t>(pose + 25U), true, pointers, frames);
    c.call_local();
    native_audio_hunter_ending_wait(c, queue);
    for (bool lower : {false, true}) {
        c.load_constant(2);
        c.store_direct(2);
        if (lower)
            c.call_local();
        else
            c.call_far();
        native_audio_upload_pose_work(c, lower);
    }
    c.call_far();
    c.call_local();
    state.controllers = native_audio_poll_controllers(c);
    c.return_far();
    c.call_far();
    const bool pressed = native_audio_ending_press(c, state.controllers, cartridge);
    c.branch(pressed);
    if (pressed) return false;
    c.change_widths();
    c.modify_direct_word();
    ++state.credits_frame;
    c.read_direct(2);
    c.load_constant(2);
    const bool more = state.credits_frame < 96;
    c.branch(more);
    if (!more) {
        c.store_direct(2);
        state.credits_frame = 0;
        c.branch(true);
    }
    return true;
}
} // namespace unirally
