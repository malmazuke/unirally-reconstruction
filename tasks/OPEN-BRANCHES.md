# OPEN-BRANCHES - the reachable branches no capture of the original has run

## Assignment

- Status: **claimed** 9 November 2026 23:41 UTC on main `19ecb59f`, on the user's instruction
  ("Start OPEN-BRANCHES"). Queued 8 November 2026 by [COVERAGE-CENSUS](COVERAGE-CENSUS.md)
  (R-0109, queue item 4). Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`):
  coordinator and integrator; a worker subagent captures and implements, a fresh subagent reviews.
  Run under [D-0013](../docs/decisions/D-0013-task-time-and-disk-budget.md): claim to merge against
  the 6-hour target, new evidence under 1 GB. Usage at the claim: 5-hour 1%, weekly 29%.
- Branch `task/open-branches` in `.worktrees/open-branches`; evidence in main
  `local/evidence/open-branches/`.
- Task provider: Anthropic.
- Milestone: M4. Review tier: **1** (race and front-end state).

R-0109's code whose reachability is open and the branches the records list as never captured: the
HUD's fields of 10 and more (R-0043), the `$77:0749 = $FF` path (R-0058), `$83:9B49`'s arm and
`$83:9C47`, the rider sprite-tile queue's last slots (`$82:BFC9-C53C`, `$82:CFF3-D194`), the loop
descriptor facing away (R-0108), a stunt score of 1000 (R-0107), track 13's cell past its flag table
(R-0107), 2P and VS menus on the locked tours' tracks, a race tie, NEXT TRACK's wrap after track 44
(R-0095), demo cycles past the twentieth title (R-0096), and the never-captured sounds (corkscrew
ejection, the 9:5x warning, the cheat page reveal, the slow checkpoint, BRONSEN's praise, R-0077).
Outcome: for each, a route of real inputs captured on the original and compared with native, or a
recorded argument that it is unreachable; native changed only where it differs.

Added by [CENSUS-REMAINDER](CENSUS-REMAINDER.md) ([R-0112](../docs/research/R-0112-census-remainder.md),
`local/evidence/census-remainder/arms.md`), the never-run arms whose feature is named: the other end
of the sprite-tile queue (`$82:B8E3-BF76` and `$82:C53E-CFC7` never stopped at their first 16 and
21 slots: a frame queuing fewer tiles than any capture); two views in HUNTER's effects (`$83:CEF3`,
`$83:D1D4`, `$83:D343`; 2P or VS on the locked tours' tracks, above); a rider pose of 4096 or more
(`$81:9E2B`, which would read bank `$A2`); the track menu's signed table offsets (`$80:BA7C`,
`$80:BB1E`, `$80:BB72`); and the HUD numbers of 10 and more at `$81:D218`, `$81:D692`, `$81:DE90`,
`$81:DFC1`, `$81:EC6F` (with R-0043's fields, above). The other 187 of the 278 small arms left R-0112 groups by
guard and leaves open.

## Results

Worker: Claude Opus 5.5 (`claude-opus-5-5`) subagent, 9 November 2026 23:42 to 10 November 2026 00:52
UTC (clock). [R-0113](../docs/research/R-0113-open-branches.md) has the table, the arguments and
the limits.

| Item | Verdict |
| --- | --- |
| HUD fields and numbers of 10 and more | unreachable: laps are at most 8 (`$83:A254`, `$83:8472`) |
| `$77:0749 = $FF` (`$80:9075`) | unreachable: NOW PLAYING replaces `$017F = $FF` before the race setup copies it |
| `$83:9B49`'s arm, `$83:9C47` | captured (`erase-colin`, `league-erase`, and review round 1's `multi-league` for the reaches past a slot); **native changed**: DEFINE PLAYER's erase takes the rider out of their leagues |
| 2P and VS on the locked tours' tracks | captured: 2P track 44 (`twop-hunter-next`), VS track 42 in two views (`vs-neon`) |
| HUNTER's two views `$83:CEF3`, `$83:D343` | captured (`twop-hunter-next`) |
| HUNTER's two views `$83:D1D4` | unreachable (`$12D1` only in one view, cleared between races: `neon-then-43`) |
| A race tie | captured (`vs-dragster-tie`, totals 3426/3426); the VS count's tie arm `$83:9296` unreachable |
| NEXT TRACK's wrap after 44 | captured (`twop-hunter-next`) |
| Demo cycles past the twentieth title | captured: the wrap to track 0 in the 32nd race (`demo-wrap`) |
| Corkscrew ejection sound, 9:5x warning | captured (`corkscrew-t14`, `clock-warning`): every sound command equal |
| Track menu's signed offsets | unreachable: the routine's only table `$80:EA35` has no byte with bit 7 |
| Sprite-tile queue ends, pose of 4096+, loop facing away, stunt score 1000, track 13's cell, cheat page reveal, slow-checkpoint clear, BRONSEN's praise | open, queued as [OPEN-BRANCHES-REMAINDER](OPEN-BRANCHES-REMAINDER.md) |

Not matched and recorded: `erase-colin`'s frame 6102 (the warning arrow one animation step late in the
original, 608 pixels; most likely the removal's CPU time); `clock-warning`'s result screen after the
clock limit, 2 frames later in the original; `league-erase`'s pictures from 15900 and `demo-wrap`'s way
back from 98340, not traced. All in the follow-up.

## Decisions (worker)

- Routes were checked on the original with a PC trace of the branch (`orig.py --watch`, the core's
  instruction ring), not with coverage captures; so no ninth static-map input, and the map regenerated
  with its eight (`regen.sh`) only for the `src/core` edit.
- `$83:9C47` is implemented on the cartridge RAM image (`LeagueTables`), not on the records' fields:
  the routine reaches past a slot in four places and the image keeps them exactly; changed bytes go back
  through `store_record_byte` (review round 1: `store_record_word` before), so `$10F5-10F8` are kept too.
- Images are compared outside R-0111's residual bytes; pictures a record already explains are counted
  apart (R-0088's sprite differences by pixel count, R-0103's two-view NEON race pictures left out with
  the race rows compared instead).
- Sound commands, not PCM: the original's `JSL $82:8000` calls against native's cue log, leaving out the
  session commands 1, 7, 8.
- The items that need a search or a data bound of their own were left open rather than half done.

## Evidence

Main `local/evidence/open-branches/` (6 MB): `manifests/` (the routes), `runs/` (each original run's
pads, PC-trace hits, digests and images), `rows/` (`race_compare2p.py` with two marked changes, probes
as `.zst`), `orig.py`, `compare.py`, `sounds.py`, `check.sh`, `checks.list`, `reverted.out` (the three
erase checks fail with main's runner), `missing.out` (a check with its images missing fails), `gcc.out`,
`regen.sh`, `make_unlocked.py`, `make_image.py` and `multi-league.bin` (review round 1's route),
`menu.py`, `imgdiff.py`, `rom.py`; `review-1/` (the reviewer's files).

- `checks.list`: nine lines, all pass on the candidate; the first eight took about 50 s together (10, 3,
  13, 8, 4, 1, 6, 4 s), `multi-league` about 3 s. Each names the pictures, images and sound commands it
  compares and fails on fewer.
- Builds: `lab-release`, `lab-debug`, `app-debug` passed; `ctest` 41/41 on `lab-debug` and `app-debug`.
- Linux GCC 13.3 (`ubuntu:24.04`, CI's flags): `options_menu.cpp` and `cartridge_ram.cpp` compile plain
  and with `-fsanitize=address,undefined`. clang-tidy and clang-format clean.
- `regen.sh` (eight inputs) and `coverage native-symbols --check`: passed; the tooling tests pass.

## Review round 1 fixes (worker)

Review: approve with should-fixes (PR #101, `review-1/review.md`). Fixed on 10 November 2026 (01:04 to
01:11 UTC):
1. Checks that compare nothing: `compare.py --pictures N --images F...` and `sounds.py --commands N`;
   `check.sh` gives every route its counts. `league-erase` with its four images missing now fails
   (`missing.out`).
2. The comment in `options_menu.cpp` and R-0113 now say where slot 5's reaches land (`$0618`,
   `$0678-0679`, `$067E` = `league_tracks[0]`, the borrow) and that a rider in no pairing place writes
   0x10 into the next slot's first pairing. Code unchanged in behaviour.
3. `multi-league` is the ninth check (moved from `review-1/`, the original rerun here with the same hit
   counts and images: `$83:9B70` 4, `$83:9C8B` 1, `$83:9D62` 2, `$83:9D77` 4); with main's runner it
   fails on 55 bytes (`reverted.out`). R-0113's Limits and OPEN-BRANCHES-REMAINDER narrowed.
Also: the write-back stores bytes (`store_record_byte`, now declared in `cartridge_ram.hpp`) and the
helper is named `WordImage` (note 7); R-0113 records the four differences as pre-existing, the phase
lead and that `multi-league` weakens the CPU-time guess (notes 4-5).

## Handoff

Commits on `task/open-branches`: `2d1b49c9` (the native change and the regenerated map), `908fbf3e` and
`3b180c5a` (the records). Not done: the items queued in OPEN-BRANCHES-REMAINDER; no independent review yet
(tier 1). The coordinator pushes and opens the pull request.
