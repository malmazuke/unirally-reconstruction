# CLASSIC-PRESENTATION-UNIFICATION - independent review

- Candidate: `f734b4ec5b34920eec14b6d226e5f8f05672b4c8` on `task/classic-presentation-unification`
  (four code/record commits over `main` at `ed504fb`).
- Reviewer checkout: `.worktrees/unification-review`, branch
  `review/classic-presentation-unification`, checked out at the candidate.
  Nothing in the task worktree or on `main` was written.
- Toolchain: isolated `cmake 3.31.10` / `ninja 1.13.2`, AppleClang 17.0.0,
  macOS arm64. Both `lab-debug` and `app-debug` rebuilt from scratch at the
  candidate (`lab-build-info.json`: commit `f734b4e`, `dirty: false`).
  The `ed504fb` "before" binaries were built by me from `git archive ed504fb`
  in a scratch tree, not taken from the task worktree.
- Verdict: **return**. One reproducible regression (finding 1) plus four
  should-fix items. Everything the record claims as a measurement reproduced
  exactly; the regression is in a path none of the record's measurements
  exercise.

---

## 1. What I reproduced, and with what result

All commands were run from `.worktrees/unification-review` at `f734b4e` with a
clean tracked tree (`git status --short` empty; the differential gates recorded
`source_diff_sha256 = e3b0c44...` , the empty-tree digest).

### Builds and suites

| Command | Result |
| --- | --- |
| `PATH=...ninja...:$PATH python3 tools/project.py build --preset lab-debug` | passed |
| `PATH=...ninja...:$PATH python3 tools/project.py build --preset app-debug` | passed |
| `ctest --test-dir build/lab-debug` | **23/23 passed** |
| `python3 tools/project.py test --suite synthetic --preset lab-debug` | **passed, 411 checks, 0 failed** |
| `PYTHONPATH=tools python3 -m unittest discover -s tests/tooling -t tests/tooling` | **385 tests, OK** |
| `PYTHONPATH=tools python3 -m unittest tests.tooling.test_frontend` | **14/14, OK** |

The brief's `-t .` form fails on Python 3.14 (`tests/tooling` is not a
package); `-t tests/tooling` and the project runner both work. A fresh
checkout also needs `artifacts/` to exist before the tooling suite runs
(two tests create temporary directories under it and error otherwise).

### 1.1 Correctness of the merge (review item 1)

I read `render_classic_race` against `git show ed504fb:src/core/presentation.cpp`
(`render_zoom_zoo`) and `git show ed504fb:src/app/frontend.cpp`
(`render_dragster_race` / `dragster_presentation_state` /
`race_picture_brightness`). The merge is faithful in structure: the lap HUD
generalises correctly (`classic_hud_lap(r, 3) == zoom_zoo_hud_lap(r)` for
`r = 0..4`), `classic_finish_view` is `dragster_presentation_state`'s finish
block verbatim, the palette-cycle and window-index freezes were factored into
`race_vblank_frame` / `race_palette_phase` without changing the legacy
`apply_dragster_palette_cycle` / `dragster_window_table_index` results (the new
tests assert equality with the legacy functions frame by frame, and I mutated
that shared frame-rewind - see 1.7 - and watched the suite fail).

**Part C reproduced in full** with my own before/after binaries
(`artifacts/review/stage_c.py`, adapted from the record's `stage_c.py` only in
its `ROOT`/`BEFORE`/`PACK`/`OUT`/fixture paths):

| scene | frame | new vs before | new vs original | before vs original |
| --- | --- | --- | --- | --- |
| initialization | 1376 | 0 | 0 | 0 |
| countdown | 1450 | 0 | 13,149 | 13,149 |
| start | 1649 | 0 | 12,944 | 12,944 |
| reversal | 1700 | 0 | 3,830 | 3,830 |
| steep_contact | 2501 | 0 | 3,379 | 3,379 |
| lap_one | 3208 | 0 | 3,359 | 3,359 |
| lap_two | 4840 | 0 | 3,343 | 3,343 |
| finish | 6484 | 0 | 3,359 | 3,359 |
| player_win | 7000 | 0 | 57,344 | 57,344 |
| player_loss | 7000 | 0 | 57,344 | 57,344 |

