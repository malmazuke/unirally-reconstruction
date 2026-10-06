#pragma once
#include "title_menu_audio_stream.hpp"
#include <SDL3/SDL.h>
#include <atomic>
#include <memory>

namespace unirally::app {
class SdlTitleMenuAudio {
public:
    explicit SdlTitleMenuAudio(const ClassicContentPack& pack);
    ~SdlTitleMenuAudio();
    void submit_frame(std::uint32_t frame, std::array<std::uint16_t, 2> words,
                      AudioCueList cues = {}, bool stop = false);
    void report();
    void set_paused(bool paused);

private:
    SDL_AudioStream* device_ = nullptr;
    std::unique_ptr<TitleMenuAudioStream> producer_;
    std::atomic<bool> callback_failed_{false};
    std::atomic<std::uint64_t> underrun_pairs_{0}, nonzero_pairs_{0};
    // The first late drop (the boot's) apart from every later one: pairs, peak level and count.
    std::uint64_t dropped_late_pairs_ = 0, first_drop_pairs_ = 0, later_drop_pairs_ = 0;
    std::uint16_t first_drop_peak_ = 0, later_drop_peak_ = 0;
    std::uint32_t later_drops_ = 0;
    std::uint32_t rate_ = 0;
    bool resumed_ = false;
    static void SDLCALL callback(void* context, SDL_AudioStream* stream, int additional_bytes,
                                 int total_bytes);
};
} // namespace unirally::app
