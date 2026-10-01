#pragma once
// Laboratory coupling of native driver work to DSP-only hardware. CPU bus
// inputs and the final IPL entry remain conditions; this is not an app path.
#include "adapter.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <memory>
#include <stdexcept>

class DriverDspDiagnostic {
public:
    DriverDspDiagnostic(const char* path, std::uint64_t clock_limit)
        : hardware_(unirally_dsp_create(), unirally_dsp_destroy),
          output_(path, std::ios::binary),
          limit_(clock_limit) {
        if (!hardware_ || !output_ || limit_ % 32)
            throw std::runtime_error("invalid DSP diagnostic output");
    }
    void write_register(std::uint64_t smp, std::uint8_t reg, std::uint8_t value) {
        if (!advance_for_access(smp)) return;
        unirally_dsp_write_register(hardware_.get(), reg, value);
    }
    void write_ram(std::uint64_t smp, std::uint16_t address, std::uint8_t value) {
        if (!advance_for_access(smp)) return;
        unirally_dsp_write_ram(hardware_.get(), address, value);
    }
    void finish() {
        advance_to(limit_);
        if (!output_) throw std::runtime_error("PCM output failed");
    }

private:
    std::unique_ptr<UnirallyDsp, decltype(&unirally_dsp_destroy)> hardware_;
    std::ofstream output_;
    std::uint64_t clocks_ = 0, limit_;
    bool advance_for_access(std::uint64_t smp) {
        const auto completed = ((smp + 63) / 64) * 32;
        if (completed > limit_) return false;
        advance_to(completed);
        return true;
    }
    void advance_to(std::uint64_t target) {
        if (target < clocks_) throw std::runtime_error("DSP clock moved backwards");
        std::array<std::int16_t, 2> pair{};
        while (clocks_ < target) {
            const auto work =
                static_cast<std::uint32_t>(std::min<std::uint64_t>(32, target - clocks_));
            const int count = unirally_dsp_run(hardware_.get(), work, pair.data());
            if (count != 2) throw std::runtime_error("DSP pair phase mismatch");
            for (const auto sample : pair) {
                const auto value = static_cast<std::uint16_t>(sample);
                output_.put(static_cast<char>(value & 255));
                output_.put(static_cast<char>(value >> 8));
            }
            clocks_ += work;
        }
    }
};