Ten of ten pixel-identical to the `ed504fb` runner, and identical against the
originals. This matches the record exactly.

Because the ten scenes leave the fade-in window almost untested (only 1376,
where brightness is 0), I added a **wider ZOOM ZOO sweep**: frames 1377-1419
plus 1500, 1600, 2000, 3000, 5000, 6000, 6700 through `--timeline`, new runner
against the `ed504fb` runner. **50 frames, worst mismatch 0.** That is a
stronger no-regression result than part C alone, and it is what makes finding 1
below specific to the single-state mode rather than general.

### 1.2 DRAGSTER on the unified renderer (review item 2)

**Part A reproduced** (shared engine driven by the frozen contract race's own
continuous-Right inputs from end-1328, drawn by
`classic_race_presentation_runner PACK --timeline ROWS FRAME OUT.ppm`, scored on
each case's rectangle against the fixture PNGs; the fixtures I used are
`.worktrees/dragster-window-effects/.../fixtures/m3-02-integration-fixtures`,
whose state and PNG SHA-256 values I first checked against
`tests/manifests/presentation/classic-crawler-dragster-v1.json` - all match):

| frame | rect pixels | v1 renderer | unified | threshold |
| --- | --- | --- | --- | --- |
| 1600 | 26,656 | 36 | 36 | 533 |
| 2000 | 50,176 | 697 | **358** | 1,003 |
| 2400 | 50,176 | 279 | 279 | 1,003 |
| 3213 | 50,176 | 445 | **322** | 1,505 |
| 3453 | 50,176 | 653 | 777 | 1,505 |
| 3678 | 57,344 | 962 | 962 | 8,601 |
| 3679 | 57,344 | 961 | 961 | 8,601 |

Identical to the record, every frame inside its threshold.

**Part B reproduced**, all 183 frames with originals, rectangle
(0, 28, 256, 196): previous 757,274 against unified **680,807** over the 132
frames the window-effects measurement scored; 114 better, 14 unchanged, 4 worse
(3452/3453 by 124, 3600/3677 by 3). Identical to the record.

I looked at the side-by-side pictures. `B-3300-side.png` (winner banner) is the
clearest: the unified renderer draws the original's banner shape, colour and
the black rider silhouette in the right pose, where the previous renderer holds
the white-helmet M3 atlas pose. `A-3453-side.png` shows the declared
difference honestly - the scene, track, banner and riders agree, and the top
authored HUD band replaces the original's in-scene red FINISH/time/WINNER text.

**The BG1/BG2 HDMA relation checked independently.** From the pack's
`physics.track.dragster.data` header the DRAGSTER origin is (832, 544). Taking
the *previous* update's shared camera from my own native rows and comparing with
the original's captured scroll registers in the v1 contract manifest:

| frame | prev camera x/y | BG1 = cam-832 | manifest BG1 | BG2 = BG1>>1 | manifest BG2 |
| --- | --- | --- | --- | --- | --- |
| 1600 | 1964 / 747 | 1132 | 1132 | 566 | 566 |
| 2000 | 7970 / 752 | 7138 | 7138 | 3569 | 3569 |
| 2400 | 13635 / 752 | 12803 | 12803 | 6401 | 6401 |
| 3213 | 25199 / 752 | 24367 | 24367 | 12183 | 12183 |
| 3453 | 25266 / 752 | 24434 | 24434 | 12217 | 12217 |

Vertically the same: 747-544 = 203 and 752-544 = 208, halved to 101 and 104,
both matching the manifest. The claim holds on every captured racing frame, not
only on the single frame the record quotes.

### 1.3 The opponent-won banner (review item 3)

`verify_banner.py` reproduced against `lose-a`'s `$80:868E` reads:

```
native finish frames [3318, 3214] loading start 3559 totals 3566 3358
shift 0: tracked agrees on 2226, disagrees on 0, first disagreement None
shift 1: tracked agrees on 1978, disagrees on 248, first disagreement (1355, 6, 0)
banner frames 345 tracked disagreements 0 alone disagreements 102
```

**2,226 of 2,226 at shift 0**, exactly as claimed, including the whole banner
past the legacy 120-update bound and the loading frames. Shift 1 reproduces the
248 figure the record's "Mistakes" section records as the wrong alignment.

