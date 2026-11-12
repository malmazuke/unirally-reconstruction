# SAVE-IMAGE-REMAINDER - the saved image's differences R-0111 leaves without an argument

## Assignment

- Status: **claimed** 12 November 2026 06:24 UTC on main `5c7522ee`, under the user's instruction of
  12 November ("you can continue working on new tasks until you get to 90% weekly usage, or the reset
  happens (7PM tonight)"), after PICTURE-RESIDUALS #104. Queued 9 November 2026 by
  [SAVE-IMAGE-EXACT](SAVE-IMAGE-EXACT.md) (R-0111, review round 1). Claude Code desktop app, Claude
  Opus 5.5 (`claude-opus-5-5`): coordinator and integrator; a worker subagent captures and
  implements, a fresh subagent reviews. Run under
  [D-0013](../docs/decisions/D-0013-task-time-and-disk-budget.md): claim to merge against the
  6-hour target, new evidence under 1 GB. Usage at the claim: 5-hour 1%, weekly 41%.
- Branch `task/save-image-remainder` in `.worktrees/save-image-remainder`; evidence in main
  `local/evidence/save-image-remainder/`.
- Task provider: Anthropic.
- Milestone: M4. Review tier: **1** (race state that reaches a saved record, and cartridge RAM).

R-0111 leaves 14,380 of the original's write frames (from frame 406) differing outside the bytes no
later boot reads, on 207 of 916 routes; most are bounded timing shifts it argues, but some are values
a player sees or a later boot reads. First, the personal bests on four `hunter-effects` routes
(`c175`, `c177`, `c210`, `c220`): native's race adds to `$77:085F` (or `$085B`) at frames 1973 and
1991 through a computer opponent's voice class read past rider 1's tallies
(`race_update.cpp:852`, `increment_opponent_tally_byte`, R-0098), where the original writes nothing;
main does the same, so this is a race difference that lands in a saved record: find which class the
original takes there. Second, the endings' own `$77:10A7` stores (`$83:B649` on, 7 routes), which a
later boot reads as the markers' phase. Third, the rest of R-0111's Value list: the name bytes after
a terminator (RENAME's keyboard leftovers), `$0742` bit 10 in one fuzz run, `$1116` on three
track_reference runs, `$10B1` and `$074D` at the end of `league/organic-full-tour`, and the league
pair's swap (`$83:95C4`, `$83:95CA`, `$0748-0749` and the race words), each modelled or argued
unreachable or unread.

Outcome: each item native's image matches the original's at every write frame of the corpus
(`local/evidence/save-image-exact/check_image.sh`, its `expected.json` re-recorded), or is recorded
with why it cannot matter; the personal-best increments fixed in the race if the original takes
another class there.

## Results

Worker: Claude Opus 5.5 (`claude-opus-5-5`) subagent, 12 November 2026 06:24-07:52 UTC, 1 h 28 min (harness
clock), in `.worktrees/save-image-remainder`. [R-0117](../docs/research/R-0117-save-image-remainder.md)
holds the findings.

- **Item 1, the hunter-effects personal bests:** the original takes no class there because it queues
  no voice: its opponent lands that jump with a z flip only (`$0CEB` takes `0x12` at 1929, the
  consumer `$81:C219` finds the queue empty after). Native's race on these routes started a frame
  early (R-0077's loading table: tracks 41 and 43 with `$10B1` = 0, the race's counter 1, take 81 upload frames in
  the original, the table 80), so the player's held Right came an update late, the player counted
  a track marker the original had not, the opponent's catch-up cap rose at 1820 and its landing at
  1929 counted a roll and two twists. With the capture's race start (`--race-initialization`, as the
  laboratory's race checks do, D-0010), the four routes, and `$1116` on four more, are equal. No native
  change; the comparison's corpus gains the captures' race starts (527 track_reference routes).
  **In play the difference stays and reaches a saved record:** native still starts these races a
  frame early, so the personal bests (`c175`, `c177`, `c210`, `c220`) and `$1116` (`c209`, `c217`,
  `c218`, `c272`) can differ in a save a later boot reads. Queued as SAVE-IMAGE-REMAINDER-2 item 5;
  `check_race_starts.sh` pins native's own start on the 527 runs (review 1).
- **Item 2, the endings' `$10A7`:** native's ending counter (`TourEnding::drop`) is stored in the
  image at its nine store sites with the original's widths. `all-gold`, `locked-gold`, `walker-gold`
  equal. A saved step of 14 to 0x7FFF at a later boot stays unmodelled (queued).
- **Item 3:** `$016C`/`$022E` (no reachable reader, R-0112) and `$0744-074B` (R-0090) argued unread
  with perturbed boots and added to the residual; the rider names' tails modelled (the DEFINE PLAYER
  warning's name); `organic-full-tour`'s end found to be a different screen (the table leaves on any
  button, Left was held) and modelled; `$0742` bit 10 on `c385` is a fuzz run's menu divergence,
  queued; the league editor's tail on `c039` queued.
- **Overlap with OPPONENT-POSE-BOUNDS:** none (that is `presentation.cpp:243`'s colour maths; the
  routes are shared, the code is not).
- Write frames from 406 differing outside the residual bytes: main 13,986 (719 of 916 routes equal),
  candidate 11,879 (724); routes moved: `g038`, `g041`, `g045`, `g070`, `c042`, `c053`, `c054`, all
  toward the original. Queued: [SAVE-IMAGE-REMAINDER-2](SAVE-IMAGE-REMAINDER-2.md).

## Decisions (worker)

- The race start is taken from the capture in the comparison, not changed in native: the original's
  upload length varies with the sound processor's state (R-0077), which native does not run; moving
  the table's cells for tracks 41/43/8 at the race's counter 1 (`$10B1` = 0) would break the six-quits measurements the table
  came from.
- `$016C-016D`, `$022E-022F` and `$0744-074B` join this task's `residual.txt` (a copy of R-0111's);
  R-0111's own file is unchanged.
- R-0111's `expected.json` is not overwritten in the shared `local/` (main's gates read it while this
  branch is in review); its re-recorded result is `expected-save-image-exact.json` here, to replace it
  at merge.
