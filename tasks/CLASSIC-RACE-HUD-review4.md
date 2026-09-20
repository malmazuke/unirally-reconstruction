# CLASSIC-RACE-HUD - independent review of `5e1f51b`, fourth round

**Verdict: return.** The narrowed predicate is right. I read `$81:EB86`-`$81:EC5E`,
`$81:ECBC` and the routine's only writer of `$0D17` out of the ROM myself, and
"the left field was **written**, not merely dirty" is exactly what the original
does; I then tested that rule against the candidate's predicate on every frame
of all 125 WRAM captures in the accepted evidence, on both tracks, and inside
the race the two agree everywhere the picture can show it. Review 3's blocking
finding is fixed and I confirmed the fix on consecutive originals I recaptured,
as well as every ZOOM ZOO transition the earlier rounds established. The five
kept-frame sweeps are byte-identical to the primary's own run.

I am returning it for a different mechanism in the same feature, which the
dispatch asked me to try and which breaks: **the finish sequence is placed by
fixed offsets from the player's finish rather than by the queue the record
describes.** When the opponent's lap counter steps on the update right after the
player finishes, the original's queue spends that update writing `finish` a
second time and everything behind it slides a picture. Native does not, so on
two accepted DRAGSTER races the corner clock is blanked one picture early and
both finish times are drawn early - 918 differing pixels over three consecutive
pictures, in the HUD's own rows and in both finish-time bands. This is not a
regression against the reviewed predecessor; it is the residual of the first
review's blocking 2 in a case no round has measured, because every
consecutive-picture finish measured so far had the two riders finishing three or
more updates apart.

Reviewer: fresh Claude Opus 5 session, no inherited conversation, isolated
worktree `.worktrees/classic-race-hud-review` detached at `5e1f51b` (branch
`review/classic-race-hud-4`). Artifacts: `artifacts/classic-race-hud-review4/`
in that worktree (ignored). Date: 20 September 2026.

I read all three earlier reports (`8671cfc`, `603d377`, `697a81c`), R-0043 and
the task record before starting, and treated every number in them as a claim.

## What I reproduced, with the exact commands

From the review worktree with
`export PATH="$PWD/local/toolchain/ninja-1.13.2-darwin-arm64:$PATH"`. `E` is
`local/evidence`, `A` is `artifacts/classic-race-hud-review4`, `M` is
`$E/m4-16-playable-zoom-zoo/m4-16`.

```sh
python3 tools/project.py build --preset app-debug         # status=passed
ctest --test-dir build/app-debug --output-on-failure      # 23/23 passed
```

### The five kept-frame sweeps

`hud_compare.py` and `hud_compare_manifest.py` are byte-identical copies of the
primary's current scripts (sha256 `a42cd8e5f14db058...`, `318c18e5e8f7e6d3...`).

```sh
python3 $A/hud_compare.py "$M/boundary-a"   "$E/m4-16-rider-art/m4-16-rider-art/original-primary"    classic.crawler.zoom-zoo 1376
python3 $A/hud_compare.py "$M/brake-a"      "$E/m4-16-rider-art/m4-16-rider-art/original-brake"      classic.crawler.zoom-zoo 1376
python3 $A/hud_compare.py "$M/trick-long-a" "$E/m4-16-rider-art/m4-16-rider-art/original-trick-long" classic.crawler.zoom-zoo 1376
python3 $A/hud_compare.py "$E/m4-16-rider-art/m4-16-idle/captures/stop-timeout-a" \
    "$E/m4-16-final-review/m4-16-final-review/orig/stop-timeout-a" classic.crawler.zoom-zoo 1376
python3 $A/hud_compare_manifest.py tests/manifests/replay/race-crawler-dragster-12000-continuous-right-fields.json \
    ../classic-race-hud/artifacts/classic-race-hud/orig-dragster/frames classic.crawler.dragster 1328
```

| Sweep | Frames | HUD rows | Rows the bar covered | Whole |
| --- | ---: | ---: | ---: | ---: |
| M4-16 primary (`boundary-a`) | 274 | **0** | **0** | 18,601 |
| brake loser race | 274 | **0** | **0** | 18,601 |
| trick-long | 192 | **0** | **0** | 12,655 |
| 10:00 time-out | 35 | 45,056 | 42,240 | 630,928 |
| DRAGSTER manifest | 18 | 24,576 | 23,040 | 304,885 |

