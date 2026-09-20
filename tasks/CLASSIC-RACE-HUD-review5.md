# CLASSIC-RACE-HUD - independent review of `d9c9485`, fifth round

**Verdict: approve.** Review 4's blocking finding is fixed, and fixed by the
mechanism the record now describes rather than by another constant. On the race
that round measured at 918 differing pixels over pictures 3215-3217, the
candidate is pixel-identical to the original **over the whole 256x224 picture on
all nineteen consecutive pictures I recaptured**, 3206-3224. Every regression
window I measured - the ZOOM ZOO finish, a ZOOM ZOO lap change, the loser race,
the DRAGSTER finish - is 0 differing pixels in all four HUD boxes, and the five
kept-frame sweeps reproduce the primary's own numbers exactly. All three
mutants the dispatch named are killed, each by a distinct `require`, and each
from a build I confirmed reported `status=passed` before the tests ran.

I found nothing blocking. I have no reproducible picture difference to offer,
so nothing below is a return. Two should-fix items are record accuracy in
`R-0043` and one overstated line in the task record; the rest are advisories,
including the two declared omissions the dispatch named.

**This round was deliberately scoped.** The dispatch gave it roughly one
review's budget and told me not to re-derive the mechanism, which rounds 1-4
did and which round 4 confirmed from the ROM. I did not re-read the ROM, did
not re-run the 125-capture predicate scan, and did not re-run the gate suites.
"What I did not check" at the bottom says exactly what that leaves unverified,
and it is more than in any previous round.

Reviewer: fresh Claude Opus 5 session, no inherited conversation, isolated
worktree `.worktrees/classic-race-hud-review` detached at `d9c9485`, branch
`review/classic-race-hud-5`. Artifacts: `artifacts/classic-race-hud-review5/`
in that worktree (ignored). Date: 20 September 2026. A previous fifth-round
session was killed by the account's five-hour session limit before writing
anything; I reused none of its measurements and recaptured and re-rendered
every picture I cite in this session.

I read review 4 (`ebf137a`) in full and treated every number in it as a claim
to reproduce. Rounds 1-3 (`8671cfc`, `603d377`, `697a81c`) I used only for the
windows they established.

## What I reproduced, with the exact commands

From the review worktree with

```sh
export PATH="$PWD/local/toolchain/ninja-1.13.2-darwin-arm64:$PWD/local/toolchain/cmake-3.31.10-darwin-arm64/bin:$PATH"
```

`E` is `local/evidence`, `A` is `artifacts/classic-race-hud-review5`, `R` is
`artifacts/classic-race-hud-review4` (the helper scripts, kept from the fourth
round in the same worktree).

```sh
python3 tools/project.py build --preset app-debug         # status=passed
ctest --test-dir build/app-debug --output-on-failure      # 100%, 23/23, 0 failed
```

`recapture.py` (sha256 `359ab534b4cc8e2d...`) is byte-identical to the
primary's current `artifacts/classic-race-hud/recapture.py`, as are
`hud_compare.py` (`a42cd8e5f14db058...`) and `hud_compare_manifest.py`
(`318c18e5e8f7e6d3...`). It replays the capture's own frozen timeline on the
audited bsnes core and asserts **every** replayed frame's video digest and WRAM
digest against `reference.json` before keeping a picture, so the extra pictures
are the frozen run's pictures. `render.py`, `score.py` and `cells.py` are my
own from round 4; `score.py`'s boxes are `hud-rows` y 15-30, `was-bar` y 0-14,
`player-time` x 104-159 y 39-54, `opponent-time` x 104-159 y 159-174, `whole`
the full 256x224.

### 1. Review 4's blocking finding: the contended finish

```sh
C="$E/dragster-ordinary-controls/dragster-ordinary-controls/originals/regression-landing-held-roll-a"
PYTHONPATH="$PWD" python3 $R/recapture.py "$C" $A/r5-orig-landing 3206-3224
#   {"frames_checked": 2066, "kept": 19}   - 2,066 frames asserted against the frozen digests
python3 $R/render.py "$C" $A/r5-native-landing classic.crawler.dragster 1328 3206 3224
python3 $R/score.py $A/r5-native-landing $A/r5-orig-landing 3206 3224
```

```
frame 3206 .. frame 3224: hud-rows 0  was-bar 0  player-time 0  opponent-time 0  whole 0
totals:                   hud-rows 0  was-bar 0  player-time 0  opponent-time 0  whole 0
```

Nineteen consecutive pictures, **zero differing pixels anywhere in the frame**.
The record claims thirteen; nineteen is what I measured, so the claim is
conservative and true.

