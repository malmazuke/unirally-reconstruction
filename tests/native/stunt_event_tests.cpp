// ROM-free checks for the stunt event (race mode 2, R-0066): the scenarios, the header's clock
// counting down and the finish it allows, the finish's settling and display, the trick tallies,
// the caption by score, the qualifying score and the stunt event's state (URTRnn07).
#include "announcements.hpp"
#include "front_end.hpp"
#include "stunt_event.hpp"
#include "zoom_zoo_movement.hpp"

#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using namespace unirally;

void require(bool value, const char* what) {
    if (!value) throw std::runtime_error(std::string("stunt event expectation failed: ") + what);
}
template <class F> void rejects(F action, const char* what) {
    bool rejected = false;
    try {
        action();
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected, what);
}

constexpr std::array<std::uint8_t, 9> stunt_tracks{2, 7, 12, 17, 22, 27, 32, 37, 42};

// A synthetic stunt event: a track header whose clock reads 0:45 (byte 2 = 45), the riders'
// start cells, and a qualifying table whose word k is 50 + k.
struct SyntheticStunt {
    std::array<std::uint8_t, 14> header{};
    std::array<std::uint8_t, 26> weights{};
    std::array<std::uint8_t, 54> qualifying{};
    ZoomZooContent content{};
    SyntheticStunt() {
        header[2] = 45;
        header[3] = 0x44;
        header[5] = 0x32;
        header[7] = 0x44;
        header[9] = 0x32;
        weights[0] = 4;
        for (unsigned k = 0; k < qualifying.size() / 2; ++k)
            qualifying[2 * k] = static_cast<std::uint8_t>(50 + k);
        content.movement.sampling.track = header;
        content.reward_weights = weights;
        content.qualifying_scores = qualifying;
    }
};

void scenarios() {
    const std::array<std::uint16_t, 9> boundaries{1335, 1336, 1343, 1330, 1331,
                                                  1360, 1349, 1368, 1399};
    for (unsigned k = 0; k < stunt_tracks.size(); ++k) {
        const auto scenario = classic_race_scenario(ClassicRaceTrack{stunt_tracks[k]});
        require(scenario.stunt_event && !scenario.tour_race && scenario.laps == 0, "mode 2");
        require(scenario.initialization_frame == boundaries[k], "boundary");
        require(scenario.stable_result_won == 105 && scenario.stable_result_lost == 105, "stable");
        require(scenario.pairing == RacePairing{0, opponent::bronsen}, "BRONSEN in the slot");
        require(scenario.hunter_tour == (stunt_tracks[k] == 42), "HUNTER's $131F");
        // $83:CC0B skips the tier: level 0, no catch-up, the non-zero mode's bound.
        require(opponent_tier(scenario, {}) == OpponentTier{0, 0, 0x48}, "no tier");
    }
    // HUNTER's stunt event takes the medal's opponent, not ANTI-UNI ($80:B351-B35F).
    const ClassicRaceTrack neon{42};
    require(classic_race_scenario(neon, {0, opponent::goldwyn}).pairing.opponent
                == opponent::goldwyn,
            "GOLDWYN on track 42");
    rejects([&] { (void)classic_race_scenario(neon, {0, opponent::anti_uni}); }, "no ANTI-UNI");
    rejects([&] { (void)classic_race_scenario(ClassicRaceTrack{41}, {0, opponent::goldwyn}); },
            "ANTI-UNI on HUNTER's races");
    require(!classic_race_scenario(ClassicRaceTrack{13}).stunt_event, "a race");
}

void qualifying_scores() {
    const SyntheticStunt stunt;
    // $83:9EEB: (3 * tour + medal, gold counting as silver) words.
    require(stunt_qualifying_score(stunt.qualifying, ClassicRaceTrack{2}, 0) == 50, "CRAWLER");
    require(stunt_qualifying_score(stunt.qualifying, ClassicRaceTrack{22}, 1) == 50 + 13, "bronze");
    require(stunt_qualifying_score(stunt.qualifying, ClassicRaceTrack{42}, 2) == 50 + 26, "silver");
    require(stunt_qualifying_score(stunt.qualifying, ClassicRaceTrack{42}, 3) == 50 + 26, "gold");
    rejects([&] { (void)stunt_qualifying_score(stunt.qualifying, ClassicRaceTrack{2}, 4); },
            "a medal past gold");
    rejects([&] { (void)stunt_qualifying_score({}, ClassicRaceTrack{2}, 0); }, "no table");
}

