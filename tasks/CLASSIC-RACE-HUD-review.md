# CLASSIC-RACE-HUD - independent review of `82f2d16`

**Verdict: return.** Two reproducible picture defects at the finish, both on
ordinary frames the original publishes and both invisible to the frame set the
task measured. Everything else I tested holds up, including the recovery itself,
which I re-derived from the ROM and from the original's pixels rather than
reading it back from the record.

Reviewer: fresh Claude Opus 5 session, no inherited conversation, isolated
worktree `.worktrees/classic-race-hud-review` detached at `82f2d16`.
Artifacts: `artifacts/classic-race-hud-review/` in that worktree (ignored).
Date: 20 September 2026.

## What I reproduced, with the exact commands

Environment, from the review worktree with
`export PATH="$PWD/local/toolchain/ninja-1.13.2-darwin-arm64:$PATH"`:

```sh
python3 tools/project.py build --preset app-debug      # status=passed
ctest --test-dir build/app-debug --output-on-failure   # 23/23 passed
```

I wrote my own sweep, `artifacts/classic-race-hud-review/hud_review_check.py`,
rather than run the primary's. It rebuilds the native timeline from a frozen
capture's own recorded input timeline, renders every frame the capture kept a
picture for, and scores the HUD rows (y 15-30), the rows the removed authored
bar covered (y 0-14) and the two centred finish-time bands. A render failure is
counted as a failure rather than dropped from the totals.

```sh
E=/Users/markfeaver/Projects/Unirally\ Decompilation/local/evidence
A=artifacts/classic-race-hud-review
python3 $A/hud_review_check.py "$E/m4-16-playable-zoom-zoo/m4-16/boundary-a" \
    "$E/m4-16-rider-art/m4-16-rider-art/original-primary" classic.crawler.zoom-zoo 1376
python3 $A/hud_review_check.py "$E/m4-16-playable-zoom-zoo/m4-16/brake-a" \
    "$E/m4-16-rider-art/m4-16-rider-art/original-brake" classic.crawler.zoom-zoo 1376
python3 $A/hud_review_check.py "$E/m4-16-rider-art/m4-16-idle/captures/stop-timeout-a" \
    "$E/m4-16-final-review/m4-16-final-review/orig/stop-timeout-a" classic.crawler.zoom-zoo 1376
```

| Sweep | Frames | HUD rows | Rows the bar covered | My log |
| --- | ---: | ---: | ---: | --- |
| M4-16 primary (`boundary-a`) | 274 | **0** | **0** | `review-primary.txt` |
| brake loser race (`brake-a`) | 274 | **0** | **0** | `review-brake.txt` |
| 10:00 time-out (`stop-timeout-a`) | 35 | 45,056 | 42,240 | `review-timeout.txt` |

The first two reproduce the claim exactly: my `review-primary.txt` frame rows are
byte-identical to the primary's `compare-primary-full.txt`, and my time-out rows
to its `compare-timeout.txt`.

Two things about those numbers that the records state loosely (see should-fix 4):

- The time-out sweep is 0 in every band on the 24 frames from 31578 to 31700 and
  differs over the **whole picture** (57,344 pixels, every pixel) on the 11
  frames from 31933, where the original has blanked the screen for result
  loading and native has not. The same is true of 6 of the 18 DRAGSTER frames
  (3454-3460): the primary's own `compare-dragster.txt` totals 24,576 differing
  HUD-row pixels over its 18 frames. R-0043's table is careful to say "12 race
  frames"; the task record's attempt 11 and the capability checkpoint are not.
- The primary's `compare-primary-full.txt` and `compare-brake-full.txt` are
  byte-identical files, which looks at first like one run recorded twice. It is
  not: I checked that `original-primary` and `original-brake` differ on 17
  pictures (`diff -rq`, frames 6420-6720) and re-ran the brake sweep against the
  brake capture and the brake pictures myself. Native reproduces the loser race
  pixel-exactly over exactly those frames, so both logs are all-zero there and
  the identity is real. Recording this so a later reader does not re-derive the
  suspicion.