Every number is identical to `verify-round3-full.log` in the primary's artifacts
and to review 3's table, with 0 render failures in each. Nothing regressed.

### The gates, re-run on the candidate rather than only cited

```sh
python3 tools/project.py test --suite synthetic --preset app-debug \
    --artifacts $A/synthetic-artifacts --report $A/synthetic.json
python3 tools/project.py native presentation-check \
    --manifest tests/manifests/presentation/classic-crawler-dragster-v1.json \
    --fixtures local/v1-fixtures-review4/winner-fixtures \
    --content-pack local/classic-crawler-dragster.pack --preset lab-debug ...   # and the loser manifest
python3 -m tools.unirally_lab.native.gate_identity --since 6e0fad6 \
    --reports "$E/classic-stunt-names-review4/classic-stunt-names-review4/gates-6e0fad6" --expect 11 \
    --pack local/classic-pal-crawler-two-tracks-v9.pack --ninja local/toolchain/ninja-1.13.2-darwin-arm64/ninja
```

* synthetic suite: `status=passed`, 50.5 s, **459 passed / 0 failed / 0 skipped
  / 0 missing / 0 timeout**;
* both v1 presentation contracts: winner `status=passed elapsed=1.598s`, loser
  `status=passed elapsed=0.62s`, run against my own `local/v1-fixtures-review4`
  copy so the shared symlink is untouched;
* `gate_identity`: rc=0, "every input is byte identical and every report
  matches; 11 gates may be cited", every restore count unchanged.

The primary's `gates-4bb9a60/` and `verify-round3-full.log` are on `4bb9a60`;
`git diff 4bb9a60..5e1f51b` is one line of `tasks/CLASSIC-RACE-HUD.md`, so the
gate run does cover the candidate. `gates.sh` now copies the v1 fixtures into a
task-local `local/v1-fixtures-classic-race-hud` instead of destroying the shared
`local/v1-fixtures` - review 3's advisory 7 is answered. I used my own
`local/v1-fixtures-review4` and left the shared link alone.

### The transitions, on consecutive originals I recaptured myself

`recapture.py` is byte-identical to the primary's (sha256 `359ab534b4cc8e2d...`):
it replays a frozen capture's own timeline on the audited core and asserts every
replayed frame's video and WRAM digests against `reference.json`, so the extra
pictures are the frozen run's pictures.

```sh
PYTHONPATH="$PWD" python3 $A/recapture.py CAPTURE OUT 3206-3224
python3 $A/render.py CAPTURE OUT classic.crawler.dragster 1328 FIRST LAST
python3 $A/score.py NATIVE ORIG FIRST LAST
python3 $A/cells.py PICTURE 2 1 29        # reads the glyphs straight out of the picture
```

| Window | Frames | HUD rows | Player band | Opponent band |
| --- | ---: | ---: | ---: | ---: |
| primary 1670-1690 (lap 1) | 21 | **0** | **0** | **0** |
| primary 3200-3216 (lap 2) | 17 | **0** | 1,736 (split) | 1,488 (split) |
| primary 4835-4850 (lap 3) | 16 | **0** | 1,791 (split) | 466 (split) |
| primary 6475-6500 (player 6484, opponent 6488) | 26 | **0** | **0** | **0** |
| brake 6480-6505 (**opponent** first) | 26 | **0** | **0** | **0** |
| down-a 3200-3216 (both riders cross, 3207/3208) | 17 | **0** | 1,416 (split) | 1,240 (split) |
| DRAGSTER `regression-landing-held-roll-a` 1595-1605 (review 3 B1) | 11 | **0** | **0** | **0** |
| DRAGSTER `random-1-a` 3206-3224 (review 3 B1) | 19 | **0** | **0** | **0** |
| DRAGSTER `random-1-a` 3630-3650 (its finish) | 21 | **0** | **0** | **0** |
| DRAGSTER `orig-banner-pause-a` 3408-3418 (its finish) | 11 | **0** | **0** | **0** |
| DRAGSTER `regression-landing-held-roll-a` 3206-3224 | 19 | **197** | **227** | **494** |
| DRAGSTER `orig-continuous-right-a` 3206-3224 | 19 | **197** | **227** | **494** |

