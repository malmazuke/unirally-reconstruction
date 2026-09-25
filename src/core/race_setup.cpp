// Race scenarios by track, the race start and a restart.

#include "word_arithmetic.hpp"
#include "zoom_zoo_movement.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <stdexcept>

namespace unirally {

ClassicRaceScenario classic_race_scenario(ClassicRaceTrack track) {
    // ZOOM ZOO: M4-16 primary, end-1376, three laps, result stable at load 115.
    // DRAGSTER: end-1328 on the accepted menu path (R-0038), one lap; the
    // stable winner/loser screens follow load 226/242 (R-0012, R-0019).
    if (track == ClassicRaceTrack::ZoomZoo) return {track, 1376, 3, 115, 115, true};
    if (track == ClassicRaceTrack::Dragster) return {track, 1328, 1, 226, 242, false};
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
    const bool hunter = track.index >= 40 && track.index <= 44;
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
            return o.lap_race ? ClassicRaceScenario{track,
                                                    o.initialization_frame,
                                                    o.laps,
                                                    115,
                                                    115,
                                                    true,
                                                    static_cast<std::uint16_t>(hunter ? 3 : 1),
                                                    static_cast<std::uint16_t>(hunter ? 96 : 0),
                                                    static_cast<std::uint16_t>(hunter ? 64 : 0),
                                                    hunter,
                                                    static_cast<std::uint16_t>(hunter ? 20 : 17)}
                              : ClassicRaceScenario{track,
                                                    o.initialization_frame,
                                                    o.laps,
                                                    226,
                                                    242,
                                                    false,
                                                    static_cast<std::uint16_t>(hunter ? 3 : 1),
                                                    static_cast<std::uint16_t>(hunter ? 96 : 0),
                                                    static_cast<std::uint16_t>(hunter ? 64 : 0),
                                                    hunter,
                                                    static_cast<std::uint16_t>(hunter ? 20 : 17)};
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

std::uint16_t race_adjustment_limit(const ClassicRaceScenario& scenario) {
    if (scenario.adjustment_limit) return scenario.adjustment_limit;
    return static_cast<std::uint16_t>(scenario.tour_race ? 0x48U : 0x60U);
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
    // The 0x04 arm also sets $0FF7, which changes the sampler ($81:8A2C) and
    // the BG1 map fetch ($81:AD1D-ADA7); that is not recovered, so it is
    // rejected, as is any other value (the original falls into BRK at $A342).
    switch (decoded_track[13]) {
    case 0x00: return {1024, 0xffff, 0, -0x18, 0x19, -0x31, 0x100};   // $81:A4C1-A4FD, 1,024 x 16
    case 0x80: return {512, 0x7fff, 1, -0x30, 0x32, -0x62, 0x200};    // $81:A483-A4BF, 512 x 32
    case 0x40: return {256, 0x3fff, 2, -0x60, 0x64, -0xc4, 0x400};    // $81:A445-A481, 256 x 64
    case 0x20: return {128, 0x1fff, 3, -0xc0, 0xc8, -0x188, 0x800};   // $81:A406-A444, 128 x 128
    case 0x10: return {64, 0x0fff, 4, -0x180, 0x190, -0x310, 0x1000}; // $81:A3C7-A405, 64 x 256
    case 0x08: return {32, 0x07ff, 5, -0x300, 0x320, -0x620, 0x2000}; // $81:A388-A3C6, 32 x 512
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
    state.native_initialization = state.complete_race = state.sustained = true;
    auto& movement = state.movement;
    movement.frame = scenario.initialization_frame;
    movement.player_input.vertical = movement.player_input.horizontal = 1;
    movement.countdown = 270; // $82:D841-D844; timer begins below 68 after countdown publication.
    movement.rewards.write_cursor = 1; // $81:C615-C619.
    for (unsigned i = 0; i < 2; ++i) {
        auto& rider = movement.riders[i];
        const auto y = content_word(track, 5 + 4 * i);
        rider.motion.x = static_cast<std::uint16_t>(content_word(track, 3 + 4 * i) << 4);
        rider.motion.y = static_cast<std::uint16_t>(y << 4);
        rider.pose.reflected = classic_race_start_reflected(track, i);
        state.reflection[i].base_velocity_cap = 448;
        state.race.riders[i].laps_remaining =
            static_cast<std::uint16_t>(scenario.laps + 1U); // Plus the initial line crossing.
        state.race.lap_times[i].fill(60000);
        state.race.total_times[i] = 60000;
    }
    state.race.camera.x =
        static_cast<std::uint16_t>((movement.riders[0].motion.x - 256U) & 0xfff0U);
    state.race.camera.y =
        static_cast<std::uint16_t>((movement.riders[0].motion.y - 256U) & 0xfff0U);
    state.race.camera.screen_xy = 0xe0e0; // $82:D724-D731 OAM initialization.
    state.opponent_retained_oam_x = 0x65; // Explicit $82:D76D-D76F HUD OAM default.
    state.start_boost.fill(384);
    state.player_announcements.queue.write_cursor = 1;
    if (content.reward_weights.size() != 26)
        throw std::invalid_argument("ZOOM ZOO reward-weight table is missing");
    // $82DB87-DB94 copies the same 26-byte $82D7A4 template into both banks,
    // so the opponent's event-one weight is that content byte too rather than
    // a constant repeated here.
    state.player_announcements.queue.event_one_weight = content.reward_weights[0];
    movement.rewards.event_one_weight = content.reward_weights[0];
    for (auto& weights : state.learned_weights)
        std::copy(content.reward_weights.begin() + 1, content.reward_weights.end(),
                  weights.begin());
    state.player_announcements.hints_active = 1;  // $82D95C fresh scenario tutorial bit.
    state.player_announcements.hint_updates = 30; // $82D972-D975.
    state.race.checkpoint_seen.fill(255);         // $81:CD2A.
    return state;
}

void restart_zoom_zoo(ZoomZooState& state, const ZoomZooContent& content) {
    const bool paused_restart = state.pause.selection == 0xffffU && state.pause.released;
    if (!state.native_initialization
        || (state.result_updates != stable_result_updates(state) && !paused_restart))
        throw std::invalid_argument(
            "ZOOM ZOO restart requires a stable result or selected paused restart");
    state = classic_race_start(content, classic_race_scenario(state.track));
}

} // namespace unirally
