# RACE-BRANCH-REMAINDER - the reachable race branches R-0107 could not capture

## Assignment

- Status: **in progress**. Claimed 7 November 2026 11:05 UTC on main `788efdaf`, on the user's
  instruction ("Start it"; Claude Code desktop app, Claude Opus 5.5 `claude-opus-5-5`;
  coordinator and integrator; a worker subagent implements, a fresh subagent reviews). Queued
  7 November 2026 by [RACE-LISTING-BRANCHES](RACE-LISTING-BRANCHES.md) (R-0107, review round 1).
  Run under [D-0013](../docs/decisions/D-0013-task-time-and-disk-budget.md): claim to merge
  against the 6-hour target, new evidence under 1 GB. Usage at the claim: 5-hour 1% (the window
  reset at 11:00 UTC), weekly 5%.
- Branch `task/race-branch-remainder` in `.worktrees/race-branch-remainder`; evidence in main
  `local/evidence/race-branch-remainder/`.
- Task provider: Anthropic.
- Milestone: M4 (original game coverage).
- Review tier: **1** (the HUD's arrow forms and tile behaviours in race state).
- Dependencies: R-0107 (the census, its searches and capture tools), R-0080 (the two-view HUD),
  R-0063 (the arrow), R-0051 and R-0047 (the tile pairs), R-0095 (two-human stunt events).

## Why

R-0107 found these branches reachable in the shipped game, or not shown unreachable, but no
capture of the original runs them. Native models each from the listing; nothing checks it:
- **The two-view arrow's other forms** (`$81:D872-DDA4`): the upper view's up arrow and its erase
  (`$81:D907-D933`, `$81:D9E8-DA4C`), the lower view's down arrow and its erase (`$81:DB75-DB9C`,
  `$81:DD4D-DDA4`), the up erase beside the cells (`$81:DBB5-DBCA`), three chevrons
  (`$81:DACE-DAE1`, `$81:DC92-DCCA`) and the blink's empty step (`$81:DAC4`, `$81:DC8D`). Up and down
  markers lie on tracks 4, 11, 19, 23, 24, 26, 28, 31, 34 and 40. Random two-pad schedules (58
  logged) and 64 idle demos gave only the upper view's down arrow and, in R-0089's corpus run 069,
  the lower view's one-chevron up arrow.
- **Rider 1's stunt score of 100 or more** (`$81:C3EA-C3F0`).
- **Tile arms not reached**: the lift on leading support (`$81:84B7`), a loop descriptor facing
  away from the rider (`$81:83C4`, `$81:83EC`) and a loop entry with a pose override inside
  `$0610-$061F` (`$81:83A6`). None appeared in 300 native schedules on tracks 15, 25 and 35.

## Outcome

- For each branch: a route of real inputs that runs it on the original (two pads that lead and
  trail on purpose, a league or VS race, or a demo set through `$77:10C8`/`$1115`), captured and
  compared with native frame for frame (pictures and race state where the state format allows),
  or a recorded reason it is unreachable.
- Native changed only where it differs, with unit tests; `checks.list` lines for the gate runner;
  the coverage of the new routes added to the static map's inputs.

## Checkpoint (worker)

Recorded in [R-0108](../docs/research/R-0108-race-branch-remainder.md). Every branch is captured but
the loop's two arms: one is unreachable by construction, the other was not reached. Native already
drew and ran every captured form, so no native code changed; the pack profile and the race state's
serialized layout are unchanged.

| Branch | Result | Route and evidence | Original against native |
| --- | --- | --- | --- |
| Upper view's up arrow and its erase (`$81:D907-D933`, `$81:D9E8-DA4C`) | Captured | t11-upper (2P, track 11, rider 0 trailing by about 23 transitions, steered into the up markers) | 1,699/1,699 pictures; up on 79 frames, 0-3 chevrons, with and without the cells |
| Lower view's up arrow without the cells, empty step, up erase beside the cells (`$81:DC8D-DCCA`, `$81:DBB5-DBCA`) | Captured | t11-lower (the pads swapped) | 1,699/1,699; up on 56 frames, 0-3 chevrons, 15 without the cells |
| Upper view's down arrow, three chevrons and the empty step (`$81:DACE-DAE1`, `$81:DAC4`) | Captured | t34-upper (2P, track 34, rider 0 on rider 1's route 651 frames later) | 2,299/2,299; down 0-3 chevrons |
| Lower view's down arrow and its erase (`$81:DD4D-DDA4`, `$81:DB75-DB9C`) | Captured | t34-lower (rider 1 514 frames later) | 2,169/2,169; down 0-3 chevrons |
| Rider 1's stunt score of 100 or more (`$81:C3EA-C3F0`) | Captured | vs-hill-rider1 (VS HILL CLIMB, pad 1 idle, pad 2 a route searched for rider 1's score) | 2,949/2,949 pictures; 2,792/2,792 race rows, trailer and tallies; `$77:0825` 100 at frame 3870, 152 at the end |
| Lift on leading support (`$81:84B7` taken) | Captured | t34-seed155 (1P, random schedule; updates 1915-1916) | 604/604 rows, 602/602 pictures |
| Loop entry with a pose override inside `$0610-$061F` (`$81:83A6`) | Unreachable by construction | every loop pose comes with the cooldown 3; the reset clears the pose at cooldown 1 before the step can be 0 at a strip [L]; 56,467 native entries [N] | - |
| Loop descriptor facing away (`$81:83C4`, `$81:83EC`) | Not reached; open | 16,187 native schedules (10,881 player and 45,586 opponent entries, 3 refusals) and 2,000 perturbations before track 25's shaft: none | - |

