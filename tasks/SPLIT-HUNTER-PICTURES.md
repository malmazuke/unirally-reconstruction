# SPLIT-HUNTER-PICTURES - HUNTER's effects in a two-view race's picture

## Assignment

- Status: **claimed** 28 October 2026 01:07 UTC on main `9980a38a`, on the user's instruction
  (Claude Code desktop app, Claude Opus 5.5 `claude-opus-5-5`; coordinator and integrator; a worker
  subagent implements, a fresh subagent reviews). Queued 10 October 2026 by
  [CREDITS-NAME](CREDITS-NAME.md) (R-0094). Run under
  [D-0013](../docs/decisions/D-0013-task-time-and-disk-budget.md): claim to merge against the
  6-hour target, new evidence under 1 GB.
- Choice: the next queued player-visible gap after PODIUM-RETURN-TIMING.
- Branch `task/split-hunter-pictures` in `.worktrees/split-hunter-pictures`; evidence in main
  `local/evidence/split-hunter-pictures/`.
- Milestone: M4 (original game coverage).
- Review tier: **2** (presentation only: the race state already matches, R-0094; one independent
  round, escalated to tier 1 if the fix touches race state).
- Dependencies: CREDITS-NAME (R-0094), HUNTER-EFFECTS (R-0052), SPLIT-SCREEN-RACE (R-0069).

## Why

"faedine" puts any race into HUNTER mode, two-view races included. Their state matches the
original (R-0094's `demo-faedine-0`: 2,201 of 2,201 rows), but the split renderer does not draw
HUNTER's effects: pictures equal on 1,194 of 2,254 frames against 2,129 with the name "mike"
(wobble mode's mosaic for its 500 frames, parts of the invisible track and hedgehog speed, the
"invisible unis" span at frames 3448-3645, the two caption rows).

## Outcome

The split picture draws every HUNTER effect as the original does, over the review's
`demo-faedine-0` and further two-view captures (2P, VS, league races with tags).
