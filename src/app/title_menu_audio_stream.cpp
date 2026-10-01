#include "title_menu_audio_stream.hpp"
#include <stdexcept>

namespace unirally::app {
namespace {
struct StreamStopped {};
}
TitleMenuAudioStream::TitleMenuAudioStream(const ClassicContentPack& pack,
                                           std::uint32_t output_rate)
    : content_(title_menu_audio_content(pack)), playback_(content_, *this, output_rate) {
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
void TitleMenuAudioStream::submit_frame(std::uint32_t frame, std::array<std::uint16_t, 2> words) {
    {
        std::lock_guard lock(input_mutex_);
        if (frame != frames_.size())
            throw std::invalid_argument("audio controller frames must be consecutive");
        // Same physical rocker policy as the original pad and native frontend.
        for (auto& word : words) {
            if ((word & 0x0c00) == 0x0c00) word &= 0xf3ff;
            if ((word & 0x0300) == 0x0300) word &= 0xfcff;
        }
        frames_.push_back(words);
    }
    available_.notify_all();
    check_failure();
}
std::uint16_t TitleMenuAudioStream::controller_word(std::uint64_t ticks, unsigned port) {
    const auto frame = ticks / 425568 + 1;
    std::unique_lock lock(input_mutex_);
    available_.wait(lock, [&] { return stopping_ || frame < frames_.size(); });
    if (stopping_) throw StreamStopped{};
    return frames_.at(static_cast<std::size_t>(frame)).at(port);
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
void TitleMenuAudioStream::run() {
    try {
        auto& native = playback_.native();
        native.initialize_title();
        while (native.phase() == TitleMenuAudioPhase::title_hold) native.title_frame();
        native.reveal_menu();
        for (;;) {
            const auto action = run_menu();
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
