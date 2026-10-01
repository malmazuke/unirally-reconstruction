#include "audio_cpu_scene.hpp"
#include <stdexcept>

namespace unirally {
namespace {
using Clock = AudioCpuWorkClock;
void read_word(Clock& c) {
    c.read_ram(2);
}
std::uint16_t read_position_difference(Clock& c, std::uint16_t current, std::uint16_t target) {
    read_word(c);
    c.update_register();
    read_word(c);
    const auto difference = static_cast<std::uint16_t>(target - current);
    c.branch(difference == 0);
    return difference;
}
std::uint16_t divide_difference(Clock& c, std::uint16_t difference) {
    c.update_register();
    c.update_register();
    c.load_constant(2);
    auto increment = static_cast<std::uint16_t>(difference >> 2);
    const bool negative = (increment & 0x2000) != 0;
    c.branch(!negative);
    if (negative) {
        c.load_constant(2);
        increment |= 0xc000;
    }
    return increment;
}
void update_horizontal_flags(Clock& c, std::uint16_t position, std::uint8_t& flags) {
    c.load_constant(2);
    const bool negative = (position & 0x8000) != 0;
    c.branch(!negative);
    c.change_widths();
    c.read_ram();
    c.load_constant();
    c.store_ram();
    c.change_widths();
    if (negative) c.branch(true);
    flags = static_cast<std::uint8_t>((flags & 0xbf) | (negative ? 0x40 : 0));
}
void update_horizontal(Clock& c, AudioCpuSceneWorkState& state) {
    const auto difference =
        read_position_difference(c, state.horizontal_current, state.horizontal_target);
    if (!difference) return;
    const auto increment = divide_difference(c, difference);
    c.store_ram(2);
    c.store_ram(2);
    read_word(c);
    c.update_register();
    read_word(c);
    c.store_ram(2);
    state.horizontal_current = static_cast<std::uint16_t>(state.horizontal_current + increment);
    c.save_register(2);
    update_horizontal_flags(c, state.horizontal_current, state.first_horizontal_flags);
    c.read_stack(2);
    c.update_register();
    c.load_constant(2);
    update_horizontal_flags(c, static_cast<std::uint16_t>(state.horizontal_current + 112),
                            state.second_horizontal_flags);
    c.restore_register(2);
    c.update_register();
    c.update_register();
    c.update_register();
    c.update_register();
    c.change_widths();
    c.store_ram();
    c.update_register();
    c.load_constant();
    c.store_ram();
    c.change_widths();
}
void update_vertical(Clock& c, AudioCpuSceneWorkState& state) {
    const auto difference =
        read_position_difference(c, state.vertical_current, state.vertical_target);
    if (!difference) return;
    const auto increment = divide_difference(c, difference);
    c.store_ram(2);
    c.store_ram(2);
    read_word(c);
    c.update_register();
    read_word(c);
    c.store_ram(2);
    state.vertical_current = static_cast<std::uint16_t>(state.vertical_current + increment);
    for (unsigned i = 0; i < 4; ++i) c.update_register();
    c.change_widths();
    c.update_register();
    c.load_constant();
    c.store_ram();
    c.update_register();
    c.load_constant();
    c.store_ram();
}
// $80:FAF5; R-0075. Static bank-80 FAF5-FBC4. No observed function duration is used.
void update_scene_work(Clock& c, AudioCpuSceneWorkState& state) {
    if (state.phase > 31) throw std::invalid_argument("invalid frontend phase");
    c.save_register();
    c.change_widths();
    c.modify_direct_byte();
    const bool wrapped = state.phase == 0;
    c.branch(!wrapped);
    if (wrapped) {
        c.load_constant();
        c.store_direct();
    }
    state.phase = static_cast<std::uint8_t>((state.phase - 1U) & 31);
    c.change_widths();
    c.read_direct(2);
    c.update_register();
    c.update_register();
    c.change_widths();
    c.read_rom(1, true, true);
    c.store_ram();
    c.store_ram();
    c.change_widths();
    update_horizontal(c, state);
    update_vertical(c, state);
    c.restore_register();
    c.return_local();
}
// $80:9318; R-0075. Static bank-80 9318-933B; R-0054 identifies the 544-byte OAM buffer.
void upload_oam(Clock& c) {
    c.change_widths();
    c.store_port(2);
    for (unsigned i = 0; i < 3; ++i) {
        c.load_constant(2);
        c.store_port(2);
    }
    c.change_widths();
    c.load_constant();
    c.store_port();
    c.load_constant();
    c.store_port();
    c.request_dma(544);
    c.return_local();
}
void change_direct_byte(Clock& c) {
    c.modify_direct_byte();
}
// $80:9869; $80:9885; R-0075. Static bank-80 9869-98A3: seven waits, OAM DMA and brightness steps.
void fade(Clock& c, AudioCpuQueueState& queue, AudioCpuSceneWorkState& scene, bool darken) {
    if (darken) c.load_constant();
    c.store_direct();
    c.load_constant(2);
    for (unsigned i = 0; i < 7; ++i) {
        c.call_local();
        native_audio_frame_wait(c, queue, scene);
        c.call_local();
        upload_oam(c);
        change_direct_byte(c);
        change_direct_byte(c);
        c.read_direct();
        c.store_port();
        c.update_register();
        c.branch(i < 6);
    }
    if (darken) {
        c.load_constant();
        c.store_port();
    }
    c.return_local();
}
}
void native_audio_title_fade(Clock& c, AudioCpuQueueState& queue, AudioCpuSceneWorkState& scene,
                             bool darken) {
    fade(c, queue, scene, darken);
}
void native_audio_finish_waited_frame(Clock& c, AudioCpuSceneWorkState& state) {
    c.call_local();
    update_scene_work(c, state);
    c.return_local();
}
void native_audio_frame_wait(Clock& c, AudioCpuQueueState& queue, AudioCpuSceneWorkState& scene) {
    c.call_far();
    native_audio_poll_queue(c, queue);
    native_audio_wait_vblank(c, queue);
    native_audio_finish_waited_frame(c, scene);
}
// $80:A8A8; R-0075. Starts at B091, ends after the call into 8399F6. A8A8 uploads 216 bytes
// from the identified base palette; original entry clocks are not inputs.
void native_audio_upload_base_palette(Clock& c, AudioCpuQueueState& queue,
                                      AudioCpuSceneWorkState& scene) {
    c.call_local();
    c.call_local();
    native_audio_frame_wait(c, queue, scene);
    c.call_local();
    upload_oam(c);
    c.load_constant();
    c.store_port();
    c.load_constant(2);
    c.store_port(2);
    c.load_constant();
    c.store_port();
    c.load_constant(2);
    c.store_port(2);
    for (unsigned i = 0; i < 3; ++i) {
        c.load_constant();
        c.store_port();
    }
    c.request_dma(216);
    c.return_local();
    c.call_far();
}
// $83:99F6; $80:B08C; R-0075. Static bank-83 99F6-9A1D and bank-80 B098-B0CA; R-0075.
void native_audio_load_nintendo_graphics(Clock& c, AudioCpuSceneWorkState& scene,
                                         const AudioCpuGraphicsAsset& palette,
                                         const AudioCpuGraphicsAsset& map,
                                         const AudioCpuGraphicsAsset& tiles) {
    c.call_local();
    c.save_register(2);
    for (unsigned pair = 0; pair < 2; ++pair) {
        c.load_constant(2);
        c.store_ram(2);
        c.store_ram(2);
    }
    scene.horizontal_current = scene.horizontal_target = 0xfd00;
    scene.vertical_current = scene.vertical_target = 0x0700;
    c.load_constant(2);
    for (unsigned i = 0; i < 4; ++i) c.store_ram(2);
    c.restore_register(2);
    c.return_local();
    c.return_far();
    for (unsigned i = 0; i < 3; ++i) {
        c.load_constant();
        c.store_port();
    }
    c.load_constant(2);
    c.load_constant();
    c.call_far();
    native_audio_cpu_upload_graphics_asset(c, palette, true);
    c.load_constant(2);
    c.load_constant();
    c.call_far();
    native_audio_cpu_upload_graphics_asset(c, map, false);
    c.load_constant(2);
    c.load_constant();
    c.call_far();
    native_audio_cpu_upload_graphics_asset(c, tiles, false);
    c.load_constant();
    c.store_port();
    c.store_port();
    c.change_widths();
}
// $80:B0CC; R-0075. Static bank-80 B0CC-B0DB; R-0054/R-0075. The hold contains 111 waits.
void native_audio_finish_nintendo_screen(Clock& c, AudioCpuQueueState& queue,
                                         AudioCpuSceneWorkState& scene) {
    c.call_local();
    fade(c, queue, scene, false);
    c.load_constant(2);
    for (unsigned i = 0; i < 111; ++i) {
        c.call_local();
        native_audio_frame_wait(c, queue, scene);
        c.update_register();
        c.branch(i < 110);
    }
    c.call_local();
    fade(c, queue, scene, true);
    c.return_local();
}
} // namespace unirally
