#pragma once

// The cartridge RAM (`$77:0000-1FFF`, 8 KiB, battery-backed) as the original keeps it, so that a
// native session can be saved and a later one, or the original, can carry on from it
// (SAVE-FILES, R-0090).
#include "front_end.hpp"

#include <span>

namespace unirally {

// `$77:10AD`: the mode the main menu last chose; the boot clears it (`$83:8B3B`).
inline constexpr std::size_t one_player_mode = 0x10ad;

// The image a cold start leaves on boot frame 405: the wipe (`$83:FB41`) and the defaults
// (`$80:8C80-8CC2`), signature first.
CartridgeImage cold_start_cartridge(const FrontEndContent& content);

// Whether the image starts with the signature `$83:8000` the boot checks (`$80:8C4E`).
bool has_signature(const CartridgeImage& image, const FrontEndContent& content);

// The image of the native state: the bytes native keeps no field for as they were loaded or as
// the cold start left them, with every field native keeps written at its address.
CartridgeImage cartridge_image(const FrontEndState& state);

// The records' fields read from an image.
OnePlayerRecords records_from_cartridge(const CartridgeImage& image);

// Before the first frame: the cartridge RAM the boot will find, its fields read at once, as the
// original reads them from power-on. At the boot's check an image with the signature keeps them
// (as after a soft reset); any other is wiped to a cold start's.
void insert_cartridge(FrontEndState& state, std::span<const std::uint8_t> image);

} // namespace unirally
