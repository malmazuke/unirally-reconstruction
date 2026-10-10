# CREDITS-NAME - the rider-name cheats: "credits" and "faedine"

## Assignment

- Status: **in progress**, claimed 10 October 2026 on main `6b6790a`, in the session the user asked
  to keep working until weekly usage reaches 50% (17% at claim). Queued by
  [COVERAGE-GAPS](COVERAGE-GAPS.md) (R-0089, queue item 4). DATA-COVERAGE runs in a parallel
  session; heavy runs share `local/locks/heavy-run.sh`.
- Worker: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`); coordinator, primary and
  integrator. Branch `task/credits-name` in `.worktrees/credits-name`.
- Milestone: M4 (original game coverage).
- Review tier: **1** (it changes the cartridge RAM's names, a race's end and HUNTER's races).
- Task provider: Anthropic.
- Dependencies: R-0060 (pause exits; `$0545`), R-0052 (HUNTER effects), R-0050 (locked tours),
  OPTIONS (R-0072: renaming a rider), SAVE-FILES (R-0090).

## Why

Each race's setup runs `$83:FB8A` (from `$83:C99E`). A first rider renamed "credits" shows a
500-frame picture and the race ends as a restart (R-0060); one renamed "faedine" gives the next
three races, on any track, HUNTER's tag effects and opponent tier. Both names then become "mike". Native has neither.
Notes from the ROM's bytes: main `local/evidence/credits-name/notes.md`.

## Outcome

- Both cheats native and frame-exact against captures of the original: the rename in OPTIONS,
  the race setups, the credits picture and its restart, HUNTER's races with and without the
  counter, the counter's countdown and the demo's clear of it.

## Checkpoint - 10 October 2026

- Implemented by an implementation subagent and checked here (the `credits` comparison and the
  `faedine-zoom` race rerun: equal as R-0094 says). Its reading corrected the queued note:
  "faedine" turns HUNTER mode on for every track for three races, not off.
- Records: [R-0094](../docs/research/R-0094-name-cheats.md). Pack profile v37.

## Review candidate

- Gates: `local/evidence/credits-name/gates.sh` against main `6b6790a`'s binaries (pack v36 for
  main, v37 for the candidate), under the shared heavy-run lock.

## Review - round 1 (returned)

- [Review](https://github.com/malmazuke/unirally-reconstruction/pull/70#pullrequestreview-5476961169)
  on `6d31dae`: return. Blocker: a split race in HUNTER mode diverged at its first tag (update 449
  of the review's `demo-faedine-0`): the original also pushes the announcement into the
  opponent's queue (`$81:C579-C594`). Should-fix: R-0094's call site, the setup routines it called
  skipped, and no captured two-pad tag. Advisories: restore checks of two-pad HUNTER races, the
  brightness constant, a pre-existing runner stop after a pause quit.
- Fixed: the opponent's queue; and the screen flip's two-view name `$24` ("invisible unis",
  `$83:CEE8-CF0F`), which the same case then showed. `demo-faedine-0` now agrees through update
  2001 (its last update, the demo's exit, differs in the camera and `$2054`). R-0094 corrected;
  the brightness expression named.
