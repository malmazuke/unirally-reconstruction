// ROM-free checks pinning the recovered rider object and look mapping (R-0036).
// Synthetic tables stand in for the packed ROM ranges; each expected value
// follows the cited original routine, not the implementation under test.
#include "rider_look.hpp"
#include "rider_object.hpp"
#include "zoom_zoo_movement.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
using namespace unirally;

void require(bool value, const char *what) {
  if (!value)
    throw std::runtime_error(std::string("rider presentation assertion failed: ") + what);
}

template <typename Function> bool throws_invalid(Function &&function) {
  try {
    function();
  } catch (const std::invalid_argument &) {
    return true;
  }
  return false;
}

// Synthetic pack spans: one bank of pose frames ($23) and two tile banks
// ($27, $28). Tile n of each bank is filled with colour n & 15, so a copied
// tile identifies its source.
struct SyntheticObjects {
  std::vector<std::uint8_t> pointers, frames, tiles;
  RiderObjectContent content() const { return {pointers, frames, tiles}; }
  void add_pose(std::uint16_t address, std::uint8_t bank_offset) {
    pointers.push_back(static_cast<std::uint8_t>(address));
    pointers.push_back(static_cast<std::uint8_t>(address >> 8U));
    pointers.push_back(bank_offset);
  }
  void put_frame(std::uint16_t address, std::array<std::uint8_t, 4> header,
                 std::vector<std::uint16_t> words) {
    auto at = static_cast<std::size_t>(address - 0x8000U);
    for (auto byte : header)
      frames[at++] = byte;
    for (auto word : words) {
      frames[at++] = static_cast<std::uint8_t>(word);
      frames[at++] = static_cast<std::uint8_t>(word >> 8U);
    }
  }
};

SyntheticObjects synthetic_objects() {
  SyntheticObjects s;
  s.frames.assign(0x8000, 0);
  s.tiles.assign(2 * 0x8000, 0);
  for (unsigned tile = 0; tile < 2048; ++tile) {
    const auto colour = tile & 15U;
    for (unsigned row = 0; row < 8; ++row) {
      const auto at = tile * 32U + row * 2U;
      s.tiles[at] = (colour & 1U) ? 0xff : 0;
      s.tiles[at + 1] = (colour & 2U) ? 0xff : 0;
      s.tiles[at + 16] = (colour & 4U) ? 0xff : 0;
      s.tiles[at + 17] = (colour & 8U) ? 0xff : 0;
    }
  }
  return s;
}

// Tile reference word for bank $27 + bank_index, tile number `tile`.
std::uint16_t reference(unsigned bank_index, unsigned tile) {
  return static_cast<std::uint16_t>(((tile & 0xffU) << 8U) | (bank_index << 2U) | (tile >> 8U));
}

std::uint8_t object_pixel(const RiderObjectPixels &pixels, unsigned row, unsigned column) {
  return pixels[(row * 8U + 3U) * rider_object_size + column * 8U + 3U];
}

void tile_references() {
  const auto blank = decode_rider_tile_reference(0x0000);
  require(blank.bank == 0x27 && blank.address == 0x8000, "word 0 is $27:8000");
  // Low byte 0x05: bank index 1, tile high bits 01; high byte 0x02.
  const auto mixed = decode_rider_tile_reference(0x0205);
  require(mixed.bank == 0x28 && mixed.address == 0xa040, "tile 0x102 of bank $28");
  const auto last = decode_rider_tile_reference(0xffff);
  require(last.bank == 0x66 && last.address == 0xffe0, "last addressable tile");
}