How: routes built on native by a receding-horizon search over a temporary trace build (never
committed): a leader rides a searched route, the trailer rides it hundreds of frames later or is
steered into a marker region; rider 1's stunt route searched for its score. Only the finished routes
ran on the original. Static map with the seven coverage inputs: observed bytes 87,882 to 88,343,
inferred 11,692 to 11,231, unknown 24.0%; every named range but the loop's three addresses is now
observed.

## Handoff (worker)

- Branch `task/race-branch-remainder` on claim `354064ae` (main `788efdaf`). Commits: `9ee2b11f`
  (R-0108; the coverage map `docs/map/race-branch-remainder.map.json`; `docs/map/static/`
  regenerated with seven coverage inputs; R-0089, R-0093 and R-0107 pointed at R-0108; the
  checkpoint), then this handoff (this record only). No `src`, test, pack or state-format change.
- Checks run: `checks.list` 7 of 7 lines rc 0 on the candidate's lab-release (no native change, so
  main's binaries), under the heavy-run lock (`checks-worker.out`, about 8 min); `mutants.txt`: 8
  mutants of `src/core` (the upper up arrow a row down, the lower down arrow a row up, no third
  chevron, no empty step, the lower up arrow's rows 16-17 drawn beside the cells, rider 1's hundreds
  cell dropped, rider 1's score counting one more from 100, the lift ignoring leading support), each
  failing every line that covers it, `src/core` restored and rebuilt after (diff 0); `regen.sh`
  (`coverage native-symbols` and `coverage static-map` with seven coverage inputs, the tooling tests
  `OK`) rerun after the last record edit, and `coverage native-symbols --check` passed, 0 cited
  addresses without a record; the five coverage captures `status=passed`. Not run: `ctest`,
  `clang-tidy`, `clang-format` (no C++ changed), the synthetic suite and the gates.
- Evidence: main `local/evidence/race-branch-remainder/`, 254 MB. `manifests/` and `routes/` (the
  five two-pad routes and their menu paths), `captures/t34-seed155` (work RAM raw, 119 MB:
  `compact_evidence.py` at closeout), `stunt/` (the vs-hill-rider1 probe, 72 MB, and the copied
  `race_compare2p.py`), `coverage/` (five captures, `merged.json`: the seventh static-map input),
  `listing-after/` (the listing with seven inputs), `search/` (the route search scripts and their
  logs, the tile hunts' 16,187 results), `trace_patch.py` (never committed; the trace builds are in
  the worktree's ignored `artifacts/rbr/`, 683 MB with the check outputs, deleted with the worktree).
- For the gates: `--task-checks local/evidence/race-branch-remainder/checks.list`. Expected changes
  against main: none in any binary; the tracked static map gains 461 observed bytes from the seventh
  input. Future static-map regenerations need the seventh `--coverage`
  (`local/evidence/race-branch-remainder/coverage/merged.json`), as R-0093 now says.
- Not covered (R-0108): a loop descriptor facing away from the rider (`$81:83C4`, `$81:83EC`), not
  reached in 16,187 schedules and 2,000 perturbations, reachability open (a shaft entered from below
  or its far side, or a 16-update cycle boundary on one of the shafts' opposite-facing columns);
  R-0107's stunt score of 1000, track 13's read past its flag table, whole-SRAM and audio. The two-pad
  arrow races compare pictures only (no serialized layout for their state).
- Time: worker start 11:04:46 UTC, handoff 12:01 UTC on 7 November 2026 (`date -u`), about
  57 min: reading 10 min; trace build, corpus scan and route search 20 min; captures, checks and the
  stunt route 10 min; coverage, mutants, static map and records 18 min (much of it waiting on runs).
- Next: the tier-1 independent review, then the gates.
