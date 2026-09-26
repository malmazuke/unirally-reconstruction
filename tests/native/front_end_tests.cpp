// FRONT-END-MAIN-MENU (R-0054), FRONT-END-1P-SETUP (R-0055, R-0056) and
// FRONT-END-1P-CONTINUATION (R-0057): the text printer, the SNES screen, the
// main menu's and the one-player screens' rules and the one-run result, on
// synthetic content (no ROM). The captures' frame-by-frame agreement
// is the laboratory's.
#include "front_end.hpp"
#include "snes_screen.hpp"
#include "text_printer.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <source_location>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void require(bool value,
             std::source_location where = std::source_location::current()) {
  if (!value)
    throw std::runtime_error("front-end assertion failed at line " +
                             std::to_string(where.line()));
}

template <typename Action> bool throws_logic(Action action) {
  try {
    action();
  } catch (const std::logic_error &) {
    return true;
  }
  return false;
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

// R-0056: the codes that read the game's state, and the number and time
// formats.
void text_variable_tests() {
  std::array<std::uint8_t, 256> table{};
  table['A'] = 10;
  table['B'] = 11;
  table['1'] = 12;
  table['2'] = 13;
  table['_'] = 0xaf;
  unirally::TextVariables variables;
  variables.word = [](std::uint16_t address) -> std::uint16_t {
    return address == 0xb2 ? 1 : 12;
  };
  const std::array<std::uint8_t, 5> names{'A', 0xff, 'B', 'B', 0xff};
  variables.track_names = names;
  unsigned placed_object = 99, placed_at = 0;
  variables.place_object = [&](unsigned object, unsigned position) {
    placed_object = object;
    placed_at = position;
  };
  // F7: track 1's name ("BB") at the cursor; with FC after it, centred on that
  // row alone.
  unirally::TextMap map;
  unirally::TextCursor cursor;
  const std::array<std::uint8_t, 7> track{0xfe, 1, 2, 0xf7, 0xb2, 0x00, 0xff};
  unirally::print_text(map, cursor, track, table, &variables);
  require(map.words[2 * 32 + 1] == (0x2000 | 22) &&
          cursor.position == 2 * 32 + 5);
  const std::array<std::uint8_t, 6> centred{0xf7, 0xb2, 0x00, 0xfc, 0x05, 0xff};
  unirally::print_text(map, cursor, centred, table, &variables);
  require(map.words[5 * 32 + 14] == (0x2000 | 22)); // (33 - 4) / 2 = 14
  // FD: word 12 as five digits, the leading zeros blank; EF: the object at the
  // cursor.
  const std::array<std::uint8_t, 9> number{0xfe, 0,    8,    0xfd, 0x00,
                                           0x01, 0xef, 0x02, 0xff};
  unirally::print_text(map, cursor, number, table, &variables);
  require(map.words[8 * 32 + 3] == (0x2000 | 24) && placed_object == 2 &&
          placed_at == 8 * 32 + 7); // three blanks, then two big digits
  // Refused: a variable code without variables.
  require(refuses([&] { unirally::print_text(map, cursor, track, table); }));
  // EE after F7: the name in capitals (R-0057). F8: rider 1's name from its
  // 16-byte record.
  const std::array<std::uint8_t, 5> lower{'a', 0xff, 'b', 'b', 0xff};
  variables.track_names = lower;
  const std::array<std::uint8_t, 8> upper{0xfe, 0,    11,   0xf7,
                                          0xb2, 0x00, 0xee, 0xff};
  unirally::print_text(map, cursor, upper, table, &variables);
  require(map.words[11 * 32] == (0x2000 | 22) &&
          map.words[11 * 32 + 2] == (0x2000 | 22));
  std::array<std::uint8_t, 32> riders{};
  riders.fill(0xff);
  riders[16] = 'A';
  variables.rider_names = riders;
  const std::array<std::uint8_t, 7> rider{0xfe, 0, 12, 0xf8, 0xb2, 0x00, 0xff};
  unirally::print_text(map, cursor, rider, table, &variables);
  require(map.words[12 * 32] == (0x2000 | 20) &&
          cursor.position == 12 * 32 + 2);
  // F1: word 12 as a race time, `_0:00.12`; the `1` after a small blank and
  // five big glyphs.
  const std::array<std::uint8_t, 7> race_time{0xfe, 0,    14,  0xf1,
                                              0xb4, 0x00, 0xff};
  unirally::print_text(map, cursor, race_time, table, &variables);
  require(map.words[14 * 32 + 11] == (0x2000 | 24) &&
          map.words[14 * 32 + 13] == (0x2000 | 26));
  // `$83:8BE7` and `$83:8C7B`.
  const auto digits = [](std::uint16_t value) {
    const auto text = unirally::five_digit_text(value);
    return std::string(text.begin(), text.begin() + 5);
  };
  require(digits(0) == "____0" && digits(7) == "____7" &&
          digits(10000) == "10000" && digits(65535) == "65535");
  const std::array<std::uint8_t, 18> words{'_', 'q', 'u',  'i', 't', '_',
                                           '_', '_', 0xff, '_', 'n', 'o',
                                           '_', 't', 'i',  'm', 'e', 0xff};
  const auto time = [&](std::uint16_t value) {
    const auto text = unirally::race_time_text(value, words);
    return std::string(text.begin(), text.end());
  };
  require(time(0x1770) == "_1:00.00" && time(0x7fff) == "_5:27.67" &&
          time(0xea5f) == "_9:59.99" && time(0xea60) == "_no_time" &&
          time(0xea61) == "_quit___");
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
  // Clipping the main screen to black always (CGWSEL bits 7-6 = 3) also stops
  // the halving, as in bsnes (`halve && windowAbove[x]`): black + blue 31 is
  // full blue.
  registers.colour_select = 0xc2;
  frame = unirally::render_snes_screen(memory, registers);
  require(frame.pixels[0] == 0 && frame.pixels[1] == 0 &&
          frame.pixels[2] == 255);
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
    content.assets[id] = keep(id == 69 ? 13568 : id == 68 ? 8000 : 64);
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
  // Poses up to 0x1432, the last the endings use (R-0062).
  constexpr std::size_t poses = 0x1433;
  storage.emplace_back(poses * 3, 0);
  for (std::size_t k = 0; k < poses; ++k)
    storage.back()[k * 3 + 1] = 0x80;
  content.uni_pictures.pose_pointers = storage.back();
  content.uni_pictures.pose_frames = keep(4);
  content.uni_pictures.object_tiles = keep(32);
  // The one-player screens (profile v17), synthetic: every stream prints 'A'.
  for (unsigned id : {22U, 23U, 24U, 25U, 26U, 32U, 33U, 34U, 35U, 36U, 37U})
    content.assets[id] = keep(32);
  content.medal_tiles = content.track_menu_tiles = keep(0xc00);
  const auto bytes =
      [&](std::vector<std::uint8_t> value) -> std::span<const std::uint8_t> {
    storage.push_back(std::move(value));
    return storage.back();
  };
  content.tour_menu_text = bytes({'A', 0xff, 'A', 0xff, 'A', 0xff, 'A', 0xff});
  content.tour_badge_places = keep(18);
  content.tour_levels = bytes({0, 1, 0, 1, 0, 2, 0, 2, 3, 3});
  content.tour_arrow_targets = keep(40);
  content.tour_badge_pictures = keep(30);
  content.medal_places = keep(36);
  content.medal_attributes = keep(4);
  std::vector<std::uint8_t> names;
  for (int k = 0; k < 50; ++k)
    names.insert(names.end(), {'A', 0xff});
  content.tour_names = content.track_names = bytes(names);
  content.track_menu_layout = bytes(std::vector<std::uint8_t>(0x23, 3));
  content.track_menu_text = bytes({0xfe, 7, 8, 0xf7, 0xb2, 0, 0xff});
  content.medal_words = bytes(std::vector<std::uint8_t>(29, 0xff));
  content.marker_tiles = keep(21);
  std::vector<std::uint8_t> kinds(40, 'A');
  for (const std::size_t end : {9U, 14U, 23U, 39U})
    kinds[end] = 0xff;
  content.race_kind_words = bytes(kinds);
  std::vector<std::uint8_t> now(0x88, 'A');
  for (const std::size_t end : {0x0eU, 0x26U, 0x3aU, 0x46U, 0x78U, 0x85U})
    now[end] = 0xff;
  content.now_playing_text = bytes(now);
  content.time_words = bytes(std::vector<std::uint8_t>(18, 0xff));
  content.laps = bytes(std::vector<std::uint8_t>(50, 3));
  content.qualifying_scores = keep(60);
  // The result screen (profile v18): its stream prints the track's name.
  content.result_text = bytes({0xf7, 0xce, 0x00, 0xfc, 0x02, 0xff});
  content.result_icons = keep(12);
  // The medal award (profile v20): its assets, tables (the bounce at 9, the
  // poses from 0x22, all pose 0x1340) and the medal's second art.
  for (const unsigned id : {0x3bU, 0x53U, 0x54U, 0x55U, 0x56U, 0x5cU, 0x64U, 0x65U})
    content.assets[id] = keep(64);
  storage.emplace_back(0xcb, 0);
  for (std::size_t step = 0; step < 58; ++step) {
    storage.back()[0x22 + 2 * step] = 0x40;
    storage.back()[0x23 + 2 * step] = 0x13;
  }
  storage.back()[0] = 0x78; // the medal's first entry: (0x78, 0x09)
  storage.back()[1] = 0x09;
  storage.back()[9] = 6; // the bounce's first height
  content.award_tables = storage.back();
  content.award_medal_art = keep(0x2000);
  // The gold endings (profile v22): their assets and tables.
  for (const unsigned id : {0x3cU, 0x3eU, 0x3fU, 0x40U, 0x41U, 0x42U, 0x43U, 0x4cU, 0x52U,
                            0x57U, 0x5eU, 0x5fU, 0x60U, 0x61U, 0x62U, 0x63U})
    content.assets[id] = keep(64);
  // Each tour's table longer than its script reads (CRAWLER's 82 bytes the
  // longest).
  for (auto &table : content.ending_tables)
    table = keep(0x60);
  // The lap result (profile v19): empty streams.
  content.lap_result_text = content.lap_result_record = bytes({0xff});
  content.lap_result_player = content.lap_result_opponent = bytes({0xff});
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
  require(state.screen == unirally::FrontEndScreen::main_menu &&
          !state.mode_chosen);
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
  require(!state.latches.moved);
  // Opposing directions read as neither (the D-pad's rocker).
  run(state, content, 1, {0x0c00, 0});
  require(state.menu.selection == 1 && !state.latches.moved);
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
  require(state.line_colours.size() == 129 && state.line_colours[1].row == 64 &&
          state.line_colours[1].index == 0x80);
  // Controller 2 is not read; Down moves once a press and stops at row 7.
  run(state, content, 1, {0, 0x0400});
  require(state.rider_menu.row == 0);
  for (int k = 0; k < 9; ++k) {
    run(state, content, 1, {0x0400, 0});
    run(state, content, 1);
  }
  require(state.rider_menu.row == 7 &&
          state.arrow.target_y == 0x0290 + 7 * 0x0180);
  // Up at row 0 stays; Right changes column (and the arrow's mirror and
  // palette).
  for (int k = 0; k < 8; ++k) {
    run(state, content, 1, {0x0800, 0});
    run(state, content, 1);
  }
  require(state.rider_menu.row == 0);
  run(state, content, 1, {0x0100, 0});
  require(state.arrow.target_x == 0x0780 &&
          (state.oam_buffer[119 * 4 + 3] & 0x40) == 0);
  run(state, content, 1, {0x0400, 0});
  run(state, content, 1);
  // Y goes back: three frames out, then 42 to the main menu, on 1P again.
  run(state, content, 1, {0x4000, 0});
  require(state.screen == FrontEndScreen::rider_menu_exit &&
          state.line_colours.empty());
  run(state, content, 3);
  require(state.screen == FrontEndScreen::main_menu_return);
  run(state, content, 42);
  require(state.screen == FrontEndScreen::main_menu &&
          state.menu.selection == 0 && state.menu.idle == 480 &&
          state.slide.scroll == 0 && !state.mode_chosen);
  // Choosing again starts on MIKE; B chooses the rider under the arrow.
  run(state, content, 1, {0x1000, 0});
  run(state, content, 43);
  run(state, content, 1, {0x0100, 0});
  run(state, content, 1, {0x8000, 0});
  require(state.screen == FrontEndScreen::rider_menu_exit &&
          !state.mode_chosen);
  run(state, content, 3);
  require(state.screen == FrontEndScreen::tour_menu_entry &&
          !state.mode_chosen && state.rider_menu.rider == 1);
}

// Runs until the screen changes to `screen` (at most `limit` frames), holding
// `pads` on the first frame only.
bool run_to(unirally::FrontEndState &state,
            const unirally::FrontEndContent &content,
            unirally::FrontEndScreen screen, unirally::FrontEndPads pads = {},
            std::uint32_t limit = 200) {
  for (std::uint32_t k = 0; k < limit; ++k) {
    unirally::update_front_end(state, content,
                               k == 0 ? pads : unirally::FrontEndPads{});
    if (state.screen == screen)
      return true;
  }
  return false;
}

void one_player_setup_tests() {
  using unirally::FrontEndScreen;
  std::vector<std::vector<std::uint8_t>> storage;
  const auto content = synthetic_content(storage);
  auto state = unirally::start_front_end();
  run(state, content, 430);
  require(run_to(state, content, FrontEndScreen::rider_menu, {0x1000, 0}));
  require(
      run_to(state, content, FrontEndScreen::tour_menu, {0x8000, 0})); // MIKE
  // A cold start opens the left column only: Right stays; Up at the top stays;
  // Down moves.
  run(state, content, 1, {0x0100, 0});
  require(state.tour_menu.cursor == 0);
  run(state, content, 1, {0x0800, 0});
  run(state, content, 1);
  run(state, content, 1, {0x0400, 0});
  require(state.tour_menu.cursor == 2 && state.latches.down);
  run(state, content, 1);
  // Y goes back to PICK YOUR UNI, which slides back in; B there comes back to
  // PICK TOUR, on the tour chosen last (none: CRAWLER).
  require(run_to(state, content, FrontEndScreen::rider_menu, {0x4000, 0}));
  require(state.slide.back && state.rider_menu.returning);
  require(run_to(state, content, FrontEndScreen::tour_menu, {0x8000, 0}));
  require(state.tour_menu.cursor == 0);
  run(state, content, 1, {0x0400, 0});
  run(state, content, 1);
  require(run_to(state, content, FrontEndScreen::track_menu, {0x1000, 0}));
  // Up from the first track wraps to the medal line; a choice there keeps
  // BRONZE (best 0); Down (or Select) wraps back to the first track.
  require(state.track_menu.cursor == 0);
  run(state, content, 1, {0x0800, 0});
  require(state.track_menu.cursor == 5);
  run(state, content, 1);
  run(state, content, 2, {0x0080, 0});
  require(state.tour_menu.medal == 0 && state.track_menu.medal_latched);
  run(state, content, 1);
  run(state, content, 1, {0x2000, 0});
  require(state.track_menu.cursor == 0);
  run(state, content, 1);
  run(state, content, 1, {0x0400, 0});
  run(state, content, 1);
  require(run_to(state, content, FrontEndScreen::now_playing, {0x1000, 0}));
  require(state.tour_menu.track == 11 && state.now_playing.opponent == 0x11);
  // Right to Exit, Left to Race; Y back to PICK TRACK on the chosen track.
  run(state, content, 1, {0x0100, 0});
  require(state.arrow.target_x == 0x0680);
  run(state, content, 1);
  run(state, content, 1, {0x0200, 0});
  require(state.arrow.target_x == 0x0300);
  require(run_to(state, content, FrontEndScreen::track_menu, {0x4000, 0}));
  require(state.track_menu.cursor == 1 && state.slide.back);
  // The race: NOW PLAYING again, then Race fades out over 7 frames.
  require(run_to(state, content, FrontEndScreen::now_playing, {0x1000, 0}));
  require(run_to(state, content, FrontEndScreen::race_fade, {0x1000, 0}));
  run(state, content, 6);
  require(!state.mode_chosen && state.registers.brightness == 3);
  run(state, content, 1);
  require(state.mode_chosen && state.registers.force_blank &&
          state.registers.brightness == 1 &&
          state.mode == unirally::FrontEndMode::one_player &&
          state.tour_menu.track == 11);
  // With every track of a tour won, the original's PICK TRACK never ends;
  // native refuses it.
  auto won = unirally::start_front_end();
  run(won, content, 430);
  require(run_to(won, content, FrontEndScreen::rider_menu, {0x1000, 0}));
  require(run_to(won, content, FrontEndScreen::tour_menu, {0x8000, 0}));
  won.records.tracks_done[0] = won.records.tracks_done[1] =
      won.records.tracks_done[2] = 1;
  won.records.tracks_done[3] = won.records.tracks_done[4] = 1;
  bool refused = false;
  try {
    run_to(won, content, FrontEndScreen::track_menu, {0x1000, 0});
  } catch (const std::logic_error &) {
    refused = true;
  }
  require(refused);
}

// R-0057: the race's return, the one-run result screen and its waits, and the
// records and the scoring on the way back to PICK TRACK.
void race_result_tests() {
  using unirally::FrontEndScreen;
  std::vector<std::vector<std::uint8_t>> storage;
  const auto content = synthetic_content(storage);
  // MIKE on the first track of CRAWLER, to the race.
  const auto to_race = [&] {
    auto state = unirally::start_front_end();
    run(state, content, 430);
    require(run_to(state, content, FrontEndScreen::rider_menu, {0x1000, 0}));
    require(run_to(state, content, FrontEndScreen::tour_menu, {0x8000, 0}));
    require(run_to(state, content, FrontEndScreen::track_menu, {0x1000, 0}));
    require(run_to(state, content, FrontEndScreen::now_playing, {0x1000, 0}));
    require(run_to(state, content, FrontEndScreen::race, {0x1000, 0}));
    require(state.mode_chosen && state.tour_menu.track == 0);
    return state;
  };
  auto won = to_race();
  // Refused before the race; accepted once, the menus' words restored in its
  // frame 101 and the result screen's first frame after its 104.
  auto early = unirally::start_front_end();
  require(throws_logic(
      [&] { unirally::return_from_race(early, content, 5000, {}); }));
  unirally::return_from_race(won, content, 5000, {3357, 4000});
  require(won.screen == FrontEndScreen::race_return && !won.mode_chosen &&
          won.frame == 5001);
  run(won, content, 103);
  require(won.screen == FrontEndScreen::race_return &&
          won.tour_menu.track == 0 && won.rider_menu.rider == 0);
  run(won, content, 1);
  require(won.screen == FrontEndScreen::race_result &&
          won.slide.scroll == 0 && won.slide.shown_half == 0x1000);
  // Built in three frames and faded in over seven; a pad held from the race
  // holds the result until both pads are released.
  run(won, content, 10, {0x0100, 0});
  require(won.registers.brightness == 14 && !won.registers.force_blank);
  run(won, content, 20, {0x0100, 0});
  require(won.screen == FrontEndScreen::race_result &&
          !won.race_result.released);
  run(won, content, 1);
  require(won.race_result.released);
  // One frame's press is not enough; two running leave.
  run(won, content, 1, {0x1000, 0});
  run(won, content, 1);
  require(won.screen == FrontEndScreen::race_result);
  run(won, content, 2, {0x1000, 0});
  require(won.screen == FrontEndScreen::race_result_exit);
  // Five frames out: the records, the scoring, then PICK TRACK again.
  run(won, content, 4);
  require(won.screen == FrontEndScreen::race_result_exit);
  run(won, content, 1);
  require(won.screen == FrontEndScreen::track_menu_entry);
  const auto &records = won.records;
  require(records.tracks_done[0] == 1 && !records.race_lost &&
          records.record_times[0][0] == 3357 &&
          records.record_holders[0][0] == 0 &&
          records.record_times[1][0] == 0xea60 &&
          records.statistics[0][0] == 1 && records.statistics[0][1] == 1 &&
          records.player_wins == 1 && records.opponent_wins == 0);
  // A computer opponent (0x11) keeps no counts; HUNTER's medals are 2 from the
  // cold start.
  require(records.medals[8 * 16] == 2 && records.medals[7 * 16] == 0);
  // A loss: a record still, no track won; leaving PICK TRACK spends a try.
  auto lost = to_race();
  unirally::return_from_race(lost, content, 5000, {5000, 4000});
  require(run_to(lost, content, FrontEndScreen::race_result, {}, 104));
  run(lost, content, 12);
  run(lost, content, 2, {0x8000, 0});
  require(run_to(lost, content, FrontEndScreen::track_menu_entry, {}, 5));
  require(lost.records.tracks_done[0] == 0 && lost.records.race_lost &&
          lost.records.record_times[0][0] == 5000 &&
          lost.records.statistics[0][1] == 0 && lost.records.tries == 3);
  require(run_to(lost, content, FrontEndScreen::track_menu));
  require(run_to(lost, content, FrontEndScreen::track_menu_exit, {0x1000, 0}));
  run(lost, content, 1);
  require(lost.records.race_lost);
  run(lost, content, 1);
  require(!lost.records.race_lost && lost.records.tries == 2);
  // A tie is a loss, and the player's win too.
  auto tied = to_race();
  unirally::return_from_race(tied, content, 5000, {4000, 4000});
  require(run_to(tied, content, FrontEndScreen::race_result, {}, 104));
  run(tied, content, 12);
  run(tied, content, 2, {0x8000, 0});
  require(run_to(tied, content, FrontEndScreen::track_menu_entry, {}, 5));
  require(tied.records.race_lost && tied.records.statistics[0][1] == 1 &&
          tied.records.player_wins == 1 && tied.records.opponent_wins == 0 &&
          tied.records.record_times[0][0] == 4000 &&
          tied.records.record_holders[0][0] == 0);
  // R-0060: the race's pause menu. A restart (0xEA62) skips the result: the
  // text cleared at r + 104, faded in, NOW PLAYING at r + 112; nothing scored.
  auto restart = to_race();
  unirally::return_from_race(restart, content, 5000, {0xea62, 0xea60});
  run(restart, content, 104);
  require(restart.screen == FrontEndScreen::race_restart &&
          restart.text.words[0] == 0x004c);
  run(restart, content, 7);
  require(restart.screen == FrontEndScreen::race_restart &&
          restart.registers.brightness == 12);
  run(restart, content, 1);
  require(restart.screen == FrontEndScreen::now_playing_entry &&
          restart.registers.brightness == 14 &&
          restart.records.statistics[0][0] == 0 && !restart.records.race_lost);
  // R-0061: a race whose tutorial hints ended sets its rider's bit ($83:CE2C).
  auto hints_over = to_race();
  unirally::RaceTimes over_times{0xea61, 0xea60};
  over_times.tutorial_hints_over = true;
  unirally::return_from_race(hints_over, content, 5000, over_times);
  require(hints_over.records.tutorial_bits == 1);
  // A quit (0xEA61) is a loss without a time: no record placed, so the way out
  // is a frame shorter, PICK TRACK on its fourth frame.
  auto quit = to_race();
  unirally::return_from_race(quit, content, 5000, {0xea61, 0xea60});
  require(quit.records.tutorial_bits == 0);
  require(run_to(quit, content, FrontEndScreen::race_result, {}, 104));
  run(quit, content, 12);
  run(quit, content, 2, {0x8000, 0});
  require(quit.screen == FrontEndScreen::race_result_exit);
  run(quit, content, 3);
  require(quit.screen == FrontEndScreen::race_result_exit);
  run(quit, content, 1);
  require(quit.screen == FrontEndScreen::track_menu_entry &&
          quit.records.statistics[0][2] == 1 && quit.records.race_lost &&
          quit.records.record_times[0][0] == 0xea60);
  // A win whose time places nowhere (three faster records) leaves as quickly.
  auto slow = to_race();
  for (unsigned place = 0; place < 3; ++place)
    slow.records.record_times[place][0] = static_cast<std::uint16_t>(1000 + place);
  unirally::return_from_race(slow, content, 5000, {3000, 4000});
  require(run_to(slow, content, FrontEndScreen::race_result, {}, 104));
  run(slow, content, 12);
  run(slow, content, 2, {0x8000, 0});
  run(slow, content, 3);
  require(slow.screen == FrontEndScreen::race_result_exit);
  run(slow, content, 1);
  require(slow.screen == FrontEndScreen::track_menu_entry &&
          slow.records.tracks_done[0] == 1 &&
          !slow.race_result.record_placed);
  // No time: a loss without one, and no record.
  auto timeless = to_race();
  unirally::return_from_race(timeless, content, 5000, {0xea60, 4000});
  require(run_to(timeless, content, FrontEndScreen::race_result, {}, 104));
  run(timeless, content, 12);
  run(timeless, content, 2, {0x8000, 0});
  require(run_to(timeless, content, FrontEndScreen::track_menu_entry, {}, 5));
  require(timeless.records.statistics[0][2] == 1 &&
          timeless.records.record_times[0][0] == 0xea60 &&
          timeless.records.record_holders[0][0] == 0x10);
  // A full top three: the new time takes second place, the others move down.
  const auto finish = [&](unirally::FrontEndState &state,
                          unirally::RaceTimes totals) {
    unirally::return_from_race(state, content, 5000, totals);
    require(run_to(state, content, FrontEndScreen::race_result, {}, 104));
    run(state, content, 12);
    run(state, content, 2, {0x8000, 0});
    return run_to(state, content, FrontEndScreen::track_menu_entry, {}, 5);
  };
  auto full = to_race();
  for (unsigned place = 0; place < 3; ++place) {
    full.records.record_times[place][0] =
        static_cast<std::uint16_t>(3000 + 200 * place);
    full.records.record_holders[place][0] =
        static_cast<std::uint8_t>(5 + place);
  }
  require(finish(full, {3100, 4000}));
  require(full.records.record_times[0][0] == 3000 &&
          full.records.record_times[1][0] == 3100 &&
          full.records.record_times[2][0] == 3200 &&
          full.records.record_holders[0][0] == 5 &&
          full.records.record_holders[1][0] == 0 &&
          full.records.record_holders[2][0] == 6);
  // A time equal to a record: `$80:F082` keeps the player's row first (the 1P
  // mark on the first row), and the insert, strictly less, puts it second.
  auto equal = to_race();
  equal.records.record_times[0][0] = 3357;
  equal.records.record_holders[0][0] = 5;
  unirally::return_from_race(equal, content, 5000, {3357, 4000});
  require(run_to(equal, content, FrontEndScreen::race_result, {}, 104));
  run(equal, content, 2);
  require(equal.oam_buffer[100 * 4 + 1] == 0x58);
  run(equal, content, 10);
  run(equal, content, 2, {0x8000, 0});
  require(run_to(equal, content, FrontEndScreen::track_menu_entry, {}, 5));
  require(equal.records.record_times[0][0] == 3357 &&
          equal.records.record_holders[0][0] == 5 &&
          equal.records.record_times[1][0] == 3357 &&
          equal.records.record_holders[1][0] == 0);
  // The tour's fifth done track completes it (R-0059): the done tracks
  // cleared, the bronze medal, the award screen.
  auto fifth = to_race();
  for (unsigned track = 1; track < 5; ++track)
    fifth.records.tracks_done[track] = 1;
  unirally::return_from_race(fifth, content, 5000, {3000, 4000});
  require(run_to(fifth, content, FrontEndScreen::race_result, {}, 104));
  run(fifth, content, 12);
  run(fifth, content, 2, {0x8000, 0});
  require(run_to(fifth, content, FrontEndScreen::tour_award, {}, 5));
  require(fifth.records.medals[0] == 1 && fifth.records.tracks_done[0] == 0 &&
          fifth.records.tracks_done[4] == 0);
  // A second race in the session: PICK TRACK comes back on the next undone
  // track, which NOW PLAYING then races.
  require(run_to(won, content, FrontEndScreen::track_menu));
  require(won.track_menu.cursor == 1);
  require(run_to(won, content, FrontEndScreen::now_playing, {0x1000, 0}));
  require(run_to(won, content, FrontEndScreen::race, {0x1000, 0}));
  require(won.tour_menu.track == 1 && won.mode_chosen);
  // Choosing a rider starts a new run (`$80:BBD6-BBE5`): no track done, three
  // tries, and PICK TRACK on the tour's first track again.
  auto again = to_race();
  require(finish(again, {3000, 4000}));
  require(run_to(again, content, FrontEndScreen::track_menu));
  require(again.records.tracks_done[0] == 1);
  require(run_to(again, content, FrontEndScreen::tour_menu, {0x4000, 0}));
  require(run_to(again, content, FrontEndScreen::rider_menu, {0x4000, 0}));
  require(run_to(again, content, FrontEndScreen::tour_menu, {0x8000, 0}));
  require(again.records.tracks_done[0] == 0 && again.records.tries == 3);
  require(run_to(again, content, FrontEndScreen::track_menu, {0x1000, 0}));
  require(again.track_menu.cursor == 0);
}

// R-0058: a lap race's result, with lap-won's times (MIKE 1:38.02 against
// BRONSEN 1:38.10 on ZOOM ZOO): the graph, the best laps, the one-press exit.
void lap_result_tests() {
  using unirally::FrontEndScreen;
  std::vector<std::vector<std::uint8_t>> storage;
  const auto content = synthetic_content(storage);
  // MIKE against BRONSEN on ZOOM ZOO, to the race.
  const auto to_lap_race = [&] {
    auto state = unirally::start_front_end();
    run(state, content, 430);
    require(run_to(state, content, FrontEndScreen::rider_menu, {0x1000, 0}));
    require(run_to(state, content, FrontEndScreen::tour_menu, {0x8000, 0}));
    require(run_to(state, content, FrontEndScreen::track_menu, {0x1000, 0}));
    run(state, content, 1, {0x0400, 0}); // ZOOM ZOO
    run(state, content, 1);
    require(run_to(state, content, FrontEndScreen::now_playing, {0x1000, 0}));
    require(run_to(state, content, FrontEndScreen::race, {0x1000, 0}));
    require(state.tour_menu.track == 1);
    return state;
  };
  unirally::RaceTimes times{0x264a, 0x2652, true};
  times.player_laps[0] = 0x0cb2;
  times.player_laps[1] = 0x0cc0;
  times.player_laps[2] = 0x0cd8;
  times.opponent_laps[0] = 0x0cb2;
  times.opponent_laps[1] = 0x0ccc;
  times.opponent_laps[2] = 0x0cd4;
  // To the result's first frame, then through its fade and out with a press.
  const auto to_result = [&](unirally::FrontEndState &state,
                             const unirally::RaceTimes &race) {
    unirally::return_from_race(state, content, 6725, race);
    require(run_to(state, content, FrontEndScreen::race_result, {}, 104));
  };
  const auto leave = [&](unirally::FrontEndState &state) {
    run(state, content, 20);
    run(state, content, 1, {0x1000, 0});
    require(run_to(state, content, FrontEndScreen::track_menu_entry, {}, 5));
  };
  const auto high = [](const unirally::FrontEndState &state, unsigned entry) {
    return state.oam_buffer[512 + entry / 4];
  };
  auto state = to_lap_race();
  unirally::return_from_race(state, content, 6725, times);
  require(run_to(state, content, FrontEndScreen::race_result, {}, 104));
  // The build: 8 x 8 objects, the dots' targets (the capture's), no record.
  run(state, content, 1);
  const auto &dots = state.race_result.dots;
  require(state.registers.obsel == 0x03 && dots[0].target_x == 0x03b0 &&
          dots[9].target_x == 0x0cb0);
  require(dots[0].target_y == 0x0510 && dots[1].target_y == 0x0480 &&
          dots[2].target_y == 0x0380 && dots[3].target_y == 0x0f00);
  require(dots[10].target_y == 0x0520 && dots[11].target_y == 0x0410 &&
          dots[12].target_y == 0x03c0);
  require(state.oam_buffer[512 + 104 / 4] == 0x5a); // no record: 106-107 hidden
  // The streams' frame: the best lap a personal best; BRONSEN's under the saved
  // word `$0064` (0x1300), so all four markers show.
  run(state, content, 1);
  require(state.records.best[1] == 0x0cb2 &&
          state.oam_buffer[512 + 96 / 4] == 0xaa);
  // The graph settles as in the capture, one pixel short on the first column.
  run(state, content, 800);
  const auto at = [&](unsigned entry) {
    return std::pair{state.oam_buffer[entry * 4], state.oam_buffer[entry * 4 + 1]};
  };
  require(at(0) == std::pair<std::uint8_t, std::uint8_t>{0x3a, 0x51});
  require(at(3) == std::pair<std::uint8_t, std::uint8_t>{0x6b, 0xf0});
  require(at(11) == std::pair<std::uint8_t, std::uint8_t>{0x4a, 0x40});
  // Pad 2 does not leave (`$77:0742` bit 10); one frame of pad 1 does.
  run(state, content, 1, {0, 0x1000});
  require(state.screen == FrontEndScreen::race_result);
  run(state, content, 1, {0x1000, 0});
  require(state.screen == FrontEndScreen::race_result_exit);
  // The records take the best lap; the win marks ZOOM ZOO done.
  require(run_to(state, content, FrontEndScreen::track_menu_entry, {}, 5));
  require(state.records.record_times[0][1] == 0x0cb2 &&
          state.records.record_holders[0][1] == 0 &&
          state.records.tracks_done[1] == 1);
  // With a record (0:32.00, rider 5's): the line at its height, entries 20-29
  // and the icons 106 and 112 shown. The spread is under 200, so the floor is
  // the top (0:32.88) less 200: 0xB8 - 128 x 112 / 200 = 0x71.
  auto record = to_lap_race();
  record.records.record_times[0][1] = 0x0c80;
  record.records.record_holders[0][1] = 5;
  to_result(record, times);
  run(record, content, 1);
  require(record.oam_buffer[20 * 4 + 1] == 0x71 &&
          record.oam_buffer[29 * 4 + 1] == 0x71);
  require(high(record, 104) == 0x6a && high(record, 112) == 0x56 &&
          high(record, 20) == 0xaa && high(record, 24) == 0xaa &&
          high(record, 28) == 0x5a);
  // A tie is a loss; the player's best lap still goes into the records.
  auto tie = to_lap_race();
  auto tied = times;
  tied.opponent_total = tied.player_total;
  to_result(tie, tied);
  leave(tie);
  require(tie.records.race_lost && tie.records.tracks_done[1] == 0 &&
          tie.records.record_times[0][1] == 0x0cb2 &&
          tie.records.statistics[0][1] == 1);
  // No time: every slot 0xEA60. A loss without a time, no record, and the
  // best (9:59.99) kept.
  auto timeless = to_lap_race();
  unirally::RaceTimes none{0xea60, 0x2652, true};
  to_result(timeless, none);
  leave(timeless);
  require(timeless.records.statistics[0][2] == 1 &&
          timeless.records.record_times[0][1] == 0xea60 &&
          timeless.records.best[1] == 0xea5f);
  // A zero slot: the result's best lap takes it, the records skip it.
  auto zero = to_lap_race();
  auto zeroed = times;
  zeroed.player_laps[3] = 0;
  to_result(zero, zeroed);
  leave(zero);
  require(zero.records.best[1] == 0 &&
          zero.records.record_times[0][1] == 0x0cb2);
  // Past CRAWLER's tracks native keeps no saved word: no opponent markers.
  auto elsewhere = to_lap_race();
  elsewhere.saved.tour_menu.track = 6;
  to_result(elsewhere, times);
  run(elsewhere, content, 2);
  require(high(elsewhere, 96) == 0xee);
  // A button held from the race, the d-pad too: no test before the graph's
  // first pass (the fade's last frame), then the result ends on the next.
  auto held = to_lap_race();
  to_result(held, times);
  run(held, content, 10, {0x0100, 0});
  require(held.screen == FrontEndScreen::race_result);
  run(held, content, 1, {0x0100, 0});
  require(held.screen == FrontEndScreen::race_result_exit);
}

// R-0059: a forced completion (pad 1 exactly Select + X + R on the scoring
// frame), the award's frames, its way back to PICK TOUR and on to PICK TRACK,
// and the unlock rule.
void award_tests() {
  using unirally::FrontEndScreen;
  std::vector<std::vector<std::uint8_t>> storage;
  const auto content = synthetic_content(storage);
  auto state = unirally::start_front_end();
  run(state, content, 430);
  require(run_to(state, content, FrontEndScreen::rider_menu, {0x1000, 0}));
  require(run_to(state, content, FrontEndScreen::tour_menu, {0x8000, 0}));
  require(run_to(state, content, FrontEndScreen::track_menu, {0x1000, 0}));
  require(run_to(state, content, FrontEndScreen::now_playing, {0x1000, 0}));
  require(run_to(state, content, FrontEndScreen::race, {0x1000, 0}));
  // Three tours already bronze: the fourth makes level 1 (four at bronze).
  for (const unsigned tour : {1U, 2U, 3U})
    state.records.medals[tour * 16] = 1;
  unirally::return_from_race(state, content, 3454, {5000, 4000}); // a loss
  require(run_to(state, content, FrontEndScreen::race_result, {}, 104));
  run(state, content, 12);
  // Select + X + R leaves the result and, still held on the scoring frame
  // (q + 3), completes the tour.
  constexpr std::uint16_t select_x_r = 0x2050;
  run(state, content, 2, {select_x_r, 0});
  require(state.screen == FrontEndScreen::race_result_exit);
  run(state, content, 3, {select_x_r, 0});
  require(state.screen == FrontEndScreen::tour_award &&
          state.records.medals[0] == 1 && !state.records.race_lost &&
          state.award.medal == 1);
  // The fade out (14 to 0), forced blank, the loads at q + 100 (mode 2).
  run(state, content, 1);
  require(state.registers.brightness == 14);
  run(state, content, 15);
  require(state.registers.force_blank && state.registers.mode == 3);
  run(state, content, 81);
  require(state.registers.mode == 2 && !state.cycle.running);
  run(state, content, 3);
  require(state.registers.obsel == 0xa3);
  // The objects at q + 109, the fade in to 15 by q + 125.
  run(state, content, 6);
  require(state.oam_buffer[0] == 0x78 && state.oam_buffer[1] == 0x09);
  run(state, content, 16);
  require(state.registers.brightness == 15 && !state.registers.force_blank);
  // Steps 1-4 hold the medal; step 5's build drops it 4 lines.
  run(state, content, 12);
  require(state.oam_buffer[1] == 0x09 && state.award.step == 4);
  run(state, content, 1);
  require(state.oam_buffer[1] == 0x0d);
  // Step 43 (medal step 0x26) takes three more frames: the second art, then
  // tile 0x84 written to OAM directly, then both objects at the bounce's first
  // height (0x71 - 6).
  run(state, content, 115);
  require(state.award.step == 43 &&
          state.award.phase == unirally::AwardPhase::second_art_low);
  run(state, content, 2);
  require(state.oam_buffer[2] == 0x84 && state.video.oam[2] == 0x84 &&
          state.video.oam[3] == 0x10);
  run(state, content, 1);
  require(state.oam_buffer[1] == 0x6b && state.oam_buffer[5] == 0x6b);
  // After step 0x39 the reset step: back to 0x2B and 0x26.
  run(state, content, 44);
  require(state.award.step == 0x2b && state.award.medal_step == 0x26);
  // A press on pad 2 is ignored; pad 1's seen by an upload leaves after the
  // test, then 132 frames to PICK TOUR, which slides back.
  run(state, content, 60, {0, 0x1000});
  require(state.award.exit_frame == 0);
  run(state, content, 3, {0x1000, 0});
  require(run_to(state, content, FrontEndScreen::tour_menu_entry, {}, 140));
  // Level 1 is pending (R-0062): PICK TOUR is drawn at level 0, then reveals
  // level 1 after its slide, four frames later than without a reveal.
  require(state.cycle.running && state.registers.mode == 3 &&
          state.tour_menu.returning && state.records.tour_levels[0] == 0 &&
          state.records.pending_reveal == 1);
  // PICK TOUR's Y leads on to PICK TRACK, after `$80:A858`'s two frames.
  require(run_to(state, content, FrontEndScreen::tour_menu, {}, 60));
  require(state.records.tour_levels[0] == 1 && state.records.pending_reveal == 0 &&
          !state.tour_menu.revealing);
  run(state, content, 1);
  require(run_to(state, content, FrontEndScreen::award_return, {0x4000, 0}));
  run(state, content, 2);
  require(state.screen == FrontEndScreen::track_menu_entry &&
          !state.award.after_completion);

  // A completion from records `medals` (tour 0 first, the rider's), forced,
  // the award left at once, to PICK TOUR's entry. With `now_playing_back`,
  // NOW PLAYING is left with Y once before the race (`$00AC` = 2).
  const auto complete = [&](std::array<std::uint8_t, 8> medals,
                            bool now_playing_back, std::uint8_t level) {
    auto run_state = unirally::start_front_end();
    run(run_state, content, 430);
    require(run_to(run_state, content, FrontEndScreen::rider_menu, {0x1000, 0}));
    require(run_to(run_state, content, FrontEndScreen::tour_menu, {0x8000, 0}));
    require(run_to(run_state, content, FrontEndScreen::track_menu, {0x1000, 0}));
    require(run_to(run_state, content, FrontEndScreen::now_playing, {0x1000, 0}));
    if (now_playing_back) {
      require(run_to(run_state, content, FrontEndScreen::track_menu, {0x4000, 0}));
      require(run_to(run_state, content, FrontEndScreen::now_playing, {0x1000, 0}));
    }
    require(run_to(run_state, content, FrontEndScreen::race, {0x1000, 0}));
    for (unsigned tour = 0; tour < 8; ++tour)
      run_state.records.medals[tour * 16] = medals[tour];
    run_state.records.tour_levels[0] = level;
    unirally::return_from_race(run_state, content, 3454, {3000, 4000});
    require(run_to(run_state, content, FrontEndScreen::race_result, {}, 104));
    run(run_state, content, 12);
    run(run_state, content, 5, {select_x_r, 0});
    // A gold medal plays the tour's ending (R-0062); the others show the award.
    require(run_state.screen == (run_state.award.medal >= 3 ? FrontEndScreen::tour_ending
                                                            : FrontEndScreen::tour_award));
    // Start held through a few of the animation's upload frames.
    for (unsigned k = 0; k < 700; ++k) {
      const bool held = k >= 130 && k < 140;
      unirally::update_front_end(run_state, content,
                                 {static_cast<std::uint16_t>(held ? 0x1000 : 0), 0});
      if (run_state.screen == FrontEndScreen::tour_menu_entry) break;
    }
    require(run_state.screen == FrontEndScreen::tour_menu_entry);
    return run_state;
  };
  // `$00AC` = 2: PICK TOUR slides forward, PICK TRACK then back.
  auto forward = complete({0, 0, 0, 0, 0, 0, 0, 0}, true, 0);
  require(!forward.tour_menu.slides_back && forward.track_menu.returning);
  require(run_to(forward, content, FrontEndScreen::tour_menu, {}, 60));
  run(forward, content, 1);
  require(run_to(forward, content, FrontEndScreen::award_return, {0x1000, 0}));
  run(forward, content, 2);
  require(forward.screen == FrontEndScreen::track_menu_entry &&
          forward.track_menu.returning);
  // The level a completion leaves: the pending reveal when a count matched
  // (PICK TOUR shows the level below it until its slide ends; R-0062).
  const auto revealed = [](const unirally::FrontEndState &state) {
    return state.records.pending_reveal ? state.records.pending_reveal
                                        : state.records.tour_levels[0];
  };
  // Gold: CRAWLER's ending, then PICK TOUR; the level from the exact counts.
  // All eight gold is level 3.
  auto gold = complete({2, 3, 3, 3, 3, 3, 3, 3}, false, 2);
  require(gold.records.medals[0] == 3 && revealed(gold) == 3 &&
          gold.registers.mode == 3);
  // Already gold: the medal stays 3.
  auto again = complete({3, 0, 0, 0, 0, 0, 0, 0}, false, 0);
  require(again.records.medals[0] == 3 && revealed(again) == 0);
  // Six at silver or better is level 2; five at bronze is no level (the
  // counts must be exact); a level of 3 never changes.
  auto silver = complete({1, 2, 2, 2, 2, 2, 0, 0}, false, 1);
  require(silver.records.medals[0] == 2 && revealed(silver) == 2);
  auto five = complete({0, 1, 1, 1, 1, 0, 0, 0}, false, 0);
  require(five.records.medals[0] == 1 && revealed(five) == 0);
  auto top = complete({0, 1, 1, 1, 0, 0, 0, 0}, false, 3);
  require(revealed(top) == 3);
  // HUNTER's medal (tour 8, 2 from the cold start) is not counted: three
  // bronze and this one make four, level 1.
  auto hunter = complete({0, 1, 1, 1, 0, 0, 0, 0}, false, 0);
  require(hunter.records.medals[8 * 16] == 2 &&
          revealed(hunter) == 1);
}

// R-0062: every tour's gold ending with the synthetic content: the script runs
// to its last frame t', PICK TOUR comes on t' + 133, and NMI's hook first runs
// on t' + 118, or on t' + 119 after WALKER's and JUMPER's endings.
void ending_tests() {
  using unirally::FrontEndScreen;
  std::vector<std::vector<std::uint8_t>> storage;
  const auto content = synthetic_content(storage);
  auto raced = unirally::start_front_end();
  run(raced, content, 430);
  require(run_to(raced, content, FrontEndScreen::rider_menu, {0x1000, 0}));
  require(run_to(raced, content, FrontEndScreen::tour_menu, {0x8000, 0}));
  require(run_to(raced, content, FrontEndScreen::track_menu, {0x1000, 0}));
  require(run_to(raced, content, FrontEndScreen::now_playing, {0x1000, 0}));
  require(run_to(raced, content, FrontEndScreen::race, {0x1000, 0}));
  // t' by tour (`$00D0`): CRAWLER, JUMPER, SHUFFLER, BOUNDER, WALKER, RUNNER,
  // HOPPER, SPRINTER.
  constexpr std::array<std::uint32_t, 8> last_frames{336, 409, 380, 299,
                                                     393, 321, 516, 356};
  constexpr std::uint32_t hook_frame = 118, pick_tour_frame = 133;
  constexpr std::uint16_t select_x_r = 0x2050;
  for (std::uint8_t tour = 0; tour < 8; ++tour) {
    auto state = raced;
    // The tour and its first track in the menus the race's return restores,
    // at level 2, where PICK TOUR shows all eight.
    state.records.tour_levels[0] = 2;
    state.saved.tour_menu.tour = tour;
    state.saved.tour_menu.track = static_cast<std::uint8_t>(tour * 5);
    state.records.medals[tour * 16] = 2;
    unirally::return_from_race(state, content, 3454, {3000, 4000});
    require(run_to(state, content, FrontEndScreen::race_result, {}, 104));
    run(state, content, 12);
    run(state, content, 5, {select_x_r, 0});
    require(state.screen == FrontEndScreen::tour_ending &&
            state.script_frame == 0 && state.records.medals[tour * 16] == 3);
    const auto last = last_frames[tour];
    run(state, content, last + hook_frame - 1);
    const auto cycle = [&] {
      return std::pair{state.cycle.delay, state.cycle.phase};
    };
    const auto before = cycle();
    run(state, content, 1);
    const bool late = tour == 1 || tour == 4; // JUMPER, WALKER
    require((cycle() == before) == late);
    if (late) {
      run(state, content, 1);
      require(cycle() != before);
      run(state, content, pick_tour_frame - hook_frame - 2);
    } else {
      run(state, content, pick_tour_frame - hook_frame - 1);
    }
    require(state.screen == FrontEndScreen::tour_ending &&
            state.script_frame == last + pick_tour_frame - 1);
    run(state, content, 1);
    require(state.screen == FrontEndScreen::tour_menu_entry);
  }
}

} // namespace

int main() try {
  text_printer_tests();
  text_variable_tests();
  snes_screen_tests();
  main_menu_tests();
  rider_menu_tests();
  one_player_setup_tests();
  race_result_tests();
  lap_result_tests();
  award_tests();
  ending_tests();
  return 0;
} catch (const std::exception &error) {
  std::fprintf(stderr, "%s\n", error.what());
  return 1;
}
