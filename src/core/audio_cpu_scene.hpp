#pragma once
#include "audio_cpu_boot.hpp"
#include "audio_cpu_interrupt.hpp"
#include "audio_cpu_queue.hpp"
#include <span>

namespace unirally {
// Reduced frontend state read by FAF5. Word differences wrap before the
// arithmetic divide by four. Positions are original fixed-point words.
struct AudioCpuSceneWorkState {
    std::uint8_t phase = 0, first_horizontal_flags = 0, second_horizontal_flags = 0;
    // Fresh 8 KiB cartridge RAM is erased FF; title clears $77:10D0 after
    // restoring the sixteen saved tour-level bytes. R-0075.
    std::uint8_t title_levels_pending = 255;
    std::uint8_t menu_mode = 255;
    std::uint16_t horizontal_current = 0, horizontal_target = 0;
    std::uint16_t vertical_current = 0, vertical_target = 0;
    bool operator==(const AudioCpuSceneWorkState&) const = default;
};
struct AudioCpuTitleHoldState {
    std::uint16_t remaining = 110;
    std::uint8_t code_index = 0;
    std::array<std::uint16_t, 2> controllers{};
    bool operator==(const AudioCpuTitleHoldState&) const = default;
};
void native_audio_begin_hunter_code(AudioCpuWorkClock& clock, AudioCpuSceneWorkState& scene,
                                    AudioCpuInterruptWorkState& interrupt,
                                    std::array<std::uint8_t, 8192>& cartridge);
bool native_audio_hunter_code_frame(AudioCpuWorkClock& clock, AudioCpuQueueState& queue,
                                    AudioCpuSceneWorkState& scene, std::uint16_t& remaining);
void native_audio_hunter_first_fade(AudioCpuWorkClock& clock, AudioCpuQueueState& queue);
void native_audio_begin_title_hold(AudioCpuWorkClock& clock);
bool native_audio_title_hold_frame(AudioCpuWorkClock& clock, AudioCpuQueueState& queue,
                                   AudioCpuSceneWorkState& scene, AudioCpuTitleHoldState& state);
void native_audio_clear_menu_text_work(AudioCpuWorkClock& clock);
void native_audio_finish_menu_records(AudioCpuWorkClock& clock, AudioCpuSceneWorkState& scene,
                                      std::array<std::uint8_t, 8192>& cartridge,
                                      std::span<const std::uint8_t, 1158> defaults);
void native_audio_reset_menu_selection(AudioCpuWorkClock& clock, AudioCpuSceneWorkState& scene,
                                       std::array<std::uint8_t, 8192>& cartridge);
void native_audio_menu_league_defaults(AudioCpuWorkClock& clock,
                                       std::array<std::uint8_t, 8192>& cartridge);
void native_audio_menu_first_record_defaults(AudioCpuWorkClock& clock,
                                             std::array<std::uint8_t, 8192>& cartridge,
                                             std::span<const std::uint8_t, 1158> defaults,
                                             std::span<const std::uint8_t, 50> track_types);
bool native_audio_begin_menu_records(AudioCpuWorkClock& clock, AudioCpuQueueState& queue,
                                     AudioCpuSceneWorkState& scene,
                                     std::array<std::uint8_t, 8192>& cartridge,
                                     std::span<const std::uint8_t, 12> signature);
void native_audio_park_arrow(AudioCpuWorkClock& clock, AudioCpuSceneWorkState& scene);
void native_audio_menu_oam(AudioCpuWorkClock& clock, AudioCpuSceneWorkState& scene);
void native_audio_upload_oam(AudioCpuWorkClock& clock);
std::array<std::uint16_t, 2> native_audio_poll_controllers(AudioCpuWorkClock& clock);
void native_audio_finish_waited_frame(AudioCpuWorkClock& clock, AudioCpuSceneWorkState& state);
void native_audio_title_fade(AudioCpuWorkClock& clock, AudioCpuQueueState& queue,
                             AudioCpuSceneWorkState& scene, bool darken);
void native_audio_frame_wait(AudioCpuWorkClock& clock, AudioCpuQueueState& queue,
                             AudioCpuSceneWorkState& scene);
void native_audio_palette_dma_work(AudioCpuWorkClock& clock, AudioCpuQueueState& queue,
                                   AudioCpuSceneWorkState& scene);
void native_audio_menu_first_palette(AudioCpuWorkClock& clock, AudioCpuQueueState& queue,
                                     AudioCpuSceneWorkState& scene,
                                     AudioCpuInterruptWorkState& interrupt,
                                     const AudioCpuGraphicsAsset& palette);
void native_audio_menu_graphics(AudioCpuWorkClock& clock, const AudioCpuSceneWorkState& scene,
                                const std::array<AudioCpuGraphicsAsset, 128>& assets);
void native_audio_upload_base_palette(AudioCpuWorkClock& clock, AudioCpuQueueState& queue,
                                      AudioCpuSceneWorkState& scene);
void native_audio_load_nintendo_graphics(AudioCpuWorkClock& clock, AudioCpuSceneWorkState& scene,
                                         const AudioCpuGraphicsAsset& palette,
                                         const AudioCpuGraphicsAsset& map,
                                         const AudioCpuGraphicsAsset& tiles);
void native_audio_finish_nintendo_screen(AudioCpuWorkClock& clock, AudioCpuQueueState& queue,
                                         AudioCpuSceneWorkState& scene);
void native_audio_initialize_title_work(AudioCpuWorkClock& clock, AudioCpuSceneWorkState& scene,
                                        AudioCpuInterruptWorkState& interrupt);
void native_audio_load_title_graphics(AudioCpuWorkClock& clock, AudioCpuSceneWorkState& scene,
                                      AudioCpuInterruptWorkState& interrupt,
                                      const AudioCpuGraphicsAsset& palette,
                                      const AudioCpuGraphicsAsset& map,
                                      const AudioCpuGraphicsAsset& tiles);
} // namespace unirally
