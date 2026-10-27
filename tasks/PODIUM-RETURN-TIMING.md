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
   about 170,400: 5 of 12, delays 1, 2, 5, 8 and 9). Native has no state that gives the driver's reply; R-0095's phase rule
   stays (the first tour's frozen gate relies on it).

Captures and results (`checks.list`, run once at `92d2c923`, `checks-92d2c923.out`; candidate and
main `6155f3d2`, both on pack v41; main's lag counted under the same recorded limits, as review
round 1 corrected):

| Capture | Pictures equal: candidate / main | Arrow's lag changes outside the limits: candidate / main | League races, league fields |
|---|---|---|---|
| `odd-tours` w0, w1 (3 members) | 5,678 / 4,912 of 6,459; 6,010 / 4,934 of 7,003 | 0 / 2; 0 / 4 | all equal; fields 0 differing (main 1 in w0) |
| `four` w0 (4) | 5,153 / 4,352 of 5,160 | 0 / 4 | all equal; 0 |
| `five` w0, w1 (5) | 7,334 / 5,465 of 12,377; 3,022 / 2,950 of 9,600 | 0 / 19; 0 / 5 (w1 to 127,125) | all equal; w0 0 (main 2), w1 7,200 both (another race result from 127,125, R-0098) |
| `tours-a` w0-w8, `tours-b` w0-w1 (2 members) | equal to main in every window (e.g. w0 3,096 of 3,206) | 0 / 0 (the only changes are at the podium's way back, a limit) | all equal; 0 |
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

- Branch `task/podium-return-timing`, base claim `540cd51e` on main `6155f3d2`. Commits: `2237a40f` (the cost models, the late frames: `menu_overruns.cpp`, RECORDS' entry, the
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
  `tools/` changed); `checks.list` 44 of 44 lines rc 0 (`checks-92d2c923.out` at the same tree, 1 h 20 min, under
  the heavy-run lock). Synthetic suite and gates not run by the worker.
- Evidence: main `local/evidence/podium-return-timing/` 3.7 MB (scripts, traces, `pairs-data.json`,
  `cycles-records-*.json`, `podium-upload-cf5.json`, `base-6155f3d2/` main's runner); no new
  captures, so nothing to strip.
- Time (claim 11:07 UTC to handoff 13:17 UTC, 2 h 10 min wall clock (harness-measured)): reading 10 min;
  measurements and models (lag tool, the 27 way outs, RECORDS' cycles, the podium's uploads) about
  45 min; native code and tests 15 min; `checks.list` written and run 1 h 20 min, records and
  maps alongside it.
- Next: the tier-1 review, then the gates. The gate catalogue does not hold `checks.list` yet.

## Review round 1 (returned on `2af37d34`) - the fixes

Review: main `local/evidence/podium-return-timing/review-1/review.md` (changes requested; the
reviewer's refit in `review-1/fit/`, its band traces in `review-1/band/`). Fixes in `811cd4f7`
(code) and the records commit after it.

1. **Blocking, the three-member boundary:** fixed. The first model put a three-member way out
   after two placings in a one-run race a frame late from standings' digits of 19; the
   reviewer's band (odd-tours' first way out with its points edited) measured 12, 18 and 22
   (twice) on time and 28 (twice) late. The cause was the NMI inside a print that crosses a
   frame's end (1,060 clocks), absorbed by the fitted constant. The model now adds the NMI per
   frame end the print passes, costs the decimal conversion's loops at their traced CPU clocks a
   unit (`digit_steps.py`: 122 for the tens and above, 90 for the units, against the review's
   listing count of 108 and 80) and refits the title and row constants over the 34 way outs
   (221,460 + 41,320 a row). All 34 counts and standings' waits were in their measured frames,
   error -1,689 to +1,694 clocks. Its claimed edge (units 27 on time with 89 clocks to spare, 28
   late) and nearest boundary (454 clocks) were wrong: review round 2 measured 24-27 late, see
   below. `model.py` checks the band's 7 traces too; `menu_overrun_tests` tests 22 (frame 3) and
   28 (frame 4).
2. **Podium count:** corrected to 5 of 12 (delays 1, 2, 5, 8, 9) in R-0102 and the Checkpoint;
   the reviewer's sawtooth (the driver answering at a poll about every 75,000 clocks) is in
   R-0102 finding 5 as the next experiment for the limit.
3. **Results table:** `prt_one.sh steps` now gives main the same recorded limits. Main's lag
   changes outside them: `odd-tours` 2 and 4, `four` 4, `five` 19 and 5, `tours-a` and `tours-b`
   0, `wr-stats2` 1 (the candidate 0 everywhere); R-0102's table and the Checkpoint's restate it.
4. **Shared rules:** `played_after_count` and `another_pair` serve both `count_league_pair` and
   `time_league_exit`; `league_awards_scoring_frame` is the awards' frames' one source; `[]`
   throughout.
5. **Notes:** `show_league_table` takes the late frames, so `finish_league_pair` no longer sets
   and clears them; one anonymous namespace; an explicit 0 for a rider without races; no row
   clamp (the print is title + rows, so two rows cost less, as they do); `statistics_frames_late`
   alongside `table_frames_late`; `model.py`'s docstring names `menu_overruns.cpp`.

Rerun (`checks.list`, all 44 lines, `checks-r1.out` at `811cd4f7`): 44 of 44 lines rc 0, 1 h 15 min. Every picture count, race and field count
is the same as the first run's (no measured way out changes frame; the edge moved only where
the band, not these captures, has standings).
Also `ctest` app-debug and lab-debug 41/41, `clang-tidy` clean on the changed files, `clang-format`
applied, `coverage native-symbols --check` passed after `regen.sh`.

## Handoff (worker, review round 1)

- Commits: `811cd4f7` (the refit, the shared helpers), then the records commit (R-0102, this
  record, the maps).
- Time: from 13:42 UTC (the review's return) to 15:12 UTC (about 1 h 30 min): the band's digit loops and the refit
  about 20 min, code and tests 15 min, records 15 min, the rerun 1 h 15 min under the lock, alongside.
- Evidence: main `local/evidence/podium-return-timing/`  53M (adds `digit_steps.py`, `checks-r1/`).

## Review round 2 (returned on `15791894`) - the fix

Review: main `local/evidence/podium-return-timing/review-2/review.md` (changes requested, finding 1
only; findings 2-4 confirmed fixed, the 122 and 90 clocks a unit confirmed as the listing's
cycles at SlowROM speed). The reviewer traced units 24, 25, 26 and 27 (`review-2/band/`): all late
in the original, all on time in round 1's refit, whose edge (27/28) lay in the band's gap between
22 and 28. The missing term: one-digit words ran 417-590 clocks under the model, two-digit ones
21-24 over.

Fix, option (a), in `3084f6be` (code; see the handoff for the commit list):
- `$83:8C64-8C76` blanks each leading zero of a word's five digits but the last; the instruction
  trace (`digit_steps.py`) gives it 200 CPU clocks a pass. `StandingsWork::leading_zeros` counts
  them (4 for a one-digit word, 3 for two digits).
- The title and row constants are refitted for the smallest largest error over the review's 38
  way outs (`pairs_refit.py`): 222,000 + 39,650 a row. All 38 are in their measured frames; the
  error over the band is -125 to +58 clocks (-1,257 to +1,260 over the leagues, whose other
  content varies).
- Units 23, which the band had not covered, traced twice (`band-r2/`): 8, 7, 6 is late (as the
  model says) and 7, 7, 7 on time. Its print is 270 clocks quicker: a three-way tie prints three
  firsts, and the ordinals' cost is not modelled. `model.py` places 39 of 40 and names that one
  as a recorded limit; it exits 1 on any other.
- The edge, for three members, two placings, a one-run race and one-digit numbers: units 22 on
  time (the original 92 clocks before the frame's end, the model 78), 23 late (but a three-way
  tie), 24 and up late. The nearest boundary is 78 clocks, set by the original's own 92 at 22: no
  model can have a margin of a few hundred there, so R-0102's Not covered records the knife edge
  (units about 20-26, ties, other names) with the band as evidence. `menu_overrun_tests` tests
  22 (frame 3), 23 and 24 (frame 4).
- R-0102's and this record's "27 with 89 to spare" and "nearest boundary 454" are corrected.

Rerun (`run_checks.sh OUTDIR 'league-tours|model|frozen'`, the 40 league, model and frozen-gate
lines, `checks-r2.out` at `3084f6be`): 40 of 40 lines rc 0, 1 h 20 min; every picture, race,
field and lag count the same as the earlier runs' (no measured way out changes frame). RECORDS' lines not rerun (that path is unchanged).
`ctest` app-debug and lab-debug 41/41; `clang-tidy` clean on `menu_overruns.cpp` and
`league_menu.cpp`; `clang-format` applied; `coverage native-symbols --check` passed after
`regen.sh`.

## Handoff (worker, review round 2)

- Commits: `3084f6be` (the leading zeros, the refit, the tests), then the records commit
  (R-0102, this record, the maps).
- Time (harness clock): 15:20 UTC (the review's return) to 16:48 UTC, about 1 h 30 min: the zero loop's trace, the refit and
  the units-23 traces about 25 min, code and tests 10 min, records 15 min; the league rerun
  1 h 20 min under the lock, records alongside it.
- Evidence added: `band-r2/` (the two units-23 traces and their scripts), `pairs_refit.py`,
  `digit_steps.py`'s zero loop, `checks-r2/`.
