#include "race_hud.hpp"

#include "announcements.hpp"

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
    // R-0068: a stunt event's setup writes `stunt` from column 2 instead ($81:D771-D816, which
    // also sets $053F); its riders never cross the line for good, so `finish` never replaces it.
    if (scenario.stunt_event) return {"stunt", 2};
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

// The digits the clock cells hold for `state`'s timer. A stunt event's clock, once stopped,
// keeps 0:00.0: the tick after it wraps the timer's lower digits to 5, 9 and 9 and clamps the
// minutes ($81:C830), and $81:C86A then skips the writes to the cells `$0E2F-$0E3B`.
RaceTimerDigits classic_hud_timer(const ZoomZooState& state) {
    if (classic_race_scenario(state.track).stunt_event && state.stunt.clock_stopped) return {};
    return state.movement.timer;
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

// The character table $80:81F4 as R-0043 read it: indexes 0-9 are the digits, 10-35 `a`-`z`.
// The punctuation after them is not a digit any score reaches, so it is refused.
constexpr unsigned character_table_digits = 10, character_table_letters = 26;
char hud_character(unsigned index) {
    if (index < character_table_digits) return static_cast<char>('0' + index);
    if (index < character_table_digits + character_table_letters)
        return static_cast<char>('a' + (index - character_table_digits));
    throw std::invalid_argument("stunt score is outside the character table's digits and letters");
}

struct DecimalCells {
    unsigned hundreds{}, tens{}, units{};
};
// $81:C374-C3A3 and $81:CD8D-CDB5: the hundreds by subtracting 100 until below it, then the
// tens by subtracting 10; the word is read as signed, and a stunt score never reaches 0x8000.
DecimalCells decimal_cells(std::uint16_t value) {
    if (value & 0x8000U) throw std::invalid_argument("stunt score is outside its supported domain");
    return {value / 100U, value / 10U % 10U, value % 10U};
}

} // namespace

// $81:CAAD-CB10: minutes, tens, seconds, tenths and hundredths of the crossing.
std::string classic_hud_crossing_text(const std::array<std::uint16_t, 5>& d) {
    return {classic_hud_digit(d[0]), ':', classic_hud_digit(d[1]),
            classic_hud_digit(d[2]), ':', classic_hud_digit(d[3]),
            classic_hud_digit(d[4])};
}

// $81:F29C-F2FD: the hundreds cell is written only when `$12FB` (the hundreds less one) is not
// negative, the tens cell only when `$12F7` (hundreds plus tens, less one) is not, the units
// always.
std::array<char, 3> stunt_score_cells(std::uint16_t score, std::array<char, 3> held) {
    const auto cells = decimal_cells(score);
    if (cells.hundreds > 0) held[0] = hud_character(cells.hundreds);
    if (cells.hundreds + cells.tens > 0) held[1] = hud_character(cells.tens);
    held[2] = hud_character(cells.units);
    return held;
}