I also re-ran the checks a presentation change is supposed to answer for:

```sh
python3 tools/project.py test --suite synthetic --preset app-debug \
    --artifacts $A/synthetic-artifacts --report $A/synthetic.json        # status=passed, 65.1s
python3 tools/project.py native presentation-check --manifest tests/manifests/presentation/classic-crawler-dragster-v1.json \
    --fixtures local/v1-fixtures-review/winner-fixtures --content-pack local/classic-crawler-dragster.pack \
    --preset lab-debug ...                                               # winner status=passed
#   ... and the loser manifest                                           # loser  status=passed
python3 -m tools.unirally_lab.native.gate_identity --since 6e0fad6 \
    --reports "$E/classic-stunt-names-review4/classic-stunt-names-review4/gates-6e0fad6" --expect 11 \
    --pack local/classic-pal-crawler-two-tracks-v9.pack \
    --ninja local/toolchain/ninja-1.13.2-darwin-arm64/ninja
```

On the gate citation: I re-ran `gate_identity` in my own checkout at the
candidate and it reports "9 objects, 19 repository files build
src/core/zoom_zoo_runner ... every input is byte identical and every report may
be cited", with the eleven gates' restore counts unchanged. The citation is
sound **here** because the tool proves the claim rather than asserting it: it
asks ninja for the objects the differential runner links and compares each
recorded dependency's bytes, so a presentation-only change cannot hide in it.
It licenses skipping the differential compares only, and this change's real risk
is in the picture, which is where I spent the review. The v1 fixtures have to be
copied, not linked, exactly as the candidate's `docs/BUILD_AND_VALIDATION.md`
correction says; I copied them into `local/v1-fixtures-review/`.

## The recovery, re-derived rather than believed

Before testing the implementation I checked the three claims the rest of it
rests on, from the ROM and from the original's pixels.

- `artifacts/classic-race-hud-review/glyph_read.py` reads the v9 pack's header
  itself, pulls `presentation.classic.font.v1`, builds every 8x16 glyph as the
  pair (t, t+$10), and matches each cell of a kept original frame. On the M4-16
  primary's frame 3208 it reads, at tilemap row 2 (y 15-30), columns 2-4 as
  tiles `$01 $4e $03` and columns 24-29 as `$0a $45 $03 $02 $45 $05`, every one
  in ink `#e70000` - that is `1/3` and `0:32:5`, the cells are 8x16 at row 2,
  the glyphs are in the caption's own sheet, and no pack content is missing.
  Claim 1 confirmed independently.
- The character table is at `$80:81F4`, not `$81:81F4`. ROM offset 0x0001F4
  reads `0a 01 02 03 04 05 06 07 08 09 0b 0c 0d 0e 0f 20 21 ... 2f 40 41 42 43
  44 45 46 47 48`, which is index 0 -> the `0` glyph, 1-9 -> `$01`-`$09`,
  `a`-`e` -> `$0b`-`$0f`, `f` -> `$20`, `v`-`z` -> `$40`-`$44`, then the
  punctuation from `$45`. `$81:81F4` is code (`c9 70 17 30 07 ...`). R-0043 and
  the header comment have it right; the task record's attempt 7 does not.
- DRAGSTER's left field really is the word: decoding the DRAGSTER original at
  frame 1400 gives tiles `$2c $0b $0d $0f` at columns 2-5, which is `r a c e`,
  and frames 3450 and 3453 give `$20 $23 $28 $23 $2d $22`, `f i n i s h`, with
  the clock cells already blank. Claim 2 is confirmed behaviourally on both
  tracks (ZOOM ZOO counts, DRAGSTER says `race`); that the selecting flag is
  specifically `$77:074B` I did not verify at the ROM level.
