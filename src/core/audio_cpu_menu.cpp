#include "audio_cpu_scene.hpp"
#include <stdexcept>

namespace unirally {
// $80:D20E; R-0075. Static bank-80 D20E-D263. The native title hook remains
// active through palette DMA, reset work and the first identified raw asset.
void native_audio_menu_first_palette(AudioCpuWorkClock& c, AudioCpuQueueState& queue,
                                     AudioCpuSceneWorkState& scene,
                                     AudioCpuInterruptWorkState& interrupt,
                                     const AudioCpuGraphicsAsset& palette) {
    c.call_local();
    c.call_local();
    native_audio_frame_wait(c, queue, scene);
    c.call_local();
    native_audio_palette_dma_work(c, queue, scene);
    c.load_constant();
    c.store_port();
    c.call_local();
    native_audio_initialize_title_work(c, scene, interrupt);
    for (unsigned i = 0; i < 10; ++i) {
        c.load_constant();
        c.store_port();
    }
    for (unsigned i = 0; i < 3; ++i) c.store_port();
    c.change_widths();
    c.load_constant(2);
    c.load_constant(2);
    c.call_far();
    native_audio_cpu_upload_graphics_asset(c, palette, true);
}
// $83:91F7; R-0075. The cold erased mode byte selects asset2 at palette240.
// A selected rider's mode1 path needs its own typed palette selection.
void native_audio_menu_graphics(AudioCpuWorkClock& c, const AudioCpuSceneWorkState& scene,
                                const std::array<AudioCpuGraphicsAsset, 128>& assets) {
    if (scene.menu_mode == 1) throw std::invalid_argument("cold menu rider palette domain");
    c.call_far();
    c.call_local();
    c.save_register();
    c.change_widths();
    c.read_ram(2, true);
    c.load_constant(2);
    c.load_constant(2);
    c.branch(false);
    c.load_constant(2);
    c.load_constant(2);
    c.call_far();
    native_audio_cpu_upload_graphics_asset(c, assets[2], true);
    c.restore_register();
    c.return_local();
    c.return_far();
    c.load_constant(2);
    c.load_constant(2);
    c.call_far();
    native_audio_cpu_upload_graphics_asset(c, assets[28], true);
    constexpr std::array<unsigned, 6> tiles{89, 91, 77, 70, 68, 69};
    for (const auto id : tiles) {
        c.load_constant(2);
        c.load_constant(2);
        c.call_far();
        native_audio_cpu_upload_graphics_asset(c, assets[id], false);
    }
}
namespace {
using Clock = AudioCpuWorkClock;
// $80:D2C1; R-0075. Reset 128 sprite records and the 32 packed high-bit bytes.
void reset_menu_sprite_work(Clock& c) {
    c.change_widths();
    c.load_constant(2);
    c.load_constant();
    c.load_constant(2);
    for (unsigned sprite = 0; sprite < 128; ++sprite) {
        for (unsigned byte = 0; byte < 4; ++byte) c.store_ram(1, false, true);
        for (unsigned step = 0; step < 5; ++step) c.update_register();
        c.branch(sprite < 127);
    }
    c.load_constant();
    c.load_constant(2);
    c.load_constant(2);
    for (unsigned packed = 0; packed < 32; ++packed) {
        c.store_ram(1, false, true);
        c.update_register();
        c.update_register();
        c.branch(packed < 31);
    }
}
// $80:D30B; R-0075. Fill the eight menu sprite and choice-table entries.
void fill_menu_choice_work(Clock& c) {
    c.load_constant(2);
    c.load_constant(2);
    for (unsigned choice = 0; choice < 8; ++choice) {
        c.load_constant();
        c.store_ram(1, false, true);
        for (unsigned field = 0; field < 2; ++field) {
            c.save_register(2);
            c.update_register();
            c.read_rom(1, false, true);
            c.restore_register(2);
            c.store_ram(1, false, true);
        }
        for (unsigned step = 0; step < 5; ++step) c.update_register();
        c.branch(choice < 7);
    }
    c.store_ram();
    c.store_ram();
    c.load_constant(2);
    for (unsigned choice = 0; choice < 8; ++choice) {
        c.read_rom(1, false, true);
        c.store_ram(1, false, true);
        c.update_register();
        c.branch(choice < 7);
    }
    c.load_constant();
    c.store_direct();
    for (unsigned group = 0; group < 6; ++group) {
        c.load_constant();
        c.store_ram();
        if (group >= 2) c.store_ram();
    }
}
} // namespace
// $80:D2AD; R-0075. Static bank-80 D2AD-D36F, ending before the first layout wait.
void native_audio_menu_oam(Clock& c, AudioCpuSceneWorkState& scene) {
    c.change_widths();
    c.load_constant(2);
    c.store_ram(2);
    scene.horizontal_current = 0;
    c.load_constant(2);
    c.store_ram(2);
    scene.vertical_current = 0x0f00;
    c.store_ram();
    c.store_ram();
    reset_menu_sprite_work(c);
    for (unsigned group = 0; group < 4; ++group) {
        c.load_constant();
        c.store_ram();
        if (group < 2) c.store_ram();
    }
    c.call_far();
    native_audio_park_arrow(c, scene);
    fill_menu_choice_work(c);
    scene.first_horizontal_flags = scene.second_horizontal_flags = 0xd5;
}
} // namespace unirally
