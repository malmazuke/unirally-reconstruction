#pragma once
// The laboratory's per-frame outputs, in the one form `front_end_runner` and the app (`unirally`)
// both write, so a scripted app run compares with the runner line for line (APP-PARITY, R-0110).

#include "presentation.hpp"
#include "zoom_zoo_movement.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace unirally {

// One race timeline row: "FRAME HEX\n", the race's serialized state after that frame's update
// (`--race-timeline`, `--one-player-timeline`).
std::string race_state_row(std::uint32_t frame, const ZoomZooState& state);

// A picture as `--picture` writes it: a binary PPM, 256x224.
std::vector<std::uint8_t> ppm_picture(const RgbFrame& frame);

// 64-bit FNV-1a: a regression check between two runs, not a security hash.
std::uint64_t output_digest(std::span<const std::uint8_t> bytes);

// One `--frame-digests` line: "FRAME KIND DIGEST\n", the digest in 16 hex digits. KIND is `P`
// (a picture's PPM), `V` (the VRAM) or `R` (the cartridge RAM).
std::string digest_line(std::uint32_t frame, char kind, std::span<const std::uint8_t> bytes);

} // namespace unirally
