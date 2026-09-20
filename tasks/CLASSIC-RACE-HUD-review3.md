# CLASSIC-RACE-HUD - independent review of `eff7638`, third round

**Verdict: return.** The second review's blocking finding is genuinely fixed and
I confirmed it on consecutive originals I recaptured myself, together with every
other ZOOM ZOO transition that matters: the three lap changes, the player's
finish, the opponent's finish, the loser race where the opponent finishes first,
and the crossing case `ordinary-controls/down-a` 3206-3211 that returned the
task. All five kept-frame sweeps, the synthetic suite, both v1 contracts and the
`gate_identity` citation reproduce on the candidate, and the three mutations the
dispatch asked for each fail the suite.

But the widened predicate is a second fit, not the original's rule, and this
time the defect is on **DRAGSTER**. The implementation holds the clock whenever
either rider's `laps_remaining` changes, on both tracks. On DRAGSTER the
original holds it on exactly one of those updates - the one where the player's
counter reaches zero and the word `finish` is written - and on none of the
others, because `$053F` is set and R-0043's own sentence for that branch is
"the field is left alone". Native draws the tenths digit one picture late on
four accepted DRAGSTER originals I measured, including the one capture that
contains both behaviours 197 frames apart. The previous candidate `428d1d3`,
which compared the field's text, drew all of those frames correctly, so this
round is a regression on the second track as well as a third instance of the
same class of defect.

Reviewer: fresh Claude Opus 5 session, no inherited conversation, isolated
worktree `.worktrees/classic-race-hud-review` detached at `eff7638` (branch
`review/classic-race-hud-3`). Artifacts: `artifacts/classic-race-hud-review3/`
in that worktree (ignored). Date: 20 September 2026.

I read both earlier reports (`8671cfc` on `review/classic-race-hud` and
`603d377` on `review/classic-race-hud-2`), R-0043 and the task record before
starting, and treated every number in them as a claim.

## What I reproduced, with the exact commands

From the review worktree with
`export PATH="$PWD/local/toolchain/ninja-1.13.2-darwin-arm64:$PATH"`. `E` is
`local/evidence` (the symlink into the main checkout), `A` is
`artifacts/classic-race-hud-review3`, `M` is
`$E/m4-16-playable-zoom-zoo/m4-16`.

```sh
python3 tools/project.py build --preset app-debug        # status=passed
ctest --test-dir build/app-debug --output-on-failure     # 23/23 passed
```

### The five kept-frame sweeps

I ran the primary's own scripts, copied unchanged into my artifacts directory
(`hud_compare.py` is byte-identical to the primary's; `hud_compare_manifest.py`
is the primary's current copy, which now counts render failures - review 2's
advisory 7).

```sh
python3 $A/hud_compare.py "$M/boundary-a"  "$E/m4-16-rider-art/m4-16-rider-art/original-primary"    classic.crawler.zoom-zoo 1376
python3 $A/hud_compare.py "$M/brake-a"     "$E/m4-16-rider-art/m4-16-rider-art/original-brake"      classic.crawler.zoom-zoo 1376
python3 $A/hud_compare.py "$M/trick-long-a" "$E/m4-16-rider-art/m4-16-rider-art/original-trick-long" classic.crawler.zoom-zoo 1376
python3 $A/hud_compare.py "$E/m4-16-rider-art/m4-16-idle/captures/stop-timeout-a" \
    "$E/m4-16-final-review/m4-16-final-review/orig/stop-timeout-a" classic.crawler.zoom-zoo 1376
python3 $A/hud_compare_manifest.py tests/manifests/replay/race-crawler-dragster-12000-continuous-right-fields.json \
    <the primary's orig-dragster/frames> classic.crawler.dragster 1328
```

| Sweep | Frames | HUD rows | Rows the bar covered | Whole | My log |
| --- | ---: | ---: | ---: | ---: | --- |
| M4-16 primary (`boundary-a`) | 274 | **0** | **0** | 18,601 | `sweep-primary.txt` |
| brake loser race | 274 | **0** | **0** | 18,601 | `sweep-brake.txt` |
| trick-long | 192 | **0** | **0** | 12,655 | `sweep-tricklong.txt` |
| 10:00 time-out | 35 | 45,056 | 42,240 | 630,928 | `sweep-timeout.txt` |
| DRAGSTER manifest | 18 | 24,576 | 23,040 | 304,885 | `sweep-dragster.txt` |

