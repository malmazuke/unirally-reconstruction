#include "title_menu_audio_playback.hpp"
#include "audio_state_archive.hpp"

namespace unirally {
namespace {
void validate_ownership(const TitleMenuAudioPlaybackState& state) {
    if (!state.native.engine.pending_pcm.empty())
        throw std::invalid_argument("playback PCM must be owned by the output queue");
    if (state.output.source_pairs != state.native.engine.dsp.clocks / 32)
        throw std::invalid_argument("native DSP and output source counts differ");
}
}
NativeTitleMenuAudioPlayback::NativeTitleMenuAudioPlayback(const TitleMenuAudioContent& content,
                                                           AudioControllerSource& controllers,
                                                           std::uint32_t output_rate,
                                                           AudioEngineEventSink* events)
    : content_(&content),
      controllers_(&controllers),
      output_(output_rate),
      native_(content, controllers, events) {
    native_.set_pcm_sink(this);
}
TitleMenuAudioPlaybackState NativeTitleMenuAudioPlayback::snapshot() {
    // Native snapshot collects any unconsumed DSP output into the attached queue.
    auto native = native_.snapshot();
    std::lock_guard lock(output_mutex_);
    return {std::move(native), output_.state()};
}
void NativeTitleMenuAudioPlayback::restore(const TitleMenuAudioPlaybackState& state) {
    validate_ownership(state);
    NativeAudioOutput candidate_output;
    candidate_output.restore(state.output);
    NativeTitleMenuAudio candidate_native(*content_, *controllers_);
    candidate_native.restore(state.native);
    // Both owners validate before either running owner changes. Moving the
    // output preserves the stable sink object's address retained by native_.
    native_.restore(state.native);
    std::lock_guard lock(output_mutex_);
    output_ = std::move(candidate_output);
}
void NativeTitleMenuAudioPlayback::append_pcm(std::span<const std::int16_t> samples) {
    std::lock_guard lock(output_mutex_);
    output_.append_pcm(samples);
}
std::vector<std::int16_t> NativeTitleMenuAudioPlayback::take_pairs(std::size_t maximum) {
    std::lock_guard lock(output_mutex_);
    return output_.take_pairs(maximum);
}
std::uint64_t NativeTitleMenuAudioPlayback::source_pairs() {
    std::lock_guard lock(output_mutex_);
    return output_.state().source_pairs;
}
std::uint64_t NativeTitleMenuAudioPlayback::delivered_pairs() {
    std::lock_guard lock(output_mutex_);
    return output_.state().delivered_pairs;
}
std::size_t NativeTitleMenuAudioPlayback::drop_late_output(std::uint64_t due, std::size_t ceiling,
                                                           std::size_t keep) {
    std::lock_guard lock(output_mutex_);
    return output_.drop_late(due, ceiling, keep);
}
namespace {
constexpr std::array<std::uint8_t, 8> playback_magic{'U', 'R', 'A', 'P', '0', '0', '0', '1'};
}
std::vector<std::uint8_t>
serialize_title_menu_audio_playback(const TitleMenuAudioPlaybackState& state) {
    validate_ownership(state);
    auto native = serialize_title_menu_audio(state.native);
    auto output = serialize_audio_output(state.output);
    auto magic = playback_magic;
    audio_state_detail::Archive archive;
    archive.fields(magic, native, output);
    return archive.take_output();
}
TitleMenuAudioPlaybackState
deserialize_title_menu_audio_playback(std::span<const std::uint8_t> bytes) {
    audio_state_detail::Archive archive(bytes);
    std::array<std::uint8_t, 8> magic{};
    archive.value(magic);
    if (magic != playback_magic) throw std::invalid_argument("audio playback format differs");
    std::vector<std::uint8_t> native, output;
    archive.fields(native, output);
    archive.require_end();
    TitleMenuAudioPlaybackState state{deserialize_title_menu_audio(native),
                                      deserialize_audio_output(output)};
    validate_ownership(state);
    return state;
}
} // namespace unirally
