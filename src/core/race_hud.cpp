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
// the stored digit above it (the original's direct-page scratch bytes 2, 1 and 0).
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
    // `EOR` with 0xFF on the 8-bit minute: the one's complement, not the negation.
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

namespace {

// The race NMI takes its race path once the fade `$0FF1` has reached 5 ($80:8642-865B).
constexpr unsigned race_nmi_fade = 5;
// A lead under 10 transitions shows one chevron; from 20 the arrow runs from three.
constexpr unsigned short_lead = 10, long_lead = 20;

} // namespace

// $81:E8D2-$81:E8E5 steps `$1257` every race NMI and `$1255` each time it wraps; the length
// is chosen from `$1255` at $81:E9AC-$81:E9DB, the CMPs read as signed.
unsigned classic_arrow_chevrons(std::uint16_t lead, unsigned race_nmis) {
    const unsigned step = (race_nmis >> 3U) & 3U;
    const auto below = [lead](unsigned bound) {
        return (static_cast<std::uint16_t>(lead - bound) & 0x8000U) != 0;
    };
    if (below(short_lead)) return 1;
    // `2 - $1255` goes negative on step 3, and `AND #1`, `INC` turn that into 2.
    if (below(long_lead)) return step < 3 ? 2 - step : 2;
    return 3 - step;
}

// $82:9822 writes the arrow word `$0FD5` on every update, after the riders' movement and before
// their contact, so the track marker it reads is the previous update's. The race mode's stunt
// event (`$77:074B` of 2), which also blanks it, has no native scenario. The finished flag is
// this update's when the player crossed the line (`$81:823B` runs first); the 10:00 time-out
// sets it after the word is written, so there the arrow stays one update longer.
std::optional<ClassicRaceArrow>
classic_race_arrow(const ZoomZooState& previous, const ZoomZooState& updated, unsigned race_nmis) {
    const auto& player = updated.movement.riders[0].progress;
    const auto own = player.transition_count;
    const auto other = updated.movement.riders[1].progress.transition_count;
    const bool behind = (static_cast<std::uint16_t>(own - other) & 0x8000U) != 0;
    if ((own >> 1U) == (other >> 1U) || !behind) return std::nullopt;
    const auto& rider = updated.race.riders[0];
    const bool crossed = rider.finished && rider.laps_remaining == 0;
    if (crossed || previous.race.riders[0].finished) return std::nullopt;
    // $81:E9DE: nothing is drawn after a rejected transition.
    if (player.transition_rejected) return std::nullopt;
    const auto marker = previous.movement.riders[0].progress.marker_word;
    return ClassicRaceArrow{
        static_cast<ClassicRaceArrow::Direction>(marker >> 14U),
        classic_arrow_chevrons(static_cast<std::uint16_t>(other - own), race_nmis)};
}

// One update of the HUD's text layer: the NMI's counter and arrow, then what the update asks
// the queue for and the one field the queue services, as the original's does one a frame.
void ClassicRaceHudClock::observe_update(const ZoomZooState& previous,
                                         const ZoomZooState& updated) {
    on_screen_ = latest_;
    if (updated.fade_level >= race_nmi_fade) ++race_nmis_;
    redraw_arrow(previous, updated);
    request_fields(previous, updated);
    request_caption(previous, updated);
    service_one_field(updated);
}

// $81:E8E8-$81:EB83: after an update whose progress phase is clear the NMI leaves the arrow
// as it is; otherwise it takes down the arrow it drew last and draws the one the update asks
// for. The up arrow's rows 5-6 are skipped while the player's centred cells show (`$0D19`).
void ClassicRaceHudClock::redraw_arrow(const ZoomZooState& previous, const ZoomZooState& updated) {
    if (updated.movement.progress_phase == 0) return;
    latest_.arrow = classic_race_arrow(previous, updated, race_nmis_);
    if (latest_.arrow && latest_.arrow->direction == ClassicRaceArrow::Direction::Up)
        latest_.arrow->middle_rows_covered = latest_.player_cells.has_value();
}

