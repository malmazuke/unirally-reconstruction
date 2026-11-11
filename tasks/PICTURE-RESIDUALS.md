# PICTURE-RESIDUALS - the picture differences the records leave, by system

## Assignment

- Status: **claimed** 11 November 2026 22:51 UTC on main `ce558a3f`, on the user's instruction
  ("Next task", after CPU-CYCLE-TIMING #103). Queued 8 November 2026 by
  [COVERAGE-CENSUS](COVERAGE-CENSUS.md) (R-0109, queue item 7). Claude Code desktop app, Claude
  Opus 5.5 (`claude-opus-5-5`): coordinator and integrator; a worker subagent captures and
  implements, a fresh subagent reviews. Run under
  [D-0013](../docs/decisions/D-0013-task-time-and-disk-budget.md): claim to merge against the
  6-hour target, new evidence under 1 GB. Usage at the claim: 5-hour 1%, weekly 37%.
- Branch `task/picture-residuals` in `.worktrees/picture-residuals`; evidence in main
  `local/evidence/picture-residuals/`.
- Task provider: Anthropic.
- Milestone: M4. Review tier: **2** (presentation).

The records list pictures that still differ from the original after every task: split pictures where
a rider passes under the HUD, track 42's two-view NEON, line 113 under a mosaic block, 41 small sprite
pictures in demo races, 470 pixels in the GO letters' area, NOW PLAYING's arrow and icon animation,
the result icons and three league pictures (R-0046, R-0071, R-0073, R-0083, R-0084, R-0088, R-0098,
R-0103). Outcome: each traced to its cause in the original's PPU state and fixed in native, or
recorded with the reason it cannot be (a hardware effect native does not model), with every
compared picture of the routes involved equal.

## Handoff

Not started.