- The warned name is kept as a copy taken when the warning prints and dropped when OPTIONS opens a
  picker, rather than a general model of the printer's scratch: no other path to the keyboard is in
  the corpus.
- `c385`, `c039`, the saved marker step and the markers' edges are queued rather than fixed in this
  task's budget.

## Evidence

All in main `local/evidence/save-image-remainder/` (13 MB).

- Item 1: `voice-trace-c175.txt` (`voice_trace.py c175 1960 1995`, the consumer `$81:C219-C2CA`),
  `trace-c175-1927-1929.txt.zst` (bank 82 at the brake `$82:A3FB`), `wram_watch.py` watches of
  `$0CEB-0D14`, `$0D25-0D30`, `$0FCD-0FD0`, `$0415-0418`, `$04BB-04BE`, `$1275-1288` (commands in
  R-0117); native instrumented locally (not committed). `race_starts.py` (`race-starts.json`): 484
  equal, 42 one frame early, 1 one frame late.
- Perturbed boots (`perturb.py`, R-0111 review 1's method, nine warm routes): `perturb-016C-016D`,
  `-022E-022F`, `-0744-074B`, `-riders-after-terminator` (pictures equal), `-leagues-after-terminator`
  (`c116` picture from 6,757 under XOR), `-names-after-terminator`.
- `g070`: the original's pictures (`orig_pic.py`) and native's (`--picture`) at 60,740-61,500: the
  podium at 61,000 in the original, the table natively before the change; after it native's podium
  equals the original's two frames later (`g070pics.py`).
- Behaviour: `stdout_same.py` main against candidate on all 919 routes (menu state every frame and
  sound cues): only `g070` differs, from 60,754 (`stdout-main-vs-candidate.json`).
- Checks (`checks.list`, `$G` with a space): `check_image.sh` PASS on the candidate
  (`check-candidate.log`), FAIL on main's runner `base/front_end_runner` (7 routes: `c042`, `c053`,
  `c054`, `g038`, `g041`, `g045`, `g070`; `check-main.log`).
- Builds: `project.py build` `status=passed` for lab-release, lab-debug, app-debug; `ctest` 41/41 on
  lab-debug and app-debug; `clang-format` applied; `clang-tidy -p build/lab-debug` clean on the five
  changed `.cpp`; GCC 13.3 (`ubuntu:24.04`, CI's warnings as errors) plain and sanitized OK on the
  five (`gcc.sh`, `gcc.out`); `coverage-census/regen.sh` (eight inputs) regenerated the static map
  and native symbols, tooling tests OK; `coverage native-symbols --check` `status=passed`. ASan
  presets not run (they hang on this host).

## Handoff

- Commits on `task/save-image-remainder`: `82545231` (native: the endings' `$10A7`, the warned name's
  tail, the table's any button; native symbols and static map), then the records commit (R-0117, this
  record, SAVE-IMAGE-REMAINDER-2 and its README row).
- Done: items 1 and 2; item 3 but `c385` and `c039`. Not done: SAVE-IMAGE-REMAINDER-2's five items (item 5 is item 1's race start in play).
- Not pushed, no PR, no review yet (tier 1).
- What the gates should expect (`--task-checks local/evidence/save-image-remainder/checks.list`):
  - this task's `check_image.sh`: PASS on the candidate, FAIL on main (7 routes, toward the original);
  - SAVE-IMAGE-EXACT's `check_image.sh`: FAIL on the candidate against its `expected.json` on the same
    7 routes (better); `expected-save-image-exact.json` here is the re-recorded result for it;
  - any step replaying `league/organic-full-tour` (`g070`): its menus, cues and pictures change from
    frame 60,754 (the podium 659 frames sooner, as the original); every other corpus route's menu
    state and cues are unchanged; endings and renames change only cartridge RAM (the `R` digests of
    `--records` on `g038`, `g041`, `g045`, `c042`, `c053`, `c054`).
  - SAVE-IMAGE-EXACT's `behaviour_check.sh` (no `g070`): run here, 15 of 15 routes equal on every
    output but the cartridge RAM (`behaviour-check.log`); expected unchanged.

