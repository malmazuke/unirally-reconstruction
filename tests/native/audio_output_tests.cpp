#include "audio_output.hpp"
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
template <class Action>
void rejects(Action action) {
    bool rejected = false;
    try {
        action();
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected, "malformed audio output accepted");
}
void check_known_interpolation() {
    unirally::NativeAudioOutput output(48060);
    output.append_pcm(std::array<std::int16_t, 8>{0, 0, 300, -300, 600, -600, 900, -900});
    require(output.take_pairs(20)
                == std::vector<std::int16_t>{0, 0, 200, -200, 400, -400, 600, -600, 800, -800},
            "signed stereo interpolation differs");
    require(output.state().fraction == 16020, "resampler fraction differs");
}
void check_chunking_restore_and_rejection() {
    std::vector<std::int16_t> source;
    for (int i = 0; i < 700; ++i) {
        source.push_back(static_cast<std::int16_t>(-32768 + i * 91));
        source.push_back(static_cast<std::int16_t>(32767 - i * 91));
    }
    for (const unsigned rate : {8000U, 32040U, 44100U, 48000U, 192000U}) {
        unirally::NativeAudioOutput whole(rate), chunked(rate);
        whole.append_pcm(source);
        for (std::size_t i = 0; i < source.size(); i += 2) {
            chunked.append_pcm(std::span(source).subspan(i, 2));
            if (i == 118) {
                const auto bytes = unirally::serialize_audio_output(chunked.state());
                unirally::NativeAudioOutput restored;
                restored.restore(unirally::deserialize_audio_output(bytes));
                require(unirally::serialize_audio_output(restored.state()) == bytes,
                        "audio output file changed on restore");
                chunked = std::move(restored);
            }
        }
        require(whole.state() == chunked.state(), "chunked output continuation differs");
        const auto expected = whole.take_pairs(10000);
        auto actual = chunked.take_pairs(13);
        const auto saved = unirally::serialize_audio_output(chunked.state());
        chunked.restore(unirally::deserialize_audio_output(saved));
        const auto rest = chunked.take_pairs(10000);
        actual.insert(actual.end(), rest.begin(), rest.end());
        require(actual == expected, "partial device drain changed stereo output");
        require(whole.state() == chunked.state(), "device drain counters differ");
        auto invalid = chunked.state();
        invalid.fraction = rate;
        rejects([&] { chunked.restore(invalid); });
        require(whole.state() == chunked.state(), "invalid fraction mutated output");
        invalid = chunked.state();
        invalid.fraction = (invalid.fraction + 1) % rate;
        rejects([&] { chunked.restore(invalid); });
        invalid = chunked.state();
        ++invalid.source_pairs;
        rejects([&] { chunked.restore(invalid); });
        require(whole.state() == chunked.state(), "inconsistent counters mutated output");
        auto trailing = saved;
        trailing.push_back(0);
        rejects([&] { unirally::deserialize_audio_output(trailing); });
        trailing = saved;
        trailing.pop_back();
        rejects([&] { unirally::deserialize_audio_output(trailing); });
    }
}
}
int main() {
    try {
        check_known_interpolation();
        check_chunking_restore_and_rejection();
        std::cout << "audio output fraction, queue and file checks passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
