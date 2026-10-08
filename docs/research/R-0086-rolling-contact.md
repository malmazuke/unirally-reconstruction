# R-0086 - The slope after an inverted face stops the rider

Status: research result and native change
([ROLLING-CONTACT](../../tasks/ROLLING-CONTACT.md)), 9 October 2026, on main `5da988e`.
PAL ROM SHA-256 `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, audited bsnes
core `7d5aa1e656b9171524d01b1b22917197d8121cb4`. The captures are in main
`local/evidence/rolling-contact/` and `local/evidence/hunter-effects/review-withheld/`.

Tags: **[L]** listing only, **[C]** confirmed in a capture.

## The question

HUNTER-EFFECTS' review capture `w-rev-buttons` (TWO LOOPS, track 41; Right from 1,585 with Y, A,
L, R, B, X and Left under effect 7) matched native to update 1,737 (frame 3,128). There the
player's velocity y was 72 natively and 0 in the original (R-0052's known divergence). The rider
was in surface mode on its second contact after a 32-update flight, rising (velocity y -112) and
moving left (velocity x -36) into the underside of a mirrored high tile (descriptor `$C50E`, high
byte `$C5`, angle 24).

## Answer

The contact routine works on scratch copies of the velocities, x in `$0FA9` and y in `$0FAB`. Two
of its steps touch velocity x on this update, in this order:

1. **The inverted face.** `$81:924E-9275`: with velocity y negative and a high tile (bit 7 of the
   selected high byte `$02EC`), a mirrored face (bit 6) clears `$0FA9` when it is negative, and an
   unmirrored face clears it when it is not negative. [L]
2. **The slope.** Continued contact off the leading support with a surface angle below 26 sets
   velocity y from velocity x: `$81:96B6` loads `$0FA9`, shifts it by the angle's shift
   (`$81:96B9-96CB`), multiplies it by the angle's multiplier (`$81:96D0-96DC`), negates it for a
   negative angle and again for a high tile (`$81:96E1-96F9`), and stores it in `$0FAB`
   (`$81:970B`). [L]

So the slope reads velocity x as the inverted face left it. After a stop, velocity y follows from
zero. Native shifted the incoming velocity x (-36), before the stop, and got 72.

The velocity x half of the slope (`$81:97DB-97E1`, half the angle added to `$0FA9`) also reads
the scratch word. It runs only for a tile that is not high (`$81:97CC-97D4`), and the inverted
face changes velocity x only for a high tile, so the two never meet. Native now reads the same
word for both, to follow the listing.

## Capture [C]

- **`w-rev-buttons`** (HUNTER-EFFECTS' capture, 2,210 race updates from the boundary at 1,391).
  At frame 3,127 the player is at (2839, 1261) with velocity (-32, -131); at 3,128 the original's
  velocity x and y are both 0 and the surface angle is 24.
- **`access-w-rev-buttons`.** `access capture` of the same inputs to frame 3,135, with the tours
  unlocked by cartridge RAM writes after frame 610 (`w-rev-buttons.json`) instead of the
  reference's preloaded image. Its work RAM `$0200-$1FFF` equals the reference on every race frame
  (1,391-3,135); only the direct page and stack below `$0200` differ. Watched PCs, frames
  3,124-3,130:
  - `$81:9265` runs once, at 3,128, with A = `$FFDC` (velocity x -36): the mirrored face's test
    falls through to `LDA #0 / STA $0FA9`.
  - The player's slope on 3,128: `$81:96CD` holds A = 0 (the shifted `$0FA9`) with Y = 2 (the
    multiplier), and `$81:970B` stores 0 to `$0FAB`.
  - On every other frame `$81:9265` does not run.

- **A native search, then the original.** `search.py` ran main's and the fixed `zoom_zoo_runner`
  on random 4,000-update schedules (moves, jumps, tricks, both rotations; held 4-60 updates) on
  tracks 2-44: 8 seeds per track found one schedule where they differ, 48 seeds found ten, on
  tracks 12, 14, 21, 40, 41 and 42. Four were captured on the original (`track_reference
  capture`, the schedule held from the race boundary). In each, main diverges on exactly the
  update the search found, and the fix is exact to the end of the capture:

  | Capture | Track | Update (frame) | Tile | Main's velocity y | Original | Fix exact |
  | --- | --- | --- | --- | --- | --- | --- |
  | `w-rev-buttons` | 41 TWO LOOPS | 1,737 (3,128) | `$C50E`, angle 24 | 72 | 0 | 2,210 of 2,210 |
  | `seed7-a` (repeat `seed7-b` identical) | 41 | 576 (1,967) | `$C50E`, angle 24 | 64 | 0 | 910 of 910 |
  | `t14-seed23` | 14 | 606 (1,982) | `$C22C`, angle 24 | 56 | 0 | 925 of 925 |
  | `t21-seed20` | 21 | 2,129 (3,496) | `$C64C`, angle 8 | 135 | 0 | 2,334 of 2,334 |
  | `t14-seed28` | 14 | 1,356 (2,732) | `$822C`, angle -24 | 55 | -1 | 1,675 of 1,675 |

  The first four are mirrored faces met with velocity x negative. `t14-seed28` is an unmirrored
  face met with velocity x not negative: the stop leaves 0, the negative angle's correction
  (`$81:96E5-96EA`, `1 - product`) makes it 1, and the high tile negates it to -1, as the
  original shows.

## Native

`follow_slope` (`src/core/vertical_contact.cpp`) shifts `moved.velocity_x`, the velocity x after
`support()`'s inverted-face test, and adds the angle's half to it.

- `track_reference explore` on the five captures: exact to the end of each (above). Main's
  binary diverges on each at the update named.
- A unit test (`zoom_zoo_trial_tests`) rises into a mirrored and an unmirrored high face with
  velocity x negative: the mirrored one ends with velocity (0, 0), the unmirrored with velocity x
  kept and velocity y 18. It fails on main's code.
- The race equivalence sweep (432 runs, 2,387,105 updates, 1,290 restarts, 2,538 pictures)
  equals main: none of its schedules reaches the branch.

## Tested domain and limits

- The changed result needs a supported, non-leading continued contact with an angle magnitude
  below 26 (of 31) on a high tile whose inverted face stops a rising rider. The captures cover
  the player on tracks 14, 21 and 41, mirrored and unmirrored faces and positive and negative
  angles.
- The opponent runs the same contact code; no capture shows the opponent on this branch.
- Track 12's hit (a stunt event) and tracks 40 and 42 are native search results only.