// $81:CDB8-CE0F: a zero hundreds count becomes -1, and then the tens and units move left one
// cell and the third takes `$80:822A`, which the pictures show blank (bowl-lose `0/68`).
std::string stunt_qualifying_text(std::uint16_t qualifying_score) {
    const auto cells = decimal_cells(qualifying_score);
    if (cells.hundreds == 0) return {hud_character(cells.tens), hud_character(cells.units), ' '};
    return {hud_character(cells.hundreds), hud_character(cells.tens), hud_character(cells.units)};
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
// their contact, so the track marker it reads is the previous update's. A stunt event (race mode
// 2) blanks it every update ($82:98B7-98C8, R-0068), so it shows no arrow. The finished flag is
// this update's when the player crossed the line (`$81:823B` runs first); the 10:00 time-out
// sets it after the word is written, so there the arrow stays one update longer.
namespace {
// One rider's arrow (`rider` 0 the player above, 1 the lower view's), as $82:9822 computes it.
std::optional<ClassicRaceArrow> trailing_rider_arrow(const ZoomZooState& previous,
                                                     const ZoomZooState& updated,
                                                     unsigned race_nmis, std::size_t rider) {
    if (classic_race_scenario(updated.track).stunt_event) return std::nullopt;
    const auto& progress = updated.movement.riders[rider].progress;
    const auto own = progress.transition_count;
    const auto other = updated.movement.riders[1U - rider].progress.transition_count;
    const bool behind = (static_cast<std::uint16_t>(own - other) & 0x8000U) != 0;
    if ((own >> 1U) == (other >> 1U) || !behind) return std::nullopt;
    const auto& lap = updated.race.riders[rider];
    const bool crossed = lap.finished && lap.laps_remaining == 0;
    if (crossed || previous.race.riders[rider].finished) return std::nullopt;
    // $81:E9DE: nothing is drawn after a rejected transition.
    if (progress.transition_rejected) return std::nullopt;
    const auto marker = previous.movement.riders[rider].progress.marker_word;
    return ClassicRaceArrow{
        static_cast<ClassicRaceArrow::Direction>(marker >> 14U),
        classic_arrow_chevrons(static_cast<std::uint16_t>(other - own), race_nmis)};
}
} // namespace

std::optional<ClassicRaceArrow>
classic_race_arrow(const ZoomZooState& previous, const ZoomZooState& updated, unsigned race_nmis) {
    return trailing_rider_arrow(previous, updated, race_nmis, 0);
}

std::optional<ClassicRaceArrow> classic_race_lower_arrow(const ZoomZooState& previous,
                                                         const ZoomZooState& updated,
                                                         unsigned race_nmis) {
    return trailing_rider_arrow(previous, updated, race_nmis, 1);
}

// One update of the HUD's text layer: the NMI's counter and arrow, then what the update asks
// the queue for and the one field the queue services, as the original's does one a frame.
void ClassicRaceHudClock::observe_update(const ZoomZooState& previous,
                                         const ZoomZooState& updated) {
    on_screen_ = latest_;
    clear_after_pause(previous, updated);
    if (updated.fade_level >= race_nmi_fade) ++race_nmis_;
    redraw_arrow(previous, updated);
    request_fields(previous, updated);
    if (classic_race_scenario(updated.track).stunt_event) request_stunt_fields(previous, updated);
    request_caption(previous, updated);
    if (updated.split_screen) request_split_fields(previous, updated);
    service_one_field(updated);
}

// $83:F915-F92C: a paused update that closes the menu (CONTINUE GAME, or a reopening while Start
// is still held) zeroes the map from row 5 column 8 to row 9 column 8 after the NMI, so this
// picture already shows the player's cells and an up arrow's rows 5-6 blank. The NMI's next
// arrow redraw and the queue's next write of the cells put them back (R-0078).
void ClassicRaceHudClock::clear_after_pause(const ZoomZooState& previous,
                                            const ZoomZooState& updated) {
    // `$130F` keeps the row the menu last opened at until it opens again.
    if (previous.pause.selection == 0 && updated.pause.selection != 0)
        menu_lower_view_ = updated.pause.lower_view;
    if (!classic_pause_menu_closed(previous, updated)) return;
    // In a split race the same 129 words start at the menu's own row (`$130F`: 0x18A8 for pad
    // 1's, 0x1A68 for pad 2's), so they take that view's caption, which stays blank until its
    // next upload, and its side arrow, until the NMI next redraws it (R-0082).
    if (updated.split_screen) {
        const bool lower = menu_lower_view_;
        for (auto* hud : {&on_screen_, &latest_}) {
            (lower ? hud->opponent_caption_event : hud->caption_event) = 0;
            auto& arrow = lower ? hud->lower_arrow : hud->arrow;
            if (arrow
                && (arrow->direction == ClassicRaceArrow::Direction::Left
                    || arrow->direction == ClassicRaceArrow::Direction::Right))
                arrow.reset();
        }
        return;
    }
    for (auto* hud : {&on_screen_, &latest_}) {
        hud->player_cells_cleared = true;
        if (hud->arrow && hud->arrow->direction == ClassicRaceArrow::Direction::Up)
            hud->arrow->middle_rows_covered = true;
    }
}

// A split race's own requests (R-0082):
// - `$034F`, rider 1's clock: `$81:C6D1-C6DC` asks for its digits on the update after the top
//   clock's tick (`$0E29` = 1) while rider 1 is unfinished, so a VS race's forced finish leaves
//   the last digits written; its last crossing asks for the blank (`$81:824E`). A stunt event's
//   count-down clock asks for both clocks together ($81:C7EE-C7F1).
// - `$0EE9`, rider 1's caption: its consumer (`$81:BEF1-BF31`) takes an event into `$0EC7`
//   (`$81:C05C-C0A0`), or, on its first dry look after a take (`$11C3`), copies the blank
//   message (`$81:BFB9-BFD0`).
void ClassicRaceHudClock::request_split_fields(const ZoomZooState& previous,
                                               const ZoomZooState& updated) {
    const auto& before = previous.race.riders[1];
    const auto& after = updated.race.riders[1];
    if (before.laps_remaining != 0 && after.laps_remaining == 0) pending_.lower_clock_blank = true;
    const bool stunt_event = classic_race_scenario(updated.track).stunt_event;
    const auto& timer = updated.movement.timer;
    if (!after.finished
        && (stunt_event ? latest_.lower_clock != classic_hud_clock(classic_hud_timer(updated))
                        : timer.subframe == 1U))
        pending_.lower_clock = true;
    const auto& queue_before = previous.movement.rewards;
    const auto& queue_after = updated.movement.rewards;
    if (queue_after.read_cursor != queue_before.read_cursor) {
        opponent_caption_buffer_ = queue_after.entries[queue_after.read_cursor];
        opponent_consumed_since_blank_ = true;
    } else if (queue_before.cooldown <= 2U && queue_after.cooldown == announcement::empty_queue_wait
               && opponent_consumed_since_blank_) {
        opponent_caption_buffer_ = 0;
        opponent_consumed_since_blank_ = false;
    } else {
        return;
    }
    pending_.opponent_caption = true;
}

// The split chain's clocks (`$81:E014-E245`): the top clock's blank or digits, then the
// bottom clock's. Returns whether one was written, which spends the NMI.
bool ClassicRaceHudClock::service_split_clock(const ZoomZooState& updated) {
    if (pending_.lower_clock_blank) {
        latest_.lower_clock_blanked = true;
        pending_.lower_clock_blank = pending_.lower_clock = false;
        return true;
    }
    if (!pending_.lower_clock) return false;
    pending_.lower_clock = false;
    if (latest_.lower_clock_blanked) return false;
    latest_.lower_clock = classic_hud_clock(classic_hud_timer(updated));
    return true;
}

// $81:E8E8-$81:EB83: after an update whose progress phase is clear the NMI leaves the arrow
// as it is; otherwise it takes down the arrow it drew last and draws the one the update asks
// for. The up arrow's rows 5-6 are skipped while the player's centred cells show (`$0D19`).
void ClassicRaceHudClock::redraw_arrow(const ZoomZooState& previous, const ZoomZooState& updated) {
    // $81:DB10-DDA4: a split race's lower arrow is redrawn every NMI; its rows 16-17 are skipped
    // while rider 1's cells show (`$0D1B`, R-0080).
    if (updated.split_screen) {
        latest_.lower_arrow = classic_race_lower_arrow(previous, updated, race_nmis_);
        if (latest_.lower_arrow
            && latest_.lower_arrow->direction == ClassicRaceArrow::Direction::Up)
            latest_.lower_arrow->middle_rows_covered = latest_.opponent_cells.has_value();
    }
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
    // in the same update, so the cursor alone can miss a consumption. Two other signs are
    // exact: the display cleared from a dry look (`$81:BED1` clears `$03ED` only after taking an
    // event) and a change of the carried text (`hunter.caption`).
    const bool hunter = classic_race_scenario(updated.track).hunter_tour;
    const bool taken =
        after.queue.read_cursor == (before.queue.read_cursor + 1U) % slots
        || (before.empty_display && !after.empty_display)
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

// R-0068: a stunt event's own requests. The clock's stopping tick sets `$034D` without new
// digits. `$81:C357-C3D0`, at the end of the player's queue consumer, compares the score
// `$77:07BB` with the one it last split (`$12B9`) and, when they differ, splits it and sets
// `$12C9`: on the update a trick's points are paid, the update its caption is taken.
void ClassicRaceHudClock::request_stunt_fields(const ZoomZooState& previous,
                                               const ZoomZooState& updated) {
    if (updated.stunt.clock_stopped && !previous.stunt.clock_stopped) pending_.clock_rewrite = true;
    const auto score = updated.player_announcements.queue.feature_total;
    if (score == score_buffer_) return;
    score_buffer_ = score;
    pending_.score = true;
}

// A checkpoint crossing's cells: the first rider through a slot stores the clock and draws
// nothing ($81:CA38-CA61); the second draws the split against it. $81:CB13 runs the player
// before the opponent within one update, so a slot the player has just stored is seen by the
// opponent's crossing of the same update. The opponent's writer ($81:F1B5-F1C6) takes its
// first cell from the constant $80:8220, the minus glyph, and never reads the sign byte
// `$11BD`; the player's ($81:EF5E-EF6E) reads `$11BB`: measured on every opponent split of
// the M4-16 primary (14 pixels a frame until this was applied). The split chain's writer
// (`$81:E49F-E6F3`) reads `$11BD` in both layouts, so a split race shows rider 1's own sign
// (R-0082).
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
    if (rider == 1 && !updated.split_screen) cell.text[0] = '-';
    return cell;
}

// Service the first pending field and stop, as $81:F357 does: the left field, the clock's
// blanking, the clock's digits, each rider's cells, then the caption. A split race's chain
// (`$81:D853`, R-0082) puts the bottom clock after the top one and rider 1's caption last, and
// always clears the left field, whose handler writes both views.
void ClassicRaceHudClock::service_one_field(const ZoomZooState& updated) {
    const bool finished = updated.race.riders[0].laps_remaining == 0;
    const bool split = updated.split_screen;
    if (pending_.left && split) {
        pending_.left = false;
        return;
    }
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
    // because the race clock has stopped. $81:C6F0-C6F8 sets it only while the player is
    // unfinished, so a VS race's forced finish leaves the last digits written (R-0081).
    if (!latest_.clock_blanked && updated.race.riders[0].finished != forced_finish) {
        auto digits = classic_hud_clock(classic_hud_timer(updated));
        if (latest_.clock != digits || pending_.clock_rewrite) {
            latest_.clock = std::move(digits);
            pending_.clock_rewrite = false;
            return;
        }
    }
    if (split && service_split_clock(updated)) return;
    // The player's cells, drawn or blanked, overwrite an up arrow's rows 5-6.
    const auto cover_arrow = [this](std::size_t rider) {
        auto& arrow = rider == 0 ? latest_.arrow : latest_.lower_arrow;
        if (arrow && arrow->direction == ClassicRaceArrow::Direction::Up)
            arrow->middle_rows_covered = true;
    };
    for (std::size_t rider = 0; rider < 2; ++rider) {
        auto& cell = pending_.cells[rider];
        auto& held = rider == 0 ? latest_.player_cells : latest_.opponent_cells;
        if (cell.kind == ClassicHudCellRequest::Kind::Draw) {
            held = cell.text;
            cell = {};
            cover_arrow(rider);
            if (rider == 0) latest_.player_cells_cleared = false;
            return;
        }
        if (cell.kind == ClassicHudCellRequest::Kind::Blank) {
            cell = {};
            // A finished rider's cells are never blanked, and the handler goes on to the next
            // field without spending the update.
            if (!updated.race.riders[rider].finished) {
                held.reset();
                cover_arrow(rider);
                if (rider == 0) latest_.player_cells_cleared = false;
                return;
            }
        }
    }
    // $81:F28B-F303: a stunt event's score field, before the caption.
    if (pending_.score) {
        latest_.score_cells = stunt_score_cells(score_buffer_, latest_.score_cells);
        pending_.score = false;
        return;
    }
    // $81:F30C-$81:F34D, the last task: the caption rows take the buffer's sixteen characters.
    if (pending_.caption) {
        latest_.caption_event = caption_buffer_;
        pending_.caption = false;
        return;
    }
    // $81:E87C-E8C5, the split chain's last: rider 1's caption.
    if (pending_.opponent_caption) {
        latest_.opponent_caption_event = opponent_caption_buffer_;
        pending_.opponent_caption = false;
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
    // R-0068: the score's cells as the NMI last wrote them; a score never falls during a
    // run, so without the queue's history the state's own score gives the same cells.
    if (scenario.stunt_event) {
        const auto cells =
            published ? published->score_cells
                      : stunt_score_cells(previous_update.player_announcements.queue.feature_total,
                                          ClassicHudPublished{}.score_cells);
        hud.score_field = std::string(cells.begin(), cells.end()) + '/'
                        + stunt_qualifying_text(previous_update.stunt.qualifying_score);
    }
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
                                         : classic_hud_clock(classic_hud_timer(previous_update));
        hud.player_cells =
            published->player_cells_cleared ? "" : published->player_cells.value_or("");
        hud.opponent_cells = published->opponent_cells.value_or("");
        return hud;
    }
    if (!finished || race.finish_delay < 1)
        hud.clock = classic_hud_clock(classic_hud_timer(previous_update));
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
    // $81:F322/$81:F33C write sixteen characters to columns 8-23 of rows 10-11; a split race's
    // NMI writes rider 0's to rows 5-6 ($81:E831-E877, R-0080).
    std::string line;
    for (unsigned column = 0; column < 16; ++column)
        line.push_back(static_cast<char>(entry[column]));
    draw_bg3_text(frame, font, 8, published.split_screen ? 5 : 10, line, ink, inked);
}

// R-0043: the HUD's four fields, on the same layer, in the same font and the
// same colour as the caption. The original's own tilemap rows and columns:
// the left field and the clock on rows 2-3, the player's finish time on rows
// 5-6 and the opponent's on rows 20-21, both from column 13.
// $81:D872-DDA4: a split race's arrows (R-0080). The side arrows grow from column 28 leftward or
// from column 5 rightward on rows 6-7 above and 20-21 below; the up arrow down from row 2 (15
// below), its middle rows skipped under the cells; the down arrow down from row 11 (25 below).
void draw_split_arrow(RgbFrame& frame, std::span<const std::uint8_t> font,
                      const ClassicRaceArrow& arrow, bool lower, std::array<std::uint8_t, 3> ink,
                      std::bitset<256 * 224>& inked) {
    using Direction = ClassicRaceArrow::Direction;
    constexpr unsigned upper_side_row = 6, lower_side_row = 20, upper_up_row = 2, lower_up_row = 15,
                       upper_down_row = 11, lower_down_row = 25;
    const unsigned side_row = lower ? lower_side_row : upper_side_row,
                   up_row = lower ? lower_up_row : upper_up_row,
                   down_row = lower ? lower_down_row : upper_down_row;
    for (unsigned chevron = 0; chevron < arrow.chevrons; ++chevron) {
        if (arrow.direction == Direction::Right || arrow.direction == Direction::Left) {
            const bool right = arrow.direction == Direction::Right;
            const unsigned column = right ? right_arrow_end - chevron : left_arrow_column + chevron;
            const unsigned tile = right ? right_chevron : left_chevron;
            draw_bg3_tile(frame, font, column, side_row, tile, ink, inked);
            draw_bg3_tile(frame, font, column, side_row + 1U, tile + lower_half, ink, inked);
            continue;
        }
        const bool up = arrow.direction == Direction::Up;
        if (up && chevron > 0 && arrow.middle_rows_covered) break;
        const unsigned row = (up ? up_row : down_row) + chevron;
        const unsigned tile = up ? up_chevron : down_chevron;
        draw_bg3_tile(frame, font, vertical_arrow_column, row, tile, ink, inked);
        draw_bg3_tile(frame, font, vertical_arrow_column + 1U, row, tile + 1U, ink, inked);
    }
}

// The lower view's clock: the second timer is one tick behind the first in the captured PAL
// demo's two-view race.
std::string classic_split_lower_clock(const ZoomZooState& state) {
    auto bottom_timer = classic_hud_timer(state);
    const bool stunt_event = classic_race_scenario(state.track).stunt_event;
    if (!stunt_event && state.movement.timer.subframe == 0 && bottom_timer.tenths)
        --bottom_timer.tenths;
    else if (!stunt_event && state.movement.timer.subframe == 0
             && (bottom_timer.seconds || bottom_timer.tens_seconds || bottom_timer.minutes)) {
        bottom_timer.tenths = 9;
        if (bottom_timer.seconds)
            --bottom_timer.seconds;
        else {
            bottom_timer.seconds = 9;
            if (bottom_timer.tens_seconds)
                --bottom_timer.tens_seconds;
            else {
                bottom_timer.tens_seconds = 5;
                --bottom_timer.minutes;
            }
        }
    }
    return classic_hud_clock(bottom_timer);
}

void draw_split_hud(RgbFrame& frame, const ZoomZooState& state,
                    const ClassicRacePresentationContent& content, const ClassicHudText& hud,
                    const std::optional<ClassicHudPublished>& published,
                    std::array<std::uint8_t, 3> ink, std::array<std::uint8_t, 3> opponent_ink,
                    unsigned opponent_caption_event, std::bitset<256 * 224>& inked) {
    const auto font = content.caption_font;
    // $81:D1B2-D30C initializes two lap counters; $81:EB44-EB83 writes
    // the active fields at rows 1 and 15. The second timer is one tick
    // behind the first in the captured PAL demo's two-view race.
    draw_bg3_text(frame, font, hud.left_column, 1, hud.left, ink, inked);
    draw_bg3_text(frame, font, 24, 1, hud.clock, ink, inked);
    const auto& opponent = state.race.riders[1];
    const auto lap = classic_hud_lap(opponent.laps_remaining, content.scenario.laps);
    const auto lower_left = content.scenario.stunt_event ? std::string("stunt")
                          : opponent.finished && opponent.laps_remaining == 0
                              ? std::string("finish")
                          : !content.scenario.tour_race
                              ? std::string("race")
                              : std::to_string(lap) + "/" + std::to_string(content.scenario.laps);
    // $81:D192-D4DB writes the lower "race" or lap from column 2, and `$0D17`'s "finish" from
    // column 1, as above (R-0080).
    draw_bg3_text(frame, font, lower_left == "finish" ? 1U : 2U, 15, lower_left, opponent_ink,
                  inked);
    // The lower clock as the split chain last wrote it (R-0082); without that history, from the
    // state: one tick behind the top, blank once rider 1 has crossed its last line ($81:824B,
    // $81:E13B-E1C2, R-0080).
    if (published && (published->lower_clock || published->lower_clock_blanked)) {
        if (!published->lower_clock_blanked)
            draw_bg3_text(frame, font, 24, 15, *published->lower_clock, opponent_ink, inked);
    } else if (!(opponent.finished && opponent.laps_remaining == 0)) {
        draw_bg3_text(frame, font, 24, 15, classic_split_lower_clock(state), opponent_ink, inked);
    }
    draw_bg3_text(frame, font, 13, 3, hud.player_cells, ink, inked);
    draw_bg3_text(frame, font, 13, 17, hud.opponent_cells, opponent_ink, inked);
    // $81:E87C-E8C5: rider 1's caption, all sixteen cells from column 8 of rows 19-20 (R-0080).
    if (const auto caption = classic_caption_text(opponent_caption_event, content.captions)) {
        std::string line;
        for (unsigned column = 0; column < 16; ++column)
            line.push_back(static_cast<char>((*caption)[column]));
        draw_bg3_text(frame, font, 8, 19, line, opponent_ink, inked);
    }
    const auto name = [&](unsigned rider) {
        std::string result;
        const auto record = content.rider_names.subspan(rider * 16U, 16U);
        for (auto byte : record) {
            if (byte == ' ' || byte == '_' || byte == 0xffU || byte == 0x00U) break;
            if (byte >= 'A' && byte <= 'Z') byte += 'a' - 'A';
            result.push_back(static_cast<char>(byte));
        }
        return result;
    };
    const auto top_name = name(state.pairing.rider);
    const auto bottom_name = name(state.pairing.opponent);
    draw_bg3_text(frame, font, 30U - static_cast<unsigned>(top_name.size()), 11, top_name, ink,
                  inked);
    draw_bg3_text(frame, font, 30U - static_cast<unsigned>(bottom_name.size()), 25, bottom_name,
                  opponent_ink, inked);
    const auto arrow =
        published ? published->arrow : classic_arrow_without_history(state, content.scenario);
    if (arrow) draw_split_arrow(frame, font, *arrow, false, ink, inked);
    if (published && published->lower_arrow)
        draw_split_arrow(frame, font, *published->lower_arrow, true, opponent_ink, inked);
}

// $83:F6B9-F6D0 places the menu at VRAM 0x18A8 and 0x18ED (the map at 0x1800: row 5 column 8 and
// row 7 column 13) when pad 1 paused; $83:F807-F8A7 puts tile 0x46 ("<", the left chevron)
// in column 24 beside the choice and the blank 0x59 beside the other.
constexpr unsigned pause_first_row = 5, pause_second_row = 7, pause_first_column = 8,
                   pause_second_column = 13, pause_cursor_column = 24;
constexpr unsigned pause_first_choice_cells = 15, pause_second_choice_cells = 4;
// $83:F6D9-F6F3: pad 2's menu at VRAM 0x1A68 and 0x1AAD, 0x1C0 words (14 rows) further on.
constexpr unsigned pause_lower_view_rows = 14;

bool classic_pause_menu_closed(const ZoomZooState& previous, const ZoomZooState& updated) {
    return updated.pause.selection == 0
        && updated.pause.suspended_updates > previous.pause.suspended_updates;
}

Bg3Cells classic_pause_menu_cells(bool lower_view) {
    const unsigned first_row = pause_first_row + (lower_view ? pause_lower_view_rows : 0U),
                   second_row = pause_second_row + (lower_view ? pause_lower_view_rows : 0U);
    Bg3Cells cells;
    const auto hold = [&cells](unsigned column, unsigned row, unsigned count) {
        for (unsigned at = column; at < column + count; ++at) {
            cells.set(row * 32U + at);
            cells.set((row + 1U) * 32U + at);
        }
    };
    hold(pause_first_column, first_row, pause_first_choice_cells);
    hold(pause_second_column, second_row, pause_second_choice_cells);
    hold(pause_cursor_column, first_row, 1);
    hold(pause_cursor_column, second_row, 1);
    return cells;
}

bool classic_pause_shows_message(const ZoomZooState& state) {
    return state.pause.selection != 0 && state.league_statistics.enabled
        && !state.movement.countdown && !state.race.riders[0].finished
        && !state.race.riders[1].finished;
}

constexpr unsigned pause_message_cells = 16;

Bg3Cells classic_pause_message_cells(bool lower_view) {
    const unsigned row = pause_first_row + (lower_view ? pause_lower_view_rows : 0U);
    Bg3Cells cells;
    for (unsigned column = pause_first_column; column < pause_first_column + pause_message_cells;
         ++column) {
        cells.set(row * 32U + column);
        cells.set((row + 1U) * 32U + column);
    }
    return cells;
}

void draw_classic_pause_message(RgbFrame& frame, const ClassicRacePresentationContent& content,
                                std::uint8_t rider, bool lower_view,
                                std::array<std::uint8_t, 3> ink, std::bitset<256 * 224>& inked) {
    const auto font = content.caption_font;
    const auto at = std::size_t{rider} * pause_message_cells;
    if (font.size() != 2048 || content.pause_messages.size() < at + pause_message_cells) return;
    // $83:F731-F764: a space is tile 0x80, which draws nothing; the letters take the caption
    // alphabet's tiles ($80:81FE). `!` and `"` load `$80:8223` and `$80:8224` as words, so the
    // next byte lands in the tile word's high bits: `"` is tile 0x261, past the font, and shows
    // nothing (`league`, rider 9's "mellowin", R-0082). `!` (tile 0x160) is not captured, so
    // it keeps the caption's glyph.
    std::string line;
    for (const auto byte : content.pause_messages.subspan(at, pause_message_cells))
        line.push_back(byte == '"' ? ' ' : static_cast<char>(byte));
    draw_bg3_text(frame, font, pause_first_column,
                  pause_first_row + (lower_view ? pause_lower_view_rows : 0U), line, ink, inked);
}

void draw_classic_pause_menu(RgbFrame& frame, const ClassicRacePresentationContent& content,
                             std::uint16_t selection, std::string_view second_choice,
                             bool lower_view, std::array<std::uint8_t, 3> ink,
                             std::bitset<256 * 224>& inked) {
    const auto font = content.caption_font;
    if (font.size() != 2048) return;
    const unsigned first_row = pause_first_row + (lower_view ? pause_lower_view_rows : 0U),
                   second_row = pause_second_row + (lower_view ? pause_lower_view_rows : 0U);
    // $83:F616: a blank (0x79), "continue game" and a blank; $83:F636: "quit". The standalone
    // race's "restart" is centred under the first line instead.
    constexpr std::string_view first_choice = "continue game";
    constexpr unsigned first_choice_column = pause_first_column + 1U;
    draw_bg3_text(frame, font, first_choice_column, first_row, first_choice, ink, inked);
    const auto centred = static_cast<unsigned>(first_choice.size() - second_choice.size()) / 2U;
    const auto second_column =
        second_choice == "quit" ? pause_second_column : first_choice_column + centred;
    draw_bg3_text(frame, font, second_column, second_row, second_choice, ink, inked);
    // The selection is 1 for the first choice and 0xFFFF for the second.
    const unsigned cursor_row = selection & 0x8000U ? second_row : first_row;
    draw_bg3_tile(frame, font, pause_cursor_column, cursor_row, left_chevron, ink, inked);
    draw_bg3_tile(frame, font, pause_cursor_column, cursor_row + 1U, left_chevron + lower_half, ink,
                  inked);
}

void draw_classic_hud(RgbFrame& frame, const ZoomZooState& state,
                      const ClassicRacePresentationContent& content,
                      std::optional<std::uint32_t> opponent_finish_frame,
                      const std::optional<ClassicHudPublished>& published,
                      std::array<std::uint8_t, 3> ink, std::array<std::uint8_t, 3> opponent_ink,
                      unsigned opponent_caption_event, std::bitset<256 * 224>& inked) {
    const auto font = content.caption_font;
    if (font.size() != 2048) return;
    const auto hud =
        classic_race_hud_text(state, content.scenario, opponent_finish_frame, published);
    if (state.split_screen) {
        draw_split_hud(frame, state, content, hud, published, ink, opponent_ink,
                       opponent_caption_event, inked);
        return;
    }
    draw_bg3_text(frame, font, hud.left_column, 2, hud.left, ink, inked);
    draw_bg3_text(frame, font, 24, 2, hud.clock, ink, inked);
    draw_bg3_text(frame, font, 13, 5, hud.player_cells, ink, inked);
    draw_bg3_text(frame, font, 13, 20, hud.opponent_cells, ink, inked);
    draw_bg3_text(frame, font, 23, 24, hud.score_field, ink, inked);
    auto arrow =
        published ? published->arrow : classic_arrow_without_history(state, content.scenario);
    // Without history the cells drawn above stand for `$0D19`.
    if (!published && arrow && arrow->direction == ClassicRaceArrow::Direction::Up)
        arrow->middle_rows_covered = !hud.player_cells.empty();
    if (arrow) draw_classic_arrow(frame, font, *arrow, ink, inked);
}

} // namespace unirally