Every row reproduces R-0043's measurement table and the second review's numbers
exactly, with 0 render failures in each. The last two rows are the arithmetic
both earlier reviews checked: the non-zero frames are the 11 blank ones from
31933 and the 6 from 3454, each contributing the whole of both boxes, so the 24
hold frames and the 12 DRAGSTER race frames are 0.

### The gates, re-run on the candidate rather than cited

```sh
python3 tools/project.py test --suite synthetic --preset app-debug \
    --artifacts $A/synthetic-artifacts --report $A/synthetic.json
#   status=passed, 50.1 s, 459 passed / 0 failed / 0 skipped, source eff7638, dirty false
python3 tools/project.py native presentation-check --manifest tests/manifests/presentation/classic-crawler-dragster-v1.json \
    --fixtures local/v1-fixtures-review/winner-fixtures --content-pack local/classic-crawler-dragster.pack --preset lab-debug ...
#   winner status=passed; the loser manifest status=passed
python3 -m tools.unirally_lab.native.gate_identity --since 6e0fad6 \
    --reports "$E/classic-stunt-names-review4/classic-stunt-names-review4/gates-6e0fad6" --expect 11 \
    --pack local/classic-pal-crawler-two-tracks-v9.pack --ninja local/toolchain/ninja-1.13.2-darwin-arm64/ninja
#   "comparing 6e0fad6 with eff7638 ... every input is byte identical and every
#   report matches; 11 gates may be cited", every restore count unchanged
```

I also read the primary's own gate run. `artifacts/classic-race-hud/gates-run-final.log`
in `.worktrees/classic-race-hud` is on `c6a73f0`; `git diff c6a73f0..eff7638` is
one line of `tasks/CLASSIC-RACE-HUD.md`, and attempt 22 of the record names the
commit, so review 2's should-fix 5 is answered. The synthetic line now reads
`rc=0 status=passed` and the script takes it from `synthetic.json` rather than
an unflushed log, so should-fix 4 is answered too. Both build and ctest logs of
all five presets read `status=passed` / `100% tests passed`, the fuzz log ends
`seeds 40 ... aborts 0`, and both hidden runs report 0 rider-pose fallback
frames. I did not re-run the preset matrix, the hidden runs or the fuzz.

### The transitions, on consecutive originals I recaptured myself