- The compose rule is genuinely exercised by the frames that scored zero, not
  merely inherited from R-0042. `artifacts/classic-race-hud-review/hud_cover_scan.py`
  finds kept original frames whose HUD ink no longer forms a whole glyph, i.e.
  cells something is drawing over: primary frames 1480, 1583, 1600, 1620, 1640,
  1649, 1860 ... and trick-long 1620, 2500, 2660, 2700 among many others, all of
  which my sweeps score at 0 differing pixels. On primary frame 6488 the
  finish-time cells at row 5 columns 13-14 are flat `9c9cff` in both the
  original and native, which is the channel-6 winner banner covering them.
  Claim 5 confirmed.

## My independent cases

Three captures the task never used, plus one picture set that did not exist
before this review.

**1. `original-trick-long`, 192 kept frames** (`trick-long-a`), the long-trick
race, where the rider spends time in the air near the HUD rows:

```sh
python3 $A/hud_review_check.py "$E/m4-16-playable-zoom-zoo/m4-16/trick-long-a" \
    "$E/m4-16-rider-art/m4-16-rider-art/original-trick-long" classic.crawler.zoom-zoo 1376
```

192 frames, HUD rows **0**, rows the bar covered **0**. Residual is the split
time (33 frames) and the declared off-screen arrow. Nothing new.

**2. `orig/pause-a`, 36 consecutive frames 5995-6030**, a pause:

```sh
python3 $A/hud_review_check.py "$E/m4-16-playable-zoom-zoo/m4-16/pause-a" \
    "$E/m4-16-final-review/m4-16-final-review/orig/pause-a" classic.crawler.zoom-zoo 1376
```

Frames 5995-5999 and 6010-6030 are 0 in every band. Frames 6000-6009 differ over
the **whole** picture (57,344 pixels each). The original dims every colour while
paused - its HUD band's commonest colours go from `174b4b`/`26796d` to
`0a2626`/`020a0a` - and native does not. That is the pause menu's own style,
explicitly out of scope and pre-existing, but it does mean the HUD is not
correct on a paused frame either (advisory 10).

**3. `orig/compound-reverse-a`, 137 frames** (`compound-reverse-a`), which keeps
consecutive frames across its finish - this is the case that found the defects:

```sh
python3 $A/hud_review_check.py "$E/m4-16-playable-zoom-zoo/m4-16/continued-controls/compound-reverse-a" \
    "$E/m4-16-final-review/m4-16-final-review/orig/compound-reverse-a" classic.crawler.zoom-zoo 1376
```

```
frame   6477: hud-rows 0    ...
frame   6478: hud-rows 9    was-bar 0  player-time 0    opponent-time 0    whole 9
frame   6479: hud-rows 165  was-bar 0  player-time 0    opponent-time 0    whole 165
frame   6480: hud-rows 0    was-bar 0  player-time 215  opponent-time 0    whole 270
frame   6490: hud-rows 0    was-bar 0  player-time 0    opponent-time 209  whole 209
```

**4. A new original picture set across the M4-16 primary's own finish.** The
kept pictures jump from 6484 straight to 6500, so no picture in the task's
evidence covers the redraw queue it recovered. I replayed the frozen `boundary-a`
timeline on the audited core and kept 6480-6500, every replayed frame checked
against the capture's own video and WRAM digests (5294 frames checked, 21 kept):

```sh
PYTHONPATH="$PWD" python3 $A/recapture.py \
    "$E/m4-16-playable-zoom-zoo/m4-16/boundary-a" $A/orig-finish-window 6480-6500
python3 $A/hud_review_check.py "$E/m4-16-playable-zoom-zoo/m4-16/boundary-a" \
    "$A/orig-finish-window" classic.crawler.zoom-zoo 1376
```

```
frame   6485: hud-rows 0    was-bar 0  player-time 0    opponent-time 0    whole 0
frame   6486: hud-rows 184  was-bar 0  player-time 0    opponent-time 0    whole 184
frame   6487: hud-rows 0    was-bar 0  player-time 228  opponent-time 0    whole 228
frame   6490: hud-rows 0    was-bar 0  player-time 0    opponent-time 209  whole 209
```