The first six rows reproduce review 3's numbers exactly; the residual in the two
centred bands mid-race is the declared signed split time and the whole-picture
residual is the declared off-screen rider arrow (36 pixels a frame at x 224-231,
y 112-126 on DRAGSTER). Rows seven to ten are the cases review 3 returned the
task for, and they are fixed: `cells.py` reads `0:33:6` - the **new** tenth, which review 3
measured native drawing as `0:33:5` - from both pictures at `random-1-a` 3215,
and the DRAGSTER finish at `orig-banner-pause-a` keeps `0:36:2` in picture 3412
and blanks at 3413, exactly as the original does. The
last two rows are blocking 1.

### The tests

I mutated the predicate three ways, rebuilt and ran ctest each time, then
restored the tree (`git status` clean, 23/23).

| Mutation | Result |
| --- | --- |
| drop the `tour_race` condition (hold on any counter step) | `presentation_gather_mapping_and_determinism` FAILS at require #34 |
| hold on nothing (always publish) | FAILS at require #27 |
| **drop the finish condition (`wrote_finish=false`)** | **23/23 still pass** - see should-fix 2 |

## My independent case: the ROM, then every capture

### What the routine actually does

```
$81:8187  LDY $0FF9 / LDA #$0001 / STA $0D17 / LDA $0EFB,y / DEC / STA $0EFB,y
$81:EB86  LDA $0D17 / BNE $EB8E        flag clear -> JMP $ECC2 (the clock handler)
$81:EB8E  LDA $0EFB / BEQ $EB9E        the player's laps are zero -> write `finish`
$81:EB93  LDA $053F / BNE $EB9B        suppressed -> JMP $ECC2, no write, flag NOT cleared
$81:EB98  JMP $EC61                    -> write the lap number
$81:EC5E / $81:EC98                    both write paths reach
$81:ECBC  STZ $0D17 / JMP $F357
```

`STA $0D17` at `$81:818D` is the **only** write of a set value to that address in
the whole ROM, and it sits inside the lap-counter decrement with the rider
chosen by `$0FF9`, so "either rider's lap counter sets the flag" is the ROM's
own structure and not an inference from captures. The left field is therefore
written on an update exactly when `$0D17 != 0 && ($0EFB == 0 || $053F == 0)`,
which is the candidate's predicate. The asymmetry the record leans on is real
and deliberate-looking: the two-player routine's own suppressed branch
(`$81:DEE2` / `$81:DEEA`) jumps to `$81:E00E`, which *does* `STZ $0D17`; the
one-player routine's `$81:EB9B` does not.

`$053F` is written in four places, all `LDA #$0001` in the mode-0 field setup
(`$81:D292`, `$81:D39B`, `$81:D6EB`, `$81:D774`), read in three (`$81:DDB4`,
`$81:DEE2`, `$81:EB93`), and cleared by nothing. I checked the scenario flag
against it rather than assuming: over the race window of all 125 captures
`$053F` never changes value, and it is 0 on all 86 ZOOM ZOO races and 1 on all
39 DRAGSTER ones (including the three `classic-stunt-names` captures, which are
DRAGSTER races). `tour_race` is a faithful stand-in for `$053F == 0` inside this
product's two tracks; R-0043's "Domain and limits" already says it is the race
mode and not a lap count, which is the honest form of that claim.

### The predicate against every capture

`$A/write_scan.py` evaluates the ROM condition above and the candidate's
predicate on the same state pair, for every frame of every capture, and reports
where they disagree, whether the tenth ticked there, and the longest run of
consecutive updates holding `$0D17`.

```sh
python3 $A/write_scan.py --start 1376 $(zoom zoo captures)     # $A/scan-zoomzoo.txt
python3 $A/write_scan.py --start 1328 $(dragster captures)     # $A/scan-dragster.txt
```

* **ZOOM ZOO, 86 captures: zero disagreements.** Inside the race `$0D17` is set on
  4 to 8 updates (7 or 8 in a race run to the finish) and every one is a lap
  counter stepping; the flag is consumed on the next update every time.
* **DRAGSTER, 39 captures: the flag is sticky** for 124 to 30,311 consecutive
  updates - 1,639 to 2,717 in the captures that run a race to its finish, which
  brackets R-0043's "1,639 to 2,061" over more captures than it cites -
  and the candidate's predicate agrees with the ROM on every update **except**
  one shape: after the player has finished, an opponent counter step re-sets
  `$0D17` and the original writes `finish` a second time. That happens on six
  captures, always at picture 3215. On four of them it changes nothing visible,
  because the clock has already been blanked. On the other two it delays the
  blank, and that is blocking 1.

