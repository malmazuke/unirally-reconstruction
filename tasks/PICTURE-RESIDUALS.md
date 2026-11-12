# PICTURE-RESIDUALS - the picture differences the records leave, by system

## Assignment

- Status: **claimed** 11 November 2026 22:51 UTC on main `ce558a3f`, on the user's instruction
  ("Next task", after CPU-CYCLE-TIMING #103). Queued 8 November 2026 by
  [COVERAGE-CENSUS](COVERAGE-CENSUS.md) (R-0109, queue item 7). Claude Code desktop app, Claude
  Opus 5.5 (`claude-opus-5-5`): coordinator and integrator; a worker subagent captures and
  implements, a fresh subagent reviews. Run under
  [D-0013](../docs/decisions/D-0013-task-time-and-disk-budget.md): claim to merge against the
  6-hour target, new evidence under 1 GB. Usage at the claim: 5-hour 1%, weekly 37%.
- Branch `task/picture-residuals` in `.worktrees/picture-residuals`; evidence in main
  `local/evidence/picture-residuals/`.
- Task provider: Anthropic.
- Milestone: M4. Review tier: **2** (presentation).

The records list pictures that still differ from the original after every task: split pictures where
a rider passes under the HUD, track 42's two-view NEON, line 113 under a mosaic block, 41 small sprite
pictures in demo races, 470 pixels in the GO letters' area, NOW PLAYING's arrow and icon animation,
the result icons and three league pictures (R-0046, R-0071, R-0073, R-0083, R-0084, R-0088, R-0098,
R-0103). Outcome: each traced to its cause in the original's PPU state and fixed in native, or
recorded with the reason it cannot be (a hardware effect native does not model), with every
compared picture of the routes involved equal.

## Results

Worker: Claude Opus 5.5 (`claude-opus-5-5`) subagent, 11 November 2026 22:51 to 01:12 UTC (12 November, clock).
[R-0116](../docs/research/R-0116-picture-residuals.md) has the causes, the measurements and the limits.

Each item was measured again on main `ce558a3f` first (main's binaries kept in the evidence's
`base-ce558a3f/`). Four were already equal; three are fixed by four changes in `presentation.cpp`; two
remain and are queued.

| Item | On main | Verdict |
| --- | --- | --- |
| Three league pictures at the frozen 14-pixel limit (R-0073) | 0 pixels | already equal (59 of 59 at 0) |
| NOW PLAYING's arrow and icon animation (R-0071, R-0073, R-0084) | equal | already equal (`organic-cycle-five`, the 2P and VS captures) |
| The result icons and slides (R-0084) | equal | already equal (6,931 pictures of four captures) |
| 470 pixels in the GO letters at update 272 (R-0046) | equal | already equal (222 frames, both runners) |
| 41 small sprite pictures in demo races (R-0088) | 13 of 1,006; 177 of 4,974 at every frame | **fixed**: a one-view demo's HUD and caption are flat ink (`$212D` = 0) |
| Riders under the HUD in split races (R-0083, R-0098) | `zzap` 1; league 34 (11 windows) | **fixed**: `$82:B07E-B153`'s lower-view edges; the PPU's object priority |
| Track 42's two-view NEON (R-0098, R-0103) | 940 league pictures | **fixed**: colours 0-95 from the set `$83:9E1A` picks |
| Upper-view object at the left edge across the split (R-0103) | 1 moment, 8 pixels (3 windows) | open: how the original hides objects across the split is not recovered |
| Line 113 under the mosaic (R-0103) | 14 pictures, one row | open: BG1 map streaming for two views (hypothesis unconfirmed) |

Checks (`checks.list`, nine lines; candidate `3cca204a`, main `ce558a3f` with `reverted.out`):

| Check | Compares | Candidate | Main's runner | Time |
| --- | --- | --- | --- | ---: |
| `oneview-demos` | every frame of the cold lap's first two (one-view) demos, 4,974 pictures | PASS, all equal | FAIL, 4,797 equal | 127 s |
| `idle-lap` | `cold-102000`, 1,006 pictures | PASS, all equal | FAIL, 993 | 21 s |
| `zzap` | 6,074 pictures | PASS, all equal | FAIL, 6,073 (6317) | 158 s |
| `league-a` | `tours-a` w0-w8, every race picture, 17,725 | PASS (but 70040, 8 px) | FAIL, 17,076 | 468 s |
| `league-b` | `tours-b` w0-w1, `odd-tours` w0-w1, `five` w0-w1, 19,466 | PASS (but 11910, 95090, 8 px each) | FAIL, 19,138 | 499 s |
| `two-human` | `vs-control`, `vs-restart`, `twop-restart`, `league-restart`, 6,931 | PASS, all equal | PASS (already equal) | 169 s |
| `league-frozen` | LEAGUE's frozen gate, 59 pictures at 0 pixels | PASS | PASS (already 0) | 4 s |
| `go-letters` | 222 frames of race updates 190-300 | PASS, all equal | PASS (already equal) | 6 s |
| `line-113` | `demo-pic`, `vs-4`: only the 14 open line-113 pictures differ | PASS | PASS (open, the same 14) | 213 s |

## Decisions (worker)

- Re-measured before changing anything, with main's own binaries and the existing captures; an item
  equal on main is recorded and pinned by a check, not changed.
- The one-view demo's ink follows the split demo's (SPLIT-HUNTER-PICTURES): the cause is the same
  `$212D` = 0 from the demo's title, so `hud_over_riders` is `state.demo_ai`.
- The lower-view edge rule is modelled as its visible effect (the object's first two tile columns
  dropped, or the object not drawn) rather than as the 16-pixel move and the tile number, because
  native draws the composed object, not tiles: the two are the same picture since the extra columns
  are the blank half of the riders' tiles (R-0103).
- The objects' priority is modelled as the PPU's rule (the front object's pixel and priority) for every
  race, not only two-view ones: it is how the PPU composes any overlap; the race sweep shows what it
  moves.
- NEON's palette set is taken from a scenery whose set it is (`scenery.00`, `scenery.08`) rather than a
  new pack entry: the pack's scenery palettes already hold the three sets in colours 0-95 (checked:
  equal within each set), and the two stray ROM bytes are cited in the comment.
- The two open items were left after a bounded look (budget): the split's object hiding needs a
  per-line register record and line 113 a VRAM read at the picture's own time with wobble's per-line
  scroll; both queued as PICTURE-RESIDUALS-REMAINDER (tier 2).
- Checks pass on main for the items main already had equal (`two-human`, `league-frozen`,
  `go-letters`) and for the open `line-113` guard; they pin those pictures for the gates.

## Evidence

Main `local/evidence/picture-residuals/` (40 MB):
- `base-ce558a3f/`: main's runners (the re-measurement and the revert); `m0/`: main's `compare.py` and
  `menu_compare.py` runs on SPLIT-HUNTER-PICTURES' captures and the fifteen league windows, before any
  change; `make_expect.py` takes the league windows' race intervals from them.
- `oneview-demos/`: a probe of the cold lap's first two demos at every frame (4,976 PNGs, 35 MB,
  `cold.json`, `probe.json`); `states/` (with `README.txt`): five Strict states of the original
  (two-view NEON at `tours-a` 81100, `demo-pic` 2611/2612, `odd-tours` 11909/11910);
  `ppustate.py` (VRAM, CGRAM and BG registers from a state), `stateppu.py` (pack entries in a state),
  `neon_palette.py` and `neon_palette.out` (finding 5: the CGRAM against every scenery palette, the
  ROM bytes the long index reaches), `edge.py` (lower-view edge cases in a capture), `crop.py`.
