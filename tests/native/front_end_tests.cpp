// FRONT-END-MAIN-MENU (R-0054) and FRONT-END-1P-SETUP (R-0055, R-0056): the
// text printer, the SNES screen, the main menu's and the one-player screens'
// rules, on synthetic content (no ROM). The captures' frame-by-frame agreement
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
#include <vector>

namespace {

void require(bool value,
             std::source_location where = std::source_location::current()) {
  if (!value)
    throw std::runtime_error("front-end assertion failed at line " +
                             std::to_string(where.line()));
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
  // Refused: a variable code without variables, and an upper-case name after
  // F7.
  require(refuses([&] { unirally::print_text(map, cursor, track, table); }));
  const std::array<std::uint8_t, 5> upper{0xf7, 0xb2, 0x00, 0xee, 0xff};
  require(refuses(
      [&] { unirally::print_text(map, cursor, upper, table, &variables); }));
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
  storage.emplace_back(0x1321 * 3, 0);
  for (std::size_t k = 0; k < 0x1321; ++k)
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

} // namespace

int main() try {
  text_printer_tests();
  text_variable_tests();
  snes_screen_tests();
  main_menu_tests();
  rider_menu_tests();
  one_player_setup_tests();
  return 0;
} catch (const std::exception &error) {
  std::fprintf(stderr, "%s\n", error.what());
  return 1;
}
