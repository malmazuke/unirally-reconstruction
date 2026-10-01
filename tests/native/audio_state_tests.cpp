#include "title_menu_audio.hpp"
#include "title_menu_audio_playback.hpp"
#include <iostream>
#include <stdexcept>
#include <type_traits>

namespace {
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
class Controllers final : public unirally::AudioControllerSource {
    std::uint16_t controller_word(std::uint64_t, unsigned) override { return 0; }
};
static_assert(!std::is_move_constructible_v<unirally::NativeAudioEngine>);
static_assert(!std::is_move_constructible_v<unirally::NativeTitleMenuAudioPlayback>);
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
    content.score.tables.resize(621);
    content.score.score.resize(2200);
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
    // URAU0005 header8, phase1, action1, identity32, then three CPU clock words8 each.
    constexpr unsigned first_cpu_flag = 8 + 1 + 1 + 32 + 3 * 8;
    corrupt[first_cpu_flag] = 2;
    rejects([&] { unirally::deserialize_title_menu_audio(corrupt); });
}
void check_hunter_state_validation() {
    unirally::TitleMenuAudioContent content;
    content.score.tables.resize(621);
    content.score.score.resize(2200);
    Controllers controllers;
    unirally::NativeTitleMenuAudio audio(content, controllers);
    const auto before = unirally::serialize_title_menu_audio(audio.snapshot());
    auto invalid = audio.snapshot();
    invalid.hunter.credits_frame = 96;
    rejects([&] { audio.restore(invalid); });
    invalid = audio.snapshot();
    invalid.hunter.decorations.wave[7] = 20;
    rejects([&] { audio.restore(invalid); });
    invalid = audio.snapshot();
    invalid.hunter.timed_remaining = 1201;
    rejects([&] { audio.restore(invalid); });
    invalid = audio.snapshot();
    invalid.hunter.first_press = true;
    rejects([&] { audio.restore(invalid); });
    invalid = audio.snapshot();
    invalid.engine.cpu.reveal_hdma.channels[1].cursor = 256;
    rejects([&] { audio.restore(invalid); });
    invalid = audio.snapshot();
    invalid.engine.cpu.reveal_hdma.pending = 3;
    rejects([&] { audio.restore(invalid); });
    invalid = audio.snapshot();
    invalid.engine.cpu.reveal_hdma.setup_position = 13;
    rejects([&] { audio.restore(invalid); });
    invalid = audio.snapshot();
    invalid.engine.ipl_timers.ticks = 2;
    rejects([&] { audio.restore(invalid); });
    require(unirally::serialize_title_menu_audio(audio.snapshot()) == before,
            "invalid HUNTER continuation mutated running audio");
}
void check_playback_ownership() {
    unirally::TitleMenuAudioContent content;
    content.score.tables.resize(621);
    content.score.score.resize(2200);
    Controllers controllers;
    unirally::NativeTitleMenuAudioPlayback playback(content, controllers, 48000);
    playback.native().finish_pcm_to(96);
    const auto state = playback.snapshot();
    require(state.output.source_pairs == 3 && state.output.fraction != 0
                && !state.output.pending.empty(),
            "combined queue was not exercised");
    const auto bytes = unirally::serialize_title_menu_audio_playback(state);
    playback.restore(unirally::deserialize_title_menu_audio_playback(bytes));
    require(unirally::serialize_title_menu_audio_playback(playback.snapshot()) == bytes,
            "combined playback file changed on restore");
    auto invalid = state;
    invalid.native.engine.pending_pcm = {123, -321};
    rejects([&] { playback.restore(invalid); });
    invalid = state;
    invalid.output.fraction ^= 1;
    rejects([&] { playback.restore(invalid); });
    invalid = state;
    invalid.native.engine.dsp.clocks += 32;
    rejects([&] { playback.restore(invalid); });
    invalid = state;
    invalid.native.engine.cpu.ticks = 1;
    rejects([&] { playback.restore(invalid); });
    require(unirally::serialize_title_menu_audio_playback(playback.snapshot()) == bytes,
            "invalid combined playback mutated a running owner");
    auto bad_file = bytes;
    bad_file.pop_back();
    rejects([&] { unirally::deserialize_title_menu_audio_playback(bad_file); });
}
}
int main() {
    try {
        check_file_and_transactional_restore();
        check_playback_ownership();
        check_hunter_state_validation();
        std::cout << "canonical audio file checks passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
