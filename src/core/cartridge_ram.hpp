#pragma once

// The cartridge RAM (`$77:0000-1FFF`, 8 KiB, battery-backed) as the original keeps it, so that a
// native session can be saved and a later one, or the original, can carry on from it
// (SAVE-FILES, R-0090).
#include "front_end.hpp"

#include <cstdint>
#include <span>

namespace unirally {

// `$77:10AD`: the mode the main menu last chose; the boot clears it (`$83:8B3B`).
inline constexpr std::size_t one_player_mode = 0x10ad;
// `$77:10FD`: a pending tour reveal for each of the 16 riders; the boot clears them (`$83:8B41`).
inline constexpr std::size_t pending_reveals = 0x10fd, pending_reveal_riders = 16;

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

// The cartridge RAM a power-on without a saved image finds: every byte `$FF`, as in the reference
// core. Inserted like a saved image, its fields read as the original reads them before the boot's
// check wipes them (SAVE-IMAGE-EXACT, R-0111).
CartridgeImage power_on_cartridge();

// Before the first frame: the cartridge RAM the boot will find, its fields read at once, as the
// original reads them from power-on. At the boot's check an image with the signature keeps them
// (as after a soft reset); any other is wiped to a cold start's.
void insert_cartridge(FrontEndState& state, std::span<const std::uint8_t> image);

// `$77:0750` = `$77:0742`'s low byte as the image holds it (the menus' picks, R-0105, R-0111).
void store_race_mode_from_option_byte(FrontEndState& state);

// A word the original stores at a cartridge RAM offset it computes (`$80:F88D`, R-0111): into
// whichever field native keeps at each byte, else into the bytes native keeps no field for; an
// offset past the 8 KiB lands on its mirror, as the reference core maps banks `$77` and `$78`.
// The bytes the image derives from front-end state (`$0742` bits 1, 9 and 10, the pending
// reveals) are not patched.
void store_record_word(FrontEndState& state, std::size_t offset, std::uint16_t value);
// The same for one byte.
void store_record_byte(FrontEndState& state, std::size_t offset, std::uint8_t byte);

// A race update's increments of cartridge RAM bytes past rider 1's tallies
// (`ZoomZooState::cartridge_increments`, an overrun of the original; R-0098): each adds 1 to one
// byte of a saved best time, without a carry.
void add_race_cartridge_increments(FrontEndState& state, std::span<const std::uint16_t> offsets);

} // namespace unirally
