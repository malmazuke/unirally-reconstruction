#include "result_screen.hpp"

#include "picture.hpp"
#include "presentation.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>

// The result screen: its title, times, text and backgrounds.
namespace unirally {

// Title glyphs are 2 x 2 tiles of the result font. The letters of "dragster"
// and "complete" were observed (a 14, c 18, d 1A, e 1C, g 20, l 2A, m 2C,
// o 00, p 30, r 34, s 36, t 38); they fit one layout, which gives the other
// letters and the digits (TRACK-BREADTH): digits 0-9 at 2 x digit, so "o"
// shares the zero; a-n from $14 in steps of two; p-z two tiles lower, since
// "o" has no glyph of its own. f, u and n are confirmed on FLAT FUN's original
// result; the other letters outside the observed twelve are the layout's
// reading.
std::uint16_t result_title_tile(char glyph) {
    if (glyph >= '0' && glyph <= '9') return static_cast<std::uint16_t>((glyph - '0') * 2);
    if (glyph == 'o') return 0x00;
    if (glyph >= 'a' && glyph <= 'n') return static_cast<std::uint16_t>(0x14 + (glyph - 'a') * 2);
    if (glyph >= 'p' && glyph <= 'z')
        return static_cast<std::uint16_t>(0x14 + (glyph - 'a') * 2 - 2);
    throw std::invalid_argument("unsupported Classic result title glyph");
}

namespace {

void write_result_title(std::array<std::uint8_t, 65536>& vram, int x, int y,
                        std::string_view text) {
    for (const char glyph : text) {
        // A name's underscore is its space, one tile wide (FLAT FUN's original
        // result, part of the review of TRACK-BREADTH's result title).
        if (glyph == '_') {
            x += 1;
            continue;
        }
        const auto tile = result_title_tile(glyph);
        set_map_word(vram, x, y, static_cast<std::uint16_t>(0x3c00U | tile));
        set_map_word(vram, x + 1, y, static_cast<std::uint16_t>(0x3c00U | (tile + 1U)));
        set_map_word(vram, x, y + 1, static_cast<std::uint16_t>(0x3c00U | (tile + 0x50U)));
        set_map_word(vram, x + 1, y + 1, static_cast<std::uint16_t>(0x3c00U | (tile + 0x51U)));
        x += 2;
    }
}

std::uint16_t result_text_tile(char glyph) {
    switch (glyph) {
    case ' ': return 0xce;
    case '.': return 0xa8;
    case ':': return 0xcc;
    // The small result font is contiguous from '.' at $A8: digits 0-9 are
    // $A9-$B2 and letters follow from 'A' at $B3 without 'O', which reuses
    // '0'. Ordinary finish times reach every digit (R-0038 frame 3860 shows 9).
    case '0': return 0xa9;
    case '1': return 0xaa;
    case '2': return 0xab;
    case '3': return 0xac;
    case '4': return 0xad;
    case '5': return 0xae;
    case '6': return 0xaf;
    case '7': return 0xb0;
    case '8': return 0xb1;
    case '9': return 0xb2;
    case 'A': return 0xb3;
    case 'E': return 0xb7;
    case 'I': return 0xbb;
    case 'K': return 0xbd;
    case 'L': return 0xbe;
    case 'M': return 0xbf;
    case 'N': return 0xc0;
    case 'O': return 0xa9;
    case 'P': return 0xc1;
    case 'R': return 0xc3;
    case 'S': return 0xc4;
    case 'T': return 0xc5;
    case 'Y': return 0xca;
    default: throw std::invalid_argument("unsupported Classic result text glyph");
    }
}

void write_result_text(std::array<std::uint8_t, 65536>& vram, int x, int y, std::string_view text) {
    for (const char glyph : text) {
        const auto tile = result_text_tile(glyph);
        set_map_word(vram, x, y, static_cast<std::uint16_t>(0x3c00U | tile));
        set_map_word(vram, x, y + 1, static_cast<std::uint16_t>(0x3c00U | (tile + 0x3cU)));
        ++x;
    }
}

// The title is the track's name, 0xFF-terminated lowercase ASCII from the name table
// (`$83:9FFA`, track order). The DRAGSTER assets carry the table's first name, DRAGSTER's own;
// another one-run track passes its own (TRACK-BREADTH, found in the user's live play of EAST
// and LOOPER).
std::string_view result_title(std::span<const std::uint8_t> result_assets,
                              std::span<const std::uint8_t> track_name) {
    const auto title_seed = track_name.empty() ? result_assets.subspan(5208, 16) : track_name;
    const auto terminator = std::find(title_seed.begin(), title_seed.end(), std::uint8_t{0xff});
    if (terminator == title_seed.end())
        throw std::invalid_argument("Classic result title seed lacks terminator");
    return {reinterpret_cast<const char*>(title_seed.data()),
            static_cast<std::size_t>(terminator - title_seed.begin())};
}

bool time_is_consistent(const std::array<std::uint16_t, 5>& digits, std::uint16_t centiseconds) {
    if (digits[0] > 9 || digits[1] > 5 || digits[2] > 9 || digits[3] > 9 || digits[4] > 9)
        return false;
    const auto displayed = static_cast<unsigned>(digits[0]) * 6000U
                         + static_cast<unsigned>(digits[1]) * 1000U
                         + static_cast<unsigned>(digits[2]) * 100U
                         + static_cast<unsigned>(digits[3]) * 10U + digits[4];
    return displayed == centiseconds;
}

// Only the result compositions the original was observed to publish are drawn: the winner's
// at loading 225 or 226, the loser's at 242, with both riders' times consistent and the
// outcome matching them. Returns whether the player's row reads NO TIME: $81:C73E-C75B ends
// the race at 10:00 with both riders finished and the lap-short player's total left at the
// 60000 no-time sentinel, and holds 9:59.9 when it does, so a no-time player total belongs
// only to that timed-out race (R-0039; clock-limit original, stable result 32016-32200).
// Equal times mean both riders crossed on one update; the player's crossing is processed
// first, so the race counts it as won (R-0038 tie original).
bool check_result_composition(const RaceFinishState& finish, const RaceTimerDigits& clock) {
    const bool observed_winner_publication =
        finish.outcome == RaceOutcome::PlayerWon
        && ((finish.phase == RacePhase::ResultLoading && finish.result_loading_updates == 225)
            || (finish.phase == RacePhase::ResultScreen && finish.result_loading_updates == 226));
    const bool observed_loser_publication = finish.outcome == RaceOutcome::PlayerLost
                                         && finish.phase == RacePhase::ResultScreen
                                         && finish.result_loading_updates == 242;
    const bool clock_expired =
        clock.minutes == 9 && clock.tens_seconds == 5 && clock.seconds == 9 && clock.tenths == 9;
    const bool player_has_no_time = finish.finish_time_centiseconds[0] >= 60000 && clock_expired;
    const bool times_are_consistent =
        finish.rider_finished[0] && finish.rider_finished[1]
        && (player_has_no_time
            || time_is_consistent(finish.finish_time_digits[0], finish.finish_time_centiseconds[0]))
        && time_is_consistent(finish.finish_time_digits[1], finish.finish_time_centiseconds[1]);
    const bool outcome_is_consistent =
        (finish.outcome == RaceOutcome::PlayerWon
         && finish.finish_time_centiseconds[0] <= finish.finish_time_centiseconds[1])
        || (finish.outcome == RaceOutcome::PlayerLost
            && finish.finish_time_centiseconds[0] > finish.finish_time_centiseconds[1]);
    if ((!observed_winner_publication && !observed_loser_publication) || !times_are_consistent
        || !outcome_is_consistent)
        throw std::invalid_argument("unsupported Classic result composition");
    return player_has_no_time;
}

} // namespace

// $80:C431 first fills the map, then writes these fields in this order. Each small-font glyph
// is a vertical tile pair; title glyphs are two-by-two. The result state carries the observed
// five timer digits. The title is centred on the 32-tile row by its width in tiles, two per
// letter and one per space: DRAGSTER starts at tile 8, EAST at 12 and FLAT FUN at 9, as in the
// original's results (TRACK-BREADTH); the rule is fitted to those three.
void build_result_map(std::array<std::uint8_t, 65536>& vram, const RaceFinishState& finish,
                      const RaceTimerDigits& clock, std::span<const std::uint8_t> result_assets,
                      std::span<const std::uint8_t> track_name) {
    for (std::size_t entry = 0; entry < 1024; ++entry)
        set_map_word(vram, static_cast<int>(entry % 32), static_cast<int>(entry / 32), 0x004c);
    const auto title = result_title(result_assets, track_name);
    const bool player_has_no_time = check_result_composition(finish, clock);
    const auto spaces = static_cast<int>(std::count(title.begin(), title.end(), '_'));
    const int width = 2 * (static_cast<int>(title.size()) - spaces) + spaces;
    if (width > 32) throw std::invalid_argument("Classic result title is wider than the screen");
    write_result_title(vram, 16 - width / 2, 2, title);
    write_result_title(vram, 8, 5, "complete");
    write_result_text(vram, 7, 8, "PLAYER     TIME");
    const auto& digits = finish.finish_time_digits[0];
    std::array<char, 8> time{
        {' ', static_cast<char>('0' + digits[0]), ':', static_cast<char>('0' + digits[1]),
         static_cast<char>('0' + digits[2]), '.', static_cast<char>('0' + digits[3]),
         static_cast<char>('0' + digits[4])}};
    write_result_text(vram, 7, 11, "MIKE    ");
    write_result_text(vram, 17, 11,
                      player_has_no_time ? std::string_view(" NO TIME")
                                         : std::string_view(time.data(), time.size()));
    for (const int row : {14, 17, 20}) {
        write_result_text(vram, 7, row, "SOMEONE ");
        write_result_text(vram, 17, row, " NO TIME");
    }
}

namespace {

struct ResultBackgroundPixel {
    std::uint8_t palette_index{};
    std::uint8_t palette_group{};
    std::uint8_t priority{};
    bool direct_colour{};
};

ResultBackgroundPixel result_bg1_pixel(const std::array<std::uint8_t, 65536>& vram, int x, int y) {
    constexpr int vertical_scroll = 82;
    const int py = (y + vertical_scroll + 1) & 511;
    const int map_screen = py >= 256 ? 1 : 0;
    const int map_y = (py / 8) & 31;
    const int map_x = x / 8;
    const auto map_at = static_cast<std::size_t>(map_screen * 0x800 + (map_y * 32 + map_x) * 2);
    const auto entry =
        static_cast<std::uint16_t>(vram[map_at] | (static_cast<unsigned>(vram[map_at + 1]) << 8U));
    int tile_x = x & 7, tile_y = py & 7;
    if (entry & 0x4000U) tile_x = 7 - tile_x;
    if (entry & 0x8000U) tile_y = 7 - tile_y;
    const auto value = tile_pixel_8bpp(vram, 0x6000, entry & 0x3ffU, tile_x, tile_y);
    const auto priority = static_cast<std::uint8_t>(value == 0 ? 0 : (entry & 0x2000U ? 7 : 3));
    const auto palette_group = static_cast<std::uint8_t>((entry >> 10U) & 7U);
    // CGWSEL=$02 selects subscreen blending (bit 1). Direct colour is bit 0 and
    // is disabled in the captured result state, so BG1 still indexes CGRAM.
    return {value, palette_group, priority, false};
}

ResultBackgroundPixel result_bg2_pixel(const std::array<std::uint8_t, 65536>& vram, int x, int y) {
    const int py = (y + 1) & 511;
    const int map_screen = py >= 256 ? 2 : 0;
    const int map_y = (py / 8) & 31;
    const int map_x = x / 8;
    const auto map_at =
        static_cast<std::size_t>(0x2000 + map_screen * 0x800 + (map_y * 32 + map_x) * 2);
    const auto entry =
        static_cast<std::uint16_t>(vram[map_at] | (static_cast<unsigned>(vram[map_at + 1]) << 8U));
    int tile_x = x & 7, tile_y = py & 7;
    if (entry & 0x4000U) tile_x = 7 - tile_x;
    if (entry & 0x8000U) tile_y = 7 - tile_y;
    const auto value = tile_pixel(vram, 0x4000, entry & 0x3ffU, tile_x, tile_y);
    const auto palette = static_cast<std::uint8_t>(((entry >> 10U) & 7U) * 16U + value);
    const auto priority = static_cast<std::uint8_t>(value == 0 ? 0 : (entry & 0x2000U ? 5 : 1));
    return {palette, 0, priority, false};
}

} // namespace

void render_result_background(RgbFrame& frame, const RaceFinishState& finish,
                              const RaceTimerDigits& clock, const ClassicResultContent& content) {
    // These writes replay the observed $82:B296 copier sequence. Addresses are
    // VRAM byte addresses; the SNES VMADD register observed by the capture uses
    // word addresses. Later writes intentionally replace overlapping content.
    std::array<std::uint8_t, 65536> vram{};
    copy_wrapping(vram, 0xc000, content.result_base_vram.subspan(0, 8192));
    copy_wrapping(vram, 0x0000, content.result_base_vram.subspan(8192, 2816));
    copy_wrapping(vram, 0x4000, content.result_base_vram.subspan(11008, 8960));
    copy_wrapping(vram, 0x6340, content.result_base_vram.subspan(19968, 8000));
    // Frame 3546 resets VMADD to word 0x3D80 before the remaining copier run.
    copy_wrapping(vram, 0x7b00, content.result_base_vram.subspan(27968, 13568));

    // The two result-specific 4bpp payloads retain the current VMADD ordering:
    // word 0x3D80 (byte 0x7B00), then word 0x7A00 (byte 0xF400, wrapping through
    // byte 0x0000).
    copy_wrapping(vram, 0x7b00, content.result_assets.subspan(216, 1920));
    copy_wrapping(vram, 0xf400, content.result_assets.subspan(2136, 3072));

    build_result_map(vram, finish, clock, content.result_assets, content.track_name);

    // The result palettes below replace every colour the pose-keyed race cycle
    // could have set, so the neutral layout is byte-identical here.
    auto cgram = build_race_cgram(content.palette, false);
    std::copy(content.result_palette.begin(), content.result_palette.end(), cgram.begin());
    // The seven-frame palette cycle's stable-frame phase is captured at frame
    // 3678: CGRAM 108..111 receive colours 0x4A52, 0x4631, 0x4210 and
    // 0x56B5.
    constexpr std::array<std::uint8_t, 8> winner_cycle{0x52, 0x4a, 0x31, 0x46,
                                                       0x10, 0x42, 0xb5, 0x56};
    // The release-3000 loss reaches its first complete result on frame 3800.
    // Its last palette-cycle write at frame 3797 is the observed two-word
    // rotation below; this is presentation state and does not alter gameplay.
    constexpr std::array<std::uint8_t, 8> loser_cycle{0x10, 0x42, 0xb5, 0x56,
                                                      0x52, 0x4a, 0x31, 0x46};
    const auto& cycle = finish.outcome == RaceOutcome::PlayerLost ? loser_cycle : winner_cycle;
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
        std::copy_n(content.result_palette_tail.begin()
                        + static_cast<std::ptrdiff_t>(32 + block * 32),
                    32, cgram.begin() + static_cast<std::ptrdiff_t>(416 + block * 32));
    }
    for (int y = 0; y < 224; ++y)
        for (int x = 0; x < 256; ++x) {
            const auto bg1 = result_bg1_pixel(vram, x, y);
            const auto bg2 = result_bg2_pixel(vram, x, y);
            const auto above = bg2.priority > bg1.priority ? bg2 : bg1;
            const auto main_colour =
                above.priority == 0   ? colour_word(cgram, 0)
                : above.direct_colour ? snes_direct_colour(above.palette_index, above.palette_group)
                                      : colour_word(cgram, above.palette_index);
            // TS enables only OBJ. Where the bounded result renderer omits an OBJ,
            // the subscreen is transparent and bsnes disables halve/subscreen blend.
            // The result fade reaches and retains INIDISP brightness 14 at frame
            // 3568; frame 3679 has no later write. bsnes rounds each five-bit
            // channel after multiplying by brightness / 15.
            pixel(frame, x, y, colour_word_rgb(apply_snes_brightness(main_colour, 14)));
        }
}

