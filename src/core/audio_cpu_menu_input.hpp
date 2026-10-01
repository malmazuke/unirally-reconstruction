#pragma once
#include "audio_cpu_scene.hpp"

namespace unirally {
enum class AudioCpuMenuAction : std::uint8_t { waiting, selected, attract, wipe_ram, hunter };
struct AudioCpuMenuInputState {
    std::uint16_t idle_remaining = 480;
    std::uint8_t selection = 0;
    bool direction_latched = false;
    std::array<std::uint16_t, 2> controllers{};
    bool operator==(const AudioCpuMenuInputState&) const = default;
};
void native_audio_begin_menu_input(AudioCpuWorkClock& clock, AudioCpuSceneWorkState& scene,
                                   std::array<std::uint8_t, 8192>& cartridge,
                                   AudioCpuMenuInputState& state);
AudioCpuMenuAction native_audio_menu_input_frame(AudioCpuWorkClock& clock,
                                                 AudioCpuQueueState& queue,
                                                 AudioCpuSceneWorkState& scene,
                                                 std::array<std::uint8_t, 8192>& cartridge,
                                                 AudioCpuMenuInputState& state,
                                                 std::span<const std::uint8_t, 5> arrow_positions);
} // namespace unirally
