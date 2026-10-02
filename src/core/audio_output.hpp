#pragma once
#include "audio_engine.hpp"

namespace unirally {
// Modern output conversion, separate from original DSP arithmetic. Source is
// interleaved signed stereo at 32040 Hz. Fraction uses output-rate units.
struct AudioOutputState {
    std::uint32_t rate = 48000, fraction = 0, next_pair = 0;
    std::uint64_t source_pairs = 0, generated_pairs = 0, delivered_pairs = 0;
    std::vector<std::int16_t> source, pending;
    bool operator==(const AudioOutputState&) const = default;
};
class NativeAudioOutput final : public AudioPcmSink {
public:
    explicit NativeAudioOutput(std::uint32_t rate = 48000);
    void append_pcm(std::span<const std::int16_t> samples) override;
    // Drain at most the requested stereo pairs. Shortage is observable to the
    // device caller; the native queue never inserts invented source samples.
    std::vector<std::int16_t> take_pairs(std::size_t maximum);
    const AudioOutputState& state() const { return state_; }
    // Host latency policy: once the next pair to drain runs more than `ceiling` pairs behind
    // pair `due` (the time the game has reached), drops queued pairs, oldest first, until it is
    // `keep` behind or the queue is empty. Dropped pairs count as delivered; returns them.
    std::size_t drop_late(std::uint64_t due, std::size_t ceiling, std::size_t keep);
    void restore(const AudioOutputState& state);

private:
    AudioOutputState state_;
    void convert();
};
std::vector<std::uint8_t> serialize_audio_output(const AudioOutputState& state);
AudioOutputState deserialize_audio_output(std::span<const std::uint8_t> bytes);
} // namespace unirally
