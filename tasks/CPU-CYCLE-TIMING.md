# CPU-CYCLE-TIMING - the race and menu effects that depend on CPU cycles

## Assignment

- Status: **claimed** 11 November 2026 07:24 UTC on main `a804372d`, on the user's instruction
  ("Next task", after SOUND-PROCESSOR-TIMING #102). Queued 8 November 2026 by
  [COVERAGE-CENSUS](COVERAGE-CENSUS.md) (R-0109, queue item 6). Claude Code desktop app, Claude
  Opus 5.5 (`claude-opus-5-5`): coordinator and integrator; a worker subagent captures and
  implements, a fresh subagent reviews. Run under
  [D-0013](../docs/decisions/D-0013-task-time-and-disk-budget.md): claim to merge against the
  6-hour target, new evidence under 1 GB. Usage at the claim: 5-hour 0%, weekly 35%.
- Branch `task/cpu-cycle-timing` in `.worktrees/cpu-cycle-timing`; evidence in main
  `local/evidence/cpu-cycle-timing/`.
- Task provider: Anthropic.
- Milestone: M4. Review tier: **1** (race arithmetic and frame timing).

Two classes of difference need the CPU's cycle count, not just its instructions: the multiply helper
`$81:B67C` hit mid-way by an HDMA write sharing the mode-7 latch (DOWNER in two views, R-0095; track
41's landing against ANTI-UNI, R-0098, R-0102), and prints that run past their frame or sit on a
knife edge (R-0095, R-0100, R-0102). Outcome: a cycle-cost model of the race update and of the
front end's prints good enough to place each listed case, checked on the original's captures, with
native's results equal on those routes.

## Results

Worker: Claude Opus 5.5 (`claude-opus-5-5`) subagent, 11 November 2026 07:22 to 08:25 UTC (clock).
[R-0115](../docs/research/R-0115-cpu-cycle-timing.md) has the measurements and the limits.

A print that runs past its frame leaves the text buffer as far as its last store before vblank, and
the printer's stores come at least about 1,000 clocks apart, so a cost model can place the cut when
its error is below the gap. A menu frame's work starts with a spread of about 500 clocks (where
vblank finds the frame wait's sound poll), which no model of native's state can know. The multiply
needs the race update's clock to about 15 clocks, and the race's frame start depends on the previous
frame's exact clock: out of reach of any cost model.

| Case | Verdict |
| --- | --- |
| DOWNER in two views (R-0095) | cannot: the M7A stores straddle the split's HDMA at line 112, 272,522 clocks into the frame; needs the clock within about 15 clocks; no change |
| Track 41's landing against ANTI-UNI (R-0098, R-0102) | cannot, as above; the whole race (`odd-tours`) has 76 products of which 1 is hit |
| TRACK RECORDS' grid halves past their frames (R-0100) | **native changed**: 15 of 16 traced prints equal at the frame's end (`$00CE` and text); `rs-high` 1463 a knife edge (124 clocks) |
| HIGH SCORES' leaders past their frame (R-0100) | measured (six prints, margins 472-2,750 / 1,406-3,910 clocks), not modelled; queued |
| VS CHAMPIONS' header frame (R-0095, R-0100) | measured (5343 margins 956 / 1,138; 9302 a knife edge), not modelled; queued |
| The lap results' first frame (R-0095) | not a cycle case: the original clears the text buffer in result frame 1; frame 0 holds race RAM |
| W2b 15672, the rename keyboard (R-0095) | not judged: the capture's menus differ from 13,024 on (after its second race), on main too; queued |
| The three-member knife edge (R-0102) | unchanged; the start's spread explains part of R-0102's error |

## Decisions (worker)

- The multiply gets no native change. A hit needs the first M7A store in a window of about 30 clocks
  of a line, after 150,000-270,000 clocks of race work and a frame start that depends on the previous
  frame's exact clock (the race's two-instruction wait loop, `$82:D4A0-D4A2`): a per-instruction cycle
  mirror of the race, which the project rejects. The clean product is the original's in 75 of 76
  products of the scanned race.
- The grid model counts writes, not words: a step is a picture row, `$00CE`, a letter or a dot
  column, in the original's order, so `track_records.cpp` prints the same sequence and stops after the
  modelled count; frames 64 and 87 print the whole half again, which equals printing the rest.
- The model's start is the mean of the traced starts, not a value tuned to the knife edge: a start
  12 clocks earlier (27,198 for 27,210) would also place `rs-high` 1463, but that print's outcome in
  the original depends on the sound poll's phase, which the model cannot know (R-0115 finding 2). It
  is recorded as a residual and kept in the checks' expectations.
- HDMA is costed per visible line (24 clocks, fitted) because one rate for all lines left the model's
  error swinging by 1,400 clocks between the display and vblank.
- HIGH SCORES and VS CHAMPIONS were measured but not modelled within the budget: their margins
  (about 500 clocks at the tightest) are near the start's spread, and each needs the generic
  printer's per-code costs; queued with the tools as tier 2 (no picture differs).

## Evidence

Main `local/evidence/cpu-cycle-timing/` (1.1 MB): `trace_frames.py`, `print_cut.py` (any frame's
instructions with clocks, the printer's stores around a frame's end), `grid_prints.py`,
`grid-prints.json` (the 16 grid prints traced), `grid_model.py` (the native model in Python, `--fit`
the step costs), `multiply_hits.py`, `multiply-odd-tours.json` (every `$81:B67C` call of the track-41
race), `check.sh`, `checks.list`, `run_checks.sh`, `expect-grid.json`, `checks.out`, `reverted.out`,
`main-runner/` (main's `front_end_runner`), `gcc.sh`, `gcc.out`, `regen.sh`.
- `checks.list`, five lines, all PASS (`checks.out`, run with an output path containing a space):
  `grid-rs-track` 54 s, `grid-rs-high` 55 s, `grid-wr-track` 28 s, `grid-rv-records` 51 s (each:
  frames compared and pictures exactly as recorded, all pictures equal, the differing frames by field
  exactly `expect-grid.json`), `grid-model` 19 s (the 16 prints traced afresh equal `grid-prints.json`,
  the model places 15 of 16). A missing or short result fails.
- `reverted.out`: with main's runner the four `grid-*` checks fail (`$00CE` and text at every grid
  frame: 16 text frames and 8 `$00CE` frames more); `grid-model` passes by construction.
- Builds `lab-release`, `lab-debug`, `app-debug`; `ctest` 41/41 on each. `front_end_tests` pins the
  model's counts (171, 170 with a palette step, 166 with every icon, 170 for `rs-track` 1163's icons,
  176 for a second half; since review round 1 also 170, 175 and 174 for second halves with icons). Linux GCC 13.3 (`ubuntu:24.04`, CI's warnings as errors):
  `menu_overruns.cpp`, `track_records.cpp`, `tour_menu.cpp` (and since review round 1 `league_menu.cpp`) compile plain and with
  `-fsanitize=address,undefined`. clang-format clean on the changed `src/core` files. ASan not run
  locally (macOS 27).
- `regen.sh` (eight inputs, unchanged) and `coverage native-symbols --check`: passed; the tooling tests
  pass. No new `front_end_runner` option, so no gate corpus change.
- Races: the race runners (`lab_runner`, `movement_runner`, `split_demo_runner`, `zoom_zoo_runner`,
  `classic_race_presentation_runner`, `presentation_runner`) are byte-identical to the build before the
  change (SHA-256 compared); `front_end_runner` changes only TRACK RECORDS' entry frames 63, 64, 86
  and 87, which no race passes through, and its state after them is as before.

## Review round 1 fixes (worker)

Worker: Claude Opus 5.5 subagent, 11 November 2026, on the review of PR #103 (approve with
should-fixes; main `local/evidence/cpu-cycle-timing/review-1/`).
1. should-fix, done. Tour 8's extra 2,140 clocks are the second level check at `$80:ED7E JSL $83:9F14`
   (under `$80:ED74-ED7C`); `$83:8DA4`'s call runs for every tour (`$83:8DA0`'s `CMP #$08; BEQ` to the
   next instruction) and is part of the tour head. Fixed in `menu_overruns.cpp` and R-0115 finding 3;
   the value is unchanged.
