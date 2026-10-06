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
// After a 1P menu exit each frame's sound queue work from the native game drives
// the producer instead (D-0010); `stop` ends production where it is not recovered.
class TitleMenuAudioStream final : private AudioControllerSource {
public:
    explicit TitleMenuAudioStream(const ClassicContentPack& pack, std::uint32_t output_rate);
    ~TitleMenuAudioStream();
    void submit_frame(std::uint32_t frame, std::array<std::uint16_t, 2> words,
                      AudioCueList cues = {}, bool stop = false);
    std::vector<std::int16_t> take_pairs(std::size_t maximum);
    void check_failure();
    std::uint64_t source_pairs();
    std::uint64_t delivered_pairs();
    // Host latency policy, not original timing: output pair n plays the producer's clock at
    // n / rate seconds, and the game submitting `frame` has reached that frame's start. Drops
    // output running more than `ceiling` pairs late (a slow start or a stalled producer) to
    // `keep` pairs late; returns the pairs dropped and their peak. The frame's start is taken
    // 306,900 ticks (0.72 frame) before its input is read at vertical blank, so both bounds act
    // about 0.72 frame larger: conservative.
    AudioLateDrop trim_late_output(std::uint32_t frame, std::size_t ceiling, std::size_t keep);
    std::uint64_t navigation_count() const { return navigation_count_.load(); }
    std::uint64_t restart_count() const { return restart_count_.load(); }
    std::uint64_t cued_frames() const { return cued_frames_.load(); }
    bool cued_stopped() const { return cued_stopped_.load(); }

private:
    TitleMenuAudioContent content_;
    std::uint32_t output_rate_;
    NativeTitleMenuAudioPlayback playback_;
    std::mutex input_mutex_;
    std::condition_variable available_;
    struct FrameInput {
        std::array<std::uint16_t, 2> words{};
        AudioCueList cues;
        bool stop = false;
    };
    std::vector<FrameInput> frames_;
    bool stopping_ = false;
    std::exception_ptr failure_;
    std::atomic<std::uint64_t> navigation_count_{0}, restart_count_{0}, cued_frames_{0};
    std::atomic<bool> cued_stopped_{false};
    std::thread producer_;
    std::uint16_t controller_word(std::uint64_t ticks, unsigned port) override;
    void run();
    AudioCpuMenuAction run_menu();
    void run_hunter();
    void run_cued();
};
} // namespace unirally::app
