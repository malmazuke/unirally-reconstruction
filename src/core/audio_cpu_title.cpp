#include "audio_cpu_scene.hpp"
#include <array>

namespace unirally {
namespace {
using Clock = AudioCpuWorkClock;
// $80:A16A; R-0075. Static bank-80 A16A-A1F1; R-0054/R-0075. Installs the title NMI hook
// and initializes native frontend target words and cleared work variables.
void initialize_title_work(Clock& c, AudioCpuSceneWorkState& scene) {
    for (unsigned i = 0; i < 2; ++i) {
        c.load_constant(2);
        c.store_direct(2);
    }
    c.load_constant(2);
    c.load_constant();
    c.store_direct(2);
    c.store_direct();
    for (unsigned i = 0; i < 2; ++i) {
        c.load_constant(2);
        c.store_direct(2);
    }
    for (unsigned i = 0; i < 2; ++i) {
        c.load_constant();
        c.store_ram();
    }
    for (unsigned i = 0; i < 2; ++i) {
        c.load_constant(2);
        c.store_ram(2);
    }
    scene.horizontal_target = 0x04fd;
    scene.vertical_target = 0x0580;
    c.load_constant();
    c.store_direct();
    for (unsigned i = 0; i < 2; ++i) {
        c.load_constant(2);
        c.store_direct(2);
    }
    c.load_constant();
    c.store_direct();
    c.load_constant(2);
    c.store_direct(2);
    c.load_constant(2);
    constexpr std::array<unsigned, 29> clear_widths{2, 1, 2, 1, 2, 1, 2, 2, 1, 2, 2, 2, 2, 2, 2,
                                                    2, 1, 2, 2, 2, 1, 2, 2, 1, 1, 2, 1, 1, 1};
    for (const auto width : clear_widths) c.store_direct(width);
    c.return_local();
}
}
// $80:F55F; R-0075. Static bank-80 8864-8867/F55F-F5B8. Cartridge state selects the tour-level copy.
// Ends before the write enabling NMI; interrupt handling is a later domain.
void native_audio_load_title_graphics(AudioCpuWorkClock& c, AudioCpuSceneWorkState& scene,
                                      const AudioCpuGraphicsAsset& palette,
                                      const AudioCpuGraphicsAsset& map,
                                      const AudioCpuGraphicsAsset& tiles) {
    c.call_local();
    initialize_title_work(c, scene);
    c.call_local();
    c.load_constant();
    c.store_port();
    c.rom_reads(4);
    c.ram_reads();
    const bool restore_levels = scene.title_levels_pending != 0;
    c.branch(!restore_levels);
    if (restore_levels) {
        c.change_widths();
        c.load_constant(2);
        c.load_constant(2);
        for (unsigned i = 0; i < 16; ++i) {
            c.rom_reads(4);
            c.ram_reads();
            c.store_ram(1, true, true);
            c.update_register();
            c.update_register();
            c.branch(i < 15);
        }
        c.load_constant();
        c.store_ram(1, true);
        scene.title_levels_pending = 0;
    }
    for (unsigned i = 0; i < 5; ++i) {
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
}
} // namespace unirally
