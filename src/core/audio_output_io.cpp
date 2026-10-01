#include "audio_output.hpp"
#include "audio_state_archive.hpp"

namespace unirally {
namespace {
void fields(audio_state_detail::Archive& archive, AudioOutputState& state) {
    archive.fields(state.rate, state.fraction, state.next_pair, state.source_pairs,
                   state.generated_pairs, state.delivered_pairs, state.source, state.pending);
}
constexpr std::array<std::uint8_t, 8> output_magic{'U', 'R', 'A', 'O', '0', '0', '0', '1'};
}
std::vector<std::uint8_t> serialize_audio_output(const AudioOutputState& state) {
    NativeAudioOutput checked;
    checked.restore(state);
    auto owned = state;
    auto magic = output_magic;
    audio_state_detail::Archive archive;
    archive.value(magic);
    fields(archive, owned);
    return archive.take_output();
}
AudioOutputState deserialize_audio_output(std::span<const std::uint8_t> bytes) {
    audio_state_detail::Archive archive(bytes);
    std::array<std::uint8_t, 8> magic{};
    archive.value(magic);
    if (magic != output_magic) throw std::invalid_argument("audio output format differs");
    AudioOutputState state;
    fields(archive, state);
    archive.require_end();
    NativeAudioOutput checked;
    checked.restore(state);
    return state;
}
} // namespace unirally
