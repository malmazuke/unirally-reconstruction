#include "presentation.hpp"
#include "zoom_zoo_movement.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>

// The race's window tables and palette cycles, by frame: the countdown, the finish and the result.
namespace unirally {

// The channel-6 window family: 25 tables of 899 bytes at $15:8000, addressed
// through the 16-bit offsets of $83:E55C. The last byte of each is the HDMA
// run terminator, which `render_window_xor` does not consume.
static constexpr unsigned window_table_stride = 899, window_table_body = 898,
                          window_table_count = 25;

// DRAGSTER's race vblank begins at $82:D7F5 + 6 updates, so the first frame the
// channel-6 setup $80:868E publishes is initialization frame 1328 plus 6. The
// countdown starts at $11C5 = 270 and the driver of frame n-1 chooses the table
// frame n shows, so that table was chosen with $11C5 = 270 - (n - 1334).
static constexpr std::uint32_t dragster_race_setup_frame = 1334;

static constexpr unsigned dragster_window_countdown_start = 270;

// $83:E759/E611/E663/E6C5 index 5 + $1229. $83:CC08 loads $1229 from $0BA7,
// the player's start reflection, which is 1 on DRAGSTER, so the legacy v1
// path's transitions draw index 6 (`classic_window_transition_member` derives
// it from the track for the shared renderer: 5 on ZOOM ZOO).
static constexpr unsigned dragster_window_transition_index = 6;

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
static constexpr unsigned winner_window_first = 7, winner_window_cycle = 18,
                          winner_window_life_updates = 360;

void ClassicWindowPointer::reset() {
    drivers_ = {};
    order_ = {};
    pending_ = {};
    ordered_ = pending_count_ = 0;
    chosen_.reset();
    published_.reset();
    observed_ = false;
}

void ClassicWindowPointer::observe_update(const ZoomZooState& previous, const ZoomZooState& updated,
                                          unsigned transition_member) {
    // R-0040: the vblank setup of frame N publishes the selection the drivers
    // made in update N-1; the race vblank runs from initialization + 6 and
    // for the last time on result-loading update 1.
    const auto scenario = classic_race_scenario(updated.track);
    observed_ = true;
    if (updated.movement.frame >= scenario.initialization_frame + 6U
        && updated.result_updates <= 1U)
        published_ = chosen_;
    // The pause menu disables channel 6 for every update it diverts, from the
    // update that opens it through the update that resumes (countdown-pause
    // original: no window on 1401-1502 for a pause opened on 1400 and resumed
    // on 1501) and any later update on which Start is still held (ZOOM ZOO
    // pause-countdown original: held 1460-1462, no window through 1463); the
    // drivers neither run nor age through it.
    if (zoom_zoo_update_was_paused(previous, updated)) {
        chosen_.reset();
        return;
    }
    if (updated.result_updates > 1U) return;
    // Drivers armed by the previous update's finish run from this update.
    // Each rider finishes once per race, so at most two are ever armed; a
    // caller that observes a new race without reset() gets no third arming.
    for (std::size_t i = 0; i < pending_count_ && ordered_ < order_.size(); ++i)
        order_[ordered_++] = pending_[i];
    pending_count_ = 0;
    for (std::size_t rider = 0; rider < 2 && pending_count_ < pending_.size(); ++rider)
        if (!previous.race.riders[rider].finished && updated.race.riders[rider].finished) {
            drivers_[rider] = {};
            pending_[pending_count_++] = rider;
        }
    // $0300 counts from race start: 0 at the initialization boundary. Its
    // parity is the frame's only when that boundary is even, as on DRAGSTER
    // and ZOOM ZOO; six cold-start tracks start on an odd frame (part 3 review).
    const bool parity_set = ((updated.movement.frame - scenario.initialization_frame) & 1U) != 0U;
    std::optional<unsigned> request;
    for (std::size_t i = 0; i < ordered_ && !request; ++i) {
        auto& driver = drivers_[order_[i]];
        if (driver.dead) continue;
        // $0F07/$0F09: set to 360 while the index is zero, counted down once
        // per driver update of either parity (random-1: 359 after 3215, zero
        // after 3574, blank from 3576; reversal: 359 after both 3326 and 3327
        // as its first update is even), frozen through a pause.
        if (driver.index == 0U) driver.life = winner_window_life_updates;
        if (driver.life == 0U) {
            driver.dead = true;
            continue;
        }
        --driver.life;
        if (parity_set)
            driver.index = driver.index == 0U ? 8U : driver.index == 24U ? 7U : driver.index + 1U;
        request = driver.index == 0U ? winner_window_first : driver.index;
    }
    chosen_ = request ? request
                      : classic_countdown_window(previous.movement.countdown, parity_set,
                                                 transition_member);
}

std::optional<unsigned> zoom_zoo_palette_cycle_index(std::uint32_t frame) {
    // $82:D382-D496 runs from frame 1382 while $0B92 is set: it loads the
    // colours from index $0B84, then advances $0B84 modulo 16. $0B84 ends frame
    // n at (n-1381)&15, through pause, so frame n draws index (n-1382)&15.
    if (frame < 1382U) return std::nullopt;
    return (frame - 1382U) & 15U;
}

namespace {

// Seventeen 16-word tables at $80:82AB: colours 96-111, then colour 0.
void load_race_palette_phase(std::array<std::uint8_t, 512>& cgram,
                             std::span<const std::uint8_t> tables, unsigned index,
                             bool colour_zero) {
    if (tables.size() != 544) throw std::invalid_argument("race palette cycle has the wrong size");
    for (std::size_t table = 0; table < (colour_zero ? 17U : 16U); ++table) {
        const auto source = table * 32U + index * 2U;
        const auto destination = table < 16 ? (96U + table) * 2U : 0U;
        cgram[destination] = tables[source];
        cgram[destination + 1] = tables[source + 1];
    }
}

// The frame whose race vblank selections are on screen: the race vblank, and
// with it the palette routine and the channel-6 setup, runs for the last time
// on result-loading update 1, so from update 2 the original keeps that update's
// selection ($420C is never rewritten after it, R-0040).
std::optional<std::uint32_t> race_vblank_frame(std::uint32_t frame, std::uint16_t loading_updates) {
    if (loading_updates == 0) return frame;
    if (loading_updates - 1U > frame) return std::nullopt;
    return frame - (loading_updates - 1U);
}

struct RacePalettePhase {
    unsigned index;
    bool colour_zero;
};

// $82:D382-D496 first runs on `setup_frame`. Racing frame n draws phase
// (n-setup)&15 with colour 0; the loading freeze keeps colours 96-111 at the
// last vblank's phase with a black colour 0, until the result palettes load.
std::optional<RacePalettePhase>
race_palette_phase(std::uint32_t frame, std::uint16_t loading_updates, std::uint32_t setup_frame) {
    const auto shown = race_vblank_frame(frame, loading_updates);
    if (!shown || *shown < setup_frame) return std::nullopt;
    return RacePalettePhase{(*shown - setup_frame) & 15U, loading_updates == 0};
}

void apply_race_palette_phase(std::array<std::uint8_t, 512>& cgram,
                              std::span<const std::uint8_t> tables,
                              std::optional<RacePalettePhase> phase) {
    if (!phase) return;
    load_race_palette_phase(cgram, tables, phase->index, phase->colour_zero);
    if (!phase->colour_zero) cgram[0] = cgram[1] = 0;
}

// The legacy finish struct's loading count, or zero while it is not loading.
std::uint16_t legacy_loading_updates(const RaceFinishState& finish) {
    return finish.phase == RacePhase::ResultLoading ? finish.result_loading_updates
                                                    : std::uint16_t{0};
}

} // namespace

void apply_zoom_zoo_palette_cycle(std::array<std::uint8_t, 512>& cgram,
                                  std::span<const std::uint8_t> tables, std::uint32_t frame) {
    if (tables.size() != 544)
        throw std::invalid_argument("ZOOM ZOO race palette cycle has the wrong size");
    const auto index = zoom_zoo_palette_cycle_index(frame);
    if (!index) return;
    load_race_palette_phase(cgram, tables, *index, true);
}

void apply_dragster_palette_cycle(std::array<std::uint8_t, 512>& cgram,
                                  std::span<const std::uint8_t> tables,
                                  const MovementState& state) {
    if (tables.size() != 544)
        throw std::invalid_argument("DRAGSTER race palette cycle has the wrong size");
    // Original DRAGSTER replays match (frame-1334)&15 on every racing frame,
    // and the phase holds from loading update 1 (3454, 3559) through at least
    // update 75 (R-0037).
    apply_race_palette_phase(
        cgram, tables,
        race_palette_phase(state.frame, legacy_loading_updates(state.finish), 1334U));
}

void apply_classic_race_palette_cycle(std::array<std::uint8_t, 512>& cgram,
                                      std::span<const std::uint8_t> tables,
                                      const ZoomZooState& state, std::uint32_t setup_frame) {
    if (tables.size() != 544) throw std::invalid_argument("race palette cycle has the wrong size");
    apply_race_palette_phase(
        cgram, tables, race_palette_phase(state.movement.frame, state.result_updates, setup_frame));
}

std::span<const std::uint8_t> dragster_window_table(std::span<const std::uint8_t> tables,
                                                    unsigned index) {
    if (tables.size() != window_table_count * window_table_stride)
        throw std::invalid_argument("Classic window table family has the wrong size");
    if (index >= window_table_count)
        throw std::invalid_argument("Classic window table index is out of range");
    const auto at = static_cast<std::size_t>(index) * window_table_stride;
    if (tables[at + window_table_body] != 0)
        throw std::invalid_argument("Classic window table lacks its run terminator");
    return tables.subspan(at, window_table_body);
}

namespace {

// Updates since the winning rider's finish: 0 on the update that records it.
// The banner driver runs from the next update, so it starts one frame later.
std::optional<std::uint32_t> updates_since_winner_finish(const RaceFinishState& finish) {
    if (finish.outcome == RaceOutcome::PlayerWon) {
        // The player owns the global finish delay, so its count is exact for
        // the whole banner: 0..240, held at 240 once result loading starts,
        // which is the last update on which the driver runs.
        if (finish.phase == RacePhase::FinishDelay
            || (finish.phase == RacePhase::ResultLoading && finish.result_loading_updates))
            return finish.player_finish_delay;
        return std::nullopt;
    }
    if (finish.outcome == RaceOutcome::PlayerLost) {
        // The opponent's finish frame is only recoverable from the legacy
        // state while its 120-update finish animation counter runs (R-0040).
        const auto remaining = finish.finish_animation_countdown[1];
        if (remaining == 0 || remaining > 120U) return std::nullopt;
        return 120U - remaining;
    }
    return std::nullopt;
}

// Odd frames in [from, to): the banner steps of driver frames from..to-1.
std::uint32_t odd_frames(std::uint32_t from, std::uint32_t to) {
    return to <= from ? 0U : to / 2U - from / 2U;
}

// The selection on screen for `frame`, shared by both state forms, from the
// frames of the first and the latest finish (equal when one rider finished),
// as `ClassicWindowPointer` would publish it without a diverted update.
std::optional<unsigned> window_table_index_for(std::uint32_t frame, std::uint16_t loading_updates,
                                               std::optional<std::uint32_t> first_finish,
                                               std::optional<std::uint32_t> latest_finish,
                                               std::uint32_t setup_frame,
                                               unsigned transition_member) {
    // The drivers' parity is `$0300`'s, which counts from the initialization
    // boundary (setup_frame - 6). Shifting every frame by the boundary's own
    // parity makes the frame parities below `$0300`'s; on the even boundaries
    // of DRAGSTER and ZOOM ZOO the shift is zero. Precondition: setup_frame is
    // the race vblank's first frame, initialization + 6, as every caller passes.
    const std::uint32_t boundary_parity = (setup_frame - 6U) & 1U;
    frame -= boundary_parity;
    setup_frame -= boundary_parity;
    if (first_finish) *first_finish -= boundary_parity;
    if (latest_finish) *latest_finish -= boundary_parity;
    const auto shown = race_vblank_frame(frame, loading_updates);
    if (!shown) return std::nullopt;
    // The winner banner replaces the countdown family; the two never overlap in
    // a race the countdown can hold at the line. The picture shows the choice
    // of driver frame shown - 1.
    if (first_finish) {
        const auto first = *first_finish;
        if (*shown < first + 2U) return std::nullopt; // the finish update selects nothing
        const auto driver = *shown - 1U;
        // The first driver's life runs from its first odd update; its phase
        // is the odd updates since its first update.
        const auto first_odd = ((first + 1U) & 1U) != 0U ? first + 1U : first + 2U;
        if (driver <= first_odd + winner_window_life_updates - 1U)
            return winner_window_first
                 + static_cast<unsigned>(odd_frames(first + 1U, *shown) % winner_window_cycle);
        // It stopped on the update after; the other rider's driver, if armed,
        // runs from the later of that update and the update after its finish.
        if (!latest_finish || *latest_finish <= first) return std::nullopt;
        const auto second = std::max(first_odd + winner_window_life_updates, *latest_finish + 1U);
        const auto second_odd = (second & 1U) != 0U ? second : second + 1U;
        if (driver < second || driver > second_odd + winner_window_life_updates - 1U)
            return std::nullopt;
        return winner_window_first
             + static_cast<unsigned>(odd_frames(second, *shown) % winner_window_cycle);
    }
    if (*shown < setup_frame) return std::nullopt;
    const auto elapsed = *shown - setup_frame;
    if (elapsed >= dragster_window_countdown_start) return std::nullopt;
    // The driver of frame shown - 1 read 270 - elapsed and its own parity.
    return classic_countdown_window(
        static_cast<std::uint16_t>(dragster_window_countdown_start - elapsed),
        ((*shown - 1U) & 1U) != 0U, transition_member);
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
    return 5U + (classic_race_start_reflected(decoded_track, 0) ? 1U : 0U);
}

std::optional<unsigned> classic_countdown_window(std::uint16_t countdown, bool parity_set,
                                                 unsigned transition_member) {
    // $83:E59C dispatches on $11C5 as the update read it, before its own
    // decrement, and inside each digit on the digit's own threshold: above it
    // the transition table, below it the digit's table.
    if (countdown == 0U) return std::nullopt;
    if (countdown >= 250U) return transition_member;
    if (countdown >= 221U) return 0U;
    if (countdown >= 190U) return transition_member;
    if (countdown >= 161U) return 1U;
    if (countdown >= 130U) return transition_member;
    if (countdown >= 101U) return 2U;
    if (countdown >= 70U) return transition_member;
    // $83:E728 picks between the two GO tables on the $0300 parity: an odd
    // driver update's choice is member 3, on screen on the even frame after it.
    return parity_set ? 3U : 4U;
}

std::optional<unsigned> dragster_window_table_index(const MovementState& state) {
    const auto loading = legacy_loading_updates(state.finish);
    std::optional<std::uint32_t> finish;
    if (const auto since = updates_since_winner_finish(state.finish))
        if (const auto shown = race_vblank_frame(state.frame, loading)) finish = *shown - *since;
    return window_table_index_for(state.frame, loading, finish, finish, dragster_race_setup_frame,
                                  dragster_window_transition_index);
}

std::optional<std::uint32_t> classic_opponent_finish_frame(const ZoomZooState& state) {
    // Both finish times are needed, so this serves a restored state only once
    // the player has finished: it recovers the banner's remainder when the
    // player finished within its 360 frames (lose-a: 104 frames after the
    // opponent) and nothing when the whole banner preceded the player's finish
    // (random-1: 422 frames after, reversal: 967). Live play does not depend on
    // it; the history tracker records the opponent's finish as it happens.
    const auto& race = state.race;
    if (!race.riders[0].finished || !race.riders[1].finished) return std::nullopt;
    if (race.total_times[0] >= no_time || race.total_times[1] >= no_time) return std::nullopt;
    // The finish delay counts once per race update and holds at 240 from the
    // update before result loading, so the frame it last advanced on is the
    // loading start minus the loading count (lose-a: finish 3318, delay 240 at
    // 3558, loading update 1 at 3559).
    const auto counted = state.result_updates ? state.movement.frame
                                                    - std::min<std::uint32_t>(state.movement.frame,
                                                                              state.result_updates)
                                              : state.movement.frame;
    if (race.finish_delay > counted) return std::nullopt;
    const auto player_finish = counted - race.finish_delay;
    // The clock's parity term is `contact_phase`, 0 at the initialization
    // boundary, so a finish frame's parity is taken relative to it.
    const auto boundary = classic_race_scenario(state.track).initialization_frame;
    // finish_centiseconds: two per frame plus the frame parity, so
    // total[0]-total[1] = 2(fa-fb)+(fa&1)-(fb&1); one parity of fb fits, in
    // either finish order.
    const int difference =
        static_cast<int>(race.total_times[0]) - static_cast<int>(race.total_times[1]);
    for (const unsigned parity : {0U, 1U}) {
        const int twice_gap = difference - static_cast<int>((player_finish - boundary) & 1U)
                            + static_cast<int>(parity);
        if (twice_gap % 2 != 0) continue;
        const int gap = twice_gap / 2;
        if (gap > static_cast<int>(player_finish)) continue;
        const auto opponent_finish =
            static_cast<std::uint32_t>(static_cast<int>(player_finish) - gap);
        if (((opponent_finish - boundary) & 1U) == parity) return opponent_finish;
    }
    return std::nullopt;
}

std::optional<unsigned>
classic_window_table_index(const ZoomZooState& state, std::uint32_t setup_frame,
                           std::optional<std::uint32_t> opponent_finish_frame,
                           unsigned transition_member) {
    const auto& race = state.race;
    // The player's finish frame from its delay (0..240, held at 240 from the
    // update before result loading); the opponent's from the history or, once
    // both have finished, from the two finish times. The first finisher's
    // driver runs first; the other's follows once it stops.
    std::optional<std::uint32_t> player_finish, opponent_finish = opponent_finish_frame;
    if (race.riders[0].finished) {
        const auto counted =
            state.result_updates
                ? state.movement.frame
                      - std::min<std::uint32_t>(state.movement.frame, state.result_updates)
                : state.movement.frame;
        if (race.finish_delay <= counted) player_finish = counted - race.finish_delay;
    }
    if (race.riders[1].finished && !opponent_finish)
        opponent_finish = classic_opponent_finish_frame(state);
    std::optional<std::uint32_t> first, latest;
    for (const auto& finish : {player_finish, opponent_finish}) {
        if (!finish) continue;
        first = first ? std::min(*first, *finish) : *finish;
        latest = latest ? std::max(*latest, *finish) : *finish;
    }
    return window_table_index_for(state.movement.frame, state.result_updates, first, latest,
                                  setup_frame, transition_member);
}

unsigned classic_race_prior_fade(const ZoomZooState& state, const ZoomZooState* previous_update,
                                 const ClassicRaceScenario& scenario) {
    // $0FF1 grows by one per race update from initialization and holds at 30
    // ($83:CCC1-CCC9); the single-state runner passes the state as its own
    // previous, and after saturation the state alone cannot give the
    // preceding level, so the accepted frame formula is used there.
    if (previous_update && previous_update->movement.frame < state.movement.frame)
        return std::min(30U, unsigned(previous_update->fade_level));
    const auto first = scenario.initialization_frame;
    return state.movement.frame <= first ? 0U : std::min(30U, state.movement.frame - first - 1U);
}

RaceFinishState classic_finish_view(const ZoomZooState& race) {
    RaceFinishState finish = race.movement.finish;
    for (std::size_t rider = 0; rider < 2; ++rider) {
        const auto& lap = race.race.riders[rider];
        finish.rider_finished[rider] = lap.finished != 0;
        if (!lap.finished) continue;
        // One lap: the recorded crossing digits and the total are the finish time.
        finish.finish_time_digits[rider] = lap.time_digits;
        finish.finish_time_centiseconds[rider] = race.race.total_times[rider];
    }
    finish.player_finish_delay = race.race.finish_delay;
    finish.result_loading_updates = race.result_updates;
    if (race.race.riders[0].finished)
        finish.outcome =
            classic_race_player_won(race) ? RaceOutcome::PlayerWon : RaceOutcome::PlayerLost;
    if (race.result_updates && race.result_updates >= stable_result_updates(race))
        finish.phase = RacePhase::ResultScreen;
    else if (race.result_updates)
        finish.phase = RacePhase::ResultLoading;
    else if (race.race.riders[0].finished)
        finish.phase = RacePhase::FinishDelay;
    return finish;
}

} // namespace unirally