The glyphs show it is the queue and not a coincidence
(`python3 $R/cells.py <picture> 2 1 29`, row 2 columns 1-29):

| Picture | Original | Native (this candidate) | Native at `5e1f51b` (review 4) |
| ---: | --- | --- | --- |
| 3213 | `.race..................0:33:5` | identical | identical |
| 3214 | `finish.................0:33:5` | identical | identical |
| 3215 | `finish.................0:33:5` | **identical** | `finish.......................` |
| 3216 | `finish.......................` | identical | player's and opponent's times already drawn |
| 3217-3219 | `finish.......................` | identical | opponent's time still early |

So the second `finish` write - the opponent's counter stepping on the update
after the player's laps reach zero, which re-sets `$0D17` while `$0EFB` is
already zero - now costs native the same update it costs the original, and the
clock, the player's time and the opponent's time each wait a picture behind it.
The 197 / 227+247 / 247 pixels review 4 measured at 3215-3217 are gone, and so
is the whole-picture residual in this window.

### 2. No regression

```sh
PYTHONPATH="$PWD" python3 $R/recapture.py "$E/m4-16-playable-zoom-zoo/m4-16/boundary-a" \
    $A/r5-orig-boundary 3200-3216,6475-6500          # 5,294 frames asserted, 43 kept
PYTHONPATH="$PWD" python3 $R/recapture.py "$E/m4-16-playable-zoom-zoo/m4-16/brake-a" \
    $A/r5-orig-brake 6480-6505                       # 5,299 frames asserted, 26 kept
PYTHONPATH="$PWD" python3 $R/recapture.py "$E/dragster-window-pause/window-pause/orig-banner-pause-a" \
    $A/r5-orig-banner 3408-3418                      # 2,260 frames asserted, 11 kept
python3 $R/render.py <same capture> $A/r5-native-<name> <scenario> <init frame> FIRST LAST
python3 $R/score.py $A/r5-native-<name> $A/r5-orig-<name> FIRST LAST
```

| Window | Pictures | hud-rows | was-bar | player band | opponent band | whole |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| DRAGSTER contended finish, `regression-landing-held-roll-a` 3206-3224 | 19 | **0** | **0** | **0** | **0** | **0** |
| ZOOM ZOO finish, `boundary-a` 6475-6500 | 26 | **0** | **0** | **0** | **0** | **0** |
| ZOOM ZOO loser race, `brake-a` 6480-6505 | 26 | **0** | **0** | **0** | **0** | 57 |
| DRAGSTER finish, `orig-banner-pause-a` 3408-3418 | 11 | **0** | **0** | **0** | **0** | 217 |
| ZOOM ZOO lap-2 change, `boundary-a` 3200-3216 | 17 | **0** | **0** | 1,736 | 1,488 | 3,224 |

The lap-change row reproduces review 4's numbers to the pixel (1,736 and
1,488); that residual is the declared signed split time, advisory 5 below. The
two whole-picture residuals are advisory 6.

The zeros in the finish-time bands are meaningful, not two blank bands agreeing:

```sh
python3 $R/cells.py $A/r5-orig-boundary/frame-6487.png 5 13 19   # '1:38:02'
python3 $R/cells.py $A/r5-orig-brake/frame-6490.png   20 13 19   # '1:38:10'
```

The player's time appears in picture 6487 and the opponent's in 6490 on the
primary, and the loser race draws the opponent's `1:38:10` at 6490 - native
matches each glyph, in the same picture, on both.

### 3. The kept-frame sweeps

```sh
M=$E/m4-16-playable-zoom-zoo/m4-16
python3 $R/hud_compare.py "$M/boundary-a"   "$E/m4-16-rider-art/m4-16-rider-art/original-primary"    classic.crawler.zoom-zoo 1376
python3 $R/hud_compare.py "$M/brake-a"      "$E/m4-16-rider-art/m4-16-rider-art/original-brake"      classic.crawler.zoom-zoo 1376
python3 $R/hud_compare.py "$M/trick-long-a" "$E/m4-16-rider-art/m4-16-rider-art/original-trick-long" classic.crawler.zoom-zoo 1376
python3 $R/hud_compare.py "$E/m4-16-rider-art/m4-16-idle/captures/stop-timeout-a" \
    "$E/m4-16-final-review/m4-16-final-review/orig/stop-timeout-a" classic.crawler.zoom-zoo 1376
python3 $R/hud_compare_manifest.py tests/manifests/replay/race-crawler-dragster-12000-continuous-right-fields.json \
    ../classic-race-hud/artifacts/classic-race-hud/orig-dragster/frames classic.crawler.dragster 1328
```

