# CLASSIC-RACE-HUD - independent re-review of `428d1d3`

**Verdict: return.** Both of the first review's blocking findings are genuinely
fixed, and I confirmed each of them against consecutive original pictures I
recaptured myself rather than against the primary's sets. But the rule the fix
rests on is still narrower than the original's, and the difference is visible:
on an ordinary mid-race frame of a capture the task never used, the original
holds the corner clock and the candidate ticks it. That is the same defect the
first review returned as its blocking 2 - moved, not removed.

Re-reviewer: fresh Claude Opus 5 session, no inherited conversation, isolated
worktree `.worktrees/classic-race-hud-review` detached at `428d1d3`.
Artifacts: `artifacts/classic-race-hud-review2/` in that worktree (ignored).
Date: 20 September 2026.

I read the first review (`8671cfc` on `review/classic-race-hud`) before starting
and treated every number in it, and in the task record, as a claim.

## What I reproduced, with the exact commands

Environment, from the re-review worktree with
`export PATH="$PWD/local/toolchain/ninja-1.13.2-darwin-arm64:$PATH"`.
`E` is the main checkout's `local/evidence`, `A` is
`artifacts/classic-race-hud-review2`.

```sh
python3 tools/project.py build --preset app-debug          # status=passed
ctest --test-dir build/app-debug --output-on-failure       # 23/23 passed
```

### The four kept-frame sweeps and the DRAGSTER manifest sweep

I ran the primary's own sweep scripts, copied unchanged into my artifacts
directory, on the candidate's build.

```sh
python3 $A/hud_compare.py "$E/m4-16-playable-zoom-zoo/m4-16/boundary-a" \
    "$E/m4-16-rider-art/m4-16-rider-art/original-primary" classic.crawler.zoom-zoo 1376
python3 $A/hud_compare.py "$E/m4-16-playable-zoom-zoo/m4-16/brake-a" \
    "$E/m4-16-rider-art/m4-16-rider-art/original-brake" classic.crawler.zoom-zoo 1376
python3 $A/hud_compare.py "$E/m4-16-playable-zoom-zoo/m4-16/trick-long-a" \
    "$E/m4-16-rider-art/m4-16-rider-art/original-trick-long" classic.crawler.zoom-zoo 1376
python3 $A/hud_compare.py "$E/m4-16-rider-art/m4-16-idle/captures/stop-timeout-a" \
    "$E/m4-16-final-review/m4-16-final-review/orig/stop-timeout-a" classic.crawler.zoom-zoo 1376
python3 $A/hud_compare_manifest.py \
    tests/manifests/replay/race-crawler-dragster-12000-continuous-right-fields.json \
    <the primary's orig-dragster/frames> classic.crawler.dragster 1328
```

| Sweep | Frames | HUD rows | Rows the bar covered | Whole | My log |
| --- | ---: | ---: | ---: | ---: | --- |
| M4-16 primary (`boundary-a`) | 274 | **0** | **0** | 18,601 | `sweep-primary.txt` |
| brake loser race | 274 | **0** | **0** | 18,601 | `sweep-brake.txt` |
| trick-long | 192 | **0** | **0** | 12,655 | `sweep-tricklong.txt` |
| 10:00 time-out | 35 | 45,056 | 42,240 | 630,928 | `sweep-timeout.txt` |
| DRAGSTER manifest | 18 | 24,576 | 23,040 | 304,885 | `sweep-dragster.txt` |

Every one of these reproduces R-0043's measurement table exactly. The last two
rows are the arithmetic the record spells out and I checked on the face of my
own logs: the time-out's non-zero frames are exactly the 11 from 31933 (4,096
and 3,840 each, the whole of both boxes) and its 24 hold frames are 0; the
DRAGSTER sweep's non-zero frames are exactly the 6 from 3454 and its 12 race
frames are 0. The candidate's claims about these sweeps are accurate.

### The gates, re-run rather than cited

