# RECORD-SCREENS - the records screens and the rider word after a result

## Assignment

- Status: **claimed** 23 October 2026 on main `ff1efb6` (Claude Code desktop app, Claude Opus 5.5
  `claude-opus-5-5`; coordinator and integrator; a worker subagent implements, a fresh subagent
  reviews). Queued 14 October 2026 by [MENU-INPUT](MENU-INPUT.md) (R-0097). Weekly usage at claim 31%.
- Choice: the user asked for this task by name. No other session holds a claim (the only remote
  task branch is the merged `task/podium-audio`).
- Branch `task/record-screens` in `.worktrees/record-screens`; evidence in main
  `local/evidence/record-screens/`.
- Task provider: Anthropic.
- Milestone: M4 (original game coverage).
- Review tier: **1** (menu state in `src/core`: the records screens' frame waits and exits, and the
  rider word `$00CA`).
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

## Checkpoint - 23 October 2026 (implementation worker, Claude Opus 5.5)

Record: [R-0100](../docs/research/R-0100-record-screens.md). Evidence: main
`local/evidence/record-screens/` (nine probe captures `probe-*` with their manifests, five sound
schedules `*.cues.txt` with their stripped `*-disp1/2` captures, the warm-boot images in `images/`,
`checks.sh`, `make_ca.py`, the baseline `base-ff1efb6/` the coordinator built). Results below are
`checks.sh` at `2a15a8c` (`artifacts/record-screens/checks-2a15a8c.out` in the worktree): pictures
equal against the original, candidate and main.

| Item | Native before | Native after | Captures and result |
|---|---|---|---|
| TRACK RECORDS' entry | logo raised at once, the grid printed at once, static HDMA, the loop 6 frames early | `$80:D5E6`/`$80:EB76` frame by frame: logo slide, `$0C70` tables and the 57-pass wipe, the two grid halves past their frames (`$00CE` 25, 45), 18 scroll passes, the records' objects | `rs-track` 2200/2200 (main 487), `rs-high` 2300/2300 (main 50), `rv-records` 2100/2100 (main 1141) |
| TRACK RECORDS' loop | 32 pixels a press, wrapping column, one latch, a time in every column | `$80:E3E9`'s easing scroll, unwrapped separately latched column with a sound per press, `$80:C269`'s scores, the colours and rows a frame after the print | same captures; Start held from before the loop leaves on its first poll (2138) |
| TRACK RECORDS' way out | one frame, RECORDS printed and slid again | `$80:9F7A`: 62 frames, the wipe out, HDMA off, RECORDS' loop without a print | same captures |
| HIGH SCORES | pad 1's B/Y/X/Start/A at once; palettes at once; leaders' ties and averages off | print past the choice's frame, slide, three colour frames, `$80:C206` (either pad, two frames running), back slide; leaders as `$80:D7D5-D8F8` | `rs-track` (one-frame B ignored 2450; pad 2 X + pad 1 A leaves 2501; L held leaves 2696), `rs-high`, `rv-records` |
| `$00CA` after results | the first rider (race setup) | `$80:CFA9`, `$80:D00F`, `$80:9041`, `$80:90D9`, `$80:F227`, `$80:F2AC`, `$83:879E`; NOW PLAYING's push and pull (`$80:B194`, `$80:B4C0`) | `rs-ca-2p-lap` 2153/2153 (main 1878: PLAYER SCORES' rider), `rs-ca-vs-lap` 2568/2568 (2293), `rs-ca-2p-stunt` 1694/1694 (1419), `rs-ca-vs-stunt` 2109/2109 (1834), `rs-ca-np` 1950/1950 (1675); `$00CA` equal on every menu frame of every capture |
| PLAYER SCORES' mask | a frame after RECORDS' choice | in the choice's frame (`$80:DACF-DAD6`) | `rv-ca` `$00CA` equal from 3450 (135 against 7 before) |
| VS challenger's NOW PLAYING print | native since MENU-INPUT round 1 | unchanged | `rs-ca-vs-lap`, `rs-ca-vs-stunt`, `vs-challenger` |
| `$80:FBEE`'s menu | native since LEAGUE-TOURS (R-0098 finding 5) | unchanged | `rs-podium-menu` 1000/1000 from 12300, candidate and main; the podium's own frames differ in the fields menu_compare.py skips, as main |
| Found on the way | warm boots started the markers' step at 0 | `$77:10A7` taken from the image | every warm OPTIONS/RECORDS frame's OAM (`rs-track` from 660) |

Sound schedules (frame waits included, from 620): `rs-track` 2630 lines, `rs-high` 2712,
`rs-ca-2p-lap` 16414, `rs-ca-vs-stunt` 9289: 0 frames differ; `rs-podium-menu` 14297: 1 frame
(10603, the awards' frame wait, as recorded in R-0097 and on main).

Remaining differences (R-0100 Not covered): the prints that run past their frame (text and `$00CE`
on TRACK RECORDS' entry frames 63 and 86, HIGH SCORES' choice frame), assets 29/30 not in pack v40
(no picture differs), the HDMA tables checked through the pictures only.

## Handoff (worker)

- Branch `task/record-screens`, base claim `ce33f1e` on main `ff1efb6`. Commits: `05b4a3e`
  (TRACK RECORDS and HIGH SCORES, the markers' step), `549da89` (`$00CA`'s writers, the runner's
  `--rider-word`), `2a15a8c` (PLAYER SCORES' mask), then R-0100, this record and the regenerated
  `docs/map/static/`.
- Pack: v40 unchanged (assets 29/30 would need v41; not needed for any picture).
- Checks run: `ctest` app-debug 41/41 and lab-debug 41/41; `clang-tidy` (LLVM 19) clean on the
  changed files (function size included); `clang-format` (19) clean; `coverage native-symbols
  --check` passed, 0 of 1,644 cited addresses without a record (`regen.sh`, the five raw coverage
  inputs; tooling tests `test_native_symbols`, `test_static_map`, `test_tracks` OK); `checks.sh` at
  `2a15a8c` as the table above (`artifacts/record-screens/checks-2a15a8c.out`). Synthetic suite and
  the coordinator's gates not run by the worker. The commit after `2a15a8c` changes a comment and
  the records only.
- Commands: `zsh local/evidence/record-screens/checks.sh OUTDIR` (under `heavy-run.sh`; zsh), R-0100
  Reproduction. The gates' `gates.sh` calls it.
- Next: the coordinator's gates and review. Follow-ups: NAME-CHECK (queued); a pack v41 with assets
  29/30 if a later screen shows colours 0x00-0x1F there.