| Sweep | Frames | hud-rows | was-bar | whole |
| --- | ---: | ---: | ---: | ---: |
| M4-16 primary (`boundary-a`) | 274 | **0** | **0** | 18,601 |
| brake loser race | 274 | **0** | **0** | 18,601 |
| trick-long | 192 | **0** | **0** | 12,655 |
| 10:00 time-out | 35 | 45,056 | 42,240 | 630,928 |
| DRAGSTER manifest | 18 | 24,576 | 23,040 | 304,885 |

0 render failures in each. Every number is identical to review 4's table and to
the primary's `verify-round4.log`, including the nonzero totals of the last two
rows, which are the same in round 3, round 4 and here. I did not investigate
what those two carry; the point is that they did not move. Nothing regressed.

### 4. The tests bind

Each mutant was applied to `src/core/presentation.cpp` alone, rebuilt, run, and
the tree restored (`git status` clean, 23/23 afterwards). **I checked the build
status before trusting each result**, because round 4 produced a false
"survivor" when a dropped term left a variable unused and `-Werror` failed the
build while the stale binary passed.

| Mutation | Build | ctest |
| --- | --- | --- |
| the sprint finish must hold: `if(finished \|\| ...tour_race)` -> `if(finished && ...tour_race)` | `status=passed` | **fails at require #28** |
| the opponent's step must dirty the field: `if(stepped(0) \|\| stepped(1))` -> `if(stepped(0))` | `status=passed` | **fails at require #25** |
| the blank must consume an update: drop the `return` from the `pending_.clock_blank` branch | `status=passed` | **fails at require #22** |

Three distinct requires, three passing builds, all 96% / 1 of 23 failing. The
two `&&`/dropped-operand forms were chosen precisely so that every variable
stays used and the mutation cannot hide behind a compile failure.

This also answers review 4's should-fix 2. That round's complaint was that
deleting the finish half of the predicate still passed 23/23 because the
fixture's held and republished strings were both `0:33:7`. The sprint fixture's
digits are still equal either side of the finish (`0:33:5`), but the assertions
are no longer about the digits alone: `require(!still.clock_blanked)` and
`require(run(...).clock_blanked)` pin *which picture* the blank lands in, and
that is what kills mutant one. The check now binds the half it was added for.

### 5. The gates, cited rather than re-run

`git diff --stat 547fbfa d9c9485` is `docs/STATE.md`, `tasks/CLASSIC-RACE-HUD.md`,
`tasks/NEXT_SESSION.md` and `tasks/README.md` - records only, no source and no
tests. The primary's `artifacts/classic-race-hud/gates-547fbfa/` therefore
covers the candidate's code and tests exactly. Reading those logs:
23/23 ctest on each of the five presets; `synthetic.json` `status: passed`;
both v1 presentation contracts passed; 0 rider-pose fallback frames on both
hidden app runs; fuzz 40 seeds / 382,535 updates / 79 races / 0 aborts;
`gate_identity` "every input is byte identical and every report matches; 11
gates may be cited", restore counts unchanged. I did not re-run any of them
this round.

## Why the change is right where I did not measure it

Review 4 established, from the ROM and from a frame-by-frame scan of all 125
captures, that the original writes the left field on an update exactly when
`$0D17 != 0 && ($0EFB == 0 || $053F == 0)`. The candidate's new model is not a
different predicate - it is the same one, restated as a queue:

* old: `wrote = (tour_race && either counter stepped) || (the player's laps reached 0)`
* new: `pending_.left |= either counter stepped`, consumed when `finished || tour_race`

These agree on every update except one shape. On a tour race a step is consumed
the same update, as before. On a sprint mid-race the step leaves `pending_.left`
standing and the clock handler runs, as before. At the player's finish
`stepped(0)` is true and `finished` is true, so exactly one write, as before.
The only new write is a step on a sprint **after** the player has finished -
which is precisely the case review 4 returned the task for, and which the ROM
does write. Review 4's agreement over 125 captures therefore carries forward
rather than needing a re-run, and the fix is a strict repair of the one
disagreement rather than a new rule.

The generalisation review 4 asked for is also there: it warned that a ZOOM ZOO
race in which the opponent crosses **any** line one update after the player
finishes would slide the same way, and nothing excluded it. The new model
consumes that update on a tour race too, so it is covered on both tracks rather
than only on the measured one.

## Findings

### Should-fix 1 - `R-0043`'s new write count is wrong again, and this time it contradicts itself

`docs/research/R-0043-classic-race-hud.md:198-201` now reads:

> A tour race therefore writes the left field seven or eight times, and a sprint
> twice - once at its first crossing, which is not held, and once at the finish.