2. should-fix, done. The start that would place `rs-high` 1463 is 12 clocks earlier (27,198 for
   27,210), checked with `grid_model.py`'s functions; Decisions corrected.
3. note, done. R-0115's Limits give the reviewer's three withheld layouts: 7 of 8 placed, the miss
   (`wx-dense` 1163: 165 writes, model 166; 1663 with the same inputs 166) the start's spread.
4. note, done. R-0115's Limits say the validation set is narrow (no traced second half had icons; the
   one palette-step print is the miss), so the 1,100-clock step is consistent, not confirmed.
5. note, done. Finding 4: the 29,597 product's M7B is 0, so it says nothing about the window; the
   three hits are the window's evidence.
6. note, done. `front_end_tests`: a second half with every icon (170), nine icons in tour 5 (175) and
   the same with a palette step (174), the Python model's counts.
7. note, done. `palette_delay(state)` is now one inline helper in `menu_overruns.hpp`, used by
   `league_menu.cpp` and `track_records.cpp`; in `grid_half_steps_in_frame` each `before` is set at the
   end of its loop body and the rank change is added only between ranks. No behaviour change: the unit
   counts and the checks are as before.
8. note, done. `checks.out` is rerun on the committed head (below).
Builds `lab-release`, `lab-debug`, `app-debug`, `ctest` 41/41 on each; GCC 13.3 plain and sanitized
on `menu_overruns.cpp`, `track_records.cpp`, `tour_menu.cpp`, `league_menu.cpp`; clang-format on the
changed `src/core` files; `regen.sh` and `native-symbols --check` passed.

## Handoff

- Commits on `task/cpu-cycle-timing`: `37ad1044`, the native change (`src/core/menu_overruns.*`,
  `track_records.cpp`, `tour_menu.cpp`, `front_end_screens.hpp`, `tests/native/front_end_tests.cpp`),
  the regenerated static map and symbols, R-0115, this record, the follow-up and its README row; then this handoff.
- Done: every listed case measured and given a verdict (Results); the grid prints changed; checks,
  builds, ctest, GCC and the static map as in Evidence. Not done: review, gates, closeout, STATE.md
  and NEXT_SESSION.md (the coordinator's).
- Open, queued as [CPU-CYCLE-TIMING-REMAINDER](CPU-CYCLE-TIMING-REMAINDER.md) (tier 2): HIGH SCORES'
  and VS CHAMPIONS' prints at their frame's end, W2b's lag after its second race.
- For the gates: checks that compare the text map or `$00CE` on TRACK RECORDS' entry frames 63 and 86
  will differ from main as expected (`reverted.out`); no picture and no later frame changes.
  SOUND-PROCESSOR-TIMING-REMAINDER's "waits after a race" item names this task's model as a route;
  R-0115 finding 4 says the race's CPU work cannot be modelled to the clock.
