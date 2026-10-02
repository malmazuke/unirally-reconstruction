#include "title_menu_audio_stream.hpp"
#include <stdexcept>

namespace unirally::app {
namespace {
struct StreamStopped {};
}
TitleMenuAudioStream::TitleMenuAudioStream(const ClassicContentPack& pack,
                                           std::uint32_t output_rate)
    : content_(title_menu_audio_content(pack)), output_rate_(output_rate),
      playback_(content_, *this, output_rate) {
    producer_ = std::thread([this] { run(); });
}
TitleMenuAudioStream::~TitleMenuAudioStream() {
    {
        std::lock_guard lock(input_mutex_);
        stopping_ = true;
    }
    available_.notify_all();
    if (producer_.joinable()) producer_.join();
}
void TitleMenuAudioStream::submit_frame(std::uint32_t frame, std::array<std::uint16_t, 2> words,
                                        AudioCueList cues, bool stop) {
    {
        std::lock_guard lock(input_mutex_);
        if (frame != frames_.size())
            throw std::invalid_argument("audio controller frames must be consecutive");
        // Same physical rocker policy as the original pad and native frontend.
        for (auto& word : words) {
            if ((word & 0x0c00) == 0x0c00) word &= 0xf3ff;
            if ((word & 0x0300) == 0x0300) word &= 0xfcff;
        }
        frames_.push_back({words, std::move(cues), stop});
    }
    available_.notify_all();
    check_failure();
}
std::uint16_t TitleMenuAudioStream::controller_word(std::uint64_t ticks, unsigned port) {
    const auto frame = ticks / 425568 + 1;
    std::unique_lock lock(input_mutex_);
    available_.wait(lock, [&] { return stopping_ || frame < frames_.size(); });
    if (stopping_) throw StreamStopped{};
    return frames_.at(static_cast<std::size_t>(frame)).words.at(port);
}
std::size_t TitleMenuAudioStream::trim_late_output(std::uint32_t frame, std::size_t ceiling,
                                                   std::size_t keep) {
    constexpr std::uint64_t frame_clocks = 425568, master_clock = 21281370; // PAL
    if (frame == 0) return 0;
    const auto due = (std::uint64_t{frame} - 1) * frame_clocks * output_rate_ / master_clock;
    return playback_.drop_late_output(due, ceiling, keep);
}
std::vector<std::int16_t> TitleMenuAudioStream::take_pairs(std::size_t maximum) {
    return playback_.take_pairs(maximum);
}
std::uint64_t TitleMenuAudioStream::source_pairs() {
    return playback_.source_pairs();
}
std::uint64_t TitleMenuAudioStream::delivered_pairs() {
    return playback_.delivered_pairs();
}
void TitleMenuAudioStream::check_failure() {
    std::exception_ptr failure;
    {
        std::lock_guard lock(input_mutex_);
        failure = failure_;
    }
    if (failure) std::rethrow_exception(failure);
}
AudioCpuMenuAction TitleMenuAudioStream::run_menu() {
    auto& native = playback_.native();
    for (;;) {
        const auto before = native.menu_selection();
        const auto action = native.menu_frame();
        if (native.menu_selection() != before) ++navigation_count_;
        if (action != AudioCpuMenuAction::waiting) return action;
    }
}
void TitleMenuAudioStream::run_hunter() {
    auto& native = playback_.native();
    while (native.hunter_entry_frame()) {}
    native.hunter_first_fade();
    native.hunter_reveal_first_page();
    while (native.hunter_page_wait_frame()) {}
    native.hunter_reveal_second_page();
    while (native.hunter_timed_wait_frame()) {}
    native.hunter_prepare_credits();
    native.hunter_build_first_pose();
    native.hunter_build_second_pose();
    native.hunter_finish_credits_setup();
    while (native.hunter_credits_frame()) {}
    native.hunter_finish_credits();
    native.restart_title();
    while (native.phase() == TitleMenuAudioPhase::warm_title_hold) native.title_frame();
    native.reveal_menu();
    ++restart_count_;
}
// D-0010: from the 1P exit's frame on, run each frame's reported queue work. Production stops
// (silence) where the game reports a scene outside the recovered sound domain.
void TitleMenuAudioStream::run_cued() {
    auto& native = playback_.native();
    constexpr std::uint64_t frame_clocks = 425568, first_frame_boundary = 306900;
    auto frame =
        static_cast<std::uint32_t>((native.cpu_ticks() - first_frame_boundary) / frame_clocks + 1);
    for (;; ++frame) {
        FrameInput input;
        {
            std::unique_lock lock(input_mutex_);
            available_.wait(lock, [&] { return stopping_ || frame < frames_.size(); });
            if (stopping_) throw StreamStopped{};
            input = frames_.at(frame);
        }
        if (input.stop) {
            cued_stopped_ = true;
            return;
        }
        native.cue_frame(frame, input.cues);
        ++cued_frames_;
    }
}
void TitleMenuAudioStream::run() {
    try {
        auto& native = playback_.native();
        native.initialize_title();
        while (native.phase() == TitleMenuAudioPhase::title_hold) native.title_frame();
        native.reveal_menu();
        for (;;) {
            const auto action = run_menu();
            constexpr std::uint8_t one_player = 0; // the main menu's first entry
            if (action == AudioCpuMenuAction::selected && native.menu_selection() == one_player
                && content_.has_race_set()) {
                run_cued();
                break;
            }
            if (action != AudioCpuMenuAction::hunter) break;
            run_hunter();
        }
    } catch (const StreamStopped&) {
    } catch (...) {
        std::lock_guard lock(input_mutex_);
        failure_ = std::current_exception();
    }
}
} // namespace unirally::app