A write that "is not held" is a contradiction in this record's own terms: an
update that writes the left field is exactly the update the queue spends, which
is what "held" means throughout. And the same record says four lines earlier
that on a sprint the flag "stands set for the rest of the race" from the first
crossing "while the clock goes on being republished every update" - i.e. the
crossing does **not** write. It is also the premise of the whole narrowed
predicate that a sprint's crossings are not writes; the candidate's own code
comment says so, and mutant two of section 4 exists to defend it.

A sprint writes the field **once, at the finish** - and **twice in the six
captures** where the opponent's counter steps on the very next update, which is
the second `finish` write this round was returned to fix. That is the number a
reader needs, and it is the number the task record's row 26 gets right. Review 4
called this "the third wrong count in this one sentence's history"; the
correction is the fourth. It is one line and the record is open.

The "seven or eight" for a tour race matches review 4's scan and the primary's
`queue_probe.py`, and is right.

### Should-fix 2 - the task record claims a fix that was only half applied

`tasks/CLASSIC-RACE-HUD.md` row 27 says "the comments name `$81:EB91`/`$81:EB93`
correctly". Only `src/core/presentation.cpp` was corrected; its new comment
("`$81:EB93` reads `$053F` and branches to `$81:EB9B`, whose `JMP` enters the
clock handler") is accurate and is the best statement of it in the tree. But
review 4's advisory 4 named three places, and two still carry the loose wording:

* `src/core/presentation.hpp:195` - "when `$053F` is set, $81:EB93 falls
  through at $81:EB9B to the clock". `$81:EB93` is `LDA $053F` and `$81:EB96`
  is a `BNE` to `$81:EB9B`, which is itself a `JMP $ECC2`. Nothing falls
  through.
* `docs/research/R-0043-classic-race-hud.md:142` - the same sentence.
* `src/core/presentation.hpp:189` also still says "`$81:EC5E` and `$81:EB98`
  both end at `$81:ECBC`"; `$81:EB98` is `JMP $EC61`, and it is the path from
  there that ends at `$81:ECBC`.

The mechanism described is right in each case; only the wording would mislead
someone checking it against a disassembler, and an accepted record claiming the
fix is applied is worse than the wording. Either finish the edit or soften the
row. This is a should-fix rather than a return because it moves no pixel.

### Advisory 3 - the stale paragraph above the new one in `presentation.hpp`

`presentation.hpp:187-199` is the previous round's description of the
written/not-written predicate, left in place immediately above the new
queue paragraph at 200-210. Both are true and they do not conflict, but the old
one is now the *derivation* of a special case of the new one, and it is the one
carrying the loose citations of should-fix 2. Folding it into the queue
paragraph would leave one statement of the mechanism instead of two, and the
queue paragraph is the better of them.

### Advisory 4 - the no-history fallback now falls back further than its comment says

`classic_race_presentation_runner.cpp:123-131` tells the reader that without a
previous state "the clock digits are derived from the state rather than
followed". After this round the fallback branch of `classic_race_hud_text` also
places the three finish fields at the old fixed offsets, which review 4 proved
wrong in the contended case. `presentation.hpp:177-181` does declare this, and
the runner's comment correctly says "this debug form is the only caller without
the history" - `src/app/frontend.cpp` and the `--timeline` form both keep the
tracker, and every picture in this report was rendered through `--timeline`. So
nothing shipping uses it. Worth one clause at the debug form so the two
comments agree.

### Advisory 5 - the declared omissions, unchanged

Both are open by declaration and neither is a return:

* **The original's signed split time**, which occupies the two centred cells
  during the race and which this task draws only at the finish. Measured on
  `boundary-a` 3200-3216: 1,736 pixels in the player's band and 1,488 in the
  opponent's, identical to review 4.
* **The off-screen rider arrow**, 36 pixels a frame in the sweeps from picture
  1620 onward.

`docs/STATE.md` names both in the same terms, so the record and the measurement
agree.

### Advisory 6 - two whole-picture residuals I located but did not attribute

`brake-a` picture 6495 differs by 57 pixels at x 124-134, y 83-93, and
`orig-banner-pause-a` picture 3414 by 217 pixels at x 97-134, y 83-93. Both are
in the rider band and nowhere near the three row pairs `draw_bg3_text` writes
(y 16-31, 40-55, 160-175), and `draw_classic_hud` marks `inked` only inside
those rows, so this change cannot produce them; they are the pre-existing rider
residual the sweeps' whole-picture totals already carry. I did **not** confirm
that by building the predecessor and re-rendering those two pictures, so I am
recording it as an observation rather than as "unchanged".

### Advisory 7 - what could still return this, and where it is not measured

The three things the fourth round confirmed from the ROM - the only writer of
`$0D17`, the `$053F` branch, and the predicate - are settled. This round's
*new* claims are the dispatcher's **order** (left field, then `$034D` the
clock, then `$0349` the player's time, then the opponent's) and "`$034D` is
positive while the digits the timer keeps differ from the ones the cells hold".
The order is measured in the uncontended ZOOM ZOO race (R-0043's 6485-6490
table, which I reproduced at 6487 and 6490) and in the contended DRAGSTER race
(3215-3219, above), and the addresses are cited, but I did not re-derive it
from the ROM - the dispatch scoped that out.

