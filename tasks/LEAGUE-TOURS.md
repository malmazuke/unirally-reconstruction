# LEAGUE-TOURS - league beyond the first CRAWLER tour

## Assignment

- Status: **accepted** 16 October 2026 (tier 1, [#81](https://github.com/malmazuke/unirally-reconstruction/pull/81),
  review round 2 returned only for Linux CI, fixed in `9cf5541`), integrated by merge commit.
  Claimed 14 October 2026 on main `ceb50cf` (Claude Code desktop app, Claude Opus 5.5
  `claude-opus-5-5`; coordinator and integrator; a worker subagent implements, a fresh subagent
  reviews). Queued by [COVERAGE-GAPS](COVERAGE-GAPS.md) (R-0089, queue item 6). Weekly usage at
  claim 28%.
- Choice: R-0089's next queue item and the largest player-visible gap left there; ahead of the
  smaller RECORD-SCREENS and NAME-CHECK, SPLIT-HUNTER-PICTURES and the infrastructure task
  GATES-AGAINST-MAIN. No other session holds a claim. The user asked for the next task after the
  second evidence cleanup (#80).
- Branch `task/league-tours` in `.worktrees/league-tours`; evidence in main
  `local/evidence/league-tours/`.
- Task provider: Anthropic.
- Milestone: M4 (original game coverage).
- Review tier: **1** (league state, scoring and cartridge RAM in `src/core`).
- Dependencies: LEAGUE (R-0073), TWO-HUMAN-STUNTS (R-0095), SAVE-FILES (R-0090), MODE-AUDIO
  (R-0091), MENU-INPUT (R-0097).

## Why

LEAGUE (R-0073) accepted a league over the first CRAWLER tour: its setup, pairings, races,
scoring, standings, podium and continuation. R-0073 leaves the rest outside: the league's other
tours and their events (all 44 played events), tour progression and what the original does when
a league's tour is finished or a later tour is picked. R-0089 lists "league is covered for the
first CRAWLER tour only" among the whole features native lacks.

## Outcome

- A league played natively through every tour the original offers it, frame-exact against
  captures of the original: PICK TOUR's choices and locks for a league, each tour's events (lap,
  stunt and special tracks) with league pairing and scoring, the standings and podium after each
  tour, and the cartridge RAM's league fields (membership, played, points, pairing, cursor).
- What native does differently today is measured first (a capture of a second tour against main).

## Boundaries

- The league's first tour stays as accepted (R-0073's frozen gate unchanged).
- Presentation residuals R-0073 and R-0095 record stay with them unless this work fixes them.

## Checkpoint - 15 October 2026 (implementation worker, Claude Opus 5.5)

Record: [R-0098](../docs/research/R-0098-league-tours.md). Evidence: main
`local/evidence/league-tours/` (driven captures `tours-a`, `tours-b`, `odd-tours` with their
`*-drive.json` and windowed probes `probe-*-wN`; `picks-l0/1/2`, `podium-back`, `t-crawler`;
the sound schedule `cues/tour-cues*`; `checks.sh`, `final_checks.sh`, the baseline `base-ceb50cf/`;
the final run `final-29c5786/` and its `.out`).

**Native before** (main `ceb50cf`): a second tour started from the slots played like the first,
but after a podium PICK TOUR's cursor started on the last tour (`$00CE`/`$00D0` not stepped), so
the same presses chose another tour; HUNTER's tagged races with two humans diverged (rider 1's
pad not reversed); NEON with two views ran NEON's lighting and no tags; a two-human stunt
qualified on medal 0; a league's computer opponent on HUNTER was refused ("the one-player menus
cannot choose this race's pairing"); the awards printed in place, the continuation stepped the
decorations, Y after a podium went to the slots; event totals, the pair's count and the stunt
totals were written a few frames early; the lap result kept no second human's personal best.

**Native now**: every R-0098 finding 2-15 (commits below). Measured at `29c5786` (`final-29c5786/`):

