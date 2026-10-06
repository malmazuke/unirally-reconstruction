#include "sdl_audio.hpp"
#include "content_pack.hpp"
#include <iostream>
#include <stdexcept>

namespace unirally::app {
SdlTitleMenuAudio::SdlTitleMenuAudio(const ClassicContentPack& pack) {
    if (pack.optional_entry("audio.hunter-graphics-work-directory").empty())
        throw std::invalid_argument("--native-title-menu-audio requires a v31 or later content pack");
    SDL_AudioSpec spec{SDL_AUDIO_S16, 2, 48000};
    device_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, callback, this);
    if (!device_) throw std::runtime_error(SDL_GetError());
    try {
        SDL_AudioSpec hardware;
        if (!SDL_GetAudioDeviceFormat(SDL_GetAudioStreamDevice(device_), &hardware, nullptr))
            throw std::runtime_error(SDL_GetError());
        if (hardware.freq < 8000 || hardware.freq > 192000)
            throw std::runtime_error("audio device rate outside native output conversion domain");
        spec.freq = hardware.freq;
        if (!SDL_SetAudioStreamFormat(device_, &spec, nullptr))
            throw std::runtime_error(SDL_GetError());
        rate_ = static_cast<std::uint32_t>(spec.freq);
        producer_ = std::make_unique<TitleMenuAudioStream>(pack, rate_);
    } catch (...) {
        SDL_DestroyAudioStream(device_);
        device_ = nullptr;
        throw;
    }
}
SdlTitleMenuAudio::~SdlTitleMenuAudio() {
    // Destroying the stream stops callbacks before the queue owner is released.
    if (device_) SDL_DestroyAudioStream(device_);
}
void SDLCALL SdlTitleMenuAudio::callback(void* context, SDL_AudioStream* stream,
                                         int additional_bytes, int) {
    if (additional_bytes <= 0) return;
    auto& self = *static_cast<SdlTitleMenuAudio*>(context);
    try {
        const auto requested = (static_cast<std::size_t>(additional_bytes) + 3) / 4;
        const auto samples = self.producer_->take_pairs(requested);
        self.underrun_pairs_ += requested - samples.size() / 2;
        std::uint64_t nonzero = 0;
        for (std::size_t i = 0; i < samples.size(); i += 2)
            if (samples[i] || samples[i + 1]) ++nonzero;
        self.nonzero_pairs_ += nonzero;
        if (!samples.empty()
            && !SDL_PutAudioStreamData(stream, samples.data(),
                                       static_cast<int>(samples.size() * 2)))
            self.callback_failed_ = true;
    } catch (...) {
        self.callback_failed_ = true;
    }
}
void SdlTitleMenuAudio::submit_frame(std::uint32_t frame, std::array<std::uint16_t, 2> words,
                                     AudioCueList cues, bool stop) {
    // Host latency policy: output more than four PAL frames behind the game is dropped to two
    // behind. A slow producer start once left every later sound 17 frames late. Dropped pairs
    // count in native_audio_delivered_pairs as well as native_audio_dropped_late_pairs.
    const std::size_t frame_pairs = rate_ / 50;
    if (resumed_) {
        const auto drop = producer_->trim_late_output(frame, 4 * frame_pairs, 2 * frame_pairs);
        if (drop.pairs && !dropped_late_pairs_) {
            first_drop_pairs_ = drop.pairs;
            first_drop_peak_ = drop.peak;
        } else if (drop.pairs) {
            later_drop_pairs_ += drop.pairs;
            later_drop_peak_ = std::max(later_drop_peak_, drop.peak);
            ++later_drops_;
        }
        dropped_late_pairs_ += drop.pairs;
    }
    producer_->submit_frame(frame, words, std::move(cues), stop);
    if (callback_failed_) throw std::runtime_error("native audio device callback failed");
    // Two PAL frames of priming are a host latency policy. They never supply
    // producer clocks or controller words and are not original timing evidence.
    if (!resumed_ && producer_->source_pairs() >= 1282) {
        if (!SDL_ResumeAudioStreamDevice(device_)) throw std::runtime_error(SDL_GetError());
        resumed_ = true;
    }
}
void SdlTitleMenuAudio::set_paused(bool paused) {
    if (!resumed_) return;
    const bool changed =
        paused ? SDL_PauseAudioStreamDevice(device_) : SDL_ResumeAudioStreamDevice(device_);
    if (!changed) throw std::runtime_error(SDL_GetError());
}
void SdlTitleMenuAudio::report() {
    producer_->check_failure();
    std::cout << "native_audio_rate=" << rate_
              << " native_audio_source_pairs=" << producer_->source_pairs()
              << " native_audio_delivered_pairs=" << producer_->delivered_pairs()
              << " native_audio_nonzero_pairs=" << nonzero_pairs_.load()
              << " native_audio_underrun_pairs=" << underrun_pairs_.load()
              << " native_audio_dropped_late_pairs=" << dropped_late_pairs_
              << " native_audio_first_drop_pairs=" << first_drop_pairs_
              << " native_audio_first_drop_peak=" << first_drop_peak_
              << " native_audio_later_drops=" << later_drops_
              << " native_audio_later_drop_pairs=" << later_drop_pairs_
              << " native_audio_later_drop_peak=" << later_drop_peak_
              << " native_audio_navigation_count=" << producer_->navigation_count()
              << " native_audio_restart_count=" << producer_->restart_count()
              << " native_audio_cued_frames=" << producer_->cued_frames()
              << " native_audio_cued_stopped=" << producer_->cued_stopped() << '\n';
}
} // namespace unirally::app
