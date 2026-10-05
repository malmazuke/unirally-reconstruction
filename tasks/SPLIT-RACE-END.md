# SPLIT-RACE-END - a two-human race runs until both have finished

## Assignment

- Status: ready (prepared 5 October 2026 from SPLIT-HUD-GAPS's review, R-0080).
- Milestone: M4 breadth
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider and model: to be recorded at claim
- Routing: **tier 1** (D-0008): when a race ends is race state and timing.
- Provider quota window (D-0004): sample at claim.
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
TWO-PLAYER-VS's captures had both riders finish together. Recover the two-human finish and
result load for 2P, VS and league pairs, against captures where the riders finish far apart.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| State and timing | `split_compare.py` extended to the race's end and the result's frames | Equal on every frame through the result | JSON |
| Nothing else moves | Gates, sweeps | Unchanged in one-player play | gate logs |
| Review | Tier 1 | Approved | review on the pull request |
