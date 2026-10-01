#include "sdl_audio.hpp"
#include <iostream>
#include <stdexcept>

namespace unirally::app {
SdlTitleMenuAudio::SdlTitleMenuAudio(const ClassicContentPack& pack) {
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
void SdlTitleMenuAudio::submit_frame(std::uint32_t frame, std::array<std::uint16_t, 2> words) {
    producer_->submit_frame(frame, words);
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
              << " native_audio_navigation_count=" << producer_->navigation_count() << '\n';
}
} // namespace unirally::app