**Independent cases.** `artifacts/review/indep_banner.py` takes the *original's*
own serialized rows for `random-1-a`, `reversal-a` and `primary-a`, finds the
frame each rider's `finished` flag turns on, and drives
`classic_race_presentation_runner --window-index` over those rows:

| case | player finish | opponent finish | total[0]-total[1] | 2(fa-fb)+(fa&1)-(fb&1) | holds |
| --- | --- | --- | --- | --- | --- |
| random-1-a | 3636 | 3214 | 844 | 844 | yes |
| reversal-a | 4292 | 3325 | 1933 | 1933 | yes (odd `fb`) |
| primary-a | 3211 | 3214 | -5 | -5 | yes |

So the arithmetic identity is confirmed on a case with unequal parities, which
`lose-a` (3318/3214, both even) does not exercise. Over the 120-frame banner
band the tracked history selects a member on **120 of 120 frames in all three
cases**, and on `primary-a` (player won) the history-free derivation agrees with
the tracked one on all 120. The loading-frame handling checks out too: the
derivation is `frame - result_updates` while the vblank freeze is
`frame - (result_updates - 1)`, and the `lose-a` run has zero disagreements
across the loading frames, which is what distinguishes the two.

The derivation's real coverage is narrower than the record reads, though - see
finding 4.

### 1.4 The legacy v1 path (review item 4)

```
python3 tools/project.py native presentation-check \
  --manifest tests/manifests/presentation/classic-crawler-dragster-v1.json \
  --fixtures artifacts/review/fixtures/winner \
  --content-pack <main>/local/classic-crawler-dragster.pack --preset lab-debug ...
```

- winner: **36 / 697 / 279 / 445 / 653 / 962 / 961**, status passed.
- loser (`classic-crawler-dragster-loser-v1.json`): **1,073**, status passed.

Byte-for-byte the accepted figures. The v1 path is untouched.

(`presentation-check` only accepts fixtures under the checkout's own `local/` or
`artifacts/`, so I copied the private fixtures into ignored
`artifacts/review/fixtures/`.)

### 1.5 Engine content naming (review item 5)

`content pack-inspect --pack local/classic-crawler-two-tracks-v8.pack --rules
tests/manifests/content/classic-crawler-two-tracks-pack.json`: passed, 56
entries, profile `classic.pal.crawler.two-tracks.v8`. Comparing the eight pairs
by payload bytes read straight out of the pack file:

| bytes | `physics.*` | `zoom.*` alias | identical |
| --- | --- | --- | --- |
| 32,768 | `physics.rider.collision-poses` | `zoom.collision-poses` | yes |
| 17,249 | `physics.rider.collision-templates` | `zoom.collision-templates` | yes |
| 512 | `physics.rider.displacement-table` | `zoom.displacement-table` | yes |
| 128 | `physics.rider.pose-slopes` | `zoom.pose-slopes` | yes |
| 80 | `physics.track.progress-transitions` | `zoom.progress-transitions` | yes |
| 64 | `physics.rider.idle-pose-table` | `zoom.idle-pose-table` | yes |
| 18 | `physics.speed.decrements` | `zoom.speed-decrements` | yes |
| 9 | `physics.speed.masks` | `zoom.speed-masks` | yes |

50,828 duplicated bytes; the rules manifest gives both members of every pair the
same source offset and length. Nothing reads the aliases any more: the eight
`zoom.*` names now appear only in the pack registry inventory in
`content_pack.cpp` and in the rules manifest.

**Differential gates run by me at the candidate** with my own `app-debug`
`zoom_zoo_runner` and a clean tracked tree:

```
m4-16-primary          rc=0   (zoom_zoo_playable compare, boundary-a/b, v11 freeze)
dragster-primary       rc=0
dragster-random-1      rc=0
dragster-reversal      rc=0
```

`m4-16-primary.json` records `status: passed`, race domain [1376, 6724], 724
projected bytes plus the result/restart domains; the three DRAGSTER reports
record `acceptance: true`.