- Checks: `check.sh`, `checks.list`, `verify.py`, `expect.json`, `pictures.py`, `race_pictures.py`,
  `fill_league.py`, `run_checks.sh`, `go-letters/` (the two TRACK-BREADTH routes' manifests);
  `checks-3cca204a.out` (candidate, G under the main checkout's path, which has a space) with
  `checks-3cca204a-league-reverify.out`, `reverted.out` (main's runner, `RUNNER=`).
- `checks-3cca204a.out`: all nine run in 1,665 s; seven PASS on the first run, and `league-a` and
  `league-b` ran with placeholder counts (-1) because their windows' race-picture counts were taken from
  this run (`fill_league.py`); their saved outputs then pass `verify.py` with the filled counts
  (`checks-3cca204a-league-reverify.out`), `reverted.out` ran with them, and both were rerun clean on
  `1dc2d759` with the filled counts: PASS, 470 s and 500 s (`checks-1dc2d759-league.out`).
- Main's runner (`reverted.out`, 1,666 s): `oneview-demos`, `idle-lap`, `zzap`, `league-a` and `league-b` FAIL (the numbers in the table above); `two-human`, `league-frozen`, `go-letters` and `line-113` PASS, as main already had those pictures (or, for line 113, the same 14).
- What moves (main's runner against the candidate's):
  - The gate corpus replay (`replay-3cca204a.json`, `python3 -m tools.unirally_lab.gates.replay`, 536
    cases, 611 s): 492 equal, 44 differ, in pictures only (no state row, timeline, cue, text or record
    stream differs). The steps: LEAGUE-TOURS' windows (`lt-*`, and their PODIUM-RETURN-TIMING copies
    `checks-checks-22`, `-29` to `-44`, with `four-w0`), `idle-demo`, `two-pad` (`zzap`) and
    `checks4` (TWO-HUMAN-STUNTS' `w1-2p-hill`).
  - Each of those, every retained picture on both sides against the original (`moves.py`, `moves-3cca204a/`):
    the fifteen league windows and `four-w0` (LEAGUE-TOURS' review, PODIUM-RETURN-TIMING's
    `checks-checks-22`/`-44`; 88,008 retained pictures): 981 change, every one from main's 1-1,190 differing pixels to
    0 (none away from the original): `tours-a` w0 7, w1 2,
    w2 315, w5 4, w6 313, w7 6, w8 1; `tours-b` w0 3, w1 3; `odd-tours` w0 313, w1 3; `five` w0 3, w1 1;
    `four-w0` 7 (9500, 23230, 23260, 24200, 31340, 31360, 47120: 2-6 pixels each to 0); w3 and w4 none.
  - `idle-demo` (`cold-102000`): 13 pictures change, each from 1-68 differing pixels to 0. `two-pad`'s
    `zzap`: 6317 from 22 to 0. `checks4`'s `w1-2p-hill`: 2881, 2883, 2887, 2888 from 4 pixels each to 0
    (5,046 to 5,050 of 5,050; a corkscrew in a two-view race).
  - The race sweep (`race-sweep-3cca204a.json`, 432 runs, 2,538 pictures of `classic_race_presentation_runner`):
    0 differences.
  - The v1 presentation contracts (the gates' `v1-contracts` command, `lab-debug`): winner and loser
    `status=passed` (`v1-winner.log`, `v1-loser.log`).
  - Not run: the eleven frozen race gates (rows only; no race state changes) and the app's pictures.
- Builds `lab-release`, `lab-debug`, `app-debug`; `ctest` 41/41 on `lab-debug` and `app-debug`. Linux
  GCC 13.3 (`ubuntu:24.04`, CI's warnings as errors, `gcc.sh`, `gcc.out`): `presentation.cpp` compiles
  plain and with `-fsanitize=address,undefined`. clang-format clean on the changed files. ASan not run
  locally (macOS 27). `regen.sh` (eight inputs, unchanged) regenerated the static map and native
  symbols; `coverage native-symbols --check` passed (1,783 ROM addresses, all cited by a record) and the
  tooling tests pass. No new runner option, so no corpus change.

## Handoff

- Commits on `task/picture-residuals`: `3cca204a` (the native change, `src/core/presentation.cpp`,
  `presentation.hpp`), then the records commit (R-0116, this record, PICTURE-RESIDUALS-REMAINDER and its
  README row, the regenerated static map and native symbols).
- Done: every item re-measured on main and given a verdict (Results); the four changes; checks,
  revert, builds, ctest, GCC, the static map, the corpus replay and the race sweep as in Evidence.
- Not done: review, gates, closeout, STATE.md and NEXT_SESSION.md (the coordinator's).
- Open, queued as [PICTURE-RESIDUALS-REMAINDER](PICTURE-RESIDUALS-REMAINDER.md) (tier 2): objects across
  the split at the left edge (R-0116 finding 6), line 113 under wobble mode's mosaic (finding 7), and
  `organic-cycle-exit`'s second race starting late (seen, not a picture residual).
- For the gates: the corpus replay's 44 differing cases are pictures only, in the steps listed in
  Evidence, each change toward the original. Expect `lt-*`, `checks-checks-22`/`-29` to `-44`,
  `idle-demo`, `two-pad` and `checks4` to report more pictures equal than main; `race-sweep` same as
  main. The v1 contracts pass here; the frozen race gates read rows, which this change does not touch.
