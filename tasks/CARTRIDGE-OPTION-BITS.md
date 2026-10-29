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
(`option_bits_image_tests`, and bit 8 in `lap_result_tests`). Review round 1 found five of the
image's bit sites untested; `6289e2a1` adds tests for them, and each of the 19 listed changes
removed in turn now fails one test (`mutants.txt`).

Warm boots against the original (`warm_pair.sh`; main's runner `base-7c19d681`):

| Check | Main | Candidate |
| --- | --- | --- |
| `cheat-vs` (title code, VS race; the original clears `$10D0` at 1847) | 260/260 pictures; `$10D0`, `$0742-0743` differ | 260/260; equal |
| `cheat-league` (title code, LEAGUE, slot, table, pair; cleared at 1401) | 281/380: the podium from 1405; `$10D0` differs | 380/380; equal |
| `league-slot` (bit 13 owed; the slot's choice clears it at 830) | 281/380: the podium | 380/380 |

Image sweep (`bits_sweep.py`, 236 manifests, sampled every 500th frame, so a shorter difference
shows only when a sample falls in it): manifests with a `$0742-0743` bit differing fall from 212
(bit 1), 19 (2), 89 (3), 74 (8), 54 (9), 89 (10) and 1 (13) on main to 8, 0, 7, 0, 0, 0 and 1; no
other byte differs anywhere it did not on main. Sampled every 5th frame (review round 1,
`review-1/dense.json`, at `e2be787e`) the candidate's counts are 14 (bit 1), 0 (2), 7 (3), 0 (8),
19 (9), 2 (10) and 1 (13). Every difference is timing: bit 9 a few frames at VS's first pick, bit
10 35 frames at 1P's gate, bit 1 20-115 frames around the logo's moves and the demo's `0x0A`, bit
3 the demo's `0x0A`, bit 13 the league's table before the podium. Each is bounded and no later
boot reads the bit before it is rewritten (R-0105).

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
  (`mutants.txt`): five changes checked at the handoff (widened to 19 in review round 1);
  `checks.list` 4 of 4 lines rc 0 at
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
  (`league-podium-owed.bin`: the league image with bit 13 set), `checks-first-pairs-superseded/`
  (the first warm pairs, before the image bits; superseded by `checks-d491dcc6/`),
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
- Next: the tier-1 independent review, then the gates. (Done: review round 1 below; after its
  fixes, the gates.)

## Review round 1 (approved with three should-fix findings, `e2be787e`)

A fresh subagent in its own checkout ([PR 92](https://github.com/malmazuke/unirally-reconstruction/pull/92),
report also in `local/evidence/cartridge-option-bits/review-1/review.md`): approve. It recounted
the census independently (129 accesses, no indexed store, DMA or bank-`$77` window reaching the
words) and recaptured the three warm checks, which passed. A dense sweep, warm pairs over 8 images
and 7 scripts, and native against native found no new byte moved. Findings and replies
(fixes in `6289e2a1`, records in the commit after it):

1. **2P clears bit 2 at the race, not at NOW PLAYING's print.** Fixed: `now_playing.cpp` now clears
   it in NOW PLAYING's race choice for 2P (`$80:BD6B-BD74`; the continuation's race reaches the
   same site through `$80:BD9D`); 1P and the league keep the print-time clear. R-0105's bit 2 row
   corrected. The reviewer's case (`cold-429` with bit 2 set, `twop-restart`'s pads, every 5th
   frame; `review-1/warm_bits.py`): before, bit 2 differed 1705-1845 with 1680/1680 pictures;
   now 1680/1680 pictures and no `$0742-0743` bit differs (`review-1-fix/bit2-cand.out`). The
   control with the unchanged `cold-429` image is also clean (`review-1-fix/cold-429-cand.out`).
2. **The sweep is sampled.** The checkpoint and R-0105 now say so and give the dense counts
   and spans as timing differences that no later boot reads. The fix for finding 1 cannot change
   the dense counts: bit 2 was equal in every corpus manifest even with the print-time clear.
3. **The mutant claim was wider than the tests.** Tests added for every site named:
   `note_option_bits` (`now_playing_option_bit_tests`: 1P, 2P and VS, through NOW PLAYING and its
   race), the second picks' bits 2 and 3, 1P's pick clearing bit 3, the award's and the tour
   ending's bit 8 at their sound upload frame, and bit 1 from `logo.raised`. The lap graph's clear
   is now tested from a set bit. `mutants.txt` lists 19 changes; each one, removed, fails a test.

Notes: R-0105's census method now covers the `$00:0199` block moves (`$80:9E8A`, `$80:D499`
write names in bank `$77`; neither reaches `$0740-0751`). The three image fields in
`front_end.hpp` are on a line each. The first warm pairs moved to
`checks-first-pairs-superseded/`.