```sh
python3 tools/project.py test --suite synthetic --preset app-debug \
    --artifacts $A/synthetic-artifacts --report $A/synthetic.json   # status=passed, 61.4s
python3 tools/project.py native presentation-check --manifest tests/manifests/presentation/classic-crawler-dragster-v1.json \
    --fixtures local/v1-fixtures-review/winner-fixtures --content-pack local/classic-crawler-dragster.pack \
    --preset lab-debug ...                                          # winner status=passed
#   ... and the loser manifest                                      # loser  status=passed
python3 -m tools.unirally_lab.native.gate_identity --since 6e0fad6 \
    --reports "$E/classic-stunt-names-review4/classic-stunt-names-review4/gates-6e0fad6" --expect 11 \
    --pack local/classic-pal-crawler-two-tracks-v9.pack \
    --ninja local/toolchain/ninja-1.13.2-darwin-arm64/ninja
```

`gate_identity` reports "comparing 6e0fad6 with 428d1d3 ... 9 objects, 19
repository files build src/core/zoom_zoo_runner ... every input is byte
identical and every report matches; 11 gates may be cited", with every restore
count unchanged. I agree the citation is sound for this change: the tool proves
the claim from ninja's own dependency list rather than asserting it, and this
change touches no symbol the differential runner links. I ran it on the
candidate itself, which the primary's own gate run did not (see should-fix 5).

### The first review's two blocking findings, on originals I recaptured myself

`recapture.py` replays a frozen capture's own timeline on the audited core and
asserts every replayed frame's video and WRAM digests against the frozen
`reference.json`, so the extra pictures are the frozen run's pictures. I copied
it from the primary's worktree byte for byte (it is identical to the first
review's copy) and read it before using it.

```sh
PYTHONPATH="$PWD" python3 $A/recapture.py "$E/m4-16-playable-zoom-zoo/m4-16/boundary-a" \
    $A/orig-primary-finish 6475-6500                     # 5294 frames checked, 26 kept
PYTHONPATH="$PWD" python3 $A/recapture.py "$E/m4-16-playable-zoom-zoo/m4-16/continued-controls/compound-reverse-a" \
    $A/orig-cr 3205-3222,6470-6500                       # 5294 frames checked, 49 kept
PYTHONPATH="$PWD" python3 $A/recapture.py "$E/m4-16-playable-zoom-zoo/m4-16/brake-a" \
    $A/orig-brake-finish 6480-6505                       # 5299 frames checked, 26 kept
python3 $A/render.py CAPTURE OUT classic.crawler.zoom-zoo 1376 FIRST LAST
python3 $A/score.py OUT ORIG FIRST LAST
```

| Window | Frames | HUD rows | Player band | Opponent band | Whole |
| --- | ---: | ---: | ---: | ---: | ---: |
| primary 6475-6500 (finish, opponent at 6488) | 26 | **0** | **0** | **0** | **0** |
| compound-reverse 6470-6500 (finish at 6477) | 31 | **0** | **0** | **0** | 55 |
| brake 6480-6505 (**opponent** finishes first, 6488; player 6492) | 26 | **0** | **0** | **0** | 57 |
| compound-reverse 3205-3222 (the lap change at 3212) | 18 | **0** | 2,000 | 3,224 | 5,260 |

So blocking 1 is fixed and the three gates are right rather than merely shifted:
the blanked clock, the player's time and the opponent's time now land on the
original's pictures in three different races, including the loser race where the
opponent's time is published four updates before the player finishes and the
queue interleaves differently. Blocking 2 is fixed on the case that found it:
compound-reverse picture 6478 and the lap change at 3213 are both 0. The
residual in the two centred bands at 3205-3222 is the declared split time, and
the 55/57 whole-picture pixels are one frame of channel-6 window shape
(advisory 9).

### DRAGSTER's own finish transition, which the first review could not check

I captured my own DRAGSTER originals from the accepted manifest rather than
using the primary's:

```sh
python3 tools/project.py access capture \
    --manifest tests/manifests/replay/race-crawler-dragster-12000-continuous-right-fields.json \
    --out $A/orig-dragster-finish --from-frame 3436 --to-frame 3456 \
    --wram-series-range 0x0340 0x0C00 --wram-series-every 1 --frame-image 3436 ... --frame-image 3456
python3 $A/hud_compare_manifest.py <that manifest> $A/orig-dragster-finish/frames classic.crawler.dragster 1328
```

Frames **3436-3453 are pixel-identical over the whole picture**, eighteen
consecutive frames across `race` becoming `finish` and the clock blanking. From
3454 the whole picture differs, which is the declared screen-off for result
loading. DRAGSTER's finish is now measured end to end.

### The new unit tests bite

I mutated each corrected gate back to its pre-review value and rebuilt:

| Mutation | Result |
| --- | --- |
| `race.finish_delay<1` back to `<2` | `presentation_gather_mapping_and_determinism` FAILS |
| `race.finish_delay>=2` back to `>=3` | FAILS |
| `*opponent+1U` back to `*opponent+2U` | FAILS |
| `ClassicRaceHudClock` publishes on every update (no hold) | FAILS |

Log `mutations.txt`. The tree was restored and rebuilt afterwards; `git status`
is clean. The tests are no longer the kind that passed through the first
review's off-by-one.

## My independent case: the queue model, tested against the captures' own WRAM

The interesting question the correction opens is not whether the three failing
frames now pass, but whether "the left field goes first and spends the update"
is implemented as the original's rule. The implementation's predicate is that
the left field's **text** changes:

```cpp
if(classic_hud_left_field(updated,scenario).text==classic_hud_left_field(previous,scenario).text)
    latest_=classic_hud_clock(updated.movement.timer);
```

The original's rule, in R-0043's own reading of `$81:EB86`, is the dirty flag
`$0D17`. I tested the two against each other over whole races using nothing but
the frozen captures' own per-frame WRAM (`$0D17`, the clock's `$034D`, the four
digit tiles `$0E2F/$0E33/$0E37/$0E3B`, the player's laps `$0EFB` and the byte
two along, `$0EFD`). `clock_model.py` simulates both and prints every picture on
which they choose a different update's digits, and whether the digits differ.

Calibration first, from R-0043's own finish table: `$0D17` is sampled set at the
end of update 6484 and the left field is written on update 6485, which shows in
picture 6485. So a flag set at the end of update N-1 spends update N and the
clock cells keep what update N-1 wrote. The refinement for DRAGSTER is R-0043's
own: with `$053F` set the field "is left alone", and indeed `$0D17` is set on
every DRAGSTER race frame while the clock plainly keeps running, so that branch
does not spend the update.

```sh
python3 $A/clock_model.py   # run over all 51 M4-16 ZOOM ZOO captures
python3 $A/queue_scan.py CAPTURE FIRST LAST OUT.json
```

The result (`clock-model-zoomzoo.json`, `dirty-cause.txt`): in **every one** of
the 51 captures, `$0D17` is set three or four times on updates where the
player's own lap field does not change at all. What sets it is the **opponent's**
lap counter `$0EFD`, whose steps are exactly those frames - typically 1598,
3208, 4846 and the opponent's finish at 6488 - and across all 51 captures there
is not a single set of `$0D17` that is not one of the two riders' lap counters
stepping. The candidate's text predicate cannot see any of the opponent's.

On the four races the task and the first review measured, that is invisible by
luck: the tenth does not happen to tick on those updates. On three other races
in the same accepted set it does.

## Findings

### Blocking 1 - the clock still ticks on updates the original spends on the left field

The opponent's lap crossings and its finish dirty the left field. The original
then redraws the player's unchanged lap text and the clock cells stand for
another picture; the candidate republishes them.

Reproduced on `ordinary-controls/down-a`, an accepted M4-16 capture this task
never used, where the player crosses on update 3207 and the **opponent** on
3208:

```sh
PYTHONPATH="$PWD" python3 $A/recapture.py \
    "$E/m4-16-playable-zoom-zoo/m4-16/ordinary-controls/down-a" $A/orig-down-3200 3200-3216
python3 $A/render.py "$E/m4-16-playable-zoom-zoo/m4-16/ordinary-controls/down-a" \
    $A/native-down-3200 classic.crawler.zoom-zoo 1376 3206 3211
python3 $A/cells.py $A/orig-down-3200/frame-3209.png   2 24 29    # '0:32:4'
python3 $A/cells.py $A/native-down-3200/frame-3209.ppm 2 24 29    # '0:32:5'
python3 $A/score.py $A/native-down-3200 $A/orig-down-3200 3206 3211
```

```
frame   3206: hud-rows 0   ...  whole 0
frame   3207: hud-rows 0   ...  whole 0
frame   3208: hud-rows 0   ...  whole 0
frame   3209: hud-rows 33  was-bar 0  player-time 0  opponent-time 0  whole 33
frame   3210: hud-rows 0   ...  whole 0
```

The 33 pixels are x 233-238, y 19-29: tilemap column 29 of row 2, the tenths
digit, and nothing else. The left field is `2/3` in both pictures on every frame
of the window, so this is the clock alone. Decoded cell by cell, the original
reads `0:32:4` on pictures 3206, 3207, 3208 **and 3209** and `0:32:5` from 3210;
the candidate reads `0:32:5` from 3209.

The capture's own WRAM shows the mechanism directly, with the two flags R-0043
names:

```
update 3203-3206: $0E3B tile 4  $0D17=0  $034D=0   player laps 3  opponent laps 3
update 3207:      $0E3B tile 5  $0D17=1  $034D=1   player laps 2  opponent laps 3
update 3208:      $0E3B tile 5  $0D17=1  $034D=1   player laps 2  opponent laps 2
update 3209:      $0E3B tile 5  $0D17=0  $034D=1   player laps 2  opponent laps 2
update 3210:      $0E3B tile 5  $0D17=0  $034D=0   player laps 2  opponent laps 2
```

The clock's own dirty flag stays pending across updates 3207, 3208 and 3209 and
is only cleared on 3210, because the left field took the queue twice: once for
the player's crossing and once for the opponent's. The candidate holds for the
first and not the second.

The same disagreement is predicted and visible on `held-controls/select-a`,
`ordinary-controls/up-a` and their `-b` twins, all at picture 3209, and is
present but invisible (the tenth does not tick) at pictures 1599, 4847 and 6489
of every other ZOOM ZOO capture in the set, including `boundary-a`, `brake-a`,
`trick-long-a` and `compound-reverse-a`. In the primary the opponent's second
crossing happens to fall on the same update as the player's, which is why
neither the task's frame set nor the first review's recaptures could see it.

This is the first review's blocking 2 with a different cause for the same
staleness, so I am classifying it the same way. It also makes the recovered rule
itself wrong where the records state it (should-fix 2).

The fix does not need new state. Both riders' lap counters are already in the
published 742-byte record: I matched serialized offsets 423 and 445 against
`$0EFB` and `$0EFD` frame by frame on `down-a`, including the 3208 step. A
predicate of "either rider's `laps_remaining` changed" is exactly equivalent to
`$0D17` on all 51 captures I scanned (`dirty-cause.txt`: 0 unexplained sets),
and it covers the player's finish and the opponent's finish, which are just the
last step of each counter. The existing unit tests would still pass under it -
they only move the player's counter - so the fix is additive to them, and a case
that moves the opponent's counter should be added beside them.

### Should-fix 2 - the records state the queue rule more narrowly than the original's

R-0043: "an update that changes the lap count or the finish word spends that
update", and "`ClassicRaceHudClock` ... publishes the digits on every observed
update except one that changes the left field". `presentation.hpp`: "An update
that changes it is an update on which the clock is not republished". The
original's condition is `$0D17`, which the same record correctly identifies two
paragraphs earlier as "the dirty flag", and which the opponent sets three or
four times a race without changing anything the player's field shows. The
narrowing is what produced blocking 1, so this is not only prose: it is the
recovered mechanism stated as something it is not, which AGENTS asks the project
not to do. State what sets `$0D17`, and say that this evidence is the two lap
counters.

