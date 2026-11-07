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
