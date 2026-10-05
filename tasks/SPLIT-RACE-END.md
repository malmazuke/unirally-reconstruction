# SPLIT-RACE-END - a two-human race runs until both have finished

## Assignment

- Status: **accepted** 5 October 2026 (tier 1, [PR #56](https://github.com/malmazuke/unirally-reconstruction/pull/56));
  claimed the same day by the session that closed SPLIT-HUD-GAPS, on main `321e3d9`
  after that task's merge (main equal to `origin/main`, closeout written). Prepared the same day
  from SPLIT-HUD-GAPS's review (R-0080).
- Milestone: M4 breadth
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider and model: Anthropic; Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`),
  one session as coordinator, primary and integrator
- Routing: **tier 1** (D-0008): when a race ends is race state and timing.
- Provider quota window (D-0004): at claim (5 October 2026 about 17:30 Sydney) weekly 42%, five-hour
  24%; standing rule: continue until weekly 80%.
- Dependencies: TWO-PLAYER-VS (R-0071), SPLIT-HUD-GAPS (R-0080).
- Branch and isolated worktree: `task/split-race-end` in `.worktrees/split-race-end`.
- Owned paths: the race's finish (`src/core/race_progress.cpp`), race state and its serializer
  (`src/core/zoom_zoo_movement.hpp`, `src/core/race_state_io.cpp`, `src/core/race_setup.cpp`), the
  forced finish's picture (`src/core/presentation.*`, `src/core/race_hud.cpp`,
  `src/core/race_windows.cpp`), the VS flag's setters (`src/core/front_end_runner.cpp`,
  `src/app/frontend.hpp`, `src/app/sdl_main.cpp`), native tests, R-0081, this record,
  `docs/STATE.md`, `tasks/README.md`, `tasks/NEXT_SESSION.md`.

## Outcome and boundaries

In the review's `zzap` (2P ZOOM ZOO, main `local/evidence/split-hud-gaps/review/`) native ends the
race 241 frames after rider 0 finishes (7856), as a one-player race's finish display does; the
original keeps both riders racing until rider 1 finishes (8067), and in VS forces the other rider
finished 360 updates after the first (`$83:EA5D-EA69`, `$83:EBCB-EBD7`, R-0080's listing report).
TWO-PLAYER-VS's accepted captures did not reach this case (not yet checked against their work
RAM). Recover the two-human finish and
result load for 2P, VS and league pairs, against captures where the riders finish far apart.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| State and timing | `split_compare.py` extended to the race's end and the result's frames | Equal on every frame through the result | JSON |
| Nothing else moves | Gates, sweeps | Unchanged in one-player play | gate logs |
| Review | Tier 1 | Approved | review on the pull request |

## Evidence and attempts

| Attempt | Hypothesis | Experiment | Observation | Next decision |
| --- | --- | --- | --- | --- |
| 1 (17:30-17:45) | A split race starts its finish display only when both riders have finished | `zzap`'s work RAM; `$83:E7C1-E803` | `$0F0F` counts from 8067, rider 1's finish, not rider 0's at 7615. `$83:E7C3`: with `$0DE1` (split) set the count needs `$0EFF` and `$0F01`, else only `$0EFF` | Apply to every split race |
| 2 (17:45-17:50) | Native's league-only rule generalizes | `update_finish`: `!split_screen \|\| riders[1].finished`; `split_compare.py` gains `$0F0F` | `zzap` 6,073 race frames, 0 differences (was ending at 7856); `twop`, `twop-plain`, `league` unchanged | VS first (below) |
| 3 (17:55-18:10) | VS forces the second rider finished 360 updates after the first | Captures `vs-idle` (VS, pad 2 idle, 5,200 frames, images 3900-5199) and `zzap-long` (the review's 2P run to 8,800 frames) in main `local/evidence/split-race-end/` | `vs-idle`: pad 1 finishes on 3985; `$0F07` = 359 on 3987 and falls by one every update; at 0 on 4347 `$0F01` = 0xFFFF (rider 1 forced finished), `$0F09` starts at 359 the same way, and `$0F0F` counts 1 on 4348 to 240 on 4587 (result load); `$0F03`/`$0F05` cycle 7-24 every other update while a rider is finished (finish animation?) | Implement after the listing reader's report (`$0F03-$0F0B` meanings, which native fields exist) |
| 4 (18:10-18:50) | The listing's drivers in race state, run in VS only, end a VS race | `drive_banner` after each `finish_rider`; `versus` set by the front end; VS split states append the drivers | `vs-idle`: 0 word differences, the race returns at 4588 as the original's; `zzap-long` 0, returns at 8308; R-0079's and the reviews' thirteen captures still 0 | Pictures |
| 5 (18:50-19:05) | The forced finish's picture follows | `boxes.py` on both captures | Two gaps on `vs-idle`: the lower view undimmed on 4347 (the forced rider's block runs that update) and the lower clock running on (0:44:6 against 0:43:2 at 4420; `$81:C6F0` skips its rewrite). Fixed in `dim_finished_views` and `ClassicHudPublished::lower_clock`; the rest is SPLIT-CAPTIONS' classes and the results' icons | League bit 2 |
| 6 (19:05-19:20) | A league pair is not VS | `league-idle`: a league pair, pad 2 idle, to 9,500 frames | Rider 0's driver spent on 9320 with no force through 9499; native equal (0 word differences, restores at 9000 and 9400) | Gates, PR #56, tier-1 review |
| 7 (19:35-19:50) | Review round 1 (early findings): the player's clock gate | The reviewer's withheld `vs-opp` (rider 1 first): 238 pictures with the player's clock running on; `$81:C6F0-C6F8` is the player's gate, `$81:C6D1-C6DC` rider 1's. `ClassicRaceHudClock` stops the player's rewrites once forced; citations corrected; the static map and symbol index regenerated (CI's two `test_native_symbols` failures) | `vs-opp`: 0 word differences, no clock difference left, restores at 4300, 4349 and 4400 equal; `vs-idle` unchanged | Re-gate, re-review |

## Plan as first written (from the listing report, main `local/evidence/split-race-end/listing-report.md`)

1. Race state: a `versus` flag (`$77:0750` bit 2, set at race setup from the front end's VS mode;
   measure a league race's bit 2 before assuming it clear) and each rider's banner driver
   (`$0F03`/`$0F05` index, `$0F07`/`$0F09` life) in `ZoomZooState`. Serialize: VS split states take
   their own layout letters (2P and league states keep their bytes); the drivers' words only while
   live, size-detected like `pause.lower_view`.
2. `update_finish`: per finished rider in order 0 then 1, the brake/pose block, then the driver
   (outside `finish_rider`'s phase-1 return): reset the life to 360 while the index is 0, decrement
   every update, step the index on `$0300` = 1 updates (7..24), cross-gated (a live driver of the
   other rider waits); at life 0, in VS, force the other rider finished (0xFFFF, no time: laps,
   totals and digits untouched); rider 0 forcing rider 1 lets rider 1's block run that update.
3. Serializer guards: `finished` may be 0xFFFF and "finished with laps left" in a VS state.
4. `ClassicWindowPointer` reads the state's drivers instead of its own copy (the forced rider's
   driver starts the same update).
5. Evidence: `vs-idle` and `zzap-long` with `split_compare.py` past the race's end (both finish
   flags, `$0F0F`, `$0F03-$0F09`) and through the result; TWO-PLAYER-VS's and LEAGUE's manifests
   in the front-end sweep; R-0081; gates; tier-1 review.

Departures from the plan: the drivers run in race state only in VS and only VS states carry them
(8 bytes after the split trailer, detected by size), so every other layout keeps its bytes; the
window pointer keeps its own copy and only starts a forced rider 1's driver in the same update
(R-0081, "Not covered").

## Handoff (checkpoint, 5 October 2026 about 17:55 Sydney; superseded by the attempts above)

- Worktree `.worktrees/split-race-end`, branch `task/split-race-end`, base `321e3d9`. Uncommitted at
  this record: the `update_finish` change (rule above), committed with this record as WIP; not
  pushed.
- **Blocking before a PR: VS's forced finish.** `$83:EA11-EA6F` (rider 0 finished, path on updates
  with `$0304` bit 0 set): `$0F09` set skips; `$0F03` = 0 starts `$0F07` = 0x168 (360); each pass
  decrements `$0F07`; at 0, if `$77:0750` bit 2 (VS), `$0F01` = 0xFFFF (rider 1 forced finished) and
  `$11FD` = 0xDB4E. `$83:EBCB-EBD7` mirrors it for rider 0. Native has no counter and no VS flag in
  the race state, so with the new rule a VS race whose second rider never finishes would never end.
  Read `$83:E8E0-EA72` and `$83:EA72-EC13` fully (what `$0F03`, `$0F05`, `$0F09`, `$0F0B` are; which
  updates decrement), add the counter (race state: likely serialized only while live, as
  `pause.lower_view`) and the VS flag to the race, then capture VS with pad 2 idle (TWO-PLAYER-VS's
  `mode-2-both-right` with port 2's Right removed; `local/evidence/split-pause-menu/make_split.py`
  can drop events by editing) through the result.
- Then: a 2P capture through the result with the finishes far apart (extend the review's `zzap`
  manifest, main `local/evidence/split-hud-gaps/review/zzap.json`, to about 8700 frames); the front-
  end sweep will show TWO-PLAYER-VS's and LEAGUE's manifests moving only where the riders finish
  apart; R-0081; gates; tier-1 review; PR.

## Gates

`local/evidence/split-race-end/gates.sh` against main `321e3d9`'s binaries
(`local/evidence/split-race-end/base-321e3d9/`); the final head's run is
`gates-cf5ed6a.out`, with `gates-32d051e.out` before it.
- Presets and ctest: 41 of 41 on each of the four presets.
- Synthetic suite, v1 contracts and eight hidden runs: all pass.
- The eleven differential gates: all pass.
- Race sweep: 432 runs, 0 differences.
- Cues: all seven identical to main.
- Tooling: 544 OK.
- Rules: no function over 80 lines; native-symbols passes.
- Fuzz: 40 aborts, as on main.
- Front-end sweep: 169 of 179 equal. The other ten are all TWO-PLAYER-VS's
  (`summary-32d051e.txt`):
  - the nine VS manifests end and draw as on main, and their rows are main's plus the VS block;
  - `mode-1-first-right` (2P, pad 2 idle) races on past main's 4226, as the original's frame 5000
    shows; frame 5499 has 0 differing pixels, against main's 56,647.
- The thirteen earlier split captures and the four race-end captures: 0 word differences.
- Every restore checked equal.

## Review and integration

- Reviewer: a fresh Claude Opus 5.5 subagent in its own clone, tier 1, with preregistered cases.
- Round 1 on `ba07f88`: early findings.
  - Its withheld `vs-opp` (rider 1 first, `local/evidence/split-race-end/review/`) had equal race
    words, but 238 pictures had the player's clock running on. `$81:C6F0` is the player's gate,
    `$81:C6D1` rider 1's.
  - The static map and native-symbol index were stale (CI red).
  - Both were fixed in `32d051e`.
- Review of `32d051e`: approved.
  - The withheld captures `vs-opp`, `vs-late` (the second finish within the driver's life) and
    `vs-later` (a forced rider with a lap left): 0 word and driver differences.
  - Twelve restores equal.
  - States other than VS match main byte for byte.
  - Ten malformed VS blocks refused.
  - Low findings: the reader's display-count rule, the league wrapper's VS state, the banner
    constants defined three times, the lower-clock step, and two record notes. All were fixed in
    `cf5ed6a` except the evidence-comment nits.
- Round 2 on `cf5ed6a`: approved, nothing new.
- Integration: merged by merge commit after the final head's checks; closeout in main
  `artifacts/split-race-end-integration/closeout.json`.