## Findings

### Blocking 1 - the blanked clock and both finish times are drawn one update late

R-0043's own update-by-update table is right about the original and the
implementation is one update behind it on three of the four fields. Decoding
both pictures with `glyph_read.py`:

| Picture | Original | Native |
| --- | --- | --- |
| 6485 | `finish`, clock `1:38:0` | same - correct |
| 6486 | clock cells **blank** | still draws `1:38:0` |
| 6487 | player time `1:38:02` at row 5 | row 5 still empty |
| 6488 | (banner covers row 5) | player time appears here |
| 6490 | opponent time `1:38:10` at row 20 | row 20 still empty |
| 6491 | - | opponent time appears here |

Reproduce:

```sh
PYTHONPATH="$PWD" python3 $A/recapture.py "$E/m4-16-playable-zoom-zoo/m4-16/boundary-a" $A/orig-finish-window 6480-6500
python3 $A/hud_review_check.py "$E/m4-16-playable-zoom-zoo/m4-16/boundary-a" "$A/orig-finish-window" classic.crawler.zoom-zoo 1376
# frame 6486 hud-rows 184, frame 6487 player-time 228, frame 6490 opponent-time 209
python3 $A/render_frames.py "$E/m4-16-playable-zoom-zoo/m4-16/boundary-a" \
    $A/native-finish classic.crawler.zoom-zoo 1376 6480 6500
python3 $A/glyph_read.py $A/orig-finish-window/frame-6486.png 2 24 29     # no glyph
python3 $A/glyph_read.py $A/native-finish/frame-6486.ppm     2 24 29      # $01 $45 $03 $08 $45 $0a
```

and independently, in a different race, at `compound-reverse-a` frames 6479,
6480 and 6490 (the table above).

The cause is that the queue's update numbers were applied to a state that is
already one update behind the picture. `classic_race_presentation_runner`
renders picture N from `state` at frame N with `previous_update` at frame N-1,
and `draw_classic_hud` reads the **previous** state, so a field gated on
`finish_delay >= k` first appears in picture `finish + k + 1`. Measured from
native's own output, `state(6484+d).finish_delay == d` on this race, so:

- `src/core/presentation.cpp:1458` `if(!finished || race.finish_delay<2)` blanks
  from picture 6487; the original blanks at 6486, so the gate is `<1`.
- `src/core/presentation.cpp:1463` `finish_delay>=3` draws the player's time at
  6488; the original draws it at 6487, so the gate is `>=2`.
- `src/core/presentation.cpp:1466` `previous_update.movement.frame>=*opponent+2U`
  draws the opponent's time at 6491; the original draws it at 6490, so it is
  `+1U`. (`ClassicRaceHistoryTracker::observe_update` records
  `opponent_finish_frame_=updated.movement.frame`, 6488 here, which native's own
  behaviour confirms.)

`src/core/presentation.cpp:1444`, the left field, has no delay gate and is
correct: `finish` appears at 6485 in both, and at 6478 in both on
compound-reverse.

The new unit tests cannot catch this because they asserted the queue positions
against `finish_delay` values directly (`won.race.finish_delay = 2` blanks the
clock) without ever tying a delay to a picture frame, so they encode the same
off-by-one and pass.

### Blocking 2 - the corner clock keeps running on the update the player finishes

On `compound-reverse-a` picture 6478, the first picture after the player
finished, the original still shows the digits it last published and native has
advanced the tenths:

```sh
python3 $A/glyph_read.py "$E/m4-16-final-review/m4-16-final-review/orig/compound-reverse-a/frame-6478.png" 2 24 29
#   column 24: $01  25: $45  26: $03  27: $07  28: $45  29: $08     -> 1:37:8
python3 $A/glyph_read.py $A/native-cr/frame-6478.ppm 2 24 29
#   column 24: $01  25: $45  26: $03  27: $07  28: $45  29: $09     -> 1:37:9
```

