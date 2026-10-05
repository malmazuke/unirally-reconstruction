// Race scenarios by track, the race start and a restart.

#include "race_camera.hpp"
#include "word_arithmetic.hpp"
#include "zoom_zoo_movement.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <utility>

namespace unirally {
namespace {

// The result is stable at load 115 after a lap race (ZOOM ZOO's), and after a one-run race
// at 226 when the player won and 242 when it lost (DRAGSTER's; R-0012, R-0019).
constexpr std::uint16_t lap_race_stable_result = 115;
constexpr std::uint16_t one_run_stable_won = 226, one_run_stable_lost = 242;
// The HUNTER tour's tier ($83:CC0B-CC29): AI level 3, a progress adjustment bound of 0x60 and
// the opponent's catch-up term 0x40.
constexpr OpponentTier hunter_tier{3, 0x40, 0x60};
constexpr std::uint8_t first_hunter_track = 40, last_hunter_track = 44;
// $83:CC2B-CC7C: the AI level is the opponent's character less 16; SILVIA (level 2) adds 0x20 to
// the track's catch-up byte and GOLDWYN (3) 0x40. The bound is 0x60 (a one-run race) or 0x48 (a
// lap race) less the catch-up, 8-bit, or the base when that sets bit 7.
constexpr std::uint8_t opponent_level_base = 16;
constexpr std::uint16_t silvia_catch_up = 0x20, goldwyn_catch_up = 0x40;
constexpr std::uint16_t one_run_adjustment_limit = 0x60, lap_adjustment_limit = 0x48;
// The race start ($82:D7C6-DBD6): a 270-update countdown ($82:D841-D844), a base speed cap
// of 448, no times (60000), the camera 256 above and left of the player in 16-unit cells,
// the rider sprites off screen (0xE0E0; $82:D724-D731), the opponent's HUD OAM x 0x65
// ($82:D76D-D76F), start boosts of 384, the first hint phase 30 ($82:D972-D975) and every
// checkpoint unseen (0xFF; $81:CD2A).
constexpr std::uint16_t start_countdown = 270, base_speed_cap = 448;
constexpr unsigned camera_margin = 256, camera_cell_mask = 0xfff0;
constexpr std::uint16_t sprites_off_screen = 0xe0e0, start_boost = 384, first_hint_phase = 30;
constexpr std::uint8_t opponent_hud_oam_x = 0x65, checkpoint_unseen = 0xff;
constexpr std::size_t reward_weight_count = 26;
// R-0066: the stunt result screen takes over from result load 106 ($80:F0EE, dispatched once the
// race's return has run; STUNT-RESULT), so a stunt event's result is stable at load 105.
constexpr std::uint16_t stunt_stable_result = 105;
// $83:9EEB: five tracks a tour; three qualifying scores a tour (no medal, bronze, silver or gold).
constexpr unsigned tracks_per_tour = 5, qualifying_levels = 3;
constexpr std::uint8_t best_medal_gold = 3;

// A scenario the original sets up for `track`: a lap race or a one-run race, and the
// HUNTER tour's opponent on its tracks.
ClassicRaceScenario observed_scenario(ClassicRaceTrack track, std::uint32_t initialization_frame,
                                      std::uint16_t laps, bool lap_race) {
    const bool hunter = track.index >= first_hunter_track && track.index <= last_hunter_track;
    return {track,
            initialization_frame,
            laps,
            lap_race ? lap_race_stable_result : one_run_stable_won,
            lap_race ? lap_race_stable_result : one_run_stable_lost,
            lap_race,
            hunter,
            {0, hunter ? opponent::anti_uni : opponent::bronsen}};
}

// R-0066: a stunt event (race mode 2): no laps (one line crossing to start, $82:DB96-DBB2
// storing 0 + 1), not a tour race, BRONSEN in the opponent's slot. On HUNTER's tour it keeps
// `$131F`, but not ANTI-UNI ($80:B351-B35F skip $80:B361 in mode 2); HUNTER's stunt event is NEON,
// which runs its lighting in place of the tag effects (R-0068).
// NEON, HUNTER's stunt event: the one track whose one-player race sets `$12D1`.
constexpr std::uint8_t neon_track = 42;

ClassicRaceScenario stunt_scenario(ClassicRaceTrack track, std::uint32_t initialization_frame) {
    const bool hunter = track.index >= first_hunter_track && track.index <= last_hunter_track;
    ClassicRaceScenario scenario{
        track,  initialization_frame,  0, stunt_stable_result, stunt_stable_result, false,
        hunter, {0, opponent::bronsen}};
    scenario.stunt_event = true;
    // $82:D98F-D9A7 and $82:DC22-DC3A: track 42 in one-player play sets `$12D1` (R-0068).
    scenario.neon_lighting = track.index == neon_track;
    return scenario;
}

// The race start's clock ($82:D7FD-D836): the track header's minutes (byte 1) and seconds (byte
// 2, split into tens and units), no tenths. It counts down from there unless it is 0:00, when it
// counts up; every race track's header holds 0:00 and every stunt event's 0:45.
RaceTimerDigits start_clock(std::span<const std::uint8_t> decoded_track, bool stunt_event) {
    constexpr unsigned header_minutes = 1, header_seconds = 2;
    const unsigned seconds = decoded_track[header_seconds];
    const RaceTimerDigits clock{decoded_track[header_minutes],
                                static_cast<std::uint16_t>(seconds / 10U),
                                static_cast<std::uint16_t>(seconds % 10U), 0, 0};
    const bool counts_down = clock.minutes || clock.tens_seconds || clock.seconds;
    if (counts_down != stunt_event)
        throw std::invalid_argument(
            "the track header's clock does not fit the race mode (a stunt event's counts down)");
    return clock;
}

} // namespace

std::uint16_t stunt_qualifying_score(std::span<const std::uint8_t> table, ClassicRaceTrack track,
                                     std::uint8_t best_medal) {
    if (best_medal > best_medal_gold) throw std::invalid_argument("a medal is 0 to 3");
    const unsigned level = best_medal == best_medal_gold ? best_medal - 1U : best_medal;
    const unsigned index = track.index / tracks_per_tour * qualifying_levels + level;
    if (table.size() < 2U * (index + 1U))
        throw std::invalid_argument("the stunt events' qualifying scores are missing (pack v17)");
    return static_cast<std::uint16_t>(content_word(table, 2U * index));
}

ClassicRaceScenario classic_race_scenario(ClassicRaceTrack track) {
    // ZOOM ZOO: M4-16 primary, end-1376, three laps, result stable at load 115.
    // DRAGSTER: end-1328 on the accepted menu path (R-0038), one lap; the
    // stable winner/loser screens follow load 226/242 (R-0012, R-0019).
    if (track == ClassicRaceTrack::ZoomZoo)
        return {track, 1376, 3, lap_race_stable_result, lap_race_stable_result, true, false};
    if (track == ClassicRaceTrack::Dragster)
        return {track, 1328, 1, one_run_stable_won, one_run_stable_lost, false, false};
    // TRACK-BREADTH part 2 (R-0046 observations 6-8): the other race tracks a
    // cold start reaches, as the original sets them up. The race mode ($77:074B)
    // and lap count ($77:0744) are read at each track's initialization boundary;
    // a one-run race stores 0 laps and races one, as DRAGSTER does. The frame is
    // the boundary on the laboratory's menu path (`track_reference`), a label
    // only. The stable result updates follow the race mode's accepted track
    // (DRAGSTER for mode 0, ZOOM ZOO for mode 1); the new tracks' results
    // compared since agree: one-run won and lost, and a lap race (R-0049).
    struct Observed {
        std::uint8_t index;
        std::uint16_t initialization_frame, laps;
        bool lap_race;
    };
    // The HUNTER tour's tracks: $83:CC0B-CC29 sets $1283 = 0x40, $1281 = 0x60 and
    // $1275 = 3 directly when $131F is nonzero (1 on all five HUNTER captures, 0
    // on the other 40) and skips $83:CC59. Every other race track has level 1.
    // LOCKED-TOURS: the race tracks of the five tours a cold start does not
    // list, observed through PICK TOUR unlocked by a preloaded cartridge RAM
    // (track_reference capture --unlock-tours); their frames label that path.
    static constexpr std::array<Observed, 34> observed{
        {{3, 1418, 1, false},  {4, 1419, 3, true},  {10, 1417, 1, false}, {11, 1368, 3, true},
         {13, 1392, 1, false}, {14, 1376, 7, true}, {20, 1360, 1, false}, {21, 1367, 3, true},
         {23, 1386, 1, false}, {24, 1402, 3, true}, {30, 1390, 1, false}, {31, 1397, 3, true},
         {33, 1403, 1, false}, {34, 1407, 5, true}, {5, 1377, 1, false},  {6, 1374, 3, true},
         {8, 1411, 1, false},  {9, 1395, 5, true},  {15, 1389, 1, false}, {16, 1382, 3, true},
         {18, 1386, 1, false}, {19, 1415, 5, true}, {25, 1394, 1, false}, {26, 1384, 3, true},
         {28, 1411, 1, false}, {29, 1404, 3, true}, {35, 1432, 1, false}, {36, 1388, 5, true},
         {38, 1419, 1, false}, {39, 1451, 2, true}, {40, 1464, 1, false}, {41, 1391, 5, true},
         {43, 1428, 1, false}, {44, 1428, 3, true}}};
    for (const auto& o : observed)
        if (o.index == track.index)
            return observed_scenario(track, o.initialization_frame, o.laps, o.lap_race);
    // STUNT-EVENT-RACE (R-0066): the stunt events, place 2 of every tour, their boundaries on
    // the laboratory's menu path (the cold start's four, then LOCKED-TOURS' unlocked path).
    static constexpr std::array<std::pair<std::uint8_t, std::uint16_t>, 9> stunt_events{
        {{2, 1335},
         {12, 1343},
         {22, 1331},
         {32, 1349},
         {7, 1336},
         {17, 1330},
         {27, 1360},
         {37, 1368},
         {42, 1399}}};
    for (const auto& [index, initialization_frame] : stunt_events)
        if (index == track.index) return stunt_scenario(track, initialization_frame);
    throw std::invalid_argument("classic race track has no recovered scenario");
}

bool classic_race_has_scenario(ClassicRaceTrack track) {
    try {
        (void)classic_race_scenario(track);
        return true;
    } catch (const std::invalid_argument&) {
        return false;
    }
}

ClassicRaceScenario classic_race_scenario(ClassicRaceTrack track, RacePairing pairing,
                                          bool tutorial_hints) {
    auto scenario = classic_race_scenario(track);
    // The one-player menus give HUNTER's race tracks ANTI-UNI ($80:B361-B369) and the other
    // races, and every stunt event, BRONSEN, SILVIA or GOLDWYN by the rider's medal
    // ($80:B31F-B346).
    const bool chosen =
        scenario.hunter_tour && !scenario.stunt_event
            ? pairing.opponent == opponent::anti_uni
            : pairing.opponent >= opponent::bronsen && pairing.opponent <= opponent::goldwyn;
    if (pairing.rider >= rider_characters || !chosen)
        throw std::invalid_argument("the one-player menus cannot choose this race's pairing");
    scenario.pairing = pairing;
    scenario.tutorial_hints = tutorial_hints;
    return scenario;
}

ClassicRaceScenario classic_local_race_scenario(ClassicRaceTrack track, RacePairing pairing,
                                                bool tutorial_hints, bool opponent_tutorial_hints) {
    if (pairing.rider >= rider_characters || pairing.opponent >= rider_characters
        || pairing.rider == pairing.opponent)
        throw std::invalid_argument("local race requires two distinct human riders");
    auto scenario = classic_race_scenario(track);
    scenario.pairing = pairing;
    scenario.tutorial_hints = tutorial_hints;
    scenario.opponent_tutorial_hints = opponent_tutorial_hints;
    return scenario;
}

OpponentTier opponent_tier(const ClassicRaceScenario& scenario,
                           std::span<const std::uint8_t> catch_up_by_track) {
    // A stunt event's AI flag `$0C6D` is already clear ($83:CBD8), so $83:CC0B skips the tier,
    // HUNTER's included: level 0, no catch-up, and the non-zero mode's bound 0x48 ($83:CC72).
    if (scenario.stunt_event) return {0, 0, lap_adjustment_limit};
    if (scenario.pairing.opponent < rider_characters)
        return {0, 0, scenario.tour_race ? lap_adjustment_limit : one_run_adjustment_limit};
    if (scenario.hunter_tour) return hunter_tier;
    OpponentTier tier;
    tier.ai_level = static_cast<std::uint8_t>(scenario.pairing.opponent - opponent_level_base);
    if (tier.ai_level == 2 || tier.ai_level == 3) {
        if (catch_up_by_track.size() <= scenario.track.index)
            throw std::invalid_argument("the opponent's catch-up table is missing (pack v21)");
        tier.catch_up =
            static_cast<std::uint8_t>(catch_up_by_track[scenario.track.index]
                                      + (tier.ai_level == 2 ? silvia_catch_up : goldwyn_catch_up));
    }
    const auto base = scenario.tour_race ? lap_adjustment_limit : one_run_adjustment_limit;
    const auto limit = static_cast<std::uint8_t>(base - tier.catch_up);
    tier.adjustment_limit = (limit & 0x80U) ? base : limit;
    return tier;
}

bool classic_race_start_reflected(std::span<const std::uint8_t> decoded_track, unsigned rider) {
    if (rider > 1U) throw std::invalid_argument("classic races have two riders");
    if (decoded_track.size() < 11) throw std::invalid_argument("ZOOM ZOO track header missing");
    return (content_word(decoded_track, 5 + 4 * rider) & 1U) == 0;
}

TrackGeometry track_geometry(std::span<const std::uint8_t> decoded_track) {
    if (decoded_track.size() < 14)
        throw std::invalid_argument("track header lacks its playfield shape");
    // $81:A304-A342 switches on byte 13, a quarter of the column count, into
    // one arm per shape; every arm covers 16,384 cells. The 0x00 and 0x40 arms
    // are observed (DRAGSTER, ZOOM ZOO); the 0x80, 0x20, 0x10 and 0x08 arms
    // store the same fields with the same progression and are read from the
    // static listing (TRACK-BREADTH, R-0046) until a capture executes them.
    // The 0x04 arm (SPRINTER's stunt event, track 37; R-0066) continues the progression and
    // also sets $0FF7, which stops the sampler from clamping a negative y ($81:8A2C-8A2F); its
    // other use, the BG1 map fetch ($81:AD1D-ADA7), is the picture's. Any other value is
    // rejected (the original falls into BRK at $A342).
    switch (decoded_track[13]) {
    case 0x00: return {1024, 0xffff, 0, -0x18, 0x19, -0x31, 0x100};   // $81:A4C1-A4FD, 1,024 x 16
    case 0x80: return {512, 0x7fff, 1, -0x30, 0x32, -0x62, 0x200};    // $81:A483-A4BF, 512 x 32
    case 0x40: return {256, 0x3fff, 2, -0x60, 0x64, -0xc4, 0x400};    // $81:A445-A481, 256 x 64
    case 0x20: return {128, 0x1fff, 3, -0xc0, 0xc8, -0x188, 0x800};   // $81:A406-A444, 128 x 128
    case 0x10: return {64, 0x0fff, 4, -0x180, 0x190, -0x310, 0x1000}; // $81:A3C7-A405, 64 x 256
    case 0x08: return {32, 0x07ff, 5, -0x300, 0x320, -0x620, 0x2000}; // $81:A388-A3C6, 32 x 512
    case 0x04:                                                        // $81:A343-A387, 16 x 1,024
        return {16, 0x03ff, 6, -0x600, 0x640, -0xc40, 0x4000, true};
    default: throw std::invalid_argument("track playfield shape is outside the recovered tracks");
    }
}

// Initializer provenance: $82:D7C6-D7FA clears the working state; D89D-D904
// derives positions from the decompressed track header in 16-world-unit cells.
// DB25-DB7F initializes lap SRAM, DB96-DBBD applies the selected race settings.
ZoomZooState classic_crawler_zoom_zoo_start(const ZoomZooContent& content) {
    return classic_race_start(content, classic_race_scenario(ClassicRaceTrack::ZoomZoo));
}

ZoomZooState classic_crawler_dragster_race_start(const ZoomZooContent& content) {
    return classic_race_start(content, classic_race_scenario(ClassicRaceTrack::Dragster));
}

ZoomZooState classic_race_start(const ZoomZooContent& content,
                                const ClassicRaceScenario& scenario) {
    const auto track = content.movement.sampling.track;
    if (track.size() < 11) throw std::invalid_argument("ZOOM ZOO track header missing");
    ZoomZooState state{};
    state.track = scenario.track;
    state.pairing = scenario.pairing;
    state.opponent_tier = opponent_tier(scenario, content.opponent_catch_up);
    state.native_initialization = state.complete_race = state.sustained = true;
    auto& movement = state.movement;
    movement.frame = scenario.initialization_frame;
    movement.timer = start_clock(track, scenario.stunt_event);
    if (scenario.stunt_event) // $80:99ED
        state.stunt.qualifying_score =
            stunt_qualifying_score(content.qualifying_scores, scenario.track, scenario.best_medal);
    movement.player_input.vertical = movement.player_input.horizontal = direction::neutral;
    movement.countdown = start_countdown; // the timer begins below 68
    movement.rewards.write_cursor = 1;    // $81:C615-C619
    for (unsigned i = 0; i < 2; ++i) {
        auto& rider = movement.riders[i];
        const auto y = content_word(track, 5 + 4 * i);
        rider.motion.x = static_cast<std::uint16_t>(content_word(track, 3 + 4 * i) << 4);
        rider.motion.y = static_cast<std::uint16_t>(y << 4);
        rider.pose.reflected = classic_race_start_reflected(track, i);
        state.reflection[i].base_velocity_cap = base_speed_cap;
        state.race.riders[i].laps_remaining =
            static_cast<std::uint16_t>(scenario.laps + 1U); // Plus the initial line crossing.
        state.race.lap_times[i].fill(no_time);
        state.race.total_times[i] = no_time;
    }
    state.race.camera.x = static_cast<std::uint16_t>((movement.riders[0].motion.x - camera_margin)
                                                     & camera_cell_mask);
    state.race.camera.y = static_cast<std::uint16_t>((movement.riders[0].motion.y - camera_margin)
                                                     & camera_cell_mask);
    state.race.camera.screen_xy = sprites_off_screen;
    state.opponent_retained_oam_x = opponent_hud_oam_x;
    state.start_boost.fill(start_boost);
    state.player_announcements.queue.write_cursor = 1;
    if (content.reward_weights.size() != reward_weight_count)
        throw std::invalid_argument("ZOOM ZOO reward-weight table is missing");
    // $82DB87-DB94 copies the same 26-byte $82D7A4 template into both banks,
    // so the opponent's event-one weight is that content byte too rather than
    // a constant repeated here.
    state.player_announcements.queue.event_one_weight = content.reward_weights[0];
    movement.rewards.event_one_weight = content.reward_weights[0];
    for (auto& weights : state.learned_weights)
        std::copy(content.reward_weights.begin() + 1, content.reward_weights.end(),
                  weights.begin());
    state.player_announcements.hints_active = scenario.tutorial_hints ? 1 : 0; // $82:D94C-D96F
    state.player_announcements.hint_updates = first_hint_phase;
    // $82:D93E-D94C: rider 1's bit is `1 << character` only for characters 1-15; for MIKE the
    // branch at $82:D943 skips the store, so a rider 1 who is MIKE never has hints (R-0082).
    state.opponent_hints.active =
        scenario.opponent_tutorial_hints && scenario.pairing.opponent != 0;
    state.race.checkpoint_seen.fill(checkpoint_unseen);
    return state;
}

void restart_zoom_zoo(ZoomZooState& state, const ZoomZooContent& content) {
    const bool paused_restart = state.pause.selection == 0xffffU && state.pause.released;
    if (!state.native_initialization
        || (state.result_updates != stable_result_updates(state) && !paused_restart))
        throw std::invalid_argument(
            "ZOOM ZOO restart requires a stable result or selected paused restart");
    // $82:D94C reads the rider's tutorial bit again, which $83:CE2C set in the cartridge RAM
    // when the hints ended: the restart's hints run only if they still were.
    const bool hints = state.player_announcements.hints_active != 0;
    // Rider 1's bit likewise ($83:CEB1, R-0082); the demo writes no bit back, so its rider 1's
    // hints run again.
    const bool opponent_hints = state.demo_ai || state.opponent_hints.active;
    // The race's setup reads the same medal again ($80:99ED): the qualifying score stays.
    const auto qualifying_score = state.stunt.qualifying_score;
    auto scenario =
        state.split_screen && !state.demo_ai
            ? classic_local_race_scenario(state.track, state.pairing, hints, opponent_hints)
        : state.demo_ai ? classic_race_scenario(state.track)
                        : classic_race_scenario(state.track, state.pairing, hints);
    if (state.demo_ai) {
        scenario.pairing = state.pairing;
        scenario.tutorial_hints = hints;
    }
    const bool split = state.split_screen;
    const bool demo_ai = state.demo_ai;
    const bool league = state.league_statistics.enabled, versus = state.versus;
    state = classic_race_start(content, scenario);
    state.league_statistics.enabled = league;
    state.versus = versus;
    if (split) initialize_split_cameras(state);
    state.demo_ai = demo_ai;
    state.opponent_hints.active = opponent_hints;
    state.stunt.qualifying_score = qualifying_score;
}

} // namespace unirally
