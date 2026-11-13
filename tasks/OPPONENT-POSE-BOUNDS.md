# OPPONENT-POSE-BOUNDS - a computer opponent's colour maths read past its table

## Assignment

- Status: **claimed** 12 November 2026 23:06 UTC on main `e11cbe11`, on the user's instruction ("next
  task", after SAVE-IMAGE-REMAINDER #105). Queued 9 November 2026 by
  [SAVE-IMAGE-EXACT](SAVE-IMAGE-EXACT.md) (review round 1). Claude Code desktop app, Claude Opus 5.5
  (`claude-opus-5-5`): coordinator and integrator; a worker subagent captures and implements, a fresh
  subagent reviews. Run under [D-0013](../docs/decisions/D-0013-task-time-and-disk-budget.md): claim
  to merge against the 6-hour target, new evidence under 1 GB. Usage at the claim: 5-hour 2%,
  weekly 1%.
- Branch `task/opponent-pose-bounds` in `.worktrees/opponent-pose-bounds`; evidence in main
  `local/evidence/opponent-pose-bounds/`.
- Task provider: Anthropic.
- Milestone: M4. Review tier: **1** (race presentation content for a scenario the menus reach).

A libc++-hardened debug build (`_LIBCPP_HARDENING_MODE_DEBUG`) aborts on the corpus route `c175`
(`hunter-effects`, R-0109): `presentation.cpp:243`
`colour_math.subspan(colour_math_size * scenario.pairing.opponent, colour_math_size)` with the
computer opponent `0x14` (ANTI-UNI), called from `classic_race_presentation_content` from the
runner's `start_race`. The release build reads past the table without a check; a Linux debug run of
that route would abort. The code and the scenario are main's (SAVE-IMAGE-EXACT did not change them).
Outcome: the original's colour maths for opponents past the table recovered from the listing and
used, or the read bounded as the original's is, with a test on opponent `0x14` and the route's
pictures unchanged.

## Results

Worker: Claude Opus 5.5 (`claude-opus-5-5`) subagent, 12 November 2026 23:07 UTC - 13 November 00:21 UTC,
1 h 14 min (harness-measured), in `.worktrees/opponent-pose-bounds`.
[R-0118](../docs/research/R-0118-opponent-pose-bounds.md) holds the findings.

- **What the original does:** `$82:D4DC` has entries for the 16 rider characters only (64 bytes;
  `$82:D51C` is code). The lower lines take the second rider's entry (`$77:0749`) only in a race with
  two views (`$82:D5BE-D5C4`, `$77:0750` bit 3), always between two humans; otherwise the player's
  again (`$82:D5CC`). The original's HUNTER race against ANTI-UNI (`w-rev-buttons`, frames
  1395-2500) has MIKE's entry `2F 40 80 04` in both halves of `$7E:2084-2095`.
- **Native:** `rider_colour_math_entry` gives a rider character's entry and nothing for a computer
  opponent; `lower_view_colour_math` picks the second rider's only with two views (refusing two views
  without one), and the lower ink and CGADSUB use it. No picture changes: native drew the lower
  view's ink only in split races.
- **A second read of the same kind** found by the hardened corpus run: the one-run result's row for a
  computer opponent. The original skips it (`$80:D011-D014`; the census's trace: 299 of 327 entries
  take the branch); native printed it, indexing the best times with rider 16 in a league against the
  computer (past the 800 words) and, on every one-run result against the computer, placing the
  new-best markers 97 and 99 on a fifth row and loading its colours. Native now skips it: its OAM
  equals the original's on all 243 + 762 result frames of `cont-win` and `cont-loss` (main differs
  on 241 + 760).
- **The hardened corpus:** main aborts on 1,238 of 1,924 cases (the colour maths, no pictures
  drawn). The final runner (both fixes, review round 1) aborts on none of the 1,924 with the pictures
  drawn on every frame the checks compare (`sweep-final-pics.json`, 22 min); 11 cases exit 1 with the
  runner's own refusals on both.
- **Main against the candidate** (the gates' replay of the whole corpus): 408 of 1,924 cases differ,
  in stdout only (the OAM buffer's entries 97, 99, 101 and 103 and CGRAM colours 0xC0-0xCF, from a
  one-run result against the computer until a later screen rewrites them); no picture, cartridge
  RAM, cue or timeline changes. `c175`'s pictures are unchanged.

## Decisions (worker)

- The colour-maths fix bounds the read as the original does (by the views), rather than extending the
  table: the bytes past it are `$82:D51C`'s code, which the original never reads as colour maths.
- A computer opponent's `opponent_colour_math` is empty rather than a copy of the player's, so a
  two-view race with one fails loudly (`lower_view_colour_math`) instead of drawing a guess; no route
  reaches it.
- The result-row skip tests `now_playing.opponent` (native's `$017F` at the result, as `sorted_rows`
  and `$00CA` already do); the printed row's rider stays `second_rider` (equal to it for every human
  opponent), unchanged.