## Review round 1 fixes (worker)

Review round 1 (PR #105, "approve with should-fixes"; main `local/evidence/save-image-remainder/review-1/`),
fixed 12 November 2026 08:12-08:40 UTC by the same worker (Claude Opus 5.5).

1. should-fix, item 1 closed in the comparison only: R-0117 now cites D-0010 (the laboratory takes a
   race's start from its capture), says the one-frame start on HUNTER's preloaded tours (two
   preloaded starts, 42 runs) still reaches a saved record in play (the personal bests on `c175`,
   `c177`, `c210`, `c220`; `$1116` on `c209`, `c217`, `c218`, `c272`), in its findings and Limits;
   queued as SAVE-IMAGE-REMAINDER-2 item 5 (README row updated). New check
   `check_race_starts.sh` in `checks.list`: native's own first race update on the 527
   track_reference runs must equal `race-starts.json` (484 equal, 42 early, 1 late), so a moved start
   fails although `check_image.sh` takes the captures'.
2. note, `$10A7` in two places: one shared `marker_step_word` (`front_end_screens.hpp`), used by
   `tour_ending.cpp`'s `drop_word`, with a comment on why the markers and the endings cannot conflict
   within a session (`$80:BC7B-BC7E` before any `$80:EB23` turn; review 1's `g045` run).
3. note: R-0117's "item 4 below" is now item 3.
4. note: the table cell is named consistently: the image's `$10B1` = 0, so the race's counter, which
   indexes R-0077's table, is 1 (R-0117 and this record).
5. note: `residual.txt` says `$0744-074B` are rewritten by every race setup before any read; R-0117
   lists the in-session reads and the setup stores that precede them.
6. note: the warned name is copied from a bounds-checked `subspan` of the rider's 16-byte record,
   after a range check that refuses a rider past 15 (`options_menu.cpp`), with the `$80:C603` /
   `$80:9B2F` citation.
7. note: `league_table_frame` comments why its Y/X test needs no pad gate (`$80:B74A` applies
   `$0742` bits 9 and 10, which nothing on the league's path sets; `$80:ABC8` opens both pads).

Rerun after the edits: `project.py build` passed on lab-release, lab-debug and app-debug; `ctest`
41/41 on lab-debug and app-debug; `clang-format` applied; `clang-tidy` clean on the changed `.cpp`;
GCC 13.3 plain and sanitized OK on the six files (`gcc.sh`, `gcc-r1.out`); `coverage-census/regen.sh`
regenerated the static map and native symbols (tooling tests OK); `native-symbols --check` passed
(1,797 addresses, 0 without a record). `checks.list` on the committed head with a `$G` containing a
space: see `checks-r1.log` in the evidence folder.


## Closeout (coordinator)

- Review: tier 1, one round on PR #105. Round 1 (`08590c91`): approve with should-fixes. The
  should-fix was that the personal bests are fixed only in the comparison, while native still starts
  HUNTER's preloaded races a frame early in play, so 8 routes' saved records can still differ; the
  record now cites D-0010, says so, queues it (SAVE-IMAGE-REMAINDER-2 item 5) and pins native's start
  (`check_race_starts.sh`). There were also 6 notes: one `$10A7` constant and why the markers and the
  endings cannot conflict, an item number, the song counter's naming, `residual.txt`'s wording, a
  bounds-checked name copy, and the league table's Y/X test without `$80:B74A`'s pad gate (kept with a
  comment). All were fixed in `3ac99c84`; the coordinator checked the bounds-checked copy, the new
  check and the clean-head checks rerun. Replies on the PR.
- Gates (D-0012) on `3ac99c84`, TMPDIR `/private/tmp/sirg`:
  - `gates-1` (the selection, 2 h 35 min): 101 steps same-as-main and 20 passed; both task checks
    pass. Changed: `fe-equivalence` with `fe-classify`, one of 179 front-end cases,
    `league/organic-full-tour`. The coordinator diffed its state rows between main's runner and the
    candidate's: every row is equal up to 14,766, where the candidate leaves the league table and
    reaches the podium earlier (16,635 rows, main 17,685), the change this task makes.
  - `gates-2` (`--only` the two, `--expect-changed` the two, 47 s): `status=passed`.
- CI on `3ac99c84`: macOS and Ubuntu lab jobs pass.
- After the merge, SAVE-IMAGE-EXACT's `local/evidence/save-image-exact/expected.json` is replaced
  by `expected-save-image-exact.json` from this task's evidence (the old one kept beside it as
  `expected-before-save-image-remainder.json`), so its `check_image.sh` passes on main again.
- Time: claim `cf15c6b4` 06:24 UTC on 12 November 2026 to merge about 11:15 UTC, about 4 h 50 min,
  inside D-0013's 6-hour target. Worker 1 h 28 min plus 6 min of review fixes; review 19 min; gates
  2 h 35 min + 1 min.
- Evidence: main `local/evidence/save-image-remainder/`, 15 MB. Worktrees `save-image-remainder` and
  `save-image-remainder-review` and the branch removed after the merge.