// The update raises `$0EE7` in two places, both in the player's consumer `$81:BEA8`, which runs
// when the queue's cooldown is zero: taking an event (`$81:C057`), which steps the read cursor
// on and writes the event's sixteen characters, and the first look at a dry queue (`$81:BFB4`),
// which sets `$03ED` (native `empty_display`) and writes the HUD message buffer (blank outside
// the HUNTER tour, the effect's name on it) only if `$11C1` says an event was taken since the
// last dry look. The cooldown alone does not tell: when the hints end `$81:C5B9` clears it and
// a consumption in the same update sets a lower one (M4-16 primary 2742: 92 to 40). The NMI
// uploads the text as it stands when the task is reached.
void ClassicRaceHudClock::request_caption(const ZoomZooState& previous,
                                          const ZoomZooState& updated) {
    const auto& before = previous.player_announcements;
    const auto& after = updated.player_announcements;
    const auto slots = static_cast<unsigned>(after.queue.entries.size());
    // On the HUNTER tour a front-of-queue announcement ($81:C55B) steps the cursor back later
    // in the same update; the text it carries (`hunter.caption`) still changes.
    const bool hunter = classic_race_scenario(updated.track).hunter_tour;
    const bool taken =
        after.queue.read_cursor == (before.queue.read_cursor + 1U) % slots
        || (hunter && !after.empty_display && updated.hunter.caption != previous.hunter.caption);
    if (taken) {
        caption_buffer_ =
            hunter ? updated.hunter.caption : after.queue.entries[after.queue.read_cursor];
        consumed_since_blank_ = true;
    } else if (after.empty_display && !before.empty_display && consumed_since_blank_) {
        caption_buffer_ = hunter ? updated.hunter.caption : 0U;
        consumed_since_blank_ = false;
    } else {
        return;
    }
    pending_.caption = true;
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
    const bool first_through = slot < previous.race.checkpoint_seen.size()
                            && slot_not_yet_crossed(previous.race.checkpoint_seen[slot]) && !stored;
    if (first_through) {
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
// blanking, the clock's digits, each rider's cells, then the caption.
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
    // The player's cells, drawn or blanked, overwrite an up arrow's rows 5-6.
    const auto cover_arrow = [this](std::size_t rider) {
        if (rider == 0 && latest_.arrow
            && latest_.arrow->direction == ClassicRaceArrow::Direction::Up)
            latest_.arrow->middle_rows_covered = true;
    };
    for (std::size_t rider = 0; rider < 2; ++rider) {
        auto& cell = pending_.cells[rider];
        auto& held = rider == 0 ? latest_.player_cells : latest_.opponent_cells;
        if (cell.kind == ClassicHudCellRequest::Kind::Draw) {
            held = cell.text;
            cell = {};
            cover_arrow(rider);
            return;
        }
        if (cell.kind == ClassicHudCellRequest::Kind::Blank) {
            cell = {};
            // A finished rider's cells are never blanked, and the handler goes on to the next
            // field without spending the update.
            if (!updated.race.riders[rider].finished) {
                held.reset();
                cover_arrow(rider);
                return;
            }
        }
    }
    // $81:F30C-$81:F34D, the last task: the caption rows take the buffer's sixteen characters.
    if (pending_.caption) {
        latest_.caption_event = caption_buffer_;
        pending_.caption = false;
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
    if (finished && race.finish_delay >= 2 && race.total_times[0] < no_time)
        hud.player_cells = hud_time(race.total_times[0]);
    if (race.riders[1].finished && race.total_times[1] < no_time) {
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
// tile and the tile 0x10 above it.
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
    // R-0052: on the HUNTER tour the row is carried, since a front-of-queue
    // announcement overwrites the slot it was drawn from, and a dry queue
    // shows the HUD message buffer there.
    if (classic_race_scenario(published.track).hunter_tour)
        return classic_caption_text(published.hunter.caption, captions);
    const auto& announcements = published.player_announcements;
    if (announcements.empty_display) return std::nullopt;
    // `movement.rewards` is the opponent's queue ($0D11/$0D13 cursors); the
    // player's, the one the captions follow, is the announcements' own.
    const auto& queue = announcements.queue;
    return classic_caption_text(queue.entries[queue.read_cursor], captions);
}

std::optional<std::span<const std::uint8_t>>
classic_caption_text(unsigned event, std::span<const std::uint8_t> captions) {
    if (captions.size() != 4080 || event == 0) return std::nullopt;
    return captions.subspan((event - 1U) * 16U, 16U);
}

namespace {

// One BG3 tile of the race screen's text layer at tilemap `column` and `row`, in the ink
// wherever the 2bpp tile has a nonzero pixel. BG3 scrolls by one line, which is why the
// caption's row 10 shows at y 79 and the HUD's row 2 at y 15 (R-0042, R-0043).
void draw_bg3_tile(RgbFrame& frame, std::span<const std::uint8_t> font, unsigned column,
                   unsigned row, unsigned tile, std::array<std::uint8_t, 3> ink,
                   std::bitset<256 * 224>& inked) {
    const auto at = static_cast<std::size_t>(tile) * 16U;
    for (unsigned line = 0; line < 8; ++line) {
        const unsigned low = font[at + 2U * line], high = font[at + 2U * line + 1U];
        for (unsigned bit = 0; bit < 8; ++bit) {
            if (!(((low | high) >> (7U - bit)) & 1U)) continue;
            const int x = int(column) * 8 + int(bit), y = int(row) * 8 - 1 + int(line);
            if (x < 0 || x >= 256 || y < 0 || y >= 224) continue;
            pixel(frame, x, y, ink);
            inked.set(static_cast<std::size_t>(y) * 256 + static_cast<std::size_t>(x));
        }
    }
}

// A line of text. A glyph is eight pixels wide and sixteen tall, drawn as the tile the
// character names and the tile 0x10 above it, so a field at tilemap row r covers rows r and
// r+1.
void draw_bg3_text(RgbFrame& frame, std::span<const std::uint8_t> font, unsigned column,
                   unsigned row, std::string_view text, std::array<std::uint8_t, 3> ink,
                   std::bitset<256 * 224>& inked) {
    for (std::size_t index = 0; index < text.size(); ++index) {
        const auto tile = classic_caption_tile(text[index]);
        if (!tile) continue;
        const auto at = column + static_cast<unsigned>(index);
        draw_bg3_tile(frame, font, at, row, *tile, ink, inked);
        draw_bg3_tile(frame, font, at, row + 1U, *tile + 0x10U, ink, inked);
    }
}

// The arrow's chevrons in the font sheet: the left one's two halves, its mirror's, and the
// up and down chevrons, each two tiles side by side.
constexpr unsigned left_chevron = 0x46, right_chevron = 0x47, up_chevron = 0x48,
                   down_chevron = 0x4a, lower_half = 0x10;
constexpr unsigned side_arrow_row = 14, up_arrow_row = 4, down_arrow_row = 24;
constexpr unsigned left_arrow_column = 5, right_arrow_end = 28, vertical_arrow_column = 15;

// $81:E9EE-$81:EB83: the right arrow grows leftward from column 28, the left one rightward
// from column 5, the up one downward from row 4 and the down one from row 24.
void draw_classic_arrow(RgbFrame& frame, std::span<const std::uint8_t> font,
                        const ClassicRaceArrow& arrow, std::array<std::uint8_t, 3> ink,
                        std::bitset<256 * 224>& inked) {
    using Direction = ClassicRaceArrow::Direction;
    for (unsigned chevron = 0; chevron < arrow.chevrons; ++chevron) {
        switch (arrow.direction) {
        case Direction::Right:
        case Direction::Left: {
            const bool right = arrow.direction == Direction::Right;
            const unsigned column = right ? right_arrow_end - chevron : left_arrow_column + chevron;
            const unsigned tile = right ? right_chevron : left_chevron;
            draw_bg3_tile(frame, font, column, side_arrow_row, tile, ink, inked);
            draw_bg3_tile(frame, font, column, side_arrow_row + 1U, tile + lower_half, ink, inked);
            break;
        }
        case Direction::Up:
        case Direction::Down: {
            const bool up = arrow.direction == Direction::Up;
            if (up && chevron > 0 && arrow.middle_rows_covered) break;
            const unsigned row = (up ? up_arrow_row : down_arrow_row) + chevron;
            const unsigned tile = up ? up_chevron : down_chevron;
            draw_bg3_tile(frame, font, vertical_arrow_column, row, tile, ink, inked);
            draw_bg3_tile(frame, font, vertical_arrow_column + 1U, row, tile + 1U, ink, inked);
            break;
        }
        }
    }
}

// Without the NMI's history, the arrow from the update drawn alone. The first race NMI is
// the picture after the update at boundary + 5, so the NMI after the update at frame F has
// counted F - boundary - 4 (exact while every update since the boundary advanced
// `movement.frame`, as paused and skipped ones do). The update's own marker stands in for the
// previous update's. After an update with the progress phase clear the arrow on screen was
// drawn a picture earlier from the update before, which a single state does not have; this
// one stands in for it too.
std::optional<ClassicRaceArrow> classic_arrow_without_history(const ZoomZooState& drawn,
                                                              const ClassicRaceScenario& scenario) {
    const auto frame = drawn.movement.frame, first = scenario.initialization_frame + race_nmi_fade;
    if (drawn.fade_level < race_nmi_fade || frame < first) return std::nullopt;
    const unsigned held = drawn.movement.progress_phase == 0 ? 1U : 0U;
    return classic_race_arrow(drawn, drawn, frame - first + 1U - held);
}

} // namespace

// With the HUD queue's history the caption is what the queue last uploaded.
void draw_classic_caption(RgbFrame& frame, const ZoomZooState& published,
                          const ClassicRacePresentationContent& content,
                          const std::optional<ClassicHudPublished>& hud,
                          std::array<std::uint8_t, 3> ink, std::bitset<256 * 224>& inked) {
    const auto font = content.caption_font;
    if (font.size() != 2048) return;
    const auto selected = hud ? classic_caption_text(hud->caption_event, content.captions)
                              : classic_caption_entry(published, content.captions);
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
    const auto arrow =
        published ? published->arrow : classic_arrow_without_history(state, content.scenario);
    if (arrow) draw_classic_arrow(frame, font, *arrow, ink, inked);
}

} // namespace unirally