- The second fix is in this task rather than queued: it is the same class (an index by a computer
  opponent past a table), the listing and the census decide it, and a capture of the original shows
  native moving to the original's OAM.
- Unit tests are ROM-free (`race_pairing_tests`: the entry for 0-15, none for 16 and 0x11-0x14, the
  lower view's choice; `front_end_tests`, review round 1: a one-run result against BRONSEN with
  `second_rider` 0 and 16 leaves entries 97 and 99 off the fifth row's line, 101 and 103's
  attributes alone and every other best time unchanged; it fails with the skip removed). The
  result skip is also pinned against the original by `check_result_oam.sh`.
- The hardened build is a throwaway (`build/hardened`, removed); `check_hardened.sh` builds its own in
  `$G`.

## Evidence

All in main `local/evidence/opponent-pose-bounds/` (102 MB, most of it the two OAM captures).

- `hardened_sweep.py`: every corpus case through one runner. Main hardened, no pictures:
  `sweep-main-nopics-p0..3.json` (1,249 failing: 1,238 subspan aborts, 11 refusals). Candidate after
  the colour-maths fix, with pictures: `sweep-cand-pics-p0..2.json` (16 `std::array` aborts in
  `personal_best`, three eighths swept); after both fixes, with pictures on 1,224 cases:
  `sweep-cand2-pics-rerun012.json` (the 23 earlier failures), `sweep-cand2-pics-p3..7.json` (only the
  refusals). Review round 1: the final runner hardened, with pictures, on all 1,924 cases:
  `sweep-final-pics.json` (only the 11 refusals of `expected-refusals.json`, same messages).
- The original's colour maths: `hunter-effects/review-withheld/w-rev-buttons` (existing capture),
  `$0748-0750` and `$7E:2084-2095` read at frames 1300, 1395, 1400, 1500, 2500
  (`colour_math_original.py`, `colour-math-original.txt`).
- The original's result OAM: `oam-cont-win`, `oam-cont-loss` (`access capture` of
  front-end-1p-continuation's manifests, copied here, `$0A00-$162F` every frame, 0xC20-byte
  rows), of which `result_oam.py` compares the OAM buffer `$0A00-$0C1F`.
- `replay-main-vs-candidate.json`: the gates' replay (`tools.unirally_lab.gates.replay`) of the whole
  corpus, main's lab-release runner (`main-bin/front_end_runner`, built at the claim `272990e6`)
  against the candidate's.
- Checks (`checks.list`, `$G` with a space): `check_hardened.sh` PASS on the candidate (built from
  the checkout, 1,924 cases, 11 expected refusals; `check-hardened-candidate.log`), FAIL on main's
  hardened runner (1,238 unexpected aborts; `check-hardened-main.log`). Since the gates it sweeps the
  corpus's catalogue cases only (Closeout): PASS on the candidate (444 cases), FAIL on main built
  hardened from `e11cbe11` (104 aborts, all the colour maths' `subspan`); `check_result_oam.sh` PASS on the candidate (0 of 243 and 0 of 762
  result frames differ), FAIL on main's `main-bin/front_end_runner` (241 and 760;
  `check-result-oam/`).
