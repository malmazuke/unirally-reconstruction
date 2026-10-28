# CARTRIDGE-OPTION-BITS - what sets the cartridge's option words, and what they do

## Assignment

- Status: **claimed** 28 October 2026 21:59 UTC on main `a6a1b385`, on the user's instruction
  (Claude Code desktop app, Claude Opus 5.5 `claude-opus-5-5`; coordinator and integrator; a worker
  subagent implements, a fresh subagent reviews). Queued 9 October 2026 by
  [COVERAGE-GAPS](COVERAGE-GAPS.md) (R-0089, queue item 7); the record is written at the claim.
  Run under [D-0013](../docs/decisions/D-0013-task-time-and-disk-budget.md): claim to merge against
  the 6-hour target, new evidence under 1 GB.
- Branch `task/cartridge-option-bits` in `.worktrees/cartridge-option-bits`; evidence in main
  `local/evidence/cartridge-option-bits/`.
- Task provider: Anthropic.
- Milestone: M4 (original game coverage).
- Review tier: **1** (cartridge RAM and the state the bits gate).
- Dependencies: R-0052 (HUNTER), R-0054 (boot and main menu), R-0089, R-0090 (save files),
  R-0093 (data coverage), R-0094 and R-0104 (option bit 3 is two views).

## Why

R-0089 listed the cartridge option words' unrecovered paths as a gap: what sets `$77:0742`'s
bits and their effects (bits 1, 4, 9 and 10 in R-0054; bit 13 in R-0090 and R-0099), HUNTER's
`$12D1` palette mode (R-0052), the save image's `$10D0` and the unmodelled bytes (R-0090), and
the `$82:DC2E-DC3D` path that picks the never-read `scenery.14` backdrop when `$77:0750` bit 3 is
clear (R-0093). Since then R-0094 and R-0104 showed that `$77:0750` bit 3 ("HUNTER's option bit
3") is the race's two-view flag, so that part is recovered.

## Outcome

- A census of `$77:0742` and `$77:0750`: every bit, every writer and reader in the static map's
  listing, whether the shipped game can set it, and whether native models it. Recorded in a
  research record with addresses.
- Each bit the shipped game can reach is modelled natively and checked against a capture of the
  original that exercises it (preloaded cartridge RAM or a warm boot where a path needs one).
- `$12D1` and the `scenery.14` path: reached and implemented, or shown unreachable from the
  listing with the reason.
- Bits no path sets are recorded as such, not implemented.

## Checkpoint (worker, 28 October 2026)

The census ([R-0105](../docs/research/R-0105-cartridge-option-words.md)) reads 129 accesses that
name `$77:0742-0743` and `$77:0750-0751` in the regenerated listing, and every recorded access in
the evidence tree's 171 access captures (no other writer or reader; 0 unresolved stores).

- **`$77:0742`:** bits 1, 2, 3, 5, 8, 9, 10, 12 and 13 are reachable; native modelled each one's
  effect. New: `$83:958C` and `$80:9F18-9F21` (below), and the saved image's bits 1, 2, 3, 8, 9
  and 10. Bit 4 is read at the main menu's entry (`$80:8889`) but nothing sets it. Bit 7 is only
  cleared. Bits 0, 6, 11, 14 and 15 are never set or tested.
- **`$77:0750`:** `$80:9A05` rewrites it before every race from `$0742` bits 2 and 3, and bit 1
  for the idle demo; all three are modelled. The menus' copies and the league pair's bit are
  dead stores. `$0751` holds the defaults' `0xC2`.
- **`$12D1` and `scenery.14`:** reachable (NEON, track 42 with one view) and implemented since
  R-0068/R-0098; inferred in the listing only because no raw coverage input holds NEON.
- **`$77:10D0`:** `$83:958C` clears it at a league pair's choice and a VS race's. Native kept it.
- **Unreachable code:** `$80:F648-F668` (a second logo hook), the pad tests `$80:B855`,
  `$80:B87A` and `$80:B89F`. Nothing in banks `$80-$83` points at them.

Native (`c8b43f41`): the flag clear (`league_menu.cpp`, `now_playing.cpp`); the slots' bit 13
(`options_menu.cpp`); the image's bits (`front_end.hpp`'s `versus_chosen`, `second_human`,
`result_objects_parked`; `cartridge_ram.cpp`; the sites in `rider_menu.cpp`, `now_playing.cpp`,
`race_result.cpp`, `award.cpp`, `tour_ending.cpp`; the pad gate kept over a soft reset and
cleared by the wipe in `front_end.cpp`). The save format and the pack profile are unchanged; a
saved file's `$0742-0743` bytes change. Tests: `league_tests` (`option_bits`), `front_end_tests`
(`option_bits_image_tests`, and bit 8 in `lap_result_tests`); each change removed in turn fails
one of them (`mutants.txt`).

