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

Worker: Claude Opus 5.5 (`claude-opus-5-5`) subagent, 12 November 2026 23:07 UTC - 13 November 00:25 UTC,
about 1 h 20 min (harness clock), in `.worktrees/opponent-pose-bounds`.
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
- **The hardened corpus:** main aborts on 1,238 of 1,924 cases (the colour maths); the candidate on
  none, with pictures drawn on the checks' frames; 11 cases exit 1 with the runner's own refusals on
  both.
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
  lower view's choice); the result skip is pinned by `check_result_oam.sh` against the original
  rather than a synthetic-content test.
- The hardened build is a throwaway (`build/hardened`, removed); `check_hardened.sh` builds its own in
  `$G`.

## Evidence

All in main `local/evidence/opponent-pose-bounds/` (102 MB, most of it the two OAM captures).

- `hardened_sweep.py`: every corpus case through one runner. Main hardened, no pictures:
  `sweep-main-nopics-p0..3.json` (1,249 failing: 1,238 subspan aborts, 11 refusals). Candidate after
  the colour-maths fix, with pictures: `sweep-cand-pics-p0..2.json` (16 `std::array` aborts in
  `personal_best`, three eighths swept); after both fixes: `sweep-cand2-pics-rerun012.json`,
  `sweep-cand2-pics-p3..7.json` (only the 11 refusals, `expected-refusals.json`).
- The original's colour maths: `hunter-effects/review-withheld/w-rev-buttons` (existing capture),
  `$0748-0750` and `$7E:2084-2095` read at frames 1300, 1395, 1400, 1500, 2500 (command in R-0118).
- The original's result OAM: `oam-cont-win`, `oam-cont-loss` (`access capture` of
  front-end-1p-continuation's manifests, copied here, `$0A00-$162F` every frame), compared by
  `result_oam.py`.
- `replay-main-vs-candidate.json`: the gates' replay (`tools.unirally_lab.gates.replay`) of the whole
  corpus, main's lab-release runner (`main-bin/front_end_runner`, built at the claim `272990e6`)
  against the candidate's.
- Checks (`checks.list`, `$G` with a space): `check_hardened.sh` PASS on the candidate (built from
  the checkout, 1,924 cases, 11 expected refusals; `check-hardened-candidate.log`), FAIL on main's
  hardened runner `main-bin/front_end_runner.hardened` (1,238 unexpected aborts;
  `check-hardened-main.log`); `check_result_oam.sh` PASS on the candidate (0 of 243 and 0 of 762
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
