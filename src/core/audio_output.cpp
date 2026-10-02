#include "audio_output.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace unirally {
namespace {
constexpr std::uint32_t source_rate = 32040;
void validate(const AudioOutputState& state) {
    if (state.rate < 8000 || state.rate > 192000 || state.fraction >= state.rate
        || state.next_pair > 4 || state.source.size() % 2 || state.pending.size() % 2
        || state.source.size() > 16 * 1024 * 1024 || state.pending.size() > 16 * 1024 * 1024
        || state.delivered_pairs > state.generated_pairs
        || state.pending.size() / 2 != state.generated_pairs - state.delivered_pairs
        || state.source.size() > 2 || state.source_pairs < state.source.size() / 2
        || state.generated_pairs > std::numeric_limits<std::uint64_t>::max() / source_rate)
        throw std::invalid_argument("invalid native audio output continuation");
    const auto numerator = state.generated_pairs * source_rate;
    const auto consumed = state.source_pairs - state.source.size() / 2;
    if (state.fraction != numerator % state.rate || numerator / state.rate < consumed
        || numerator / state.rate - consumed != state.next_pair)
        throw std::invalid_argument("audio output counters and fraction differ");
}
}
NativeAudioOutput::NativeAudioOutput(std::uint32_t rate) {
    state_.rate = rate;
    validate(state_);
}
// Linear interpolation uses signed 64-bit intermediates and division toward
// zero, followed by exact rate-ratio stepping. It is an output policy, not a
// claim about the original console or the reference core's resampler.
void NativeAudioOutput::convert() {
    const auto pairs = state_.source.size() / 2;
    auto position = std::size_t(state_.next_pair);
    while (position + 1 < pairs) {
        for (unsigned channel = 0; channel < 2; ++channel) {
            const std::int64_t first = state_.source[position * 2 + channel];
            const std::int64_t second = state_.source[(position + 1) * 2 + channel];
            const auto value =
                (first * (state_.rate - state_.fraction) + second * state_.fraction) / state_.rate;
            state_.pending.push_back(static_cast<std::int16_t>(value));
        }
        ++state_.generated_pairs;
        state_.fraction += source_rate;
        position += state_.fraction / state_.rate;
        state_.fraction %= state_.rate;
    }
    const auto consumed = std::min(position, pairs);
    state_.source.erase(state_.source.begin(),
                        state_.source.begin() + static_cast<std::ptrdiff_t>(consumed * 2));
    state_.next_pair = static_cast<std::uint32_t>(position - consumed);
}
void NativeAudioOutput::append_pcm(std::span<const std::int16_t> samples) {
    if (samples.size() % 2) throw std::invalid_argument("native PCM is not stereo");
    constexpr std::size_t maximum_samples = 16 * 1024 * 1024;
    if (samples.size() > maximum_samples - state_.source.size())
        throw std::invalid_argument("native PCM chunk exceeds output queue bound");
    const auto pairs = (samples.size() + state_.source.size()) / 2;
    std::uint64_t generated = 0;
    if (pairs > std::size_t(state_.next_pair) + 1) {
        const auto numerator =
            (pairs - state_.next_pair - 1) * std::uint64_t(state_.rate) - state_.fraction;
        generated = (numerator + source_rate - 1) / source_rate;
    }
    if (generated > maximum_samples / 2 - state_.pending.size() / 2
        || generated
               > std::numeric_limits<std::uint64_t>::max() / source_rate - state_.generated_pairs
        || samples.size() / 2 > std::numeric_limits<std::uint64_t>::max() - state_.source_pairs)
        throw std::invalid_argument("native audio output queue or counter limit reached");
    state_.source_pairs += samples.size() / 2;
    state_.source.insert(state_.source.end(), samples.begin(), samples.end());
    convert();
}
std::vector<std::int16_t> NativeAudioOutput::take_pairs(std::size_t maximum) {
    const auto count = std::min(maximum, state_.pending.size() / 2) * 2;
    std::vector<std::int16_t> out(state_.pending.begin(),
                                  state_.pending.begin() + static_cast<std::ptrdiff_t>(count));
    state_.pending.erase(state_.pending.begin(),
                         state_.pending.begin() + static_cast<std::ptrdiff_t>(count));
    state_.delivered_pairs += count / 2;
    return out;
}
std::size_t NativeAudioOutput::drop_late(std::uint64_t due, std::size_t ceiling,
                                         std::size_t keep) {
    if (keep > ceiling) throw std::invalid_argument("late output keeps more than its ceiling");
    if (due <= state_.delivered_pairs + ceiling) return 0;
    const auto late = due - state_.delivered_pairs - keep;
    const auto pending = state_.pending.size() / 2;
    return take_pairs(late < pending ? static_cast<std::size_t>(late) : pending).size() / 2;
}
void NativeAudioOutput::restore(const AudioOutputState& state) {
    validate(state);
    auto candidate = state;
    state_ = std::move(candidate);
}
} // namespace unirally
