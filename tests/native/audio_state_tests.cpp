#include "title_menu_audio.hpp"
#include <iostream>
#include <stdexcept>

namespace {
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
class Controllers final : public unirally::AudioControllerSource {
    std::uint16_t controller_word(std::uint64_t, unsigned) override { return 0; }
};
template <class Action>
void rejects(Action action) {
    bool rejected = false;
    try {
        action();
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected, "malformed audio continuation accepted");
}
void check_file_and_transactional_restore() {
    unirally::TitleMenuAudioContent content;
    content.score.menu_tables.resize(621);
    content.score.title_score.resize(2200);
    Controllers controllers;
    unirally::NativeTitleMenuAudio audio(content, controllers);
    auto state = audio.snapshot();
    state.engine.pending_pcm = {32767, -32768, 123, -321};
    const auto bytes = unirally::serialize_title_menu_audio(state);
    const auto decoded = unirally::deserialize_title_menu_audio(bytes);
    audio.restore(decoded);
    require(unirally::serialize_title_menu_audio(audio.snapshot()) == bytes,
            "canonical audio file changed on restore");
    require(audio.take_pcm() == state.engine.pending_pcm, "signed queued PCM changed");
    const auto before = unirally::serialize_title_menu_audio(audio.snapshot());
    auto invalid = audio.snapshot();
    invalid.engine.cpu.ticks = 1;
    rejects([&] { audio.restore(invalid); });
    require(unirally::serialize_title_menu_audio(audio.snapshot()) == before,
            "bad CPU continuation mutated running audio");
    invalid = audio.snapshot();
    invalid.engine.dsp.hardware[0] ^= 1;
    rejects([&] { audio.restore(invalid); });
    require(unirally::serialize_title_menu_audio(audio.snapshot()) == before,
            "bad DSP continuation mutated running audio");
    invalid = audio.snapshot();
    invalid.content_identity[0] ^= 1;
    rejects([&] { audio.restore(invalid); });
    require(unirally::serialize_title_menu_audio(audio.snapshot()) == before,
            "wrong content continuation mutated running audio");
    auto corrupt = bytes;
    corrupt[0] ^= 1;
    rejects([&] { unirally::deserialize_title_menu_audio(corrupt); });
    corrupt = bytes;
    corrupt.pop_back();
    rejects([&] { unirally::deserialize_title_menu_audio(corrupt); });
    corrupt = bytes;
    corrupt.push_back(0);
    rejects([&] { unirally::deserialize_title_menu_audio(corrupt); });
    corrupt = bytes;
    // URAU0002 header8, phase1, identity32, then three CPU clock words8 each.
    constexpr unsigned first_cpu_flag = 8 + 1 + 32 + 3 * 8;
    corrupt[first_cpu_flag] = 2;
    rejects([&] { unirally::deserialize_title_menu_audio(corrupt); });
}
}
int main() {
    try {
        check_file_and_transactional_restore();
        std::cout << "canonical audio file checks passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
