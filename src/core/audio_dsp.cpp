#include "audio_dsp.hpp"
#include <array>
#include <stdexcept>
#include <utility>

namespace unirally {
NativeAudioDsp::NativeAudioDsp() : hardware_(unirally_dsp_create(), unirally_dsp_destroy) {
    if (!hardware_) throw std::runtime_error("cannot initialize native audio DSP");
}
void NativeAudioDsp::advance_to(std::uint64_t target) {
    if (target < clocks_ || target % 32) throw std::invalid_argument("invalid DSP pair boundary");
    std::array<std::int16_t, 2> pair{};
    while (clocks_ < target) {
        if (unirally_dsp_run(hardware_.get(), 32, pair.data()) != 2)
            throw std::runtime_error("native audio DSP pair phase differs");
        pcm_.insert(pcm_.end(), pair.begin(), pair.end());
        clocks_ += 32;
    }
}
void NativeAudioDsp::write_register(std::uint64_t ticks, std::uint8_t reg, std::uint8_t value) {
    advance_to(((ticks + 63) / 64) * 32);
    unirally_dsp_write_register(hardware_.get(), reg, value);
}
void NativeAudioDsp::write_ram(std::uint64_t ticks, std::uint16_t address, std::uint8_t value) {
    advance_to(((ticks + 63) / 64) * 32);
    unirally_dsp_write_ram(hardware_.get(), address, value);
}
std::vector<std::int16_t> NativeAudioDsp::take_pcm() {
    return std::exchange(pcm_, {});
}
AudioDspState NativeAudioDsp::snapshot() {
    AudioDspState state{clocks_, std::vector<std::uint8_t>(unirally_dsp_state_size())};
    if (unirally_dsp_save(hardware_.get(), state.hardware.data(), state.hardware.size()) != 1)
        throw std::runtime_error("cannot save native audio DSP");
    return state;
}
void NativeAudioDsp::restore(const AudioDspState& state) {
    if (state.clocks % 32
        || unirally_dsp_load(hardware_.get(), state.hardware.data(), state.hardware.size()) != 1)
        throw std::invalid_argument("invalid native audio DSP continuation");
    clocks_ = state.clocks;
    pcm_.clear();
}
} // namespace unirally
