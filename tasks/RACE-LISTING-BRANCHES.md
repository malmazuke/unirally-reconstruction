# RACE-LISTING-BRANCHES - the race branches known only from the listing

## Assignment

- Status: **in progress**. Claimed 7 November 2026 03:39 UTC on main `2d516755`, on the user's
  instruction ("Yes, start it"; Claude Code desktop app, Claude Opus 5.5 `claude-opus-5-5`;
  coordinator and integrator; a worker subagent implements, a fresh subagent reviews). Queued
  9 October 2026 by [COVERAGE-GAPS](COVERAGE-GAPS.md) (R-0089, queue item 8); the record is written
  at the claim. Run under [D-0013](../docs/decisions/D-0013-task-time-and-disk-budget.md): claim to
  merge against the 6-hour target, new evidence under 1 GB. Usage at the claim: 5-hour 1% (the
  window reset at 03:20 UTC), weekly 3%.
- Branch `task/race-listing-branches` in `.worktrees/race-listing-branches`; evidence in main
  `local/evidence/race-listing-branches/`.
- Task provider: Anthropic.
- Milestone: M4 (original game coverage).
- Review tier: **1** (race state, the HUD's integer formatting, tile behaviours).
- Dependencies: R-0051 (loop and tile pairs), R-0068 (stunt HUD), R-0089 (the inventory),
  R-0093 (data coverage, the static map's corpus).

## Why

R-0089 lists race code that the static map knows only from the listing ([L]); no capture of the
original runs it:
- **HUD branches never shown:** lap and split fields drawn for `$FFFF` or `$1253 < 1`
  (`$81:D8D9`, `$81:E94F`, `$81:DB70`; blocks in `$81:D936`, `$81:DA4F`, `$81:DCE6`, `$81:E9AC`)
  and stunt scores of 100 and more (`$81:C3DC`).
- **Tile behaviours** in the table at `$81:82F0` that no capture reaches: `$81:875C` (pair 4,
  R-0051's open case; native throws on it), `$81:84B2`, `$81:831D`, `$81:83E7`, `$81:8565`.

## Outcome

- A census of each listed branch: what condition reaches it, whether the shipped game can reach
  it (which tracks or tile data hold the pair, which race states give the value), and whether
  native models it. Recorded in R-0107 with addresses.
- Each branch the shipped game can reach is captured from the original (preloaded cartridge RAM,
  a warm boot or a state origin where a route needs one), implemented natively where native
  differs or throws, and checked against that capture.
- Branches no shipped data or state reaches are recorded as unreachable, with the reason, and
  not implemented. Native keeps refusing them explicitly rather than inventing behaviour.
- Checks the gate runner can run (`checks.list`) and unit tests for each changed behaviour.
