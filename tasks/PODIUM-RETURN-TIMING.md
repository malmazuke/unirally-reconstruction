# PODIUM-RETURN-TIMING - frame overruns in the league's menus

## Assignment

- Status: **claimed** 27 October 2026 11:07 UTC on main `6155f3d2`, on the user's instruction
  (Claude Code desktop app, Claude Opus 5.5 `claude-opus-5-5`; coordinator and integrator; a worker
  subagent implements, a fresh subagent reviews). Queued 16 October 2026 by
  [LEAGUE-TOURS](LEAGUE-TOURS.md) (R-0098). Run under
  [D-0013](../docs/decisions/D-0013-task-time-and-disk-budget.md): claim to merge against the
  6-hour target, new evidence under 1 GB.
- Choice: the next queued player-visible gap; NAME-CHECK (R-0101) built the frame-cost method
  (a routine's master clocks against the frame's end) this task can reuse.
- Branch `task/podium-return-timing` in `.worktrees/podium-return-timing`; evidence in main
  `local/evidence/podium-return-timing/`.
- Task provider: Anthropic.
- Milestone: M4 (original game coverage).
- Review tier: **1** (menu timing in `src/core`).
- Dependencies: LEAGUE (R-0073), LEAGUE-TOURS (R-0098), D-0010 (upload timing).

## Why

After LEAGUE-TOURS every league race equals the original, but most later-tour menu pictures
differ by a frame, from two effects R-0098 records and native does not model:
- **The podium's way back.** In 4 of 11 podiums, and in 15 of 40 of the review's shifted exits,
  the original restores the menus a frame later. The title sound upload ends in one of two
  frames, with no pattern in the podium's phase or frame parity. After it the arrow's spin, and
  after a late one the palette cycle, stay a frame off until the league ends.
- **An event's pairs a frame apart.** In every observed event of a league with more than one
  pair (15 of 15, leagues of 3, 4 and 5), the standings after each pair but the last start a
  frame later. Native also counts some later pairs a frame early.

## Added by RECORD-SCREENS (24 October 2026)

- **RECORDS' entry with dense statistics.** `$80:D527` calls `$83:97BD`, whose 48 `$83:9D80`
  percentages run past RECORDS' entry frame when the riders have many races, so the original's
  menus are a frame behind native's from there (review capture `wr-stats2` from 901, main
  `local/evidence/record-screens-review/`; on main too). The same frame-cycle model would predict
  it ([R-0100](../docs/research/R-0100-record-screens.md) Not covered).

## Outcome

- Model what predicts each effect (the sound processor's handshake, or the frame's cycle count),
  or a fixed rule if the captures support one.
- Check each model against LEAGUE-TOURS' captures `tours-a`, `tours-b`, `odd-tours`, `five` and
  `four`, with the first tour's frozen gate unchanged.

## Checkpoint (worker, 27 October 2026)

What predicts each effect ([R-0102](../docs/research/R-0102-menu-frame-overruns.md)):

