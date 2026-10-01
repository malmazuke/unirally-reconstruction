#pragma once
#include "title_menu_audio_playback.hpp"
#include <atomic>
#include <condition_variable>
#include <exception>
#include <mutex>
#include <thread>

namespace unirally::app {
// Native producers execute on one thread. Automatic controller polling waits
// for the main loop's actual sampled frame; device callbacks drain output only.
class TitleMenuAudioStream final : private AudioControllerSource {
public:
    explicit TitleMenuAudioStream(const ClassicContentPack& pack, std::uint32_t output_rate);
    ~TitleMenuAudioStream();
    void submit_frame(std::uint32_t frame, std::array<std::uint16_t, 2> words);
    std::vector<std::int16_t> take_pairs(std::size_t maximum);
    void check_failure();
    std::uint64_t source_pairs();
    std::uint64_t delivered_pairs();
    std::uint64_t navigation_count() const { return navigation_count_.load(); }
    std::uint64_t restart_count() const { return restart_count_.load(); }

private:
    TitleMenuAudioContent content_;
    NativeTitleMenuAudioPlayback playback_;
    std::mutex input_mutex_;
    std::condition_variable available_;
    std::vector<std::array<std::uint16_t, 2>> frames_;
    bool stopping_ = false;
    std::exception_ptr failure_;
    std::atomic<std::uint64_t> navigation_count_{0}, restart_count_{0};
    std::thread producer_;
    std::uint16_t controller_word(std::uint64_t ticks, unsigned port) override;
    void run();
    AudioCpuMenuAction run_menu();
    void run_hunter();
};
} // namespace unirally::app