- Builds: `project.py build` `status=passed` for lab-release, lab-debug and app-debug; `ctest` 41/41
  on lab-debug and app-debug (`race_pairing_tiers_launches_voices_ink` runs the new test);
  `clang-format` (src/core's `.clang-format`) applied to the three changed `src/core` files. ASan
  presets not run (they hang on this host).
- GCC 13.3 (`ubuntu:24.04`, CI's warnings as errors), plain and sanitized: OK on
  `presentation.cpp` and `race_pairing_tests.cpp` (`gcc.sh`, `gcc.out`).
- `coverage-census/regen.sh` (eight inputs) regenerated the static map and native symbols (tooling
  tests OK); `coverage native-symbols --check` `status=passed`.

## Handoff

- Commits on `task/opponent-pose-bounds`: `e8c9eec9` (native: `presentation.cpp/.hpp`,
  `race_result.cpp`, the unit test), then the records commit (R-0118, this record, the static map
  and native symbols).
- Done: the original's colour maths for computer opponents recovered and followed; the result row's
  read of the same kind fixed; the whole corpus run hardened; unit test on 0x11-0x14 and 16.
- Not done: no capture of the original's two-view branch (`$82:D5C6`) or of a league's result against
  the computer (both decided by the listing; R-0118 Limits). No new task queued: nothing in scope
  is left open.
- Not pushed, no PR, no review yet (tier 1).
- What the gates should expect (`--task-checks local/evidence/opponent-pose-bounds/checks.list`):
  - this task's two checks PASS on the candidate (both FAIL on main);
  - the corpus replay: 408 cases differ in stdout only, the steps `checks-checks-01` (327 cases),
    `fe-equivalence` (27), `mode-audio`, `audio-r0076` (6 each), `checks-checks-05`, `-40`, `-41`,
    `-42`, `-43`, `lt-five-w0`, `-w1`, `lt-odd-tours-w0`, `-w1` (4-5 each), `pause-menu` (2),
    `checks-checks-20`, `-21`, `pa-cues`, `rs-records`, `rs-review-wr-1p-ca` (1 each)
    (`replay-main-vs-candidate.json`). Those steps then run against the original; a check that
    reads the OAM buffer or CGRAM at a one-run result against the computer should move toward the
    original (as `check_result_oam.sh` shows); one that reads only pictures, cues or cartridge RAM
    should be unchanged.
  - no pack or profile change.

## Review round 1 fixes (worker)

Review: [PR #106 round 1](https://github.com/malmazuke/unirally-reconstruction/pull/106#pullrequestreview-5884256682)
(no must-fix, one should-fix, four notes).

1. Should-fix, the "with pictures" claim: the final runner, built hardened, ran all 1,924 corpus cases
   with their pictures (`sweep-final-pics.json`, through `heavy-run.sh`, 22 min): only the 11 expected
   refusals. Results, Evidence and R-0118 item 4 now say exactly what each sweep ran.
2. The capture range: `$0A00-$162F` captured (0xC20-byte rows), `$0A00-$0C1F` compared, in R-0118,
   this record, `result_oam.py` and `check_result_oam.sh`. The colour-maths read of `w-rev-buttons`
   is now a script (`colour_math_original.py`, output `colour-math-original.txt`).
3. `race_result.cpp`'s comment now says the best time is indexed by `second_rider` (0 in 1P, 16 in a
   league against the computer).
4. A ROM-free unit test in `front_end_tests` (`race_result_tests`): a one-run result against BRONSEN
   with `second_rider` 0 and 16; it fails with the skip removed (checked locally).
5. The two-view branch stays a recorded limit.

Checks after the fixes: lab-release and lab-debug `status=passed`, `ctest` 41/41 on lab-debug;
clang-format on `race_result.cpp`; GCC 13.3 plain and sanitized OK on the four changed files
(`gcc.out`, now with `front_end_tests.cpp`); `regen.sh` left the static map and native symbols
unchanged, `native-symbols --check` `status=passed`. The fixes change no runner output: the native
change is a comment, the rest is tests, records and evidence scripts.

## Closeout (coordinator)

- Review: tier 1, one round on PR #106. Round 1 (`25d284ad`): no must-fix, one should-fix (the "with
  pictures" claim covered 1,224 of the 1,924 cases) and four notes (the capture range written three
  ways, the `race_result.cpp` comment, no ROM-free test of the row skip, the unobserved two-view
  branch). All handled in `ef3327e8` and `d1c9d6c4`: a hardened run with pictures on all 1,924 cases,
  the range made consistent, the comment fixed, a new `front_end_tests` case; the two-view branch
  stays a limit. The only `src/core` change after review is a comment. Replies on the PR.
- Gates (D-0012) on `d1c9d6c4`, TMPDIR `/private/tmp/opbg`:
  - A first start failed in 0 s: `build-base` found no isolated toolchain, because the review
    checkout's bootstrap had pointed the shared manifest at itself and was then removed. Main's
    bootstrap rerun fixed it.
  - `gates-1` (2 h 47 min): 98 steps same-as-main, 20 passed, 103 against the original. Changed, all
    read: `fe-equivalence` with `fe-classify` (27 of 179 cases, the stdout differences the handoff
    lists), and three moving toward the original in OAM: `lt-odd-tours-w0` (frames differing in OAM
    3,177 to 549), `lt-odd-tours-w1` (9,912 to 8,325), `rs-review-wr-1p-ca` (642 to 1); nothing else
    in those steps changed (`lt-odd-tours-w0`'s main-side traceback is the same on both).
    `check_result_oam.sh` passed. `check_hardened.sh` failed: it required exactly 1,924 cases, but
    the corpus's `checks-checks-NN` entries hold the last task's own checks and each gate run replaces
    them (1,446 cases while it ran, 539 after). The check now sweeps only the catalogue's cases
    (`--catalogue`, 444) and asks for at least 400; it passes on the candidate and fails on main
    built hardened (104 aborts).
  - `gates-2` (`--only` and `--expect-changed` the five, 6 min): `status=passed`. `gates-3` (the two
    task checks, 2.4 min): both PASS.
- CI on `25d284ad`: macOS and Ubuntu lab jobs pass.
- Time: claim `272990e6` 23:06 UTC on 12 November 2026 to merge about 04:30 UTC on 13 November,
  about 5 h 25 min, inside D-0013's 6-hour target. Worker 1 h 14 min plus 26 min of review fixes;
  review 10 min; gates 2 h 47 min + 6 min + 2.4 min, after two false starts.
- Evidence: main `local/evidence/opponent-pose-bounds/`, 10 MB after `compact_evidence.py` (the unusable `main-bin/front_end_runner.hardened`,
  whose DSP library was in a removed build tree, deleted). Worktrees `opponent-pose-bounds` and
  `opponent-pose-bounds-review` and the branch removed after the merge.