`recapture.py` (byte-identical to the primary's) replays a frozen capture's own
timeline on the audited core and asserts every replayed frame's video and WRAM
digests against `reference.json`, so the extra pictures are the frozen run's
pictures.

```sh
PYTHONPATH="$PWD" python3 $A/recapture.py "$M/boundary-a" $A/orig-primary 1670-1690,3200-3216,4835-4850,6475-6500
PYTHONPATH="$PWD" python3 $A/recapture.py "$M/brake-a" $A/orig-brake 6480-6505
PYTHONPATH="$PWD" python3 $A/recapture.py "$M/ordinary-controls/down-a" $A/orig-down 3200-3216
python3 $A/render.py CAPTURE OUT classic.crawler.zoom-zoo 1376 FIRST LAST
python3 $A/score.py OUT ORIG FIRST LAST
```

| Window | Frames | HUD rows | Player band | Opponent band |
| --- | ---: | ---: | ---: | ---: |
| primary 1670-1690 (lap 1, update 1675) | 21 | **0** | **0** | **0** |
| primary 3200-3216 (lap 2, update 3208) | 17 | **0** | 1,736 (split) | 1,488 (split) |
| primary 4835-4850 (lap 3, update 4840) | 16 | **0** | 1,791 (split) | 466 (split) |
| primary 6475-6500 (player 6484, opponent 6488) | 26 | **0** | **0** | **0** |
| brake 6480-6505 (**opponent** first, 6488; player 6492) | 26 | **0** | **0** | **0** |
| down-a 3200-3216 (player 3207, opponent 3208) | 17 | **0** | 1,416 (split) | 1,240 (split) |

The second review's blocking case is fixed where it found it: `down-a` picture
3209 is now 0 differing pixels in the HUD rows, and so is every frame of that
window. The residual in the two centred bands mid-race is the declared signed
split time; the whole-picture residual is 36 pixels a frame of the declared
off-screen rider arrow, and 0 on the two finish windows.

### The new tests bite

I mutated the predicate back three ways, rebuilt and ran ctest each time.

| Mutation | Result |
| --- | --- |
| hold on the player's counter only (`!stepped(...,0)`) | `presentation_gather_mapping_and_determinism` FAILS at require #31 |
| hold on the left field's **text** (the `428d1d3` form) | FAILS at require #31 |
| no hold at all (always publish) | FAILS at require #27 |

The tree was restored and rebuilt afterwards (23/23, `git status` clean). The
new `crossing` case in `presentation_tests.cpp` is what catches the first two,
and it is tied to picture 3209 of a named capture, which is the shape the first
review asked for.

## My independent case: the flag, both counters and both tracks

The implementation's predicate is now

```cpp
if(!stepped(previous,updated,0) && !stepped(previous,updated,1))
    latest_=classic_hud_clock(updated.movement.timer);
```

and R-0043 states the rule it implements as "the flag is `$0D17`, and what sets
it is **either rider's lap counter stepping**". I tested that against the
captures' own WRAM rather than against the implementation: for every frame of
every capture, `$0D17` (the left field's dirty flag), `$0EFB` and `$0EFD` (the
two counters), `$0E3B` (the tenths tile), `$034D` (the clock's own flag) and
`$053F` (the suppress flag), classified three ways - flag set with a counter
step, flag set without one, and a counter step with the flag clear
(`dirty_scan.py`, `dirty-zoomzoo.txt`, `dirty-dragster.txt`).

**ZOOM ZOO: the predicate is exact.** Over all 53 M4-16 ZOOM ZOO captures,
`$0D17` is set on seven or eight updates inside the race and every one of them
is one of the two counters stepping; there is no set the predicate fails to
explain, and no counter step inside the race with the flag clear. My scan agrees
frame for frame with the primary's rewritten `queue_probe.py` on the five
captures it cites. The long "still pending" run the probe prints afterwards is
genuinely unreachable rather than untested: in `boundary-a` it starts at 6830
against a race that ends at 6488, and in the 10:00 time-out capture it starts at
31930 - where I recaptured the originals 31925-31936 and found the picture is a
**single colour** until 31933, when the result screen starts drawing. There is
no picture in which the original is still drawing the in-race HUD and the flag
is pending.

**DRAGSTER: the predicate does not apply at all.** `$053F` is 1 for the whole
DRAGSTER race, and the branch R-0043 describes as "the field is left alone"
never clears `$0D17`, so the flag is set on every frame from the first crossing
to the end - 1,600 to 2,700 updates per capture in the 21 DRAGSTER captures I
scanned - while the clock plainly keeps running. A set flag therefore does not
spend the update on DRAGSTER; only an update on which the routine actually
writes something does, and the only such update is the one where `$0EFB` reaches
zero and `finish` replaces `race`.

`dragster-window-pause/window-pause/orig-banner-pause-a` contains both
behaviours 197 frames apart, and its own WRAM shows why:

```
frame  player  opp  $0D17  tenth  $034D        frame  player  opp  $0D17  tenth  $034D
 3213       1    1      1      5      0         3410       1    0      1      2      0
 3214       1    0      1      6      1         3411       0    0      1      3    255
 3215       1    0      1      6      0         3412       0    0      0      3    255
```

At 3214 the opponent's counter steps and the tenth ticks; the flag was already
set and nothing is redrawn, so the clock is republished and the original's
picture 3215 reads `0:33:6`. At 3411 the player's counter reaches zero; the left
field is written on 3412, the clock cells keep `0:36:2` in that picture, and
`$034D` goes negative so 3413 is blank. Native draws 3412 correctly and 3215 one
tenth late.

## Findings

### Blocking 1 - on DRAGSTER the clock is held on updates the original does not spend

The candidate holds the clock on every update either counter steps, on both
tracks. On DRAGSTER three of the four counter steps in a race - each rider
crossing the start line, and the opponent finishing - dirty nothing the original
redraws, so the original publishes the clock on them and the candidate does not.
Where the tenth ticks on such an update, native's tenths digit is one picture
late.

Reproduced on four accepted DRAGSTER originals this task never used, on pictures
I recaptured consecutively:

```sh
E=local/evidence; A=artifacts/classic-race-hud-review3
C="$E/dragster-window-pause/window-pause/orig-banner-pause-a"
PYTHONPATH="$PWD" python3 $A/recapture.py "$C" $A/orig-dr-banner2 3210-3218
python3 $A/render.py "$C" $A/native-dr-banner2 classic.crawler.dragster 1328 3210 3218
python3 $A/score.py $A/native-dr-banner2 $A/orig-dr-banner2 3210 3218
python3 $A/cells.py $A/orig-dr-banner2/frame-3215.png  2 24 29    # '0:33:6'
python3 $A/cells.py $A/native-dr-banner2/frame-3215.ppm 2 24 29   # '0:33:5'
```

```
frame   3214: hud-rows 0  was-bar 0  player-time 0  opponent-time 0  whole 36
frame   3215: hud-rows 6  was-bar 0  player-time 0  opponent-time 0  whole 42
frame   3216: hud-rows 0  was-bar 0  player-time 0  opponent-time 0  whole 36
```

The six pixels are x 233-238, y 19-26: tilemap column 29 of row 2, the tenths
digit, and nothing else. The 36 elsewhere is the declared off-screen arrow, on
every frame of the window including the matching ones.

| Original | Update | Who steps | Picture | Original | Native | HUD-row pixels |
| --- | ---: | --- | ---: | --- | --- | ---: |
| `dragster-window-pause/.../orig-banner-pause-a` | 3214 | opponent finishes | 3215 | `0:33:6` | `0:33:5` | 6 |
| `dragster-ordinary-controls/.../random-1-a` | 3214 | opponent finishes | 3215 | `0:33:6` | `0:33:5` | 6 |
| `dragster-clock-limit/idle-a` (the 10:00 idle) | 3214 | opponent finishes | 3215 | `0:33:6` | `0:33:5` | 6 |
| `dragster-ordinary-controls/.../regression-landing-held-roll-a` | 1599 | **player** crosses the start line | 1600 | `0:01:3` | `0:01:2` | 17 |

In `random-1-a` the player races on until 3636 and in `idle-a` never finishes at
all, so the wrong digit stands in a fully drawn, running clock; in
`regression-landing-held-roll-a` it is 1,600 frames before the finish and it is
the **player's own** counter that causes it.

This is a regression against the reviewed predecessor, not only against the
original. Rebuilding the candidate with the `428d1d3` text-equality predicate
and nothing else changed draws both frames exactly:

```
frame   3215: hud-rows 0 ... whole 0          (text-equality build, random-1-a)
frame   1600: hud-rows 0 ... whole 470        (text-equality build, landing; 470 is the 487 above minus these 17)
```

Scope, from the WRAM scan over the 21 DRAGSTER captures in the accepted
evidence: every capture has three wrongly held updates, and six of the eleven
distinct cases (`random-1`, `random-2`, `random-3`,
`regression-landing-held-roll`, `orig-banner-pause` and its `-odd` twin, and the
clock-limit `idle`) have at least one where the tenth ticks while the clock is
still on screen. It is invisible to every gate the task runs: the eleven
differential compares are state-level and cite `gate_identity`, the DRAGSTER
kept-frame sweep samples every 20 frames and its manifest is the one race where
the opponent's step falls after the player's finish (so the clock is blanked two
pictures later anyway), and every clock unit test is a tour race.

The fix does not need new state and does not disturb anything measured above.
The original spends the update when the routine writes the field, which is the
tour race's lap number (either counter, as now) or the word `finish` when
`$0EFB` reaches zero - that is, `scenario.tour_race || the player's counter
reached 0`. `classic_hud_left_field` already carries both conditions. Measured
against my windows that predicate gives the candidate's ZOOM ZOO behaviour
unchanged, keeps the DRAGSTER finish hold that picture 3412 of
`orig-banner-pause-a` demands, and drops the three holds that produce the table
above. Whatever form is chosen should come with a DRAGSTER unit case beside the
ZOOM ZOO ones and a consecutive-picture measurement on one of the six affected
originals.

### Should-fix 2 - the recovered rule is recorded without the branch that contradicts it

`presentation.hpp` and R-0043 both state, unqualified, that "the flag is
`$0D17`, and **either** rider's lap counter sets it", and that an update that
dirties the field spends the queue. R-0043 states the contradicting fact two
sections earlier - with `$053F` set "the field is left alone" - and never
reconciles the two, so the record now asserts a mechanism that is false on one
of the product's two tracks. The measurable version is above: on DRAGSTER the
flag is set continuously and only the `finish` write spends an update. AGENTS
asks the project not to describe a plausible reading as the recovered
behaviour; this is the third round in which the stated rule is narrower or wider
than the original's, so it is worth stating what was measured on each track
rather than one sentence covering both.

### Should-fix 3 - `queue_probe.py` still cannot see its own counter-example

The rewritten probe is a real improvement - it scans `$0D17` itself, which the
first version read and discarded - but it was run on five ZOOM ZOO captures
only, and R-0043 cites it as the evidence that "the rule was checked rather than
fitted". Run unchanged on any DRAGSTER capture it prints 1,600 to 2,700 "still
pending" updates and the four counter steps, which is the disproof of the rule
it is cited for. This is the same shape of hole as the 20-frame picture set in
round 1 and the player-only scan in round 2: a search whose domain excludes the
counter-example. `dirty_scan.py` in my artifacts is a drop-in replacement that
takes both tracks and reports the two failure directions separately.

### Should-fix 4 - no DRAGSTER case among the clock tests

Every `ClassicRaceHudClock` case in `presentation_tests.cpp` is a ZOOM ZOO
state. Nothing in the suite binds the non-tour behaviour, which is why blocking
1 passes 23/23. One case with `scenario.tour_race == false`, an opponent step
mid-race and a player step to zero would have failed this candidate.

### Should-fix 5 - two counts in R-0043 do not match its own captures

R-0043 says `$0D17` is set on "six or seven updates inside the race proper" over
the five cited ZOOM ZOO captures. The probe's own output is seven for
`boundary-a` and `brake-a` and eight for `down-a`, `compound-reverse-a` and
`trick-long-a`. The same sentence says the no-history fallback "differs from the
original on the pictures after those updates - six or seven per race": the
pictures that can differ are only those where the tenth ticked on a hold update,
which is 0 to 2 per race in those five captures (`boundary-a` 0, `brake-a` 1 at
the finish, `down-a` 1, `compound-reverse-a` 2, `trick-long-a` 1), and fewer
still once the blanked clock is taken out. Both numbers are cheap to correct
from the probe's own log.

### Advisory 6 - the "one picture" wording survives in two places

Review 2's should-fix 6 was applied to R-0043 but not to
`presentation.hpp`'s `published_clock` paragraph or to the single-state comment
in `classic_race_presentation_runner.cpp`, which still read "exact except on the
one picture after an update that redrew the left field". That is literally
defensible (one picture per such update) and neither caller is on a gated path,
so it is advisory, but the two wordings no longer match the record they cite.

### Advisory 7 - `gates.sh` still rewrites the shared `local/` layout

Unchanged from review 2's advisory 11: the script does
`rm -rf local/v1-fixtures && mkdir -p local/v1-fixtures` inside the primary's
worktree, where that path was a symlink into the main checkout's `local/`. It
removed only the link, and `docs/BUILD_AND_VALIDATION.md` now explains why the
fixtures must be copied rather than linked, which is a genuine improvement. A
script that rewrites the shared layout is still one character away from deleting
the fixtures it copies.

### Advisory 8 - the counter steps outside the race, for completeness

Every capture on both tracks has one update where **both** counters step with
`$0D17` clear (ZOOM ZOO 1291, DRAGSTER 1243), and `restart-explore` has a second
at 7850: race setup and restart. The candidate would hold the clock on them. It
is unreachable - native's timeline begins at the scenario's
`initialization_frame` (1376 and 1328) with the counters already set, the
`--timeline` runner refuses a timeline that does not, and both the app and the
fuzz runner replace `LivePresentation` wholesale on a restart, which
value-initializes the tracker - and `reset()` now clears `clock_` as well. I
checked this by reading `sdl_main.cpp`, `dragster_fuzz_runner.cpp` and
`classic_race_presentation_runner.cpp` rather than by playing.

### What I checked and found sound

The second review's blocking 1 is fixed and the fix is right rather than
shifted, on three races and every transition listed above. The 60000 guard on
the player's finish time (review 2's advisory 8) is now symmetric with the
opponent's and is honestly labelled in the comment as belt and braces. The
renderer cannot throw on anything the HUD gives it: `draw_bg3_text` bounds-checks
every pixel, and every character `classic_race_hud_text` produces - digits,
`:`, `/`, `finish`, `race` - is in `classic_caption_tile`'s table, which the
change widened rather than narrowed. `hud_time` and `classic_hud_clock` reduce
every field modulo 10 and `classic_hud_lap` clamps before subtracting, so there
is no undefined arithmetic on out-of-range input. No baseline, threshold or
expected result is weakened anywhere in `92f46ba..eff7638`, and the synthetic
run reports 0 skipped.

## Readability

The extraction into `classic_hud_left_field` / `classic_hud_clock` /
`draw_bg3_text` reads well and I checked it is behaviour-preserving against the
inline form it replaced. Deriving every gate from a picture number and citing
the frames is the part of this change a later reader most needs, and it is
clear. The one thing I would change beyond blocking 1 is the comment inside
`observe_update`: it explains the ZOOM ZOO crossing case at length and does not
mention that the other track never takes this branch, which is exactly the fact
a reader needs to keep the rule true.

## What I did not check

- Hosted CI on this tip, which the acceptance table requires at integration;
  GCC `-Werror` has caught what macOS Clang did not before.
- The five-preset build matrix, the two hidden app runs and the 40-seed fuzz. I
  read the primary's `gates-c6a73f0` logs for those and re-ran app-debug's build
  and ctest, the synthetic suite, both v1 contracts and `gate_identity` on the
  candidate itself.
- The eleven differential compares, which are cited rather than re-run. I re-ran
  the citation and agree it is sound for a presentation-only change: the tool
  proves byte identity from ninja's own dependency list, and the gate binary
  links no presentation symbol.
- Live play, and the paused pictures, where the original dims the whole screen
  and native does not (a declared omission). My DRAGSTER cases avoid the pause
  windows of the captures they come from.
- Whether review 2's advisory 9 (a one-picture channel-6 window difference at
  the finish) predates this change; I saw the same 57 pixels on brake 6495 and
  did not build the base commit to attribute them.
- The signed split time, two-player, the pre-race and result writers, and the
  rest of the `$81:CFF6` family, all declared out of scope.
- `$77:074B` as the race mode at the ROM level, like both earlier reviewers; I
  confirmed only the behaviour it selects.

## What I would need to approve

Blocking 1 fixed - the clock held only on updates the original's left field is
actually written, which on DRAGSTER is the player's finish alone - with a
DRAGSTER unit case beside the ZOOM ZOO ones and consecutive originals measured
on one of the four captures in the table above (`orig-banner-pause-a` 3210-3218
and 3405-3416 is the cheapest pair, because it shows both behaviours in one
race). Should-fix 2 to 5 should ride with it, because they are the reasons this
counter-example survived two rounds. Everything else I measured on this
candidate holds and I would not ask for any of it to be re-done.