Checks after the fixes: `ctest` lab-debug and app-debug 41/41; `clang-tidy` 19 clean on the nine
`src/core` files; `clang-format` 19 clean on the changed lines (`git-clang-format`); `regen.sh`
and `coverage native-symbols --check` passed, 0 cited addresses without a record. `checks.list`
not rerun (the gates run it next).

Handoff after review round 1: commits `6289e2a1` (2P's bit 2 at its race; the tests) and the
records commit after it (R-0105, this record, `docs/map/static/` regenerated for `$80:BD6B`). Time:
00:01-00:12 UTC on 29 October 2026 (`date -u`), about 11 min. Next: the gates.

## Gates round 1 (`4358c01f`)

The gates (D-0012) failed one step, `app-podium-audio` (PODIUM-AUDIO's `app_run.sh`): the app's
cue log differed from `front_end_runner`'s from frame 3180 (`gates-1/`, rerun `gates-2/`).
`rs-records` changed for the better (`wr-1p-ca` 223 bytes differing, main 224).

- **Cause: the step's route needed the old native behaviour.** It warm-boots
  `podium-audio/app/podium-pending.bin` (bit 13 set), enters LEAGUE, chooses the slot (900), the
  table (1300) and continues (1700), expecting the podium at 1720. The original clears bit 13
  when the slots are left (`$80:9F18-9F21`; its cartridge RAM's `$0743` changes at frame 900),
  so it shows NOW PLAYING at 1720 and goes into a league race at 2700. Native now does the same.
  Warm boots on that route against the original (`gates-round-1/`, every 20th picture): the
  candidate 180/180, main 93/180 (main shows a podium from 1720 the original does not).
- **The app and the runner differ in that race, on main too.** Main's app (built at `a6a1b385`
  in a temporary worktree, since removed) against main's runner on the same route with bit 13
  cleared in the image: the same divergence at 3180 (`gates-round-1/main-no-podium`: the app logs
  pause cues). With the image unchanged main's two still agree, because main never reaches the
  race inside 3,600 frames (`gates-round-1/main-podium-pending`). Neither is this change's bug.
  The runner matches the original here, and the app's league-race difference is the one
  PODIUM-AUDIO already recorded for `organic-full-tour-turnaround`.
- **Fix:** `local/evidence/podium-audio/app_run.sh` now compares through frame 3179 (`N=3180`),
  with the reason in its header; the previous script is kept as
  `app_run.sh.before-cartridge-option-bits`. On main's tree the route still reaches the podium
  (1720-2704) inside those frames. After this task merges, the app check covers NOW PLAYING and the
  race's load, not a podium: no warm route reaches a podium in the original, since every way into
  the league passes the slots. No native code changed, and `league_tests`' `option_bits`
  already checks the slots' clear, so no new unit test.
- **Rerun:** `local/locks/heavy-run.sh cob-app python3 -m tools.unirally_lab.gates run
  --task-checks local/evidence/cartridge-option-bits/checks.list --only app-podium-audio --out
  local/evidence/cartridge-option-bits/gates-3`: `status=passed`, the step passed in 65 s ("the
  app's cue log equals front_end_runner's through frame 3179 (3558 lines, 0 podium lines)").
- Checks: `ctest` lab-debug and app-debug 41/41, `clang-tidy` 19 clean on the nine `src/core`
  files, `clang-format` clean on the changed `src` lines (`league_tests.cpp` keeps the file's own
  4-space style), `coverage native-symbols --check` passed.
- Follow-up proposed: an app check of the podium's audio needs an in-session route, a league's
  last event raced in the app, and so the app's league-race difference from the runner fixed first
  (PODIUM-AUDIO's note; the pause cues at 3180 here).
- Time: 02:49-03:00 UTC on 29 October 2026 (`date -u`).
