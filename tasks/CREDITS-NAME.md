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
