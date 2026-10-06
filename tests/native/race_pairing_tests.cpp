// ROM-free checks of what a one-player race takes from its rider and opponent (R-0061): the
// pairings the menus can choose, the opponent's tier, the AI's launch by level, the riders'
// voices, the ink's colour math and the tutorial hints the menus pass on.
#include "announcements.hpp"
#include "front_end.hpp"
#include "opponent_ai.hpp"
#include "presentation.hpp"
#include "race_camera.hpp"
#include "reward_queue.hpp"
#include "zoom_zoo_movement.hpp"
#include <array>
#include <cstdint>
#include <stdexcept>

static void require(bool value) {
    if (!value) throw std::runtime_error("race pairing expectation failed");
}
template <class F> static void rejects(F action) {
    bool rejected = false;
    try {
        action();
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected);
}

namespace {
using namespace unirally;

// A sparse stand-in for `$83:C8B3` (00 01 01 02 ...): 1 at ZOOM ZOO (track 1) and 0x20 at
// track 39 (a lap race), 0 elsewhere.
std::array<std::uint8_t, 45> catch_up_table() {
    std::array<std::uint8_t, 45> table{};
    table[1] = 1;
    table[39] = 0x20;
    return table;
}

void pairings_the_menus_choose() {
    const auto dragster = ClassicRaceTrack::Dragster;
    const ClassicRaceTrack hunter{40};
    require(classic_race_scenario(dragster).pairing == RacePairing{0, opponent::bronsen});
    require(classic_race_scenario(hunter).pairing == RacePairing{0, opponent::anti_uni});
    for (const auto opponent : {opponent::bronsen, opponent::silvia, opponent::goldwyn})
        require(classic_race_scenario(dragster, {15, opponent}).pairing.opponent == opponent);
    require(classic_race_scenario(hunter, {3, opponent::anti_uni}).pairing.rider == 3);
    require(!classic_race_scenario(dragster, {1, opponent::bronsen}, false).tutorial_hints);
    rejects([&] { (void)classic_race_scenario(dragster, {16, opponent::bronsen}); });
    rejects([&] { (void)classic_race_scenario(dragster, {0, opponent::anti_uni}); });
    rejects([&] { (void)classic_race_scenario(dragster, {0, 16}); });
    rejects([&] { (void)classic_race_scenario(hunter, {0, opponent::goldwyn}); });
}

void opponent_tiers() {
    const auto table = catch_up_table();
    const auto tier = [&](ClassicRaceTrack track, std::uint8_t opponent) {
        const auto scenario = track.index >= 40 && track.index <= 44
                                ? classic_race_scenario(track)
                                : classic_race_scenario(track, {0, opponent});
        return opponent_tier(scenario, table);
    };
    // BRONSEN: level 1, no catch-up, the race mode's bound (one-run 0x60, lap 0x48).
    require(tier(ClassicRaceTrack::Dragster, opponent::bronsen) == OpponentTier{1, 0, 0x60});
    require(tier(ClassicRaceTrack::ZoomZoo, opponent::bronsen) == OpponentTier{1, 0, 0x48});
    // SILVIA and GOLDWYN, as the captures show them (R-0061).
    require(tier(ClassicRaceTrack::Dragster, opponent::silvia) == OpponentTier{2, 0x20, 0x40});
    require(tier(ClassicRaceTrack::Dragster, opponent::goldwyn) == OpponentTier{3, 0x40, 0x20});
    require(tier(ClassicRaceTrack::ZoomZoo, opponent::silvia) == OpponentTier{2, 0x21, 0x27});
    require(tier(ClassicRaceTrack::ZoomZoo, opponent::goldwyn) == OpponentTier{3, 0x41, 0x07});
    // 0x48 - 0x60 sets bit 7: the bound stays the base.
    require(tier(ClassicRaceTrack{39}, opponent::goldwyn) == OpponentTier{3, 0x60, 0x48});
    // HUNTER's tier is its own, whatever the table.
    require(tier(ClassicRaceTrack{40}, opponent::anti_uni) == OpponentTier{3, 0x40, 0x60});
    // BRONSEN needs no table; SILVIA does.
    require(opponent_tier(classic_race_scenario(ClassicRaceTrack::Dragster), {}).ai_level == 1);
    rejects([] {
        (void)opponent_tier(
            classic_race_scenario(ClassicRaceTrack::Dragster, {0, opponent::silvia}), {});
    });
}

// An opponent rising off the ground at a jump marker, the player `lead` transitions ahead.
ZoomZooState rising_at_jump_marker(std::uint16_t level, int lead, std::uint8_t counter) {
    ZoomZooState state{};
    state.opponent_tier.ai_level = level;
    auto& opponent = state.movement.riders[1];
    opponent.progress.marker_word = 0x2000;
    opponent.contact.unsupported_count = 4;
    opponent.motion.velocity_y = static_cast<std::uint16_t>(-16);
    state.movement.riders[1].progress.transition_count = 20;
    state.movement.riders[0].progress.transition_count = static_cast<std::uint16_t>(20 + lead);
    state.movement.animation_counter = counter;
    state.movement.rewards.feature_total = 1;
    return state;
}

void launches_by_level() {
    const auto launched = [](const ZoomZooState& state) {
        return state.movement.opponent_ai.impulse_countdown != 0;
    };
    const auto suppression = [](const ZoomZooState& state) {
        return state.movement.opponent_ai.suppression_counter;
    };
    // Level 1: the player lets it launch by leading by 3 (30), else no launch (0).
    auto state = rising_at_jump_marker(1, 3, 0);
    (void)update_opponent_controller(state);
    require(launched(state) && suppression(state) == 30);
    state = rising_at_jump_marker(1, 2, 0);
    (void)update_opponent_controller(state);
    require(!launched(state) && suppression(state) == 0);
    // Level 2 ($83:E17D): the lead + 15, launching unless the counter ends in 7.
    state = rising_at_jump_marker(2, 0, 0);
    (void)update_opponent_controller(state);
    require(launched(state) && suppression(state) == 15);
    state = rising_at_jump_marker(2, -9, 6);
    (void)update_opponent_controller(state);
    require(launched(state) && suppression(state) == 6);
    state = rising_at_jump_marker(2, -9, 15);
    (void)update_opponent_controller(state);
    require(!launched(state) && suppression(state) == 6);
    state = rising_at_jump_marker(2, -15, 1);
    (void)update_opponent_controller(state);
    require(launched(state) && suppression(state) == 0);
    state = rising_at_jump_marker(2, -16, 1);
    (void)update_opponent_controller(state);
    require(!launched(state) && suppression(state) == 0);
    // Level 3: always, 60.
    state = rising_at_jump_marker(3, -40, 7);
    (void)update_opponent_controller(state);
    require(launched(state) && suppression(state) == 60);
}

void voices() {
    using announcement::first_voice_of;
    require(first_voice_of(0) == 72 && first_voice_of(1) == 72 && first_voice_of(2) == 88);
    require(first_voice_of(15) == 184 && first_voice_of(opponent::bronsen) == 200);
    require(first_voice_of(opponent::silvia) == 216 && first_voice_of(opponent::goldwyn) == 216);
    require(first_voice_of(opponent::anti_uni) == 232);
}

void ink_colour_math() {
    constexpr std::uint16_t colour_27 = 0x000d;
    // `$82:D4DC`'s MIKE, ANDREW, DAVE and TONY rows.
    constexpr std::array<std::uint8_t, 4> mike{0x2f, 0x40, 0x80, 0x04}, andrew{0x20, 0x40, 0x9f, 0x04},
        dave{0x3f, 0x5f, 0x9f, 0x04}, tony{0x3f, 0x5f, 0x9f, 0x84};
    require(classic_race_ink(colour_27, mike) == 0x001c);
    require(classic_race_ink(colour_27, andrew) == 0x7c0d);
    require(classic_race_ink(colour_27, dave) == 0x7fff);
    require(classic_race_ink(colour_27, tony) == 0x0000);
    rejects([] { (void)classic_race_ink(colour_27, {}); });
}

void menus_pass_the_tutorial_bit() {
    FrontEndState state{};
    state.tour_menu.track = 0;
    state.rider_menu.rider = 1;
    state.now_playing.opponent = opponent::silvia;
    auto scenario = one_player_race_scenario(state);
    require(scenario.pairing == RacePairing{1, opponent::silvia} && scenario.tutorial_hints);
    state.records.tutorial_bits = 1; // MIKE's hints are over, not ANDREW's
    require(one_player_race_scenario(state).tutorial_hints);
    state.records.tutorial_bits = 2;
    require(!one_player_race_scenario(state).tutorial_hints);
}

// A DRAGSTER race start from a synthetic header, as dragster_race_tests builds it.
struct SyntheticRace {
    std::array<std::uint8_t, 14> header{};
    std::array<std::uint8_t, 26> weights{};
    std::array<std::uint8_t, 45> table = catch_up_table();
    ZoomZooContent content{};
    SyntheticRace() {
        header[3] = 0x44;
        header[5] = 0x32;
        header[7] = 0x44;
        header[9] = 0x32;
        weights[0] = 4;
        content.movement.sampling.track = header;
        content.reward_weights = weights;
        content.opponent_catch_up = table;
    }
};

void restarts_keep_the_pairing() {
    SyntheticRace race;
    const auto silvia = classic_race_scenario(ClassicRaceTrack::Dragster, {1, opponent::silvia});
    auto state = classic_race_start(race.content, silvia);
    require(state.pairing == RacePairing{1, opponent::silvia});
    require(state.opponent_tier == OpponentTier{2, 0x20, 0x40});
    require(state.player_announcements.hints_active == 1);
    // A paused restart ($0EF3 = -1, released): the hints run again only if they still were.
    state.pause.selection = 0xffff;
    state.pause.released = 1;
    auto again = state;
    restart_zoom_zoo(again, race.content);
    require(again.player_announcements.hints_active == 1 && again.pairing == state.pairing);
    // R-0084: a race restarted on its own keeps `$77:10B1`; the menus' next
    // race counts it.
    require(state.race_counter == 1 && again.race_counter == 1);
    auto later = state;
    later.race_counter = 5;
    restart_zoom_zoo(later, race.content);
    require(later.race_counter == 5);
    state.player_announcements.hints_active = 0;
    restart_zoom_zoo(state, race.content);
    require(state.player_announcements.hints_active == 0);
    require(state.pairing == RacePairing{1, opponent::silvia});
    require(state.opponent_tier == OpponentTier{2, 0x20, 0x40});
    // The hints-off start itself.
    const auto off = classic_race_start(
        race.content, classic_race_scenario(ClassicRaceTrack::Dragster, {0, opponent::bronsen}, false));
    require(off.player_announcements.hints_active == 0);
}

void paired_states_read_back() {
    SyntheticRace race;
    const auto scenario = classic_race_scenario(ClassicRaceTrack::Dragster, {0, opponent::silvia});
    auto state = classic_race_start(race.content, scenario);
    state.movement.opponent_ai.suppression_counter = 15; // a level-2 word
    const auto bytes = serialize_zoom_zoo(state);
    const auto read = deserialize_zoom_zoo(bytes, {0, opponent::silvia}, true, race.table);
    require(serialize_zoom_zoo(read) == bytes);
    require(read.pairing == scenario.pairing && read.opponent_tier == state.opponent_tier);
    // SILVIA's tier needs the table; without the pairing the word is not BRONSEN's.
    rejects([&] { (void)deserialize_zoom_zoo(bytes, {0, opponent::silvia}, true, {}); });
    rejects([&] { (void)deserialize_zoom_zoo(bytes); });
    // R-0084: the one-player layout carries no race counter; the menus' is
    // taken.
    state.race_counter = 2;
    const auto second = serialize_zoom_zoo(state);
    require(second == bytes);
    require(
        deserialize_zoom_zoo(second, {0, opponent::silvia}, true, race.table, 2)
            .race_counter == 2);
    require(read.race_counter == 1);
    rejects([&] {
      (void)deserialize_zoom_zoo(second, {0, opponent::silvia}, true,
                                 race.table, 6);
    });
    // BRONSEN's word is 0 or 30.
    state.pairing = {0, opponent::bronsen};
    state.opponent_tier = opponent_tier(classic_race_scenario(ClassicRaceTrack::Dragster), {});
    state.movement.opponent_ai.suppression_counter = 30;
    require(deserialize_zoom_zoo(serialize_zoom_zoo(state)).opponent_tier.ai_level == 1);
    // A hints-off start reads back only as one.
    auto off = classic_race_start(
        race.content, classic_race_scenario(ClassicRaceTrack::Dragster, {0, opponent::bronsen}, false));
    const auto off_bytes = serialize_zoom_zoo(off);
    require(!deserialize_zoom_zoo(off_bytes, {0, opponent::bronsen}, false, {})
                 .player_announcements.hints_active);
    rejects([&] { (void)deserialize_zoom_zoo(off_bytes); });
}

} // namespace

