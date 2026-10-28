# TWO-VIEW-POWER-BOUNCE - power bounce for both riders in a two-view race

## Assignment

- Status: **claimed** 28 October 2026 09:28 UTC on main `5f161c9c`, on the user's instruction
  (Claude Code desktop app, Claude Opus 5.5 `claude-opus-5-5`; coordinator and integrator; a worker
  subagent implements, a fresh subagent reviews). Queued 28 October 2026 by
  [SPLIT-HUNTER-PICTURES](SPLIT-HUNTER-PICTURES.md) (R-0103, review round 1). Run under
  [D-0013](../docs/decisions/D-0013-task-time-and-disk-budget.md): claim to merge against the
  6-hour target, new evidence under 1 GB.
- Branch `task/two-view-power-bounce` in `.worktrees/two-view-power-bounce`; evidence in main
  `local/evidence/two-view-power-bounce/`.
- Milestone: M4 (original game coverage).
- Review tier: **1** (race state).
- Dependencies: HUNTER-EFFECTS (R-0052), CREDITS-NAME (R-0094), SPLIT-HUNTER-PICTURES (R-0103).

## Why

In a two-view race (2P, VS, the split demo, a two-view league) under HUNTER's power bounce
(effect 2, `$132B`), native's race state diverges from the original's in rider 1's velocity:
SPLIT-HUNTER-PICTURES' `twop-104` from frame 5548 (update 3521) and `vs-124` from frame 4981
(update 2954). Native passes the landing matrix 0 to rider 0 only (`race_update.cpp:807`,
`index == 0`). The listing at `$81:94A8-94B2` skips the test that excludes rider 1 when the race
has two views (option bit 3), so both riders land with matrix `$0572`; native's own comment at
`vertical_contact.cpp:338` says the same. The original's RAM at both divergence frames shows a
two-view race with power bounce on.

## Outcome

Rider 1 lands with power bounce's matrix in a two-view race as the original does: race rows
equal past the divergence on `twop-104` and `vs-124` (captures and manifests in main
`local/evidence/split-hunter-pictures/`), with one-view races and the split demo unchanged.
Native's pictures of those races after the divergence then compare too.

## Checkpoint (worker, 28 October 2026)

The listing's whole condition ([R-0104](../docs/research/R-0104-two-view-power-bounce.md)):
`$81:94A8-94C3` gives a landing matrix 0 (`$0572`) when power bounce (`$132B`) is on and either
the race has two views (`$77:0750` bit 3: 2P, VS, the split demo, a two-view league race) or the
rider is the player (`$0FF9` zero; `$81:8DB0` stores 0 for rider 0, `$81:8F04` 2 for rider 1).
In one view rider 1 never takes it.

Native: `race_update.cpp` passes the effect for both riders; `lands_with_power_bounce`
(`vertical_contact.cpp`) decides from the rider and the options word, whose bit 3 native already
sets from `split_screen` (`0xC20A`). The context field `landing_matrix_zero` is now
`power_bounce`. Unit test in `zoom_zoo_trial_tests.cpp`: the condition's four cases, and a landing
through `resolve_vertical_contact` in each (it fails with the old player-only condition).

Race rows (`race_check.py` / `race_compare.py`, candidate against main `5f161c9c`, pack v41):

| Capture | Candidate | Main |
| --- | --- | --- |
| `twop-104` (2P, 11 rider-1 landings under power bounce) | 5,973 of 5,973 | 3,521, then unequal from 5548 |
| `vs-124` (VS, 8) | 5,973 of 5,973 | 2,954, then unequal from 4981 |
| `twop-67` (2P, new capture, 3 power bounces, 23) | 5,973 of 5,973 | 3,030, then unequal from 5057 |
| `one-88` (one view, new capture, 4 rider-1 landings under power bounce) | 4,023 of 4,023 | the same |
| `vs-4`, `demo-pic` (two views, no power bounce) | 5,973; 2,201 | the same |
| `one-67`, `faedine`, `faedine-zoom`, `faedine-count`, `warm-hunter-faedine` (one view) | 3,699; 1,671; 3,023; 1,274; 1,571 | the same |