1. **An event's pairs a frame apart: the frame's cycle count, modelled.** A pair's way out runs
   `$80:C786` (one checksum run, one more per time placed, a little more for a lap race), the
   count (another checksum run) and the standings' print (by rows and the units of its numbers)
   with no frame wait between. 27 way outs traced (`odd-tours` 9, `four` 4, `five` 14); the model
   (`src/core/menu_overruns.cpp`) puts all 27 counts and standings' waits in their measured
   frames, within 2,007 clocks, the nearest boundary 1,453 clocks away. Three-member standings are
   late after two placings once their digits grow (every event but `odd-tours`' first); four- and
   five-member ones always; one placing counts the pair a frame later. A five-row print elsewhere
   (from PICK TOUR, the awards) runs past its frame too (`five`: 7 standings).
   - Before (main): the arrow's lag changes at 6 of `odd-tours`' 9 standings, all 4 of `four`'s and
     every one of `five`'s 14 plus 7 other five-row prints. After: none.
2. **RECORDS' entry with dense statistics: the frame's cycle count, modelled.** `$83:97BD`'s CPU
   time is `32,362 + 34 x stores + 7,704 x percentages + 330 x halvings + 112 x quotient bits`
   (per call exact on all 39 of `wr-stats2`'s; totals within 40 clocks on seven entries). About 19
   or more percentages push RECORDS' frame wait a frame on. `wr-stats2`: pictures 800 of 800
   from 900 (main 517); the arrow's lag no longer changes (main: 799 frames off).
3. **The podium's way back: the sound processor, a limit.** The title's upload takes a constant
   23,235,246-23,236,014 clocks over 12 presses of one podium; its start, after the driver's reply
   to the request, varies 123,796-194,290 clocks into its frame and decides the frame (late from
   about 170,400: 4 of 12). Native has no state that gives the driver's reply; R-0095's phase rule
   stays (the first tour's frozen gate relies on it).

Captures and results (`checks.list`, run once at `92d2c923`, `checks-92d2c923.out`; candidate and
main `6155f3d2`, both on pack v41):

| Capture | Pictures equal: candidate / main | Arrow's lag changes outside the limits: candidate / main | League races, league fields |
|---|---|---|---|
| `odd-tours` w0, w1 (3 members) | 5,678 / 4,912 of 6,459; 6,010 / 4,934 of 7,003 | 0 / 4; 0 / 4 | all equal; fields 0 differing (main 1 in w0) |
| `four` w0 (4) | 5,153 / 4,352 of 5,160 | 0 / 4 | all equal; 0 |
| `five` w0, w1 (5) | 7,334 / 5,465 of 12,377; 3,022 / 2,950 of 9,600 | 0 / 24; 0 / 883 (w1 to 127,125) | all equal; w0 0 (main 2), w1 7,200 both (another race result from 127,125, R-0098) |
| `tours-a` w0-w8, `tours-b` w0-w1 (2 members) | equal to main in every window (e.g. w0 3,096 of 3,206) | 0 / 2 per podium (all at the podium's way back) | all equal; 0 |
| `t-crawler`, `podium-back`, `backs`, `picks-l0/1/2` | (lag only) | 0 / 0 | - |
| `wr-stats2` (RECORDS, dense) | 800 / 517 of 800 | 0 / 1 (799 frames off) | - |
| `wr-stats` (RECORDS, none) | 800 / 800 of 800 | 0 / 0 | - |

The remaining differences: the podium's way back (finding 5) in every two-member window and after
each podium; a race's return two frames apart in `five` 19,780, as on main (after the race R-0098 lists as
loading a frame off at 16,596; R-0077), which leaves `five` w0's later menus two frames apart; HUNTER's two-view pictures (SPLIT-HUNTER-PICTURES) and the odd league's
computer result objects (R-0098); one text-map frame per late print. `model.py`: 27 of 27 way outs
and 7 of 7 RECORDS entries in their measured frames. LEAGUE's frozen gate (first tour, pack v40):
passed.

## Handoff (worker)

- Branch `task/podium-return-timing`, base claim `540cd51e` on main `6155f3d2`. Commits: `92d2c923` (the cost models, the late frames: `menu_overruns.cpp`, RECORDS' entry, the
  way out and the five-row print), then the records commit (native tests, R-0102, the maps, this
  record).
- Files: `src/core/menu_overruns.{hpp,cpp}` (new), `league_menu.cpp` (`time_league_exit`, the
  standings' late frame), `race_result.cpp`, `options_menu.cpp`, `front_end.cpp`, `front_end.hpp`,
  `front_end_screens.hpp`, `CMakeLists.txt`, `tests/native/front_end_tests.cpp`
  (`menu_overrun_tests`), R-0102, regenerated `docs/map/static/`. No pack change (v41 or v40 both
  work); `tools/unirally_lab/` unchanged.
- Checks run: `ctest` app-debug 41/41 and lab-debug 41/41 (new `menu_overrun_tests` in
  `front_end_tests`); `clang-tidy` (LLVM 19) clean on `menu_overruns.cpp`, `league_menu.cpp`,
  `race_result.cpp`, `options_menu.cpp`, `front_end.cpp`, `front_end_tests.cpp`; `clang-format` 19
  applied to the `src/core` files; `coverage native-symbols --check` passed, 0 cited addresses
  without a record (`regen.sh`, the five raw coverage inputs); tooling tests 618 OK (no Python under
  `tools/` changed); `checks.list` 44 of 44 lines rc 0 (`checks-92d2c923.out`, 1 h 20 min, under
  the heavy-run lock). Synthetic suite and gates not run by the worker.
- Evidence: main `local/evidence/podium-return-timing/` 3.7 MB (scripts, traces, `pairs-data.json`,
  `cycles-records-*.json`, `podium-upload-cf5.json`, `base-6155f3d2/` main's runner); no new
  captures, so nothing to strip.
- Time (claim 11:07 UTC to handoff 13:17 UTC, 2 h 10 min wall clock (harness-measured)): reading 10 min;
  measurements and models (lag tool, the 27 way outs, RECORDS' cycles, the podium's uploads) about
  45 min; native code and tests 15 min; `checks.list` written and run 1 h 20 min, records and
  maps alongside it.
- Next: the tier-1 review, then the gates. The gate catalogue does not hold `checks.list` yet.
