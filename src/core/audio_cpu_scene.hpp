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
void native_audio_build_pose_work(AudioCpuWorkClock& clock, std::uint16_t pose,
                                  std::span<const std::uint8_t> pointers,
                                  std::span<const std::uint8_t> frames);
void native_audio_upload_pose_work(AudioCpuWorkClock& clock, bool lower_buffer = false);
struct AudioCpuTitleHoldState {
    std::uint16_t remaining = 110;
    std::uint8_t code_index = 0;
    std::array<std::uint16_t, 2> controllers{};
    bool operator==(const AudioCpuTitleHoldState&) const = default;
};
// $83:9A1E animator counters. Only counters influence this native work clock;
// tile/position bytes belong to the separately recovered presentation.
struct AudioCpuDecorationWorkState {
    std::uint8_t delay = 0, pair_step = 0, pair_cycle = 0, trio_step = 0;
    std::uint8_t wave_delay = 1, sway = 0;
    std::array<std::uint8_t, 8> wave{0, 3, 5, 8, 10, 13, 15, 18};
    bool operator==(const AudioCpuDecorationWorkState&) const = default;
};
struct AudioCpuHunterWorkState {
    AudioCpuDecorationWorkState decorations;
    std::array<std::uint16_t, 2> controllers{};
    bool wait_started = false, first_press = false, timed_started = false;
    std::uint16_t timed_remaining = 1200;
    std::uint16_t reveal_remaining = 74;
    std::uint16_t credits_frame = 0;
    bool operator==(const AudioCpuHunterWorkState&) const = default;
};
void native_audio_begin_hunter_code(AudioCpuWorkClock& clock, AudioCpuSceneWorkState& scene,
                                    AudioCpuInterruptWorkState& interrupt,
                                    std::array<std::uint8_t, 8192>& cartridge);
bool native_audio_hunter_code_frame(AudioCpuWorkClock& clock, AudioCpuQueueState& queue,
                                    AudioCpuSceneWorkState& scene, std::uint16_t& remaining);
void native_audio_hunter_first_fade(AudioCpuWorkClock& clock, AudioCpuQueueState& queue);
void native_audio_hunter_fade_down(AudioCpuWorkClock& clock, AudioCpuQueueState& queue);
void native_audio_hunter_ending_wait(AudioCpuWorkClock& clock, AudioCpuQueueState& queue);
bool native_audio_ending_press(AudioCpuWorkClock& clock,
                               const std::array<std::uint16_t, 2>& controllers,
                               const std::array<std::uint8_t, 8192>& cartridge);
void native_audio_hunter_finish_credits_setup(AudioCpuWorkClock& clock, AudioCpuQueueState& queue);
bool native_audio_hunter_credits_frame(AudioCpuWorkClock& clock, AudioCpuQueueState& queue,
                                       AudioCpuHunterWorkState& state,
                                       const std::array<std::uint8_t, 8192>& cartridge,
                                       std::span<const std::uint8_t, 96> poses,
                                       std::span<const std::uint8_t> pointers,
                                       std::span<const std::uint8_t> frames);
bool native_audio_hunter_page_wait_frame(AudioCpuWorkClock& clock, AudioCpuQueueState& queue,
                                         AudioCpuSceneWorkState& scene,
                                         AudioCpuHunterWorkState& state);
void native_audio_hunter_begin_second_page(AudioCpuWorkClock& clock, AudioCpuQueueState& queue,
                                           AudioCpuSceneWorkState& scene,
                                           const std::array<AudioCpuGraphicsAsset, 128>& graphics,
                                           std::span<const std::uint8_t, 72> offsets,
                                           std::span<const std::uint8_t, 38> brightness);
bool native_audio_hunter_timed_frame(AudioCpuWorkClock& clock, AudioCpuQueueState& queue,
                                     AudioCpuHunterWorkState& state,
                                     const std::array<std::uint8_t, 8192>& cartridge);
void native_audio_step_decorations(AudioCpuWorkClock& clock, AudioCpuDecorationWorkState& state);
std::array<std::uint64_t, 4> native_audio_hunter_prepare_credits(
    AudioCpuWorkClock& clock, AudioCpuQueueState& queue, AudioCpuInterruptWorkState& interrupt,
    const std::array<AudioCpuGraphicsAsset, 128>& graphics, std::span<const std::uint8_t, 17> text,
    std::span<const std::uint8_t, 256> characters);
void native_audio_print_credits(AudioCpuWorkClock& clock, std::span<const std::uint8_t, 17> text,
                                std::span<const std::uint8_t, 256> characters);
void native_audio_hunter_begin_first_page(AudioCpuWorkClock& clock, AudioCpuQueueState& queue,
                                          AudioCpuSceneWorkState& scene,
                                          const std::array<AudioCpuGraphicsAsset, 128>& graphics,
                                          std::span<const std::uint8_t, 72> offsets,
                                          std::span<const std::uint8_t, 38> brightness);
bool native_audio_hunter_reveal_frame(AudioCpuWorkClock& clock, AudioCpuQueueState& queue,
                                      AudioCpuSceneWorkState& scene, std::uint16_t& remaining);
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
