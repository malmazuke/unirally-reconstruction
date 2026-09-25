// FRONT-END-MAIN-MENU (R-0054) and FRONT-END-1P-SETUP (R-0055): the text
// printer, the SNES screen, the main menu's and the rider menu's rules, on
// synthetic content (no ROM). The captures' frame-by-frame agreement is the
// laboratory's.
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
  // A colour written during the picture shows from its row on: the object
  // (drawn on row 0) turns red for a write on row 0, not for one on row 1.
  const std::array<unirally::SnesLineColour, 1> later{{{1, 129, 0x001f}}};
  frame = unirally::render_snes_screen(memory, registers, later);
  require(frame.pixels[0] == 0 && frame.pixels[2] == 255);
  const std::array<unirally::SnesLineColour, 1> first{{{0, 129, 0x001f}}};
  frame = unirally::render_snes_screen(memory, registers, first);
  require(frame.pixels[0] == 255 && frame.pixels[2] == 0);
  require(memory.cgram[129 * 2] == 0); // the memory itself is not changed
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
  // The rider menu: palettes (rider r's colour 1 is r + 1), 22 names "r" and
  // their 0xFF, a title, the animator's tables and blank uni pictures (every
  // picture at $23:8000, an empty mask; the blank tile at $27:8000).
  for (unsigned id = 6; id <= 21; ++id) {
    storage.emplace_back(32, 0);
    storage.back()[2] = static_cast<std::uint8_t>(id - 5);
    content.assets[id] = storage.back();
  }
  storage.emplace_back(22 * 16, 0);
  for (std::size_t r = 0; r < 22; ++r) {
    storage.back()[r * 16] = 'A';
    storage.back()[r * 16 + 1] = 0xff;
  }
  content.rider_names = storage.back();
  storage.push_back({0xfc, 0x01, 'A', 0xff});
  content.rider_menu_title = storage.back();
  content.decoration_frames = keep(76);
  storage.emplace_back(0x1321 * 3, 0);
  for (std::size_t k = 0; k < 0x1321; ++k)
    storage.back()[k * 3 + 1] = 0x80;
  content.uni_pictures.pose_pointers = storage.back();
  content.uni_pictures.pose_frames = keep(4);
  content.uni_pictures.object_tiles = keep(32);
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
  require(state.screen == unirally::FrontEndScreen::main_menu && !state.mode_chosen);
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
  // Controller 2's A chooses the selected mode: 1P opens the rider menu.
  run(state, content, 1, {0, 0x0080});
  require(!state.mode_chosen &&
          state.screen == unirally::FrontEndScreen::rider_menu_entry);
  // Left alone, the main menu starts the demo after 481 of its frames.
  auto idle = unirally::start_front_end();
  run(idle, content, 900);
  require(!idle.mode_chosen);
  run(idle, content, 1);
  require(idle.mode_chosen && idle.mode == unirally::FrontEndMode::demo &&
          idle.frame == 901);
}

void rider_menu_tests() {
  using unirally::FrontEndScreen;
  std::vector<std::vector<std::uint8_t>> storage;
  const auto content = synthetic_content(storage);
  auto state = unirally::start_front_end();
  run(state, content, 430);
  run(state, content, 1, {0x1000, 0}); // 1P
  // The names slide in over 42 frames; the logo has slid up; the arrow aims at
  // MIKE, the last rider chosen (none yet).
  run(state, content, 41);
  require(state.screen == FrontEndScreen::rider_menu_entry);
  run(state, content, 1);
  require(state.screen == FrontEndScreen::rider_menu);
  require(state.slide.scroll == 256 && state.slide.shown_half == 0x1400);
  require(state.logo.offset == 0x52 && state.registers.bg[0].vofs == 0x52);
  require(state.arrow.target_x == 0x0680 && state.arrow.target_y == 0x0290);
  require(state.text.words[4 * 32 + 5] != 0x004c); // MIKE's name
  // Each frame of the menu splits the palettes: colour 0, then 128 rows.
  run(state, content, 1);
  require(state.line_colours.size() == 129 &&
          state.line_colours[1].row == 64 && state.line_colours[1].index == 0x80);
  // Controller 2 is not read; Down moves once a press and stops at row 7.
  run(state, content, 1, {0, 0x0400});
  require(state.rider_menu.row == 0);
  for (int k = 0; k < 9; ++k) {
    run(state, content, 1, {0x0400, 0});
    run(state, content, 1);
  }
  require(state.rider_menu.row == 7 && state.arrow.target_y == 0x0290 + 7 * 0x0180);
  // Up at row 0 stays; Right changes column (and the arrow's mirror and palette).
  for (int k = 0; k < 8; ++k) {
    run(state, content, 1, {0x0800, 0});
    run(state, content, 1);
  }
  require(state.rider_menu.row == 0);
  run(state, content, 1, {0x0100, 0});
  require(state.arrow.target_x == 0x0780 && (state.oam_buffer[119 * 4 + 3] & 0x40) == 0);
  run(state, content, 1, {0x0400, 0});
  run(state, content, 1);
  // Y goes back: three frames out, then 42 to the main menu, on 1P again.
  run(state, content, 1, {0x4000, 0});
  require(state.screen == FrontEndScreen::rider_menu_exit && state.line_colours.empty());
  run(state, content, 3);
  require(state.screen == FrontEndScreen::main_menu_return);
  run(state, content, 42);
  require(state.screen == FrontEndScreen::main_menu && state.menu.selection == 0 &&
          state.menu.idle == 480 && state.slide.scroll == 0 && !state.mode_chosen);
  // Choosing again starts on MIKE; B chooses the rider under the arrow.
  run(state, content, 1, {0x1000, 0});
  run(state, content, 43);
  run(state, content, 1, {0x0100, 0});
  run(state, content, 1, {0x8000, 0});
  require(state.screen == FrontEndScreen::rider_menu_exit && !state.mode_chosen);
  run(state, content, 3);
  require(state.mode_chosen && state.mode == unirally::FrontEndMode::one_player &&
          state.rider_menu.rider == 1);
}

} // namespace

int main() try {
  text_printer_tests();
  snes_screen_tests();
  main_menu_tests();
  rider_menu_tests();
  return 0;
} catch (const std::exception &error) {
  std::fprintf(stderr, "%s\n", error.what());
  return 1;
}