void the_start() {
    SyntheticStunt stunt;
    auto scenario = classic_race_scenario(ClassicRaceTrack{12});
    scenario.best_medal = 1;
    const auto state = classic_race_start(stunt.content, scenario);
    const auto& clock = state.movement.timer;
    require(clock.minutes == 0 && clock.tens_seconds == 4 && clock.seconds == 5 && !clock.tenths,
            "0:45.0");
    require(state.race.riders[0].laps_remaining == 1 && state.race.riders[1].laps_remaining == 1,
            "one line crossing");
    require(state.stunt.qualifying_score == 50 + 7, "JUMPS with a bronze");
    // The header's clock and the race mode must agree.
    rejects([&] { (void)classic_race_start(stunt.content, classic_race_scenario(ClassicRaceTrack{13})); },
            "a race's clock at 0:45");
    stunt.header[2] = 0;
    rejects([&] { (void)classic_race_start(stunt.content, scenario); }, "a stunt clock at 0:00");
}

void the_clock() {
    const SyntheticStunt stunt;
    auto state = classic_race_start(stunt.content, classic_race_scenario(ClassicRaceTrack{2}));
    update_stunt_clock(state, false);
    require(state.movement.timer.subframe == 0, "not before the countdown's 68");
    for (int k = 0; k < 5; ++k) update_stunt_clock(state, true);
    const auto& clock = state.movement.timer;
    require(clock.tens_seconds == 4 && clock.seconds == 4 && clock.tenths == 9, "0:44.9");
    state.movement.timer = {0, 0, 0, 0, 4};
    state.movement.riders[1].contact.unsupported_count = 2; // in the air
    state.movement.riders[0].motion.velocity_y = 0xff00;    // -256: not falling fast
    state.movement.riders[0].contact.unsupported_count = 1;
    update_stunt_clock(state, true);
    require(clock.minutes == 0 && clock.tens_seconds == 5 && clock.seconds == 9 && clock.tenths == 9
                && !clock.subframe,
            "the tick after 0:00.0 leaves 0:59.9");
    require(state.stunt.clock_stopped == 1, "stopped");
    require(state.race.riders[0].finished && !state.race.riders[1].finished, "only the supported");
    update_stunt_clock(state, true);
    require(clock.tenths == 9 && !clock.subframe, "a stopped clock stays");
    state.movement.riders[1].contact.unsupported_count = 1;
    state.movement.riders[1].motion.velocity_y = 0xfeff; // -257: falling fast
    update_stunt_clock(state, true);
    require(!state.race.riders[1].finished, "not while falling fast");
    state.movement.riders[1].motion.velocity_y = 0x40;
    update_stunt_clock(state, true);
    require(state.race.riders[1].finished && state.race.total_times[1] == no_time, "no time");
}

void the_finish() {
    const SyntheticStunt stunt;
    auto state = classic_race_start(stunt.content, classic_race_scenario(ClassicRaceTrack{2}));
    state.race.riders[0].finished = state.race.riders[1].finished = 1;
    state.movement.riders[0].contact.unsupported_count = 3; // still in the air
    state.player_announcements.hints_active = 1;
    require(!update_stunt_finish(state), "waits for the landing");
    require(!state.stunt.settled[0] && state.stunt.settled[1] && state.player_announcements.hints_active,
            "the standing opponent settles");
    require(!update_stunt_finish(state) && state.movement.riders[1].motion.velocity_y == 0x80,
            "a settled rider is held at 128");
    state.movement.riders[0].contact.unsupported_count = 0;
    auto& queue = state.player_announcements.queue;
    queue.entries[1] = announcement::roll;
    queue.write_cursor = 2; // one caption still to show
    require(!update_stunt_finish(state) && !state.player_announcements.hints_active,
            "both stand: the hints end");
    require(!state.stunt.finish_display, "the queue is not yet empty");
    queue.read_cursor = 1;
    require(!update_stunt_finish(state) && state.stunt.finish_display, "starts on the next update");
    require(update_stunt_finish(state), "then runs");
}