### Should-fix 3 - `queue_probe.py` cannot find the case it is cited for

The record's attempt 18 and R-0043 both rest on it: "Searching the captures' own
WRAM for updates that change both `$0E3B` and `$0EFB` finds exactly three". That
is true of the question the probe asks, but the probe reads `$0D17` into its row
and then never uses it, and it searches only for the player's counter. By
construction it could not have found the opponent-driven updates, which are the
majority of the cases. Cited as the evidence for a general rule, it is the same
shape of hole as the 20-frame picture set: a search whose domain excludes the
counter-example. `clock_model.py` and `dirty-cause.txt` in my artifacts are a
drop-in replacement for the question that actually matters.

### Should-fix 4 - the committed gate log does not show the synthetic result it is cited for

`artifacts/classic-race-hud/gates-run-b716bd9.log` has a blank line under
`== synthetic suite (app-debug) ==`, while attempt 19 records "synthetic
`status=passed` (60.8 s)". The number is real - `gates-b716bd9/synthetic.json`
says `status: passed, elapsed_seconds: 60.774`, 459 checks - and I reproduced
`status=passed` myself on the candidate. But the retained run log, which is what
a later reader opens, shows nothing there. Worse, the repair in the current
`gates.sh` is `sync; echo "  rc=$? ..."`, so the `rc` it now prints is `sync`'s
exit status and never the suite's. Re-run the script so the log matches it, or
have the record cite `synthetic.json` explicitly.

### Should-fix 5 - the gates were run on `b716bd9`, not on the candidate

`gates-b716bd9` and its `gate_identity` line ("comparing 6e0fad6 with b716bd9")
are one commit behind the candidate the review was dispatched on. The delta is
one line of `tasks/CLASSIC-RACE-HUD.md`, so nothing is actually at risk, and I
re-ran the synthetic suite, both v1 contracts and `gate_identity` on `428d1d3`
myself with the same results. The record should say which commit the gate
artifacts belong to and why that is sufficient, rather than leaving the reader
to diff it.

### Should-fix 6 - the history fallback's claim is wrong by the same margin

`presentation.hpp` promises that without a `ClassicRaceHistory` the digits are
"exact except on the one picture after an update that redrew the left field".
Two problems. It is one picture *per such update*, which is four or five a race,
not one; and by blocking 1 the fallback is also wrong on the pictures after the
opponent's crossings, which the same sentence does not admit. Nothing in the
suite or the contracts uses the single-state path - `frontend.cpp` and the
`--timeline` runner always pass the history, and `render_classic_race(state,
content, &previous)` is only reachable from the documented three-argument
runner - so this is a documentation defect and a developer-tool inaccuracy, not
a gate risk. It should be reworded once blocking 1 is fixed.

### Advisory 7 - `hud_compare_manifest.py` does not count render failures

`hud_compare.py` counts them and prints "N frames compared, M render failures".
`hud_compare_manifest.py` prints the failure line for the frame and then simply
omits it from the totals and from the compared count, so a systematic render
failure on the DRAGSTER sweep would read as a smaller, cleaner run. Nothing was
dropped in any run I made (18 of 18 and 21 of 21), but the DRAGSTER row of
R-0043's table is produced by the tool that cannot report the skip.

### Advisory 8 - the no-time sentinel is still guarded on one side only

Unchanged from the first review's advisory 8: `hud_time(60000)` prints
`0:00:00`, the opponent's time is guarded by `total_times[1]<60000U` and the
player's is not. I could not reach it either - `finished` requires
`laps_remaining==0`, which is a real crossing - so it is still an asymmetry
rather than a defect. A comment or a symmetric guard closes it.

### Advisory 9 - a one-picture channel-6 window difference at the finish

