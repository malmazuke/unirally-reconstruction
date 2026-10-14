# MENU-INPUT - menu input the listing shows and native lacks

## Assignment

- Status: **accepted** 14 October 2026 (tier 1, [#79](https://github.com/malmazuke/unirally-reconstruction/pull/79),
  approved in review round 2 at `84036e7`), integrated by merge commit. Claimed 14 October 2026 on
  main `e304868` (Claude Code desktop app, Claude Opus 5.5 `claude-opus-5-5`; coordinator and
  integrator; a worker subagent implements, a fresh subagent reviews). Queued 10 October 2026 by
  [MODE-AUDIO](MODE-AUDIO.md) (R-0091). Weekly usage at claim 14%.
- Choice: the oldest queued row and native game behaviour still missing (the full-coverage goal);
  ahead of R-0089's LEAGUE-TOURS, CARTRIDGE-OPTION-BITS and RACE-LISTING-BRANCHES and of
  GATES-AGAINST-MAIN, which are larger or infrastructure. No other session holds a claim.
- Branch `task/menu-input` in `.worktrees/menu-input`; evidence in main `local/evidence/menu-input/`.
- Task provider: Anthropic.
- Milestone: M4 (original game coverage).
- Review tier: **1** (menu state in `src/core`: the front end's input handling and its state words).
- Dependencies: OPTIONS (R-0072), LEAGUE (R-0073), TWO-PLAYER-VS (R-0071).

## Why

Placing the menus' sounds (R-0091) read input handling native does not have:

- the keyboard: Start goes straight to OK (`$80:B916`), and held directions repeat;
- the league member picker takes A as well as Start (`$80:B8C4`) and marks with B, Y or X;
- VS refuses the champion as challenger (`$80:C0B1-C0BB`, with `$80:B0FA`'s sound);
- the league awards take a press only after `$80:B051`;
- the two-player continuation menu wraps at its ends (the review's `rv-twop-quit`: a fifth Down
  picks NEXT TRACK in the original, QUIT in native);
- the original's second-player menu does not slide (`$80:CBC3`);
- GROUP TABLES' Up and Down change the tour shown (`$80:DF37-DF77`), which native lacks;
- PLAYER SCORES' Down also takes Select and Up is tested first (`$80:B794`).

## Outcome

Each captured from the original and made native, with the menus' frozen comparisons unchanged
elsewhere.

## Added by TWO-HUMAN-STUNTS (review round 3, 11 October 2026)

- OPTIONS' hidden PICK TRACK markers still differ after returning from the rename keyboard (W5:
  939 frames against main's 1251); the generic list loop's marker turn (`$80:EB23`) is native only
  in OPTIONS and the continuation, not the league slots or RECORDS.
- DEFINE PLAYER's pick writes the rider word `$017D` natively (2) but not in the original (0), so
  the next 1P rider menu starts on another rider (W5 from frame 2943). Capture:
  `local/evidence/two-human-stunts-review/` W5.

## Checkpoint - 14 October 2026 (implementation worker, Claude Opus 5.5)

Record: [R-0097](../docs/research/R-0097-menu-input.md). Evidence: main `local/evidence/menu-input/`
(twelve probe captures `probe-*`, eleven sound schedules `*.cues.txt` with their `*-disp1/2`
captures, `checks.sh`, the sweep in `sweep/`, the baseline `base-e304868/`). Results below are
`checks.sh` at `adb6356` (`artifacts/menu-input/checks-adb6356.out` in the worktree): pictures
equal against the original, native and main.

| Item | Native before | Native after | Captures and result |
|---|---|---|---|
| Keyboard: Start to OK, held Left/Right repeat | Start ignored, no repeat, edge-detected presses | `$80:A25F-A297` order and `$008F` latches, repeat 19 then 7 frames, Start OK without latch; league names 17 characters | `kb-input` 2100/2100 (main 920); `league-name` 2143/2400 (long-name lag, Not covered); cues equal |
| League picker: A finishes, B/Y/X mark | Start only, Y/B marks, pad 1 | `$80:B8C4`, choice latch `$0010` bit 0, marks through `$80:9E0F`, shown next frame, logo and shadow timing | `league-pick` 2400/2400 (main 1280); cues 2247 equal |
| VS refuses the winner | native since TWO-HUMAN-STUNTS | unchanged | `vs-challenger` 700/700 from 6600; cues 9129 equal |
| Awards press after `$80:B051` | native since TWO-HUMAN-STUNTS | unchanged | `awards-press` presses at 10640 and 10720 ignored, 10760 leaves; 615/700 as main (awards slide residual) |
| Continuation wraps | native since TWO-HUMAN-STUNTS | unchanged | `twop-quit` 1000/1000; `rv-twop-quit` cues 9434 equal |
| Second pick does not slide | native since TWO-HUMAN-STUNTS | unchanged | both 2P/VS captures' rider menus equal on every frame |
| GROUP TABLES Up/Down | none; screen not frame-exact | `records_scores.cpp`: `$80:DDFC` frame by frame, Up/Down/Select change `$00CC` with the result's sound | `records-input` 1650/1650 (main 770), `scores-input` 1950/1950, `scores-carry` 3100/3100; cues equal |
| PLAYER SCORES Up first, Select as Down | Down first, latched, pad 1, exits Y/X/choice/Right | `$80:DACF` frame by frame: Up, Down/Select, `$80:C24C`, `$80:B6F8` exits incl. L/R, `$80:C206`, rider `$00CA` | same captures |
| OPTIONS hidden markers after the keyboard | 939 OAM frames on W5 | every return runs the loop from the slide's end; RECORDS and league slots turn markers | `w5` 3800/3800 (main 2592), `opt-backs` 2300/2300 |
| DEFINE PLAYER writes `$017D` | written (2 vs 0) | picks go to `$00CA`; league picker no longer writes 7 | `w5`, `kb-input`, `league-pick` rider word equal |
| Found on the way | warnings had no way out; entries copied OAM early; league slot columns all 2; `$0089` reset; picker colours restored | `$80:C17B`/`$80:9C07` answers; no early OAM copy; `$80:9F74`; delay kept; colours kept | `warnings` 3200/3200 (main 1345), `scores-carry` |

Remaining differences (R-0097 Not covered): the name check `$83:84D9` (no refusal of listed
words, and its run time: a long name's OK frame lags a frame in the original), one-frame text
buffer overruns (OK's OPTIONS print, GROUP TABLES' first print), the awards' slide and a frame
wait at the result's way out (as main), HIGH SCORES/TRACK RECORDS not recaptured, `$80:FBEE`'s
menu not captured.

## Handoff (worker)

- Branch `task/menu-input`, head after this record's commit (see `git log`); base claim `693f99d`
  on main `e304868`. Commits: `b5c8ff5` (keyboard, league picker, OPTIONS returns, `$017D`),
  `05e172a` (PLAYER SCORES and GROUP TABLES), `b8e3e8b` (league name length), `7ce9cfa` (the
  warnings), `adb6356` (OPTIONS' entries, slot columns, colours), then the record, R-0097 and the
  regenerated `docs/map/static/`.
- Checks run: `ctest` app-debug 41/41 and lab-debug 41/41; `clang-tidy` (LLVM 19) clean on the
  changed files; `clang-format` applied; `coverage native-symbols --check` 0 uncited, the
  tooling tests OK (`regen.sh` with the five raw coverage inputs). Synthetic suite and the
  coordinator's gates not run by the worker.
- Commands: `zsh local/evidence/menu-input/checks.sh` (under `heavy-run.sh`), the sweep
  `fe_equivalence.py` and `classify.py` (R-0097 Reproduction).
- Next: the coordinator's gates (`local/evidence/menu-input/gates.sh`, which calls `checks.sh`)
  and review. Follow-ups worth queueing: the name check's refusal and cycle timing; HIGH SCORES
  and TRACK RECORDS frame by frame; `$80:FBEE`'s menu.

## Gates - head `9cbff08` (main `local/evidence/menu-input/gates-9cbff08.out`)

Run by the coordinator under `local/locks/heavy-run.sh` with `gates_and_synthetic.sh` (DEMO-AUDIO's
gates on main `e304868`'s binaries `base-e304868/`, pack v39 on both sides, plus this task's
`checks.sh`), 00:48-03:34 UTC 14 October 2026. The worker's implementation ran 2 h 15 min
(harness-measured).
- Four presets build, ctest 41 of 41 each (ASan presets unavailable on this host; Linux CI covers
  them). v1 contracts, eight hidden app runs and the front end passed. Fuzz as on main (40 seeds,
  40 aborts: recorded non-pass).
- Synthetic suite: lab-debug passed; app-debug failed only on the timeout probe
  (`test_failure_and_timeout_are_distinguished`), with the reviewer's two unlocked builds
  overlapping it. Rerun on `85b3c18` under the lock: passed
  (`artifacts/menu-input/synthetic-85b3c18/`).
- Eleven differential gates passed with main's row digests. Race sweep 432 runs, 0 differences.
  Every pause, two-pad, race-end, look, restart and restore comparison as recorded; track sweeps
  equal main; idle demo lap 34 of 34, pictures 963 of 1,006.
- Earlier tasks' captures as recorded: TWO-HUMAN-STUNTS' league frozen pictures and review
  captures, CREDITS-NAME, WIPE-RAM (eight of eight, schedules equal), DEMO-AUDIO's schedules
  (6, 0, 2, 4; review 183, 2, 12; conditional check equal; the app's cue log equal),
  MODE-AUDIO's five schedules 0 frames, R-0077's twelve equal, R-0076's seven identical to main,
  the cold cartridge image equal.
- Front-end sweep: 108 of 179 equal to main, the 71 differing as the worker classified (this
  task's fields only).
- This task's `checks.sh`: as the checkpoint's table (W5 3800/3800, `kb-input` 2100/2100,
  `league-pick` 2400/2400, `league-name` 2143/2400, `warnings` 3200/3200, `records-input`
  1650/1650, `scores-input` 1950/1950, `scores-carry` 3100/3100); sound schedules 0 frames
  differing but `league-name` 5 (the long-name lag) and `awards-press` 1.
- Tooling tests 576 passed; functions over 80 lines 0; native symbols current.
- Hosted CI on `9cbff08`: Linux GCC failed, `std::invalid_argument` without `<stdexcept>` in
  `records_scores.cpp` (found by the reviewer). Fixed in `85b3c18`; CI green on all three checks.

## Review round 1 (returned on `9cbff08`) - the worker's fixes

Review: https://github.com/malmazuke/unirally-reconstruction/pull/79#pullrequestreview-5512919917
(main `local/evidence/menu-input-review/review-9cbff08.md`). Fixes in `36028c2` and `cc2e2bf`;
results from `checks.sh` at `cc2e2bf` (`artifacts/menu-input/checks-cc2e2bf.out` in the worktree),
which now also runs the review's seven captures and its two sound schedules.

1. Linux build (`<stdexcept>`): fixed by the coordinator in `85b3c18`.
2. PLAYER SCORES' first rider (`$00CA`): fixed in `36028c2`. Native now writes it where the
   original does: the 1P pick (`$80:BBBE-BBC1`), an OPTIONS picker's Y with bit 7 (`$80:C139-C13B`,
   `$80:D457-D459`), NOW PLAYING's name lines (`$80:B5C0`, traced in `twop-quit`, `vs-challenger`,
   `awards-press`), PICK TOUR's medals (`$80:9759-975C`) and the race's setup (`$80:99E8-99EB`).
   `rv-ca` 3400/3400 (was 2904). The result screens' writers (one-run rows traced: they leave the
   first rider; lap and stunt rows, the 1P scoring, NOW PLAYING's Race) and the VS challenger's
   NOW PLAYING print are recorded in R-0097's Not covered.
3. LOST percentage: fixed in `36028c2` (100 less WON% and FAILED%, `$83:8FEC-8FF5`; the
   percentages through `$83:91D0`/`$83:9D80`, which also leave `$77:10F9`/`$10FB`). `rv-scores`
   2300/2300 (was 1721), one text frame (GROUP TABLES' first print, as R-0097).
4. GROUP TABLES' tie order: fixed in `36028c2` (`$80:EFD4`'s selection sort). `rv-scores` as above.
5. Bar ends at 256 or more: fixed in `36028c2` (`$83:8F2F-8F5D` and `$80:9AE2-9B0B`'s
   normalisation, `$80:9719`'s division). `rv-bars` 1400/1400 (was 1064).
6. HIGH SCORES and TRACK RECORDS' pad 2: TRACK RECORDS reads either pad through the gate now
   (`36028c2`; its tests are the gated `$80:B7B9`-`$80:B830`, `$80:D70F-D714`): `rv-records`
   1141/2100 (was 1060, main 1060). HIGH SCORES waits with `$80:C206` (any button of either pad on
   two frames running), not the gated tests; its screen was not recaptured, so it stays in
   R-0097's Not covered.
7. Long-name lag at a full rider name: R-0097 says so (`rv-kb` from 1832).
8. Sweep numbers: rerun at `cc2e2bf`: 108 of 179 equal, 71 differ; 64 of 545 captured frames
   change, 63 now equal, 1 closer (`organic-slot-selector` 2000), none further. R-0097 corrected.
9. Printer codes: `text_printer.hpp` names them (`print_code`), used by `records_scores.cpp`'s
   streams and `text_printer.cpp`.

Also: `print_now_playing` stayed under 80 lines (`note_named_riders`). Checks at `cc2e2bf`: ctest
app-debug and lab-debug 41/41, clang-tidy clean (function size), clang-format clean,
`coverage native-symbols --check` 0 uncited (1553 cited), tooling tests OK; the task's captures
as before (W5 3800/3800, `kb-input` 2100/2100, `league-pick` 2400/2400, `league-name` 2143/2400,
`opt-backs` 2300/2300, `warnings` 3200/3200, `records-input` 1650/1650, `scores-input` 1950/1950,
`scores-carry` 3100/3100, `vs-challenger` 700/700, `twop-quit` 1000/1000, `awards-press` 615/700),
sound schedules 0 frames differing but `league-name` 5 and `awards-press` 1 (as recorded); the
review's `rv-pick` 2200/2200, `rv-warn` 6700/6700, `rv-kb` 1879/2300.

## Gates - head `84036e7` (main `local/evidence/menu-input/gates-84036e7.out`)

Same script, 05:00-08:01 UTC 14 October 2026, against main `e304868`'s binaries. The worker's
round-1 fixes ran 56 min (harness-measured).
- Four presets build, ctest 41 of 41 each; synthetic suite passed on app-debug and lab-debug; v1
  contracts, hidden app runs and the front end passed; fuzz as on main (40 aborts, recorded
  non-pass).
- Eleven differential gates passed with main's row digests; race sweep 432 runs, 0 differences;
  every pause, two-pad, race-end, look, restart, restore, track-sweep, idle-demo, CREDITS-NAME,
  WIPE-RAM, TWO-HUMAN-STUNTS, DEMO-AUDIO, MODE-AUDIO, R-0077 and R-0076 result as at `9cbff08`.
- `checks.sh`: the task's twelve captures as before; the review's `rv-ca` 3400/3400, `rv-scores`
  2300/2300, `rv-bars` 1400/1400, `rv-pick` 2200/2200, `rv-warn` 6700/6700, `rv-kb` 1879/2300
  (the long-name lag), `rv-records` 1141/2100 (HIGH SCORES, Not covered; main 1060). Sound
  schedules 0 frames differing but `league-name` 5 and `awards-press` 1, as recorded.
- Tooling tests 576 passed; functions over 80 lines 0; native symbols current. Hosted CI green.

## Review round 2 (approved)

- [Review](https://github.com/malmazuke/unirally-reconstruction/pull/79#pullrequestreview-5514984092)
  of `84036e7`: approved. Every round-1 fix checked against the ROM; three new captures equal on
  every picture: `rv-ca3` 2700/2700 (2P NOW PLAYING writes `$00CA` = 2, then PLAYER SCORES),
  `rv-figs` 2300/2300 (figures above 255, 700 of 1000, a three-way tie), `rv-ca2` 2600/2600.
  Evidence in main `local/evidence/menu-input-review/`.
- Advisories: HIGH SCORES' exit (`$80:C206`, recorded in Not covered and queued with
  [RECORD-SCREENS](RECORD-SCREENS.md)); a corrupt record's FAILED figure, added to R-0097's Not
  covered. The commit after `84036e7` changes records only.

## Handoff (closeout)

- Commits on `task/menu-input`: the claim `693f99d`; the worker's `b5c8ff5`, `05e172a`,
  `b8e3e8b`, `7ce9cfa`, `adb6356`, `9cbff08`; `85b3c18` (GCC include); `d4d9554` (gates);
  round 1 `36028c2`, `cc2e2bf`, `c197ef6`, `84036e7`; then this closing record.
- The closing session moves the worktree's `artifacts/menu-input/` (gate logs, checks, synthetic
  reruns) to main `artifacts/menu-input-integration/` with `closeout.json`, deletes the two
  worktrees' build output and removes `.worktrees/menu-input` and `.worktrees/menu-input-review`
  and the local branch. Captures stay in main `local/evidence/menu-input/` and
  `local/evidence/menu-input-review/` (a merged task's, kept under D-0011).
- Queued: [RECORD-SCREENS](RECORD-SCREENS.md) (HIGH SCORES and TRACK RECORDS frame by frame,
  `$80:FBEE`'s menu, the result screens' `$00CA` writers) and [NAME-CHECK](NAME-CHECK.md) (the
  keyboard's word check and the long-name lag).