| Capture | League races: rows equal | League fields (every 25 frames) | Pictures equal (native; main) |
|---|---|---|---|
| `tours-a-w0` SHUFFLER | 19,608 of 19,608 | 1,211 equal | 3,096 of 3,206; main 2,896 |
| `tours-a-w1` RUNNER | 15,772 of 15,772 | 1,088 equal | 2,908 of 3,799 |
| `tours-a-w2` HUNTER | 18,372 of 18,372 | 1,124 equal | 2,235 of 3,889; main 1 (races 0 of 15,595: it plays another tour) |
| `tours-a-w3` JUMPER | 22,546 of 22,546 | 1,360 equal | 2,891 of 4,480 |
| `tours-a-w4` WALKER | 18,557 of 18,557 | 1,198 equal | 2,488 of 4,075 |
| `tours-a-w5` SPRINTER | 21,818 of 21,818 | 1,330 equal | 2,811 of 4,404 |
| `tours-a-w6` HUNTER | 18,372 of 18,372 | 1,124 equal | 1,706 of 3,890 |
| `tours-a-w7` SHUFFLER | 19,633 of 19,633 | 1,244 equal | 2,486 of 4,189 |
| `tours-a-w8` JUMPER, EXIT | 22,546 of 22,546 | 1,395 equal | 3,385 of 5,511 |
| `tours-b-w0` HOPPER | 21,383 of 21,383 | 1,323 equal | 4,550 of 4,656; main 4,350 |
| `tours-b-w1` BOUNDER, EXIT | 20,521 of 20,521 | 1,314 equal | 3,829 of 5,310 |
| `odd-tours-w0` HUNTER, 3 members | 18,372 of 18,372 (8 races) | 2,043 of 2,044 | 4,912 of 6,459; main refuses the race |
| `odd-tours-w1` BOUNDER, 3 members | 20,521 of 20,521 (10 races) | 2,424 equal | 4,934 of 7,003 |
| `t-crawler` CRAWLER, EXIT | 20,668 of 20,668 | 1,287 equal | 3,208 of 3,260 |
| `picks-l0/1/2` | - | 275 equal each | 1,699 of 1,700 each; main 1,699 (l0) |
| `podium-back` | 2,776 of 2,776 | 623 equal | 5,490 of 5,500; main 2,949 |

Every league race of every tour (all 44 league tracks, 76 races in the driven captures) is equal
on every update. Sound schedule `tour-cues`: 4 frames of 19,339 lines differ: the result's way
out (10603, R-0095's, as main) and the podium's own music (11420, 11501-11502). League frozen gate
(`league_frozen.py`, pack v39): passed, all six cases, every listed picture 0 px. Front-end sweep
against main `ceb50cf` (`fe_equivalence.py`, 179 manifests, 1,269,921 frames): 149 equal, 30
differ, all `league/` manifests, none with another exit; `classify.py` over their 301 original
frames: 10 change, 7 closer (the awards now slide), 3 further (`organic-three`,
`-three-clean`, `-three-complete` 15700: 111 to 123 px, the awards' rider icons in a manifest
already a frame off since R-0095's table-print overrun). Fields that move are this task's: the
awards' scroll and halves, `$00CE`/`$00D0`, the continuation's decorations, entries 104-111 at the
standings, the pair count's frame.

**Not covered** (R-0098): the podium's way back a frame late in about a third of podiums (the
sound upload's end; the arrow's spin and the palette cycle then stay a frame apart until the
league ends, which is most of the differing pictures in windows 1-8); the podium's own music;
HUNTER's effects in a two-view picture (SPLIT-HUNTER-PICTURES' domain; captures ready); a few
10-22 px race pictures where riders pass under the HUD; cycle-timed print overruns (the odd
league's standings after a first pair, `odd-tours` 27880; the pair count after a human-computer
result a frame early, `odd-tours` 35575); a race's own loading a frame early in `odd-tours` (NEON
and the next race; the comparisons pass the original's initialization); VS's two-view NEON
scenery 12 from the listing only.

## Handoff (worker)

- Branch `task/league-tours`, head `29c5786` plus this record's commit; base claim `3e0b69b`
  on main `ceb50cf`. Commits: `45a6582` (tour cursor, Y after podium, stunt totals, HUNTER
  reversal for pad 2, two-view NEON tags, two-human qualifying medal), `7dca7a5` (awards slide
  and two passes, continuation loop, second human's lap personal best), `e805f80` (league
  computer opponent, two-view NEON scenery, pair count, standings entry), `b70e3b3` (awards'
  frames by rows printed), `db0c615` (podium sound cues), `08e0dc9` (maps), `ea7bd90` (totals
  with the result's rows, count before the awards, R-0098, unit tests), `29c5786` (revert of an
  OAM copy that made picks-l0 6400 differ).
