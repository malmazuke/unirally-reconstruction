#pragma once
#include "audio_cpu_boot.hpp"
#include "audio_cpu_queue.hpp"

namespace unirally {
// Reduced frontend state read by FAF5. Word differences wrap before the
// arithmetic divide by four. Positions are original fixed-point words.
struct AudioCpuSceneWorkState {
    std::uint8_t phase = 0, first_horizontal_flags = 0, second_horizontal_flags = 0;
    // Fresh 8 KiB cartridge RAM is erased FF; title clears $77:10D0 after
    // restoring the sixteen saved tour-level bytes. R-0075.
    std::uint8_t title_levels_pending = 255;
    std::uint16_t horizontal_current = 0, horizontal_target = 0;
    std::uint16_t vertical_current = 0, vertical_target = 0;
    bool operator==(const AudioCpuSceneWorkState&) const = default;
};
void native_audio_finish_waited_frame(AudioCpuWorkClock& clock, AudioCpuSceneWorkState& state);
void native_audio_title_fade(AudioCpuWorkClock& clock, AudioCpuQueueState& queue,
                             AudioCpuSceneWorkState& scene, bool darken);
void native_audio_frame_wait(AudioCpuWorkClock& clock, AudioCpuQueueState& queue,
                             AudioCpuSceneWorkState& scene);
void native_audio_upload_base_palette(AudioCpuWorkClock& clock, AudioCpuQueueState& queue,
                                      AudioCpuSceneWorkState& scene);
void native_audio_load_nintendo_graphics(AudioCpuWorkClock& clock, AudioCpuSceneWorkState& scene,
                                         const AudioCpuGraphicsAsset& palette,
                                         const AudioCpuGraphicsAsset& map,
                                         const AudioCpuGraphicsAsset& tiles);
void native_audio_finish_nintendo_screen(AudioCpuWorkClock& clock, AudioCpuQueueState& queue,
                                         AudioCpuSceneWorkState& scene);
void native_audio_load_title_graphics(AudioCpuWorkClock& clock, AudioCpuSceneWorkState& scene,
                                      const AudioCpuGraphicsAsset& palette,
                                      const AudioCpuGraphicsAsset& map,
                                      const AudioCpuGraphicsAsset& tiles);
} // namespace unirally
