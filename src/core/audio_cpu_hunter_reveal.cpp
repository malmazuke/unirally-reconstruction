#include "audio_cpu_scene.hpp"
#include <stdexcept>

namespace unirally {
namespace {
using Clock = AudioCpuWorkClock;
// $80:B139; preserve word accumulator around the two queued commands.
void reveal_cue(Clock& c, AudioCpuQueueState& queue) {
    c.save_register();
    c.change_widths();
    c.save_register(2);
    c.load_constant(2);
    c.call_far();
    native_audio_enqueue(c, queue, 8, 79);
    c.load_constant(2);
    c.call_far();
    native_audio_enqueue(c, queue, 2, 2);
    c.restore_register(2);
    c.restore_register();
    c.return_local();
}
// $80:E2D8-E2F6; indexed byte copies, with X/Y and BPL work retained.
void copy_reveal_work(Clock& c, unsigned bytes) {
    c.load_constant(2);
    c.load_constant(2);
    for (unsigned i = 0; i < bytes; ++i) {
        c.read_rom(1, false, true);
        c.store_ram(1, false, true);
        c.update_register();
        c.update_register();
        c.branch(i + 1 < bytes);
    }
}
// $80:E2F8-E31F; two direct channels, fixed destination modes 2 and 0.
void configure_reveal_channels(Clock& c) {
    for (unsigned channel = 0; channel < 2; ++channel) {
        for (const auto bytes : {1U, 1U, 2U, 1U}) {
            c.load_constant(bytes);
            c.store_port(bytes);
        }
    }
}
// $80:E330-E367; alternate the first/second table header, wrapping at 8 bits.
void reveal_frame(Clock& c, AudioCpuQueueState& queue, AudioCpuSceneWorkState& scene,
                  unsigned remaining) {
    c.save_register(2);
    c.save_register(2);
    c.call_local();
    native_audio_frame_wait(c, queue, scene);
    c.call_local();
    native_audio_upload_oam(c);
    c.update_register();
    c.load_constant();
    const bool even = (remaining & 1) == 0;
    c.branch(even);
    for (const auto offset : {even ? 0U : 3U, even ? 72U : 74U}) {
        c.read_ram();
        c.update_register();
        c.load_constant();
        c.store_ram();
        c.increment_reveal_header(offset);
    }
    if (!even) c.branch(true);
    c.restore_register(2);
    c.restore_register(2);
    c.update_register();
    c.update_register();
    c.branch(remaining != 0);
}
// $80:E2CF-E37A; R-0075. Native data tables and CPU/HDMA work derive all timing.
void reveal_page(Clock& c, AudioCpuQueueState& queue, AudioCpuSceneWorkState& scene,
                 std::span<const std::uint8_t, 72> offsets,
                 std::span<const std::uint8_t, 38> brightness) {
    c.call_local();
    c.change_widths();
    c.call_local();
    reveal_cue(c, queue);
    copy_reveal_work(c, 72);
    copy_reveal_work(c, 38);
    c.copy_reveal_tables(offsets, brightness);
    configure_reveal_channels(c);
    c.call_local();
    native_audio_frame_wait(c, queue, scene);
    c.load_constant();
    c.set_reveal_hdma_enabled(true);
    c.load_constant(2);
    c.load_constant(2);
}
} // namespace
// $80:E330-E37A; R-0075. A call boundary between reveal frames retains
// active HDMA work. The last frame follows the original final blank wait.
bool native_audio_hunter_reveal_frame(Clock& c, AudioCpuQueueState& queue,
                                      AudioCpuSceneWorkState& scene, std::uint16_t& remaining) {
    if (remaining > 74) throw std::invalid_argument("invalid reveal frame count");
    reveal_frame(c, queue, scene, remaining);
    const bool more = remaining != 0;
    remaining = static_cast<std::uint16_t>(remaining - 1U);
    if (more) return true;
    c.call_local();
    native_audio_frame_wait(c, queue, scene);
    c.set_reveal_hdma_enabled(false);
    c.load_constant();
    c.store_port();
    c.store_port();
    c.store_port();
    c.return_local();
    c.return_far();
    return false;
}
// $83:AB9F-ABF9; normal first page. Static listing is the reading source;
// cold-cpu-hunter-first-page-ready-frozen.json supplies independent endpoints.
void native_audio_hunter_begin_first_page(Clock& c, AudioCpuQueueState& queue,
                                          AudioCpuSceneWorkState& scene,
                                          const std::array<AudioCpuGraphicsAsset, 128>& graphics,
                                          std::span<const std::uint8_t, 72> offsets,
                                          std::span<const std::uint8_t, 38> brightness) {
    for (unsigned i = 0; i < 4; ++i) {
        c.load_constant();
        c.store_port();
    }
    c.read_ram(1, true);
    c.branch(true);
    for (const auto id : {103U, 110U, 107U}) {
        c.load_constant(2);
        c.load_constant();
        c.call_far();
        native_audio_cpu_upload_graphics_asset(c, graphics[id], id == 103);
    }
    c.call_far();
    reveal_page(c, queue, scene, offsets, brightness);
}
// $83:ABFD-AC1F; second newspaper page, following the two sampled presses.
void native_audio_hunter_begin_second_page(Clock& c, AudioCpuQueueState& queue,
                                           AudioCpuSceneWorkState& scene,
                                           const std::array<AudioCpuGraphicsAsset, 128>& graphics,
                                           std::span<const std::uint8_t, 72> offsets,
                                           std::span<const std::uint8_t, 38> brightness) {
    c.call_local();
    native_audio_hunter_fade_down(c, queue);
    for (const auto id : {104U, 111U, 108U}) {
        c.load_constant(2);
        c.load_constant();
        c.call_far();
        native_audio_cpu_upload_graphics_asset(c, graphics[id], id == 104);
    }
    c.call_far();
    reveal_page(c, queue, scene, offsets, brightness);
}
} // namespace unirally