I also re-ran the shared-engine restore path the naming change could have
broken: a mid-race ZOOM ZOO state (frame 1876 of `boundary-a-native.txt`) fed to
`zoom_zoo_runner --seed ... --content-pack ...` still runs, so the new
"a content pack binds the complete-race content" guard does not block the
M4-13/M4-14 opponent probes, which restore complete-race seeds.

### 1.6 The launcher (review item 6)

Real runs, not fixtures:

- `build/app-debug/src/app/unirally.app/Contents/MacOS/unirally --supported-profiles`
  prints `classic.pal.crawler.dragster.v1` and
  `classic.pal.crawler.two-tracks.v8`, rc 0.
- Typed v1 pack: `frontend run --track dragster --pack <main>/local/classic-crawler-dragster.pack`
  -> `status=failed`, and the check names both profiles and a remedy:
  "existing pack is invalid and was not replaced: Classic pack extraction-rules
  identity is incompatible; it records profile `classic.pal.crawler.dragster.v1`
  and this launch needs `classic.pal.crawler.two-tracks.v8`; move ... aside, or
  pass `--rom PATH` with `--replace-pack` to rebuild it". The 18 September
  playtest defect (silent substitution reported as a pass) is closed.
- No `--pack`, both tracks: selects
  `local/classic-crawler-two-tracks-v8.pack` "by its profile", reports the
  supported profile, launches, `status=passed`.
- `--rom` over a v7 pack without `--replace-pack`: refused with the same remedy,
  the v7 file untouched.
- Hidden 4,000-update runs, `--fixed-controller-mask 128`, both tracks:
  `rider-pose fallback frames: 0` on each; DRAGSTER reaches
  `race phase 3; outcome 1`, `result updates 226`, `totals 3357/3358`. The
  user's playtest had 21 of 21 fallback frames, so the renderer gap is closed.
- First-launch and report-alias behaviour: `test_frontend` covers both and
  passes 14/14; I saw the "refusing to write --report onto --pack path ..."
  refusal and the `[skipped] supported_profile` path for an executable that
  reports nothing. No regression found there.

`--replace-pack` itself is finding 5.

### 1.7 Readability and evidence (review item 7)

- Names and units read well: `classic_race_presentation_content`,
  `classic_window_table_index`, `classic_opponent_finish_frame`,
  `ClassicRaceHistory`. Comments cite the addresses they reconstruct
  (`$82:D382-D496`, `$83:E59C`, `$83:E728`, `$80:883F`, `$81:A304-A51B`,
  `$81:A445`/`$81:A4C1`, `$80:868E`) and the research records.
- **Mutation testing** (in a scratch copy of the candidate, never in the review
  checkout). Three separate mutations of new helpers, each rebuilt and run
  through `ctest --test-dir build/lab-debug`:
  1. drop the frame-parity term in `classic_opponent_finish_frame`
     (`twice_gap = difference`) -> test 19 `presentation_gather_mapping_and_determinism` **fails**.
  2. remove the result-loading rewind in `race_vblank_frame`
     (`return frame;`) -> test 19 **fails**.
  3. make `classic_hud_lap` ignore its `laps` argument (old 3-lap form)
     -> test 19 **fails**.
  The new tests genuinely exercise the new code.
- No weakened comparisons: the v1 contract thresholds, rectangles and manifests
  are untouched, and the deleted `frontend_contract_tests` block for
  `dragster_presentation_state` reappears verbatim as `classic_finish_view`
  coverage in `presentation_tests.cpp`. The deleted `presentation_position` /
  `race_picture_brightness` assertions belong to functions the task removes.
- No accidental content commits: 26 files, all source/tests/tools/docs, no
  binary blobs, nothing over 64 KiB but `presentation.cpp` itself (78 KiB of
  source).
- Hosted-CI risk: I could not reproduce a GCC build - this machine has only
  AppleClang (`/usr/bin/g++` is clang). `-Wall -Wextra -Wpedantic -Wconversion
  -Wsign-conversion -Wshadow -Werror` are on for every preset and all five
  preset builds pass locally. Reading the new code for GCC-only traps I found
  none: `coarse_columns`/`position_mask` are `uint16_t` widened into `int`
  (`columns * 64` peaks at 65,536, inside `int`), the `visible_left/right`
  narrowings are explicit `static_cast`, `for (const auto profile :
  supported_pack_profiles())` iterates `string_view` by value, and
  `ClassicRaceHistoryTracker::on_screen()` returns a small aggregate by value.
  The `TrackGeometry` braced initialisers in `rider_presentation_tests.cpp` use
  constant expressions that fit their targets, so they are not narrowing errors.
  **This is a read, not a reproduction** - I am recording GCC as unverified
  locally rather than as a pass.