void composition() {
  auto s = synthetic_objects();
  // Header 81 02 04 04: row 0 column 1, row 1 column 2, row 2 column 3,
  // row 3 column 4 and row 4 column 6 ($83:F2FF-$83:F3F8 bit extraction).
  s.add_pose(0x8000, 0);
  s.put_frame(0x8000, {0x81, 0x02, 0x04, 0x04},
              {reference(0, 1), reference(0, 2), reference(0, 3), reference(0, 4), reference(0, 6)});
  // Overlay covering row 0 columns 1 and 2 and row 1 column 2.
  s.add_pose(0x8020, 0);
  s.put_frame(0x8020, {0xc1, 0x00, 0x00, 0x00},
              {reference(0, 9), reference(0, 10), reference(0, 11)});
  const auto objects = s.content();

  const auto plain = compose_rider_object(objects, 0, std::nullopt, RiderRowClip::none);
  require(object_pixel(plain, 0, 1) == 1 && object_pixel(plain, 1, 2) == 2 &&
              object_pixel(plain, 2, 3) == 3 && object_pixel(plain, 3, 4) == 4 &&
              object_pixel(plain, 4, 6) == 6,
          "row masks place tiles in row-major order at columns 1-6");
  unsigned opaque_tiles = 0;
  for (unsigned row = 0; row < 8; ++row)
    for (unsigned column = 0; column < 8; ++column)
      opaque_tiles += object_pixel(plain, row, column) != 0;
  require(opaque_tiles == 5, "only masked slots are drawn");

  // The overlay replaces row 0 column 1 and row 1 column 2, consuming the
  // covered pose words, and adds row 0 column 2.
  const auto overlaid = compose_rider_object(objects, 0, std::uint16_t{1}, RiderRowClip::none);
  require(object_pixel(overlaid, 0, 1) == 9 && object_pixel(overlaid, 0, 2) == 10 &&
              object_pixel(overlaid, 1, 2) == 11,
          "overlay tiles take precedence");
  require(object_pixel(overlaid, 2, 3) == 3 && object_pixel(overlaid, 4, 6) == 6,
          "covered pose words are skipped, later words stay aligned");

  const auto top = compose_rider_object(objects, 0, std::nullopt, RiderRowClip::top_row);
  require(object_pixel(top, 0, 1) == 0 && object_pixel(top, 1, 2) == 2,
          "top-row clip drops row 0 and skips its words");
  const auto bottom = compose_rider_object(objects, 0, std::nullopt, RiderRowClip::bottom_row);
  require(object_pixel(bottom, 4, 6) == 0 && object_pixel(bottom, 3, 4) == 4,
          "bottom-row clip drops row 4");

  require(throws_invalid([&] { (void)compose_rider_object(objects, 2, std::nullopt, RiderRowClip::none); }),
          "pose beyond the pointer table fails closed");
  s.add_pose(0x8040, 1); // bank $24 is not packed
  const auto missing_bank = s.content();
  require(throws_invalid([&] { (void)compose_rider_object(missing_bank, 2, std::nullopt, RiderRowClip::none); }),
          "frame outside the packed frames fails closed");
  s.add_pose(0x8060, 0);
  s.put_frame(0x8060, {0x80, 0x00, 0x00, 0x00}, {reference(2, 0)}); // bank $29 absent
  const auto missing_tile = s.content();
  require(throws_invalid([&] { (void)compose_rider_object(missing_tile, 3, std::nullopt, RiderRowClip::none); }),
          "tile outside the packed tiles fails closed");
}

void oam_projection() {
  // $81:A445 (ZOOM ZOO) and $81:A4C1 (DRAGSTER) playfield sets.
  const TrackGeometry zoom{256, 0x3fff, 2, -0x60, 0x64, -0xc4, 0x400};
  const TrackGeometry dragster{1024, 0xffff, 0, -0x18, 0x19, -0x31, 0x100};
  const auto at = [&](int dx, int dy, bool reflected = false) {
    return project_rider_oam(static_cast<std::uint16_t>(1000 + dx), static_cast<std::uint16_t>(2000 + dy),
                             1000, 2000, reflected, zoom);
  };
  const auto dragster_at = [&](int dx) {
    return project_rider_oam(static_cast<std::uint16_t>(1000 + dx), 2000, 1000, 2000, false, dragster);
  };
  // DRAGSTER scales X by 1: visible from -49 through 255 ($0425/$0427).
  require(dragster_at(-49).visible && dragster_at(-49).x == -49 && !dragster_at(-50).visible,
          "DRAGSTER lower X limit $FFCF unscaled");
  require(dragster_at(255).visible && dragster_at(255).x == 255 && !dragster_at(256).visible,
          "DRAGSTER upper X limit $0100 unscaled");
  // Frame 1700 of the primary timeline: player 8576,1520 with camera 8440,1430
  // is OAM entry 98 X 136, Y 90, attribute $26 (no flip).
  const auto observed = project_rider_oam(8576, 1520, 8440, 1430, false, zoom);
  require(observed.visible && observed.x == 136 && observed.y == 90 && !observed.horizontal_flip &&
              observed.clip == RiderRowClip::none,
          "observed primary OAM");
  require(at(0, 0, true).horizontal_flip, "reflection sets the horizontal flip");
  require(at(0, -41).visible && !at(0, -42).visible, "$82:AD03 upper Y limit");
  require(at(0, 224).visible && !at(0, 225).visible, "$82:AD08 lower Y limit");
  require(at(0, -33).clip == RiderRowClip::top_row && at(0, -32).clip == RiderRowClip::none,
          "$82:AD96 top-row clip threshold");
  require(at(0, 207).clip == RiderRowClip::none && at(0, 208).clip == RiderRowClip::bottom_row,
          "$82:AD9B bottom-row clip threshold");
  require(at(0, -41).y == 0xd7, "Y keeps the low byte");
  require(at(-49, 0).visible && at(-49, 0).x == -49 && !at(-50, 0).visible,
          "scaled X lower limit $FF3C and ninth X bit");
  require(at(255, 0).visible && at(255, 0).x == 255 && !at(256, 0).visible, "scaled X upper limit $0400");
}

