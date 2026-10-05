# SPLIT-HUD-GAPS - the split race's captions, lower arrow and lower colour math

## Assignment

- Status: **accepted** 5 October 2026 (tier 2, [PR #55](https://github.com/malmazuke/unirally-reconstruction/pull/55));
  claimed the same day by the session that closed SPLIT-PAUSE-MENU, on main
  `52f7a79` after that task's merge (main equal to `origin/main`, closeout written). Prepared the
  same day from R-0079's split picture classes and R-0071's residual.
- Milestone: M4 breadth
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`); one
  session is coordinator, primary and integrator; a read-only Claude Opus 5.5 subagent reads the
  split HUD listing
- Actual model/reasoning effort, routing rationale: **tier 2** (D-0008: presentation on recovered
  layers; the HUD history is presentation-only).
- Provider quota window (D-0004): at claim (5 October 2026 about 14:50 Sydney) weekly 40%,
  five-hour 7%; standing rule: continue until weekly 80%.
- Reviewer: one independent round with a withheld capture.
- Dependencies: SPLIT-PAUSE-MENU (R-0079), TWO-PLAYER-VS (R-0071), LEAGUE (R-0073).
- Base commit: `52f7a79` (main at claim).
- Branch and isolated worktree: `task/split-hud-gaps` in `.worktrees/split-hud-gaps`.
- Owned paths: `src/core/race_hud.*` (the split HUD), `src/core/presentation.*` (object colour
  math), native tests, a research record, this record, `docs/STATE.md`, `tasks/README.md`,
  `tasks/NEXT_SESSION.md`.

## Outcome and boundaries

A two-human split race's picture still differs from the original's in three places R-0079 measured
(a race with no pause shows all three; in `twop-plain` 353 of 530 pictures differ):

1. **Tutorial captions.** The split HUD (`$81:D853`, chosen by `$0DE1` at `$80:8803`) prints each
   rider's caption in its own view, rows 5-6 and 19-20 from column 8; native prints the player's in
   the one-player rows 10-11 (`$81:F311`) and the lower rider's not at all.
2. **The lower view's side arrow** in column 28 of rows 20-21 (and column 5 on ZOOM ZOO: the
   review's `zz2p` 2431, 2495, 2653-2689), which native does not draw.
3. **The lower view's colour math** where its rider covers its BG3 ink: the original darkens the
   rider; native adds the upper view's measured red (R-0042).

Also, found by SPLIT-PAUSE-MENU's review: in 2P the original darkens a finished rider's view while
the race goes on (`drfin` 3967-3989, about 27,000 pixels; native dims only a league pair's), and the
finished split HUD differs in the timer cell and shows TO ROLL where native shows WINNER (`drfin`
3967-4014). And the CONTINUE GAME clear of the pauser's rows in a split race (R-0079 "Not
covered"), which needs (1) first. Recover them from the split HUD routine and the colour math HDMA, and make the
pictures of R-0079's captures and `twop-plain` exact. Race state does not move.

## Inputs and prerequisites

Main `local/evidence/split-pause-menu/` (`twop-plain`, `twop`, `twop2`, `league`, the review's
`zz2p` and `drfin`, `classify.py`),
the static listing for `$81:D853-E8C8`.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Pictures | `classify.py` over R-0079's captures | No caption, arrow or colour-math class left | JSON |
| Nothing else moves | Gates, sweeps, R-0078/R-0079 captures | Unchanged elsewhere | gate logs |
| Review | Tier 2 | Approved with a withheld capture | review on the pull request |

## Evidence and attempts

| Attempt | Hypothesis | Experiment | Observation | Next decision |
| --- | --- | --- | --- | --- |
| 1 (14:50-15:05) | The five gaps trace to the split HUD NMI `$81:D853` and the split colour/brightness HDMA | A read-only subagent read `$81:D853-E8C8`, `$82:9822`, `$82:D57F-D64B`, `$83:E7C1-EBD7` (report: main `local/evidence/split-hud-gaps/listing-report.md`) | Captions: rider 0's `$0EA7` at rows 5-6 and rider 1's `$0EC7` at rows 19-20 from column 8, in a split chain that also serves the bottom clock and cells; arrows for whichever rider trails, in its own view, split rows for up/down; colour math per view from `$82:D4DC` by rider (TONY, rider 9, subtracts); a finished view dims in every split race (`$83:E8E0`, `$83:EA72`), not only a league's; the bottom clock blanks when rider 1 finishes; the bottom "finish" at column 1 | Implement in this order, each checked on `twop-plain`, `twop`, `league`, `zz2p`, `drfin` with `classify.py` |
| 2 (15:10-15:20) | The colour math, the finished views and the lower clock follow the listing | `RaceObjectMath` takes each view's CGADSUB (TONY subtracts); `dim_finished_views` in every split race; the lower clock blank after rider 1's crossing, the lower "finish" from column 1 | `league`'s 44 lower-view paused pictures equal; `drfin`'s finished lower view dims as the original's; its 25 paused pictures differ only in caption cells | Captions |
| 3 (15:20-15:30) | Each rider's caption sits in its own view | The player's caption at row 5 in a split race; rider 1's untrimmed from column 8 of row 19 | `twop-plain` 177 -> 235 equal; what remains in caption cells: the top caption changing a picture off (the split NMI chain's upload order) and rider 1's tutorial hints, which native's engine never queues for a human rider 1 (race state: a tier-1 follow-up) | Arrows |
| 4 (15:30-15:45) | The split arrows are the trailing rider's, in its own view | `classic_race_lower_arrow`, redrawn every update; `draw_split_arrow` with the split rows (side 6-7/20-21, up 2/15, down 11/25) and full length; tests | `twop-plain` and `league` have no class left but caption cells; `zz2p`, `twop` and `drfin` only their post-race screens and `drfin`'s two finish pictures (3967-3968, the lower fields' upload) | Records, gates, review |

## Plan (from attempt 1)

1. Colour math per view (`RaceObjectMath`/`rider_pixel`: CGRAM 27 plus or minus the object by the
   view's `$82:D4DC` entry; add mode unchanged).
2. Finished views dim in every split race (`dim_finished_league_views`).
3. Finished split HUD: bottom clock blank once rider 1 finishes; bottom "finish" at column 1.
4. Captions: the player's at row 5 in a split race; rider 1's queue followed in
   `ClassicRaceHudClock` and drawn at row 19 from column 8; the split chain's upload priority.
5. Arrows: the trailing rider's arrow in its own view, split rows, bottom redrawn every NMI.

## Handoff

- Source candidate `9e5df31` (gated); the head adds a test fix (`5879295`), named arrow rows
  (`aafa43a`, no picture moves: the re-review's counts equal) and records.
  [PR #55](https://github.com/malmazuke/unirally-reconstruction/pull/55).
- Gates (`local/evidence/split-hud-gaps/gates.sh` on a detached checkout of `9e5df31`, logs in main
  `artifacts/split-hud-gaps-integration/gates-9e5df31/`): four presets build, ctest 41/41 each;
  synthetic suite passed; v1 contracts pass; hidden app runs pass with no pose fallbacks; the eleven
  frozen race gates pass with 6,023 restores and the same row digests; race sweep against main
  `cbbd9ba`: 2,387,105 updates identical, its 400 differing pictures all with the menu open; the
  front-end sweep's 239 differing pictures (47 manifests) are all two-pad race frames, state equal
  but for SPLIT-PAUSE-MENU's lower-view byte; the split demo's 326 pictures equal; R-0078's and
  R-0079's captures as in R-0080; cues identical to main; tooling 544 OK; rules clean. Fuzz aborts
  40/40 as on main.
- Hosted CI: `9e5df31`'s Ubuntu job aborted in `presentation_tests` (the new test's default pairing
  read past the name table under libstdc++'s bounds checks; reproduced with libc++ hardening),
  fixed in `5879295`; CI passes on `5879295` and `aafa43a`.
- Queued: SPLIT-CAPTIONS (tier 1), SPLIT-RACE-END (tier 1).
- Usage: weekly 40% at claim and at the records.

## Review and integration

- Reviewer: a fresh Claude Opus 5.5 subagent, tier 2. Round 1 on `9e5df31`: reproduced R-0080's
  table and the demo check; its withheld `zzap` (2P ZOOM ZOO to both finishes; main
  `local/evidence/split-hud-gaps/review/`) has 0 state differences over 5,830 race frames and 4,940
  of 6,074 pictures equal (main's renderer 1,272), every side arrow equal. Findings: the split HUD
  still flat over the riders (a fix breaks 6 demo pictures; recorded, SPLIT-CAPTIONS); a two-human
  race ending 241 frames early (as on main; SPLIT-RACE-END); the lower cells' sign (as on main;
  SPLIT-CAPTIONS); record precision; the test's name-table read (fixed); unnamed rows (named).
- Round 2 on `aafa43a`: approved; two record nits fixed in the head.
- Integration: merged by merge commit after the final head's checks; closeout in main
  `artifacts/split-hud-gaps-integration/closeout.json`.