The case that would test it hardest is one I could not find measured anywhere:
a race where **the tenth ticks on the update right after the opponent
finishes**. The model says the clock takes that update and the opponent's time
waits a picture; if the original's dispatcher orders `$0349`/the opponent's
writer ahead of `$034D`, it would not. On the two finishes I measured the tick
did not fall there. If this task is ever reopened, that is the first place to
look, and it is cheap to scan for.

### What I checked and found sound

* No baseline, threshold or expected result is weakened in `5e1f51b..d9c9485`.
  The `require`s the test diff removes are the old delay-based assertions the
  queue model replaces, and the block ends with more of them than it started
  with; three of them now kill mutations that the previous shape did not.
* No undefined or unchecked arithmetic in the new code: `classic_hud_clock`
  reduces every field modulo 10, the `total_times[...] < 60000U` sentinel guard
  is preserved on both finish times, and the queue holds only booleans and a
  `std::optional<std::string>`.
* `reset()` clears `latest_`, `on_screen_` **and** the new `pending_` set, so a
  restart cannot carry a pending field into the next race; `sdl_main.cpp` and
  `dragster_fuzz_runner.cpp` both replace `LivePresentation` wholesale on a
  backwards frame, and the gate run's 1,242 fuzz pause-restarts are clean.
* `ClassicHudPublished` is compared by a defaulted `operator==`, and
  `published()` still lags exactly one update, which the test's `prime` helper
  asserts explicitly rather than assuming.
* `git status` is clean at `d9c9485` with the mutants restored, and
  `local/v1-fixtures` is untouched - I never ran `native presentation-check`
  this round, so I never needed to copy it.

## What I did not check - this round was scoped

The dispatch allowed roughly one review's budget and told me not to re-derive a
mechanism four rounds had already established. The following are therefore
**unverified by me in this round**, and an approval that rests on them should
say so:

- **The ROM.** I did not read `$81:EB86`-`$81:EC5E`, `$81:ECBC`, `$81:818D` or
  `$81:F357`. Review 4 did, and I relied on it. In particular the dispatcher
  **order** the new model encodes is cited from the record, not re-derived.
- **The 125-capture predicate scan.** Not re-run. I argued the agreement
  forward from review 4 structurally, in "Why the change is right where I did
  not measure it"; that is an argument about the code, not a measurement.
- **The gate suites.** The five-preset build matrix, the synthetic suite, both
  v1 presentation contracts, the two hidden app runs, the 40-seed fuzz and the
  eleven differential compares are all **cited** from
  `artifacts/classic-race-hud/gates-547fbfa/` and `verify-round4.log`, as the
  dispatch permitted. I verified only that the gate commit covers the
  candidate's code and tests, by diffing `547fbfa..d9c9485`.
- **Hosted CI on this tip**, which the acceptance table requires at
  integration. GCC `-Werror` has caught what macOS Clang did not before, and
  `codex/*` branches skip hosted CI.
- **A build of the predecessor.** Every "unchanged" in this report is against
  review 4's published numbers and the primary's logs, not against a
  predecessor binary I built myself. The two residuals in advisory 6 are the
  place that matters.
- **The DRAGSTER manifest sweep's originals**, which are the primary's
  `orig-dragster/frames`; I did not recapture those 18. I recaptured 99
  pictures of my own across four captures (19 + 43 + 26 + 11), 14,919 frames
  asserted against frozen digests in total.
- **`docs/STATE.md`'s claim** that "DRAGSTER 1400-3453 and its whole finish
  transition are pixel-identical over the whole picture" - I measured 3206-3224
  of one DRAGSTER race at whole-picture 0 and 3408-3418 of another at 217, so
  the claim is race-specific and I did not reproduce the range it names.
- **Live play, the paused pictures** where the original dims the screen, the
  countdown, the restart path and the time-out hold - all measured by earlier
  rounds, none re-measured here.
- **Two-player, the pre-race and result screens, and the lap field's two-digit
  forms**, declared out of scope by R-0043.