### Everything else the dispatch asked me to break

The countdown, a pause, a restart, a time-out, the result-loading region, a tie
and an opponent that finishes first are all sound, and I checked each rather
than reasoning about it:

* **Countdown and race setup.** Both counters step once at setup (ZOOM ZOO 1291,
  DRAGSTER 1243) with `$0D17` clear, and the candidate would hold there. It is
  unreachable: native's timeline starts at the scenario's `initialization_frame`
  (1376, 1328) with the counters already set. `$A/write_scan.py` run from frame
  0 shows it; run from the initialization frame it does not appear.
* **Pause.** `clock_.observe_update` is called before
  `ClassicRaceHistoryTracker`'s `result_updates || zoom_zoo_update_was_paused`
  return, so a suspended update *does* publish - but with the timer frozen it
  republishes the same six characters, and the four pause captures
  (`pause-a`, `pause-countdown-a`, `orig-countdown-pause-a`,
  `orig-banner-pause-a`) show zero disagreement with the ROM rule.
* **Restart.** `sdl_main.cpp` and `dragster_fuzz_runner.cpp` both replace
  `LivePresentation` wholesale when the frame goes backwards, and `reset()`
  clears `clock_`. `restart-explore` shows no disagreement inside its race.
* **Time-out.** `$0EFB` never reaches zero, so the ROM takes the suppressed
  branch on DRAGSTER and the lap-number branch on ZOOM ZOO; `dragster-clock-limit/idle-a`
  has 0 writes in 30,311 dirty updates and `stop-timeout-a` has 5, all counter
  steps, all matched.
* **Result-loading region.** The race's WRAM is reused there (`$0D17` reads 8,
  the clock bytes read nonsense), which is the long "still pending" run the
  probes print. The in-race HUD is not drawn in those pictures.
* **A tie.** `regression-countdown-actions-tie-a`: both counters step on the same
  update, one write, and the candidate agrees.
* **An opponent that finishes first.** `brake-a` and `loss-a`, both matched, and
  the brake loser race's 6480-6505 pictures are 0 in every box.
* **A DRAGSTER race where the opponent crosses after the player's finish.** This
  is the one that breaks. Blocking 1.

## Findings

### Blocking 1 - the finish sequence is placed by fixed offsets, not by the queue

`classic_race_hud_text` gates the three post-finish fields on constants:

```cpp
if(!finished || race.finish_delay<1)                 hud.clock=...;          // blank at finish+2
if(finished && race.finish_delay>=2 && ...)          hud.player_time=...;    // player time at finish+3
if(... && previous_update.movement.frame>=*opponent+1U) hud.opponent_time=...; // opponent time at opponent+2
```

Those offsets are the queue's output for a race where nothing else is competing
for it, which is every race measured so far. The queue itself is what R-0043
recovers: one field per update, the left field first, and an update that writes
the left field returns before the clock handler. After the player finishes,
`$0D17` is clear and the queue is free - **unless the opponent's counter steps on
the very next update**, which re-sets the flag while `$0EFB` is already zero, so
`$81:EB9E` writes `finish` a second time and every field behind it slides one
picture.

Reproduced on two accepted DRAGSTER races this task never measured, on pictures
I recaptured consecutively:

```sh
E=local/evidence; A=artifacts/classic-race-hud-review4
C="$E/dragster-ordinary-controls/dragster-ordinary-controls/originals/regression-landing-held-roll-a"
PYTHONPATH="$PWD" python3 $A/recapture.py "$C" $A/orig-dr-landing 3206-3224
python3 $A/render.py "$C" $A/native-dr-landing classic.crawler.dragster 1328 3206 3224
python3 $A/score.py $A/native-dr-landing $A/orig-dr-landing 3206 3224
python3 $A/cells.py $A/orig-dr-landing/frame-3215.png   2 1 29   # 'finish.................0:33:5'
python3 $A/cells.py $A/native-dr-landing/frame-3215.ppm 2 1 29   # 'finish.......................'
```

