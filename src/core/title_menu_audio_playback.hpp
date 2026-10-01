#pragma once
#include "audio_output.hpp"
#include "title_menu_audio.hpp"
#include <mutex>

namespace unirally {
struct TitleMenuAudioPlaybackState {
    TitleMenuAudioState native;
    AudioOutputState output;
};
// One call-boundary owner for native production and modern output. Device
// callbacks consume pairs outside this owner; the OS mixer is not serialized.
class NativeTitleMenuAudioPlayback final : private AudioPcmSink {
public:
    NativeTitleMenuAudioPlayback(const TitleMenuAudioContent& content,
                                 AudioControllerSource& controllers, std::uint32_t output_rate,
                                 AudioEngineEventSink* events = nullptr);
    NativeTitleMenuAudio& native() { return native_; }
    std::vector<std::int16_t> take_pairs(std::size_t maximum);
    std::uint64_t source_pairs();
    std::uint64_t delivered_pairs();
    TitleMenuAudioPlaybackState snapshot();
    void restore(const TitleMenuAudioPlaybackState& state);

private:
    const TitleMenuAudioContent* content_;
    AudioControllerSource* controllers_;
    NativeAudioOutput output_;
    NativeTitleMenuAudio native_;
    std::mutex output_mutex_;
    void append_pcm(std::span<const std::int16_t> samples) override;
};
std::vector<std::uint8_t>
serialize_title_menu_audio_playback(const TitleMenuAudioPlaybackState& state);
TitleMenuAudioPlaybackState
deserialize_title_menu_audio_playback(std::span<const std::uint8_t> bytes);
} // namespace unirally
