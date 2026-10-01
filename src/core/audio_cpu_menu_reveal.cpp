#include "audio_cpu_text.hpp"
#include <stdexcept>

namespace unirally {
namespace {
using Clock = AudioCpuWorkClock;
using Cartridge = std::array<std::uint8_t, 8192>;
// $80:937B; R-0075. Upload the complete 2048-byte native text work map.
void upload_text_map(Clock& c) {
    c.read_direct(2);
    c.store_port(2);
    constexpr std::array<unsigned, 6> widths{2, 1, 2, 1, 1, 1};
    for (const auto width : widths) {
        c.load_constant(width);
        c.store_port(width);
    }
    c.request_dma(2048);
    c.return_local();
}
// $80:A877; R-0075. Prepare the 1920-byte font copy, wait and upload OAM before this DMA.
void upload_menu_font(Clock& c, AudioCpuQueueState& queue, AudioCpuSceneWorkState& scene) {
    constexpr std::array<unsigned, 6> widths{2, 2, 1, 2, 1, 1};
    for (const auto width : widths) {
        c.load_constant(width);
        c.store_port(width);
    }
    c.call_local();
    native_audio_frame_wait(c, queue, scene);
    c.call_local();
    native_audio_upload_oam(c);
    c.load_constant();
    c.store_port();
    c.request_dma(1920);
    c.return_local();
}
// $80:F52B; $80:AD0F; R-0075. Clear the league-naming bit and finish common palette/font work.
void finish_menu_uploads(Clock& c, AudioCpuQueueState& queue, AudioCpuSceneWorkState& scene,
                         Cartridge& cartridge) {
    c.call_local();
    c.save_register();
    c.change_widths();
    c.read_ram(2, true);
    c.load_constant(2);
    c.store_ram(2, true);
    cartridge[0x0742] &= 0xfd;
    c.restore_register();
    c.return_local();
    c.call_local();
    native_audio_palette_dma_work(c, queue, scene);
    c.call_local();
    upload_menu_font(c, queue, scene);
    c.load_constant();
    c.store_ram(1, true);
    cartridge[0x10ad] = scene.menu_mode = 0;
    c.return_local();
}
// $80:ACF7; R-0075. Upload the prepared labels, restore the saved base and mark them drawn.
void finish_initial_text(Clock& c, AudioCpuQueueState& queue, AudioCpuSceneWorkState& scene,
                         AudioCpuTextWorkState& text, Cartridge& cartridge) {
    c.call_local();
    native_audio_frame_wait(c, queue, scene);
    c.call_local();
    upload_text_map(c);
    c.restore_register(2);
    c.store_direct(2);
    c.load_constant();
    c.store_direct();
    text.drawn = true;
    c.store_direct();
    c.read_direct();
    c.branch(true);
    finish_menu_uploads(c, queue, scene, cartridge);
    text.prepared = false;
}
} // namespace
// $80:887B; $80:ACD5; R-0075. Fade the first menu in, then refresh its already drawn work.
// Ends before the first interactive menu call at889A. The cold intro bit is clear.
void native_audio_reveal_main_menu(Clock& c, AudioCpuQueueState& queue,
                                   AudioCpuSceneWorkState& scene, AudioCpuTextWorkState& text,
                                   Cartridge& cartridge) {
    if (!text.prepared || text.drawn) throw std::invalid_argument("menu labels are not prepared");
    finish_initial_text(c, queue, scene, text, cartridge);
    c.call_local();
    native_audio_title_fade(c, queue, scene, false);
    c.call_local();
    native_audio_frame_wait(c, queue, scene);
    c.call_local();
    c.change_widths();
    c.read_direct();
    c.branch(true);
    c.read_direct();
    c.branch(true);
    finish_menu_uploads(c, queue, scene, cartridge);
    c.load_constant(2);
    c.store_direct(2);
    c.read_ram(1, true);
    c.load_constant();
    const bool show_intro = (cartridge[0x0742] & 16) != 0;
    c.branch(!show_intro);
    if (show_intro) throw std::invalid_argument("menu intro outside recovered cold domain");
}
} // namespace unirally
