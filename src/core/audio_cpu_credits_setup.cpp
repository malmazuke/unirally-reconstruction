#include "audio_cpu_scene.hpp"

namespace unirally {
namespace {
using Clock = AudioCpuWorkClock;
void credits_graphics(Clock& c, const std::array<AudioCpuGraphicsAsset, 128>& graphics) {
    c.change_widths();
    for (const auto id : {0U, 1U, 89U, 91U, 77U, 70U, 68U, 69U}) {
        c.load_constant(2);
        c.load_constant(2);
        c.call_far();
        native_audio_cpu_upload_graphics_asset(c, graphics[id], id == 0 || id == 1);
    }
    c.change_widths();
    c.load_constant(2);
    c.load_constant();
    c.call_far();
    native_audio_cpu_upload_graphics_asset(c, graphics[93], false);
    for (const auto id : {6U, 10U, 9U, 8U, 7U, 11U, 14U, 58U}) {
        c.load_constant(2);
        c.load_constant();
        c.call_far();
        native_audio_cpu_upload_graphics_asset(c, graphics[id], true);
    }
}
void upload_credits_text(Clock& c) {
    c.call_local();
    c.read_direct(2);
    c.store_port(2);
    for (const auto bytes : {2U, 1U, 2U, 1U, 1U, 1U}) {
        c.load_constant(bytes);
        c.store_port(bytes);
    }
    c.request_dma(2048);
    c.return_local();
    c.return_far();
}
void credits_objects(Clock& c) {
    c.load_constant(2);
    c.load_constant(2);
    for (unsigned i = 0; i < 132; ++i) {
        c.read_rom(1, true, true);
        c.store_ram(1, false, true);
        c.update_register();
        c.update_register();
        c.branch(i < 131);
    }
    c.load_constant();
    for (unsigned i = 0; i < 8; ++i) c.store_ram();
    c.load_constant();
    c.store_ram();
    c.call_far();
    c.call_local();
    native_audio_upload_oam(c);
    c.return_far();
}
} // namespace
// $83:AC32-AD67; R-0075 static listing, frozen credits-pose-ready endpoint.
std::array<std::uint64_t, 4> native_audio_hunter_prepare_credits(
    Clock& c, AudioCpuQueueState& queue, AudioCpuInterruptWorkState& interrupt,
    const std::array<AudioCpuGraphicsAsset, 128>& graphics, std::span<const std::uint8_t, 17> text,
    std::span<const std::uint8_t, 256> characters) {
    c.call_local();
    native_audio_hunter_fade_down(c, queue);
    c.load_constant();
    c.call_far();
    native_audio_clear_menu_text_work(c);
    for (unsigned i = 0; i < 10; ++i) {
        c.load_constant();
        c.store_port();
    }
    std::array<std::uint64_t, 4> marks{};
    marks[0] = c.ticks();
    c.load_constant();
    c.store_direct();
    interrupt.scroll = 79;
    c.store_port();
    for (unsigned i = 0; i < 4; ++i) c.store_port();
    credits_graphics(c, graphics);
    marks[1] = c.ticks();
    c.read_direct(2);
    c.store_direct(2);
    c.load_constant(2);
    c.call_local();
    native_audio_print_credits(c, text, characters);
    c.call_far();
    upload_credits_text(c);
    marks[2] = c.ticks();
    credits_objects(c);
    marks[3] = c.ticks();
    return marks;
}
} // namespace unirally
