#pragma once
#include "audio_cpu_scene.hpp"

namespace unirally {
// Native work owned across menu printing/upload phases; cursor uses tile units.
struct AudioCpuTextWorkState {
    std::uint16_t cursor = 0, attribute = 0;
    bool prepared = false, drawn = false;
    bool operator==(const AudioCpuTextWorkState&) const = default;
};
void native_audio_reveal_main_menu(AudioCpuWorkClock& clock, AudioCpuQueueState& queue,
                                   AudioCpuSceneWorkState& scene, AudioCpuTextWorkState& text,
                                   std::array<std::uint8_t, 8192>& cartridge);
void native_audio_begin_menu_text(AudioCpuWorkClock& clock, AudioCpuTextWorkState& state,
                                  std::span<const std::uint8_t> text,
                                  std::span<const std::uint8_t, 256> characters);
} // namespace unirally
