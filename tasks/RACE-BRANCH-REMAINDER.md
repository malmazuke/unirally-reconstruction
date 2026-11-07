# RACE-BRANCH-REMAINDER - the reachable race branches R-0107 could not capture

## Assignment

- Status: **queued** 7 November 2026 by [RACE-LISTING-BRANCHES](RACE-LISTING-BRANCHES.md)
  (R-0107, review round 1). Unclaimed.
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
