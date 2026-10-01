#include "audio_title_menu_data.hpp"
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
class Pcm final : public unirally::AudioPcmSink {
public:
    explicit Pcm(std::ostream& file) : file_(file) {}
    void append_pcm(std::span<const std::int16_t> samples) override {
        audio_test::write_pcm(file_, samples);
        ++chunks;
    }
    std::uint64_t chunks = 0;

private:
    std::ostream& file_;
};
}
int main(int argc, char** argv) {
    try {
        if (argc != 11 && argc != 12)
            throw std::invalid_argument(
                "title_menu_audio_runner DATA INPUT EVENTS PCM MENU_FRAMES DSP_END "
                "SAVE_PHASE SAVE_FRAME STATE RESTORE (phase: none|cold|title|menu|hunter|"
                "first-reveal|second-reveal|page-wait|timed-wait|credits-loop|reset|warm-title; "
                "restore: - or file) [--stream-pcm|--hunter-<endpoint>]");
        const auto content = audio_test::content(argv[1]);
        Controllers controllers(argv[2]);
        Events events(argv[3]);
        std::ofstream pcm(argv[4], std::ios::binary);
        if (!pcm) throw std::runtime_error("cannot write PCM");
        unirally::NativeTitleMenuAudio audio(content, controllers, &events);
        Pcm streamed(pcm);
        const std::string mode = argc == 12 ? argv[11] : "";
        const std::array<std::string_view, 14> hunter_modes{
            "--hunter-exit",        "--hunter-entry",         "--hunter-fade",
            "--hunter-page",        "--hunter-pressed",       "--hunter-timed",
            "--hunter-credits",     "--hunter-credits-setup", "--hunter-first-pose",
            "--hunter-second-pose", "--hunter-loop",          "--hunter-leaving",
            "--hunter-reset",       "--hunter-warm-menu"};
        const auto found = std::find(hunter_modes.begin(), hunter_modes.end(), mode);
        const unsigned hunter_stage = found == hunter_modes.end()
                                        ? 0U
                                        : static_cast<unsigned>(found - hunter_modes.begin() + 1);
        const bool hunter = hunter_stage != 0;
        if (argc == 12) {
            if (mode == "--stream-pcm") {
                if (std::string(argv[7]) != "none" || std::string(argv[10]) != "-")
                    throw std::invalid_argument("streamed runner requires uninterrupted input");
                audio.set_pcm_sink(&streamed);
            } else if (!hunter) {
                throw std::invalid_argument("unknown native audio diagnostic mode");
            }
        }
        unsigned completed_menu = 0;
        if (std::string(argv[10]) != "-") {
            const auto saved = audio_test::read(argv[10]);
            const auto state = unirally::deserialize_title_menu_audio(saved);
            if (unirally::serialize_title_menu_audio(state) != saved)
                throw std::runtime_error("audio file is not canonical");
            audio.restore(state);
            if (unirally::serialize_title_menu_audio(audio.snapshot()) != saved)
                throw std::runtime_error("restored audio state differs");
            if (audio.phase() == unirally::TitleMenuAudioPhase::menu)
                completed_menu = static_cast<unsigned>(std::stoul(argv[8]));
        }
        const auto target = static_cast<unsigned>(std::stoul(argv[5]));
        const std::string save_phase = argv[7];
        const auto save_frame = static_cast<unsigned>(std::stoul(argv[8]));
        const auto save = [&] {
            const auto state = unirally::serialize_title_menu_audio(audio.snapshot());
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
            ++title_frames;
            if (save_phase == "title" && title_frames == save_frame) {
                save();
                return 0;
            }
        }
        if (audio.phase() == unirally::TitleMenuAudioPhase::title_complete) audio.reveal_menu();
        while (completed_menu < target && audio.phase() == unirally::TitleMenuAudioPhase::menu) {
            const auto action = audio.menu_frame();
            if (action != unirally::AudioCpuMenuAction::waiting
                && !(hunter && action == unirally::AudioCpuMenuAction::hunter))
                throw std::runtime_error("native menu crossed the tested audio domain");
            ++completed_menu;
            std::cout << "menu_frame_end=" << audio.cpu_ticks() << '\n';
            if (save_phase == "menu" && completed_menu == save_frame) {
                save();
                return 0;
            }
        }
        if (hunter && audio.phase() == unirally::TitleMenuAudioPhase::menu)
            throw std::runtime_error("HUNTER menu code was not entered");
        unsigned hunter_frames = 0;
        if (hunter_stage >= 2) {
            while (audio.phase() == unirally::TitleMenuAudioPhase::menu_exit
                   || audio.phase() == unirally::TitleMenuAudioPhase::hunter_entry) {
                audio.hunter_entry_frame();
                ++hunter_frames;
                std::cout << "hunter_frame_end=" << audio.cpu_ticks() << '\n';
                if (save_phase == "hunter" && hunter_frames == save_frame) {
                    save();
                    return 0;
                }
            }
        }
        if (hunter_stage >= 3 && audio.phase() == unirally::TitleMenuAudioPhase::hunter_ready)
            audio.hunter_first_fade();
        if (hunter_stage >= 4 && audio.phase() == unirally::TitleMenuAudioPhase::hunter_faded)
            audio.hunter_begin_first_page();
        unsigned first_reveal_frames = 0;
        if (hunter_stage >= 4)
            while (audio.phase() == unirally::TitleMenuAudioPhase::hunter_first_reveal) {
                audio.hunter_reveal_frame();
                if (save_phase == "first-reveal" && ++first_reveal_frames == save_frame) {
                    save();
                    return 0;
                }
            }
        unsigned page_frames = 0;
        if (hunter_stage >= 5)
            while (audio.phase() == unirally::TitleMenuAudioPhase::hunter_first_page) {
                audio.hunter_page_wait_frame();
                if (save_phase == "page-wait" && ++page_frames == save_frame) {
                    save();
                    return 0;
                }
            }
        if (hunter_stage >= 6
            && audio.phase() == unirally::TitleMenuAudioPhase::hunter_page_pressed)
            audio.hunter_begin_second_page();
        unsigned second_reveal_frames = 0;
        if (hunter_stage >= 6)
            while (audio.phase() == unirally::TitleMenuAudioPhase::hunter_second_reveal) {
                audio.hunter_reveal_frame();
                if (save_phase == "second-reveal" && ++second_reveal_frames == save_frame) {
                    save();
                    return 0;
                }
            }
        unsigned timed_frames = 0;
        if (hunter_stage >= 7)
            while (audio.phase() == unirally::TitleMenuAudioPhase::hunter_timed_wait) {
                audio.hunter_timed_wait_frame();
                if (save_phase == "timed-wait" && ++timed_frames == save_frame) {
                    save();
                    return 0;
                }
            }
        if (hunter_stage >= 8
            && audio.phase() == unirally::TitleMenuAudioPhase::hunter_credits_ready)
            for (auto mark : audio.hunter_prepare_credits())
                std::cout << "credits_work_mark=" << mark << '\n';
        if (hunter_stage >= 9
            && audio.phase() == unirally::TitleMenuAudioPhase::hunter_credits_prepared)
            audio.hunter_build_first_pose();
        if (hunter_stage >= 10 && audio.phase() == unirally::TitleMenuAudioPhase::hunter_first_pose)
            audio.hunter_build_second_pose();
        if (hunter_stage >= 11
            && audio.phase() == unirally::TitleMenuAudioPhase::hunter_second_pose)
            audio.hunter_finish_credits_setup();
        unsigned credits_frames = 0;
        if (hunter_stage >= 12)
            while (audio.phase() == unirally::TitleMenuAudioPhase::hunter_credits_loop) {
                audio.hunter_credits_frame();
                std::cout << "credits_frame_end=" << audio.cpu_ticks() << '\n';
                if (save_phase == "credits-loop" && ++credits_frames == save_frame) {
                    save();
                    return 0;
                }
            }
        if (hunter_stage >= 13 && audio.phase() == unirally::TitleMenuAudioPhase::hunter_leaving)
            audio.hunter_finish_credits();
        if (save_phase == "reset"
            && audio.phase() == unirally::TitleMenuAudioPhase::hunter_reset_ready) {
            save();
            return 0;
        }
        if (hunter_stage >= 14) {
            if (audio.phase() == unirally::TitleMenuAudioPhase::hunter_reset_ready)
                audio.restart_title();
            if (save_phase == "warm-title") {
                save();
                return 0;
            }
            while (audio.phase() == unirally::TitleMenuAudioPhase::warm_title_hold)
                audio.title_frame();
            if (audio.phase() == unirally::TitleMenuAudioPhase::warm_title_complete)
                audio.reveal_menu();
        }
        audio.finish_pcm_to(std::stoull(argv[6]));
        audio_test::write_pcm(pcm, audio.take_pcm());
        audio_test::write(argv[9], unirally::serialize_title_menu_audio(audio.snapshot()));
        if (!events.file) throw std::runtime_error("native event output failed");
        std::cout << "pcm_chunks=" << streamed.chunks << '\n';
        std::cout << "computed_cpu_clock=" << audio.cpu_ticks() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