On two of the three finish windows I recaptured, exactly one frame differs
outside the four boxes: compound-reverse 6480 (55 pixels) and brake 6495 (57).
In both, the original shows the flat window colour `0a2626` in an 11-by-11 area
around x 124-134, y 83-93 and native shows caption ink `e70000` there, so
native's channel-6 member is one step smaller than the original's for that
picture and leaves the caption glyphs exposed. This is BG3 rows 10-11, the
caption, not the HUD, and it is `ClassicWindowPointer`/R-0040 rather than
anything this task wrote - but I did not build the base commit to confirm it
predates the change, and the 20-frame picture set never samples it. Worth a
record somewhere, in this task or the window one.

### Advisory 10 - claims that are still broader than the logs

Two of the first review's should-fixes are now correctly scoped in R-0043 and
the task record, and I checked the arithmetic against my own logs. Two things
it asked for are still open: the split-time residual is quantified in pixels but
not in frequency (64 of the 274 primary frames, 33 of the 192 trick-long ones),
and nothing names the paused frames, where the original dims the whole picture
and native does not, as an exclusion from "where it draws". Both were advisory
before and remain so.

### Advisory 11 - owned records and process nits

`docs/STATE.md` is an owned path and still has no CLASSIC-RACE-HUD paragraph and
no link to R-0043; the registry row is there. I expect this at integration. Also:
`gates.sh` does `rm -rf local/v1-fixtures && mkdir -p local/v1-fixtures` inside
the primary's worktree, where `local/v1-fixtures` was a symlink into the main
checkout's `local/`; it is now a real directory there. It removed only the link,
so nothing was lost, but a script that rewrites the shared `local/` layout is
one character away from deleting the fixtures it copies.

## Readability

The correction reads better than what it replaced. Pulling `classic_hud_left_field`
and `classic_hud_clock` out names the two things the queue orders, and I checked
the extraction is behaviour-preserving against the old inline form. The comment
on `classic_race_hud_text` now derives each gate from a picture number and cites
the frames, which is exactly what the first review asked for and is the part of
this change I would most want a later reader to find. `ClassicRaceHudClock`'s
own header explains the one-update lag against the rider overlays clearly.

One readability note that is really blocking 1 in another form: the comment
inside `observe_update` says "an update that changes it spends the queue", and a
reader will take that as the recovered rule. It is the implemented approximation
to it.

## What I did not check

- Hosted CI on this tip. Not run; the acceptance table requires it at
  integration and GCC `-Werror` has caught what macOS Clang did not before.
- The five-preset build matrix, the two hidden app runs and the 40-seed fuzz. I
  relied on the primary's `gates-b716bd9` run for those, having re-run
  app-debug's build and ctest, the synthetic suite, both v1 contracts and
  `gate_identity` on the candidate itself.
- The eleven differential compares, which are cited rather than re-run. I
  re-ran the citation.
- Whether advisory 9's window difference predates this change; that needs a
  build of `92f46ba`.
- That `$77:074B` is the race mode, at the ROM level. Like the first reviewer I
  confirmed only the behaviour it selects.
- Live play, two-player, the pre-race and result writers, and the rest of the
  `$81:CFF6` family, all declared out of scope.
- The DRAGSTER time-out and the DRAGSTER opponent, beyond the flag scan: my
  model run over `dragster-clock-limit/idle-a` and two
  `dragster-ordinary-controls` originals found no visible disagreement, but I
  produced consecutive DRAGSTER pictures only for 3436-3456.

## What I would need to approve

Blocking 1 fixed - the clock held on every update the original's left field is
redrawn, not only those that change its text - and measured on consecutive
originals of a race where the opponent's crossing and a tenth tick coincide.
`down-a` 3206-3211 is the cheapest such case and costs one `recapture.py` run.
Should-fix 2 and 3 should ride with it, because they are the reason the
counter-example was not found; 4, 5 and 6 are record and evidence corrections.
Everything else I measured on this candidate holds, and I would not ask for any
of it to be re-done.
