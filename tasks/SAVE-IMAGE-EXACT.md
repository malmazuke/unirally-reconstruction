# SAVE-IMAGE-EXACT - the saved image equal to the original's at every save point

## Assignment

- Status: **accepted** 9 November 2026 (tier 1, #99), outcome met in part (R-0111). Claimed
  8 November 2026 10:03 UTC on main `901018a5`, on the user's instruction
  ("Start SAVE-IMAGE-EXACT"). Queued 8 November 2026 by [COVERAGE-CENSUS](COVERAGE-CENSUS.md)
  (R-0109, queue item 2). Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`):
  coordinator and integrator; a worker subagent implements, a fresh subagent reviews. Run under
  [D-0013](../docs/decisions/D-0013-task-time-and-disk-budget.md): claim to merge against the
  6-hour target, new evidence under 1 GB. Usage at the claim: 5-hour 8%, weekly 20%.
- Branch `task/save-image-exact` in `.worktrees/save-image-exact`; evidence in main
  `local/evidence/save-image-exact/`.
- Task provider: Anthropic.
- Milestone: M4. Review tier: **1** (cartridge RAM state and its write timing).

SAVE-FILES (R-0090) keeps the whole image, but bytes native keeps no field for stay as loaded or as
the cold start left them: `$074C-074F`, `$10AD`'s values other than 1, `$10A7` written back as
loaded, `$0750`'s race words, the league totals a 2P or VS result writes (`$0618 + 16 x slot`,
R-0095), and the write timing of `$10B1` and `$1115-1117`; a save taken during a race lacks what the
race writes until it returns (R-0090, R-0095, R-0100, R-0105). Outcome: native's image equals the
original's byte for byte at every frame the original writes cartridge RAM, over the corpus's warm and
cold routes (the ones R-0109 lists), or each remaining byte is recorded with why it cannot matter.

## Checkpoint (worker, 8 November 2026)

[R-0111](../docs/research/R-0111-save-image-exact.md) holds the detail.

- **Measured first.** The original's cartridge RAM after every frame of R-0109's 919 routes
  (5,048,046 frames; `original_trace.py`, change logs, 12 MB), and native's image after every frame
  (the runner's new `--cartridge-trace`), compared at every frame the original's image changes
  (367,329 write frames) and, through the logs, at every frame. Main: 355,423 of the 363,007 write
  frames from frame 406 differ; 16 of 916 routes are equal there.
- **Changes.** Power-on's `$FF` image read like a save; `$10AD`'s every value at the original's
  stores; `$10A7`, `$10D1-10D2` and the per-rider `$10FD` as the menus store them; `$83:9987`'s
  track words, the race's setup `$80:99A4-9A23`, the podium's `$80:BE8A-BEB1` and `$80:999F`, the
  idle demo's `$0742` 0x0A and `$83:C8E0`, the picks' `$0750` copies; the song counter on the
  race's sound load and the tutorial bits as the hints end, during the race, through two core hooks
  the runner and the app share (`update_race_loading`, `update_race_cartridge`); `$80:F88D`'s
  totals in every mode, at the cursor word; `$073E/$0740`; the names' checksums kept as loaded
  while the names are.
- **Decisions.** The bytes no later boot reads (the race's own words, the work RAM copy, the
  checksums without a reader, league, ending and race work bytes: `residual.txt`) are recorded, not
  modelled; native keeps no work RAM layout for the copy. The checksums without a reader keep the
  boot's, as on main: following their records was tried and put more write frames apart (192,449
  differing against 183,128), since the original sums them only at the end of each update; the
  measure decides where nothing reads them. A pack without qualifying scores (before v17) leaves `$0753`.

## Handoff (worker)

- Branch `task/save-image-exact` on claim `20f1eab4` (main `901018a5`). Commits: `e41810f9` (the
  runner's `--cartridge-trace`, no behaviour change), `bccd3914` (the image changes and tests),
  `d6604a38` (the credits demo's `$1115` flip, found by the behaviour run; `bccd3914`'s message
  claims no picture change, which held only after this), `03717daa` (only the names' checksums
  follow their records; the others keep the boot's, as on main), then the records commit (R-0111, this
  handoff, R-0109's pointer, BUILD_AND_VALIDATION's runner line, the regenerated native symbols
  and static map).
- **Review tier 1** (cartridge RAM state and its write timing). `src/core` and `src/app` change;
  the save format and the pack profile do not (the image is the original's layout, old saves load
  as before). The saved bytes do change: a save written by the candidate differs from main's.
- **Outcome partly met.** The image is not byte-equal at every write frame: outside the bytes no
  later boot reads, 709 of 916 routes are equal from frame 406, and on the other 207 14,380 write
  frames differ, mostly timing shifts R-0111 argues; the rest (hunter-effects' personal bests from a
  race difference, the endings' `$10A7`, the Value list) has no cannot-matter argument yet and is
  queued as [SAVE-IMAGE-REMAINDER](SAVE-IMAGE-REMAINDER.md). Figures below as of review round 1's
  residual (`$02BE-02BF`, `$073E-0741`, `$10F9-10FC` added).
- Results (R-0111; 916 of 919 routes, every frame; main is `e41810f9`'s runner, main plus the
  trace option):

| Measure | Main | Candidate |
| --- | ---: | ---: |
| write frames (the original's image changes), from frame 406 | 363,007 | 363,007 |
| differing at those | 355,423 | 181,547 |
| differing outside the bytes no later boot reads (`residual.txt`) | 353,073 | 14,380 |
| routes equal at every write frame from 406 | 16 | 90 |
| routes equal outside the residual bytes | 16 | 709 |
| bytes that differ somewhere (any frame) | 4,099 | 3,727 |

  Per group (routes differing from 406, main to candidate): `$0618` totals 406 to 2, `$073E` 328
  to 12, `$0744-074B` 789 to 20, `$074C` 789 to 7, `$074D-074F` 780 to 6, `$0750` 795 to 7, `$106B`
  777 to 0, `$10A7` 884 to 12, `$10AD` 160 to 6, `$10B1` 764 to 1, `$10D1-10D2` 142 to 0, `$10FD`
  120 to 0, `$1115` 14 to 5, `$1116-1117` 374 to 4. Unchanged and recorded: the race words and
  work RAM copy (777), the checksums without a reader (as main), `$1118` (446), league and ending
  work bytes; the wipe's frames 403-404 on every cold route; timing shifts listed in R-0111.
- Behaviour: main's runner and the candidate's equal on stdout, stderr, cues, race, one-player and
  look timelines and every frame's picture and VRAM digest on all 919 routes at `bccd3914` but
  `credits-name/demo-credits` (fixed in `d6604a38`; the 26 demo and credits routes rerun equal), 11
  routes stopping identically on both sides (`behaviour-bccd3914.json`,
  `behaviour-final-demo-credits.json`); at `03717daa` (image only) `behaviour_check.sh`'s 15 routes
  equal.
- The app: its save equals the runner's image at 11 points of two one-player races (`app_save.sh`,
  at `03717daa`).
- Checks run: `project.py build` `status=passed` for lab-release, lab-debug and app-debug; `ctest`
  lab-debug 41/41 and app-debug 41/41 (new `save_image_exact_tests`); `clang-tidy` clean on every
  changed `.cpp` (`-p build/lab-debug`, `src/app` with app-debug; three functions split to stay
  under 80 lines: `take_rider_pick`, `run_loading_frame`, `set_up_demo_race`); `git-clang-format`
  on the changed lines; `coverage native-symbols --check` `status=passed` (0 citations without a
  record); the static map regenerated with the eight inputs (`coverage-census/regen.sh`) and
  `tests.tooling.test_native_symbols`, `test_static_map`, `test_tracks` OK. ASan presets not run
  (hang on this host). Gates not run by the worker.
- Mutants (`mutants.py`, each change reverted alone, lab-release rebuilt, `check_image.sh`;
  `mutants/mutants.txt`): 18 changes (power-on, `$10AD`, `$10A7`, `$10D1`, `$10D2`, the song
  counter at the choice, the tutorial bits at the return, `$83:9987`'s words, the race's setup
  words, the demo's `$83:C8E0`, the demo's option byte, the picks' `$0750`, `$80:F88D` outside a
  league, `$073E/$0740`, the names' checksum kept as loaded, the reveal per rider, the podium's
  places, the flag keeping its byte): 17 make `check_image.sh` fail; the reveal per rider survives
  the corpus (no route has a pending reveal with `$CA` other than 0) and fails ctest
  (`save_image_exact_tests`).
- For the gates: `--task-checks local/evidence/save-image-exact/checks.list`: `check_image.sh`
  (1 min, the corpus sweep against `expected.json`), `app_save.sh` (4 min), `behaviour_check.sh
  "$B/front_end_runner"` (15 routes, every picture, about 3 min). Each was run from this worktree
  (a path with a space) and fails on a mismatch or a missing output. Expected against main: only
  the cartridge RAM digests (`R`) move; no picture, row, cue or timeline.
- Limits (R-0111): the residual bytes are argued from R-0090's perturbation and the listing
  (`$10B2`, `$10B6-10BE` a hypothesis); the wipe's split over frames 403-405 is not modelled (a
  save from those two frames boots 3 frames sooner, same records); the league pair's swap
  (`$83:95C4`, `$83:95CA`), the endings' own `$10A7` and the race's words are not modelled; the
  pending reveal is one field shown at the current rider; the app is compared on one route; 3
  corpus routes poke bytes the runner has no `--record-write` for and are not compared.
- Evidence: main `local/evidence/save-image-exact/`, 46 MB: the scripts (`routes.py`,
  `original_trace.py`, `native_trace.py`, `compare.py`, `report.py`, `groups.py`, `classify.py`,
  `where.py`, `show.py`, `ranges.py`, `sweep.sh`, `expected.py`, `check_image.sh`,
  `behaviour_same.py`, `behaviour_check.sh`, `app_save.sh`, `mutants.py`, `checks.list`), the
  inputs (`routes.json`, `residual.txt`, `expected.json`), the original's change logs
  (`original/`, 12 MB), main's and the candidate's (`native-main/`, `native-candidate/`), the
  reports (`report-base-vs-candidate.txt`, `groups-*.txt`, `classify-candidate.*`), the behaviour
  runs, `mutants/` and `base-trace/` (main's runner with the trace option, `e41810f9`). Private
  `TMPDIR` under the worktree's `artifacts/tmp`.
- Time: worker 10:04:15 to 12:32 UTC on 8 November 2026 (`date -u`), about 2 h 28 min: reading and
  the trace option 20 min; the original's capture 26 min (alongside the first changes); the changes
  and sweeps about 1 h; the full behaviour run 61 min (records written alongside); mutants (25 min),
  the checksum decision and the closing checks after.

## Review round 1 fixes (worker)

Review 1 (main `local/evidence/save-image-exact/review-1/review.md`, approve with should-fixes) on
`85962576`.

- Commits: `bd229f63` (a save with league cursors the game never writes no longer ends a 2P, VS or
  1P result: `$80:F88D`'s store past the 8 KiB is skipped (round 2: stored at its mirror instead), with a unit test; `store_field_byte`
  casts before its shift), then the records commit (R-0111, this handoff, R-0109, BUILD_AND_VALIDATION,
  the two queued tasks and their README rows, the regenerated native symbols and static map).
- 1. The records say plainly that the outcome is met in part (R-0111's answer line, the
  handoff's, R-0109's pointer).
- 2. Queued [SAVE-IMAGE-REMAINDER](SAVE-IMAGE-REMAINDER.md) (hunter-effects' personal bests from
  native's race increments at `$085F`/`$085B`, `race_update.cpp:852`; the endings' `$10A7`; the
  rest of the Value list) and [OPPONENT-POSE-BOUNDS](OPPONENT-POSE-BOUNDS.md) (`presentation.cpp:243`
  for opponent `0x14` under a hardened build); both named in R-0111's limits.
- 3. `$02BE-02BF` (the members' checksum), `$073E-0741` and `$10F9-10FC` argued from the listing
  and added to `residual.txt`; `expected.json` re-recorded. Outside the residual bytes: 14,380
  write frames on 207 routes differ (was 17,544 on 214); 709 of 916 routes equal (was 702). Main
  on the same residual: 353,073 and 16. The rest of the numbers are unchanged (181,547 differing
  write frames from 406, 90 routes equal there).
- 4. R-0111's residual section cites R-0090 for what it covers and review 1's perturbation (9
  warm routes, two ways, with its positive controls) for the rest; `$10B2` stated as without a
  capture.
- 5. `check_image.sh` fails on a runner exit code other than the recorded ones (the 3 refused
  pokes); `app_save.sh` adds R-0089's `diff-fuzz-135` (g020), points 1598, 1599, 2400, 3199 around
  its hints' end; `store_result_totals` skips past 8 KiB; the shift cast.
- 6. Recorded in R-0111's limits and timing list: `--records`/`R` skip the demo race's frames (also
  in BUILD_AND_VALIDATION), the credits demo's `$83:C8E0` a frame early, one core per process.
- Mutants (`mutants/`): an app built without the race hook's tutorial bits: `app_save.sh` DIFFERS
  at g020 1599, 2400 and 3199 (`app-tutorial-at-return/app_save.out`); a runner that exits 1 after
  complete outputs: `check_image.sh` DIFFERS on the exit codes (`runner-exits-1/`); the 8 KiB check
  removed: ctest fails (`array::at`).
- Checks: `ctest` lab-debug and app-debug 41/41; `clang-tidy` clean on `cartridge_ram.cpp` and
  `front_end_tests.cpp`; `git-clang-format` clean; `coverage native-symbols --check` passed (0
  citations without a record); the static map regenerated (`coverage-census/regen.sh`, the tooling
  tests OK); checks.list run once from `artifacts/review 1 checks` (a path with a space, `$B` main's
  runner with the trace option), 13:45 to 13:52 UTC: `check_image.sh` equal to the recorded result
  on every route, `app_save.sh` 15 of 15 points equal, `behaviour_check.sh` 15 of 15 routes equal
  (`review-1-fixes-checks.log`).
- Time: 13:28:43 to 13:54 UTC on 8 November 2026 (`date -u`), about 25 min.

## Review round 2 fixes (worker)

Review 2 (main `local/evidence/save-image-exact/review-2/review.md`, approve with one should-fix and
two notes) on `d6483757`.

- Commit `fd292d08`: corrupt league cursors now land where the original's store does.
  `store_result_totals` computes `$80:F88D`'s index in 16 bits, and `store_record_byte` stores at
  the 8 KiB's mirror (`& 0x1FFF`), through whichever record field covers the byte (a `Patch` codec
  over `transfer_records`), else into the bytes native keeps no field for. The unit test now expects
  `$0616-0619` for all cursors `0xFF` and `$0618` for a cursor word of 0x1000. Note 2: renamed
  `records.active_league_members` to `league_members_checksum`, which is what `$02BE` is
  (`$83:9157-916A`), with a comment that native stores the edited slot's members there. Note 3: the
  comment sits on the store it explains. Then the records commit (R-0111's limits, this subsection).
- Results: on review 2's five poked runs (`routes-cursor.json`, the original's logs in
  `original-cursor/`), native's image equals the original's at every write frame outside the
  residual bytes. The exception is `$0743`'s one-frame bit timing on 4 of them, as on the unpoked
  routes. Main differs on 517-531 write frames each. A check line: `check_cursor.sh`, recorded in
  `expected-cursor.json`, about 30 s. Mutant: the round-1 skip restored makes it fail ("outside the
  residual bytes 1993"; `mutants/cursor-skip/`). `expected.json` is unchanged: `check_image.sh` is
  equal on every route, since no corpus route's store reaches past the 8 KiB or an unaligned field
  byte.
- Checks: `ctest` lab-debug and app-debug 41/41; `clang-tidy` clean on `cartridge_ram.cpp`,
  `league_menu.cpp`, `options_menu.cpp` and `front_end_tests.cpp`; `git-clang-format` applied;
  `coverage native-symbols --check` passed (0 citations without a record); the static map
  regenerated. checks.list (now four lines) ran once from `artifacts/review 2 checks` with `$B`
  main's plain `front_end_runner` (built at `901018a5` in `.worktrees/save-image-exact-main`):
  14:21 to 14:29 UTC, `check_image.sh` equal to the recorded result on every route,
  `app_save.sh` 15 of 15 points equal, `check_cursor.sh` equal to `expected-cursor.json`,
  `behaviour_check.sh` 15 of 15 routes equal (`review-2-fixes-checks.log`).
- Time: 14:13:40 to 14:29 UTC on 8 November 2026 (`date -u`), about 20 min.

## Gates round 1 (`e0922dbf`)

The coordinator's gate run (main `local/evidence/save-image-exact/gates-1/`, `gates-1.out`) came
back `status=failed` for two reasons. The worker did not run the gate runner.

- **The corpus recorder.** checks.list's `check_image.sh` and `check_cursor.sh` passed their own
  comparisons. But the recorder (`tools/unirally_lab/gates/corpus.py`) refused all 919 runner
  invocations: `unknown front_end_runner option '--cartridge-trace'`. Fix (this commit):
  `--cartridge-trace` is an output (`OUTPUTS`, one value) whose stream is the replay's `R` digests
  (`records`). `normalize` drops it from the inputs, so a traced invocation keys as the plain one,
  and replay runs main's runner without it. New test `test_a_cartridge_trace_is_an_output` in
  `tests/tooling/test_gates.py`. `test_gates` and `test_gate_identity` give 31 tests OK. With the
  option taken out of `OUTPUTS`, the new test errors.
- **18 checks against the original changed** from their run on main. Each moves the image toward
  the original's; no other output moves:
  - `checks4` (2P and VS warm records): the `$1116` and `$106C` differences from the original are
    gone.
  - `lt-tours-a-w0` to `lt-tours-a-w8`, `lt-tours-b-w0`, `lt-tours-b-w1`, `lt-odd-tours-w0`,
    `lt-odd-tours-w1`, `lt-podium-back`, `lt-t-crawler`: in the league tours' records comparison,
    the image's ranges that differ from the original shrink (`$106C`, `$10B1`, `$10D1`,
    `$1115-1117` gone).
  - `lt-five-w0`: as the league tours. Main's list there is cut at 12 ranges by
    `league-tours/summarise.py`. Rerun in full with main's binary (`gates-1/base-bin`,
    `901018a5`), it has 14: `$106C`, `$106F-1072`, `$10AF`, `$10B1`, `$10B4`, `$10BD`, `$10BF`,
    `$10C1`, `$10C3`, `$10C5-10C6`, `$10CF`, `$10D1`, `$10F3`, `$1116-1117`
    (`gates-1-followup/records-five-w0-main.json`). So `$10F3` differs from the original on main
    too. The candidate's 10 ranges are main's less `$106C`, `$10B1`, `$10D1` and `$1116-1117`.
  - `rs-records`: `wr-stats`, `wr-stats2` and `wr-track` go from 1 differing byte (`$10A7`) to 0;
    `wr-1p-ca` from 223 bytes to 206.
- **fe-replay.** All 519 cases differ only in the `records` stream (the cartridge RAM). Every
  other stream is equal.
- For `--expect-changed`:
  `checks4,lt-tours-a-w0,lt-tours-a-w1,lt-tours-a-w2,lt-tours-a-w3,lt-tours-a-w4,lt-tours-a-w5,lt-tours-a-w6,lt-tours-a-w7,lt-tours-a-w8,lt-tours-b-w0,lt-tours-b-w1,lt-odd-tours-w0,lt-odd-tours-w1,lt-podium-back,lt-t-crawler,lt-five-w0,rs-records`

## Gates round 2 (`87571019`)

- `gates-2` at `87571019` (17:13-18:02 UTC, 2,914 s): `status=passed`, with `--only` the task's 4
  checks, the 18 checks round 1 found `changed`, `fe-replay` and `synthetic`, and
  `--expect-changed` the 18 (each read in round 1, every one toward the original). The task's 4
  checks rc 0; the 18 `changed (expected)`; `synthetic` passed (the new recorder test included);
  `fe-replay` 535 recorded runner invocations, 16,431,946 frames, every one differing in the
  `records` stream only (the cartridge RAM) and equal on every other stream, rc 0 on both sides.
- Round 1 ran every other step on `e0922dbf`: 85 checks against the original `same-as-main`, 20
  passed, 19 skipped (the frozen race gates and their sweeps: no race binary input changed;
  `pa-anchors` and the conditional audio checks read only the original's captures). Between the
  two rounds only `tools/unirally_lab/gates/corpus.py` and its test changed, which no native
  binary reads.
- Hosted CI: the first run on `d0d41da4` failed the Linux job: GCC's `-Wrange-loop-construct`
  (an error under `-Werror`) on `race_result.cpp:374`, a range-for over pairs bound by value.
  Bound by reference in the next commit (no behaviour change: the pairs are two constants); every
  C++ file this branch changes then compiles cleanly under GCC 13.3 (Ubuntu 24.04) with CI's
  warning flags (`-fsyntax-only`, in a container), and lab-debug ctest is 41/41.
- The second run (`353e7619`) failed the Linux sanitizer build: `-Wsign-conversion` on
  `cartridge_ram.cpp:136`, a `std::uint16_t` mask promoted to `int` before its shift. Widened to
  `unsigned` first (no behaviour change). Every changed C++ file but `sdl_main.cpp` (SDL3 has no
  Ubuntu package) then compiles to object code under GCC 13.3 with CI's flags, with and without
  `-fsanitize=address,undefined`; ctest 41/41 on lab-debug and app-debug.

## Review (rounds 1 and 2)

A fresh subagent in its own checkout at the candidate.
- [Round 1](https://github.com/malmazuke/unirally-reconstruction/pull/99#pullrequestreview-5819004428)
  at `85962576` (12:33-13:28 UTC): approve with should-fixes, handled in "Review round 1 fixes".
- [Round 2](https://github.com/malmazuke/unirally-reconstruction/pull/99#pullrequestreview-5819004498)
  at `d6483757` (13:53-14:13 UTC): approve with one should-fix (the original's store for corrupt
  cursors), handled in "Review round 2 fixes" and checked by `check_cursor.sh`.

## Handoff (closeout)

- Commits on `task/save-image-exact`: `20f1eab4` (claim), `e41810f9` to `85962576` (worker),
  `bd229f63`, `d6483757` (review 1's fixes), `fd292d08`, `e0922dbf` (review 2's fixes), `87571019`
  (gates 1's recorder fix), then this record.
- Time, claim 10:03 UTC to merge about 18:30 UTC, about 8 h 25 min against D-0013's 6 hours:
  worker 2 h 28 min; review round 1 55 min; fixes 26 min; round 2 20 min; fixes 18 min; gates
  round 1 2 h 35 min; the recorder fix 7 min; gates round 2 49 min; PR and closeout about 30 min.
  The overrun is the second gate round (the recorder did not know the task's new option) and two
  review rounds of fixes.
- Disk: main `local/evidence/save-image-exact/` 120 MB after `compact_evidence.py`, under the 1 GB
  budget; the reviews' output in `review-1/` and `review-2/` there.
- The outcome is met in part (R-0111's answer): SAVE-IMAGE-REMAINDER and OPPONENT-POSE-BOUNDS are
  queued. The task's four checks (`checks.list`) are not in the catalogue.
- The closing session removes `.worktrees/save-image-exact` and `.worktrees/save-image-exact-review`
  and the local branch, then bootstraps and rebuilds app-debug in the main checkout.
- Next in R-0109's queue: CENSUS-REMAINDER (tier 3); SAVE-IMAGE-REMAINDER and
  OPPONENT-POSE-BOUNDS are queued behind the queue's tier 1 items.
