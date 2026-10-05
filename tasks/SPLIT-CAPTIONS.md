# SPLIT-CAPTIONS - rider 1's tutorial hints and the split HUD's upload order

## Assignment

- Status: ready (prepared 5 October 2026 by the SPLIT-HUD-GAPS session, from R-0080's remaining
  caption differences).
- Milestone: M4 breadth
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider: to be recorded at claim
- Worker/session/runtime/model: to be recorded at claim
- Actual model/reasoning effort, routing rationale: **tier 1** (D-0008): rider 1's announcement
  queue (`movement.rewards`) is serialized race state.
- Provider quota window (D-0004): sample at claim.
- Dependencies: SPLIT-HUD-GAPS (R-0080), TWO-PLAYER-VS (R-0071), LEAGUE (R-0073), the split demo's
  opponent hints (R-0069).
- Base commit: the `main` tip at claim.
- Branch and isolated worktree: `task/split-captions` in `.worktrees/split-captions`.
- Owned paths: `src/core/race_update.cpp` and the announcement queue code (rider 1's hints),
  `ClassicRaceHudClock` and `ClassicRaceHistoryTracker` (the split upload chain), native tests, a
  research record, this record, `docs/STATE.md`, `tasks/README.md`, `tasks/NEXT_SESSION.md`.

## Outcome and boundaries

After SPLIT-HUD-GAPS a two-human split race's picture differs from the original's only in caption
cells (R-0080), for two reasons:

1. **Rider 1's tutorial hints.** A human rider 1 whose tutorial bit is set gets the tutorial
   sentences in its own queue (`$0CEB`, cursors `$0D11`/`$0D13`, hold `$0CA7`, `$11C3`, `$03EF`,
   `$12E5`: `$81:BEF1-BF31`, `$81:C05C-C0CD`), shown in the lower view ("give you", "bigger
   boosts" in `twop-plain`). Native queues hints for rider 1 only in the split demo.
2. **The split chain's upload order.** The split NMI (`$81:D853`) services one field a picture in
   the order left fields, top clock, bottom clock (requested an update after the top), top cells,
   bottom cells, stunt scores, top caption, bottom caption; native's `ClassicRaceHudClock` follows
   the one-player chain, so a caption change can land a picture off.

The same chain times the lower fields at a finish (`drfin` 3967-3968, `leaguefin` 8961-8963), and
the split CONTINUE clear (the pauser's rows from `$130F`, 129 words) takes the lower arrow's cells
on the closing pictures (`leaguefin` 9050-9053).

Recover rider 1's hint queue in two-human races, the split chain and the split clear, and make
every caption cell of R-0079's and R-0080's captures exact. One-player play does not move.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| State | Rider 1's queue words against the original's on R-0079's captures | Equal on every race frame | JSON |
| Pictures | `classify.py` over R-0079's captures | No caption class left | JSON |
| Nothing else moves | Gates, sweeps | Unchanged in one-player play | gate logs |
| Review | Tier 1 | Approved | review on the pull request |