void tallies_and_captions() {
    StuntEvent stunt;
    tally_stunt_trick(stunt, 0, 4);   // roll x1
    tally_stunt_trick(stunt, 0, 0);   // a weight of 0 still counts
    tally_stunt_trick(stunt, 34, 12); // tabletop: mega x2
    tally_stunt_trick(stunt, 30, 1);  // z flip city: z flip x4
    require(stunt.tallies[trick_family::roll][0] == TrickTally{2, 4}, "roll x1");
    require(stunt.tallies[trick_family::mega][1] == TrickTally{1, 12}, "mega x2");
    require(stunt.tallies[trick_family::z_flip][3] == TrickTally{1, 1}, "z flip x4");
    for (int k = 0; k < 255; ++k) tally_stunt_trick(stunt, 8, 1);
    require(stunt.tallies[trick_family::flip][0] == TrickTally{255, 255}, "flip x1");
    tally_stunt_trick(stunt, 8, 1);
    require(stunt.tallies[trick_family::flip][0] == TrickTally{0, 256}, "a one-byte count");
    rejects([&] { tally_stunt_trick(stunt, 40, 1); }, "past the table");
    rejects([&] { tally_stunt_trick(stunt, 3, 1); }, "an odd class");
    // $83:E940-E957: a 16-bit difference's sign.
    require(stunt_finish_announcement(68, 68) == announcement::draw, "equal draws");
    require(stunt_finish_announcement(49, 68) == announcement::loser, "below loses");
    require(stunt_finish_announcement(121, 90) == announcement::winner, "above wins");
    require(stunt_finish_announcement(0x9000, 68) == announcement::loser, "the sign bit");
}

// The stunt event's state carries its words (URTRnn07) and round-trips; the guards refuse what
// the original cannot reach.
void the_state() {
    const SyntheticStunt stunt;
    auto state = classic_race_start(stunt.content, classic_race_scenario(ClassicRaceTrack{7}));
    const auto bytes = serialize_zoom_zoo(state);
    const std::array<std::uint8_t, 8> magic{'U', 'R', 'T', 'R', '0', '7', '0', '7'};
    require(bytes.size() == 1006 && std::equal(magic.begin(), magic.end(), bytes.begin()),
            "URTR0707, 1,006 bytes");
    require(classic_race_state_magic(ClassicRaceTrack{7}) == magic, "the identity");
    const auto restored = deserialize_zoom_zoo(bytes);
    require(serialize_zoom_zoo(restored) == bytes && restored.stunt == state.stunt, "round trip");
    validate_zoom_zoo_content_state(restored, stunt.content);
    auto forged = restored;
    forged.stunt.qualifying_score = 49;
    rejects([&] { validate_zoom_zoo_content_state(forged, stunt.content); }, "another tour's score");
    constexpr std::size_t stunt_words = 916, tallies = stunt_words + 10;
    auto padded = bytes;
    padded[tallies + 1] = 1;
    rejects([&] { (void)deserialize_zoom_zoo(padded); }, "a two-byte count");
    auto points = bytes;
    points[tallies + 2] = 4;
    rejects([&] { (void)deserialize_zoom_zoo(points); }, "points that are not the score");
    auto stopped = bytes;
    stopped[stunt_words + 2] = 1;
    rejects([&] { (void)deserialize_zoom_zoo(stopped); }, "a stop before the clock's end");
    auto race = bytes;
    race[4] = '1';
    race[5] = '3';
    rejects([&] { (void)deserialize_zoom_zoo(race); }, "a race track's name");
    auto short_form = std::vector<std::uint8_t>(bytes.begin(), bytes.begin() + stunt_words);
    short_form[7] = '6';
    rejects([&] { (void)deserialize_zoom_zoo(short_form); }, "the race layout");
    auto on_a_race = classic_race_start(stunt.content, classic_race_scenario(ClassicRaceTrack{7}));
    on_a_race.track = ClassicRaceTrack{13};
    on_a_race.stunt.qualifying_score = 50;
    rejects([&] { (void)serialize_zoom_zoo(on_a_race); }, "a race keeps no stunt words");
}

void the_score_decides() {
    const SyntheticStunt stunt;
    auto state = classic_race_start(stunt.content, classic_race_scenario(ClassicRaceTrack{2}));
    state.player_announcements.queue.feature_total = 49;
    require(!classic_race_player_won(state), "49 of 50 loses");
    state.player_announcements.queue.feature_total = 50;
    require(classic_race_player_won(state), "$83:88E1 counts equal as a win");
}

void the_menus_medal() {
    FrontEndState state{};
    state.tour_menu.track = 22;
    state.tour_menu.tour = 4;
    state.rider_menu.rider = 3;
    state.now_playing.opponent = opponent::silvia;
    state.records.medals[4 * 16 + 3] = 3;
    const auto scenario = one_player_race_scenario(state);
    require(scenario.stunt_event && scenario.best_medal == 3, "the rider's best medal on the tour");
}

} // namespace

int main() {
    try {
        scenarios();
        qualifying_scores();
        the_start();
        the_clock();
        the_finish();
        tallies_and_captions();
        the_state();
        the_score_decides();
        the_menus_medal();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
