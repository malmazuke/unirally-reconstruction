#pragma once
// One SNES picture from video memory and the PPU registers, as the reference emulator (bsnes'
// fast PPU, `sfc/ppu-fast`) draws it: backgrounds in modes 0, 1 and 3, objects, colour math and
// the master brightness. The front end's screens are drawn with it (R-0054). Windows, mosaic,
// hires modes, mode 7 and offset-per-tile are not modelled; a picture that enables them is
// refused.
#include "presentation.hpp"

#include <array>
#include <cstdint>

namespace unirally {

struct SnesVideoMemory {
    std::array<std::uint8_t, 65536> vram{}; // byte addressed; a VRAM word is two bytes
    std::array<std::uint8_t, 512> cgram{};  // 256 BGR555 colours, little-endian
    std::array<std::uint8_t, 544> oam{};    // 128 four-byte entries, then 32 bytes of high bits
};

struct SnesBackground {
    std::uint16_t map_word{};  // BGnSC bits 7-2, as a word address
    std::uint8_t map_size{};   // BGnSC bits 1-0: 32x32, 64x32, 32x64, 64x64 tiles
    std::uint16_t tile_word{}; // BGnmNBA's nibble, as a word address
    bool large_tiles{};        // BGMODE bit 4 + n: 16x16 tiles
    std::uint16_t hofs{}, vofs{};
};

struct SnesVideoRegisters {
    bool force_blank{true};    // INIDISP bit 7
    std::uint8_t brightness{}; // INIDISP bits 3-0
    std::uint8_t mode{};       // BGMODE bits 2-0
    bool bg3_priority{};       // BGMODE bit 3 (mode 1)
    std::array<SnesBackground, 4> bg{};
    std::uint8_t obsel{};         // OBSEL: size (bits 7-5), name select, name base
    std::uint8_t main_screen{};   // TM: bits 0-3 BG1-4, bit 4 OBJ
    std::uint8_t sub_screen{};    // TS
    std::uint8_t colour_select{}; // CGWSEL
    std::uint8_t colour_math{};   // CGADSUB
    std::uint16_t fixed_colour{}; // COLDATA, as BGR555
    bool windows_or_mosaic{};     // any window or mosaic enable: refused
};

// The picture, in the output colours of the race presentation (`colour_word_rgb`).
RgbFrame render_snes_screen(const SnesVideoMemory& memory, const SnesVideoRegisters& registers);

} // namespace unirally