- Files: `src/core/league_menu.cpp`, `league_completion.cpp`, `tour_menu.cpp`, `now_playing.cpp`,
  `race_result.cpp`, `lap_result.cpp`, `stunt_result.cpp`, `race_update.cpp`, `race_setup.cpp`,
  `presentation.cpp`, `hunter_effects.cpp`, `front_end_runner.cpp`, `front_end.hpp`,
  `front_end_screens.hpp`, `zoom_zoo_movement.hpp`, `src/app/frontend.hpp`,
  `tests/native/league_tests.cpp`, `docs/research/R-0098-league-tours.md`, `docs/map/static/*`.
  No pack change (v39).
- Checks (`final_checks.sh`, `final-29c5786.out`): app-debug and lab-debug build, ctest 41 of 41
  each; clang-tidy 0 warnings on the changed `src/core` files (one function over 80 lines found
  and split); clang-format clean; `coverage native-symbols --check` current, 0 uncited;
  static map regenerated with the five raw coverage inputs (`regen.sh`). Synthetic suite and the
  coordinator's gates not run by the worker.
- Commands: `bash local/evidence/league-tours/checks.sh WORKTREE OUTDIR` (candidate on 18
  captures, main on six, the sound schedule); `final_checks.sh OUTDIR`; captures per R-0098
  Reproduction. The heavy evidence is about 13 GB (`probe-*` series).
