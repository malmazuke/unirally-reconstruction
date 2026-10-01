#pragma once
#include "adapter.h"
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace unirally {
struct AudioDspState {
    std::uint64_t clocks = 0;
    std::vector<std::uint8_t> hardware;
    bool operator==(const AudioDspState&) const = default;
};
// DSP hardware only. One stereo pair per32 DSP clocks, at32040 pairs/second.
// Accesses catch up to the pinned fast-DSP batch boundary derived from SMP ticks.
class NativeAudioDsp {
public:
    NativeAudioDsp();
    void write_register(std::uint64_t smp_ticks, std::uint8_t reg, std::uint8_t value);
    void write_ram(std::uint64_t smp_ticks, std::uint16_t address, std::uint8_t value);
    void advance_to(std::uint64_t dsp_clocks);
    std::uint64_t clocks() const { return clocks_; }
    std::vector<std::int16_t> take_pcm();
    AudioDspState snapshot();
    void restore(const AudioDspState& state);

private:
    std::unique_ptr<UnirallyDsp, decltype(&unirally_dsp_destroy)> hardware_;
    std::uint64_t clocks_ = 0;
    std::vector<std::int16_t> pcm_;
};
} // namespace unirally
