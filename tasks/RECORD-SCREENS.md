# RECORD-SCREENS - the records screens and the rider word after a result

## Assignment

- Status: **queued** 14 October 2026 by [MENU-INPUT](MENU-INPUT.md) (R-0097). Unclaimed.
- Milestone: M4 (original game coverage).
- Review tier: **1** (menu state in `src/core`).
- Dependencies: OPTIONS (R-0072), MENU-INPUT (R-0097), TWO-HUMAN-STUNTS (R-0095).

## Why

MENU-INPUT made PLAYER SCORES and GROUP TABLES frame-exact but left, in R-0097's Not covered:
- HIGH SCORES and TRACK RECORDS keep R-0072's implementation: their entry's logo, frame waits
  and way back differ (review `rv-records`: 1,141 of 2,100 pictures equal). HIGH SCORES leaves
  at once on pad 1, where the original waits with `$80:C206` (any button of either pad on two
  frames running, `$80:D9B2`).
- `$00CA`'s writers on the result screens (`$80:9041`, `$80:90D9`, `$80:F227`, `$80:F2AC`, the 1P
  scoring `$83:879E`) and the NOW PLAYING print after a VS challenger pick (`$80:B18D`), so a 2P
  or VS lap or stunt result may leave another rider for a later PLAYER SCORES.
- `$80:FBEE`'s menu after the league podium runs the generic list loop (hidden markers, the
  move's sound); native's does neither. Not captured.

## Outcome

Each captured from the original and made native, with the menus' frozen comparisons and
MENU-INPUT's `checks.sh` unchanged elsewhere.