That is the 9 differing pixels the sweep reports at that frame. The native
picture comes from `artifacts/classic-race-hud-review/render_frames.py`, which
runs the engine over the capture's own inputs and renders one frame through
`classic_race_presentation_runner --timeline`:

```sh
python3 $A/render_frames.py "$E/m4-16-playable-zoom-zoo/m4-16/continued-controls/compound-reverse-a" \
    $A/native-cr classic.crawler.zoom-zoo 1376 6478 6478
```

`classic_race_hud_text` derives the clock from `previous_update.movement.timer`
on every update until the blank. The original stops publishing clock digits once
the finish takes over the redraw queue, so the cells hold whatever was last
written. On the M4-16 primary the tenth happened not to tick on that update, so
frame 6485 matches and the defect is invisible there; on compound-reverse it
ticks. This is separate from blocking 1: fixing the blank to `finish_delay<1`
still leaves `finish_delay==0` deriving a fresh, wrong clock.

Worth determining which side moves: the eleven differential gates compare the
engine's own state and pass, so I would expect the timer value itself to be
right and the defect to be that presentation publishes a clock on an update
where the original publishes none. If instead the engine's timer advances on the
finishing update where the original's does not, that is a movement finding and
belongs in its own record, not a presentation patch.

### Should-fix 3 - the measurement cannot see the behaviour it claims

The acceptance criterion is a sweep "over every kept race frame that has an
original picture", and the kept ZOOM ZOO pictures step 20 frames apart and jump
6484 -> 6500. Every field of the recovered redraw queue changes inside that gap,
so the headline "0 differing pixels" was compatible with all four fields being
wrong. This is the same shape of hole the CLASSIC-STUNT-NAMES review caught
twice. Required evidence should include consecutive original frames across the
finish on both tracks - producing them costs one `recapture.py` run, as above -
and the DRAGSTER equivalent, whose captured frames jump 3400 -> 3450 (the
original already reads `finish` with a blanked clock at 3450) and are screen-off
from 3454, so DRAGSTER's own finish transition is currently unmeasured end to end.

### Should-fix 4 - the picture claims are stated more broadly than the logs support

"The HUD rows (y 15-30) differ by 0 pixels on every frame of all three" (task
record, attempt 11) is false for 6 of the 18 DRAGSTER frames (24,576 pixels) and
for 11 of the 35 time-out frames (45,056 pixels). The cause is the known
screen-off transition and R-0043's measurement table scopes it correctly ("12
race frames"), but the task record and the capability checkpoint carry the
unqualified version, and the acceptance table's "every pixel matches, with no
exception claimed" is not what the logs say. State the frame counts that are
actually clean.

### Should-fix 5 - a wrong address in the task record

Attempt 7 says "the digit table at `$81:81F4`". The table is at `$80:81F4`
(ROM 0x0001F4, `0a 01 02 ... 09 0b ...`); `$81:81F4` is code. R-0043 and
`presentation.hpp` are correct, so this is the task record alone.

### Should-fix 6 - the handoff records none of the work

Every field of the task record's Handoff section still reads "to be recorded",
including "Commands executed, outcomes and report hashes" and "Verified
findings", while the record makes twelve attempts' worth of claims. The exact
sweep invocations exist nowhere in the repository or the artifacts: I had to
reconstruct the capture directories, the start id and the `first_row` argument
from the scripts and the picture names to reproduce the headline number. AGENTS
requires a fresh agent to resume without the previous conversation; at the
moment it cannot re-run the measurement without guessing.

### Advisory 7 - a picture-level regression check is what is missing

The 23 ctest tests pass and do exercise the new text function, but nothing in
the suite renders a frame at a finish transition. A small contract over a
handful of consecutive frames around the player's and the opponent's finish
would have caught both blocking findings and would keep catching them.

### Advisory 8 - the no-time sentinel is guarded on one side only

