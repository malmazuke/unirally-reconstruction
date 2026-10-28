# PODIUM-RETURN-TIMING - frame overruns in the league's menus

## Assignment

- Status: **accepted** 27 October 2026 (tier 1, [#88](https://github.com/malmazuke/unirally-reconstruction/pull/88),
  approved in review round 6 at `2405d91f`), integrated by merge commit. Claimed 11:07 UTC on main
  `6155f3d2`, on the user's instruction
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

Captures and results (`checks.list`, run once at `92d2c923`, the tree `2237a40f` holds after its message was reworded, `checks-92d2c923.out`; candidate and
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
- Units 23, which the band had not covered, traced twice (`band-r2/`): 8, 7, 6 and 7, 7, 7.
- This round's edge (units 22 on time, 23 late but a three-way tie, the nearest boundary 78 and
  "no model can have more margin than the original's 92 at 22") was wrong: it took each trace's
  frame from its wait's entry. Review round 3 corrects it, below.
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

## Review round 3 (returned on `f9d99b39`) - the fix

Review: main `local/evidence/podium-return-timing/review-3/review.md` (changes requested, finding 1
only; the leading zeros' 200 clocks a pass and their count confirmed from the listing). The
model and the band's measured frames both took the frame the standings' wait is entered in, but
the wait (`$80:FADF`) polls the sound queue (`JSL $82:8035`) before its first `$4212` read, 576-616
clocks later here, and the read decides. Measured by the first `$80:FAF1` after the print (the
reviewer's `wait_exit.py` over the 44 way outs, and four new band traces 5,4,3, 5,5,3, 6,5,3 and
6,5,4 with read labels, `review-3/band/`): units 14 and 15 on time, 16, 17, 18, 22 and the tie
7,7,7 late (entered 92-514 clocks before the frame's end).

Fix in `136fb4fd` (code; see the handoff):
- `standings_wait_clocks` adds the poll (596 clocks, the middle of the traced 576-616) and
  returns the first read's clock, whose frame is the standings' frame. `model.py` takes each
  trace's measured frame from its first `$80:FAF1` after the print, less one.
- The band's prints split by ties as well (no tie about 125 clocks over one tie, a three-way tie
  about 145 under): `StandingsWork::new_ranks` (distinct points less one) costs 100 clocks a
  rank, fitted on the band, not traced.
- Refit (`pairs_refit.py`: the largest margin with the entry error within 1,500): 221,930 + 39,650
  a row. All 44 way outs (27 and the band's 17) are in their measured frames; the model's wait
  entry is within -12 to +75 clocks of the band's, -1,327 to +1,490 of the leagues'; the nearest
  boundary of a modelled read is 92 clocks (6, 5, 3, late; 5, 5, 3 and 5, 4, 3 101 and 93 on the
  on-time side; the band's units are 93 clocks apart). The edge for three members, two
  placings, a one-run race and one-digit points: units 15 on time and 16 late with no tie
  (16/17 with one tie, 17/18 with a three-way tie).
- `KNOWN = {'band p777'}` and the tie claims are gone: 7, 7, 7 is late, as the model says.
  `menu_overrun_tests` pins 15 (5, 5, 3) and 14 (5, 4, 3) in frame 3, 16 (6, 5, 3) and 23
  (7, 7, 7) in frame 4.
- RECORDS' model already timed the first read (`cycles_records.py` measures `$80:FAE3`), so its
  frames do not change; R-0102 says so. R-0102 notes that R-0101's keyboard check uses the same
  method, the entry-or-read question there an open follow-up (no NAME-CHECK change).
- R-0102's "22 on time, 92 clocks", "no model can have more margin than 92" and "23 late but a
  three-way tie" are removed; Not covered records the knife edge (units about 14-17 for this way
  out; ties, other names, tens digits and the poll's spread) with the band (17 traces).

Rerun (`run_checks.sh OUTDIR 'league-tours|model|frozen'`, the 40 league, model and frozen-gate
lines, `checks-r3.out` at `136fb4fd`): 40 of 40 lines rc 0, 1 h 22 min; every picture, race,
field and lag count the same as the earlier runs' (no league way out's frame moves; the edge moved
only where the band, not these captures, has standings). RECORDS' lines not rerun (that model's frames are
unchanged). `ctest` app-debug and lab-debug 41/41; `clang-tidy` clean on `menu_overruns.cpp` and
`league_menu.cpp`; `clang-format` applied; `coverage native-symbols --check` passed after
`regen.sh`.

## Handoff (worker, review round 3)

- Commits: `136fb4fd` (the poll, the ranks, the refit, the tests), then the records commit
  (R-0102, this record, `model.py`, the maps).
- Time (harness clock): 16:56 UTC (the review's return) to 18:23 UTC, about 1 h 27 min: the model's read target, the
  rank term and the refit about 20 min, code and tests 10 min, records 20 min; the league rerun
  1 h 22 min under the lock, records alongside it.
- Evidence: the reviewer's `review-3/` (band traces, `wait_exit.py`); `pairs_refit.py` updated;
  `checks-r3/`.

## Review round 4 (returned on `e5865232`) - the fix

Review: main `local/evidence/podium-return-timing/review-4/review.md` (changes requested; the poll
term, the RECORDS claim and the rank term confirmed, the rank term for rows in points order). The
edge of round 3 holds only when the members' rows already stand in points order: the standings'
selection sort (`$80:EFD4`) stores the lowest so far (`$80:EFE9-EFF4`) for each points word at or
below it, over all eight rows, so its cost depends on the rows' order. Seven new reviewer traces
(`review-4/band/`): 3, 5, 6, 5, 3, 6 and 3, 6, 5 at units 16 are on time in the original where the
model said late; 8, 8, 6, 8, 6, 8, 7, 8, 8 and 9, 8, 5 late, as it said.

Fix in `a359ad0c`:
- `standings_order` counts the sort's stores (`StandingsWork::sort_stores`, 36 for rows in points
  order, 24 for 3, 5, 6). A store costs 55 CPU clocks with the refresh, one fitted figure: the
  listing gives 58 for a lower word and 48 for an equal one, but over the band's 24 traces the
  split fits within about 119 clocks and one figure of 54-56 within about 48, so the band does not
  support the split.
- Refit (`pairs_refit.py`): 220,040 + 39,600 a row. All 51 traced way outs (27 and 24 band) are in
  their measured frames (`model.py`, `model-r4.txt`); the model's wait entry is within -12 to +75
  clocks of the band's and -1,428 to +710 of the leagues' (from -1,327 to +1,490 without the
  sort); the nearest boundary of a modelled read is 92 clocks (6, 5, 3), with 5, 5, 3 101 and
  5, 4, 3 94 on the on-time side and the three units-16 traces in other orders 305-645.
- The edge, three members, two placings, a one-run race, one-digit points: units 15 on time and 16
  late with the rows in points order; other orders move it up (5, 3, 6 19/20, 3, 5, 6 and 3, 6, 5
  22/23). R-0102, the code's comments and Not covered say so; the `new_ranks` comment says it
  counts distinct values, which are the new ranks because the rows print in points order.
- `menu_overrun_tests` adds 3, 5, 6 at units 16 in frame 3.

Checks: `model.py` 51 of 51 (rc 0); `ctest` app-debug and lab-debug 41/41; `clang-tidy` clean on
`menu_overruns.cpp` and `league_menu.cpp`; `clang-format` applied; `coverage native-symbols
--check` passed after `regen.sh`. `checks.list` not rerun this round, on the coordinator's
instruction (no measured league way out has moved frame in three rounds; the gates run all of it
after the review).

## Handoff (worker, review round 4)

- Commits: `a359ad0c` (the sort stores, the refit, the test), then the records commit (R-0102,
  this record, `model.py`, the maps).
- Time (harness clock): 18:30 UTC (the review's return) to 18:34 UTC by `date -u` in the session.
- Evidence: the reviewer's `review-4/` as it is; `model.py` (the sort term and the 7 traces),
  `pairs_refit.py` (the band's row orders), `model-r4.txt`.

## Review round 5 (returned on `3e443dc2`) - the fix

Review: main `local/evidence/podium-return-timing/review-5/review.md` (changes requested for one
term; the sort's store count confirmed exact, the single 55 clocks accepted, the edge by row
order matching the band). The leagues' errors (-1,428 to +710) were not "other names and
content": every large one lines up with the NMI that steps the palette cycle (`$00C8` run out),
which costs about 1,100 clocks more than an ordinary NMI. The four prints off by -921 to -955 cross
a stepping NMI; the ten off by -744 to -1,384 before the print cross one in the records or count,
or start after one (`review-5/upstream.txt`, `nmi-phase.txt`). About 2 in 7 two-placing way outs
meet a step, about 12 units at the edge.

Modelled (option one), in `2ee06f29`:
- `LeagueExitWork::palette_delay` is native's `$00C8` as the records' frame's NMI leaves it (none
  while the cycle does not run); the way out adds 1,100 clocks at the records' start when that
  NMI stepped (delay 6) and at each frame end its records, count and print cross whose NMI steps
  (the k-th end from the frame's start when k = delay + 1, mod 7). The standings' print elsewhere
  (`standings_print_frames_late`) takes the same term. The 1,100 is fitted on the 13 outliers, not
  traced. The rule was checked against the 27 way outs' sampled `$00C8`: the step falls at the
  frame end the reviewer's split puts each error at.
- Refit (`pairs_refit.py`, the largest margin with the leagues' error within 600): 220,340 + 39,500
  a row. All 51 traced way outs are in their measured frames (`model.py`, `model-r5.txt`); the
  model's wait entry is within -12 to +75 clocks of the band's and -528 to +510 of the leagues'
  (from -1,428 to +710); the nearest boundary of a modelled read is 92 clocks (6, 5, 3). The
  edge for rows in points order stays at units 15/16 with no step in the way out.
- R-0102's attribution of the leagues' error to "other names and content" is corrected: the step
  is a finding of its own, and what remains (-528 to +510) is put to each league's names and other
  content; Not covered names the step's fitted cost and its effect on the edge.
- `menu_overrun_tests`: 5, 4, 3 (on time with the band's delay 3) comes a frame later with the
  delay at 1, whose step falls during the count, and stays on time with the cycle not running; a
  four-row print elsewhere about 1,000 clocks short of its frame's end runs over after an NMI that
  stepped. A step inside a three-row two-placing print cannot move it a frame: such a print only
  crosses a frame end when it is already late, so the test steps in the count instead.

Checks: `model.py` 51 of 51 (rc 0); `ctest` app-debug and lab-debug 41/41; `clang-tidy` clean on
`menu_overruns.cpp` and `league_menu.cpp`; `clang-format` applied; `coverage native-symbols
--check` passed after `regen.sh`. `checks.list` not rerun, on the coordinator's instruction.

## Handoff (worker, review round 5)

- Commits: `2ee06f29` (the palette step, the refit, the tests), then the records commit (R-0102,
  this record, `model.py`, `pairs_refit.py`, the maps).
- Time (`date -u` in the session): 18:38 UTC (the review's return) to 18:43 UTC.
- Evidence: the reviewer's `review-5/` as it is (`model.py` reads its `nmi-phase.txt`);
  `model-r5.txt`.

## Review round 6 (returned on `2405d91f`) - approved

Review: main `local/evidence/podium-return-timing/review-6/review.md`. The verdict is approve,
with one records correction. The checks:
- The step rule matches native's palette code: the NMI that finds 0 steps, the cycle is 7 frames
  long, and 6 means the frame's own NMI just stepped.
- The cycle-not-running case holds.
- The fitted 1,100 clocks are stated as fitted.

The correction: R-0102 said native keeps `$00C8` in the original's phase. It does not always. The
reviewer ran native over the five windows:

| Capture | Native's delay equals the original's |
|---|---|
| `odd-tours` | 9 of 9 |
| `four` | 4 of 4 |
| `five` | 2 of 14 |

In `five`, native's delay is two frames off from the race that returned two frames apart at
19,780 onward. With native's delays, all 51 traces stay in their frames, but the league error
widens to -961 to +1,610 clocks.

R-0102 now states these measurements. Its limit adds that where the cycle is off (after a race
returned frames apart, or one frame off after each late podium), the step can land at the wrong
frame end, moving a way out near the edge by about 12 units. This is a records-only change by the
coordinator, made without a re-review, as the reviewer allowed.

## Review (rounds 1-6, approved `2405d91f`)

A fresh subagent in its own checkout at the PR head. It traced the original with edited league
points (a band of 24 way outs over the rounds) to test the standings model at the edge.

| Round | Head | Verdict | What it found |
|---|---|---|---|
| [1](https://github.com/malmazuke/unirally-reconstruction/pull/88#pullrequestreview-5667974616) | `2af37d34` | changes | The standings' NMI hidden in the fitted constant, so three members were late at ordinary scores. Also: the podium count 5 of 12, main's column without the limits, duplicated count rules. |
| [2](https://github.com/malmazuke/unirally-reconstruction/pull/88#pullrequestreview-5669609984) | `15791894` | changes | Units 24-27 traced late where the refit said on time: the leading-zero pass was missing. |
| [3](https://github.com/malmazuke/unirally-reconstruction/pull/88#pullrequestreview-5671106360) | `f9d99b39` | changes | The frame wait polls the sound queue (about 596 clocks) before its first `$4212` read, so the edge is units 15/16, not 22/23. |
| [4](https://github.com/malmazuke/unirally-reconstruction/pull/88#pullrequestreview-5672342374) | `e5865232` | changes | The selection sort's stores depend on row order; reordered points at units 16 are on time. |
| [5](https://github.com/malmazuke/unirally-reconstruction/pull/88#pullrequestreview-5672444925) | `3e443dc2` | changes | The NMI that steps the palette cycle costs about 1,100 clocks; it explains every large league error. |
| [6](https://github.com/malmazuke/unirally-reconstruction/pull/88#pullrequestreview-5672566652) | `2405d91f` | approve | One records correction: native's `$00C8` matches the original's at 9/9, 4/4 and 2/14 way outs, not always (fixed in `379a664d`). |

Each round's fixes are in the sections above. The model ends with all 51 traced way outs in their
measured frames and the edge, its row-order and palette-step shifts recorded as limits in R-0102.

## Gates (D-0012)

`local/locks/heavy-run.sh python3 -m tools.unirally_lab.gates run --task-checks
local/evidence/podium-return-timing/checks.list` at `379a664d` (main
`local/evidence/podium-return-timing/gates-1*`, 2 h 32 min). No pack change, so no `--base-pack`.
- Every selected step passed or matched main except 7 that changed, all as this task intends:
  - Pictures equal to the original, candidate (main): `lt-odd-tours-w0` 5,678 of 6,459 (4,912),
    `lt-odd-tours-w1` 6,010 of 7,003 (4,934), `lt-five-w0` 7,334 of 12,377 (5,465), `lt-five-w1`
    3,022 of 9,600 (2,950). In `odd-tours-w0` and `five-w0` main's league fields also drift from
    the original (`played_points`, and `pair_cursor` in `five`); the candidate's do not.
  - `rs-review-wr-stats2`: 800 of 800 pictures, main 51 against the original.
  - `pa-cues`: `pa-odd-tours` 2,406 frames differ, main 2,415 (the same as main elsewhere).
  - `pa-pcm-pa-odd-tours`: 2,400,194 identical pairs, main 2,399,853; within 1 dB 39,533, main
    39,139.
- `gates-2*`: those 7 with `--expect-changed`, `status=passed` (14 min).
- Hosted CI green on all three checks.
- Not done here: the catalogue (`tools/unirally_lab/gates/catalogue.py`) holds neither this task's
  checks nor NAME-CHECK's; they go in with the next catalogue change or `--full --record` run.

## Handoff (closeout)

- Commits on `task/podium-return-timing`: `540cd51e` (claim), `2237a40f`, `2af37d34` (worker),
  `811cd4f7`, `15791894`, `3084f6be`, `f9d99b39`, `136fb4fd`, `e5865232`, `a359ad0c`, `3e443dc2`,
  `2ee06f29`, `2405d91f` (review rounds 1-5's fixes), `379a664d` (round 6's records correction), then
  this record.
- Time, claim 11:07 UTC to merge about 21:50 UTC, about 10 h 45 min against D-0013's 6 hours:
  - worker 2 h 10 min;
  - five fix rounds 4 h 40 min, of which about 4 h were the `checks.list` reruns of rounds 1-3
    (1 h 15-22 min each);
  - six review rounds about 55 min;
  - gates 2 h 32 min, the rerun 14 min;
  - closeout about 15 min.
- Lesson: between review rounds the full `checks.list` rerun bought nothing, since the gates run it
  again and no measured frame moved in any round. Rounds 4 and 5 skipped it and took 5 minutes
  each. A fix round should rerun only `model.py`, the tests and lints, and leave the captures to
  the gates.
- Disk: main `local/evidence/podium-return-timing/` 193 MB with gate output, under the 1 GB budget;
  compacted at closeout.
- Follow-up, closed 28 October 2026: R-0101's (NAME-CHECK's) keyboard model already counts clocks
  to the frame wait's first `$4212` read (`$80:FAE3`), so the poll is in its tail constants;
  `model.py` still places all 24 calls in their measured frames. Nothing to change.
- The closing session removes `.worktrees/podium-return-timing` and
  `.worktrees/podium-return-timing-review` and the local branch.
