#include "audio_score.hpp"
#include <iostream>
#include <stdexcept>

static void require(bool result, const char* message) {
    if (!result) throw std::runtime_error(message);
}
int main() {
    try {
        unirally::TitleMenuAudioData data{std::vector<std::uint8_t>(621), std::vector<std::uint8_t>(2200)};
        // Eight entry pointers select the authored call/loop sequence.
        for (unsigned voice = 0; voice < 8; ++voice) data.menu_tables[4 * voice + 3] = 0x1d;
        const std::uint8_t sequence[]{0x82, 6, 0x1d, 0x80, 0, 0, 0x84, 2, 12, 1, 0x85, 0x83};
        std::copy(std::begin(sequence), std::end(sequence), data.title_score.begin());
        unirally::TitleMenuAudioScore continuous(data), restored(data);
        continuous.start_music(1);
        continuous.update_voice(0, false);
        require(continuous.take_reads() == std::vector<unirally::AudioScoreRead>{{0, 0x1d00, 0x82},
            {0, 0x1d01, 6}, {0, 0x1d02, 0x1d}, {0, 0x1d06, 0x84}, {0, 0x1d07, 2},
            {0, 0x1d08, 12}, {0, 0x1d09, 1}}, "call/loop reads differ");
        restored.restore(continuous.state());
        for (unsigned update = 0; update < 6; ++update) {
            continuous.update_voice(0, false); restored.update_voice(0, false);
            require(continuous.take_reads() == restored.take_reads(), "restored reads differ");
            require(continuous.state() == restored.state(), "restored state differs");
        }
        require(!continuous.state().voices[0].enabled && continuous.state().voices[0].stack_position == 0,
                "call/loop continuation did not terminate");
        auto snapshot = continuous.state(); snapshot.voices[0].stack_position = 255;
        bool rejected = false;
        try { restored.restore(snapshot); } catch (const std::invalid_argument&) { rejected = true; }
        require(rejected, "invalid stack accepted");
        // A zero duration is 256 updates, not an immediate next-note dispatch.
        data.title_score[0] = 12; data.title_score[1] = 0; data.title_score[2] = 0x80;
        unirally::TitleMenuAudioScore zero(data); zero.start_music(1); zero.update_voice(0, false);
        for (unsigned update = 0; update < 255; ++update) zero.update_voice(0, false);
        require(zero.state().voices[0].enabled, "zero duration did not wrap");
        zero.update_voice(0, false); require(!zero.state().voices[0].enabled, "zero duration never ended");

        // Restore while a nonzero software envelope and score loop are active.
        const std::uint8_t voiced[]{0xb3, 64, 128, 0xa2, 1, 0, 3, 127, 5, 80, 6,
                                    12, 30, 0x81, 11, 0x1d};
        std::copy(std::begin(voiced), std::end(voiced), data.title_score.begin());
        unirally::AudioPitchData pitch; pitch.notes[12] = 0x1000; pitch.sample_fraction[0] = 64;
        unirally::TitleMenuAudioScore voiced_continuous(data, &pitch), voiced_restored(data, &pitch);
        voiced_continuous.start_music(1);
        for (std::uint8_t update = 0; update < 9; ++update) voiced_continuous.update_voice(0, false, update);
        require(voiced_continuous.state().voices[0].arithmetic.gain != 0 &&
                voiced_continuous.state().voices[0].arithmetic.output_pitch == 0x1400,
                "authored voiced sequence remained silent");
        voiced_continuous.take_reads(); voiced_restored.restore(voiced_continuous.state());
        for (std::uint8_t update = 9; update < 129; ++update) {
            voiced_continuous.update_voice(0, false, update); voiced_restored.update_voice(0, false, update);
            require(voiced_continuous.state() == voiced_restored.state(), "voiced restored state differs");
            require(voiced_continuous.take_reads() == voiced_restored.take_reads(), "voiced restored reads differ");
        }
        std::cout << "native score calls, loops, unsigned duration and restore pass\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