// R-0082: in a two-human race rider 1 has tutorial hints of its own, from its own bit, unless
// it is MIKE; its groups come every 300 updates from 0, thirty updates after the player's, into
// its own queue. A split state keeps the flag in its trailer and recovers the count from the clock.
void rider_one_hints() {
    SyntheticRace race;
    const auto dragster = ClassicRaceTrack::Dragster;
    auto state = classic_race_start(race.content, classic_local_race_scenario(dragster, {1, 2}, true, true));
    initialize_split_cameras(state);
    require(state.opponent_hints.active);
    require(!classic_race_start(race.content, classic_local_race_scenario(dragster, {1, 0}, true, true))
                 .opponent_hints.active);
    require(!classic_race_start(race.content, classic_local_race_scenario(dragster, {1, 2}, true, false))
                 .opponent_hints.active);
    for (unsigned update = 0; update < 270; ++update) update_tutorial_hints(state);
    require(state.player_announcements.queue.entries[1] == announcement::first_hint + 4
            && state.movement.rewards.write_cursor == 1);
    for (unsigned update = 270; update < 300; ++update) update_tutorial_hints(state);
    require(state.movement.rewards.write_cursor == 5 && state.opponent_hints.group == 1);
    for (unsigned slot = 1; slot < 5; ++slot)
        require(state.movement.rewards.entries[slot] == announcement::first_hint + 3 + slot);
    // Queueing ends them on a scoring event or a voice from 150, never on a hint or a lower voice.
    require(announcement::ends_opponent_hints(1) && announcement::ends_opponent_hints(21)
            && announcement::ends_opponent_hints(150) && announcement::ends_opponent_hints(255));
    require(!announcement::ends_opponent_hints(22) && !announcement::ends_opponent_hints(40)
            && !announcement::ends_opponent_hints(149));
}