```
frame   3215: hud-rows 197  was-bar 0  player-time 0    opponent-time 0    whole 197
frame   3216: hud-rows 0    was-bar 0  player-time 227  opponent-time 247  whole 474
frame   3217: hud-rows 0    was-bar 0  player-time 0    opponent-time 247  whole 247
```

The 197 pixels at 3215 are x 193-238, y 19-29: the clock cells, columns 24-29 of
rows 2-3, and nothing else. The differences at 3216 and 3217 are x 105-158 in
the two finish-time bands and nothing else.

| Picture | Original | Native |
| ---: | --- | --- |
| 3214 | `finish` written; clock still `0:33:5` | same |
| 3215 | `finish` written **again** (the opponent's counter stepped on 3214); clock still `0:33:5` | clock already blank |
| 3216 | the clock is blanked | the player's time `0:33:57` **and** the opponent's `0:33:58` |
| 3217 | the player's time `0:33:57` | the opponent's time still early |
| 3218 | the opponent's time `0:33:58` | matches from here on |

The same three pictures, pixel for pixel (197 / 227+247 / 247), on
`$E/dragster-window-pause/window-pause/orig-continuous-right-a`, a race from a
different task's evidence.

Scope, from `$A/blank_scan.py` over all 125 captures: the condition is "the
opponent's counter steps on the update after the player's laps reach zero", and
it holds in 3 captures / 2 distinct races - `regression-landing-held-roll` (and
its `-b` twin) and `orig-continuous-right`. It is DRAGSTER-only in the evidence
because a one-lap sprint finishes close, but the mechanism is track-independent:
a ZOOM ZOO race in which the opponent crosses **any** line one update after the
player finishes would do the same, and nothing in the implementation or the
record excludes it. No gate can see it: the differential compares are
state-level, the DRAGSTER kept-frame set jumps 3213 to 3400 (and its own race
finishes at 3453), and every unit case gives the two riders four updates or more
between their finishes.

The fix wants the queue rather than more constants, and the candidate has
already built the hard half of it: `ClassicRaceHudClock::observe_update` now
computes "the left field was written on this update" exactly. Counting those
writes after the player's finish and subtracting them from the three gates gives
the original's placement; the opponent's gate needs the queue's order too, since
in this race its time waits behind the player's. Whatever form is chosen should
come with a unit case in which the opponent finishes on finish+1 and a
consecutive-picture measurement on one of the two races above.

### Should-fix 2 - the new DRAGSTER test does not bind the half of the predicate it is for

The round's headline change is that a DRAGSTER race holds the clock on its
finish and on nothing else. Rebuilding with `wrote_finish=false` - deleting the
finish half of the predicate outright - still passes **23/23**. The reason is in
the fixture: the test's finish pair is

```cpp
m.movement.timer.tenths = 7;
n = m;                       // n's timer is m's timer
n.race.riders[0].laps_remaining = 0;
```

so the held string and the republished string are both `0:33:7` and the two
`require`s that say "held by `finish`" are satisfied either way. The condition is genuinely load-bearing, and the
mutated build proves it costs a real picture:

```sh
# with `const bool wrote_finish=false;` and nothing else changed
ctest --test-dir build/app-debug                                   # 100% tests passed, 0 failed out of 23
python3 $A/render.py "$E/dragster-window-pause/window-pause/orig-banner-pause-a" \
    $A/nofinish-dr-banner classic.crawler.dragster 1328 3408 3418
python3 $A/score.py $A/nofinish-dr-banner $A/orig-dr-banner 3408 3418
#   frame 3412: hud-rows 17 ...
python3 $A/cells.py $A/nofinish-dr-banner/frame-3412.ppm 2 1 29    # 'finish.................0:36:3'
python3 $A/cells.py $A/orig-dr-banner/frame-3412.png    2 1 29     # 'finish.................0:36:2'
```

`orig-banner-pause-a` update 3411 reaches zero *and* ticks the tenth, so the
original's picture 3412 keeps `0:36:2` and the mutant draws `0:36:3`. A
one-character change to the fixture (`n.movement.timer.tenths = 8;`, then
require `0:33:7` and then `0:33:8`) would bind the condition against that
measured capture. As it stands this is the
same shape of hole the three earlier rounds each had: a check whose domain
cannot contain the case it is cited for.

### Should-fix 3 - R-0043's corrected count is wrong in the other direction

R-0043 now says the no-history fallback "differs from the original on the
pictures after the updates that write the field - at most four a tour race and
one a sprint". Measured over every capture: a ZOOM ZOO race run to the finish
writes the field **7 or 8** times, not four (my scan and the primary's
`queue_probe.py` agree, and the five cited captures are 7, 7, 8, 8, 8); and a
sprint writes it **twice** in six of the 39 DRAGSTER captures, which is exactly
the second `finish` write of blocking 1. The trailing clause - "none or one
picture in most races and was two in the worst measured" - is right and is the
number a reader needs; the "at most four ... and one" is the third wrong count in
this one sentence's history and is cheap to drop.

### Advisory 4 - two ROM citations in the new comments are loose

`presentation.cpp`, `presentation.hpp` and R-0043 all say `$81:EB93` "falls
through at `$81:EB9B`". `$81:EB93` is `LDA $053F` and `$81:EB96` is a `BNE` to
`$81:EB9B`, which is itself `JMP $ECC2`; nothing falls through. The same
sentences say "`$81:EC5E` and `$81:EB98` both end at `$81:ECBC`": `$81:EB98` is
`JMP $EC61`, and it is the *path* from there that ends at `$81:ECBC` (via
`$81:EC98`), not the instruction. The mechanism described is right in both
cases; only the wording would mislead someone checking it against a
disassembler. Worth a line while the record is open, given how much of this
task's history is people re-deriving these three branches.

### Advisory 5 - `dragster_probe.py` mislabels a simultaneous step

The new probe labels a step `'player' if p != rows[i-1][1] else 'opponent'`, so
an update on which both counters step (which happens at setup in every capture,
and at the finish of the tie race) is reported as the player alone. It does not
affect any conclusion drawn from it - the counts are right - but the probe is
cited in R-0043 as the evidence for the DRAGSTER half of the rule, and the same
class of blind spot in its predecessor is what returned this task twice.
`$A/write_scan.py` reports both riders and both failure directions.

### What I checked and found sound

Review 3's blocking finding is fixed and fixed at the right place, confirmed on
four DRAGSTER windows and every ZOOM ZOO transition the earlier rounds
established. No baseline, threshold or expected result is weakened anywhere in
`92f46ba..5e1f51b`: the `require`s the diff removes are the old authored-bar
assertions the task replaces, the two mutations that do bite trip requires #27
and #34 of a block that had far fewer before this task, and the synthetic run
reports 0 skipped, 0 missing and 0 timeout. There is no undefined
arithmetic: `classic_hud_clock` and `hud_time` reduce every field modulo 10 and
`classic_hud_lap` clamps before subtracting. The renderer cannot throw on
anything the engine publishes: every character `classic_race_hud_text` can emit
- digits, `:`, `/`, `race`, `finish` - is in `classic_caption_tile`'s table, and
`draw_bg3_text` bounds-checks each pixel. Review 3's advisory 6 (the "one
picture" wording) is applied in both places, and its advisory 7 (`gates.sh`
rewriting the shared `local/`) is properly fixed rather than worked around.