A mutant without the `$0FF9` test diverges from the original's `one-88` at 5636, rider 1's first
landing under power bounce.

Pictures over the captures' whole length (`compare.py`, 6,100 each): `twop-104` 6,100 (main
3,650), `vs-124` 6,099 (main 3,083), `twop-67` 6,089 (main 3,148); the candidate's 12 unequal
pictures differ on line 113 only (R-0103's mosaic limit; `twop-67`'s 11 are before power bounce
and the same on main).

Native searches (`landings.py`): two-view schedules 1-200 (VS replays the 2P races): power
bounce in 70 races, rider 1 lands under it in 67 (631 landings), the candidate differs from main
in 64, each first at such a landing. One-view schedules 1-150 against the mutant: 26 races with
power bounce, rider 1 lands under it in all 26, the mutant differs in all 26 and main does not.

## Handoff (worker)

- Branch `task/two-view-power-bounce` on claim `19763cc1` (main `5f161c9c`). Commits: `6684eb10`
  (native: `vertical_contact.cpp`/`.hpp`, `race_update.cpp`, `flat_contact.hpp`,
  `zoom_zoo_movement.hpp`, the test in `zoom_zoo_trial_tests.cpp`), `f0311639` (R-0104, R-0052's
  pointer, this record's checkpoint, `docs/map/static/` regenerated), then this handoff.
- Checks run: `ctest` lab-debug 41/41 and app-debug 41/41 (`project.py build` `status=passed`
  for app-debug, lab-debug and lab-release); the new test fails with the old player-only
  condition (negative control, restored); `clang-tidy` 19 clean on `vertical_contact.cpp` and
  `race_update.cpp`; `clang-format` 19 applied to the changed `src/core` files (the test file
  keeps its own style); `regen.sh` (five raw coverage inputs) and `coverage native-symbols
  --check` passed, 0 cited addresses without a record; `checks.list` 14 of 14 lines rc 0 at
  `f0311639` (`checks-f0311639.out`, 16 min 41 s under the heavy-run lock; this handoff commit
  changes only this record). Synthetic suite and gates not run by the worker.
- Commands: `zsh local/evidence/two-view-power-bounce/captures.sh twop-67 one-88`;
  `python3 local/evidence/two-view-power-bounce/landings.py MODE SEED N [--frames F] --main BIN`;
  `local/locks/heavy-run.sh TWO-VIEW-POWER-BOUNCE zsh local/evidence/two-view-power-bounce/run_checks.sh OUT`.
- Evidence: main `local/evidence/two-view-power-bounce/`, 48 MB: `cap/` (manifests `twop-67`,
  `one-88`; probes with `series.bin.zst`, verified byte for byte, and `series.bin.sha256`;
  `twop-67`'s 6,100 pictures), `base-5f161c9c/` (main's runner), `mutant-either-rider/` (the
  runner without the `$0FF9` test), `search/` (the native searches), `race-*.json` (first runs),
  `pre/` (an early picture run at the same code), `checks-f0311639/`. Copies of
  SPLIT-HUNTER-PICTURES' `race_check.py` (`CAPTURE_DIR`) and CREDITS-NAME's `race_compare.py`
  (`--dir`; a probe dumped from inside a race compares from its first dumped frame). The private
  `TMPDIR` is cleared.
- For the gates: the task's checks are `--task-checks local/evidence/two-view-power-bounce/checks.list`.
  Expect race-state changes only where rider 1 lands under power bounce in a two-view race;
  the gate corpus's HUNTER two-view league windows (LEAGUE-TOURS `tours-a`) may change if one has
  power bounce.
- Not covered: a two-view league race and the split idle demo under power bounce are not
  captured (both take the same `split_screen` path); why rider 1's power-bounce landings leave the
  state unchanged in 3 of 67 searched two-view races is not traced; line 113 (R-0103) unchanged.
- Time (claim 09:28 UTC to handoff 10:20 UTC, 52 min, `date -u`): reading and baseline 10 min;
  native change and test 10 min; checks on the existing captures, searches and the two captures
  15 min; the full `checks.list` run 17 min (records and maps written meanwhile).
- Next: the tier-1 independent review, then the gates.
