// FRONT-END-MAIN-MENU (R-0054): the text printer, the SNES screen and the main
// menu's rules, on synthetic content (no ROM). The captures' frame-by-frame
// agreement is the laboratory's.
#include "front_end.hpp"
#include "snes_screen.hpp"
#include "text_printer.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

int require_count = 0;
void require(bool value) {
  ++require_count;
  if (!value)
    throw std::runtime_error("front-end assertion failed at require #" +
                             std::to_string(require_count));
}

template <typename Action> bool refuses(Action action) {
  try {
    action();
  } catch (const std::invalid_argument &) {
    return true;
  }
  return false;
}

void text_printer_tests() {
  // A table with 'A' big (entry 10) and '!' small (entry 0x81), as $80:C709 has
  // them.
  std::array<std::uint8_t, 256> table{};
  table['A'] = 10;
  table['!'] = 0x81;
  table['_'] = 0xaf;
  table[0x64] = 0xe4; // the special small entry 0x64
  unirally::TextMap map;
  unirally::TextCursor cursor;
  // F9 07: palette 7; FC 02: centre on row 2; "A!" is 3 tiles wide, so column
  // (33 - 3) / 2 = 15.
  const std::array<std::uint8_t, 7> stream{0xf9, 0x07, 0xfc, 0x02,
                                           'A',  '!',  0xff};
  unirally::print_text(map, cursor, stream, table);
  require(map.words[2 * 32 + 15] == (0x3c00 | 20));
  require(map.words[2 * 32 + 16] == (0x3c00 | 21));
  require(map.words[3 * 32 + 15] == (0x3c00 | (20 + 0x50)));
  require(map.words[3 * 32 + 16] == (0x3c00 | (20 + 0x51)));
  require(map.words[2 * 32 + 17] == (0x3c00 | 0xa0));
  require(map.words[3 * 32 + 17] == (0x3c00 | (0xa0 + 0x3c)));
  require(cursor.position == 2 * 32 + 18);
  // FE column row positions; entry 0x64's path adds the carry of its equal
  // compare and sets 0x6000 in place of the priority bit.
  unirally::TextCursor special;
  const std::array<std::uint8_t, 5> positioned{0xfe, 3, 5, 0x64, 0xff};
  unirally::print_text(map, special, positioned, table);
  require(map.words[5 * 32 + 3] == (0x6000 | 0xa4));
  require(map.words[6 * 32 + 3] == (0x6000 | (0xa4 + 0x3c)));
  // An unrecovered control code and a stream without its end are refused.
  const std::array<std::uint8_t, 2> unknown{0xf0, 0xff};
  const std::array<std::uint8_t, 1> unterminated{'A'};
  require(refuses([&] { unirally::print_text(map, cursor, unknown, table); }));
  require(
      refuses([&] { unirally::print_text(map, cursor, unterminated, table); }));
}

void set_word(unirally::SnesVideoMemory &memory, unsigned word,
              std::uint16_t value) {
  memory.vram[word * 2] = static_cast<std::uint8_t>(value);
  memory.vram[word * 2 + 1] = static_cast<std::uint8_t>(value >> 8U);
}

void snes_screen_tests() {
  unirally::SnesVideoMemory memory;
  unirally::SnesVideoRegisters registers;
  registers.force_blank = false;
  registers.brightness = 15;
  registers.mode = 3;
  registers.bg[1].map_word = 0x1000;
  registers.bg[1].tile_word = 0x2000;
  registers.main_screen = 0x02;
  // BG2 (4bpp): map entry 0 is tile 1, palette 1; the tile's first row is
  // colour 1 everywhere.
  set_word(memory, 0x1000, 0x0401);
  set_word(memory, 0x2000 + 16, 0x00ff);
  memory.cgram[(16 + 1) * 2] = 0x1f; // colour 17: red 31
  auto frame = unirally::render_snes_screen(memory, registers);
  // Screen row 0 is scanline 1, so the tile's first row is not the top row.
  require(frame.pixels[0] == 0 && frame.pixels[1] == 0);
  registers.bg[1].vofs =
      0xffff; // one line up: row 0 shows the tile's first row
  frame = unirally::render_snes_screen(memory, registers);
  require(frame.pixels[0] == 255 && frame.pixels[1] == 0 &&
          frame.pixels[2] == 0);
  // Forced blank draws black; windows are refused.
  registers.force_blank = true;
  require(unirally::render_snes_screen(memory, registers).pixels[0] == 0);
  registers.force_blank = false;
  registers.unmodelled_features = true;
  require(
      refuses([&] { (void)unirally::render_snes_screen(memory, registers); }));
  registers.unmodelled_features = false;
  // An object on the subscreen, added at half where it covers the main screen
  // (CGWSEL 2, CGADSUB 0x42: BG2, halve): red 31 + blue 31 halved is (15, 0,
  // 15).
  registers.obsel = 0x00; // 8x8 objects at word 0
  registers.sub_screen = 0x10;
  registers.colour_select = 0x02;
  registers.colour_math = 0x42;
  memory.oam[0] = 0; // x
  memory.oam[1] = 0; // y 0: drawn on scanlines 1-8, screen rows 0-7
  memory.oam[2] = 2; // tile 2
  memory.oam[3] = 0; // palette 0 (colours 128-143)
  memory.oam[512] = 0;
  set_word(memory, 2 * 16, 0x00ff);
  memory.cgram[(128 + 1) * 2 + 1] = 0x7c; // colour 129: blue 31
  frame = unirally::render_snes_screen(memory, registers);
  require(frame.pixels[0] == frame.pixels[2] && frame.pixels[1] == 0 &&
          frame.pixels[0] > 0 && frame.pixels[0] < 255);
  // Clipping the main screen to black always (CGWSEL bits 7-6 = 3) also stops the halving, as
  // in bsnes (`halve && windowAbove[x]`): black + blue 31 is full blue.
  registers.colour_select = 0xc2;
  frame = unirally::render_snes_screen(memory, registers);
  require(frame.pixels[0] == 0 && frame.pixels[1] == 0 && frame.pixels[2] == 255);
}

