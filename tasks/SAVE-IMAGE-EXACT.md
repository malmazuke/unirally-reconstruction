# SAVE-IMAGE-EXACT - the saved image equal to the original's at every save point

## Assignment

- Status: **claimed** 8 November 2026 10:03 UTC on main `901018a5`, on the user's instruction
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
- Results (R-0111; 916 of 919 routes, every frame; main is `e41810f9`'s runner, main plus the
  trace option):

| Measure | Main | Candidate |
| --- | ---: | ---: |
| write frames (the original's image changes), from frame 406 | 363,007 | 363,007 |
| differing at those | 355,423 | 181,547 |
| differing outside the bytes no later boot reads (`residual.txt`) | 353,194 | 17,544 |
| routes equal at every write frame from 406 | 16 | 90 |
| routes equal outside the residual bytes | 16 | 702 |
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
- Time: worker 10:04:15 to 12:38 UTC on 8 November 2026 (`date -u`), about 2 h 34 min: reading and
  the trace option 20 min; the original's capture 26 min (alongside the first changes); the changes
  and sweeps about 1 h; the full behaviour run 61 min (records written alongside); mutants (25 min),
  the checksum decision and the closing checks after.
