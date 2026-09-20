#include "presentation.hpp"

#include <bitset>
#include "zoom_zoo_movement.hpp"
#include "content_pack.hpp"
#include "rider_object.hpp"
#include "zoom_zoo_pack.hpp"
#include <string>
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace unirally {

std::uint16_t snes_direct_colour(std::uint8_t palette_colour,
                                 std::uint8_t palette_group) {
  // bsnes sfc/ppu/screen.cpp:152-158. In mode 3 with CGWSEL bit 1 set,
  // BG1's 8-bit pixel and the map entry's three-bit group directly form BGR555.
  return static_cast<std::uint16_t>(
      ((static_cast<unsigned>(palette_colour) << 7U) & 0x6000U) |
      ((static_cast<unsigned>(palette_group) << 10U) & 0x1000U) |
      ((static_cast<unsigned>(palette_colour) << 4U) & 0x0380U) |
      ((static_cast<unsigned>(palette_group) << 5U) & 0x0040U) |
      ((static_cast<unsigned>(palette_colour) << 2U) & 0x001cU) |
      ((static_cast<unsigned>(palette_group) << 1U) & 0x0002U));
}

std::uint16_t snes_add_colour(std::uint16_t main_colour,
                              std::uint16_t sub_colour, bool halve) {
  // bsnes sfc/ppu/screen.cpp:130-137. The masks preserve independent carries
  // for the three five-bit colour channels.
  if (halve) {
    return static_cast<std::uint16_t>(
        (main_colour + sub_colour - ((main_colour ^ sub_colour) & 0x0421U)) >>
        1U);
  }
  const auto sum = static_cast<unsigned>(main_colour) + sub_colour;
  const auto carry = (sum - ((main_colour ^ sub_colour) & 0x0421U)) & 0x8420U;
  return static_cast<std::uint16_t>((sum - carry) | (carry - (carry >> 5U)));
}

namespace {
std::uint16_t word(std::span<const std::uint8_t> b, std::size_t at) {
  if (at > b.size() || b.size() - at < 2)
    throw std::invalid_argument(
        "Dragster BG1 gather exceeds decoded track data");
  return static_cast<std::uint16_t>(b[at] |
                                    (static_cast<unsigned>(b[at + 1]) << 8U));
}
void pixel(RgbFrame &f, int x, int y, std::array<std::uint8_t, 3> c) {
  if (x < 0 || y < 0 || x >= 256 || y >= 224)
    return;
  const auto at =
      (static_cast<std::size_t>(y) * 256 + static_cast<std::size_t>(x)) * 3;
  std::copy(c.begin(), c.end(),
            f.pixels.begin() + static_cast<std::ptrdiff_t>(at));
}
void rect(RgbFrame &f, int x, int y, int w, int h,
          std::array<std::uint8_t, 3> c) {
  for (int py = y; py < y + h; ++py)
    for (int px = x; px < x + w; ++px)
      pixel(f, px, py, c);
}

void render_window_xor(RgbFrame &frame, std::span<const std::uint8_t> table,
                       std::array<std::uint8_t, 3> fixed_colour) {
  // Channel 6 uses HDMA mode 4: each active line writes WH0..WH3 ($2126-$2129).
  // The race setup combines its two inclusive horizontal windows with XOR.
  std::size_t source = 0;
  int screen_y = 0;
  while (screen_y < 224) {
    if (source >= table.size())
      throw std::invalid_argument("Classic window HDMA table is truncated");
    const auto line_control = table[source++];
    const int line_count = line_control & 0x7fU;
    if (line_count == 0 || (line_control & 0x80U) == 0)
      throw std::invalid_argument("unsupported Classic window HDMA state");
    for (int line = 0; line < line_count && screen_y < 224;
         ++line, ++screen_y) {
      if (table.size() - source < 4)
        throw std::invalid_argument("Classic window HDMA row is truncated");
      const int window1_left = table[source++];
      const int window1_right = table[source++];
      const int window2_left = table[source++];
      const int window2_right = table[source++];
      for (int screen_x = 0; screen_x < 256; ++screen_x) {
        const bool in_window1 = window1_left <= window1_right &&
                                screen_x >= window1_left &&
                                screen_x <= window1_right;
        const bool in_window2 = window2_left <= window2_right &&
                                screen_x >= window2_left &&
                                screen_x <= window2_right;
        if (in_window1 != in_window2)
          pixel(frame, screen_x, screen_y, fixed_colour);
      }
    }
  }
  if (source != table.size())
    throw std::invalid_argument("Classic window HDMA table has trailing bytes");
}

std::uint8_t channel8(std::uint16_t value) {
  const auto expanded = static_cast<unsigned>((value << 3U) | (value >> 2U));
  const auto wide = expanded * 257U;
  if (wide > 32767U)
    return static_cast<std::uint8_t>(wide >> 8U);
  const auto corrected = 32767.0 * std::pow(wide / 32767.0, 1.5);
  return static_cast<std::uint8_t>(static_cast<unsigned>(corrected) >> 8U);
}

std::array<std::uint8_t, 3> colour(const std::array<std::uint8_t, 512> &cgram,
                                   std::uint8_t index) {
  const auto at = static_cast<std::size_t>(index) * 2;
  const auto value = static_cast<std::uint16_t>(
      cgram[at] | (static_cast<unsigned>(cgram[at + 1]) << 8U));
  return {channel8(value & 31U), channel8((value >> 5U) & 31U),
          channel8((value >> 10U) & 31U)};
}

std::uint16_t colour_word(const std::array<std::uint8_t, 512> &cgram,
                          std::uint8_t index) {
  const auto at = static_cast<std::size_t>(index) * 2;
  return static_cast<std::uint16_t>(
      cgram[at] | (static_cast<unsigned>(cgram[at + 1]) << 8U));
}

std::array<std::uint8_t, 3> colour_word_rgb(std::uint16_t value) {
  return {channel8(value & 31U), channel8((value >> 5U) & 31U),
          channel8((value >> 10U) & 31U)};
}

std::uint16_t apply_snes_brightness(std::uint16_t colour_value,
                                    unsigned brightness) {
  const auto scale = [brightness](unsigned channel) {
    return (brightness * channel + 7U) / 15U;
  };
  return static_cast<std::uint16_t>(
      scale(colour_value & 31U) | (scale((colour_value >> 5U) & 31U) << 5U) |
      (scale((colour_value >> 10U) & 31U) << 10U));
}

std::array<std::uint8_t, 512>
build_race_cgram(std::span<const std::uint8_t> packed_palette, bool late_finish) {
  std::array<std::uint8_t, 512> cgram{};
  constexpr std::array<std::size_t, 6> targets{{0, 224, 256, 352, 480, 384}};
  constexpr std::array<std::size_t, 6> lengths{{192, 32, 32, 32, 32, 32}};
  std::size_t source{};
  for (std::size_t piece = 0; piece < targets.size(); ++piece) {
    std::copy_n(packed_palette.begin() + static_cast<std::ptrdiff_t>(source),
                lengths[piece],
                cgram.begin() + static_cast<std::ptrdiff_t>(targets[piece]));
    source += lengths[piece];
  }
  static constexpr std::array<std::uint8_t, 32> racing_cycle{
      16,  66,  181, 86,  107, 45,  0,   0,   255, 127, 0,
      0,   148, 82,  255, 127, 106, 73,  164, 48,  65,  20,
      164, 48,  106, 73,  81,  102, 122, 127, 81,  102};
  static constexpr std::array<std::uint8_t, 32> finish_cycle{
      16, 66,  0,   0,   255, 127, 181, 86, 107, 45, 0,  0,  148, 82, 255, 127,
      81, 102, 122, 127, 81,  102, 106, 73, 164, 48, 65, 20, 164, 48, 106, 73};
  const auto &cycle = late_finish ? finish_cycle : racing_cycle;
  std::copy(cycle.begin(), cycle.end(), cgram.begin() + 192);
  if (late_finish) {
    cgram[0] = 173;
    cgram[1] = 125;
  } else {
    cgram[0] = 255;
    cgram[1] = 127;
  }
  return cgram;
}

// The accepted M3 DRAGSTER palette: its late-finish colours are keyed to its
// own finish poses. The shared renderer never uses this keying; the race NMI
// cycle (R-0037) supplies those colours from the pack instead.
std::array<std::uint8_t, 512>
build_race_cgram(const PresentationSample &sample,
                 std::span<const std::uint8_t> packed_palette,
                 bool dragster_late_finish = true) {
  const bool late_finish =
      dragster_late_finish &&
      (sample.movement.riders[1].pose.pose_index == 0x08d5 ||
       sample.movement.riders[0].pose.pose_index == 0x04fe);
  return build_race_cgram(packed_palette, late_finish);
}

std::uint8_t tile_pixel(const std::array<std::uint8_t, 65536> &vram,
                        std::size_t tile_byte, std::uint16_t tile, int x,
                        int y) {
  const auto at = (tile_byte + (tile & 0x3ffU) * 32U) & 0xffffU;
  const auto bit = static_cast<unsigned>(7 - x);
  const auto plane01 = at + static_cast<std::size_t>(y) * 2;
  const auto plane23 = plane01 + 16;
  return static_cast<std::uint8_t>(
      ((static_cast<unsigned>(vram[plane01]) >> bit) & 1U) |
      (((static_cast<unsigned>(vram[plane01 + 1]) >> bit) & 1U) << 1U) |
      (((static_cast<unsigned>(vram[plane23]) >> bit) & 1U) << 2U) |
      (((static_cast<unsigned>(vram[plane23 + 1]) >> bit) & 1U) << 3U));
}

std::uint8_t tile_pixel_8bpp(const std::array<std::uint8_t, 65536> &vram,
                             std::size_t tile_byte, std::uint16_t tile, int x,
                             int y) {
  const auto at = (tile_byte + (tile & 0x3ffU) * 64U) & 0xffffU;
  const auto bit = static_cast<unsigned>(7 - x);
  std::uint8_t value{};
  for (unsigned plane = 0; plane < 8; ++plane) {
    const auto plane_byte = at + static_cast<std::size_t>(y) * 2U +
                            (plane / 2U) * 16U + (plane & 1U);
    value |= static_cast<std::uint8_t>(
        ((static_cast<unsigned>(vram[plane_byte & 0xffffU]) >> bit) & 1U)
        << plane);
  }
  return value;
}

std::uint8_t background_pixel(const std::array<std::uint8_t, 65536> &vram,
                              std::size_t map_base, bool wide, bool tall,
                              std::size_t tile_base, bool tiles16, int hofs,
                              int vofs, int screen_x, int screen_y) {
  const int tile_size = tiles16 ? 16 : 8;
  const int map_width = wide ? 64 : 32;
  const int map_height = tall ? 64 : 32;
  const int px = (screen_x + hofs) & (map_width * tile_size - 1);
  const int py = (screen_y + vofs + 1) & (map_height * tile_size - 1);
  int map_x = px / tile_size, map_y = py / tile_size, screen = 0;
  if (map_x >= 32) {
    ++screen;
    map_x -= 32;
  }
  if (map_y >= 32) {
    screen += wide ? 2 : 1;
    map_y -= 32;
  }
  const auto entry_at = (map_base + static_cast<std::size_t>(screen) * 0x800U +
                         static_cast<std::size_t>(map_y * 32 + map_x) * 2U) &
                        0xffffU;
  const auto entry = static_cast<std::uint16_t>(
      vram[entry_at] | (static_cast<unsigned>(vram[entry_at + 1]) << 8U));
  int tile_x = px % tile_size, tile_y = py % tile_size;
  if (entry & 0x4000U)
    tile_x = tile_size - 1 - tile_x;
  if (entry & 0x8000U)
    tile_y = tile_size - 1 - tile_y;
  auto tile = static_cast<std::uint16_t>(entry & 0x3ffU);
  if (tiles16) {
    const auto subtile =
        static_cast<unsigned>((tile_x >> 3) + ((tile_y >> 3) << 4));
    tile = static_cast<std::uint16_t>((tile + subtile) & 0x3ffU);
    tile_x &= 7;
    tile_y &= 7;
  }
  const auto value = tile_pixel(vram, tile_base, tile, tile_x, tile_y);
  return value == 0
             ? 0
             : static_cast<std::uint8_t>(((entry >> 10U) & 7U) * 16U + value);
}

void render_race_background(RgbFrame &frame, const PresentationSample &sample,
                            const PresentationContent &content,
                            const std::array<std::uint16_t, 1024> &map) {
  std::array<std::uint8_t, 65536> vram{};
  std::copy(content.bg2_tiles.begin(), content.bg2_tiles.end(),
            vram.begin() + 0x2000);
  std::copy(content.bg2_map.begin(), content.bg2_map.end(),
            vram.begin() + 0xe000);
  constexpr std::array<std::uint16_t, 40> bg1_words{
      {8192, 8448, 8224, 8480, 8256, 8512, 8288, 8544, 8320, 8576,
       8352, 8608, 8384, 8640, 8416, 8672, 8704, 8960, 8736, 8992,
       8768, 9024, 8800, 9056, 8832, 9088, 8864, 9120, 8896, 9152,
       8928, 9184, 9216, 9472, 9248, 9504, 9280, 9536, 9312, 9568}};
  for (std::size_t piece = 0; piece < bg1_words.size(); ++piece)
    std::copy_n(content.bg1_tiles.begin() +
                    static_cast<std::ptrdiff_t>(piece * 64),
                64, vram.begin() + bg1_words[piece] * 2);
  for (std::size_t y = 0; y < 32; ++y)
    for (std::size_t x = 0; x < 32; ++x) {
      const auto at = 0x1800U + (y * 32U + x) * 2U;
      const auto entry = map[y * 32 + x];
      vram[at] = static_cast<std::uint8_t>(entry);
      vram[at + 1] = static_cast<std::uint8_t>(entry >> 8U);
    }
  auto cgram = build_race_cgram(sample, content.palette,
                                content.race_palette_cycle.empty());
  if (!content.race_palette_cycle.empty())
    apply_dragster_palette_cycle(cgram, content.race_palette_cycle,
                                 sample.movement);
  rect(frame, 0, 0, 256, 224, colour(cgram, 0));
  for (int y = 0; y < 224; ++y)
    for (int x = 0; x < 256; ++x) {
      const auto bg2 =
          background_pixel(vram, 0xe000, true, true, 0x2000, false,
                           sample.bg2_scroll_x, sample.bg2_scroll_y, x, y);
      if (bg2)
        pixel(frame, x, y, colour(cgram, bg2));
      const auto bg1 =
          background_pixel(vram, 0x1800, false, false, 0x4000, true,
                           sample.bg1_scroll_x, sample.bg1_scroll_y, x, y);
      if (bg1)
        pixel(frame, x, y, colour(cgram, bg1));
    }
}

void copy_wrapping(std::array<std::uint8_t, 65536> &destination,
                   std::size_t destination_byte,
                   std::span<const std::uint8_t> source) {
  for (std::size_t index = 0; index < source.size(); ++index)
    destination[(destination_byte + index) & 0xffffU] = source[index];
}

void set_map_word(std::array<std::uint8_t, 65536> &vram, int x, int y,
                  std::uint16_t value) {
  const auto at = 0x2000U + static_cast<std::size_t>(y * 32 + x) * 2U;
  vram[at] = static_cast<std::uint8_t>(value);
  vram[at + 1] = static_cast<std::uint8_t>(value >> 8U);
}

std::uint16_t result_title_tile(char glyph) {
  switch (glyph) {
  case 'a':
    return 0x14;
  case 'c':
    return 0x18;
  case 'd':
    return 0x1a;
  case 'e':
    return 0x1c;
  case 'g':
    return 0x20;
  case 'l':
    return 0x2a;
  case 'm':
    return 0x2c;
  case 'o':
    return 0x00;
  case 'p':
    return 0x30;
  case 'r':
    return 0x34;
  case 's':
    return 0x36;
  case 't':
    return 0x38;
  default:
    throw std::invalid_argument("unsupported Classic result title glyph");
  }
}

void write_result_title(std::array<std::uint8_t, 65536> &vram, int x, int y,
                        std::string_view text) {
  for (const char glyph : text) {
    const auto tile = result_title_tile(glyph);
    set_map_word(vram, x, y, static_cast<std::uint16_t>(0x3c00U | tile));
    set_map_word(vram, x + 1, y,
                 static_cast<std::uint16_t>(0x3c00U | (tile + 1U)));
    set_map_word(vram, x, y + 1,
                 static_cast<std::uint16_t>(0x3c00U | (tile + 0x50U)));
    set_map_word(vram, x + 1, y + 1,
                 static_cast<std::uint16_t>(0x3c00U | (tile + 0x51U)));
    x += 2;
  }
}

std::uint16_t result_text_tile(char glyph) {
  switch (glyph) {
  case ' ':
    return 0xce;
  case '.':
    return 0xa8;
  case ':':
    return 0xcc;
  // The small result font is contiguous from '.' at $A8: digits 0-9 are
  // $A9-$B2 and letters follow from 'A' at $B3 without 'O', which reuses
  // '0'. Ordinary finish times reach every digit (R-0038 frame 3860 shows 9).
  case '0':
    return 0xa9;
  case '1':
    return 0xaa;
  case '2':
    return 0xab;
  case '3':
    return 0xac;
  case '4':
    return 0xad;
  case '5':
    return 0xae;
  case '6':
    return 0xaf;
  case '7':
    return 0xb0;
  case '8':
    return 0xb1;
  case '9':
    return 0xb2;
  case 'A':
    return 0xb3;
  case 'E':
    return 0xb7;
  case 'I':
    return 0xbb;
  case 'K':
    return 0xbd;
  case 'L':
    return 0xbe;
  case 'M':
    return 0xbf;
  case 'N':
    return 0xc0;
  case 'O':
    return 0xa9;
  case 'P':
    return 0xc1;
  case 'R':
    return 0xc3;
  case 'S':
    return 0xc4;
  case 'T':
    return 0xc5;
  case 'Y':
    return 0xca;
  default:
    throw std::invalid_argument("unsupported Classic result text glyph");
  }
}

void write_result_text(std::array<std::uint8_t, 65536> &vram, int x, int y,
                       std::string_view text) {
  for (const char glyph : text) {
    const auto tile = result_text_tile(glyph);
    set_map_word(vram, x, y, static_cast<std::uint16_t>(0x3c00U | tile));
    set_map_word(vram, x, y + 1,
                 static_cast<std::uint16_t>(0x3c00U | (tile + 0x3cU)));
    ++x;
  }
}

void build_result_map(std::array<std::uint8_t, 65536> &vram,
                      const RaceFinishState &finish, const RaceTimerDigits &clock,
                      std::span<const std::uint8_t> result_assets) {
  for (std::size_t entry = 0; entry < 1024; ++entry)
    set_map_word(vram, static_cast<int>(entry % 32),
                 static_cast<int>(entry / 32), 0x004c);

  const auto title_seed = result_assets.subspan(5208, 16);
  const auto terminator =
      std::find(title_seed.begin(), title_seed.end(), std::uint8_t{0xff});
  if (terminator == title_seed.end())
    throw std::invalid_argument("Classic result title seed lacks terminator");
  const std::string_view title(
      reinterpret_cast<const char *>(title_seed.data()),
      static_cast<std::size_t>(terminator - title_seed.begin()));
  const bool observed_winner_publication =
      finish.outcome == RaceOutcome::PlayerWon &&
      ((finish.phase == RacePhase::ResultLoading &&
        finish.result_loading_updates == 225) ||
       (finish.phase == RacePhase::ResultScreen &&
        finish.result_loading_updates == 226));
  const bool observed_loser_publication =
      finish.outcome == RaceOutcome::PlayerLost &&
      finish.phase == RacePhase::ResultScreen &&
      finish.result_loading_updates == 242;
  const auto time_is_consistent = [](const std::array<std::uint16_t, 5> &digits,
                                     std::uint16_t centiseconds) {
    if (digits[0] > 9 || digits[1] > 5 || digits[2] > 9 || digits[3] > 9 ||
        digits[4] > 9)
      return false;
    const auto displayed = static_cast<unsigned>(digits[0]) * 6000U +
                           static_cast<unsigned>(digits[1]) * 1000U +
                           static_cast<unsigned>(digits[2]) * 100U +
                           static_cast<unsigned>(digits[3]) * 10U + digits[4];
    return displayed == centiseconds;
  };
  // $81:C73E-C75B ends the race at 10:00 with both riders finished and the
  // lap-short player's total left at the 60000 no-time sentinel. Its crossing
  // digits still hold the start-line crossing, so they do not describe the
  // total and the original writes NO TIME in the player row instead
  // (clock-limit original, stable result 32016-32200).
  // $81:C73E-C75B holds 9:59.9 when it finishes both riders, so a no-time
  // player total only belongs to that timed-out race (R-0039). A consistent
  // time never reaches the sentinel, so this stays a strict extension.
  const bool clock_expired = clock.minutes == 9 && clock.tens_seconds == 5 &&
                             clock.seconds == 9 && clock.tenths == 9;
  const bool player_has_no_time =
      finish.finish_time_centiseconds[0] >= 60000 && clock_expired;
  const bool times_are_consistent =
      finish.rider_finished[0] && finish.rider_finished[1] &&
      (player_has_no_time ||
       time_is_consistent(finish.finish_time_digits[0],
                          finish.finish_time_centiseconds[0])) &&
      time_is_consistent(finish.finish_time_digits[1],
                         finish.finish_time_centiseconds[1]);
  // Equal times mean both riders crossed on one update; the player's crossing
  // is processed first, so the race counts it as won (R-0038 tie original).
  const bool outcome_is_consistent =
      (finish.outcome == RaceOutcome::PlayerWon &&
       finish.finish_time_centiseconds[0] <=
           finish.finish_time_centiseconds[1]) ||
      (finish.outcome == RaceOutcome::PlayerLost &&
       finish.finish_time_centiseconds[0] > finish.finish_time_centiseconds[1]);
  if (title != "dragster" ||
      (!observed_winner_publication && !observed_loser_publication) ||
      !times_are_consistent ||
      !outcome_is_consistent)
    throw std::invalid_argument("unsupported Classic result composition");

  // $80:C431 first fills the map, then writes these semantic fields in this
  // order. Each small-font glyph is a vertical tile pair; title glyphs are
  // two-by-two. The result state carries the observed five timer digits.
  write_result_title(vram, 8, 2, title);
  write_result_title(vram, 8, 5, "complete");
  write_result_text(vram, 7, 8, "PLAYER     TIME");
  const auto &digits = finish.finish_time_digits[0];
  std::array<char, 8> time{{' ', static_cast<char>('0' + digits[0]), ':',
                            static_cast<char>('0' + digits[1]),
                            static_cast<char>('0' + digits[2]), '.',
                            static_cast<char>('0' + digits[3]),
                            static_cast<char>('0' + digits[4])}};
  write_result_text(vram, 7, 11, "MIKE    ");
  write_result_text(vram, 17, 11,
                    player_has_no_time
                        ? std::string_view(" NO TIME")
                        : std::string_view(time.data(), time.size()));
  for (const int row : {14, 17, 20}) {
    write_result_text(vram, 7, row, "SOMEONE ");
    write_result_text(vram, 17, row, " NO TIME");
  }
}

struct ResultBackgroundPixel {
  std::uint8_t palette_index{};
  std::uint8_t palette_group{};
  std::uint8_t priority{};
  bool direct_colour{};
};

ResultBackgroundPixel
result_bg1_pixel(const std::array<std::uint8_t, 65536> &vram, int x, int y) {
  constexpr int vertical_scroll = 82;
  const int py = (y + vertical_scroll + 1) & 511;
  const int map_screen = py >= 256 ? 1 : 0;
  const int map_y = (py / 8) & 31;
  const int map_x = x / 8;
  const auto map_at =
      static_cast<std::size_t>(map_screen * 0x800 + (map_y * 32 + map_x) * 2);
  const auto entry = static_cast<std::uint16_t>(
      vram[map_at] | (static_cast<unsigned>(vram[map_at + 1]) << 8U));
  int tile_x = x & 7, tile_y = py & 7;
  if (entry & 0x4000U)
    tile_x = 7 - tile_x;
  if (entry & 0x8000U)
    tile_y = 7 - tile_y;
  const auto value =
      tile_pixel_8bpp(vram, 0x6000, entry & 0x3ffU, tile_x, tile_y);
  const auto priority =
      static_cast<std::uint8_t>(value == 0 ? 0 : (entry & 0x2000U ? 7 : 3));
  const auto palette_group = static_cast<std::uint8_t>((entry >> 10U) & 7U);
  // CGWSEL=$02 selects subscreen blending (bit 1). Direct colour is bit 0 and
  // is disabled in the captured result state, so BG1 still indexes CGRAM.
  return {value, palette_group, priority, false};
}

ResultBackgroundPixel
result_bg2_pixel(const std::array<std::uint8_t, 65536> &vram, int x, int y) {
  const int py = (y + 1) & 511;
  const int map_screen = py >= 256 ? 2 : 0;
  const int map_y = (py / 8) & 31;
  const int map_x = x / 8;
  const auto map_at = static_cast<std::size_t>(0x2000 + map_screen * 0x800 +
                                               (map_y * 32 + map_x) * 2);
  const auto entry = static_cast<std::uint16_t>(
      vram[map_at] | (static_cast<unsigned>(vram[map_at + 1]) << 8U));
  int tile_x = x & 7, tile_y = py & 7;
  if (entry & 0x4000U)
    tile_x = 7 - tile_x;
  if (entry & 0x8000U)
    tile_y = 7 - tile_y;
  const auto value = tile_pixel(vram, 0x4000, entry & 0x3ffU, tile_x, tile_y);
  const auto palette =
      static_cast<std::uint8_t>(((entry >> 10U) & 7U) * 16U + value);
  const auto priority =
      static_cast<std::uint8_t>(value == 0 ? 0 : (entry & 0x2000U ? 5 : 1));
  return {palette, 0, priority, false};
}

struct ClassicResultContent {
  std::span<const std::uint8_t> palette, result_assets, result_base_vram;
  std::span<const std::uint8_t> result_palette, result_palette_tail;
};

void render_result_background(RgbFrame &frame, const RaceFinishState &finish,
                              const RaceTimerDigits &clock,
                              const ClassicResultContent &content) {
  // These writes replay the observed $82:B296 copier sequence. Addresses are
  // VRAM byte addresses; the SNES VMADD register observed by the capture uses
  // word addresses. Later writes intentionally replace overlapping content.
  std::array<std::uint8_t, 65536> vram{};
  copy_wrapping(vram, 0xc000, content.result_base_vram.subspan(0, 8192));
  copy_wrapping(vram, 0x0000, content.result_base_vram.subspan(8192, 2816));
  copy_wrapping(vram, 0x4000, content.result_base_vram.subspan(11008, 8960));
  copy_wrapping(vram, 0x6340, content.result_base_vram.subspan(19968, 8000));
  // Frame 3546 resets VMADD to word $3D80 before the remaining copier run.
  copy_wrapping(vram, 0x7b00, content.result_base_vram.subspan(27968, 13568));

  // The two result-specific 4bpp payloads retain the current VMADD ordering:
  // $3D80 (byte $7B00), then $7A00 (byte $F400, wrapping through $0000).
  copy_wrapping(vram, 0x7b00, content.result_assets.subspan(216, 1920));
  copy_wrapping(vram, 0xf400, content.result_assets.subspan(2136, 3072));

  build_result_map(vram, finish, clock, content.result_assets);

  // The result palettes below replace every colour the pose-keyed race cycle
  // could have set, so the neutral layout is byte-identical here.
  auto cgram = build_race_cgram(content.palette, false);
  std::copy(content.result_palette.begin(), content.result_palette.end(),
            cgram.begin());
  // The seven-frame palette cycle's stable-frame phase is captured at frame
  // 3678: CGRAM 108..111 receive $4A52, $4631, $4210 and $56B5.
  constexpr std::array<std::uint8_t, 8> winner_cycle{0x52, 0x4a, 0x31, 0x46,
                                                     0x10, 0x42, 0xb5, 0x56};
  // The release-3000 loss reaches its first complete result on frame 3800.
  // Its last palette-cycle write at frame 3797 is the observed two-word
  // rotation below; this is presentation state and does not alter gameplay.
  constexpr std::array<std::uint8_t, 8> loser_cycle{0x10, 0x42, 0xb5, 0x56,
                                                    0x52, 0x4a, 0x31, 0x46};
  const auto &cycle = finish.outcome == RaceOutcome::PlayerLost ? loser_cycle
                                                                : winner_cycle;
  std::copy(cycle.begin(), cycle.end(), cgram.begin() + 216);

  // Replay the observed CPU palette writers after the 216-byte DMA. Byte
  // destinations are twice CGADD; repeated source blocks are intentional.
  std::copy_n(content.result_palette_tail.begin(), 32, cgram.begin() + 224);
  std::copy_n(content.palette.begin() + 256, 32, cgram.begin() + 256);
  for (const std::size_t destination : {288U, 320U, 352U}) {
    std::copy_n(content.palette.begin() + 288, 32,
                cgram.begin() + static_cast<std::ptrdiff_t>(destination));
  }
  for (std::size_t block = 0; block < 3; ++block) {
    std::copy_n(content.result_palette_tail.begin() +
                    static_cast<std::ptrdiff_t>(32 + block * 32),
                32,
                cgram.begin() + static_cast<std::ptrdiff_t>(416 + block * 32));
  }
  for (int y = 0; y < 224; ++y)
    for (int x = 0; x < 256; ++x) {
      const auto bg1 = result_bg1_pixel(vram, x, y);
      const auto bg2 = result_bg2_pixel(vram, x, y);
      const auto above = bg2.priority > bg1.priority ? bg2 : bg1;
      const auto main_colour =
          above.priority == 0 ? colour_word(cgram, 0)
          : above.direct_colour
              ? snes_direct_colour(above.palette_index, above.palette_group)
              : colour_word(cgram, above.palette_index);
      // TS enables only OBJ. Where the bounded result renderer omits an OBJ,
      // the subscreen is transparent and bsnes disables halve/subscreen blend.
      // The result fade reaches and retains INIDISP brightness 14 at frame
      // 3568; frame 3679 has no later write. bsnes rounds each five-bit
      // channel after multiplying by brightness / 15.
      pixel(frame, x, y,
            colour_word_rgb(apply_snes_brightness(main_colour, 14)));
    }
}

struct RiderAtlasGroup {
  std::size_t first_tile;
  std::span<const std::uint16_t> vram_words;
};

RiderAtlasGroup rider_atlas_group(const PresentationSample &sample) {
  static constexpr std::array<std::uint16_t, 27> frame1600{
      24608, 24624, 26800, 26816, 26832, 24864, 24880, 24896, 27056,
      27072, 27088, 25136, 25152, 25168, 27296, 27312, 27328, 27344,
      25392, 25408, 25424, 27552, 27568, 25664, 25680, 27808, 27824};
  static constexpr std::array<std::uint16_t, 20> frame2000{
      24848, 24864, 24880, 27024, 27040, 27056, 25136, 25152, 25168, 25184,
      27312, 27328, 27344, 27360, 25408, 25424, 25440, 27584, 27600, 27616};
  static constexpr std::array<std::uint16_t, 21> frame2400{
      26784, 24848, 24864, 24880, 27024, 27040, 27056,
      25136, 25152, 25168, 25184, 27312, 27328, 27344,
      27360, 25408, 25424, 25440, 27584, 27600, 27616};
  static constexpr std::array<std::uint16_t, 21> frame3213{
      24608, 24848, 24864, 24880, 27024, 27040, 27056,
      25136, 25152, 25168, 25184, 27312, 27328, 27344,
      27360, 25408, 25424, 25440, 27584, 27600, 27616};
  static constexpr std::array<std::uint16_t, 19> frame3453{
      24624, 24640, 24880, 24896, 25136, 25152, 25376, 25392, 25408, 27552,
      27568, 27584, 25648, 25664, 25680, 27792, 27808, 27824, 27840};
  const auto player = sample.movement.riders[0].pose.pose_index;
  const auto opponent = sample.movement.riders[1].pose.pose_index;
  if (player == 0x04f9 && opponent == 0x0263)
    return {0, frame1600};
  if (player == 0x0855 && opponent == 0x0895)
    return {27, frame2000};
  if (player == 0x0895 && opponent == 0x0895)
    return {47, frame2400};
  if (player == 0x0855 && opponent == 0x08d5)
    return {68, frame3213};
  if (player == 0x04fe && opponent == 0x037c)
    return {89, frame3453};
  throw std::invalid_argument("unsupported Classic rider atlas combination");
}

void load_rider_tiles(std::array<std::uint8_t, 65536> &vram,
                      const PresentationSample &sample,
                      std::span<const std::uint8_t> atlas) {
  const auto group = rider_atlas_group(sample);
  for (std::size_t tile = 0; tile < group.vram_words.size(); ++tile) {
    const auto source = (group.first_tile + tile) * 32U;
    const auto destination =
        static_cast<std::size_t>(group.vram_words[tile]) * 2U;
    std::copy_n(atlas.begin() + static_cast<std::ptrdiff_t>(source), 32,
                vram.begin() + static_cast<std::ptrdiff_t>(destination));
  }
}

void render_rider(RgbFrame &frame, const std::array<std::uint8_t, 65536> &vram,
                  const std::array<std::uint8_t, 512> &cgram, int x, int y,
                  std::uint16_t base_tile, std::uint8_t attributes) {
  constexpr std::size_t object_tile_base = 0xc000;
  for (int tile_y = 0; tile_y < 8; ++tile_y)
    for (int tile_x = 0; tile_x < 8; ++tile_x) {
      const int source_x = 7 - tile_x;
      const auto tile_offset = static_cast<unsigned>(source_x + (tile_y << 4));
      const auto tile = static_cast<std::uint16_t>(
          (base_tile & 0x100U) |
          ((static_cast<unsigned>(base_tile) + tile_offset) & 0xffU));
      for (int pixel_y = 0; pixel_y < 8; ++pixel_y)
        for (int pixel_x = 0; pixel_x < 8; ++pixel_x) {
          const auto value =
              tile_pixel(vram, object_tile_base, tile, 7 - pixel_x, pixel_y);
          if (value == 0)
            continue;
          const auto palette = static_cast<std::uint8_t>(
              128U + ((attributes >> 1U) & 7U) * 16U + value);
          pixel(frame, x + tile_x * 8 + pixel_x, y + tile_y * 8 + pixel_y,
                colour(cgram, palette));
        }
    }
}

} // namespace
std::array<std::uint16_t, 32 * 32>
build_dragster_result_map(const MovementState &state,
                          std::span<const std::uint8_t> result_assets) {
  std::array<std::uint8_t, 65536> vram{};
  build_result_map(vram, state.finish, state.timer, result_assets);
  std::array<std::uint16_t, 32 * 32> map{};
  for (std::size_t entry = 0; entry < map.size(); ++entry) {
    const auto at = 0x2000U + entry * 2U;
    map[entry] = static_cast<std::uint16_t>(
        vram[at] | (static_cast<unsigned>(vram[at + 1]) << 8U));
  }
  return map;
}
RiderFrameSelection rider_frame_for_pose(std::uint16_t pose, bool reflected) {
  // Every observed pose in the frozen Classic slice carries the reflected
  // semantic orientation. The packed atlas/OAM relationship is not recovered
  // for the contradictory orientation, so fail closed instead of silently
  // drawing the reflected object geometry.
  if (!reflected)
    throw std::invalid_argument(
        "unsupported Classic rider pose/reflection combination");
  RiderFrameId id{};
  switch (pose) {
  case 0x04f9:
    id = RiderFrameId::LeanForward;
    break;
  case 0x0263:
    id = RiderFrameId::CoastForward;
    break;
  case 0x0855:
    id = RiderFrameId::RollingForward;
    break;
  case 0x0895:
    id = reflected ? RiderFrameId::RollingReflected
                   : RiderFrameId::RollingForward;
    break;
  case 0x08d5:
    id = RiderFrameId::RollingReflected;
    break;
  case 0x04fe:
    id = reflected ? RiderFrameId::SettledReflected
                   : RiderFrameId::SettledForward;
    break;
  case 0x037c:
    id =
        reflected ? RiderFrameId::FinishReflected : RiderFrameId::FinishForward;
    break;
  default:
    throw std::invalid_argument(
        "unsupported semantic rider pose for Classic presentation");
  }
  return {id, "presentation.rider.mike.race-tiles.v1", reflected};
}
std::vector<std::uint16_t>
gather_dragster_bg1(std::span<const std::uint8_t> track, std::uint16_t source_x,
                    std::uint16_t stride, std::size_t count) {
  if (stride == 0 || (stride & 1U))
    throw std::invalid_argument(
        "Dragster BG1 stride must be a positive even byte count");
  std::vector<std::uint16_t> out;
  out.reserve(count);
  std::size_t cursor = 0x0fU + source_x;
  for (std::size_t i = 0; i < count; ++i) {
    out.push_back(word(track, cursor));
    if (i + 1 < count) {
      if (cursor > std::numeric_limits<std::size_t>::max() - stride)
        throw std::invalid_argument("Dragster BG1 gather offset overflow");
      cursor += stride;
    }
  }
  return out;
}
std::array<std::uint16_t, 480>
expand_dragster_bg1(std::span<const std::uint8_t> track) {
  constexpr std::size_t base = 0x800f, column_bytes = 32;
  if (track.size() < base + 30 * column_bytes)
    throw std::invalid_argument(
        "decoded track lacks the observed Dragster BG1 map region");
  std::array<std::uint16_t, 480> out{};
  for (std::size_t x = 0; x < 30; ++x)
    for (std::size_t y = 0; y < 16; ++y)
      out[y * 30 + x] = word(track, base + x * column_bytes + y * 2);
  return out;
}

std::array<std::uint16_t, 1024>
build_dragster_bg1_map(std::span<const std::uint8_t> track,
                       std::int16_t scroll_x, std::int16_t scroll_y) {
  constexpr int horizontal_origin_tiles = 16;
  constexpr int vertical_origin_tiles = 10;
  constexpr std::size_t selector_base = 0x5831;
  constexpr std::size_t selector_plane_bytes = 0x800;
  constexpr std::size_t definition_base = 0x800f;

  const int first_tile_x = static_cast<std::uint16_t>(scroll_x) / 16;
  const int first_tile_y = static_cast<std::uint16_t>(scroll_y) / 16;
  std::array<std::uint16_t, 1024> map{};
  for (int ring_y = 0; ring_y < 32; ++ring_y) {
    const int global_y = first_tile_y + ((ring_y - first_tile_y) & 31);
    const int relative_y = global_y - vertical_origin_tiles;
    if (relative_y < 0)
      continue;
    const auto selector_plane = static_cast<std::size_t>(relative_y / 4);
    const auto definition_y = static_cast<std::size_t>(relative_y & 3);
    if (selector_plane >= 5)
      continue;
    for (int ring_x = 0; ring_x < 32; ++ring_x) {
      const int global_x = first_tile_x + ((ring_x - first_tile_x) & 31);
      const int relative_x = global_x - horizontal_origin_tiles;
      if (relative_x < 0)
        continue;
      const auto selector_column = static_cast<std::size_t>(relative_x / 4);
      const auto definition_x = static_cast<std::size_t>(relative_x & 3);
      const auto selector_at = selector_base +
                               selector_plane * selector_plane_bytes +
                               selector_column * 2U;
      const auto selector = word(track, selector_at);
      const auto definition_at = definition_base + selector * 32U +
                                 definition_y * 8U + definition_x * 2U;
      map[static_cast<std::size_t>(ring_y * 32 + ring_x)] =
          word(track, definition_at);
    }
  }
  return map;
}
static RgbFrame render_dragster(const PresentationSample &s,
                                const PresentationContent &content,
                                const std::array<RiderArtPose, 2> *rider_art) {
  if (content.bg1_tiles.size() != 2560 || content.bg2_tiles.size() != 992 ||
      content.bg2_map.size() != 8192 || content.palette.size() != 352 ||
      content.font.size() != 2048 || content.rider_tiles.size() != 3456 ||
      content.result_assets.size() != 5224 || content.go_window.size() != 898 ||
      content.winner_window.size() != 898 ||
      content.result_base_vram.size() != 41536 ||
      content.result_palette.size() != 216 ||
      content.result_palette_tail.size() != 128 ||
      (!content.window_tables.empty() && content.window_tables.size() != 22475))
    throw std::invalid_argument(
        "Classic presentation entry size is unsupported");
  // The original publishes the completed result at end-of-frame 3678. The
  // accepted gameplay state reaches ResultScreen on the following update, so
  // presentation consumes the observed loading counter without altering the
  // gameplay transition or its serialization.
  const auto &finish = s.movement.finish;
  const bool result_visible = finish.phase == RacePhase::ResultScreen ||
                              (finish.phase == RacePhase::ResultLoading &&
                               finish.outcome == RaceOutcome::PlayerWon &&
                               finish.result_loading_updates >= 225);
  if (result_visible) {
    RgbFrame result{};
    render_result_background(result, finish, s.movement.timer,
                             {content.palette, content.result_assets,
                              content.result_base_vram, content.result_palette,
                              content.result_palette_tail});
    return result;
  }
  const auto map =
      build_dragster_bg1_map(content.track, s.bg1_scroll_x, s.bg1_scroll_y);
  RgbFrame f{};
  render_race_background(f, s, content, map);
  const auto player_pose = s.movement.riders[0].pose.pose_index;
  const auto opponent_pose = s.movement.riders[1].pose.pose_index;
  // The GO and winner windows show colour 0 through colour math. Its race
  // palette cycle is visible only there (R-0037): the accepted white and
  // (98,98,255) are colour 0 at phases 10 and 7, where the frozen frames fall.
  // Without the cycle tables (DRAGSTER v1 packs) keep those accepted colours.
  const auto window_colour =
      [&](std::array<std::uint8_t, 3> accepted) -> std::array<std::uint8_t, 3> {
    if (content.race_palette_cycle.empty())
      return accepted;
    auto cgram = build_race_cgram(s, content.palette, false);
    apply_dragster_palette_cycle(cgram, content.race_palette_cycle, s.movement);
    return colour(cgram, 0);
  };
  // R-0040: with the recovered table family the pose pair is replaced by the
  // original's own per-frame selection. A DRAGSTER v1 pack carries only the two
  // frozen tables, so it keeps the accepted pose-keyed placement unchanged.
  const auto window_index = content.window_tables.empty()
                                ? std::optional<unsigned>{}
                                : dragster_window_table_index(s.movement);
  if (content.window_tables.empty()) {
    if (player_pose == 0x04f9 && opponent_pose == 0x0263)
      render_window_xor(f, content.go_window, window_colour({255, 255, 255}));
  } else if (window_index && *window_index <= 6) {
    render_window_xor(f, dragster_window_table(content.window_tables, *window_index),
                      window_colour({255, 255, 255}));
  }
  const auto &t = s.movement.timer;
  const std::array<unsigned, 4> d{
      {t.minutes, t.tens_seconds, t.seconds, t.tenths}};
  for (std::size_t i = 0; i < 4; ++i)
    rect(f, 8 + static_cast<int>(i) * 9, 8, 3 + static_cast<int>(d[i] % 5), 10,
         {238, 238, 224});
  std::array<std::uint8_t, 65536> rider_vram{};
  auto art_state = s.movement;
  if (rider_art != nullptr) {
    for (std::size_t rider = 0; rider < rider_art->size(); ++rider) {
      art_state.riders[rider].pose.pose_index = (*rider_art)[rider].pose_index;
      art_state.riders[rider].pose.reflected = (*rider_art)[rider].reflected;
    }
  }
  const PresentationSample art_sample{art_state,      s.camera_x,
                                      s.bg1_scroll_x, s.bg1_scroll_y,
                                      s.bg2_scroll_x, s.bg2_scroll_y};
  load_rider_tiles(rider_vram, art_sample, content.rider_tiles);
  const auto rider_cgram = build_race_cgram(s, content.palette);
  for (std::size_t rider_index = 0; rider_index < 2; ++rider_index) {
    const auto &rider = s.movement.riders[rider_index];
    const auto art_pose =
        rider_art == nullptr
            ? RiderArtPose{rider.pose.pose_index, rider.pose.reflected}
            : (*rider_art)[rider_index];
    (void)rider_frame_for_pose(art_pose.pose_index, art_pose.reflected);
    const std::int64_t wide_x = static_cast<std::int16_t>(rider.motion.x) -
                                static_cast<std::int64_t>(s.camera_x) - 832;
    const int y = static_cast<std::int16_t>(rider.motion.y) - 752;
    // A 64-pixel object wholly outside the 256-pixel screen cannot contribute.
    // Narrow only coordinates in the renderer's small, representable domain.
    if (wide_x > -64 && wide_x < 256)
      render_rider(f, rider_vram, rider_cgram, static_cast<int>(wide_x), y,
                   rider_index == 0 ? 0 : 136, rider_index == 0 ? 0x66 : 0x68);
  }
  if (content.window_tables.empty()) {
    if (player_pose == 0x04fe && opponent_pose == 0x037c)
      render_window_xor(f, content.winner_window, window_colour({98, 98, 255}));
  } else if (window_index && *window_index >= 7) {
    render_window_xor(f, dragster_window_table(content.window_tables, *window_index),
                      window_colour({98, 98, 255}));
  }
  return f;
}

RgbFrame render_dragster_headless(const PresentationSample &sample,
                                  const PresentationContent &content) {
  return render_dragster(sample, content, nullptr);
}

RgbFrame render_dragster_headless_with_rider_art(
    const PresentationSample &sample, const PresentationContent &content,
    const std::array<RiderArtPose, 2> &rider_art) {
  return render_dragster(sample, content, &rider_art);
}
namespace {
// Small authored UI font. Original scene artwork remains pack content.
std::array<unsigned,7> ui_glyph(char c) {
    switch(c) {
    case '0':return {14,17,19,21,25,17,14};case '1':return {4,12,4,4,4,4,14};
    case '2':return {14,17,1,2,4,8,31};case '3':return {30,1,1,14,1,1,30};
    case '4':return {2,6,10,18,31,2,2};case '5':return {31,16,16,30,1,1,30};
    case '6':return {14,16,16,30,17,17,14};case '7':return {31,1,2,4,8,8,8};
    case '8':return {14,17,17,14,17,17,14};case '9':return {14,17,17,15,1,1,14};
    case 'A':return {14,17,17,31,17,17,17};case 'B':return {30,17,17,30,17,17,30};
    case 'C':return {14,17,16,16,16,17,14};case 'D':return {30,17,17,17,17,17,30};
    case 'E':return {31,16,16,30,16,16,31};case 'F':return {31,16,16,30,16,16,16};
    case 'G':return {14,17,16,23,17,17,15};case 'H':return {17,17,17,31,17,17,17};
    case 'I':return {14,4,4,4,4,4,14};case 'J':return {7,2,2,2,18,18,12};
    case 'K':return {17,18,20,24,20,18,17};case 'L':return {16,16,16,16,16,16,31};
    case 'M':return {17,27,21,21,17,17,17};case 'N':return {17,25,21,19,17,17,17};
    case 'O':return {14,17,17,17,17,17,14};case 'P':return {30,17,17,30,16,16,16};
    case 'Q':return {14,17,17,17,21,18,13};case 'R':return {30,17,17,30,20,18,17};
    case 'S':return {15,16,16,14,1,1,30};case 'T':return {31,4,4,4,4,4,4};
    case 'U':return {17,17,17,17,17,17,14};case 'V':return {17,17,17,17,17,10,4};
    case 'W':return {17,17,17,21,21,21,10};case 'X':return {17,17,10,4,10,17,17};
    case 'Y':return {17,17,10,4,4,4,4};case 'Z':return {31,1,2,4,8,16,31};
    case ':':return {0,4,4,0,4,4,0};case '.':return {0,0,0,0,0,4,4};
    case '/':return {1,1,2,4,8,16,16};case '-':return {0,0,0,31,0,0,0};
    // The pause menu's selection marker. Without this glyph the menu still
    // emitted "> RESUME" but drew it identically to "  RESUME", so the focused
    // entry was indistinguishable and ENTER's target was unknowable.
    case '>':return {16,8,4,2,4,8,16};
    // Space must be explicit. It has no glyph of its own, so it used to reach
    // the default and render blank only because the default was blank; once
    // the default became a visible box, every space between words drew one.
    case ' ':return {0,0,0,0,0,0,0};
    // An unmapped character used to render blank, which hides the omission at
    // exactly the moment it matters. Draw a solid block instead: a hollow box
    // differs from 'O' only in its top and bottom rows at 5x7, so it reads as
    // a letter, which is worse than blank. A solid block cannot.
    default:return {31,31,31,31,31,31,31};
    }
}
void ui_text(RgbFrame& frame,int x,int y,std::string_view text,std::array<std::uint8_t,3> ink={255,240,220}) {
    for(char c:text) {
        const auto glyph=ui_glyph(c);
        for(int row=0;row<7;++row)
            for(int col=0;col<5;++col)
                if(glyph[static_cast<std::size_t>(row)]&(1U<<(4-col)))pixel(frame,x+col,y+row,ink);
        x+=6;
    }
}
// The result screen writes NO TIME for the 60000 no-time sentinel (stop-timeout
// original result, frames 32000-32100).
std::string result_time(unsigned value);
std::string race_time(unsigned value) {
    const auto digit=[](unsigned v){return static_cast<char>('0'+v%10);};
    return {digit(value/6000),':',digit(value/1000%6),digit(value/100),'.',digit(value/10),digit(value)};
}
std::string result_time(unsigned value) {
    return value>=60000U?"NO TIME":race_time(value);
}
}
// The channel-6 window family: 25 tables of 899 bytes at $15:8000, addressed
// through the 16-bit offsets of $83:E55C. The last byte of each is the HDMA
// run terminator, which `render_window_xor` does not consume.
static constexpr unsigned window_table_stride=899, window_table_body=898,
                          window_table_count=25;
// DRAGSTER's race vblank begins at $82:D7F5 + 6 updates, so the first frame the
// channel-6 setup $80:868E publishes is initialization frame 1328 plus 6. The
// countdown starts at $11C5 = 270 and the driver of frame n-1 chooses the table
// frame n shows, so that table was chosen with $11C5 = 270 - (n - 1334).
static constexpr std::uint32_t dragster_race_setup_frame=1334;
static constexpr unsigned dragster_window_countdown_start=270;
// $83:E759/E611/E663/E6C5 index 5 + $1229. $83:CC08 loads $1229 from $0BA7,
// the player's start reflection, which is 1 on DRAGSTER, so the legacy v1
// path's transitions draw index 6 (`classic_window_transition_member` derives
// it from the track for the shared renderer: 5 on ZOOM ZOO).
static constexpr unsigned dragster_window_transition_index=6;
// $83:EA19-$83:EA5B and its twin, one driver per rider ($0F03/$0F07 and
// $0F05/$0F09): members 7..24, the index stepping on every driver update
// whose $0300 parity flag is set, for a life of 360 driver updates counted
// from the driver's first odd update (random-1: opponent 3214, first driver
// update 3215 odd, blank from 3576; reversal: opponent 3325, first driver
// update 3326 even, blank from 3688). The other rider's driver, armed by its
// finish, runs once the first stops and starts at index zero (banner-pause-odd
// original: the opponent's driver dies on member 8 at 3636 after a 61-update
// pause, the player's requests 7 on 3637). Without a pause 360 updates are
// ten member cycles, which hid the second driver behind a phase that merely
// looked continuous.
static constexpr unsigned winner_window_first=7, winner_window_cycle=18,
                          winner_window_life_updates=360;

void ClassicWindowPointer::reset() {
    drivers_={};order_={};pending_={};ordered_=pending_count_=0;
    chosen_.reset();published_.reset();observed_=false;
}
void ClassicWindowPointer::observe_update(const ZoomZooState& previous,const ZoomZooState& updated,
                                          unsigned transition_member) {
    // R-0040: the vblank setup of frame N publishes the selection the drivers
    // made in update N-1; the race vblank runs from initialization + 6 and
    // for the last time on result-loading update 1.
    const auto scenario=classic_race_scenario(updated.track);
    observed_=true;
    if(updated.movement.frame>=scenario.initialization_frame+6U && updated.result_updates<=1U)
        published_=chosen_;
    // The pause menu disables channel 6 for every update it diverts, from the
    // update that opens it through the update that resumes (countdown-pause
    // original: no window on 1401-1502 for a pause opened on 1400 and resumed
    // on 1501) and any later update on which Start is still held (ZOOM ZOO
    // pause-countdown original: held 1460-1462, no window through 1463); the
    // drivers neither run nor age through it.
    if(zoom_zoo_update_was_paused(previous,updated)) {chosen_.reset();return;}
    if(updated.result_updates>1U)return;
    // Drivers armed by the previous update's finish run from this update.
    // Each rider finishes once per race, so at most two are ever armed; a
    // caller that observes a new race without reset() gets no third arming.
    for(std::size_t i=0;i<pending_count_ && ordered_<order_.size();++i)order_[ordered_++]=pending_[i];
    pending_count_=0;
    for(std::size_t rider=0;rider<2 && pending_count_<pending_.size();++rider)
        if(!previous.race.riders[rider].finished && updated.race.riders[rider].finished) {
            drivers_[rider]={};pending_[pending_count_++]=rider;
        }
    const bool parity_set=(updated.movement.frame&1U)!=0U;
    std::optional<unsigned> request;
    for(std::size_t i=0;i<ordered_ && !request;++i) {
        auto& driver=drivers_[order_[i]];
        if(driver.dead)continue;
        // $0F07/$0F09: set to 360 while the index is zero, counted down once
        // per driver update of either parity (random-1: 359 after 3215, zero
        // after 3574, blank from 3576; reversal: 359 after both 3326 and 3327
        // as its first update is even), frozen through a pause.
        if(driver.index==0U)driver.life=winner_window_life_updates;
        if(driver.life==0U) {driver.dead=true;continue;}
        --driver.life;
        if(parity_set)driver.index=driver.index==0U?8U:driver.index==24U?7U:driver.index+1U;
        request=driver.index==0U?winner_window_first:driver.index;
    }
    chosen_=request?request:classic_countdown_window(previous.movement.countdown,parity_set,transition_member);
}

namespace {
// The decoded track a race of either track runs on: the engine's own entry.
std::span<const std::uint8_t> classic_track_data(const ClassicContentPack& pack,ClassicRaceTrack track) {
    return pack.entry(track==ClassicRaceTrack::Dragster?"physics.track.dragster.data":"zoom.track-data");
}
} // namespace

void ClassicRaceHistoryTracker::reset() {
    look_={};latest_={};on_screen_={};opponent_finish_frame_.reset();window_.reset();clock_.reset();transition_member_.reset();
}
void ClassicRaceHistoryTracker::observe_update(const ZoomZooState& previous,const ZoomZooState& updated,
                                               const ClassicContentPack& pack) {
    // R-0036: update N builds its objects with overlays chosen from the look
    // state before its own look step; picture N+1 shows them.
    on_screen_=latest_;
    if(!previous.race.riders[1].finished && updated.race.riders[1].finished)
        opponent_finish_frame_=updated.movement.frame;
    if(!transition_member_)transition_member_=classic_window_transition_member(classic_track_data(pack,updated.track));
    window_.observe_update(previous,updated,*transition_member_);
    clock_.observe_update(previous,updated);
    if(updated.result_updates || zoom_zoo_update_was_paused(previous,updated))return;
    const auto tables=rider_look_tables(pack);
    const auto engine=updated.track==ClassicRaceTrack::Dragster?dragster_race_content(pack):zoom_zoo_content(pack);
    latest_.pose=rider_overlay_poses(look_,updated,tables);
    advance_rider_look(look_,updated,engine,tables);
}
std::optional<unsigned> zoom_zoo_palette_cycle_index(std::uint32_t frame) {
    // $82:D382-D496 runs from frame 1382 while $0B92 is set: it loads the
    // colours from index $0B84, then advances $0B84 modulo 16. $0B84 ends frame
    // n at (n-1381)&15, through pause, so frame n draws index (n-1382)&15.
    if(frame<1382U)return std::nullopt;
    return (frame-1382U)&15U;
}
namespace {
// Seventeen 16-word tables at $80:82AB: colours 96-111, then colour 0.
void load_race_palette_phase(std::array<std::uint8_t,512>& cgram,std::span<const std::uint8_t> tables,
                             unsigned index,bool colour_zero) {
    if(tables.size()!=544)throw std::invalid_argument("race palette cycle has the wrong size");
    for(std::size_t table=0;table<(colour_zero?17U:16U);++table) {
        const auto source=table*32U+index*2U;
        const auto destination=table<16?(96U+table)*2U:0U;
        cgram[destination]=tables[source];cgram[destination+1]=tables[source+1];
    }
}
// The frame whose race vblank selections are on screen: the race vblank, and
// with it the palette routine and the channel-6 setup, runs for the last time
// on result-loading update 1, so from update 2 the original keeps that update's
// selection ($420C is never rewritten after it, R-0040).
std::optional<std::uint32_t> race_vblank_frame(std::uint32_t frame,std::uint16_t loading_updates) {
    if(loading_updates==0)return frame;
    if(loading_updates-1U>frame)return std::nullopt;
    return frame-(loading_updates-1U);
}
struct RacePalettePhase {
    unsigned index;
    bool colour_zero;
};
// $82:D382-D496 first runs on `setup_frame`. Racing frame n draws phase
// (n-setup)&15 with colour 0; the loading freeze keeps colours 96-111 at the
// last vblank's phase with a black colour 0, until the result palettes load.
std::optional<RacePalettePhase> race_palette_phase(std::uint32_t frame,std::uint16_t loading_updates,
                                                   std::uint32_t setup_frame) {
    const auto shown=race_vblank_frame(frame,loading_updates);
    if(!shown || *shown<setup_frame)return std::nullopt;
    return RacePalettePhase{(*shown-setup_frame)&15U,loading_updates==0};
}
void apply_race_palette_phase(std::array<std::uint8_t,512>& cgram,std::span<const std::uint8_t> tables,
                              std::optional<RacePalettePhase> phase) {
    if(!phase)return;
    load_race_palette_phase(cgram,tables,phase->index,phase->colour_zero);
    if(!phase->colour_zero)cgram[0]=cgram[1]=0;
}
// The legacy finish struct's loading count, or zero while it is not loading.
std::uint16_t legacy_loading_updates(const RaceFinishState& finish) {
    return finish.phase==RacePhase::ResultLoading?finish.result_loading_updates:std::uint16_t{0};
}
}
void apply_zoom_zoo_palette_cycle(std::array<std::uint8_t,512>& cgram,std::span<const std::uint8_t> tables,
                                  std::uint32_t frame) {
    if(tables.size()!=544)throw std::invalid_argument("ZOOM ZOO race palette cycle has the wrong size");
    const auto index=zoom_zoo_palette_cycle_index(frame);
    if(!index)return;
    load_race_palette_phase(cgram,tables,*index,true);
}
void apply_dragster_palette_cycle(std::array<std::uint8_t,512>& cgram,std::span<const std::uint8_t> tables,
                                  const MovementState& state) {
    if(tables.size()!=544)throw std::invalid_argument("DRAGSTER race palette cycle has the wrong size");
    // Original DRAGSTER replays match (frame-1334)&15 on every racing frame,
    // and the phase holds from loading update 1 (3454, 3559) through at least
    // update 75 (R-0037).
    apply_race_palette_phase(cgram,tables,race_palette_phase(state.frame,legacy_loading_updates(state.finish),1334U));
}
void apply_classic_race_palette_cycle(std::array<std::uint8_t,512>& cgram,std::span<const std::uint8_t> tables,
                                      const ZoomZooState& state,std::uint32_t setup_frame) {
    if(tables.size()!=544)throw std::invalid_argument("race palette cycle has the wrong size");
    apply_race_palette_phase(cgram,tables,race_palette_phase(state.movement.frame,state.result_updates,setup_frame));
}

std::span<const std::uint8_t> dragster_window_table(
    std::span<const std::uint8_t> tables, unsigned index) {
    if(tables.size()!=window_table_count*window_table_stride)
        throw std::invalid_argument("Classic window table family has the wrong size");
    if(index>=window_table_count)
        throw std::invalid_argument("Classic window table index is out of range");
    const auto at=static_cast<std::size_t>(index)*window_table_stride;
    if(tables[at+window_table_body]!=0)
        throw std::invalid_argument("Classic window table lacks its run terminator");
    return tables.subspan(at,window_table_body);
}

namespace {
// Updates since the winning rider's finish: 0 on the update that records it.
// The banner driver runs from the next update, so it starts one frame later.
std::optional<std::uint32_t> updates_since_winner_finish(const RaceFinishState& finish) {
    if(finish.outcome==RaceOutcome::PlayerWon) {
        // The player owns the global finish delay, so its count is exact for
        // the whole banner: 0..240, held at 240 once result loading starts,
        // which is the last update on which the driver runs.
        if(finish.phase==RacePhase::FinishDelay ||
           (finish.phase==RacePhase::ResultLoading && finish.result_loading_updates))
            return finish.player_finish_delay;
        return std::nullopt;
    }
    if(finish.outcome==RaceOutcome::PlayerLost) {
        // The opponent's finish frame is only recoverable from the legacy
        // state while its 120-update finish animation counter runs (R-0040).
        const auto remaining=finish.finish_animation_countdown[1];
        if(remaining==0 || remaining>120U)return std::nullopt;
        return 120U-remaining;
    }
    return std::nullopt;
}
// Odd frames in [from, to): the banner steps of driver frames from..to-1.
std::uint32_t odd_frames(std::uint32_t from,std::uint32_t to) {
    return to<=from?0U:to/2U-from/2U;
}
// The selection on screen for `frame`, shared by both state forms, from the
// frames of the first and the latest finish (equal when one rider finished),
// as `ClassicWindowPointer` would publish it without a diverted update.
std::optional<unsigned> window_table_index_for(std::uint32_t frame,std::uint16_t loading_updates,
                                               std::optional<std::uint32_t> first_finish,
                                               std::optional<std::uint32_t> latest_finish,
                                               std::uint32_t setup_frame,unsigned transition_member) {
    const auto shown=race_vblank_frame(frame,loading_updates);
    if(!shown)return std::nullopt;
    // The winner banner replaces the countdown family; the two never overlap in
    // a race the countdown can hold at the line. The picture shows the choice
    // of driver frame shown - 1.
    if(first_finish) {
        const auto first=*first_finish;
        if(*shown<first+2U)return std::nullopt; // the finish update selects nothing
        const auto driver=*shown-1U;
        // The first driver's life runs from its first odd update; its phase
        // is the odd updates since its first update.
        const auto first_odd=((first+1U)&1U)!=0U?first+1U:first+2U;
        if(driver<=first_odd+winner_window_life_updates-1U)
            return winner_window_first+static_cast<unsigned>(odd_frames(first+1U,*shown)%winner_window_cycle);
        // It stopped on the update after; the other rider's driver, if armed,
        // runs from the later of that update and the update after its finish.
        if(!latest_finish || *latest_finish<=first)return std::nullopt;
        const auto second=std::max(first_odd+winner_window_life_updates,*latest_finish+1U);
        const auto second_odd=(second&1U)!=0U?second:second+1U;
        if(driver<second || driver>second_odd+winner_window_life_updates-1U)return std::nullopt;
        return winner_window_first+static_cast<unsigned>(odd_frames(second,*shown)%winner_window_cycle);
    }
    if(*shown<setup_frame)return std::nullopt;
    const auto elapsed=*shown-setup_frame;
    if(elapsed>=dragster_window_countdown_start)return std::nullopt;
    // The driver of frame shown - 1 read 270 - elapsed and its own parity.
    return classic_countdown_window(static_cast<std::uint16_t>(dragster_window_countdown_start-elapsed),
                                    ((*shown-1U)&1U)!=0U,transition_member);
}
} // namespace

unsigned classic_window_transition_member(std::span<const std::uint8_t> decoded_track) {
    // $83:CC05-CC08 (the race setup that also fixes `$1281`, R-0038) stores
    // the player's reflection word `$0BA7` in `$1229`; the transition sites
    // ($83:E611, E663, E6C5, E763) select member 5 + `$1229`. At that moment
    // `$0BA7` is the start reflection the track header sets, so the member is
    // 6 on DRAGSTER and 5 on ZOOM ZOO (both tracks' captures; the ZOOM ZOO
    // originals show `$1229` = 0 throughout with member 5 on 1382-1402,
    // 1432-1462, 1492-1522 and 1552-1582).
    return 5U+(classic_race_start_reflected(decoded_track,0)?1U:0U);
}

std::optional<unsigned> classic_countdown_window(std::uint16_t countdown,bool parity_set,unsigned transition_member) {
    // $83:E59C dispatches on $11C5 as the update read it, before its own
    // decrement, and inside each digit on the digit's own threshold: above it
    // the transition table, below it the digit's table.
    if(countdown==0U)return std::nullopt;
    if(countdown>=250U)return transition_member;
    if(countdown>=221U)return 0U;
    if(countdown>=190U)return transition_member;
    if(countdown>=161U)return 1U;
    if(countdown>=130U)return transition_member;
    if(countdown>=101U)return 2U;
    if(countdown>=70U)return transition_member;
    // $83:E728 picks between the two GO tables on the $0300 parity: an odd
    // driver update's choice is member 3, on screen on the even frame after it.
    return parity_set?3U:4U;
}

std::optional<unsigned> dragster_window_table_index(const MovementState& state) {
    const auto loading=legacy_loading_updates(state.finish);
    std::optional<std::uint32_t> finish;
    if(const auto since=updates_since_winner_finish(state.finish))
        if(const auto shown=race_vblank_frame(state.frame,loading))finish=*shown-*since;
    return window_table_index_for(state.frame,loading,finish,finish,dragster_race_setup_frame,
                                  dragster_window_transition_index);
}

std::optional<std::uint32_t> classic_opponent_finish_frame(const ZoomZooState& state) {
    // Both finish times are needed, so this serves a restored state only once
    // the player has finished: it recovers the banner's remainder when the
    // player finished within its 360 frames (lose-a: 104 frames after the
    // opponent) and nothing when the whole banner preceded the player's finish
    // (random-1: 422 frames after, reversal: 967). Live play does not depend on
    // it; the history tracker records the opponent's finish as it happens.
    const auto& race=state.race;
    if(!race.riders[0].finished || !race.riders[1].finished)return std::nullopt;
    if(race.total_times[0]>=60000U || race.total_times[1]>=60000U)return std::nullopt;
    // The finish delay counts once per race update and holds at 240 from the
    // update before result loading, so the frame it last advanced on is the
    // loading start minus the loading count (lose-a: finish 3318, delay 240 at
    // 3558, loading update 1 at 3559).
    const auto counted=state.result_updates?state.movement.frame-std::min<std::uint32_t>(state.movement.frame,state.result_updates)
                                          :state.movement.frame;
    if(race.finish_delay>counted)return std::nullopt;
    const auto player_finish=counted-race.finish_delay;
    // finish_centiseconds: two per frame plus the frame parity, so
    // total[0]-total[1] = 2(fa-fb)+(fa&1)-(fb&1); one parity of fb fits, in
    // either finish order.
    const int difference=static_cast<int>(race.total_times[0])-static_cast<int>(race.total_times[1]);
    for(const unsigned parity:{0U,1U}) {
        const int twice_gap=difference-static_cast<int>(player_finish&1U)+static_cast<int>(parity);
        if(twice_gap%2!=0)continue;
        const int gap=twice_gap/2;
        if(gap>static_cast<int>(player_finish))continue;
        const auto opponent_finish=static_cast<std::uint32_t>(static_cast<int>(player_finish)-gap);
        if((opponent_finish&1U)==parity)return opponent_finish;
    }
    return std::nullopt;
}

std::optional<unsigned> classic_window_table_index(const ZoomZooState& state,std::uint32_t setup_frame,
                                                   std::optional<std::uint32_t> opponent_finish_frame,
                                                   unsigned transition_member) {
    const auto& race=state.race;
    // The player's finish frame from its delay (0..240, held at 240 from the
    // update before result loading); the opponent's from the history or, once
    // both have finished, from the two finish times. The first finisher's
    // driver runs first; the other's follows once it stops.
    std::optional<std::uint32_t> player_finish,opponent_finish=opponent_finish_frame;
    if(race.riders[0].finished) {
        const auto counted=state.result_updates?state.movement.frame-std::min<std::uint32_t>(state.movement.frame,state.result_updates)
                                              :state.movement.frame;
        if(race.finish_delay<=counted)player_finish=counted-race.finish_delay;
    }
    if(race.riders[1].finished && !opponent_finish)opponent_finish=classic_opponent_finish_frame(state);
    std::optional<std::uint32_t> first,latest;
    for(const auto& finish:{player_finish,opponent_finish}) {
        if(!finish)continue;
        first=first?std::min(*first,*finish):*finish;
        latest=latest?std::max(*latest,*finish):*finish;
    }
    return window_table_index_for(state.movement.frame,state.result_updates,first,latest,setup_frame,transition_member);
}

unsigned classic_race_prior_fade(const ZoomZooState& state,const ZoomZooState* previous_update,
                                 const ClassicRaceScenario& scenario) {
    // $0FF1 grows by one per race update from initialization and holds at 30
    // ($83:CCC1-CCC9); the single-state runner passes the state as its own
    // previous, and after saturation the state alone cannot give the
    // preceding level, so the accepted frame formula is used there.
    if(previous_update && previous_update->movement.frame<state.movement.frame)
        return std::min(30U,unsigned(previous_update->fade_level));
    const auto first=scenario.initialization_frame;
    return state.movement.frame<=first?0U:std::min(30U,state.movement.frame-first-1U);
}

RaceFinishState classic_finish_view(const ZoomZooState& race) {
    RaceFinishState finish=race.movement.finish;
    for(std::size_t rider=0;rider<2;++rider) {
        const auto& lap=race.race.riders[rider];
        finish.rider_finished[rider]=lap.finished!=0;
        if(!lap.finished)continue;
        // One lap: the recorded crossing digits and the total are the finish time.
        finish.finish_time_digits[rider]=lap.time_digits;
        finish.finish_time_centiseconds[rider]=race.race.total_times[rider];
    }
    finish.player_finish_delay=race.race.finish_delay;
    finish.result_loading_updates=race.result_updates;
    if(race.race.riders[0].finished)
        finish.outcome=classic_race_player_won(race)?RaceOutcome::PlayerWon:RaceOutcome::PlayerLost;
    if(race.result_updates && race.result_updates>=stable_result_updates(race))
        finish.phase=RacePhase::ResultScreen;
    else if(race.result_updates)
        finish.phase=RacePhase::ResultLoading;
    else if(race.race.riders[0].finished)
        finish.phase=RacePhase::FinishDelay;
    return finish;
}

unsigned classic_hud_lap(unsigned laps_remaining,unsigned laps) {
    return std::min(laps,laps+1U-std::min(laps+1U,laps_remaining));
}
// R-0043: the finish time as the original's own two fields spell it, colons
// between all three parts: `1:38:02` for 9,802 centiseconds.
std::string hud_time(unsigned centiseconds) {
    const auto digit=[](unsigned v){return static_cast<char>('0'+v%10);};
    return {digit(centiseconds/6000),':',digit(centiseconds/1000%6),digit(centiseconds/100),
            ':',digit(centiseconds/10),digit(centiseconds)};
}
// $81:EB8E, and the race setup that wrote the field before it.
ClassicHudField classic_hud_left_field(const ZoomZooState& published,const ClassicRaceScenario& scenario) {
    const auto& race=published.race;
    // At the 10:00 limit ($81:C73E-C75B) the player is finished with laps left,
    // so `$0EFB` is not zero and the field keeps the lap (stop-timeout original
    // frames 31578-31920 all read `2/3`).
    if(race.riders[0].finished && race.riders[0].laps_remaining==0)return {"finish",1};
    // The word wins over the lap count, so a DRAGSTER finish replaces `race`
    // too, and $053F only suppresses the lap number.
    if(!scenario.tour_race)return {"race",2};
    // $81:EC61-$81:ECBC puts the lap's ones digit at column 2 and its tens at
    // column 1, and $81:D68C-$81:D6E5 the total from column 4, so the count is
    // right-aligned on column 2 and the total left-aligned on 4. Neither track
    // of this product reaches a second digit in either.
    const auto lap=classic_hud_lap(race.riders[0].laps_remaining,scenario.laps);
    return {std::to_string(lap)+"/"+std::to_string(scenario.laps),lap>=10U?1U:2U};
}
std::string classic_hud_clock(const RaceTimerDigits& t) {
    return {static_cast<char>('0'+t.minutes%10),':',static_cast<char>('0'+t.tens_seconds%10),
            static_cast<char>('0'+t.seconds%10),':',static_cast<char>('0'+t.tenths%10)};
}
void ClassicRaceHudClock::observe_update(const ZoomZooState& previous,const ZoomZooState& updated) {
    on_screen_=latest_;
    // The clock keeps the digits it holds only on an update where the left
    // field is actually **written**, because only then does the queue return
    // ($81:EC5E and $81:EB98 both end at $81:ECBC, which clears `$0D17` and
    // jumps to $81:F357). Two paths write:
    //
    //   - a tour race whose dirty flag is set, which is either rider's lap
    //     counter stepping - `$0EFB` for the player and `$0EFD` for the
    //     opponent, whose crossing dirties the field without changing what it
    //     shows (review 2 B1: on `ordinary-controls/down-a` the player crosses
    //     on 3207 and the opponent on 3208, and picture 3209 still reads the
    //     pre-tick digits);
    //   - the player's laps reaching zero, which writes `finish` on either
    //     track.
    //
    // A dirty flag is **not** enough on its own. When `$053F` is set - the
    // mode-0 race, DRAGSTER, whose field is the fixed word `race` - $81:EB93
    // falls through at $81:EB9B to the clock handler instead of returning, and
    // nothing ever clears `$0D17`, so the flag stands set for the rest of the
    // race while the clock goes on being republished every update. Holding on
    // any counter step drew the tenths a picture late on three of DRAGSTER's
    // four crossings (review 3 B1, measured on four accepted DRAGSTER
    // originals, including a player crossing 1,600 updates before the finish).
    const auto stepped=[&](std::size_t rider) {
        return previous.race.riders[rider].laps_remaining!=updated.race.riders[rider].laps_remaining;
    };
    const bool wrote_lap=classic_race_scenario(updated.track).tour_race && (stepped(0) || stepped(1));
    const bool wrote_finish=previous.race.riders[0].laps_remaining!=0 && updated.race.riders[0].laps_remaining==0;
    if(!wrote_lap && !wrote_finish)latest_=classic_hud_clock(updated.movement.timer);
}
ClassicHudText classic_race_hud_text(const ZoomZooState& previous_update,
                                     const ClassicRaceScenario& scenario,
                                     std::optional<std::uint32_t> opponent_finish_frame,
                                     const std::optional<std::string>& published_clock) {
    const auto& race=previous_update.race;
    ClassicHudText hud;
    const bool finished=race.riders[0].finished && race.riders[0].laps_remaining==0;
    const auto left=classic_hud_left_field(previous_update,scenario);
    hud.left=left.text;hud.left_column=left.column;
    // The corner clock shows tenths, one digit per field. Every gate below is a
    // *picture* number, not an update number: picture N is drawn from the state
    // after update N-1, so a field the queue writes on update F shows in
    // picture F and the state this function reads then has `finish_delay ==
    // F - finish - 1`. The finish is update `finish`, `$81:ECCF` blanks the
    // clock on `finish + 2`, and that picture reads delay 1 (review B1:
    // measured against consecutive originals 6480-6500 of the M4-16 primary
    // and 6465-6500 of compound-reverse, which the kept 20-frame pictures
    // stepped straight over).
    if(!finished || race.finish_delay<1)
        hud.clock=published_clock?*published_clock:classic_hud_clock(previous_update.movement.timer);
    // The player's own time is written on `finish + 3`, which reads delay 2,
    // and the opponent's two updates after the opponent finishes, which is the
    // picture one update after that frame.
    // The 60000 no-time sentinel is not a finish time on either side. A player
    // who finishes all laps always has a real total, so this guard is belt and
    // braces rather than a measured case (review 2 A8).
    if(finished && race.finish_delay>=2 && race.total_times[0]<60000U)
        hud.player_time=hud_time(race.total_times[0]);
    if(race.riders[1].finished && race.total_times[1]<60000U) {
        const auto opponent=opponent_finish_frame?opponent_finish_frame:classic_opponent_finish_frame(previous_update);
        if(opponent && previous_update.movement.frame>=*opponent+1U)
            hud.opponent_time=hud_time(race.total_times[1]);
    }
    return hud;
}

ClassicRacePresentationContent classic_race_presentation_content(const ClassicContentPack& pack,ClassicRaceTrack track) {
    ClassicRacePresentationContent content;
    content.scenario=classic_race_scenario(track);
    content.riders=rider_object_content(pack);
    // One ROM table serves both tracks (R-0037); its accepted entry name
    // predates DRAGSTER reading it and cannot be renamed.
    content.race_palette_cycle=pack.entry("presentation.zoom.race-palette-cycle.v1");
    // One ROM window family serves both tracks too (R-0040; ZOOM ZOO's
    // selection measured in ZOOM-ZOO-WINDOW-EFFECTS).
    content.window_tables=pack.entry("presentation.effect.classic.window-tables.v1");
    // R-0042: the caption table, likewise one table for both tracks.
    content.captions=pack.entry("presentation.classic.captions.v1");
    content.caption_font=pack.entry("presentation.classic.font.v1");
    content.track=classic_track_data(pack,track);
    content.window_transition_member=classic_window_transition_member(content.track);
    switch(track) {
    case ClassicRaceTrack::ZoomZoo:
        content.track_name="ZOOM ZOO";
        content.bg1_tiles=pack.entry("zoom.bg1-tiles");
        content.bg2_tiles=pack.entry("zoom.bg2-tiles");
        content.bg2_map=pack.entry("zoom.bg2-map");
        content.palette=pack.entry("zoom.palette");
        break;
    case ClassicRaceTrack::Dragster:
        content.track_name="DRAGSTER";
        content.bg1_tiles=pack.entry("presentation.track.dragster.bg1-tiles.v1");
        content.bg2_tiles=pack.entry("presentation.track.dragster.bg2-tiles.v1");
        content.bg2_map=pack.entry("presentation.track.dragster.bg2-map.v1");
        content.palette=pack.entry("presentation.classic.palette.v1");
        content.result_assets=pack.entry("presentation.result.classic.font-layout.v1");
        content.result_base_vram=pack.entry("presentation.result.classic.base-vram.v1");
        content.result_palette=pack.entry("presentation.result.classic.palette.v1");
        content.result_palette_tail=pack.entry("presentation.result.classic.palette-tail.v1");
        break;
    }
    content.geometry=track_geometry(content.track);
    return content;
}

// R-0042: the original's on-screen captions. The player's reward queue is the
// only trigger: the event the read cursor points at indexes sixteen ASCII bytes
// in the pack's caption table, and the update after the queue consumed it the
// original writes two tilemap rows for it. Each glyph is eight pixels wide and
// sixteen tall, its halves one font row apart, so a character maps to a top
// tile and the tile `$10` above it.
//
// Every byte of entries 1 to 255 is a space, `!`, `"`, `-` or a lowercase
// letter; there is no `q` and no digit. The letters and the space are measured
// against the original's own frames, and the three punctuation glyphs are read
// from the pack's font sheet, where they are unmistakable; no original picture
// in evidence shows one. The voice entries the engine can publish to the
// player ($81:C0CE's 72-87) include three that hold `"`, so refusing them
// would end a race the original plays through.
std::optional<unsigned> classic_caption_tile(char glyph) {
    if(glyph==' ')return std::nullopt;
    if(glyph>='a' && glyph<='z') {
        const unsigned n=static_cast<unsigned>(glyph-'a');
        if(n<5)return 0x0bU+n;
        if(n<21)return 0x20U+n-5U;
        return 0x40U+n-21U;
    }
    // R-0043: the HUD's characters, from the same sheet. The original's
    // character table $80:81F4 runs 0-9 then a-z then the punctuation, and the
    // sheet's own order is 16 glyph tops followed by their 16 bottoms, which is
    // why the digits sit below `a` and `:` and `/` sit above `z`.
    if(glyph>='0' && glyph<='9')return glyph=='0'?0x0aU:static_cast<unsigned>(glyph-'0');
    switch(glyph) {
    case '!':return 0x60U;
    case '"':return 0x61U;   // the table spells an apostrophe this way
    case '-':return 0x4dU;
    case ':':return 0x45U;
    case '/':return 0x4eU;
    default:break;
    }
    throw std::invalid_argument("unsupported Classic caption glyph");
}

// The caption is cleared by the queue, not by a timer: a hint sentence ends by
// publishing an entry of sixteen spaces, and $81:BEA8-BEF1 blanks the display
// when the queue runs dry, which the engine carries as `empty_display`.
std::optional<std::span<const std::uint8_t>>
classic_caption_entry(const ZoomZooState& published,std::span<const std::uint8_t> captions) {
    if(captions.size()!=4080)return std::nullopt;
    const auto& announcements=published.player_announcements;
    if(announcements.empty_display)return std::nullopt;
    // `movement.rewards` is the opponent's queue ($0D11/$0D13 cursors); the
    // player's, the one the captions follow, is the announcements' own.
    const auto& queue=announcements.queue;
    const unsigned event=queue.entries[queue.read_cursor];
    if(event==0)return std::nullopt;
    return captions.subspan((event-1U)*16U,16U);
}

namespace {
// One BG3 text cell of the race screen. A glyph is eight pixels wide and
// sixteen tall, drawn as the tile the character names and the tile $10 above
// it, so a field at tilemap row r covers rows r and r+1. BG3 scrolls by one
// line, which is why the caption's row 10 shows at y 79 and the HUD's row 2
// at y 15 (R-0042, R-0043).
void draw_bg3_text(RgbFrame& frame,std::span<const std::uint8_t> font,unsigned column,unsigned row,
                   std::string_view text,std::array<std::uint8_t,3> ink,std::bitset<256*224>& inked) {
    for(std::size_t index=0;index<text.size();++index) {
        const auto tile=classic_caption_tile(text[index]);
        if(!tile)continue;
        for(unsigned half=0;half<2;++half) {
            const auto at=static_cast<std::size_t>(*tile+half*0x10U)*16U;
            for(unsigned line=0;line<8;++line) {
                const unsigned low=font[at+2U*line],high=font[at+2U*line+1U];
                for(unsigned bit=0;bit<8;++bit) {
                    if(((low>>(7U-bit))&1U)|((high>>(7U-bit))&1U)) {
                        const int x=int(column+index)*8+int(bit),y=int(row)*8-1+int(half)*8+int(line);
                        if(x<0 || x>=256 || y<0 || y>=224)continue;
                        pixel(frame,x,y,ink);
                        inked.set(static_cast<std::size_t>(y)*256+static_cast<std::size_t>(x));
                    }
                }
            }
        }
    }
}
void draw_classic_caption(RgbFrame& frame,const ZoomZooState& published,
                          const ClassicRacePresentationContent& content,
                          std::array<std::uint8_t,3> ink,std::bitset<256*224>& inked) {
    const auto font=content.caption_font;
    if(font.size()!=2048)return;
    const auto selected=classic_caption_entry(published,content.captions);
    if(!selected)return;
    const auto entry=*selected;
    // $81:F322/$81:F33C write sixteen characters to columns 8-23 of rows 10-11.
    std::string line;
    for(unsigned column=0;column<16;++column)line.push_back(static_cast<char>(entry[column]));
    draw_bg3_text(frame,font,8,10,line,ink,inked);
}
// R-0043: the HUD's four fields, on the same layer, in the same font and the
// same colour as the caption. The original's own tilemap rows and columns:
// the left field and the clock on rows 2-3, the player's finish time on rows
// 5-6 and the opponent's on rows 20-21, both from column 13.
void draw_classic_hud(RgbFrame& frame,const ZoomZooState& published,
                      const ClassicRacePresentationContent& content,
                      std::optional<std::uint32_t> opponent_finish_frame,
                      const std::optional<std::string>& published_clock,
                      std::array<std::uint8_t,3> ink,std::bitset<256*224>& inked) {
    const auto font=content.caption_font;
    if(font.size()!=2048)return;
    const auto hud=classic_race_hud_text(published,content.scenario,opponent_finish_frame,published_clock);
    draw_bg3_text(frame,font,hud.left_column,2,hud.left,ink,inked);
    draw_bg3_text(frame,font,24,2,hud.clock,ink,inked);
    draw_bg3_text(frame,font,13,5,hud.player_time,ink,inked);
    draw_bg3_text(frame,font,13,20,hud.opponent_time,ink,inked);
}
} // namespace

RgbFrame render_classic_race(const ZoomZooState& state,const ClassicRacePresentationContent& content,
                             const ZoomZooState* previous_update,const ClassicRaceHistory* history) {
    RgbFrame frame{};
    const auto& scenario=content.scenario;
    const bool recovered_result=!content.result_base_vram.empty();
    if(state.result_updates && !recovered_result) {
        // Authored tour result (M4-16): black until the original's first
        // visible result frame, then the lap graph fades in.
        if(state.result_updates<=108)return frame;
        rect(frame,0,0,256,224,{34,42,48});
        ui_text(frame,70,12,state.race.total_times[0]<state.race.total_times[1]?"WINNER":"RUNNER UP");
        ui_text(frame,12,32,"PLAYER       TOTAL    BEST LAP");
        const unsigned minimum=state.result.graph_minimum,maximum=state.result.graph_maximum;
        for(unsigned i=0;i<2;++i) {
            unsigned best=60000;
            for(auto lap:state.race.lap_times[i])if(lap<60000) {best=std::min(best,unsigned(lap));}
            ui_text(frame,12,45+int(i)*13,i?"BRONSEN":"MIKE");
            ui_text(frame,90,45+int(i)*13,result_time(state.result.published_totals[i]));
            ui_text(frame,156,45+int(i)*13,result_time(best));
        }
        // $83:905F-90ED excludes sentinels and enforces a 200cs graph range.

        rect(frame,45,83,1,96,{180,180,100});rect(frame,45,178,170,1,{180,180,100});
        ui_text(frame,3,83,race_time(maximum));ui_text(frame,3,169,race_time(minimum));
        for(unsigned i=0;i<2;++i)for(unsigned lap=0;lap<3;++lap) {
            const auto time=state.race.lap_times[i][lap];
            if(time>=60000)continue;
            const int y=178-static_cast<int>((time-minimum)*90U/std::max(1U,maximum-minimum));
            rect(frame,76+int(lap)*55+int(i)*5,y-2,4,4,i?std::array<std::uint8_t,3>{255,190,70}:std::array<std::uint8_t,3>{255,80,90});
        }
        ui_text(frame,70,190,std::string("LAPS ON ")+std::string(content.track_name));ui_text(frame,49,208,"ENTER TO RACE AGAIN");
        const unsigned brightness=std::min(14U,unsigned(state.result_updates-108U)*2U);
        for(auto& channel:frame.pixels)channel=static_cast<std::uint8_t>(unsigned(channel)*brightness/14U);
        return frame;
    }
    if(state.result_updates) {
        // The original publishes the completed result at end-of-frame 3678. The
        // accepted gameplay state reaches ResultScreen on the following update,
        // so presentation consumes the observed loading counter without
        // altering the gameplay transition or its serialization.
        const auto finish=classic_finish_view(state);
        const bool result_visible=finish.phase==RacePhase::ResultScreen ||
            (finish.phase==RacePhase::ResultLoading && finish.outcome==RaceOutcome::PlayerWon &&
             finish.result_loading_updates>=225);
        if(result_visible) {
            render_result_background(frame,finish,state.movement.timer,
                                     {content.palette,content.result_assets,content.result_base_vram,
                                      content.result_palette,content.result_palette_tail});
            return frame;
        }
        // Until then the race picture stays, with the palette phase and window
        // selection of the last race vblank (R-0037, R-0040).
    }
    const auto track=content.track;
    std::array<std::uint8_t,65536> vram{};
    // BG1 is 16-by-16 tiles: 128 bytes each, the lower row of 8-by-8 tiles
    // 512 bytes above the upper. Both tracks' packs carry them in this order.
    const auto tiles=content.bg1_tiles;
    for(std::size_t tile=0;tile<tiles.size()/128;++tile) {
        const auto destination=0x4000+(tile/8)*1024+(tile%8)*64;
        std::copy_n(tiles.begin()+static_cast<std::ptrdiff_t>(tile*128),64,vram.begin()+static_cast<std::ptrdiff_t>(destination));
        std::copy_n(tiles.begin()+static_cast<std::ptrdiff_t>(tile*128+64),64,vram.begin()+static_cast<std::ptrdiff_t>(destination+512));
    }
    std::copy(content.bg2_tiles.begin(),content.bg2_tiles.end(),vram.begin()+0x2000);
    std::copy(content.bg2_map.begin(),content.bg2_map.end(),vram.begin()+0xe000);
    // NMI $80883F-8849 writes INIDISP from the preceding update's $0FF1,
    // clamping (fade-15) at zero; $83CCC1-CCC9 increments $0FF1 once per race
    // update. The PPU scales each 5-bit channel before output conversion
    // (bsnes lightTable: luma*c+0.5), so brightness applies to CGRAM words, as
    // in the DRAGSTER result fade. Scaling converted pixels made mid-fade
    // frames too bright: green 15 at brightness 8 is 47 in the original, not 64.
    const bool previous_is_prior=previous_update && previous_update->movement.frame<=state.movement.frame;
    const unsigned prior_fade=classic_race_prior_fade(state,previous_update,scenario);
    const auto brightness=prior_fade>15U?prior_fade-15U:0U;
    // Colours 96-111 and 0 are cycled by the race NMI from ROM tables every
    // frame (R-0037); neither track keys them to rider poses here.
    auto cgram=build_race_cgram(content.palette,false);
    apply_classic_race_palette_cycle(cgram,content.race_palette_cycle,state,scenario.initialization_frame+6U);
    if(brightness<15U) {
        for(std::size_t at=0;at<cgram.size();at+=2) {
            const auto faded=apply_snes_brightness(static_cast<std::uint16_t>(cgram[at]|(unsigned(cgram[at+1])<<8U)),brightness);
            cgram[at]=static_cast<std::uint8_t>(faded);cgram[at+1]=static_cast<std::uint8_t>(faded>>8U);
        }
    }
    // Authored UI has no CGRAM entry; fade it through the same 1.5 output curve.
    const auto ui_scale=std::pow(brightness/15.0,1.5);
    const auto ui=[ui_scale](std::array<std::uint8_t,3> rgb) {
        for(auto& channel:rgb)channel=static_cast<std::uint8_t>(std::lround(channel*ui_scale));
        return rgb;
    };
    const std::array<std::uint8_t,3> ink=ui({255,240,220});
    const int camera_x=state.race.camera.x,camera_y=static_cast<std::int16_t>(state.race.camera.y);
    // HDMA tables are published before the current camera update. Original
    // end1382..6724 tables equal previous camera minus the initial origin;
    // BG2 uses a logical word shift, including negative wrapped scrolls.
    // Paused updates leave the camera still while velocity persists, so
    // camera - velocity is the previous camera only when the previous update
    // moved it: use the previous update's camera whenever it is available.
    // DRAGSTER publishes the same relation (original 3453: camera 25266,
    // BG1 24434, BG2 12217 against origin 832).
    const auto& geometry=content.geometry;
    const int background_x=previous_is_prior?previous_update->race.camera.x&geometry.position_mask
        :(camera_x-static_cast<std::int16_t>(state.race.camera.velocity_x))&geometry.position_mask;
    const int background_y=previous_is_prior?static_cast<std::int16_t>(previous_update->race.camera.y)
        :static_cast<std::int16_t>(static_cast<std::uint16_t>(camera_y-static_cast<std::int16_t>(state.race.camera.velocity_y)));
    const auto origin_x=static_cast<std::uint16_t>(((unsigned(word(track,3))<<4)-256U)&0xfff0U);
    const auto origin_y=static_cast<std::uint16_t>(((unsigned(word(track,5))<<4)-256U)&0xfff0U);
    const int bg_x=static_cast<std::uint16_t>(background_x-origin_x)>>1U;
    const int bg_y=static_cast<std::uint16_t>(background_y-origin_y)>>1U;
    // $81:A304-A51B: the playfield is 16,384 coarse cells of 64 units in the
    // track's column count (256 by 64 for ZOOM ZOO, 1,024 by 16 for DRAGSTER).
    const int columns=geometry.coarse_columns;
    const int world_width=columns*64,world_height=(16384/columns)*64;
    // BG1 map entries with bit 13 set are drawn above priority-2 OBJs.
    std::array<bool,256*224> bg1_above_objects{};
    for(int y=0;y<224;++y)for(int x=0;x<256;++x) {
        const auto background=background_pixel(vram,0xe000,true,true,0x2000,false,static_cast<std::int16_t>(bg_x),static_cast<std::int16_t>(bg_y),x,y);
        pixel(frame,x,y,colour(cgram,background));
        // Screen row 0 is scanline 1, as in background_pixel's vertical +1.
        const int world_x=background_x+x,world_y=background_y+y+1;
        if(world_x<0 || world_y<0 || world_x>=world_width || world_y>=world_height)continue;
        const auto selector=word(track,15+static_cast<std::size_t>((world_y/64)*columns+world_x/64)*2);
        const auto descriptor=word(track,0x800f+static_cast<std::size_t>(selector)*32+static_cast<std::size_t>((world_y%64)/16)*8+static_cast<std::size_t>((world_x%64)/16)*2);
        int px=world_x&15,py=world_y&15;
        if(descriptor&0x4000)px=15-px;
        if(descriptor&0x8000)py=15-py;
        const auto tile=static_cast<std::uint16_t>(((descriptor&1023)+(px/8)+(py/8)*16)&1023);
        const auto value=tile_pixel(vram,0x4000,tile,px&7,py&7);
        if(value) {
            pixel(frame,x,y,colour(cgram,static_cast<std::uint8_t>(((descriptor>>10)&7)*16+value)));
            bg1_above_objects[static_cast<std::size_t>(y)*256+static_cast<std::size_t>(x)]=(descriptor&0x2000)!=0;
        }
    }
    // R-0040: the countdown and winner windows show colour 0 through colour
    // math. With history the member is the one the vblank published for this
    // picture; a single restored state derives it from its counters and frame
    // (exact when no pause intervened, R-0040).
    const auto window_index=content.window_tables.empty()?std::optional<unsigned>{}
        :history && history->window_published?history->window_table
        :classic_window_table_index(state,scenario.initialization_frame+6U,
                                    history?history->opponent_finish_frame:std::nullopt,
                                    content.window_transition_member);
    const auto window_colour=colour(cgram,0);
    // R-0036: picture N shows the OBJ tiles and OAM the original published in
    // update N-1, like the BG scroll above. Entry 98 (player, tile base 0,
    // palette 3) has priority over entry 99 (opponent, base $88, palette 4);
    // both use OBJ priority 2. Without a previous update the riders are drawn
    // from this state, one update ahead.
    const auto& rider_source=previous_update?*previous_update:state;
    // R-0042: the caption is drawn here, behind the riders. Where a rider covers a glyph
    // the original does not hide the ink and does not paint it either: it adds
    // red to the sprite, `red = min(31, sprite_red + 13)` with green and blue
    // untouched, measured over 35 such pixels on frame 2100 of the M4-16
    // primary and the same on 2120, 2340 and 2600. That is colour-math
    // arithmetic; which PPU configuration produces it, and why the added 13 is
    // not half of the ink's own 5-bit 28, are not recovered. Where no sprite
    // covers the ink the caption is the flat colour, which matches the
    // original on every other measured frame of both tracks.
    std::bitset<256*224> caption_ink;
    draw_classic_caption(frame,rider_source,content,colour(cgram,22),caption_ink);
    // R-0043: the HUD is the same BG3 layer as the caption, so it composes the
    // same way: over the track, under the riders with the measured red add, and
    // under the channel-6 window members.
    draw_classic_hud(frame,rider_source,content,history?history->opponent_finish_frame:std::nullopt,
                     history?history->published_clock:std::nullopt,colour(cgram,22),caption_ink);
    for(int rider=1;rider>=0;--rider) {
        const auto& source=rider_source.movement.riders[static_cast<std::size_t>(rider)];
        const auto oam=project_rider_oam(source.motion.x,source.motion.y,rider_source.race.camera.x,
                                         rider_source.race.camera.y,source.pose.reflected,geometry);
        if(!oam.visible)continue;
        const auto overlay=history?history->overlays.pose[static_cast<std::size_t>(rider)]:std::nullopt;
        const auto pixels=compose_rider_object(content.riders,source.pose.pose_index,overlay,oam.clip);
        const unsigned object_palette=128U+(rider?4U:3U)*16U;
        draw_rider_object(pixels,oam,[&](int x,int y,std::uint8_t value) {
            const auto at=static_cast<std::size_t>(y)*256+static_cast<std::size_t>(x);
            if(bg1_above_objects[at])return;
            const auto index=static_cast<std::uint8_t>(object_palette+value);
            if(!caption_ink.test(at)) {pixel(frame,x,y,colour(cgram,index));return;}
            const auto word=colour_word(cgram,index);
            const auto added=static_cast<std::uint16_t>(std::min<unsigned>(31U,(word&31U)+13U));
            pixel(frame,x,y,{channel8(added),channel8(static_cast<std::uint16_t>((word>>5U)&31U)),
                             channel8(static_cast<std::uint16_t>((word>>10U)&31U))});
        });
    }
    // Every member covers both objects as well as the backgrounds: inside the
    // window the original shows the flat window colour and nothing else
    // (ZOOM-ZOO-WINDOW-EFFECTS: start-line frames 1450, 1583 and 1649 of the
    // M4-16 primary and countdown-pause originals with a rider under the
    // countdown sign and the GO letters, and the opponent-won banner over the
    // riding player on 6724-6800 of the countdown pause). R-0040's "0-6
    // before the riders" came from DRAGSTER frames with no rider under those
    // members; on its release-3213 race the opponent sits under the digits on
    // 57 frames, all of which this order matches.
    if(window_index)
        render_window_xor(frame,dragster_window_table(content.window_tables,*window_index),window_colour);
    if(state.pause.selection)draw_race_pause_menu(frame,state.pause.selection,ui({15,30,30}),ink);
    return frame;
}

void draw_race_pause_menu(RgbFrame& frame,std::uint16_t selection,std::array<std::uint8_t,3> panel,
                          std::array<std::uint8_t,3> ink) {
    for(auto& channel:frame.pixels)channel=static_cast<std::uint8_t>(channel/2U);
    rect(frame,55,74,146,74,panel);
    ui_text(frame,109,83,"PAUSED",ink);
    ui_text(frame,73,101,selection==1?"> RESUME":"  RESUME",ink);
    ui_text(frame,73,115,selection==0xffffU?"> RESTART RACE":"  RESTART RACE",ink);
    ui_text(frame,68,135,"UP DOWN - ENTER",ink);
}

PresentationContent dragster_presentation_content(const ClassicContentPack& pack) {
    return {pack.entry("physics.track.dragster.data"),
            pack.entry("presentation.track.dragster.bg1-tiles.v1"),
            pack.entry("presentation.track.dragster.bg2-tiles.v1"),
            pack.entry("presentation.track.dragster.bg2-map.v1"),
            pack.entry("presentation.classic.palette.v1"),
            pack.entry("presentation.classic.font.v1"),
            pack.entry("presentation.rider.mike.race-tiles.v1"),
            pack.entry("presentation.result.classic.font-layout.v1"),
            pack.entry("presentation.effect.go-window.v1"),
            pack.entry("presentation.effect.winner-window.v1"),
            pack.entry("presentation.result.classic.base-vram.v1"),
            pack.entry("presentation.result.classic.palette.v1"),
            pack.entry("presentation.result.classic.palette-tail.v1"),
            pack.optional_entry("presentation.zoom.race-palette-cycle.v1"),
            pack.optional_entry("presentation.effect.classic.window-tables.v1")};
}

} // namespace unirally