---

## 2. Findings

### 2.1 BLOCKING - the single-state renderer path draws the ZOOM ZOO fade one step too bright

`render_classic_race` (`src/core/presentation.cpp`) replaced the old frame
formula with:

```cpp
const bool previous_is_prior = previous_update && previous_update->movement.frame <= state.movement.frame;
const unsigned prior_fade = std::min(30U, previous_is_prior ? unsigned(previous_update->fade_level)
                                          : (state.fade_level ? unsigned(state.fade_level) - 1U : 0U));
```

The `else` branch is correct: without a previous update the fade must come from
`fade_level - 1`, because the NMI writes INIDISP from the *preceding* update's
`$0FF1`, as the comment three lines above says. But the condition is `<=`, and
`classic_race_presentation_runner PACK STATE OUT.ppm` sets
`previous = state` when no `PREVIOUS_STATE` is given, so `previous_is_prior` is
true, the current update's `fade_level` is used, and the correct branch is
unreachable from that mode.

`fade_level` is exactly `frame - 1376` for ZOOM ZOO, so the accepted renderer's
`min(30, frame - 1377)` is one lower. Wherever the resulting brightness differs
and is below 15, the picture differs:

| frame | fade_level | accepted prior_fade | candidate prior_fade | accepted brightness | candidate brightness |
| --- | --- | --- | --- | --- | --- |
| 1391 | 15 | 14 | 15 | 0 | 0 |
| 1392 | 16 | 15 | 16 | 0 | **1** |
| 1400 | 24 | 23 | 24 | 8 | **9** |
| 1406 | 30 | 29 | 30 | 14 | **15** |
| 1407 | 30 | 30 | 30 | 15 | 15 |

Reproduction (from the review checkout, with `$BEFORE` the `ed504fb`
`zoom_zoo_presentation_runner`, `$TL` the M4-16 `boundary-a-native.txt`):

```sh
# write frame N's row from $TL to s-N.bin, then
build/lab-debug/src/core/classic_race_presentation_runner local/classic-crawler-two-tracks-v8.pack s-N.bin new-N.ppm
$BEFORE                                                   local/classic-crawler-two-tracks-v8.pack s-N.bin old-N.ppm
# ppu.compare(new, old, 256, (0,0,256,224))
```

Full-frame mismatches, new against the accepted renderer, ZOOM ZOO frames
1377-1414:

```
1392: 40047   1393: 54243   1394: 54333   1395: 50328   1396: 38768
1397: 51276   1398: 37323   1399: 51500   1400: 37323   1401: 51276
1402: 38768   1403: 50328   1404: 54345   1405: 54168   1406: 40221
```

fifteen consecutive frames, up to 54,345 of 57,344 pixels. Every other frame in
1377-1419 is identical, and the `--timeline` mode (which supplies a genuinely
earlier state) is identical everywhere I checked.

Why the candidate's evidence missed it: part C is the only ZOOM ZOO measurement,
and only its frame-1376 scene uses the single-state mode, where the fade is 0
either way.

Why it matters even though the app is unaffected (the app always passes
`zoom_hud_state`, a real earlier update): this runner is an evidence-producing
tool. A future scene rendered through the single-state mode inside the fade
window is silently a fade step wrong, and the acceptance row for this task is
"ZOOM ZOO visual contract - unchanged or closer to the original".

Suggested fix, and a caution: derive the fade from a stricter predicate -
`previous_update && previous_update->movement.frame < state.movement.frame` -
so the existing `fade_level - 1` branch takes over when `previous == state`.
Do **not** change `previous_is_prior` itself for the BG scroll: the accepted
renderer's `previous_race` predicate was also true for `previous == state`, so
tightening the shared condition would move the background from the state's own
camera to `camera - velocity` and trade this regression for a different one.
A regression test is cheap: render 1400 with and without a `PREVIOUS_STATE` and
require the pictures to agree.

