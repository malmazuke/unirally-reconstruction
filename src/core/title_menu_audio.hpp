#pragma once
#include "audio_cpu_menu_input.hpp"
#include "audio_cpu_text.hpp"
#include "audio_cpu_upload.hpp"
#include "audio_engine.hpp"

namespace unirally {
class ClassicContentPack;
struct TitleMenuAudioContent {
    std::array<std::uint8_t, 32> identity{};
    TitleMenuAudioData score;
    AudioPitchData pitch;
    AudioCpuUploadData upload;
    std::array<AudioCpuGraphicsAsset, 128> graphics{};
    std::array<std::uint8_t, 1158> cartridge_defaults{};
    std::array<std::uint8_t, 50> track_types{};
    std::array<std::uint8_t, 12> cartridge_signature{};
    std::vector<std::uint8_t> menu_text;
    std::array<std::uint8_t, 256> characters{};
    std::array<std::uint8_t, 5> arrow_positions{};
};
enum class TitleMenuAudioPhase : std::uint8_t { cold, title_hold, title_complete, menu, menu_exit };
struct TitleMenuAudioState {
    TitleMenuAudioPhase phase = TitleMenuAudioPhase::cold;
    std::array<std::uint8_t, 32> content_identity{};
    AudioEngineState engine;
    AudioCpuQueueState queue;
    AudioCpuSceneWorkState scene;
    AudioCpuTitleHoldState title;
    AudioCpuTextWorkState text;
    AudioCpuMenuInputState menu;
    std::array<std::uint8_t, 8192> cartridge{};
};
// Semantic call boundaries own all continuation state. Controller words are
// external future input; identified content stays immutable outside the state.
class NativeTitleMenuAudio {
public:
    NativeTitleMenuAudio(const TitleMenuAudioContent& content, AudioControllerSource& controllers,
                         AudioEngineEventSink* events = nullptr);
    void initialize_title();
    bool title_frame();
    void reveal_menu();
    AudioCpuMenuAction menu_frame();
    std::uint64_t cpu_ticks() { return engine_.cpu().ticks(); }
    TitleMenuAudioPhase phase() const { return phase_; }
    TitleMenuAudioState snapshot();
    void restore(const TitleMenuAudioState& state);
    std::vector<std::int16_t> take_pcm() { return engine_.take_pcm(); }
    void finish_pcm_to(std::uint64_t clocks) { engine_.finish_pcm_to(clocks); }

private:
    const TitleMenuAudioContent* content_;
    NativeAudioEngine engine_;
    TitleMenuAudioPhase phase_ = TitleMenuAudioPhase::cold;
    AudioCpuQueueState queue_;
    AudioCpuSceneWorkState scene_;
    AudioCpuTitleHoldState title_;
    AudioCpuTextWorkState text_;
    AudioCpuMenuInputState menu_;
    std::array<std::uint8_t, 8192> cartridge_;
};
TitleMenuAudioContent title_menu_audio_content(const ClassicContentPack& pack);
std::vector<std::uint8_t> serialize_title_menu_audio(const TitleMenuAudioState& state);
TitleMenuAudioState deserialize_title_menu_audio(std::span<const std::uint8_t> bytes);
} // namespace unirally