namespace {

// Small authored UI font. Original scene artwork remains pack content.
std::array<unsigned, 7> ui_glyph(char c) {
    switch (c) {
    case '0': return {14, 17, 19, 21, 25, 17, 14};
    case '1': return {4, 12, 4, 4, 4, 4, 14};
    case '2': return {14, 17, 1, 2, 4, 8, 31};
    case '3': return {30, 1, 1, 14, 1, 1, 30};
    case '4': return {2, 6, 10, 18, 31, 2, 2};
    case '5': return {31, 16, 16, 30, 1, 1, 30};
    case '6': return {14, 16, 16, 30, 17, 17, 14};
    case '7': return {31, 1, 2, 4, 8, 8, 8};
    case '8': return {14, 17, 17, 14, 17, 17, 14};
    case '9': return {14, 17, 17, 15, 1, 1, 14};
    case 'A': return {14, 17, 17, 31, 17, 17, 17};
    case 'B': return {30, 17, 17, 30, 17, 17, 30};
    case 'C': return {14, 17, 16, 16, 16, 17, 14};
    case 'D': return {30, 17, 17, 17, 17, 17, 30};
    case 'E': return {31, 16, 16, 30, 16, 16, 31};
    case 'F': return {31, 16, 16, 30, 16, 16, 16};
    case 'G': return {14, 17, 16, 23, 17, 17, 15};
    case 'H': return {17, 17, 17, 31, 17, 17, 17};
    case 'I': return {14, 4, 4, 4, 4, 4, 14};
    case 'J': return {7, 2, 2, 2, 18, 18, 12};
    case 'K': return {17, 18, 20, 24, 20, 18, 17};
    case 'L': return {16, 16, 16, 16, 16, 16, 31};
    case 'M': return {17, 27, 21, 21, 17, 17, 17};
    case 'N': return {17, 25, 21, 19, 17, 17, 17};
    case 'O': return {14, 17, 17, 17, 17, 17, 14};
    case 'P': return {30, 17, 17, 30, 16, 16, 16};
    case 'Q': return {14, 17, 17, 17, 21, 18, 13};
    case 'R': return {30, 17, 17, 30, 20, 18, 17};
    case 'S': return {15, 16, 16, 14, 1, 1, 30};
    case 'T': return {31, 4, 4, 4, 4, 4, 4};
    case 'U': return {17, 17, 17, 17, 17, 17, 14};
    case 'V': return {17, 17, 17, 17, 17, 10, 4};
    case 'W': return {17, 17, 17, 21, 21, 21, 10};
    case 'X': return {17, 17, 10, 4, 10, 17, 17};
    case 'Y': return {17, 17, 10, 4, 4, 4, 4};
    case 'Z': return {31, 1, 2, 4, 8, 16, 31};
    case ':': return {0, 4, 4, 0, 4, 4, 0};
    case '.': return {0, 0, 0, 0, 0, 4, 4};
    case '/': return {1, 1, 2, 4, 8, 16, 16};
    case '-': return {0, 0, 0, 31, 0, 0, 0};
    // The pause menu's selection marker. Without this glyph the menu still
    // emitted "> RESUME" but drew it identically to "  RESUME", so the focused
    // entry was indistinguishable and ENTER's target was unknowable.
    case '>': return {16, 8, 4, 2, 4, 8, 16};
    // Space must be explicit. It has no glyph of its own, so it used to reach
    // the default and render blank only because the default was blank; once
    // the default became a visible box, every space between words drew one.
    case ' ': return {0, 0, 0, 0, 0, 0, 0};
    // An unmapped character used to render blank, which hides the omission at
    // exactly the moment it matters. Draw a solid block instead: a hollow box
    // differs from 'O' only in its top and bottom rows at 5x7, so it reads as
    // a letter, which is worse than blank. A solid block cannot.
    default: return {31, 31, 31, 31, 31, 31, 31};
    }
}

} // namespace

void ui_text(RgbFrame& frame, int x, int y, std::string_view text,
             std::array<std::uint8_t, 3> ink) {
    for (char c : text) {
        const auto glyph = ui_glyph(c);
        for (int row = 0; row < 7; ++row)
            for (int col = 0; col < 5; ++col)
                if (glyph[static_cast<std::size_t>(row)] & (1U << (4 - col)))
                    pixel(frame, x + col, y + row, ink);
        x += 6;
    }
}

// The result screen writes NO TIME for the 60000 no-time sentinel (stop-timeout
// original result, frames 32000-32100).
std::string result_time(unsigned value);

std::string race_time(unsigned value) {
    const auto digit = [](unsigned v) { return static_cast<char>('0' + v % 10); };
    return {digit(value / 6000), ':',         digit(value / 1000 % 6), digit(value / 100), '.',
            digit(value / 10),   digit(value)};
}

std::string result_time(unsigned value) {
    return value >= 60000U ? "NO TIME" : race_time(value);
}

} // namespace unirally