unirally::FrontEndContent
synthetic_content(std::vector<std::vector<std::uint8_t>> &storage) {
  unirally::FrontEndContent content;
  const auto keep = [&](std::size_t size) -> std::span<const std::uint8_t> {
    storage.emplace_back(size, 0);
    return storage.back();
  };
  for (const unsigned id : {1U, 2U, 5U, 27U, 28U, 31U, 68U, 69U, 70U, 72U, 74U,
                            77U, 78U, 80U, 88U, 89U, 91U})
    content.assets[id] = keep(id == 69 ? 13568 : 64);
  content.base_palette = keep(216);
  storage.emplace_back(256, 0);
  storage.back()['A'] = 10;
  content.character_table = storage.back();
  storage.push_back({0xf9, 0x07, 0xfc, 0x0a, 'A', 0xff});
  content.main_menu_text = storage.back();
  content.arrow_frames = keep(16);
  storage.push_back({10, 10, 10, 6, 5});
  content.menu_arrow_columns = storage.back();
  content.cycle_colours = keep(14);
  return content;
}

void run(unirally::FrontEndState &state,
         const unirally::FrontEndContent &content, std::uint32_t frames,
         unirally::FrontEndPads pads = {}) {
  for (std::uint32_t k = 0; k < frames; ++k)
    unirally::update_front_end(state, content, pads);
}

void main_menu_tests() {
  std::vector<std::vector<std::uint8_t>> storage;
  const auto content = synthetic_content(storage);
  auto state = unirally::start_front_end();
  run(state, content, 420);
  require(state.in_main_menu && !state.mode_chosen);
  require(state.menu.idle == 480 && state.menu.selection == 0);
  // The arrow flies in and settles three sixteenths short of its x target
  // (R-0054).
  run(state, content, 60);
  require(state.arrow.x == 0x04fa && state.arrow.target_x == 0x04fd);
  require(state.arrow.y == 0x0580);
  // A held Down moves once; a second move needs a release.
  run(state, content, 10, {0x0400, 0});
  require(state.menu.selection == 1 && state.menu.idle == 1500 - 9);
  run(state, content, 1,
      {0x0400, 0x0800}); // controller 2's Up while Down is held: latched
  require(state.menu.selection == 1);
  run(state, content, 1);
  require(!state.menu.move_latched);
  // Opposing directions read as neither (the D-pad's rocker).
  run(state, content, 1, {0x0c00, 0});
  require(state.menu.selection == 1 && !state.menu.move_latched);
  // Up past 1P wraps to OPTIONS; Select moves down and wraps back to 1P.
  run(state, content, 1, {0x0800, 0});
  run(state, content, 1);
  run(state, content, 1, {0x0800, 0});
  require(state.menu.selection == 4 && state.arrow.target_y == 0x0b80);
  require(state.arrow.target_x == 5U << 7U);
  run(state, content, 1);
  run(state, content, 1, {0x2000, 0});
  require(state.menu.selection == 0 && state.arrow.target_y == 0x0580);
  // Controller 2's A chooses the selected mode.
  run(state, content, 1, {0, 0x0080});
  require(state.mode_chosen &&
          state.mode == unirally::FrontEndMode::one_player);
  // Left alone, the main menu starts the demo after 481 of its frames.
  auto idle = unirally::start_front_end();
  run(idle, content, 900);
  require(!idle.mode_chosen);
  run(idle, content, 1);
  require(idle.mode_chosen && idle.mode == unirally::FrontEndMode::demo &&
          idle.frame == 901);
}

} // namespace

int main() try {
  text_printer_tests();
  snes_screen_tests();
  main_menu_tests();
  return 0;
} catch (const std::exception &error) {
  std::fprintf(stderr, "%s\n", error.what());
  return 1;
}
