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

## Handoff

Not started.