`classic_race_hud_text` guards the opponent's time with
`race.total_times[1]<60000U` but applies no such guard to the player's own,
where `hud_time(60000)` would print `0:00:00`. I could not reach it - the
time-out path sets `finished` false and no finishing race produces the sentinel -
so this is not a defect today, only an asymmetry worth a comment or a guard.
I confirmed the reachable character domain is safe: every HUD field is built
from `%10` digits, `:`, `/` or the words `race`/`finish`, so nothing can reach
the `std::invalid_argument` in `classic_caption_tile`, including a two-digit lap
count and any clock or finish-time digit.

### Advisory 9 - the split-time descoping is honest, and should be quantified

Leaving the signed split out does not make the picture worse: against the base
commit the same two bands scored 9,854 and 11,999 differing pixels and now score
6,347 and 9,880, and the authored bar's 838,656 are gone. Drawing the finish
times in cells the original also uses for the split is what the original does
with them, so the half-recovery is not an invention. But it is a live per-frame
deviation on ordinary race frames, not a rare one: 64 of the 274 kept primary
frames (23%) and 33 of the 192 trick-long frames still differ there. R-0043
describes it; the records should also give that frequency, and the M4-16 scene
table's "the only difference is HUD glyphs" line is now known to have been
incomplete and should be corrected where it is cited.

### Advisory 10 - "wherever it draws" should exclude paused frames

See independent case 2: the original dims the whole picture while paused and
native does not, so the HUD band is wrong on those frames too. Pre-existing and
out of scope, but the claim should name the exclusion.

### Advisory 11 - owned records not yet written

`docs/STATE.md` is listed as an owned path and carries a paragraph per completed
presentation task; it has no CLASSIC-RACE-HUD paragraph and does not link
R-0043. The registry row is added. I expect this at integration rather than at
review, so it is only a reminder.

## Readability

Good, and better than what it replaces. `draw_bg3_text` names the one thing the
caption and the HUD share and documents the cell arithmetic and the `8r - 1`
scroll in one place; `ClassicHudText` names its fields and cites the publisher
for each; `classic_race_hud_text`'s parameters make the state dependencies
explicit (the previous update, the scenario, the opponent's finish frame) where
the old `classic_race_hud` reached for the scenario itself. Units are stated
where they matter (`hud_time` centiseconds, `left_column` a tilemap column).
Evidence links are navigable: the header and `src/core/README.md` point at
R-0042/R-0043, and R-0043 points at the frames. Two notes: the comment at
`presentation.cpp:1458` says the blank happens "on the second update after the
finish", which is the misreading behind blocking 1 and should be rewritten
against a picture frame rather than a delay count; and the clock's five inline
`static_cast<char>` digit expressions would read better through the same
`digit` helper `hud_time` already has.

I found no weakened baseline, no masked skip, no changed expected result and no
accidental content in the diff, which is eight files of source, tests and
records.

## What I did not check

- DRAGSTER's finish transition against original pictures. Its captured frames
  jump 3400 -> 3450 and are screen-off from 3454, and producing new DRAGSTER
  originals needs the replay-manifest capture path rather than `recapture.py`.
  Both blocking findings are in shared code and will apply there, but I have not
  measured them on that track.
- That `$77:074B` is the race mode. I confirmed the behaviour it selects on both
  tracks, not the address.
- The two-digit lap and lap-total forms. Neither track reaches them, as R-0043
  declares; the unit test covers the column choice only.
- The hidden app runs and the 40-seed fuzz. I relied on the primary's
  `artifacts/classic-race-hud/gates-82f2d16` run for those, on the candidate
  commit, having re-run the build, ctest, the synthetic suite, the v1 contracts
  and `gate_identity` myself.
- Hosted CI. Not run on this candidate.
- Two-player, the pre-race and result writers, and the rest of the `$81:CFF6`
  family, all declared out of scope.

## What I would need to approve

Blocking 1 and blocking 2 fixed, and the fixes measured against consecutive
original frames across the finish - on both tracks if DRAGSTER originals can be
produced, on ZOOM ZOO at minimum - rather than against a frame set that steps
over them. Should-fix 3 to 6 are record and evidence corrections and can ride
with that change.