## Readability

The `observe_update` comment is now the best statement of this mechanism
anywhere in the tree, including the record: it names both write paths, says why
a dirty flag alone is not enough, and cites the capture and picture that proved
each. If blocking 1 is fixed, the thing to keep is that shape - the three finish
gates deserve the same treatment, because right now they are three bare integers
whose provenance is a table in R-0043 that only covers the uncontended case.

## What I did not check

- Hosted CI on this tip, which the acceptance table requires at integration; GCC
  `-Werror` has caught what macOS Clang did not before.
- The five-preset build matrix, the two hidden app runs and the 40-seed fuzz. I
  read the primary's `gates-4bb9a60` logs for those and re-ran app-debug's build
  and ctest, the synthetic suite, both v1 contracts and `gate_identity` on the
  candidate itself.
- The eleven differential compares, which are cited rather than re-run. I re-ran
  the citation and agree it is sound for a presentation-only change.
- Live play, and the paused pictures, where the original dims the whole screen.
- The DRAGSTER manifest sweep's original pictures are the primary's
  `orig-dragster/frames`; I did not recapture those 18, having recaptured 100
  DRAGSTER pictures of my own from four other captures.
- Two-player, the pre-race and result screens, and the lap field's two-digit
  forms, all declared out of scope by R-0043.