- Next: gates and the tier-1 review. Follow-ups worth queueing: PODIUM-RETURN-TIMING (predict the
  podium's way back from the sound processor's state; removes the spin and palette-cycle drift),
  PODIUM-AUDIO (the podium's music session), and SPLIT-HUNTER-PICTURES (already queued) with
  `tours-a-w2/w6` and `odd-tours-w0` as captures.

## Review round 1 (returned on `876aa89`) - the fixes

Review: https://github.com/malmazuke/unirally-reconstruction/pull/81#pullrequestreview-5530448051
(main `local/evidence/league-tours-review/review-876aa89.md`). The worker's fixes are `b8a1131`
to `b6c00e1`; its final checks at `b6c00e1` are in main `local/evidence/league-tours/final-r1/`.
That run took 12 hours, most of it the reviewer's `five` capture, about 160,000 frames.
`checks.sh` now runs the review's `five` (two windows) and `backs` captures too.

1. **Blocker, the least-wipeout bonus:** fixed in `b8a1131`. `$83:97A4` compares `$77:07BD` and
   `$0827`. Only the race's setup writes them, so they stay 0, and native now keeps them 0
   (R-0098 finding 16). In `five` window 0, all 15 league races are equal. Its league fields
   differ on only one sampled frame each for played points and the pair cursor, the
   pair-timing class below. The bonus holder is equal.
2. **Odd leagues' pairs a frame apart:** recorded, not fixed. The original's standings after
   each pair but the last start a frame later, and native counts some later pairs a frame early.
   The listing shows no branch that places either, so they are taken to be the frame's work
   running past the frame, like R-0095's table-print overrun. R-0098's Not covered now says this
   explains most of the odd leagues' differing menu pictures, not the podium. It suggests the
   follow-up PODIUM-RETURN-TIMING, since both need the frame's cycle count.
3. **`$77:0859` and the voices past their class table:** fixed in `f409ce5`. Pack v40 adds
   `physics.reward.opponent-voice-classes`, and native increments the same bytes the original
   does (R-0098 finding 17).
4. **R-0098 corrections:** made in `14c8e8f`. Six races load a frame off: four of the
   author's and two of the reviewer's. 2P's NEON scenery 8 is from the listing only. The
   organic-three offset is two frames.
- **Advisories, all fixed:**
  - the league's back paths slide by `$00AC` (`e333865`, finding 18): `backs` has 3,797 of
    3,800 pictures equal, against main's 3,800 differing frames;
  - the readability items (`c472276`), and HUNTER's reversal read once for both pads (`b2dd3b4`);
  - found on the way, a stunt event reads port 2 as the second pad (`ce253a3`, finding 19).
- **A new limit found by the fixed checks.** In `five` window 1, ANTI-UNI's computer race on
  track 41 diverges.
  - A focused worker traced it with an instruction trace and scanline timing (main
    `local/evidence/league-tours/contact/`). HDMA channel 4's BG1VOFS write lands between the
    two `$211B` stores of the landing matrix's first multiply (`$81:B67C`). BG1's scroll and
    mode 7 share the write latch, so the original multiplies with a corrupted M7A.
  - In `odd-tours` 30,250 this gives -154 against native's -242 for one update, which heals.
    In `five` it gives 121,489 (scanline 68): 238 against 212. That race then differs, and
    from 127,125 so do the league's fields.
  - It is the same hardware-timing class as R-0095's DOWNER mode-7 latch race, so it is
    recorded in R-0098's Not covered. No native change was made (`0ec076d`). Predicting it
    needs the race update's cycle count.

**Checks at `b6c00e1`:**
- app-debug, lab-debug and lab-release build; ctest passes 41 of 41 on both debug presets.
- clang-tidy and clang-format report 0 warnings, and `native-symbols --check` passes.
- R-0073's league frozen gate passes.
- The front-end sweep has 149 of 179 equal to main, the differing ones all `league/`.
- Every league race in `tours-a`, `tours-b`, `odd-tours` and `five` is equal on every update.
  The computer races are equal except `odd-tours` 30,250 (one update) and `five` window 1
  (the latch race).
- `tour-cues` differs on 4 of 19,339 lines, as before.

## Gates - head `246cb3e` (main `local/evidence/league-tours/gates-246cb3e.out`)

These are MENU-INPUT's gates plus this task's `checks.sh`, run against main `ceb50cf`'s binaries
`base-ceb50cf/` (pack v40 for the candidate, v39 for main). They ran from 22:20 to 01:57 UTC on
15-16 October 2026, from the task worktree, as `league_frozen.py` requires.
- **Builds and suites.** Four presets build, and ctest passes 41 of 41 on each. Both synthetic
  suites pass. The v1 contracts, hidden app runs and the front end pass. Fuzz is as on main (40
  aborts, a recorded non-pass).
- **Differential gates.** The eleven differential gates pass with main's row digests. The race
  sweep has 432 runs and 0 differences. The front-end sweep has 149 of 179 equal to main, the
  rest all `league/`.
- **Earlier tasks.** Every earlier comparison and schedule is as at `876aa89`, which equals MENU-INPUT's
  `84036e7` run except two improvements: `zzap-long`'s result pictures (800 of 800) and
  MENU-INPUT's `awards-press` (700 of 700). The league frozen pictures pass.
- **This task's captures.** They give the results in "Review round 1". `tour-cues` differs on
  4 of 19,339 lines.
- **Static checks.** 577 tooling tests pass, no function is over 80 lines, and the native
  symbols are current.
- **Hosted CI.** On `246cb3e`, Linux GCC failed on a sign conversion in `cartridge_ram.cpp`,
  which the reviewer found. It is fixed in `9cf5541`, with app-debug ctest 41 of 41, and CI is
  green on all three checks.

## Review round 2 (returned for Linux CI only)

- [Review](https://github.com/malmazuke/unirally-reconstruction/pull/81#pullrequestreview-5542258501)
  of `246cb3e`. The verdict was return only because the Linux build failed; every behaviour
  reproduced, and a reply naming the fix commit suffices.
  - The bonus fix holds on `five` and on a new four-member league `four`, whose stunt event has
    wipeouts: 8 races and 33,772 rows equal, league fields equal over 2,063 frames, where main
    diverges from 42,905.
  - `$0859` is equal, and `backs` has 3,797 of 3,800 pictures equal.
  - The two recorded limits are judged honest and acceptable.
- **Fixed in `9cf5541`:** the GCC sign conversion. Clang does not flag this case, so hosted CI
  is the check.
- **Advisories, applied in R-0098 in the closing commit:**
  - the pairs a frame apart in every observed event (15 of 15, `four` included), with the
    follow-up free to model it as one fixed frame;
  - who writes `$0827`;
  - the one-player overrun as read from the listing only.

## Handoff (closeout)

- **Commits.** The claim `3e0b69b`; the worker's `45a6582` to `876aa89`; round 1 `b8a1131` to
  `b6c00e1`, `14c8e8f`, `0ec076d` and `246cb3e`; round 2 `9cf5541`; then this closing record.
- **Cleanup by the closing session.**
  - It moves the worktree's `artifacts/league-tours/` (gate logs) to main
    `artifacts/league-tours-integration/` with `closeout.json`.
  - It deletes both worktrees' build output, removes `.worktrees/league-tours` and
    `.worktrees/league-tours-review` and the local branch.
  - Captures stay in main `local/evidence/league-tours/` and `league-tours-review/`, the
    evidence of a merged task, under D-0011.
- **Queued follow-ups.**
  - [PODIUM-RETURN-TIMING](PODIUM-RETURN-TIMING.md): the podium's way back, the pairs a frame
    apart, and R-0098's other frame-overrun effects.
  - [PODIUM-AUDIO](PODIUM-AUDIO.md): the podium's own music.
  - SPLIT-HUNTER-PICTURES gains three ready captures. The track-41 latch race stays a recorded
    hardware-timing limit, as R-0095's DOWNER does.
