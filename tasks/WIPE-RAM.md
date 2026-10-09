# WIPE-RAM - the main menu's hidden WIPE RAM menu and factory reset

## Assignment

- Status: **accepted** 10 October 2026 (tier 1, [#69](https://github.com/malmazuke/unirally-reconstruction/pull/69)),
  integrated by merge commit. Claimed 10 October 2026 on main `33f3645`, in the session the user asked
  to keep working until weekly usage reaches 50% (13% at claim). Queued by
  [COVERAGE-GAPS](COVERAGE-GAPS.md) (R-0089, queue item 3).
- Worker: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`); coordinator, primary and
  integrator. Branch `task/wipe-ram` in `.worktrees/wipe-ram`.
- Milestone: M4 (original game coverage).
- Review tier: **1** (it rewrites the cartridge RAM and changes front-end state).
- Task provider: Anthropic.
- Dependencies: FRONT-END-MAIN-MENU (R-0054), SAVE-FILES (R-0090), MODE-AUDIO (R-0091).

## Why

On the main menu, Left+A+L+R held on either pad (`$02B0`, `$80:ABEB-ABF9`) opens a two-entry
menu, WIPE RAM and MAIN MENU (`$80:A9B4`, text at `$80:A9FA`). Native recognises the code and
stops: the app shows a notice. The original's menu, its warning ("this option will reset your
game pak's memory to its factory default ... you will lose all your records") and the reset
itself are not native.

## Outcome

- The menu, the warning, both answers and the reset native and frame-exact against the original
  from a cold start and from saved images (SAVE-FILES' warm boots), with the menus' sounds.
- After the reset the cartridge image equals the original's, and the app's save file holds it.

## Checkpoint - 10 October 2026

- Captures: `confirm` and `cancel` by the coordinator, six more by the implementation subagent
  (R-0092's domain). The native screens (`src/core/wipe_ram.cpp`), the shared `wipe_cartridge()`,
  pack v36 and the app's switch from the notice were done by an implementation subagent and
  checked here (two captures' comparisons rerun: every frame equal).
- Results in R-0092: all eight captures equal on every compared frame and sound cue; the reset's
  image equals the original's from a cold start and three saved images.

## Review candidate

- Records: [R-0092](../docs/research/R-0092-wipe-ram.md).
- Gates: `local/evidence/wipe-ram/gates.sh` against main `33f3645`'s binaries (pack v35 for main,
  v36 for the candidate).

## Review - round 1 (approved)

- [Review](https://github.com/malmazuke/unirally-reconstruction/pull/69#pullrequestreview-5475468416)
  on `3ef1097`: approve, advisories only. Six of the eight comparisons rerun equal; seven withheld
  captures (the code on the title, held through slides, on pad 2 with pad 1 busy, Select+Y+A on the
  menu, long waits, every cancel button, choices before the wipe; and warm boots from the all-gold
  save with a wipe and a cancel into PICK TOUR) equal on every picture; the cold path, the pack's
  reproduction and the app checked. Advisories done: R-0092's addresses for the menu's tests and
  the DEC/INC, and `$CA` under Not covered.

## Gates - head `3ef1097` (`local/evidence/wipe-ram/gates-3ef1097.out`)

- Four presets build, ctest 41 of 41 each (ASan presets unavailable on this host; Linux CI covers
  them); synthetic suite, v1 contracts, hidden app runs and the front end passed. Fuzz as on main.
- Eleven differential gates with main's row digests; race sweep 432 runs, 0 differences; the
  front-end sweep 179 of 179 manifests equal main's runner (v35 for main, v36 for the candidate);
  every capture comparison as recorded; R-0076's cue schedules identical to main's; the idle demo
  lap 34 of 34 exact.
- WIPE-RAM: the eight captures equal on every frame and every sound cue; MODE-AUDIO's schedules
  and twelve R-0077 schedules as recorded; the cold cartridge image equal.
- Tooling tests 555 passed; functions over 80 lines 0; native symbols current.
- The original's track sweeps step first gave main's binary the v36 pack, which it refuses; rerun
  with v35 for main (`gates-3ef1097-sweeps.out`, under the shared heavy-run lock): track-breadth-2
  and the locked tours give main's rows.

## Handoff

- Next in this session's lane: CREDITS-NAME (R-0094; notes in main
  `local/evidence/credits-name/notes.md`: both name cheats, "credits" and "faedine").
