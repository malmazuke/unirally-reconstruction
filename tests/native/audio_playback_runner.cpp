#include "audio_title_menu_data.hpp"
#include "title_menu_audio_playback.hpp"
#include <iostream>
#include <limits>
#include <memory>

namespace {
class Controllers final : public unirally::AudioControllerSource {
public:
    std::vector<std::array<std::uint16_t, 2>> rows;
    explicit Controllers(const char* path) {
        std::ifstream file(path);
        unsigned frame, first, second;
        if (!file) throw std::runtime_error("cannot read controller schedule");
        while (file >> frame >> first >> second) {
            if (frame != rows.size() || first > 65535 || second > 65535)
                throw std::invalid_argument("invalid native controller schedule");
            rows.push_back({static_cast<std::uint16_t>(first), static_cast<std::uint16_t>(second)});
        }
        if (!file.eof()) throw std::invalid_argument("controller schedule truncated");
    }
    std::uint16_t controller_word(std::uint64_t ticks, unsigned port) override {
        const auto frame = ticks / 425568 + 1;
        return frame < rows.size() ? rows[frame][port] : 0;
    }
};
class Events final : public unirally::AudioEngineEventSink {
public:
    std::ofstream file;
    explicit Events(const char* path) : file(path) {
        if (!file) throw std::runtime_error("cannot write native events");
    }
    void event(char kind, std::uint64_t smp, std::uint64_t cpu, std::uint16_t address,
               std::uint8_t value) override {
        file << kind << ' ' << smp << ' ' << cpu << ' ' << address << ' ' << unsigned(value)
             << '\n';
    }
};

}
int main(int argc, char** argv) {
    try {
        if (argc != 12)
            throw std::invalid_argument(
                "audio_playback_runner DATA INPUT EVENTS PCM MENU_FRAMES DSP_END "
                "SAVE_PHASE SAVE_FRAME STATE RESTORE (phase: none|cold|title|menu; restore: - or "
                "file) OUTPUT_RATE");
        const auto content = audio_test::content(argv[1]);
        Controllers controllers(argv[2]);
        Events events(argv[3]);
        std::ofstream pcm(argv[4], std::ios::binary);
        if (!pcm) throw std::runtime_error("cannot write PCM");
        unirally::NativeTitleMenuAudioPlayback playback(
            content, controllers, static_cast<std::uint32_t>(std::stoul(argv[11])), &events);
        auto& audio = playback.native();
        const auto drain = [&] { audio_test::write_pcm(pcm, playback.take_pairs(257)); };
        unsigned completed_menu = 0;
        if (std::string(argv[10]) != "-") {
            const auto saved = audio_test::read(argv[10]);
            const auto state = unirally::deserialize_title_menu_audio_playback(saved);
            if (unirally::serialize_title_menu_audio_playback(state) != saved)
                throw std::runtime_error("audio file is not canonical");
            playback.restore(state);
            if (unirally::serialize_title_menu_audio_playback(playback.snapshot()) != saved)
                throw std::runtime_error("restored audio state differs");
            if (audio.phase() == unirally::TitleMenuAudioPhase::menu)
                completed_menu = static_cast<unsigned>(std::stoul(argv[8]));
        }
        const auto target = static_cast<unsigned>(std::stoul(argv[5]));
        const std::string save_phase = argv[7];
        const auto save_frame = static_cast<unsigned>(std::stoul(argv[8]));
        const auto save = [&] {
            const auto state = unirally::serialize_title_menu_audio_playback(playback.snapshot());
            audio_test::write(argv[9], state);
            std::cout << "saved_cpu_clock=" << audio.cpu_ticks() << " state_bytes=" << state.size()
                      << '\n';
        };
        if (audio.phase() == unirally::TitleMenuAudioPhase::cold) {
            if (save_phase == "cold") {
                save();
                return 0;
            }
            audio.initialize_title();
        }
        unsigned title_frames = 0;
        while (audio.phase() == unirally::TitleMenuAudioPhase::title_hold) {
            audio.title_frame();
            drain();
            ++title_frames;
            if (save_phase == "title" && title_frames == save_frame) {
                save();
                return 0;
            }
        }
        if (audio.phase() == unirally::TitleMenuAudioPhase::title_complete) audio.reveal_menu();
        while (completed_menu < target && audio.phase() == unirally::TitleMenuAudioPhase::menu) {
            const auto action = audio.menu_frame();
            if (action != unirally::AudioCpuMenuAction::waiting)
                throw std::runtime_error("native menu crossed the tested audio domain");
            ++completed_menu;
            drain();
            std::cout << "menu_frame_end=" << audio.cpu_ticks() << '\n';
            if (save_phase == "menu" && completed_menu == save_frame) {
                save();
                return 0;
            }
        }
        audio.finish_pcm_to(std::stoull(argv[6]));
        playback.snapshot();
        audio_test::write_pcm(pcm, playback.take_pairs(100000000));
        audio_test::write(argv[9],
                          unirally::serialize_title_menu_audio_playback(playback.snapshot()));
        if (!events.file) throw std::runtime_error("native event output failed");
        std::cout << "computed_cpu_clock=" << audio.cpu_ticks() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