// R-0083: the end of rider 1's scripted glance clears its idle latch
// ($82:87AD), which the race update's look step writes. Riders out of each
// other's view and both contacts skipped (the corkscrew's hold), so the look
// reads its stored head points and zeroed look tables.
void glance_end_clears_the_latch() {
  SyntheticRace race;
  std::array<std::uint8_t, 594> tables{};
  race.content.look_tables = tables;
  auto state = classic_race_start(
      race.content, classic_local_race_scenario(ClassicRaceTrack::Dragster,
                                                {1, 2}, true, false));
  initialize_split_cameras(state);
  state.movement.contact_phase = 0; // rider 1's look step
  state.movement.riders[1].motion.y =
      static_cast<std::uint16_t>(state.movement.riders[0].motion.y + 0x400U);
  for (auto &tiles : state.special_tiles)
    tiles.physics_hold = 8;
  state.look.head_offsets = {RiderHeadOffset{}, RiderHeadOffset{}};
  auto &look = state.look.riders[1];
  look.head = 5;
  look.glance_timer = 0xffc0; // resting
  look.sequence_number = 1;
  look.sequence_cursor = look.sequence_end = 130;
  look.sequence_delay = 2;
  state.movement.riders[1].idle_pose.cycle_latched = 1;
  auto running = state;
  advance_rider_look(running, race.content);
  require(running.movement.riders[1].idle_pose.cycle_latched == 1 &&
          running.look.riders[1].sequence_cursor == 130);
  look.sequence_delay = 1;
  advance_rider_look(state, race.content);
  require(state.movement.riders[1].idle_pose.cycle_latched == 0 &&
          state.look.riders[1].sequence_cursor == 0 &&
          state.look.riders[1].sequence_end == 0);
}

int main() {
    pairings_the_menus_choose();
    opponent_tiers();
    launches_by_level();
    voices();
    ink_colour_math();
    menus_pass_the_tutorial_bit();
    restarts_keep_the_pairing();
    paired_states_read_back();
    rider_one_hints();
    glance_end_clears_the_latch();
    return 0;
}
