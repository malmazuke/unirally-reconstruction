#include "race_hud.hpp"

#include "picture.hpp"
#include "presentation.hpp"
#include "zoom_zoo_movement.hpp"
#include <algorithm>
#include <array>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

// The race HUD: the lap and clock fields, their text queue, and the captions.
namespace unirally {

unsigned classic_hud_lap(unsigned laps_remaining, unsigned laps) {
    return std::min(laps, laps + 1U - std::min(laps + 1U, laps_remaining));
}

// R-0043: the finish time as the original's own two fields spell it, colons
// between all three parts: `1:38:02` for 9,802 centiseconds.
std::string hud_time(unsigned centiseconds) {
    const auto digit = [](unsigned v) { return static_cast<char>('0' + v % 10); };
    return {digit(centiseconds / 6000), ':', digit(centiseconds / 1000 % 6),
            digit(centiseconds / 100),  ':', digit(centiseconds / 10),
            digit(centiseconds)};
}

// $81:EB8E, and the race setup that wrote the field before it.
ClassicHudField classic_hud_left_field(const ZoomZooState& published,
                                       const ClassicRaceScenario& scenario) {
    const auto& race = published.race;
    // At the 10:00 limit ($81:C73E-C75B) the player is finished with laps left,
    // so `$0EFB` is not zero and the field keeps the lap (stop-timeout original
    // frames 31578-31920 all read `2/3`).
    if (race.riders[0].finished && race.riders[0].laps_remaining == 0) return {"finish", 1};
    // The word wins over the lap count, so a DRAGSTER finish replaces `race`
    // too, and $053F only suppresses the lap number.
    if (!scenario.tour_race) return {"race", 2};
    // $81:EC61-$81:ECBC puts the lap's ones digit at column 2 and its tens at
    // column 1, and $81:D68C-$81:D6E5 the total from column 4, so the count is
    // right-aligned on column 2 and the total left-aligned on 4. Neither track
    // of this product reaches a second digit in either.
    const auto lap = classic_hud_lap(race.riders[0].laps_remaining, scenario.laps);
    return {std::to_string(lap) + "/" + std::to_string(scenario.laps), lap >= 10U ? 1U : 2U};
}

std::string classic_hud_clock(const RaceTimerDigits& t) {
    return {
        static_cast<char>('0' + t.minutes % 10), ':', static_cast<char>('0' + t.tens_seconds % 10),
        static_cast<char>('0' + t.seconds % 10), ':', static_cast<char>('0' + t.tenths % 10)};
}

namespace {

// The original looks each value up in the character table $80:81F4, whose
// first ten entries are the digits; a value past nine would name a letter,
// which no measured race reaches, so it is refused rather than invented.
char classic_hud_digit(unsigned value) {
    if (value > 9) throw std::invalid_argument("Classic HUD digit is outside its supported domain");
    return static_cast<char>('0' + value);
}

} // namespace

// $81:CAAD-CB10: minutes, tens, seconds, tenths and hundredths of the crossing.
std::string classic_hud_crossing_text(const std::array<std::uint16_t, 5>& d) {
    return {classic_hud_digit(d[0]), ':', classic_hud_digit(d[1]),
            classic_hud_digit(d[2]), ':', classic_hud_digit(d[3]),
            classic_hud_digit(d[4])};
}

std::array<std::uint8_t, 4> classic_hud_clock_digits(const RaceTimerDigits& t) {
    return {static_cast<std::uint8_t>(t.minutes), static_cast<std::uint8_t>(t.tens_seconds),
            static_cast<std::uint8_t>(t.seconds), static_cast<std::uint8_t>(t.tenths)};
}

// $81:C94F-CA36, digit by digit from the tenths up, each borrow carried into
// the stored digit above it ($02, $01, $00 in the original's scratch).
std::string classic_hud_split_text(const std::array<std::uint8_t, 4>& clock,
                                   const std::array<std::uint8_t, 4>& stored) {
    int stored_minutes = stored[0], stored_tens = stored[1], stored_seconds = stored[2];
    int tenths = int(clock[3]) - int(stored[3]);
    if (tenths < 0) {
        tenths += 10;
        ++stored_seconds;
    }
    int seconds = int(clock[2]) - stored_seconds;
    if (seconds < 0) {
        seconds += 10;
        ++stored_tens;
    }
    int tens = int(clock[1]) - stored_tens;
    if (tens < 0) {
        tens += 6;
        ++stored_minutes;
    }
    int minutes = int(clock[0]) - stored_minutes;
    const bool negative = minutes < 0;
    // `EOR #$FF` on the 8-bit minute: the one's complement, not the negation.
    if (negative) minutes = (~minutes) & 0xff;
    if (!negative)
        return {'+',
                classic_hud_digit(unsigned(minutes)),
                ':',
                classic_hud_digit(unsigned(tens)),
                classic_hud_digit(unsigned(seconds)),
                ':',
                classic_hud_digit(unsigned(tenths))};
    // $81:C9F4-CA33: the lower digits are taken from 5, 9 and 10, and ten
    // shows as zero without a carry, so a whole-second deficit reads one
    // second short. No measured race reaches this path (R-0044).
    const int ten_tenths = 10 - tenths;
    return {'-',
            classic_hud_digit(unsigned(minutes)),
            ':',
            classic_hud_digit(unsigned(5 - tens)),
            classic_hud_digit(unsigned(9 - seconds)),
            ':',
            classic_hud_digit(unsigned(ten_tenths == 10 ? 0 : ten_tenths))};
}

// One update of the HUD's text queue: record what the update asks for, then service one
// field, as the original's queue does one a frame.
void ClassicRaceHudClock::observe_update(const ZoomZooState& previous,
                                         const ZoomZooState& updated) {
    on_screen_ = latest_;
    request_fields(previous, updated);
    service_one_field(updated);
}

// What this update asks the queue for. `$81:818D` is the only instruction in the ROM that
// sets `$0D17`, inside the lap-counter decrement, with the rider taken from `$0FF9` - so either
// rider's crossing dirties the left field. The player's last crossing also blanks the clock
// and arms its own finish time; the opponent's finish arms the opponent's. The centred
// fields (R-0044): `$81:C910` reads the countdown after the lap routine set it and before the
// clock ticks, so the clock a split is taken from is the previous update's; the crossing
// digits are the lap routine's own copy. Native's engine keeps the countdown, the
// checkpoint, the crossing digits and the shared first-seen flags; the first rider's stored
// clock is kept here.
void ClassicRaceHudClock::request_fields(const ZoomZooState& previous,
                                         const ZoomZooState& updated) {
    const auto stepped = [&](std::size_t rider) {
        return previous.race.riders[rider].laps_remaining
            != updated.race.riders[rider].laps_remaining;
    };
    if (stepped(0) || stepped(1)) pending_.left = true;
    if (previous.race.riders[0].laps_remaining != 0 && updated.race.riders[0].laps_remaining == 0)
        pending_.clock_blank = true;
    for (std::size_t rider = 0; rider < 2; ++rider) {
        const auto& before = previous.race.riders[rider];
        const auto& after = updated.race.riders[rider];
        auto& cell = pending_.cells[rider];
        // Every accepted crossing but the initial one steps the rider's next checkpoint and sets
        // the countdown, to 120 or, for the opponent first through a slot, straight on to 2
        // within the same update ($81:CA6E), so the countdown alone cannot name that crossing.
        const bool crossing = after.next_checkpoint != before.next_checkpoint
                           && after.checkpoint_display_countdown != 0;
        if (crossing) {
            if (after.checkpoint == 0)
                cell = {ClassicHudCellRequest::Kind::Draw,
                        classic_hud_crossing_text(after.time_digits)};
            else
                cell = crossing_cell(previous, rider, updated);
        }
        // $81:C91A-C922, and the opponent's first-seen cut to 2 at $81:CA6E.
        if (after.checkpoint_display_countdown == 2)
            cell = {ClassicHudCellRequest::Kind::Blank, {}};
    }
}

// A checkpoint crossing's cells: the first rider through a slot stores the clock and draws
// nothing ($81:CA38-CA61); the second draws the split against it. $81:CB13 runs the player
// before the opponent within one update, so a slot the player has just stored is seen by the
// opponent's crossing of the same update. The opponent's writer ($81:F1B5-F1C6) takes its
// first cell from the constant $80:8220, the minus glyph, and never reads the sign byte
// `$11BD`; the player's ($81:EF5E-EF6E) reads `$11BB`: measured on every opponent split of
// the M4-16 primary (14 pixels a frame until this was applied).
ClassicHudCellRequest ClassicRaceHudClock::crossing_cell(const ZoomZooState& previous,
                                                         std::size_t rider,
                                                         const ZoomZooState& updated) {
    const auto& after = updated.race.riders[rider];
    const std::size_t slot = static_cast<std::size_t>(after.laps_remaining) * 4U + after.checkpoint;
    const auto clock = classic_hud_clock_digits(previous.movement.timer);
    const bool stored = slot < slot_times_.size() && slot_times_[slot];
    const bool first = slot < previous.race.checkpoint_seen.size()
                    && (previous.race.checkpoint_seen[slot] & 0x80U) != 0 && !stored;
    if (first) {
        if (slot < slot_times_.size()) slot_times_[slot] = clock;
        return {};
    }
    if (!stored) return {}; // no history of the slot: leave the cells
    ClassicHudCellRequest cell{ClassicHudCellRequest::Kind::Draw,
                               classic_hud_split_text(clock, *slot_times_[slot])};
    if (rider == 1) cell.text[0] = '-';
    return cell;
}

// Service the first pending field and stop, as $81:F357 does: the left field, the clock's
// blanking, the clock's digits, then each rider's cells.
void ClassicRaceHudClock::service_one_field(const ZoomZooState& updated) {
    const bool finished = updated.race.riders[0].laps_remaining == 0;
    if (pending_.left) {
        // `$81:EB91` takes `finish` once the laps are gone and `$81:EB98` jumps to the
        // lap-number writer on a tour race; both paths end at `$81:ECBC`, which clears the flag
        // and returns through `$81:F357`. On a sprint mid-race neither runs: `$81:EB93` reads
        // `$053F` and branches to `$81:EB9B`, whose `JMP` enters the clock handler and leaves
        // `$0D17` set for the rest of the race.
        if (finished || classic_race_scenario(updated.track).tour_race) {
            pending_.left = false;
            return;
        }
    }
    if (pending_.clock_blank) {
        latest_.clock_blanked = true;
        pending_.clock_blank = false;
        return;
    }
    // `$034D` is positive while the digits the timer keeps differ from the ones the cells
    // hold; the handler writes them and returns. Once blanked they are never rewritten,
    // because the race clock has stopped.
    if (!latest_.clock_blanked) {
        auto digits = classic_hud_clock(updated.movement.timer);
        if (latest_.clock != digits) {
            latest_.clock = std::move(digits);
            return;
        }
    }
    for (std::size_t rider = 0; rider < 2; ++rider) {
        auto& cell = pending_.cells[rider];
        auto& held = rider == 0 ? latest_.player_cells : latest_.opponent_cells;
        if (cell.kind == ClassicHudCellRequest::Kind::Draw) {
            held = cell.text;
            cell = {};
            return;
        }
        if (cell.kind == ClassicHudCellRequest::Kind::Blank) {
            cell = {};
            // A finished rider's cells are never blanked, and the handler goes on to the next
            // field without spending the update.
            if (!updated.race.riders[rider].finished) {
                held.reset();
                return;
            }
        }
    }
}

ClassicHudText classic_race_hud_text(const ZoomZooState& previous_update,
                                     const ClassicRaceScenario& scenario,
                                     std::optional<std::uint32_t> opponent_finish_frame,
                                     const std::optional<ClassicHudPublished>& published) {
    const auto& race = previous_update.race;
    ClassicHudText hud;
    const bool finished = race.riders[0].finished && race.riders[0].laps_remaining == 0;
    const auto left = classic_hud_left_field(previous_update, scenario);
    hud.left = left.text;
    hud.left_column = left.column;
    // The corner clock shows tenths, one digit per field. Every gate below is a
    // *picture* number, not an update number: picture N is drawn from the state
    // after update N-1, so a field the queue writes on update F shows in
    // picture F and the state this function reads then has `finish_delay ==
    // F - finish - 1`. The finish is update `finish`, `$81:ECCF` blanks the
    // clock on `finish + 2`, and that picture reads delay 1 (review B1:
    // measured against consecutive originals 6480-6500 of the M4-16 primary
    // and 6465-6500 of compound-reverse, which the kept 20-frame pictures
    // stepped straight over).
    // With the queue followed, every one of these is what it actually wrote;
    // the offsets below are the fallback, and are the queue's own answer only
    // while nothing competes for it.
    if (published) {
        if (!published->clock_blanked)
            hud.clock = published->clock ? *published->clock
                                         : classic_hud_clock(previous_update.movement.timer);
        hud.player_cells = published->player_cells.value_or("");
        hud.opponent_cells = published->opponent_cells.value_or("");
        return hud;
    }
    if (!finished || race.finish_delay < 1)
        hud.clock = classic_hud_clock(previous_update.movement.timer);
    // The player's own time is written on `finish + 3`, which reads delay 2,
    // and the opponent's two updates after the opponent finishes, which is the
    // picture one update after that frame.
    // The 60000 no-time sentinel is not a finish time on either side. A player
    // who finishes all laps always has a real total, so this guard is belt and
    // braces rather than a measured case (review 2 A8).
    if (finished && race.finish_delay >= 2 && race.total_times[0] < 60000U)
        hud.player_cells = hud_time(race.total_times[0]);
    if (race.riders[1].finished && race.total_times[1] < 60000U) {
        const auto opponent = opponent_finish_frame
                                ? opponent_finish_frame
                                : classic_opponent_finish_frame(previous_update);
        if (opponent && previous_update.movement.frame >= *opponent + 1U)
            hud.opponent_cells = hud_time(race.total_times[1]);
    }
    return hud;
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
    if (glyph == ' ') return std::nullopt;
    if (glyph >= 'a' && glyph <= 'z') {
        const unsigned n = static_cast<unsigned>(glyph - 'a');
        if (n < 5) return 0x0bU + n;
        if (n < 21) return 0x20U + n - 5U;
        return 0x40U + n - 21U;
    }
    // R-0043: the HUD's characters, from the same sheet. The original's
    // character table $80:81F4 runs 0-9 then a-z then the punctuation, and the
    // sheet's own order is 16 glyph tops followed by their 16 bottoms, which is
    // why the digits sit below `a` and `:` and `/` sit above `z`.
    if (glyph >= '0' && glyph <= '9')
        return glyph == '0' ? 0x0aU : static_cast<unsigned>(glyph - '0');
    switch (glyph) {
    case '!': return 0x60U;
    case '"': return 0x61U; // the table spells an apostrophe this way
    case '+': return 0x4cU; // R-0044: the split's sign, $80:821F
    case '-': return 0x4dU;
    case ':': return 0x45U;
    case '/': return 0x4eU;
    default: break;
    }
    throw std::invalid_argument("unsupported Classic caption glyph");
}

// The caption is cleared by the queue, not by a timer: a hint sentence ends by
// publishing an entry of sixteen spaces, and $81:BEA8-BEF1 blanks the display
// when the queue runs dry, which the engine carries as `empty_display`.
std::optional<std::span<const std::uint8_t>>
classic_caption_entry(const ZoomZooState& published, std::span<const std::uint8_t> captions) {
    if (captions.size() != 4080) return std::nullopt;
    // R-0052: on the HUNTER tour the row is carried, since a front-of-queue
    // announcement overwrites the slot it was drawn from, and a dry queue
    // shows the HUD message buffer there.
    if (classic_race_scenario(published.track).hunter_tour) {
        const unsigned row = published.hunter.caption;
        if (!row) return std::nullopt;
        return captions.subspan((row - 1U) * 16U, 16U);
    }
    const auto& announcements = published.player_announcements;
    if (announcements.empty_display) return std::nullopt;
    // `movement.rewards` is the opponent's queue ($0D11/$0D13 cursors); the
    // player's, the one the captions follow, is the announcements' own.
    const auto& queue = announcements.queue;
    const unsigned event = queue.entries[queue.read_cursor];
    if (event == 0) return std::nullopt;
    return captions.subspan((event - 1U) * 16U, 16U);
}

namespace {

// One BG3 text cell of the race screen. A glyph is eight pixels wide and
// sixteen tall, drawn as the tile the character names and the tile $10 above
// it, so a field at tilemap row r covers rows r and r+1. BG3 scrolls by one
// line, which is why the caption's row 10 shows at y 79 and the HUD's row 2
// at y 15 (R-0042, R-0043).
void draw_bg3_text(RgbFrame& frame, std::span<const std::uint8_t> font, unsigned column,
                   unsigned row, std::string_view text, std::array<std::uint8_t, 3> ink,
                   std::bitset<256 * 224>& inked) {
    for (std::size_t index = 0; index < text.size(); ++index) {
        const auto tile = classic_caption_tile(text[index]);
        if (!tile) continue;
        for (unsigned half = 0; half < 2; ++half) {
            const auto at = static_cast<std::size_t>(*tile + half * 0x10U) * 16U;
            for (unsigned line = 0; line < 8; ++line) {
                const unsigned low = font[at + 2U * line], high = font[at + 2U * line + 1U];
                for (unsigned bit = 0; bit < 8; ++bit) {
                    if (((low >> (7U - bit)) & 1U) | ((high >> (7U - bit)) & 1U)) {
                        const int x = int(column + index) * 8 + int(bit),
                                  y = int(row) * 8 - 1 + int(half) * 8 + int(line);
                        if (x < 0 || x >= 256 || y < 0 || y >= 224) continue;
                        pixel(frame, x, y, ink);
                        inked.set(static_cast<std::size_t>(y) * 256 + static_cast<std::size_t>(x));
                    }
                }
            }
        }
    }
}

} // namespace

void draw_classic_caption(RgbFrame& frame, const ZoomZooState& published,
                          const ClassicRacePresentationContent& content,
                          std::array<std::uint8_t, 3> ink, std::bitset<256 * 224>& inked) {
    const auto font = content.caption_font;
    if (font.size() != 2048) return;
    const auto selected = classic_caption_entry(published, content.captions);
    if (!selected) return;
    const auto entry = *selected;
    // $81:F322/$81:F33C write sixteen characters to columns 8-23 of rows 10-11.
    std::string line;
    for (unsigned column = 0; column < 16; ++column)
        line.push_back(static_cast<char>(entry[column]));
    draw_bg3_text(frame, font, 8, 10, line, ink, inked);
}

// R-0043: the HUD's four fields, on the same layer, in the same font and the
// same colour as the caption. The original's own tilemap rows and columns:
// the left field and the clock on rows 2-3, the player's finish time on rows
// 5-6 and the opponent's on rows 20-21, both from column 13.
void draw_classic_hud(RgbFrame& frame, const ZoomZooState& state,
                      const ClassicRacePresentationContent& content,
                      std::optional<std::uint32_t> opponent_finish_frame,
                      const std::optional<ClassicHudPublished>& published,
                      std::array<std::uint8_t, 3> ink, std::bitset<256 * 224>& inked) {
    const auto font = content.caption_font;
    if (font.size() != 2048) return;
    const auto hud =
        classic_race_hud_text(state, content.scenario, opponent_finish_frame, published);
    draw_bg3_text(frame, font, hud.left_column, 2, hud.left, ink, inked);
    draw_bg3_text(frame, font, 24, 2, hud.clock, ink, inked);
    draw_bg3_text(frame, font, 13, 5, hud.player_cells, ink, inked);
    draw_bg3_text(frame, font, 13, 20, hud.opponent_cells, ink, inked);
}

} // namespace unirally