Warm boots against the original (`warm_pair.sh`; main's runner `base-7c19d681`):

| Check | Main | Candidate |
| --- | --- | --- |
| `cheat-vs` (title code, VS race; the original clears `$10D0` at 1847) | 260/260 pictures; `$10D0`, `$0742-0743` differ | 260/260; equal |
| `cheat-league` (title code, LEAGUE, slot, table, pair; cleared at 1401) | 281/380: the podium from 1405; `$10D0` differs | 380/380; equal |
| `league-slot` (bit 13 owed; the slot's choice clears it at 830) | 281/380: the podium | 380/380 |

Image sweep (`bits_sweep.py`, 236 manifests, every 500th frame): manifests with a `$0742-0743`
bit differing fall from 212 (bit 1), 19 (2), 89 (3), 74 (8), 54 (9), 89 (10) and 1 (13) on main
to 8, 0, 7, 0, 0, 0 and 1; no other byte differs anywhere it did not on main. The residue (the
demo race's `0x0A`, the league's bit 13 between table and podium) is in R-0105.

## Handoff (worker)

- Branch `task/cartridge-option-bits` on claim `7c19d681` (main `a6a1b385`). Commits: `c8b43f41`
  (native and tests), `3d808861` (R-0105, `docs/map/static/` regenerated), `d23449fe` (the
  checkpoint, R-0105's sweep and residue), `d491dcc6` (static map for the new citations), then
  this handoff (this record only).
- Checks run: `ctest` lab-debug 41/41 and app-debug 41/41 (`project.py build` `status=passed` for
  app-debug, lab-debug and lab-release); `clang-tidy` 19 clean on the nine changed `src/core`
  files (two helpers keep `print_now_playing` and `rider_menu_frame` under 80 lines);
  `clang-format` 19 on the changed lines (`award.cpp` and `tour_ending.cpp` keep one warning each
  from main; the test files keep their own style); `regen.sh` (five raw coverage inputs) and
  `coverage native-symbols --check` passed, 0 cited addresses without a record; mutants
  (`mutants.txt`): each change removed fails a test; `checks.list` 4 of 4 lines rc 0 at
  `d491dcc6` (`checks-d491dcc6.out`, about 13 min under the heavy-run lock). Synthetic suite and
  gates not run by the worker.
- Commands: `local/locks/heavy-run.sh CARTRIDGE-OPTION-BITS zsh
  local/evidence/cartridge-option-bits/run_checks.sh OUT`; a single route:
  `zsh local/evidence/cartridge-option-bits/warm_pair.sh NAME IMAGE SCRIPT STRIDE OUT` (main and
  candidate); inputs from `make_inputs.py MANIFEST|- FRAMES OUT 'P:FROM-TO:buttons'...`; the
  listing from `coverage disassemble` with the five raw coverage inputs; the dynamic census
  `python3 local/evidence/cartridge-option-bits/census/dynamic_census.py local/evidence`.
- Evidence: main `local/evidence/cartridge-option-bits/`, 14 MB: `listing/` (bank listings, ROM
  bytes), `census/` (dynamic census and the `organic-full-tour` access capture at 60740-60770),
  `scripts/` (warm inputs and the original's per-frame cartridge writes), `images/`
  (`league-podium-owed.bin`: the league image with bit 13 set), `checks/` (first warm pairs),
  `sweep/` (main against `cand-1`), `checks-d491dcc6/`, `base-7c19d681/` (main's runner),
  `cand-1/` (the swept candidate, behaviour-equal to the final one), copies of SAVE-FILES'
  `warm_boot.py` (skips `#` lines) and `warm_compare.py` (every differing byte). The private
  `TMPDIR` is cleared.
- For the gates: `--task-checks local/evidence/cartridge-option-bits/checks.list`. Expected
  changes against main: a league or VS race after the title code (`$10D0`), a league re-entered
  with a podium owed, and the saved image's `$0742-0743`. Nothing else moves the front end's
  pictures or the race.
- Not covered (R-0105): bit 4's `$80:D503` branch and the dead hook and pad tests (unreachable);
  the demo race's `0x0A` and the league's bit 13 between the table and the podium in the image;
  the image bits' exact frames within a screen's entry; bit 8's later-boot branch not
  re-captured.
- Follow-up proposed: none needed for the option words. The `organic-full-tour` trace suggests the
  league's podium path starts its setup five frames into the table, before the press
  (`$80:BE78-BEB8`, `$80:99C2` at 60764); worth a look by whoever next touches the podium.
- Time (claim 21:59 UTC to handoff 23:20 UTC, about 1 h 20 min, `date -u`): reading and listing
  15 min; census and dynamic census 20 min; native, tests and warm checks 25 min; sweeps, lints,
  records and the `checks.list` run 20 min (about 20 min of it waiting on the final run).
- Next: the tier-1 independent review, then the gates.
