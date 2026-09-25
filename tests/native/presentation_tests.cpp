#include "movement.hpp"
#include "presentation.hpp"
#include "result_screen.hpp"
#include "zoom_zoo_movement.hpp"
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
namespace {
int require_count = 0;
void require(bool v) {
  ++require_count;
  if (!v)
    throw std::runtime_error("presentation assertion failed at require #" +
                             std::to_string(require_count));
}
} // namespace
int main() {
  // Primary-source examples from bsnes' mode-3 direct-colour and add/halve
  // formulas. These guard the bit placement and per-channel carry semantics.
  require(unirally::snes_direct_colour(0x00, 0x00) == 0x0000);
  require(unirally::snes_direct_colour(0xff, 0x07) == 0x73de);
  require(unirally::snes_direct_colour(0x92, 0x05) == 0x510a);
  require(unirally::snes_add_colour(0x001f, 0x001f, false) == 0x001f);
  require(unirally::snes_add_colour(0x001f, 0x0001, true) == 0x0010);
  require(unirally::snes_add_colour(0x7c00, 0x03e0, false) == 0x7fe0);

  // $0EFB against the lap the original's HUD shows one frame later, read off
  // the frozen primary timeline's frames 1450, 1676, 3209 and 4841. Zero is a
  // clamp only: once finished the HUD shows FINISH instead of a lap.
  require(unirally::classic_hud_lap(4, 3) == 0);
  require(unirally::classic_hud_lap(3, 3) == 1);
  require(unirally::classic_hud_lap(2, 3) == 2);
  require(unirally::classic_hud_lap(1, 3) == 3);
  require(unirally::classic_hud_lap(0, 3) == 3);
  // DRAGSTER's one lap: the initial crossing, then the lap, which holds.
  require(unirally::classic_hud_lap(2, 1) == 0);
  require(unirally::classic_hud_lap(1, 1) == 1);
  require(unirally::classic_hud_lap(0, 1) == 1);
  {
    const auto zoom = unirally::classic_race_scenario(unirally::ClassicRaceTrack::ZoomZoo);
    const auto dragster = unirally::classic_race_scenario(unirally::ClassicRaceTrack::Dragster);
    unirally::ZoomZooState racing{};
    racing.race.riders[0].laps_remaining = 2;
    racing.movement.timer.seconds = 3;
    racing.movement.timer.tenths = 2;
    racing.movement.timer.subframe = 4;
    auto hud = unirally::classic_race_hud_text(racing, zoom, std::nullopt);
    // The corner clock shows tenths and no more; the subframe never reaches it.
    require(hud.left == "2/3" && hud.left_column == 2 && hud.clock == "0:03:2");
    require(hud.player_cells.empty() && hud.opponent_cells.empty());
    // The count is right-aligned on column 2, so a second digit takes column 1
    // ($81:EC94-$81:ECBC). Neither track reaches one.
    {
      auto twelve = zoom;
      twelve.laps = 12;
      racing.race.riders[0].laps_remaining = 3;
      const auto wide = unirally::classic_race_hud_text(racing, twelve, std::nullopt);
      require(wide.left == "10/12" && wide.left_column == 1);
      racing.race.riders[0].laps_remaining = 2;
    }
    // A race whose mode is not the tour race shows the word instead of a count
    // ($81:D6E8, DRAGSTER originals 1380-3213).
    racing.track = unirally::ClassicRaceTrack::Dragster;
    hud = unirally::classic_race_hud_text(racing, dragster, std::nullopt);
    require(hud.left == "race" && hud.left_column == 2);

    // The finish sequence is the redraw queue's output, not a set of offsets
    // from the finish. `ClassicRaceHudClock` follows the queue: each update it
    // services the first pending field and stops, in the order left field,
    // clock, player's time, opponent's time. Reviews 1, 2, 3 and 4 each
    // returned this task over one consequence of that, so the checks below
    // drive the queue update by update rather than asserting a delay.
    {
      const auto run = [](unirally::ClassicRaceHudClock &queue,
                          const unirally::ZoomZooState &before,
                          const unirally::ZoomZooState &after) {
        queue.observe_update(before, after);
        return queue.published();
      };
      // What `published()` reports lags one update, like the rider overlays,
      // so a fresh queue needs two idle updates before its first picture.
      const auto prime = [&run](unirally::ClassicRaceHudClock &queue,
                                const unirally::ZoomZooState &state) {
        run(queue, state, state);
        return run(queue, state, state);
      };
      // ZOOM ZOO, nothing competing: the player finishes on update 6484 and
      // the opponent on 6488, and the original's pictures are `finish` at
      // 6485, the blanked clock at 6486, the player's time at 6487 and the
      // opponent's at 6490 (measured on consecutive originals 6480-6500).
      unirally::ClassicRaceHudClock queue;
      unirally::ZoomZooState lap{};
      lap.race.riders[0].laps_remaining = 1;
      lap.race.riders[1].laps_remaining = 1;
      lap.movement.timer.minutes = 1;
      lap.movement.timer.tens_seconds = 3;
      lap.movement.timer.seconds = 8;
      require(prime(queue, lap).clock == std::optional<std::string>("1:38:0"));
      auto finishing = lap;
      finishing.race.riders[0].laps_remaining = 0;      // update 6484
      finishing.race.riders[0].finished = 1;
      finishing.race.total_times = {9802, 60000};
      // The crossing sets the display countdown and stores the crossing
      // digits, which is what requests the centred field (R-0044).
      finishing.race.riders[0].checkpoint_display_countdown = 120;
      finishing.race.riders[0].next_checkpoint = 1;
      finishing.race.riders[0].time_digits = {1, 3, 8, 0, 2};
      // `published()` reports the picture for the update before the pair just
      // observed, so each call below names the picture it returns.
      auto picture = run(queue, lap, finishing);        // 6484
      require(picture.clock == std::optional<std::string>("1:38:0") && !picture.clock_blanked);
      picture = run(queue, finishing, finishing);       // 6485: `finish`, clock intact
      require(picture.clock == std::optional<std::string>("1:38:0") && !picture.clock_blanked);
      picture = run(queue, finishing, finishing);       // 6486: the clock is blanked
      require(picture.clock_blanked && !picture.player_cells);
      picture = run(queue, finishing, finishing);       // 6487: the player's time
      require(picture.player_cells == std::optional<std::string>("1:38:02") && !picture.opponent_cells);
      auto opponent = finishing;
      opponent.race.riders[1].laps_remaining = 0;       // the opponent finishes
      opponent.race.riders[1].finished = 1;
      opponent.race.total_times[1] = 9810;
      opponent.race.riders[1].checkpoint_display_countdown = 120;
      opponent.race.riders[1].next_checkpoint = 1;
      opponent.race.riders[1].time_digits = {1, 3, 8, 1, 0};
      require(!run(queue, finishing, opponent).opponent_cells);   // 6488
      require(!run(queue, opponent, opponent).opponent_cells);    // 6489: `finish` again
      require(run(queue, opponent, opponent).opponent_cells == std::optional<std::string>("1:38:10")); // 6490
      // 118 updates after the finish the countdown reaches 2 and asks for a
      // blank, which a finished rider's handler skips without spending the
      // update ($81:EDE9): the finish time stands.
      auto late = opponent;
      late.race.riders[0].checkpoint_display_countdown = 2;
      late.race.riders[1].checkpoint_display_countdown = 2;
      run(queue, opponent, late);
      picture = run(queue, late, late);
      require(picture.player_cells == std::optional<std::string>("1:38:02") &&
              picture.opponent_cells == std::optional<std::string>("1:38:10"));

      // The same sequence with the opponent's counter stepping on the update
      // right after the player finishes: `$0D17` is set again while `$0EFB` is
      // already zero, so `finish` is written twice and everything behind it
      // waits an update. Review 4 B1 measured this on two DRAGSTER races, at
      // pictures 3215-3217, where the original still shows the clock.
      unirally::ClassicRaceHudClock competing;
      unirally::ZoomZooState sprint{};
      sprint.track = unirally::ClassicRaceTrack::Dragster;
      sprint.race.riders[0].laps_remaining = 1;
      sprint.race.riders[1].laps_remaining = 1;
      sprint.movement.timer.tens_seconds = 3;
      sprint.movement.timer.seconds = 3;
      sprint.movement.timer.tenths = 5;
      prime(competing, sprint);
      auto done = sprint;
      done.race.riders[0].laps_remaining = 0;
      done.race.riders[0].finished = 1;
      done.race.total_times = {3350, 60000};
      done.race.riders[0].checkpoint_display_countdown = 120;
      done.race.riders[0].next_checkpoint = 1;
      done.race.riders[0].time_digits = {0, 3, 3, 5, 0};
      require(run(competing, sprint, done).clock == std::optional<std::string>("0:33:5"));
      auto crossed = done;
      crossed.race.riders[1].laps_remaining = 0;   // the opponent crosses here
      crossed.race.riders[1].finished = 1;
      crossed.race.total_times[1] = 3358;
      crossed.race.riders[1].checkpoint_display_countdown = 120;
      crossed.race.riders[1].next_checkpoint = 1;
      crossed.race.riders[1].time_digits = {0, 3, 3, 5, 8};
      // `finish` is written twice, so the clock stands one picture longer than
      // it would with nothing competing for the queue, and each field behind
      // it follows one picture later.
      auto still = run(competing, done, crossed);
      require(!still.clock_blanked && still.clock == std::optional<std::string>("0:33:5"));
      still = run(competing, crossed, crossed);
      require(!still.clock_blanked);
      require(run(competing, crossed, crossed).clock_blanked);
      require(run(competing, crossed, crossed).player_cells == std::optional<std::string>("0:33:50"));
      require(run(competing, crossed, crossed).opponent_cells == std::optional<std::string>("0:33:58"));

      // R-0044: the same fields mid-race. On the M4-16 primary the opponent is
      // first through checkpoint 1 of lap 1 on update 2008 at 0:08.5 (its
      // own cells are cut to a blank at once), and the player reaches it on
      // 2077 with the clock at 0:09.8 before that update's tick to 0:09.9:
      // the original publishes `+0:01:3`, not `+0:01:4`. The clock's tick
      // takes the queue first, so the split shows two pictures later.
      unirally::ClassicRaceHudClock split;
      unirally::ZoomZooState riding{};
      riding.race.riders[0].laps_remaining = 3;
      riding.race.riders[1].laps_remaining = 3;
      riding.race.checkpoint_seen.fill(255);
      riding.movement.timer.seconds = 8;
      riding.movement.timer.tenths = 5;
      prime(split, riding);
      auto opponent_first = riding;
      opponent_first.race.riders[1].checkpoint = 1;
      opponent_first.race.riders[1].next_checkpoint = 2;
      opponent_first.race.riders[1].checkpoint_display_countdown = 2;  // cut by $81:CA6E
      opponent_first.race.checkpoint_seen[3 * 4 + 1] = 0;
      require(!run(split, riding, opponent_first).opponent_cells);
      require(!run(split, opponent_first, opponent_first).opponent_cells);
      auto before_player = opponent_first;
      before_player.race.riders[1].checkpoint_display_countdown = 0;
      before_player.movement.timer.seconds = 9;
      before_player.movement.timer.tenths = 8;
      run(split, opponent_first, before_player);
      run(split, before_player, before_player);
      require(split.published().clock == std::optional<std::string>("0:09:8"));
      auto player_crosses = before_player;
      player_crosses.race.riders[0].checkpoint = 1;
      player_crosses.race.riders[0].next_checkpoint = 2;
      player_crosses.race.riders[0].checkpoint_display_countdown = 120;
      player_crosses.movement.timer.tenths = 9;      // the tick of the same update
      auto shown = run(split, before_player, player_crosses);          // 2077
      require(!shown.player_cells && shown.clock == std::optional<std::string>("0:09:8"));
      shown = run(split, player_crosses, player_crosses);              // 2078: the clock
      require(!shown.player_cells && shown.clock == std::optional<std::string>("0:09:9"));
      shown = run(split, player_crosses, player_crosses);              // 2079: the split
      require(shown.player_cells == std::optional<std::string>("+0:01:3"));
      // The opponent's field, second through a slot, shows the minus glyph
      // whatever the sign: on the primary the opponent reaches checkpoint 2
      // of lap 2 on 3881 with the clock at 0:45.9 against the player's stored
      // 0:45.8, and the original's rows 20-21 read `-0:00:1` (R-0043's own
      // reading of picture 3900), where the player's field would read `+`.
      unirally::ClassicRaceHudClock behind;
      unirally::ZoomZooState second_lap{};
      second_lap.race.riders[0].laps_remaining = 2;
      second_lap.race.riders[1].laps_remaining = 2;
      second_lap.race.riders[0].checkpoint = 1;
      second_lap.race.riders[0].next_checkpoint = 2;
      second_lap.race.riders[1].checkpoint = 1;
      second_lap.race.riders[1].next_checkpoint = 2;
      second_lap.race.checkpoint_seen.fill(255);
      second_lap.movement.timer.tens_seconds = 4;
      second_lap.movement.timer.seconds = 5;
      second_lap.movement.timer.tenths = 8;
      prime(behind, second_lap);
      auto player_first = second_lap;
      player_first.race.riders[0].checkpoint = 2;
      player_first.race.riders[0].next_checkpoint = 3;
      player_first.race.riders[0].checkpoint_display_countdown = 120;
      player_first.race.checkpoint_seen[2 * 4 + 2] = 0;
      run(behind, second_lap, player_first);
      run(behind, player_first, player_first);
      require(!run(behind, player_first, player_first).player_cells);
      auto ticked = player_first;
      ticked.movement.timer.tenths = 9;
      run(behind, player_first, ticked);
      auto opponent_second = ticked;
      opponent_second.race.riders[1].checkpoint = 2;
      opponent_second.race.riders[1].next_checkpoint = 3;
      opponent_second.race.riders[1].checkpoint_display_countdown = 120;
      run(behind, ticked, opponent_second);
      run(behind, opponent_second, opponent_second);
      require(run(behind, opponent_second, opponent_second).opponent_cells ==
              std::optional<std::string>("-0:00:1"));
      // Both riders through an unseen slot on one update: $81:CB13 runs the
      // player first, who stores the clock and draws nothing, then the
      // opponent, who finds the slot seen and publishes a zero split with the
      // constant minus (review should-fix 1; no capture shows it).
      unirally::ClassicRaceHudClock tie;
      auto together = second_lap;
      prime(tie, together);
      auto both = together;
      for (auto rider : {0, 1}) {
        both.race.riders[static_cast<std::size_t>(rider)].checkpoint = 2;
        both.race.riders[static_cast<std::size_t>(rider)].next_checkpoint = 3;
        both.race.riders[static_cast<std::size_t>(rider)].checkpoint_display_countdown = 120;
      }
      both.race.checkpoint_seen[2 * 4 + 2] = 0;
      run(tie, together, both);
      run(tie, both, both);
      auto tied = run(tie, both, both);
      require(!tied.player_cells && tied.opponent_cells == std::optional<std::string>("-0:00:0"));
      // 118 updates later the countdown reaches 2 and the cells are blanked.
      auto expiring = player_crosses;
      expiring.race.riders[0].checkpoint_display_countdown = 2;
      run(split, player_crosses, expiring);
      require(!run(split, expiring, expiring).player_cells);
      // A rider first through a slot draws nothing for it, and a queue that
      // never saw the first crossing of a slot leaves the cells alone.
      unirally::ClassicRaceHudClock blind;
      prime(blind, before_player);
      run(blind, before_player, player_crosses);
      run(blind, player_crosses, player_crosses);
      require(!run(blind, player_crosses, player_crosses).player_cells);
      // The crossing text and the split arithmetic themselves.
      require(unirally::classic_hud_crossing_text({1, 3, 8, 0, 2}) == "1:38:02");
      require(unirally::classic_hud_split_text({0, 0, 9, 8}, {0, 0, 8, 5}) == "+0:01:3");
      require(unirally::classic_hud_split_text({0, 4, 5, 9}, {0, 4, 5, 8}) == "+0:00:1");
      require(unirally::classic_hud_split_text({1, 0, 1, 1}, {0, 5, 9, 9}) == "+0:01:2");
      require(unirally::classic_hud_split_text({0, 4, 5, 8}, {0, 4, 5, 8}) == "+0:00:0");
      // The unmeasured negative path, as $81:C9F4-CA33 spells it: the ten's
      // complement of the borrowed digits, exact for 8.5 against 9.8, and one
      // second short when the tenths come out at ten (8.5 against 10.5 is two
      // seconds, shown as one).
      require(unirally::classic_hud_split_text({0, 0, 8, 5}, {0, 0, 9, 8}) == "-0:01:3");
      require(unirally::classic_hud_split_text({0, 0, 8, 5}, {0, 1, 0, 5}) == "-0:01:0");

      // A tour race holds the clock on either rider's crossing, because both
      // dirty the field and the lap-number path writes it. A sprint holds on
      // none of them: $81:EB93 branches to $81:EB9B, which jumps to the clock
      // handler and never clears `$0D17`.
      unirally::ClassicRaceHudClock tour, plain;
      unirally::ZoomZooState lap0{}, lap1{}, lap2{};
      lap0.race.riders[0].laps_remaining = 2;
      lap0.race.riders[1].laps_remaining = 2;
      lap0.movement.timer.tens_seconds = 3;
      lap0.movement.timer.seconds = 2;
      lap0.movement.timer.tenths = 5;
      prime(tour, lap0);
      lap1 = lap0;
      lap1.race.riders[1].laps_remaining = 1;   // the opponent crosses, the tenth ticks
      lap1.movement.timer.tenths = 6;
      require(run(tour, lap0, lap1).clock == std::optional<std::string>("0:32:5"));
      lap2 = lap1;
      // The lap number takes that update, so the tenth waits a picture longer.
      require(run(tour, lap1, lap2).clock == std::optional<std::string>("0:32:5"));
      require(run(tour, lap2, lap2).clock == std::optional<std::string>("0:32:6"));
      unirally::ZoomZooState sprint0 = lap0, sprint1;
      sprint0.track = unirally::ClassicRaceTrack::Dragster;
      prime(plain, sprint0);
      sprint1 = sprint0;
      sprint1.race.riders[1].laps_remaining = 1;
      sprint1.movement.timer.tenths = 6;
      // A sprint does not take the update, so the tenth is published at once
      // and appears a picture earlier than the tour race's.
      require(run(plain, sprint0, sprint1).clock == std::optional<std::string>("0:32:5"));
      require(run(plain, sprint1, sprint1).clock == std::optional<std::string>("0:32:6"));
    }

    // 10:00 time-out ($81:C73E-C75B): finished with laps left, so `$0EFB` is
    // not zero. The original keeps the lap field and the held 9:59.9 clock and
    // shows none of the finish fields (stop-timeout original 31578-31920).
    unirally::ZoomZooState timed_out{};
    timed_out.race.riders[0].finished = 1;
    timed_out.race.riders[0].laps_remaining = 2;
    timed_out.race.finish_delay = 200;
    timed_out.race.total_times = {60000, 60000};
    timed_out.movement.timer.minutes = 9;
    timed_out.movement.timer.tens_seconds = 5;
    timed_out.movement.timer.seconds = 9;
    timed_out.movement.timer.tenths = 9;
    timed_out.movement.timer.subframe = 3;
    hud = unirally::classic_race_hud_text(timed_out, zoom, std::nullopt);
    require(hud.left == "2/3" && hud.clock == "9:59:9" && hud.player_cells.empty() && hud.opponent_cells.empty());
  }

  // Race palette cycle $82:D382-D496. Original $0B84 ends frame n at
  // (n-1381)&15, including through pause, so frame n draws (n-1382)&15.
  require(!unirally::zoom_zoo_palette_cycle_index(1381));
  require(unirally::zoom_zoo_palette_cycle_index(1382) == 0U);
  require(unirally::zoom_zoo_palette_cycle_index(1649) == 11U);
  require(unirally::zoom_zoo_palette_cycle_index(3208) == 2U);
  require(unirally::zoom_zoo_palette_cycle_index(6005) == 15U);
  {
    // Synthetic tables: word = table * 256 + index, so a copied word names
    // both its source table and entry.
    std::vector<std::uint8_t> tables(544);
    for (std::size_t table = 0; table < 17; ++table)
      for (std::size_t index = 0; index < 16; ++index) {
        tables[table * 32 + index * 2] = static_cast<std::uint8_t>(index);
        tables[table * 32 + index * 2 + 1] = static_cast<std::uint8_t>(table);
      }
    std::array<std::uint8_t, 512> cgram{};
    cgram.fill(0xee);
    unirally::apply_zoom_zoo_palette_cycle(cgram, tables, 1381);
    require(cgram[0] == 0xee && cgram[192] == 0xee);
    unirally::apply_zoom_zoo_palette_cycle(cgram, tables, 1382 + 16 + 5);
    require(cgram[192] == 5 && cgram[193] == 0);    // colour 96, table 0
    require(cgram[222] == 5 && cgram[223] == 15);   // colour 111, table 15
    require(cgram[0] == 5 && cgram[1] == 16);       // colour 0, table 16
    require(cgram[190] == 0xee && cgram[224] == 0xee); // colours 95 and 112 untouched
    bool rejected = false;
    try {
      unirally::apply_zoom_zoo_palette_cycle(cgram, std::span<const std::uint8_t>(tables).first(543), 1400);
    } catch (const std::invalid_argument &) {
      rejected = true;
    }
    require(rejected);
  }

  {
    // DRAGSTER race palette cycle (R-0037), against original CGRAM read from
    // the race-crawler-dragster winner and loser replays. Synthetic tables:
    // word = table * 256 + index names the source table and entry.
    std::vector<std::uint8_t> tables(544);
    for (std::size_t table = 0; table < 17; ++table)
      for (std::size_t index = 0; index < 16; ++index) {
        tables[table * 32 + index * 2] = static_cast<std::uint8_t>(index);
        tables[table * 32 + index * 2 + 1] = static_cast<std::uint8_t>(table);
      }
    const auto drawn = [&](std::uint32_t frame, unirally::RacePhase phase,
                           std::uint16_t loading_updates) {
      unirally::MovementState state{};
      state.frame = frame;
      state.finish.phase = phase;
      state.finish.result_loading_updates = loading_updates;
      std::array<std::uint8_t, 512> cgram{};
      cgram.fill(0xee);
      unirally::apply_dragster_palette_cycle(cgram, tables, state);
      return cgram;
    };
    using unirally::RacePhase;
    auto cgram = drawn(1333, RacePhase::Racing, 0);
    require(cgram[192] == 0xee && cgram[0] == 0xee); // before the routine runs
    cgram = drawn(1600, RacePhase::Racing, 0);       // accepted racing_cycle phase
    require(cgram[192] == 10 && cgram[193] == 0 && cgram[222] == 10 && cgram[223] == 15);
    require(cgram[0] == 10 && cgram[1] == 16);
    cgram = drawn(3453, RacePhase::FinishDelay, 0);  // accepted finish_cycle phase
    require(cgram[192] == 7 && cgram[0] == 7 && cgram[1] == 16);
    cgram = drawn(3454, RacePhase::ResultLoading, 1); // winner loading start
    require(cgram[192] == 8 && cgram[223] == 15 && cgram[0] == 0 && cgram[1] == 0);
    cgram = drawn(3528, RacePhase::ResultLoading, 75); // still frozen at 3454's phase
    require(cgram[192] == 8 && cgram[0] == 0 && cgram[1] == 0);
    cgram = drawn(3559, RacePhase::ResultLoading, 1); // loser loading start
    require(cgram[192] == 1 && cgram[0] == 0);
    cgram = drawn(10, RacePhase::ResultLoading, 20); // counter past the frame
    require(cgram[192] == 0xee && cgram[0] == 0xee);
    require(cgram[190] == 0xee && cgram[224] == 0xee); // colours 95 and 112 untouched
  }

  std::vector<std::uint8_t> track(33815);
  for (std::size_t x = 0; x < 30; ++x)
    for (std::size_t y = 0; y < 16; ++y) {
      const auto v = static_cast<std::uint16_t>(x * 16 + y);
      const auto at = 0x800f + x * 32 + y * 2;
      track[at] = static_cast<std::uint8_t>(v);
      track[at + 1] = static_cast<std::uint8_t>(v >> 8U);
    }
  const auto map = unirally::expand_dragster_bg1(track);
  require(map[0] == 0 && map[1] == 16 && map[30] == 1 && map[479] == 479);
  const auto col = unirally::gather_dragster_bg1(track, 0x8000, 2, 16);
  require(col.front() == 0 && col.back() == 15);
  bool rejected = false;
  try {
    (void)unirally::gather_dragster_bg1(track, 0, 3, 1);
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  require(rejected);
  // A synthetic selector at the observed first plane expands through the same
  // 32-byte, four-by-four metatile definition used by $81:B270.
  track[0x5831] = 1;
  for (std::size_t plane = 1; plane < 5; ++plane)
    track[0x5831 + plane * 0x800] = 1;
  for (std::size_t i = 0; i < 16; ++i) {
    const auto at = 0x800f + 32 + i * 2;
    track[at] = static_cast<std::uint8_t>(i + 1);
  }
  const auto rolling = unirally::build_dragster_bg1_map(track, 0, 160);
  require(rolling[10 * 32 + 16] == 1);
  require(rolling[10 * 32 + 19] == 4);
  require(rolling[13 * 32 + 16] == 13);
  rejected = false;
  try {
    (void)unirally::expand_dragster_bg1(
        std::span<const std::uint8_t>(track).first(0x800f));
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  require(rejected);
  require(unirally::rider_frame_for_pose(0x855, true).logical_id ==
          "presentation.rider.mike.race-tiles.v1");
  rejected = false;
  try {
    (void)unirally::rider_frame_for_pose(0xffff, false);
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  require(rejected);
  rejected = false;
  try {
    (void)unirally::rider_frame_for_pose(0x0855, false);
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  require(rejected);
  auto state = unirally::classic_crawler_dragster_start();
  state.riders[0].pose.pose_index = 0x855;
  state.riders[1].pose.pose_index = 0x895;
  const auto before = unirally::serialize_movement_state(state);
  std::vector<std::uint8_t> bg1(2560), bg2(992), bg2_map(8192), palette(352),
      font(2048), rider(3456), result(5224), go_window, winner_window,
      result_base_vram(41536), result_palette(216), result_palette_tail(128);
  const auto empty_window_table = [] {
    std::vector<std::uint8_t> table;
    for (const unsigned lines : {127U, 97U}) {
      table.push_back(static_cast<std::uint8_t>(0x80U | lines));
      for (unsigned line = 0; line < lines; ++line)
        table.insert(table.end(), {255, 0, 255, 0});
    }
    return table;
  };
  go_window = empty_window_table();
  winner_window = empty_window_table();
  const unirally::PresentationContent content{track,
                                              bg1,
                                              bg2,
                                              bg2_map,
                                              palette,
                                              font,
                                              rider,
                                              result,
                                              go_window,
                                              winner_window,
                                              result_base_vram,
                                              result_palette,
                                              result_palette_tail,
                                              {},
                                              {}};
  const auto first = unirally::render_dragster_headless({state, 0, 0, 0, 0, 0},
                                                        content),
             second = unirally::render_dragster_headless({state, 0, 0, 0, 0, 0},
                                                         content);
  require(first.pixels == second.pixels);
  require(before == unirally::serialize_movement_state(state));

  // The retained result copier resets VMADD at byte $7B00. With stable scroll
  // 82, screen pixel (0,0) selects map row 10 and character 293; this byte is
  // reachable only through the second half of that split run.
  auto result_state = state;
  result_state.finish.phase = unirally::RacePhase::ResultScreen;
  result_state.finish.outcome = unirally::RaceOutcome::PlayerWon;
  result_state.finish.result_loading_updates = 226;
  result_state.finish.rider_finished = {true, true};
  result_state.finish.finish_time_centiseconds = {3357, 3358};
  result_state.finish.finish_time_digits = {{{0, 3, 3, 5, 7}, {0, 3, 3, 5, 8}}};
  result[5208] = 'd';
  result[5209] = 'r';
  result[5210] = 'a';
  result[5211] = 'g';
  result[5212] = 's';
  result[5213] = 't';
  result[5214] = 'e';
  result[5215] = 'r';
  result[5216] = 0xff;
  constexpr std::size_t visible_map_word = 8192 + 10 * 64;
  result_base_vram[visible_map_word] = 0x25;
  result_base_vram[visible_map_word + 1] = 0x01;
  result_palette[2] = 0xff;
  result_palette[3] = 0x7f;
  const auto empty_result = unirally::render_dragster_headless(
      {result_state, 0, 0, 0, 0, 0}, content);
  result_base_vram[39814] = 0x80;
  const auto split_result = unirally::render_dragster_headless(
      {result_state, 0, 0, 0, 0, 0}, content);
  require(empty_result.pixels[0] != split_result.pixels[0]);
  // Exercise both ends of the planar shift range. The same first plane byte
  // selects the leftmost pixel with bit 7 and the rightmost pixel with bit 0.
  result_base_vram[39814] = 0x01;
  const auto low_bit_result = unirally::render_dragster_headless(
      {result_state, 0, 0, 0, 0, 0}, content);
  require(empty_result.pixels[7 * 3] != low_bit_result.pixels[7 * 3]);
  require(empty_result.pixels[0] == low_bit_result.pixels[0]);
  result_base_vram[39814] = 0x80;
  require(before == unirally::serialize_movement_state(state));

  const auto winner_map =
      unirally::build_dragster_result_map(result_state, result);
  require(winner_map[11 * 32 + 21] == 0x3cac);
  require(winner_map[11 * 32 + 23] == 0x3cae);
  require(winner_map[11 * 32 + 24] == 0x3cb0);

  auto loser_state = result_state;
  loser_state.finish.outcome = unirally::RaceOutcome::PlayerLost;
  loser_state.finish.result_loading_updates = 242;
  loser_state.finish.finish_time_centiseconds = {3566, 3358};
  loser_state.finish.finish_time_digits[0] = {0, 3, 5, 6, 6};
  const auto loser_before = unirally::serialize_movement_state(loser_state);
  const auto loser_map =
      unirally::build_dragster_result_map(loser_state, result);
  require(loser_map[11 * 32 + 21] == 0x3cae);
  require(loser_map[11 * 32 + 23] == 0x3caf);
  require(loser_map[11 * 32 + 24] == 0x3caf);
  require(loser_map[12 * 32 + 21] == 0x3cea);
  require(loser_map[12 * 32 + 23] == 0x3ceb);
  require(loser_map[12 * 32 + 24] == 0x3ceb);
  for (std::size_t entry = 0; entry < winner_map.size(); ++entry) {
    const bool time_glyph = entry == 11 * 32 + 21 || entry == 11 * 32 + 23 ||
                            entry == 11 * 32 + 24 || entry == 12 * 32 + 21 ||
                            entry == 12 * 32 + 23 || entry == 12 * 32 + 24;
    require(time_glyph || winner_map[entry] == loser_map[entry]);
  }
  // RESULT-TITLE-GLYPHS: a winner's result loads whether or not the opponent
  // has finished (DOWN+UP's original) and shows only the player's time, so the
  // map is the winner's. A loser's result still needs the opponent's finish.
  auto winner_opponent_riding = result_state;
  winner_opponent_riding.finish.rider_finished[1] = false;
  require(unirally::build_dragster_result_map(winner_opponent_riding, result) == winner_map);
  auto loser_opponent_riding = loser_state;
  loser_opponent_riding.finish.rider_finished[1] = false;
  bool loser_refused = false;
  try {
    (void)unirally::build_dragster_result_map(loser_opponent_riding, result);
  } catch (const std::invalid_argument &) {
    loser_refused = true;
  }
  require(loser_refused);
  // Review of #26 (S1): a no-time player is never a winner while the opponent
  // rides; the timed-out race ends with both riders finished (R-0039).
  auto no_time_winner_opponent_riding = winner_opponent_riding;
  no_time_winner_opponent_riding.finish.finish_time_centiseconds[0] = 60000;
  no_time_winner_opponent_riding.timer.minutes = 9;
  no_time_winner_opponent_riding.timer.tens_seconds = 5;
  no_time_winner_opponent_riding.timer.seconds = 9;
  no_time_winner_opponent_riding.timer.tenths = 9;
  bool no_time_winner_refused = false;
  try {
    (void)unirally::build_dragster_result_map(no_time_winner_opponent_riding, result);
  } catch (const std::invalid_argument &) {
    no_time_winner_refused = true;
  }
  require(no_time_winner_refused);
  const auto loser_result =
      unirally::render_dragster_headless({loser_state, 0, 0, 0, 0, 0}, content);
  require(loser_result.pixels.size() == split_result.pixels.size());
  require(loser_before == unirally::serialize_movement_state(loser_state));

  // $81:C73E-C75B ends the race at 10:00 with the player lap-short: its total
  // stays the 60000 no-time sentinel while its crossing digits still hold the
  // start-line crossing (0:00.70 in the clock-limit original). The original
  // result screen writes NO TIME in the player row, like the three empty ones.
  auto timed_out = loser_state;
  timed_out.finish.finish_time_centiseconds = {60000, 3358};
  timed_out.finish.finish_time_digits[0] = {0, 0, 0, 7, 0};
  timed_out.timer.minutes = 9;
  timed_out.timer.tens_seconds = 5;
  timed_out.timer.seconds = 9;
  timed_out.timer.tenths = 9;
  const auto timed_out_before = unirally::serialize_movement_state(timed_out);
  const auto timed_out_map =
      unirally::build_dragster_result_map(timed_out, result);
  for (std::size_t column = 17; column < 25; ++column) {
    require(timed_out_map[11 * 32 + column] == timed_out_map[14 * 32 + column]);
    require(timed_out_map[12 * 32 + column] == timed_out_map[15 * 32 + column]);
  }
  for (std::size_t entry = 0; entry < loser_map.size(); ++entry) {
    const auto column = entry % 32;
    const auto row = entry / 32;
    const bool player_time =
        (row == 11 || row == 12) && column >= 17 && column < 25;
    require(player_time || timed_out_map[entry] == loser_map[entry]);
  }

  // Only the held clock admits the sentinel: the same totals with an ordinary
  // clock are not a timed-out race and stay rejected (review finding 2).
  auto sentinel_without_limit = timed_out;
  sentinel_without_limit.timer.minutes = 0;
  sentinel_without_limit.timer.tens_seconds = 3;
  sentinel_without_limit.timer.seconds = 3;
  sentinel_without_limit.timer.tenths = 5;
  rejected = false;
  try {
    (void)unirally::build_dragster_result_map(sentinel_without_limit, result);
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  require(rejected);
  require(timed_out_before == unirally::serialize_movement_state(timed_out));
  // The admission is the sentinel, not an inconsistent time: a player total
  // below it must still agree with its digits, and an opponent with no time
  // has no original evidence and stays rejected.
  auto contradictory = timed_out;
  contradictory.finish.finish_time_centiseconds[0] = 59999;
  rejected = false;
  try {
    (void)unirally::build_dragster_result_map(contradictory, result);
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  require(rejected);
  contradictory = timed_out;
  contradictory.finish.finish_time_centiseconds[1] = 60000;
  rejected = false;
  try {
    (void)unirally::build_dragster_result_map(contradictory, result);
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  require(rejected);

  contradictory = loser_state;
  contradictory.finish.outcome = unirally::RaceOutcome::PlayerWon;
  rejected = false;
  try {
    (void)unirally::build_dragster_result_map(contradictory, result);
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  require(rejected);
  contradictory = loser_state;
  contradictory.finish.result_loading_updates = 241;
  rejected = false;
  try {
    (void)unirally::render_dragster_headless(
        {contradictory, 0, 0, 0, 0, 0}, content);
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  require(rejected);
  contradictory = loser_state;
  contradictory.finish.finish_time_digits[0][4] = 5;
  rejected = false;
  try {
    (void)unirally::build_dragster_result_map(contradictory, result);
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  require(rejected);

  // Result presentation becomes visible at the observed end-of-frame 3678
  // boundary while gameplay deliberately remains in ResultLoading update 225.
  result_state.finish.phase = unirally::RacePhase::ResultLoading;
  result_state.finish.result_loading_updates = 225;
  const auto boundary_result = unirally::render_dragster_headless(
      {result_state, 0, 0, 0, 0, 0}, content);
  require(boundary_result.pixels == split_result.pixels);
  result_state.finish.result_loading_updates = 224;
  const auto prior_loading = unirally::render_dragster_headless(
      {result_state, 0, 0, 0, 0, 0}, content);
  require(prior_loading.pixels != split_result.pixels);

  // Both extremes remain defined: the riders are wholly off-screen and the
  // wide coordinate subtraction is never narrowed or evaluated in `int`.
  state.riders[0].pose.reflected = true;
  state.riders[1].pose.reflected = true;
  (void)unirally::render_dragster_headless({state, INT32_MIN, 0, 0, 0, 0},
                                           content);
  (void)unirally::render_dragster_headless({state, INT32_MAX, 0, 0, 0, 0},
                                           content);

  auto winner_state = state;
  winner_state.riders[0].pose.pose_index = 0x04fe;
  winner_state.riders[1].pose.pose_index = 0x037c;
  auto visible_window = winner_window;
  visible_window[1] = 10;
  visible_window[2] = 20;
  auto visible_content = content;
  visible_content.winner_window = visible_window;
  const auto masked = unirally::render_dragster_headless(
      {winner_state, 0, 0, 0, 0, 0}, visible_content);
  if (masked.pixels[(15 * 3)] != 98)
    throw std::runtime_error("window mask did not cover its inclusive edge");
  if (masked.pixels == first.pixels)
    throw std::runtime_error("window table mutation did not affect output");
  {
    // With race palette tables present, the GO and winner windows show the
    // cycled colour 0 (R-0037). Phases 7 and 10 reproduce the accepted
    // (98,98,255) and white; phase 6 is the original's (121,38,255) at 3452.
    std::vector<std::uint8_t> cycle(544);
    const auto colour_zero = [&](unsigned index, std::uint16_t word) {
      cycle[16 * 32 + index * 2] = static_cast<std::uint8_t>(word);
      cycle[16 * 32 + index * 2 + 1] = static_cast<std::uint8_t>(word >> 8U);
    };
    colour_zero(6, 0x7cef);
    colour_zero(7, 0x7dad);
    colour_zero(10, 0x7fff);
    auto cycled = visible_content;
    cycled.race_palette_cycle = cycle;
    auto go_state = state;
    go_state.riders[0].pose.pose_index = 0x04f9;
    go_state.riders[1].pose.pose_index = 0x0263;
    auto go_content = cycled;
    go_content.winner_window = winner_window;
    go_content.go_window = visible_window;
    const auto window_pixel = [](unirally::MovementState rider_state,
                                 std::uint32_t frame,
                                 const unirally::PresentationContent &assets) {
      rider_state.frame = frame;
      const auto rendered =
          unirally::render_dragster_headless({rider_state, 0, 0, 0, 0, 0}, assets);
      return std::array<std::uint8_t, 3>{rendered.pixels[45], rendered.pixels[46],
                                         rendered.pixels[47]};
    };
    using Rgb = std::array<std::uint8_t, 3>;
    require(window_pixel(winner_state, 1334 + 7, cycled) == Rgb{98, 98, 255});
    require(window_pixel(winner_state, 1334 + 10, cycled) == Rgb{255, 255, 255});
    require(window_pixel(winner_state, 3452, cycled) == Rgb{121, 38, 255});
    require(window_pixel(go_state, 3452, go_content) == Rgb{121, 38, 255});
    // Without the tables (v1 packs) the accepted fixed colours stay.
    require(window_pixel(winner_state, 3452, visible_content) == Rgb{98, 98, 255});
  }
  {
    // R-0040: the recovered channel-6 table selection. Every index below is the
    // original's own $11FD pointer, read at $80:8691 in the DRAGSTER access
    // captures of the continuous-right, release-3213 and opponent-won runs.
    const auto index = [](std::uint32_t frame, unirally::RacePhase phase,
                          unirally::RaceOutcome outcome, std::uint16_t delay,
                          std::uint16_t loading, std::uint16_t opponent_animation) {
      unirally::MovementState probe{};
      probe.frame = frame;
      probe.finish.phase = phase;
      probe.finish.outcome = outcome;
      probe.finish.player_finish_delay = delay;
      probe.finish.result_loading_updates = loading;
      probe.finish.finish_animation_countdown[1] = opponent_animation;
      return unirally::dragster_window_table_index(probe);
    };
    const auto racing = [&](std::uint32_t frame) {
      return index(frame, unirally::RacePhase::Racing,
                   unirally::RaceOutcome::Pending, 0, 0, 0);
    };
    using Index = std::optional<unsigned>;
    // The race vblank publishes nothing before initialization frame 1328 + 6.
    require(racing(1333) == Index{});
    // $11C5 runs 270 down to 1; each digit shows the transition table above its
    // own threshold and its own table below it.
    require(racing(1334) == Index{6} && racing(1354) == Index{6});
    require(racing(1355) == Index{0} && racing(1383) == Index{0});
    require(racing(1384) == Index{6} && racing(1414) == Index{6});
    require(racing(1415) == Index{1} && racing(1443) == Index{1});
    require(racing(1444) == Index{6} && racing(1474) == Index{6});
    require(racing(1475) == Index{2} && racing(1503) == Index{2});
    require(racing(1504) == Index{6} && racing(1534) == Index{6});
    // GO alternates on the $0300 parity and stops when $11C5 reaches zero.
    require(racing(1535) == Index{4} && racing(1536) == Index{3});
    require(racing(1600) == Index{3} && racing(1603) == Index{4});
    require(racing(1604) == Index{} && racing(2400) == Index{});
    // The banner driver starts on the update after the finish and its choice
    // reaches the screen one frame later, so 3215 is the first index 7.
    const auto won = [&](std::uint32_t frame, std::uint16_t delay) {
      return index(frame, unirally::RacePhase::FinishDelay,
                   unirally::RaceOutcome::PlayerWon, delay, 0, 0);
    };
    require(won(3213, 0) == Index{} && won(3214, 1) == Index{});
    require(won(3215, 2) == Index{7} && won(3216, 3) == Index{8});
    require(won(3217, 4) == Index{8} && won(3218, 5) == Index{9});
    require(won(3322, 109) == Index{7});
    require(won(3450, 237) == Index{17} && won(3452, 239) == Index{18});
    require(won(3453, 240) == Index{18});
    // Result loading holds update 1's choice; the race vblank stops after it.
    require(index(3454, unirally::RacePhase::ResultLoading,
                  unirally::RaceOutcome::PlayerWon, 240, 1, 0) == Index{19});
    require(index(3600, unirally::RacePhase::ResultLoading,
                  unirally::RaceOutcome::PlayerWon, 240, 147, 0) == Index{19});
    // An odd number of loading updates: without the freeze this frame would
    // step the banner on, which the original no longer does.
    require(index(3601, unirally::RacePhase::ResultLoading,
                  unirally::RaceOutcome::PlayerWon, 240, 148, 0) == Index{19});
    require(index(3679, unirally::RacePhase::ResultScreen,
                  unirally::RaceOutcome::PlayerWon, 240, 226, 0) == Index{});
    // The opponent-won banner, while its 120-update animation counter locates
    // the finish: the opponent-won capture finishes at 3214 and shows 8 at 3216.
    const auto lost = [&](std::uint32_t frame, std::uint16_t animation) {
      return index(frame, unirally::RacePhase::Racing,
                   unirally::RaceOutcome::PlayerLost, 0, 0, animation);
    };
    require(lost(3215, 119) == Index{} && lost(3216, 118) == Index{8});
    require(lost(3217, 117) == Index{8} && lost(3218, 116) == Index{9});
    require(lost(3333, 1) == Index{12});
    // Beyond the counter the opponent's finish frame is not recoverable, and
    // native stops drawing a banner the original still shows (index 13 here).
    require(lost(3334, 0) == Index{});
    // The 360-frame $0F07 life is a defensive bound, not observed behaviour:
    // deserialize rejects a player_finish_delay above 240, so no restorable
    // state reaches it, and for the parity the captured race takes the ROM
    // driver leaves a frame earlier. Assert only what a reachable state shows
    // (review finding C).
    require(won(3453, 240).has_value());

    // The index chosen is the table actually drawn: mutating that member of the
    // family changes the picture and mutating any other member does not.
    std::vector<std::uint8_t> family(25 * 899, 0);
    for (unsigned table = 0; table < 25; ++table) {
      auto at = static_cast<std::size_t>(table) * 899;
      for (const unsigned lines : {127U, 97U}) {
        family[at++] = static_cast<std::uint8_t>(0x80U | lines);
        for (unsigned line = 0; line < lines; ++line) {
          family[at++] = 4;    // window 1 covers x 4..12
          family[at++] = 12;
          family[at++] = 128;  // window 2 is empty: left above right
          family[at++] = 127;
        }
      }
      require(at == static_cast<std::size_t>(table) * 899 + 898);
    }
    auto family_content = content;
    family_content.window_tables = family;
    auto banner_state = state;
    banner_state.frame = 3215;
    banner_state.finish.phase = unirally::RacePhase::FinishDelay;
    banner_state.finish.outcome = unirally::RaceOutcome::PlayerWon;
    banner_state.finish.player_finish_delay = 2;
    const auto drawn = unirally::render_dragster_headless(
        {banner_state, 0, 0, 0, 0, 0}, family_content);
    for (const unsigned table : {0U, 6U, 7U, 8U, 18U}) {
      auto mutated = family;
      mutated[static_cast<std::size_t>(table) * 899 + 2] = 40;  // widen window 1
      auto mutated_content = family_content;
      mutated_content.window_tables = mutated;
      const auto after = unirally::render_dragster_headless(
          {banner_state, 0, 0, 0, 0, 0}, mutated_content);
      require((after.pixels != drawn.pixels) == (table == 7U));
    }
    // A GO frame draws its own member, before the riders rather than after.
    // The window is white here, so the frame takes the pose pair whose colour 0
    // is not white; the pose no longer decides whether a window is drawn.
    auto go_frame = state;
    go_frame.riders[0].pose.pose_index = 0x04fe;
    go_frame.riders[1].pose.pose_index = 0x037c;
    for (const auto &[frame, chosen] : {std::pair<std::uint32_t, unsigned>{1600, 3},
                                        std::pair<std::uint32_t, unsigned>{1601, 4},
                                        std::pair<std::uint32_t, unsigned>{1355, 0}}) {
      go_frame.frame = frame;
      const auto go_drawn = unirally::render_dragster_headless(
          {go_frame, 0, 0, 0, 0, 0}, family_content);
      for (const unsigned table : {0U, 3U, 4U, 7U}) {
        auto mutated = family;
        mutated[static_cast<std::size_t>(table) * 899 + 2] = 40;
        auto mutated_content = family_content;
        mutated_content.window_tables = mutated;
        const auto after = unirally::render_dragster_headless(
            {go_frame, 0, 0, 0, 0, 0}, mutated_content);
        require((after.pixels != go_drawn.pixels) == (table == chosen));
      }
    }
    // The family is rejected when it is the wrong length or loses a terminator.
    rejected = false;
    try {
      (void)unirally::dragster_window_table(
          std::span<const std::uint8_t>(family).first(25 * 899 - 1), 0);
    } catch (const std::invalid_argument &) {
      rejected = true;
    }
    require(rejected);
    rejected = false;
    try {
      (void)unirally::dragster_window_table(family, 25);
    } catch (const std::invalid_argument &) {
      rejected = true;
    }
    require(rejected);
    rejected = false;
    auto unterminated = family;
    unterminated[7 * 899 + 898] = 1;
    try {
      (void)unirally::dragster_window_table(unterminated, 7);
    } catch (const std::invalid_argument &) {
      rejected = true;
    }
    require(rejected);
  }
  auto invalid_window = winner_window;
  invalid_window[0] = 0;
  auto invalid_content = content;
  invalid_content.winner_window = invalid_window;
  rejected = false;
  try {
    (void)unirally::render_dragster_headless({winner_state, 0, 0, 0, 0, 0},
                                             invalid_content);
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  if (!rejected)
    throw std::runtime_error("unsupported window HDMA control was accepted");
  rejected = false;
  try {
    auto short_content = content;
    short_content.palette = std::span<const std::uint8_t>(palette).first(351);
    (void)unirally::render_dragster_headless({state, 0, 0, 0, 0, 0},
                                             short_content);
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  require(rejected);

  {
    // The shared race shared_state drives the same palette phase, window selection
    // and result view as the legacy DRAGSTER finish struct (one renderer for
    // both tracks; CLASSIC-PRESENTATION-UNIFICATION).
    using unirally::ClassicRaceTrack;
    unirally::ZoomZooState race{};
    race.track = ClassicRaceTrack::Dragster;
    race.native_initialization = race.complete_race = race.sustained = true;
    const auto setup = unirally::classic_race_scenario(ClassicRaceTrack::Dragster).initialization_frame + 6U;
    require(setup == 1334U);
    // DRAGSTER's countdown transition member: 5 + $1229, the player's start
    // reflection latched at initialization (ZOOM-ZOO-WINDOW-EFFECTS).
    const unsigned transition = 6U;
    std::vector<std::uint8_t> phase_tables(544);
    for (std::size_t table = 0; table < 17; ++table)
      for (std::size_t index = 0; index < 16; ++index) {
        phase_tables[table * 32 + index * 2] = static_cast<std::uint8_t>(index);
        phase_tables[table * 32 + index * 2 + 1] = static_cast<std::uint8_t>(table);
      }
    const auto shared = [&](std::uint32_t frame, std::uint16_t result_updates) {
      auto shared_state = race;
      shared_state.movement.frame = frame;
      shared_state.result_updates = result_updates;
      std::array<std::uint8_t, 512> cgram{};
      cgram.fill(0xee);
      unirally::apply_classic_race_palette_cycle(cgram, phase_tables, shared_state, setup);
      return cgram;
    };
    const auto legacy = [&](std::uint32_t frame, unirally::RacePhase phase, std::uint16_t loading) {
      unirally::MovementState shared_state{};
      shared_state.frame = frame;
      shared_state.finish.phase = phase;
      shared_state.finish.result_loading_updates = loading;
      std::array<std::uint8_t, 512> cgram{};
      cgram.fill(0xee);
      unirally::apply_dragster_palette_cycle(cgram, phase_tables, shared_state);
      return cgram;
    };
    using unirally::RacePhase;
    require(shared(1333, 0) == legacy(1333, RacePhase::Racing, 0));
    require(shared(1600, 0) == legacy(1600, RacePhase::Racing, 0));
    require(shared(3453, 0) == legacy(3453, RacePhase::FinishDelay, 0));
    require(shared(3454, 1) == legacy(3454, RacePhase::ResultLoading, 1));
    require(shared(3528, 75) == legacy(3528, RacePhase::ResultLoading, 75));
    require(shared(10, 20) == legacy(10, RacePhase::ResultLoading, 20));
    require(shared(3454, 1)[0] == 0 && shared(3454, 1)[192] == 8);

    // Countdown and GO: no finish, the frame alone selects.
    for (const std::uint32_t frame : {1333U, 1334U, 1353U, 1354U, 1383U, 1500U, 1534U, 1535U, 1536U, 1603U, 1604U}) {
      auto shared_state = race;
      shared_state.movement.frame = frame;
      unirally::MovementState legacy_movement{};
      legacy_movement.frame = frame;
      require(unirally::classic_window_table_index(shared_state, setup, std::nullopt, transition) ==
              unirally::dragster_window_table_index(legacy_movement));
    }
    // Winner banner from the player's finish: finish_delay counts as the
    // legacy player_finish_delay, and loading update 1 is the last change.
    // (Only the player has finished: the legacy state knows one finish.)
    for (const std::uint32_t since : {0U, 1U, 2U, 3U, 40U, 41U, 240U}) {
      auto shared_state = race;
      shared_state.movement.frame = 3213 + since;
      shared_state.race.riders[0].finished = 1;
      shared_state.race.total_times = {3357, 60000};
      shared_state.race.finish_delay = static_cast<std::uint16_t>(since);
      unirally::MovementState legacy_movement{};
      legacy_movement.frame = shared_state.movement.frame;
      legacy_movement.finish.phase = RacePhase::FinishDelay;
      legacy_movement.finish.outcome = unirally::RaceOutcome::PlayerWon;
      legacy_movement.finish.player_finish_delay = static_cast<std::uint16_t>(since);
      require(unirally::classic_window_table_index(shared_state, setup, std::nullopt, transition) ==
              unirally::dragster_window_table_index(legacy_movement));
      if (since == 240U) {
        for (const std::uint16_t loading : {std::uint16_t{1}, std::uint16_t{2}, std::uint16_t{75}}) {
          shared_state.movement.frame = 3453 + loading;
          shared_state.result_updates = loading;
          legacy_movement.frame = shared_state.movement.frame;
          legacy_movement.finish.phase = RacePhase::ResultLoading;
          legacy_movement.finish.result_loading_updates = loading;
          const auto index = unirally::classic_window_table_index(shared_state, setup, std::nullopt, transition);
          require(index == unirally::dragster_window_table_index(legacy_movement));
          require(index == unirally::classic_window_table_index(
                               [&] { auto s = shared_state; s.movement.frame = 3454; s.result_updates = 1; return s; }(),
                               setup, std::nullopt, transition));
        }
      }
    }
    // Opponent won: its finish frame comes from the tracked history, or from
    // the two finish times once the player has finished (reversal original:
    // 3325 and 4292, totals 3363 and 5296; random-1: 3214 and 3636, 3358 and
    // 4202; both parities of the gap).
    {
      auto shared_state = race;
      shared_state.race.riders[1].finished = 1;
      shared_state.race.total_times = {60000, 3363};
      shared_state.movement.frame = 3400;
      require(!unirally::classic_opponent_finish_frame(shared_state));
      require(!unirally::classic_window_table_index(shared_state, setup, std::nullopt, transition));
      const auto tracked = unirally::classic_window_table_index(shared_state, setup, 3325U, transition);
      unirally::MovementState legacy_movement{};
      legacy_movement.frame = 3400;
      legacy_movement.finish.outcome = unirally::RaceOutcome::PlayerLost;
      legacy_movement.finish.finish_animation_countdown[1] = static_cast<std::uint16_t>(120U - (3400U - 3325U));
      require(tracked && tracked == unirally::dragster_window_table_index(legacy_movement));
      // Beyond the legacy 120-update counter the history still selects.
      shared_state.movement.frame = 3600;
      require(unirally::classic_window_table_index(shared_state, setup, 3325U, transition).has_value());
      shared_state.race.riders[0].finished = 1;
      shared_state.race.total_times = {5296, 3363};
      shared_state.movement.frame = 4300;
      shared_state.race.finish_delay = 8;
      require(unirally::classic_opponent_finish_frame(shared_state) == 3325U);
      // 975 frames after the opponent's finish its banner has run out, and the
      // player's own finish 8 frames ago has just restarted it.
      require(unirally::classic_window_table_index(shared_state, setup, std::nullopt, transition).has_value());
      shared_state.movement.frame = 4293;
      shared_state.race.finish_delay = 1;
      require(!unirally::classic_window_table_index(shared_state, setup, std::nullopt, transition));  // the finish update selects nothing
      // random-1 original: opponent 3214, expired at 3576; player 3636 restarts
      // it from the phase it stopped at: member 8 at 3638, 9 at 3640.
      auto restarted = race;
      restarted.race.riders[0].finished = restarted.race.riders[1].finished = 1;
      restarted.race.total_times = {4202, 3358};
      for (const auto &[frame, member] : {std::pair{3638U, 8U}, std::pair{3640U, 9U}, std::pair{3876U, 19U}}) {
        restarted.movement.frame = frame;
        restarted.race.finish_delay = static_cast<std::uint16_t>(frame - 3636U);
        require(unirally::classic_window_table_index(restarted, setup, std::nullopt, transition) == member);
        require(unirally::classic_window_table_index(restarted, setup, 3214U, transition) == member);
      }
      // The life is 180 steps: gone on the frame after the 181st odd driver
      // update (random-1: 3576 for the finish at 3214; reversal: 3688 for 3325).
      auto expired = race;
      expired.race.riders[1].finished = 1;
      expired.race.total_times = {60000, 3358};
      expired.movement.frame = 3575;
      require(unirally::classic_window_table_index(expired, setup, 3214U, transition) == 7U);
      expired.movement.frame = 3576;
      require(!unirally::classic_window_table_index(expired, setup, 3214U, transition));
      expired.movement.frame = 3687;
      require(unirally::classic_window_table_index(expired, setup, 3325U, transition) == 7U);
      expired.movement.frame = 3688;
      require(!unirally::classic_window_table_index(expired, setup, 3325U, transition));
      expired.movement.frame = 3327;
      require(unirally::classic_window_table_index(expired, setup, 3325U, transition) == 7U);  // even first driver update: no step
      expired.movement.frame = 3328;
      require(unirally::classic_window_table_index(expired, setup, 3325U, transition) == 8U);
      // primary-a: player 3211, opponent 3214 while the banner is alive: no gap
      // at the second finish (member 8 on 3215), the phase from the first.
      auto both = race;
      both.race.riders[0].finished = both.race.riders[1].finished = 1;
      both.race.total_times = {3353, 3358};
      for (const auto &[frame, member] : {std::pair{3213U, 7U}, std::pair{3214U, 8U}, std::pair{3215U, 8U}, std::pair{3216U, 9U}}) {
        both.movement.frame = frame;
        both.race.finish_delay = static_cast<std::uint16_t>(frame - 3211U);
        require(unirally::classic_window_table_index(both, setup, std::nullopt, transition) == member);
      }
      auto random_one = race;
      random_one.race.riders[0].finished = random_one.race.riders[1].finished = 1;
      random_one.race.total_times = {4202, 3358};
      random_one.movement.frame = 3640;
      random_one.race.finish_delay = 4;
      require(unirally::classic_opponent_finish_frame(random_one) == 3214U);
      require(!unirally::classic_opponent_finish_frame([&] { auto s = random_one; s.race.total_times[0] = 60000; return s; }()));
      // Player finished 100 frames after the opponent: the banner is still
      // running, and both derivations select the same member.
      auto late = race;
      late.race.riders[0].finished = late.race.riders[1].finished = 1;
      late.race.total_times = {3563, 3363}; // frames 3425 and 3325: 2*100 + (1-1)
      late.movement.frame = 3430;
      late.race.finish_delay = 5;
      require(unirally::classic_opponent_finish_frame(late) == 3325U);
      require(unirally::classic_window_table_index(late, setup, std::nullopt, transition) ==
              unirally::classic_window_table_index(late, setup, 3325U, transition));
      require(unirally::classic_window_table_index(late, setup, std::nullopt, transition).has_value());
      // Through result loading the delay holds at 240 from the frame before
      // loading starts (lose-a original: finishes 3214 and 3318, loading 3559).
      auto loading = race;
      loading.race.riders[0].finished = loading.race.riders[1].finished = 1;
      loading.race.total_times = {3566, 3358};
      loading.race.finish_delay = 240;
      loading.movement.frame = 3558;
      require(unirally::classic_opponent_finish_frame(loading) == 3214U);
      const auto before_loading = unirally::classic_window_table_index(loading, setup, std::nullopt, transition);
      for (const std::uint16_t updates : {std::uint16_t{1}, std::uint16_t{2}, std::uint16_t{40}}) {
        loading.movement.frame = 3558 + updates;
        loading.result_updates = updates;
        require(unirally::classic_opponent_finish_frame(loading) == 3214U);
        require(unirally::classic_window_table_index(loading, setup, std::nullopt, transition) ==
                unirally::classic_window_table_index(loading, setup, 3214U, transition));
      }
      require(before_loading.has_value());
    }
    // The fade INIDISP is written from: the earlier update's $0FF1, else the
    // accepted frame formula (a state passed as its own previous, after the
    // level saturates at 30, cannot give the preceding level).
    {
      const auto scenario = unirally::classic_race_scenario(ClassicRaceTrack::Dragster);
      auto now = race;
      auto earlier = race;
      now.movement.frame = 1328 + 16;
      now.fade_level = 16;
      earlier.movement.frame = 1328 + 15;
      earlier.fade_level = 15;
      require(unirally::classic_race_prior_fade(now, &earlier, scenario) == 15);
      require(unirally::classic_race_prior_fade(now, nullptr, scenario) == 15);
      require(unirally::classic_race_prior_fade(now, &now, scenario) == 15);  // not the state's own 16
      now.movement.frame = 1328;
      now.fade_level = 0;
      require(unirally::classic_race_prior_fade(now, &now, scenario) == 0);
      now.movement.frame = 1328 + 31;
      now.fade_level = 30;
      earlier.movement.frame = 1328 + 30;
      earlier.fade_level = 30;
      require(unirally::classic_race_prior_fade(now, &earlier, scenario) == 30);
      require(unirally::classic_race_prior_fade(now, &now, scenario) == 30);
      now.movement.frame = 1328 + 40;
      require(unirally::classic_race_prior_fade(now, &now, scenario) == 30);  // saturated: not 29
      now.movement.frame = 1328 + 31;
      earlier.fade_level = 29;
      require(unirally::classic_race_prior_fade(now, &earlier, scenario) == 29);
      auto restart = race;
      restart.movement.frame = 1328;
      earlier.movement.frame = 7000;
      earlier.fade_level = 30;
      require(unirally::classic_race_prior_fade(restart, &earlier, scenario) == 0);  // a later frame is not a previous update
    }
    // The countdown driver's selection from what it read (R-0040, the
    // countdown-pause original).
    {
      const auto choose = [](std::uint16_t countdown, bool parity) {
        return unirally::classic_countdown_window(countdown, parity, 6U);
      };
      require(choose(270, false) == 6U && choose(250, true) == 6U && choose(249, false) == 0U && choose(221, false) == 0U);
      require(choose(220, false) == 6U && choose(190, false) == 6U && choose(189, false) == 1U && choose(161, false) == 1U);
      require(choose(160, false) == 6U && choose(130, false) == 6U && choose(129, false) == 2U && choose(101, false) == 2U);
      require(choose(100, false) == 6U && choose(70, false) == 6U);
      // GO: an odd driver update chooses member 3, shown on the even frame after it.
      require(choose(69, true) == 3U && choose(69, false) == 4U && choose(1, true) == 3U && choose(1, false) == 4U);
      require(!choose(0, false) && !choose(0, true));
      // Only the transition member follows the track; the digits and GO do not.
      for (const unsigned countdown : {270U, 250U, 220U, 190U, 160U, 130U, 100U, 70U})
        require(unirally::classic_countdown_window(static_cast<std::uint16_t>(countdown), false, 5U) == 5U);
      for (const unsigned countdown : {249U, 221U, 189U, 161U, 129U, 101U, 69U, 1U})
        require(unirally::classic_countdown_window(static_cast<std::uint16_t>(countdown), false, 5U) ==
                choose(static_cast<std::uint16_t>(countdown), false));
    }
    // ZOOM ZOO (ZOOM-ZOO-WINDOW-EFFECTS): the same drivers with transition
    // member 5 from setup frame 1382, as the M4-16 primary original's $11FD
    // runs: 5 on 1382-1402, 0 on 1403-1431, 5, 1, 5, 2, 5 to 1582, then GO
    // alternating 4/3 from 1583 to 1651 and nothing from 1652; the player's
    // finish at 6484 shows member 8 from 6486 (its first driver update 6485
    // is odd) and 9 from 6488.
    {
      // The transition member comes from the player's start y word in the
      // track header, as the engine's start reflection does.
      std::vector<std::uint8_t> header(16, 0);
      header[5] = 0x5a; header[6] = 0x03;  // even y: reflected start (DRAGSTER's 0x035a)
      require(unirally::classic_race_start_reflected(header, 0) && unirally::classic_window_transition_member(header) == 6U);
      header[5] = 0x5b;                    // odd y: not reflected
      require(!unirally::classic_race_start_reflected(header, 0) && unirally::classic_window_transition_member(header) == 5U);
      header[9] = 0x02;                    // the opponent's start y word (bytes 9-10) does not move the member
      require(unirally::classic_race_start_reflected(header, 1) && unirally::classic_window_transition_member(header) == 5U);
      bool two_riders_only = false;
      try { (void)unirally::classic_race_start_reflected(header, 2); } catch (const std::invalid_argument &) { two_riders_only = true; }
      require(two_riders_only);

      const auto zoom_setup = unirally::classic_race_scenario(ClassicRaceTrack::ZoomZoo).initialization_frame + 6U;
      require(zoom_setup == 1382U);
      auto zoom = race;
      zoom.track = ClassicRaceTrack::ZoomZoo;
      const auto at = [&](std::uint32_t frame) {
        auto s = zoom;
        s.movement.frame = frame;
        return unirally::classic_window_table_index(s, zoom_setup, std::nullopt, 5U);
      };
      require(!at(1381) && at(1382) == 5U && at(1402) == 5U && at(1403) == 0U && at(1431) == 0U && at(1432) == 5U);
      require(at(1462) == 5U && at(1463) == 1U && at(1491) == 1U && at(1492) == 5U && at(1523) == 2U && at(1551) == 2U);
      require(at(1552) == 5U && at(1582) == 5U && at(1583) == 4U && at(1584) == 3U && at(1651) == 4U && !at(1652) && !at(3000));
      const auto finished = [&](std::uint32_t frame, std::uint16_t delay) {
        auto s = zoom;
        s.movement.frame = frame;
        s.race.riders[0].finished = 1;
        s.race.total_times = {9802, 60000};
        s.race.finish_delay = delay;
        return unirally::classic_window_table_index(s, zoom_setup, std::nullopt, 5U);
      };
      require(!finished(6484, 0) && !finished(6485, 1) && finished(6486, 2) == 8U && finished(6487, 3) == 8U &&
              finished(6488, 4) == 9U && finished(6489, 5) == 9U && finished(6490, 6) == 10U);
    }
    // The window pointer followed update by update (DRAGSTER-WINDOW-PAUSE):
    // a synthetic race whose countdown, pause and finish fields advance as
    // the engine's do, checked against what the originals published.
    {
      struct Script {
        std::vector<std::pair<std::uint32_t, std::uint32_t>> pauses; // states with the menu open
        std::uint32_t player_finish{0}, opponent_finish{0}, loading{0};
      };
      const auto publish = [&](const Script &script, std::uint32_t last) {
        std::vector<std::optional<unsigned>> published(last + 1);
        unirally::ClassicWindowPointer pointer;
        auto current = race;
        current.movement.frame = 1328;
        current.movement.countdown = 270;
        for (std::uint32_t frame = 1329; frame <= last; ++frame) {
          auto next = current;
          next.movement.frame = frame;
          next.pause.selection = 0;
          for (const auto &[open, close] : script.pauses)
            if (frame >= open && frame <= close) next.pause.selection = 1;
          const bool diverted = current.pause.selection || next.pause.selection;
          if (diverted) ++next.pause.suspended_updates;  // the engine's clock of diverted updates
          // The countdown handler runs once the fade reaches 5, from update 1333.
          if (!diverted && frame >= 1333 && next.movement.countdown) --next.movement.countdown;
          if (script.player_finish && frame >= script.player_finish) next.race.riders[0].finished = 1;
          if (script.opponent_finish && frame >= script.opponent_finish) next.race.riders[1].finished = 1;
          if (script.loading && frame >= script.loading) next.result_updates = static_cast<std::uint16_t>(frame - script.loading + 1);
          pointer.observe_update(current, next, transition);
          published[frame] = pointer.published();
          current = next;
        }
        return published;
      };
      // countdown-pause original: menu open on states 1400-1500 (resumed on
      // 1501) and 1561-1600 (resumed on 1601): no window on 1401-1502 and
      // 1562-1602, the digit resumes where it stopped, and GO follows the
      // frame parity after the pauses (3 on even frames) until 1746.
      {
        Script script;
        script.pauses = {{1400, 1500}, {1561, 1600}};
        script.player_finish = 3359;
        const auto p = publish(script, 1800);
        require(!p[1333] && p[1334] == 6U && p[1400] == 6U);
        for (std::uint32_t f = 1401; f <= 1502; ++f) require(!p[f]);
        require(p[1503] == 6U && p[1560] == 6U && p[1561] == 6U);
        for (std::uint32_t f = 1562; f <= 1602; ++f) require(!p[f]);
        require(p[1603] == 6U && p[1636] == 2U && p[1704] == 3U && p[1705] == 4U && p[1746] == 3U && !p[1747]);
      }
      // banner-pause-odd original (reviewer's capture): opponent 3214, menu
      // open on states 3240-3299 (61 diverted updates), player 3410: the
      // opponent's driver dies on member 8 at 3636, the player's requests 7 on
      // 3637 and steps from there.
      {
        Script script;
        script.pauses = {{3240, 3299}};
        script.opponent_finish = 3214;
        script.player_finish = 3410;
        script.loading = 3651;
        const auto p = publish(script, 3660);
        require(!p[3215] && p[3216] == 8U && p[3240] == 20U);
        for (std::uint32_t f = 3241; f <= 3301; ++f) require(!p[f]);
        require(p[3302] == 21U && p[3303] == 21U);
        require(p[3410] == 21U && p[3411] == 21U && p[3412] == 22U); // no gap at the second finish
        require(p[3636] == 8U && p[3637] == 7U && p[3638] == 8U && p[3639] == 8U && p[3640] == 9U);
        require(p[3651] == 14U && p[3652] == 14U && p[3653] == 14U); // the last race vblank's member holds through loading
      }
      // random-1 original: opponent 3214, no pause: blank from 3576 until the
      // player's finish at 3636 arms the second driver (member 8 on 3638).
      {
        Script script;
        script.opponent_finish = 3214;
        script.player_finish = 3636;
        script.loading = 3877;
        const auto p = publish(script, 3880);
        require(p[3575] == 7U);
        for (std::uint32_t f = 3576; f <= 3637; ++f) require(!p[f]);
        require(p[3638] == 8U && p[3640] == 9U && p[3876] == 19U && p[3877] == 19U);
      }
      // reversal original: opponent 3325, first driver update even.
      {
        Script script;
        script.opponent_finish = 3325;
        script.player_finish = 4292;
        script.loading = 4533;
        const auto p = publish(script, 4540);
        require(!p[3326] && p[3327] == 7U && p[3328] == 8U && p[3687] == 7U && !p[3688] && !p[4293] && p[4294] == 8U);
      }
      // primary-a (player 3211, opponent 3214) and continuous-right (3213,
      // 3214): the first driver runs on; the second's finish leaves no gap.
      {
        Script script;
        script.player_finish = 3211;
        script.opponent_finish = 3214;
        script.loading = 3452;
        const auto p = publish(script, 3460);
        require(!p[3212] && p[3213] == 7U && p[3214] == 8U && p[3215] == 8U && p[3216] == 9U);
        Script script2;
        script2.player_finish = 3213;
        script2.opponent_finish = 3214;
        script2.loading = 3454;
        const auto q = publish(script2, 3460);
        require(!p[3212] && !q[3214] && q[3215] == 7U && q[3216] == 8U);
      }
    }
    // The result view derives the legacy phases from the shared counters.
    {
      auto shared_state = race;
      shared_state.movement.frame = 1400;
      require(unirally::classic_finish_view(shared_state).phase == RacePhase::Racing);
      shared_state.race.total_times = {60000, 3358};
      shared_state.race.riders[1].finished = 1;
      shared_state.race.riders[1].time_digits = {0, 3, 3, 5, 8};
      auto view = unirally::classic_finish_view(shared_state);
      require(view.rider_finished[1] && !view.rider_finished[0] &&
              view.outcome == unirally::RaceOutcome::Pending && view.phase == RacePhase::Racing);
      shared_state.race.riders[0].finished = 1;
      shared_state.race.riders[0].time_digits = {0, 3, 3, 5, 7};
      shared_state.race.total_times = {3357, 3358};
      shared_state.race.finish_delay = 12;
      view = unirally::classic_finish_view(shared_state);
      require(view.phase == RacePhase::FinishDelay && view.outcome == unirally::RaceOutcome::PlayerWon &&
              view.player_finish_delay == 12 && view.finish_time_centiseconds[0] == 3357);
      shared_state.race.finish_delay = 240;
      shared_state.result_updates = 225;
      require(unirally::classic_finish_view(shared_state).phase == RacePhase::ResultLoading);
      shared_state.result_updates = 226;
      view = unirally::classic_finish_view(shared_state);
      require(view.phase == RacePhase::ResultScreen && view.result_loading_updates == 226);
      shared_state.race.total_times = {3359, 3358};
      shared_state.result_updates = 241;
      require(unirally::classic_finish_view(shared_state).outcome == unirally::RaceOutcome::PlayerLost &&
              unirally::classic_finish_view(shared_state).phase == RacePhase::ResultLoading);
    }
  }
  // RESULT-TITLE-GLYPHS: the printer's table $80:C709 after `EE` gives letters
  // and digits a big 2 x 2 glyph and the name table's other bytes a small one,
  // one tile wide ($80:C56C): DOWN+UP's `+` and BOO!'s `!` as in the
  // original's results, and the underscore as the small font's space.
  require(unirally::result_title_glyph('a').tile == 0x14 &&
          unirally::result_title_glyph('a').big);
  require(unirally::result_title_glyph('o').tile == 0x00 &&
          unirally::result_title_glyph('p').tile == 0x30 &&
          unirally::result_title_glyph('z').tile == 0x44 &&
          unirally::result_title_glyph('3').tile == 0x06);
  const std::array<std::pair<char, unsigned>, 4> small_title_glyphs{
      {{'_', 0xce}, {'!', 0xa0}, {'\'', 0xa3}, {'+', 0xa5}}};
  for (const auto &[glyph, tile] : small_title_glyphs)
    require(unirally::result_title_glyph(glyph).tile == tile &&
            !unirally::result_title_glyph(glyph).big);
  require(unirally::result_title_width("down+up") == 13 &&
          unirally::result_title_width("boo!") == 7 &&
          unirally::result_title_width("flat_fun") == 15);
  bool title_refused = false;
  try {
    (void)unirally::result_title_glyph('#');
  } catch (const std::invalid_argument &) {
    title_refused = true;
  }
  require(title_refused);
  // R-0042: every byte the caption table contains maps to a glyph, and the
  // three runs are 16 apart because each glyph's halves are one font row
  // apart. The voice entries the engine publishes to the player include `"`,
  // so refusing it would end a race the original plays through (review B1).
  require(!unirally::classic_caption_tile(' ').has_value());
  const std::array<std::pair<char, unsigned>, 11> caption_glyphs{
      {{'a', 0x0b}, {'e', 0x0f}, {'f', 0x20}, {'m', 0x27}, {'u', 0x2f},
       {'v', 0x40}, {'z', 0x44}, {'!', 0x60}, {'"', 0x61}, {'-', 0x4d},
       {'j', 0x24}}};
  for (const auto &[glyph, tile] : caption_glyphs)
    require(unirally::classic_caption_tile(glyph) == tile);
  // R-0043: the HUD's own characters come from the same sheet. The digits are
  // read off the original's character table $80:81F4, where 0 follows 9, and
  // `:` and `/` off the two cells the clock and the lap field publish.
  const std::array<std::pair<char, unsigned>, 5> hud_glyphs{
      {{'0', 0x0a}, {'1', 0x01}, {'9', 0x09}, {':', 0x45}, {'/', 0x4e}}};
  for (const auto &[glyph, tile] : hud_glyphs)
    require(unirally::classic_caption_tile(glyph) == tile);
  bool refused = false;
  try {
    (void)unirally::classic_caption_tile('A');
  } catch (const std::invalid_argument &) {
    refused = true;
  }
  require(refused);

  // R-0042: the queue selects the caption, and blanks it two ways. Neither the
  // blanking nor the entry lookup had a tracked check (re-review C6).
  {
    std::vector<std::uint8_t> table(4080, ' ');
    for (int i = 0; i < 16; ++i)
      table[static_cast<std::size_t>(13 * 16 + i)] = static_cast<std::uint8_t>("   wipeout      "[i]);
    unirally::ZoomZooState published{};
    auto &queue = published.player_announcements.queue;
    require(!unirally::classic_caption_entry(published, table).has_value());
    queue.read_cursor = 3;
    queue.entries[3] = 14;
    const auto shown = unirally::classic_caption_entry(published, table);
    require(shown.has_value() && std::equal(shown->begin(), shown->end(),
                                            std::string_view("   wipeout      ").begin()));
    published.player_announcements.empty_display = 1;
    require(!unirally::classic_caption_entry(published, table).has_value());
    published.player_announcements.empty_display = 0;
    queue.entries[3] = 0;
    require(!unirally::classic_caption_entry(published, table).has_value());
    // A pack whose caption entry is the wrong size is refused rather than
    // indexed (re-review E10: this mutation survived the first test).
    queue.entries[3] = 14;
    std::vector<std::uint8_t> short_table(4064, ' ');
    require(!unirally::classic_caption_entry(published, short_table).has_value());
    std::vector<std::uint8_t> long_table(4096, ' ');
    require(!unirally::classic_caption_entry(published, long_table).has_value());
  }

}
