# SPLIT-RACE-END - a two-human race runs until both have finished

## Assignment

- Status: **claimed** 5 October 2026 by the session that closed SPLIT-HUD-GAPS, on main `321e3d9`
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
- Owned paths: the race's finish and result-load code (`src/core/race_update.cpp`,
  `src/core/race_result.cpp`), native tests, a research record, this record, `docs/STATE.md`,
  `tasks/README.md`, `tasks/NEXT_SESSION.md`.

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

## Handoff (checkpoint, 5 October 2026 about 17:55 Sydney)

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