void drawing() {
  RiderObjectPixels pixels{};
  pixels[0] = 5;                          // object (0,0)
  pixels[63 * rider_object_size + 63] = 7; // object (63,63)
  std::vector<std::array<int, 3>> plotted;
  const auto collect = [&](int x, int y, std::uint8_t value) { plotted.push_back({x, y, value}); };
  RiderOam oam{true, 10, 20, false, RiderRowClip::none};
  draw_rider_object(pixels, oam, collect);
  require(plotted.size() == 2 && plotted[0] == std::array<int, 3>{10, 20, 5} &&
              plotted[1] == std::array<int, 3>{73, 83, 7},
          "unflipped placement");
  plotted.clear();
  oam.horizontal_flip = true;
  draw_rider_object(pixels, oam, collect);
  require(plotted.size() == 2 && plotted[0] == std::array<int, 3>{73, 20, 5} &&
              plotted[1] == std::array<int, 3>{10, 83, 7},
          "horizontal flip mirrors the 64-pixel object");
  plotted.clear();
  oam = {true, -60, 250, false, RiderRowClip::none};
  draw_rider_object(pixels, oam, collect);
  // Object row 63 wraps to screen row 57; column 63 lands on screen X 3.
  require(plotted.size() == 1 && plotted[0] == std::array<int, 3>{3, 57, 7},
          "vertical wrap at 256 and left-edge clipping");
  plotted.clear();
  oam.visible = false;
  draw_rider_object(pixels, oam, collect);
  require(plotted.empty(), "hidden objects draw nothing");
}

void head_steps() {
  const auto step = [](std::uint16_t head, std::uint16_t target) {
    RiderLook look;
    look.head = head;
    look.target = target;
    step_rider_head(look);
    return look.head;
  };
  require(step(0, 5) == 8 && step(8, 5) == 7 && step(4, 5) == 5, "arc 1 steps toward the target");
  require(step(8, 0) == 0 && step(10, 0) == 0, "reaching neutral 9 stores 0");
  require(step(0, 20) == 0x12 && step(0x12, 20) == 0x13, "arc 2 joins at 9/18");
  require(step(0x14, 3) == 0x13 && step(0x12, 3) == 0, "leaving arc 2 returns through 18 to 9");
  require(step(3, 30) == 2 && step(2, 30) == 0x1a && step(0x1a, 30) == 0x1b, "arc 3 joins at 2/26");
  require(step(0, 40) == 8 && step(7, 40) == 0x22 && step(0x22, 40) == 0x23, "arc 4 joins at 7/34");
  require(step(0x1b, 40) == 0x1a && step(0x1a, 40) == 2, "crossing arcs passes through the junctions");
}

void overlay_selection() {
  std::vector<std::uint8_t> bytes(594, 0);
  for (unsigned i = 0; i < 48; ++i) { // $83:EC2E side bases: head index * 18
    bytes[48 + 2 * i] = static_cast<std::uint8_t>(i * 18U);
    bytes[49 + 2 * i] = static_cast<std::uint8_t>((i * 18U) >> 8U);
  }
  const RiderLookTables tables{bytes};
  RiderLookState look;
  ZoomZooState updated;
  const auto overlay = [&](std::uint16_t head, std::uint16_t orientation, std::uint16_t override_pose = 0) {
    look.riders[1].head = head;
    updated.movement.riders[1].pose.reflected_orientation = orientation;
    updated.reflection[1].pose_override = override_pose;
    return rider_overlay_poses(look, updated, tables)[1];
  };
  require(!overlay(0, 5), "neutral head has no overlay");
  require(!overlay(3, 5, 0x0620), "a pose override suppresses the overlay");
  require(overlay(3, 5) == 0x0aec + 2 * 32 + 5, "front orientations below $10");
  require(overlay(3, 0x31) == 0x0aec + 2 * 32 + 0x10, "front orientations from $31");
  require(overlay(3, 0x12) == 2 * 18 + 2 + 0x100c, "side orientations $10-$18");
  require(overlay(3, 0x2a) == 2 * 18 + 0x0b + 0x100c, "side orientations $28-$30");
  require(!overlay(3, 0x19) && !overlay(3, 0x27), "orientations $19-$27 have no overlay");
  require(!rider_overlay_poses(look, updated, tables)[0], "rider 0 is independent");
}

void pause_updates() {
  // The engine's suspended-update clock advances on every diverted update:
  // opening the menu, holding it open, resuming, and holding Start after the
  // resume with the selection already zero (ZOOM ZOO pause-countdown original).
  ZoomZooState racing, opened, held, resumed, start_held;
  opened.pause.selection = 1;
  opened.pause.suspended_updates = 1;
  held = opened;
  held.pause.suspended_updates = 2;
  resumed.pause.suspended_updates = 3;
  start_held.pause.suspended_updates = 4;
  start_held.pause.released = 1;
  require(!zoom_zoo_update_was_paused(racing, racing), "racing update");
  require(zoom_zoo_update_was_paused(racing, opened) && zoom_zoo_update_was_paused(opened, held) &&
              zoom_zoo_update_was_paused(held, resumed),
          "entering, holding and leaving the pause menu divert the update");
  require(zoom_zoo_update_was_paused(resumed, start_held), "Start still held after the resume diverts the update");
  require(!zoom_zoo_update_was_paused(start_held, start_held), "the first update after Start is released is not diverted");
}

} // namespace

int main() try {
  tile_references();
  composition();
  oam_projection();
  drawing();
  head_steps();
  overlay_selection();
  pause_updates();
  return 0;
} catch (const std::exception &error) {
  std::cerr << error.what() << '\n';
  return 1;
}
