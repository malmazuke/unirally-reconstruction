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

## Handoff

- First experiment: the review's `zzap` (main `local/evidence/split-hud-gaps/review/zzap`, 2P ZOOM
  ZOO; rider 0 finishes about 7615, rider 1 at 8067) through `split_compare.py` extended past the
  race's end, to find the first differing word after rider 0's finish (`$0F0F`, the finish display,
  `$0EFF`/`$0F01`); read `$83:E7C1-EBD7` (R-0080's listing report, section 4) and the finish
  display/result load (`$83:F642`, `$0F0F` = 0xF0) for the two-human rule.