### 2.2 SHOULD FIX - R-0040's "Not established" section has three duplicated bullets

`docs/research/R-0040-dragster-window-effects.md` now lists the `$1229` bullet
and the `$0FF1` bullet **twice, verbatim**, and the ZOOM ZOO bullet twice with
different closing sentences ("The unified renderer binds no window family for
ZOOM ZOO until that is measured." / "it is a follow-up, not part of this
task."). The commit inserted a new copy of the surviving bullets above the old
ones instead of replacing only the resolved bullet. Reproduce with
`sed -n '/^## Not established/,/^## /p' docs/research/R-0040-dragster-window-effects.md`.

### 2.3 SHOULD FIX - the record's part-B prose contradicts its own report

The Measurements section says:

> A further 51 frames with originals but no previous render (1330-1339
> initialization, 1420-1436 and 1510-1532 countdown) score 0 to 490 each.

Both the candidate's `stage_c/report.json` and mine contain a 52nd-listed frame
in that set: **frame 3700, `unified_rect` 23,194**. The enumeration omits it and
the stated range excludes it.

The difference itself is not a regression - at 3700 the original's result screen
has animated OBJ decorations (the `1P` marker, the arrow and the per-row
trophies) and a further-advanced background phase that the recovered mode-0
result screen does not draw; `artifacts/review/stage_c/B-3700-side.png` shows
it plainly, and the frozen contract frames 3678/3679 still score 962/961. The
defect is the record, which currently reads as if the whole unscored set were
inside 490. State the real figure and name the cause, or exclude 3700 from that
sentence explicitly.

### 2.4 SHOULD FIX - the finish-time derivation's coverage is narrower than the record reads

Both the record and R-0040 present `classic_opponent_finish_frame` as the
history-free recovery of the opponent-won banner, evidenced only on `lose-a`
(opponent 3214, player 3318 - 104 frames apart). Measured on the other two
opponent-won original races, over the 120-frame banner band:

| case | opponent finish | player finish | gap | banner frames with a member, tracked | derived |
| --- | --- | --- | --- | --- | --- |
| lose-a | 3214 | 3318 | 104 | 120 | partial (102 of the first 120 frames select nothing) |
| random-1-a | 3214 | 3636 | 422 | 120 | **0** |
| reversal-a | 3325 | 4292 | 967 | 120 | **0** |

The derivation needs `race.riders[0].finished`, so it can only ever contribute
while the banner is still running when the player finishes - within about 121
frames of the opponent. `lose-a` is the one captured race where that is true.
The tracked history is what recovers the banner over its full length in every
case, which is exactly what the picture above shows.

Nothing here is wrong in the code; the claim just needs its bound written down,
e.g. "a restored state recovers the origin only when the player finished within
the banner's 120 updates; on `random-1-a` and `reversal-a` it selects nothing
and the banner needs the tracked history". Otherwise a later reader will take
the derivation for a general fallback.

### 2.5 SHOULD FIX - `--replace-pack` renames the user's pack, then can die with an unhandled traceback

`cmd_run` in `tools/unirally_lab/frontend/commands.py` renames the incompatible
pack aside inside the `validate_pack` failure handler, and only afterwards calls
`packmod.build_pack(rom, rules, rules_sha)`. That call is guarded by
`except ValueError` / `except OSError`, but `build_pack` shells out to
`tools.unirally_lab.content.landing_matrix` with `check=True`, so that
sub-extractor's failures arrive as `subprocess.CalledProcessError`, which is
neither. Observed here:

```sh
python3 tools/project.py frontend run --track dragster \
  --pack artifacts/review/incompatible.pack --rom "<ROM>" --replace-pack \
  --preset app-debug --updates 20 --hidden
# -> Traceback ... subprocess.CalledProcessError: Command '[... landing_matrix ...]'
#    returned non-zero exit status 1.
# and: artifacts/review/incompatible.pack.stale-20260918T053314Z now exists,
#      artifacts/review/incompatible.pack does not, and no report was written.
```

The underlying extractor failure is environmental in an isolated checkout
(`local/emulators` is not copied, so `landing_matrix` reports "pre-race
extractor core identity differs") and is **not** a defect of this task. The
defects are that (a) a non-`ValueError`/`OSError` extraction failure escapes as
a traceback with no report and no check, and (b) with `--replace-pack` that
happens *after* the destructive rename, so the user is left with their pack
moved and nothing on screen saying where it went. The record's launcher
evidence exercises the refusal path but records no successful real-ROM
`--replace-pack` run, so this was not seen. Catching `subprocess.SubprocessError`
alongside the others - and, better, extracting to a temporary path and renaming
the old file aside only once the new pack validates - closes both.

### 2.6 ADVISORY - `dragster_presentation_content` has no caller

Moved from `src/app/frontend.cpp` into `src/core/presentation.cpp` (declared at
`presentation.hpp:208`) with the comment "Only the frozen v1 contracts and their
runners draw through this content", but nothing in `src/`, `tests/` or `tools/`
calls it: `presentation_runner.cpp:63`, `live_presentation_runner.cpp:75`,
`frontend_contract_tests.cpp:236` and `presentation_tests.cpp:238` all build
`PresentationContent` by aggregate initialisation. Either wire the v1 runners
through it or drop it; a public function whose comment names callers that do not
exist is the kind of thing D-0003 is about.

### 2.7 ADVISORY - `zoom_zoo_runner`'s pack mode builds a discarded content aggregate

With `--content-pack`, `load()` returns `{}` for all twenty entry names, the
twenty results are assembled into `zoom_zoo_data`, and the ternary below then
discards it in favour of the pack accessors. It is harmless and the behaviour is
right, but a reader has to get to the bottom of the function to learn that the
twenty `load(...)` lines above do nothing. Hoisting the pack branch above them
would say it once.

### 2.8 ADVISORY - the `--rom` remedy names a flag the caller already passed

When `--rom` is given without `--replace-pack`, the refusal still says "pass
`--rom PATH` with `--replace-pack` to rebuild it". "add `--replace-pack`" would
be the actionable half.

### 2.9 ADVISORY - the candidate's own final gate log records the tooling suite as failed

`artifacts/unification-baseline/final-gates-run.log` at `f734b4e` shows five
presets 23/23, all four differential gates rc=0, fuzz rc=0 and both hidden runs
at 0 fallback frames - but `== tooling ==` reads `tooling FAILED` with the
Python 3.14 `discover -s tests/tooling -t .` ImportError. That is an invocation
defect in `final-gates.sh`, not a code defect, and I confirmed the suite passes
(385 tests / 411 project-runner checks) with a working invocation. The gate
table in the record should not be left reading as though the tooling suite
passed at the final candidate; say which invocation was used.

Related, minor: the record's gate table attributes the M4-16 and DRAGSTER
differential gates and the 20-command historical matrix to `cfb539d`, the second
of four commits. I re-ran the four differential gates at `f734b4e` itself and
they pass, so the results hold at the candidate - but the table should say so
rather than citing an intermediate commit.

---

## 3. Verdict

**Return.** Finding 2.1 is a specific, reproducible regression against the
accepted ZOOM ZOO renderer, introduced by this change, on fifteen consecutive
frames, in a mode the candidate's measurements do not cover. The fix is a
one-line predicate plus a two-render regression test, and the caution in 2.1
about not moving the BG-scroll fallback with it matters.

Findings 2.2-2.5 should be fixed with it: two of them are evidence-record
inaccuracies that this task's own artifacts contradict, one narrows an
overbroad claim about the derivation, and one is a destructive-then-crashing
launcher path.

Everything else checks out, and it is worth saying plainly how much: every
figure in the Measurements section reproduced exactly from my own builds of both
sides - the seven contract frames, the 132-frame release-3213 comparison, all
ten ZOOM ZOO scenes pixel-identical, the v1 winner and loser contracts at their
accepted values, 2,226 of 2,226 banner frames against the original's own
pointer, the eight byte-identical aliases, four differential gates at the
candidate, and zero rider-pose fallback frames on both tracks over 4,000
updates. The merge is a real merge and the DRAGSTER renderer is measurably
closer to the original than the one it replaces.

Re-review scope: finding 2.1's frames 1392-1406 identical again in both runner
modes, plus the record and R-0040 corrections. I do not need the other
measurements re-run.
