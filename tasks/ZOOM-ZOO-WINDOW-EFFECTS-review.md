# ZOOM-ZOO-WINDOW-EFFECTS - independent review

- Candidate: `d69af67` (code `11d50f6`, `49bd26c`; record commit `d69af67`), base `main` at `6adcde8`.
- Reviewing model: Claude Opus 5, fresh session, no prior conversation with the primary.
- Checkout: `.worktrees/zoom-zoo-window-review`, branch `review/zoom-zoo-window-effects`, at the exact
  candidate commit. Every command below was run from that directory against a build made there.
- Verdict: **return**. One blocking finding (the compose rule of claim 3 is contradicted by the
  originals it cites), plus two should-fix and three advisory items. Claims 1 and 2 reproduce exactly.

## Build and commands

```
python3 tools/project.py build --preset app-debug          # status=passed, 22.1 s
ctest --test-dir build/app-debug                            # 23/23 passed
python3 tools/project.py build --preset lab-debug ; ctest --test-dir build/lab-debug   # 23/23 passed
```
Probes written by this review live under the ignored `artifacts/review/` of this checkout:
`wram_words.py` (capture WRAM words), `rom_disassemble.py` (copied from
`local/evidence/dragster-window-effects/dragster-window-effects/`, ROOT resolved to this checkout),
`member_probe.cpp` (linked against this build's static libraries), `agree.py` (pointer agreement),
`pictures.py`, `d3213.py` (picture scoring), `compose_rule.py` (finding B1). None of them read the candidate's worktree.

## Reproductions

### 1. The pointer, `$1229` and `$0BA7` read from the captures (claim 1)

`artifacts/review/wram_words.py`, straight from each capture's `memory.wram`
(one 131072-byte image per frame from `reference.json["frames"][0]`):

| Capture | `$1229` over the whole capture | `$0BA7` at initialization | First published `$11FD` | Member |
| --- | --- | --- | --- | --- |
| ZOOM ZOO `boundary-a` | 0 only | 0 | end-of-frame 1381, `$918F` | 5 |
| ZOOM ZOO `pause-countdown-a` | 0 only | 0 | end-of-frame 1381, `$918F` | 5 |
| DRAGSTER `orig-countdown-pause-a` | 0 through 1327, 1 from 1328 | 1 | end-of-frame 1333, `$9512` | 6 |

`$11FF` is `$15` on every frame that publishes. `$918F - $8000 = 5 * 899` and `$9512 - $8000 = 6 * 899`,
so the transition members are 5 and 6 as claimed, and `$1229` is latched on the initialization frame
(DRAGSTER 1328) and never moves afterwards even though `$0BA7` does (values 0, 1, 256, 275 on
`boundary-a`).

ROM (sha256 `a1105819...1fd4e`, as R-0040 names):

```
$83:cc05  ad a7 0b     LDA $0ba7      (8-bit A; the setup that also fixes $1281 at $83:cc20)
$83:cc08  8d 29 12     STA $1229
$83:e60f  a9 05 00     LDA #$0005     ($83:e661, $83:e6c3, $83:e761 identical)
$83:e612  18 6d 29 12  CLC : ADC $1229
$83:e616  bf 5c e5 83  LDA $83e55c,x  (after ASL/TAX)
$83:e61b  69 00 80     ADC #$8000
$83:e61e  8d fd 11     STA $11fd  ;  LDA #$0015 : STA $11ff
```

A byte search of the whole 2 MiB image for every store form to `$1229`
(`8d 29 12`, `8f 29 12 00`, `9d/99/9c/9e/1c/0c/ee/ce 29 12`) finds exactly one code site,
`$83:CC08`. The only other hit, `TSB $1229` at `$14:8EAE`, sits inside a repeating data pattern
(`60 2a 60 2a 30 2a 0c 29 12 2a 12 2c`) in a graphics bank and is not code. So the latch is the sole
writer, and claim 1's mechanism is confirmed.

### 2. The header derivation (claim 1, native side)

`artifacts/review/member_probe.cpp`, linked against this build:

```
physics.track.dragster.data size=33815 start-y player=50 opponent=50 |
  old rule (y&1)==0 player=1 opponent=1 | classic_race_start_reflected player=1 opponent=1 | member=6
zoom.track-data size=50665 start-y player=93 opponent=93 |
  old rule (y&1)==0 player=0 opponent=0 | classic_race_start_reflected player=0 opponent=0 | member=5
track Dragster: classic_race_start pose.reflected player=1 opponent=1; content member=6; family 22475 bytes
track ZoomZoo:  classic_race_start pose.reflected player=0 opponent=0; content member=5; family 22475 bytes
```

`classic_race_start_reflected` returns exactly what the inlined `(y & 1) == 0` returned for both riders
on both tracks' real entries, and the derived member (6 / 5) equals the `$1229 + 5` read from the
captures. The family is 22475 = 25 * 899 bytes for both tracks. Refactor and derivation are sound.

### 3. Pointer agreement from this build (claim 1 and claim 2)

`artifacts/review/agree.py`, driving `build/app-debug/src/core/zoom_zoo_runner` from each capture's own
timeline and asking `classic_race_presentation_runner --window-index` for the published member, against
`$11FD` at the end of frame n-1, from setup to the loading frame:

| Original | State rows identical | Published member agrees |
| --- | --- | --- |
| ZOOM ZOO `boundary-a` | 6225 / 6225 | **5344 / 5344** |
| ZOOM ZOO `pause-countdown-a` | 6225 / 6225 | **6219 / 6219** |
| DRAGSTER `orig-countdown-pause-a` | 2573 / 2573 | **2267 / 2267** |

These are the record's numbers exactly. The original's own member runs on `boundary-a` are
`5:1382-1402, 0:1403-1431, 5:1432-1462, 1:1463-1491, 5:1492-1522, 2:1523-1551, 5:1552-1582`, then GO
alternating 4/3 from 1583 - the timeline the record states. On `pause-countdown-a` the original's runs
are `5:1432-1450, none:1451-1463, 5:1464-1475`, i.e. the window is off for thirteen frames, three more
than the menu selection alone would explain; native matches every one of them. The two frame-based
fallbacks disagree exactly where R-0040 and DRAGSTER-WINDOW-PAUSE already declare they do
(6129/6219 and 5769/6219 on the countdown pause, 1934/2267 on DRAGSTER's), and only there.

### 4. The diverted-update predicate against the original's own words (claim 2)

Read directly from `pause-countdown-a`'s WRAM (end-of-frame values):

```
frame : $0D49 $0D4B (heads) | $0D45 $0D47 (overlays) | $11FD
1449  :   0    36          |   0   3945             | 0x918f
1450..1462:  0 36          |   0   3945             | 0xdb4e   <- sentinel, 13 updates
1463  :   0    36          |   0   3945             | 0x918f
1464  :   0    35          |   0   3945             | 0x918f   <- the look advances again
```

The original froze the look words across updates 1450-1462 and reset the pointer to `$DB4E` over the
same span; both resume on 1463/1464. That is the engine's suspended-update clock, not the menu
selection (Start was held on 1460-1462 after the resume, with the selection already zero). The new
predicate `updated.pause.suspended_updates != previous.pause.suspended_updates` is the right reading,
and `suspended_updates` is a monotone `std::uint32_t` bounded by elapsed frames (the restore validator
in `movement.cpp:1780` enforces `suspended_updates <= frame - initialization_frame`), so the inequality
cannot saturate or wrap in a real race. Claim 2 is confirmed. I did not rebuild R-0036's `look_eval`
harness; the WRAM series above and the 6219/6219 pointer agreement on that capture cover the same
ground for the window and the look words, but see "Not done" below.

### 5. Pictures (claim 3)

`artifacts/review/pictures.py`, rectangle (0, 28, 256, 196), "before" being the `main` build at
`6adcde8` in the main checkout (not rebuilt), "after" this checkout's build:

| Original frame | Rect mismatch before / after | Changed in rect | Now matching | Newly wrong |
| --- | --- | --- | --- | --- |
| `boundary-a` 1450 | 9822 / **163** | 9659 | 9659 | 0 |
| `boundary-a` 1583 | 9066 / **341** | 8725 | 8725 | 0 |
| `boundary-a` 1649 | 9414 / **693** | 8721 | 8721 | 0 |
| `boundary-a` 6724 | 6095 / **827** | 5268 | 5268 | 0 |
| `pause-countdown-a` 1583 | 9822 / **167** | 9655 | 9655 | 0 |
| `pause-countdown-a` 1649 | 9096 / **371** | 8725 | 8725 | 0 |
| `pause-countdown-a` 6724 / 6725 | 3838 / **277** each | 3561 each | 3561 | 0 |
| `pause-countdown-a` 6800 | 3374 / **341** | 3033 | 3033 | 0 |

All of these match the record. Frames without a window (`boundary-a` 1377, 1700, 2501, 3208, 4840,
6484, 6725, 6800, 7000) are pixel-identical to the `main` build: 0 pixels changed on each. The 274
pixels each that frames 1583 and 1649 change above the rectangle (rows 20-27) also all match the
original, so nothing hides outside the scored band.

DRAGSTER release-3213 (`artifacts/review/d3213.py`, its own rows from this build):
183 frames with originals, 57 changed (1420-1570), 1060 changed pixels, **1060 now matching, 0 newly
wrong, no frame scoring worse**. Exactly the record's figures.

So every number in the record's measurement tables reproduces. The disagreement is over what they mean.

## Findings

### BLOCKING - B1. The compose rule keeps the player's object where the original covers it

Claim 3 and its native implementation (`49bd26c`: the `keep` mask in `render_window_xor`, the
`player_object` array in `render_classic_race`, `*window_index<=6U?&player_object:nullptr`) assert that
countdown members 0-6 leave the player's object (OBJ palette 3) showing. The originals the record
cites say the opposite: the window covers the player too, exactly as it covers the opponent.

Reproduce (`artifacts/review/compose_rule.py` in this checkout; it reads the member's HDMA table out of
`presentation.effect.classic.window-tables.v1` in the pack, rebuilds the same XOR region
`render_window_xor` computes, and compares the candidate's own PPM with the original PNG):

| Capture / frame | Member | Pixels the keep mask preserves in the rectangle | Of those, the original shows the window fill colour | Of those, the original shows native's colour |
| --- | --- | --- | --- | --- |
| `boundary-a` 1450 | 5 | 96 | **96** | 0 |
| `boundary-a` 1583 | 4 | 295 | **295** | 0 |
| `boundary-a` 1649 | 4 | 299 | **299** | 0 |
| `pause-countdown-a` 1583 | 5 | 100 | **100** | 0 |
| `pause-countdown-a` 1649 | 4 | 295 | **295** | 0 |

Every pixel the keep mask preserves shows the flat window colour in the original, and none shows the
object. The window is a flat replace, not a blend: on `boundary-a` 1583 all 8691 pixels the candidate
does cover inside the member-4 region are that one colour (`#ceceff`) in the original as well, so the
original shows nothing but fill anywhere inside the window - there is no player object under it to be
found. A side-by-side crop is at `artifacts/review/pictures/boundary-a/zoom-1583.png`: the main build
(left) draws both unicycles, the candidate (middle) draws the player's red/black unicycle on top of the
GO stroke, and the original (right) shows the pale stroke running straight through with only the part
of the rider outside the stroke visible.

The measured cost, `compose_rule.py`, rectangle (0, 28, 256, 196), comparing the candidate against the same
picture with the window covering both riders:

| Capture / frame | Candidate (keep the player) | Covering both riders |
| --- | --- | --- |
| `boundary-a` 1450 | 163 | **67** |
| `boundary-a` 1583 | 341 | **46** |
| `boundary-a` 1649 | 693 | **394** |
| `pause-countdown-a` 1583 | 167 | **67** |
| `pause-countdown-a` 1649 | 371 | **76** |

Covering both is strictly better on all five frames with an original picture and a countdown or GO
window over the player - 1085 pixels better in total. The record's "every pixel the change touches now
matches the original" is true only because the keep-mask pixels are ones the main build already drew
the same way, so they fall outside the "changed" set; they are 295 of the 341 residual mismatches on
`boundary-a` 1583 and 96 of the 163 on 1450, which the record attributes to "the rider overlays and
authored HUD band declared by M4-16". They are not overlay error; they are the keep mask.

The DRAGSTER release-3213 win (1060 pixels on 57 frames) does not support the keep mask either. It
comes entirely from the other half of `49bd26c`, moving members 0-6 from before the objects to after
them; the player never sits under a window in those frames, which is why the sweep reports 0 newly
wrong. The recovered rule the evidence supports is simply "every member, 0-24, composes after the
objects and covers both riders" - which also makes the `keep` parameter, the 57344-byte
`player_object` array and the `<=6U` branch unnecessary.

What to change: drop the keep mask and render every member after the objects; then correct claim 3 in
`tasks/ZOOM-ZOO-WINDOW-EFFECTS.md`, the "What a window covers" bullet of R-0040's "Established later"
section, and the `docs/STATE.md` paragraph ("the countdown members let the player's object show
through while covering the opponent's (the SNES colour-math exemption of OBJ palettes 0-3)"), all of
which currently record a false finding about the original. AGENTS.md is explicit that a plausible
invention must not be described as accurate; R-0040 is a research record other tasks will build on.
Re-score the five frames above and DRAGSTER release-3213 after the change.

Note this is not a regression against `main`: ZOOM ZOO drew no window at all at `6adcde8`, so the
candidate is still a large improvement (9066 -> 341 on 1583). The objection is to the recovered claim
and to leaving 1085 measurable pixels wrong when the simpler rule is right.

### SHOULD FIX - S1. The record and R-0040 misdescribe two of the frames they cite

`pause-countdown-a` frame 1583 is cited as "both riders stand at the line under the GO letters", but on
that capture the pause pushed the countdown back: the original's member on 1583 is 5, the transition
sign, and GO does not start until 1596 (measured in the member runs of reproduction 3 above). The
record's frame-1583 pair is therefore not the like-for-like GO comparison it is presented as. Also
`boundary-a` 1583's "346 of the 392 residual pixels ... where native drew the opponent" does not
reconcile with the 295 player pixels measured here; whatever the 346 counted, the split between player
and opponent in that sentence is not right. Both statements should be re-measured with B1's fix.

### SHOULD FIX - S2. The start-y word offsets in the new comments are wrong

`src/core/zoom_zoo_movement.hpp` says of `classic_race_start_reflected`: "the parity of its start y
word in the track header (words 3-4 for the player, 5-6 for the opponent)". The code reads
`content_word(decoded_track, 5 + 4 * rider)`, and `content_word` takes a byte offset, so the start y
words are at bytes 5-6 (player) and 9-10 (opponent); bytes 3-4 and 7-8 are the x words.
`tests/native/presentation_tests.cpp` repeats the error ("the opponent's word is 5-6 words later" for
`header[9]`, which is four bytes later). For a function whose whole job is to name a header field,
the offsets in its comment should be the ones it reads.

### ADVISORY - A1. The transition member is recomputed on every simulation update

`ClassicRaceHistoryTracker::observe_update` calls
`classic_window_transition_member(classic_track_data(pack, updated.track))`, i.e. a string-keyed pack
lookup plus a header decode, once per update for the whole race (about 5,300 times on a ZOOM ZOO
race). It is a constant of the track; computing it once in `reset()` or holding it beside the pack
would say so and cost nothing. Not a correctness problem - the value cannot change mid-race.

### ADVISORY - A2. The keep mask's storage, if any form of it survives B1

`std::array<bool, 256 * 224> player_object{}` is 57 KB zero-initialised on the stack of every
`render_classic_race` call, matching the existing `bg1_above_objects`. If B1 is fixed by removing the
mask this disappears; if some keep rule survives, a `std::vector<bool>` or a reused buffer would be
kinder, and `render_window_xor`'s raw `const std::array<bool, 256*224>*` parameter would be better as
a span so the size is not duplicated in two places.

### ADVISORY - A3. The declared omissions

Both declared omissions are acceptable as omissions: not reading CGWSEL/CGADSUB/WOBJSEL and the
TMW/TSW masks is a reasonable boundary given the pictures, and the absence of a unit test that renders
a window over an object is defensible while the tests have no rider content. But B1 is exactly the
failure mode those two omissions leave open: the compose rule was inferred from a picture score rather
than read from the registers, and nothing in the suite would have caught it. The "exact next
experiment" the record names (reading those registers) is the right one, and after B1 it should be
promoted from "next experiment" to the thing that settles the rule.

## Regression checks

- Legacy v1 DRAGSTER renderer: the four `render_window_xor` call sites at `presentation.cpp:925/927/
  967/969` are untouched, keep their before/after-object ordering and pass no keep mask;
  `dragster_window_table_index` still passes the constant `dragster_window_transition_index = 6`.
  Frozen v1 contracts therefore cannot see this change, and the DRAGSTER frozen compares below confirm it.
- Frozen differential contracts, this build's `zoom_zoo_runner` and this checkout's pack, output under
  `artifacts/review/`:
  `dragster_playable compare` primary **379 restores**, random-1 **567**, reversal **327**;
  `zoom_zoo_playable compare` M4-16 primary v11 **757 restores**. All passed, all equal to the record.
- ctest on two presets: app-debug 23/23, lab-debug 23/23.
- Test diff: no comparison is weakened, no baseline regenerated, no skip added. The changes are
  parameter threading plus new assertions; the one replaced assertion (`pause_updates` in
  `rider_presentation_tests.cpp`) is replaced because the predicate's meaning changed, and the
  replacement is stronger (four diverted cases and one non-diverted case instead of two).
- Arithmetic: `5U + reflected` cannot overflow; the keep-mask index is bounded by the same
  `screen_y < 224` / `screen_x < 256` loop bounds as the pixel write beside it;
  `classic_race_start_reflected`'s `size() < 11` check matches what `content_word(track, 9)` needs.
  No new undefined or unchecked arithmetic.
- Readability otherwise: names are descriptive and the new comments cite their evidence by address and
  capture, which is the project's standard. The `dragster_window_transition_index` comment correctly
  marks itself as the legacy v1 path's constant.

## Not done, and why

Within the review's time box I did not:

- rebuild R-0036's `look_eval` harness against this build. Reproduction 4 reads the original's look
  words for the pause window directly instead, and the 6219/6219 pointer agreement on that capture
  exercises the same predicate; a full 10,696-check look comparison was not run here.
- run the remaining five ZOOM ZOO captures' pointer agreement (`loss-a`, `pause-a`, `pause-b`,
  `late-start-a`, `pause-countdown-b`) or the other three DRAGSTER pause originals. Two ZOOM ZOO and
  one DRAGSTER capture reproduced exactly, so I have no reason to doubt the rest.
- run the v1 `presentation-check` fixtures, the synthetic suite, the hidden app runs, the fuzz runner
  or the three remaining presets. The v1 renderer is provably untouched (above), which is what those
  gates would be protecting.
- consider whether any accepted behaviour depended on the old selection-based predicate beyond what
  the frozen compares and ctest cover; the four differential compares and both ctest presets pass, and
  the predicate is only read by the look, the overlays and the window pointer.

## Verdict

**Return.** Claims 1 and 2 are recovered correctly, implemented cleanly and reproduce to the frame on
independent runs; the DRAGSTER side of claim 3 (members composing after the objects) is right and worth
1060 pixels. The keep mask that claim 3 adds on top of that is contradicted by all five ZOOM ZOO frames
whose originals show a countdown or GO window over the player, costs 1085 pixels against the simpler
rule, and is written into R-0040 and `docs/STATE.md` as a recovered fact about the original hardware.
Fix B1 and the two should-fix items, re-measure, and this is an approve.

## Re-review at 3e2d30d

- Candidate: `3e2d30d` on `task/zoom-zoo-window-effects` (`83af471` code, `989b0e9` records, on top of the
  returned `d69af67`). Reviewing model: Claude Opus 5, the same session that returned `d69af67`.
- Checkout: `.worktrees/zoom-zoo-window-review` moved to `3e2d30d`, rebuilt there before measuring
  (`python3 tools/project.py build --preset app-debug`, status=passed; binaries relinked).
- Verdict: **approve**.

### B1 re-verified

`artifacts/review/compose_rule.py`, unchanged from the returning review, re-run against the new build:

| Capture / frame | Member | Pixels the mask keeps | Pixels covered | Of those, the original shows the fill | Candidate rect mismatch | Covering both riders |
| --- | --- | --- | --- | --- | --- | --- |
| `boundary-a` 1450 | 5 | **0** | 9673 | 9673 | 67 | 67 |
| `boundary-a` 1583 | 4 | **0** | 8986 | 8986 | 46 | 46 |
| `boundary-a` 1649 | 4 | **0** | 8986 | 8986 | 394 | 394 |
| `pause-countdown-a` 1583 | 5 | **0** | 9673 | 9673 | 67 | 67 |
| `pause-countdown-a` 1649 | 4 | **0** | 8986 | 8986 | 76 | 76 |

Nothing is preserved inside any window region, every pixel inside it is the flat fill in both the
candidate and the original, and the candidate now equals the counterfactual exactly on all five frames
- the finding is closed, not narrowed. Picture scores from this build (`artifacts/review/pictures.py`,
rectangle (0, 28, 256, 196), "before" still the untouched `main` build at `6adcde8`):

| Original frame | Rect mismatch before / after | Changed in rect | Now matching | Newly wrong |
| --- | --- | --- | --- | --- |
| `boundary-a` 1450 | 9822 / **67** | 9755 | 9755 | 0 |
| `boundary-a` 1583 | 9066 / **46** | 9020 | 9020 | 0 |
| `boundary-a` 1649 | 9414 / **394** | 9020 | 9020 | 0 |
| `boundary-a` 6724 | 6095 / **827** | 5268 | 5268 | 0 |
| `pause-countdown-a` 1583 | 9822 / **67** | 9755 | 9755 | 0 |
| `pause-countdown-a` 1649 | 9096 / **76** | 9020 | 9020 | 0 |
| `pause-countdown-a` 6724 / 6725 / 6800 | 3838 / **277**, 3838 / **277**, 3374 / **341** | 3561, 3561, 3033 | all | 0 |

Every figure the coordinator reported reproduces. Frames without a window (`boundary-a` 1377, 1700,
2501, 3208, 4840, 6484, 6725, 6800, 7000) remain pixel-identical to the `main` build, 0 changed pixels
each; their rect mismatches against the originals (63 on 3208, 57 on 4840, 61 on 6484, 0 elsewhere)
are unchanged before and after. DRAGSTER release-3213 (`artifacts/review/d3213.py`): 183 frames scored,
**75 changed (1420-1602), 13,804 changed pixels, 13,804 now matching, 0 newly wrong, no frame worse** -
18 more frames and 12,744 more pixels than the keep-mask version won, which is the expected shape of
the fix (the mask had been suppressing the player wherever a DRAGSTER rider sat under a member too).

### S1, S2, A1 as applied

- **S1.** Item 4 and the picture table now read "pause-countdown 1583 (member 5, GO pushed back to
  1596 by the pause)", and the 346-of-392 sentence now says those pixels were fill in the original
  where native drew a rider, with both riders over the window at `11d50f6`. Both are correct against
  my own measurements.
- **S2.** `zoom_zoo_movement.hpp` now reads "bytes 5-6 for the player, 9-10 for the opponent; the x
  words precede each", and the test comment "the opponent's start y word (bytes 9-10)". Both match
  what `content_word(decoded_track, 5 + 4 * rider)` reads.
- **A1.** `ClassicRaceHistoryTracker` caches `transition_member_` on the first update after a reset and
  `reset()` clears it. I checked the staleness risk this introduces: the only live holder,
  `LivePresentation::history_`, is wholly replaced on every restart (`sdl_main.cpp:346` and `:437`
  assign a fresh `LivePresentation{}`), and the track is fixed for a session
  (`classic_race_presentation_content` is taken once at `sdl_main.cpp:259`), so the cache cannot
  outlive the race it was read for. Pointer agreement re-run on `pause-countdown-a` after the change:
  **6219/6219**, state rows 6225/6225 - unaffected.
- **A2** is moot as predicted: `render_window_xor` is back to three parameters and the 57 KB
  `player_object` array is gone. **A3** is accepted as recorded; the register read stays the named
  next experiment, and the rule now in the records is the one that was measured.

### Records

The exemption claim survives nowhere as a statement about the original. The only remaining mentions
are explicit retractions: R-0040's bullet ends "(The task's first reading, a colour-math exemption of
the player's object, was measured wrong by its reviewer.)"; the task record's item 4 says the first
reading "was wrong and is recorded under Mistakes"; and the new Mistakes section names the crop-vs-
measurement error. R-0040's "What a window covers" bullet now reads "everything", with the 8,691-pixel
flat-colour measurement and the corrected 75-frame DRAGSTER figure; `docs/STATE.md` now says "every
window member composes after both riders". The surviving "members 0-6 ... (before the riders)" at
R-0040 line 135 describes the legacy v1 `render_dragster`, which this task deliberately leaves alone,
so it is still accurate there. The review dispositions table in the task record matches what I measured
on every row.

One advisory, not a condition of approval: the handoff still reads "Failed approaches: none" while the
new Mistakes section records one. Worth a word next time the record is touched.

### Gates re-run on this build

- `ctest --test-dir build/app-debug`: **23/23 passed**.
- `zoom_zoo_playable compare`, M4-16 primary v11 freeze, this build's `zoom_zoo_runner` and this
  checkout's pack: **passed, 757 restores** (`artifacts/review/compare-zoom-primary-rr.json`).
- Pointer agreement, `pause-countdown-a`: 6219/6219 published, 6225/6225 state rows.
- Legacy v1 DRAGSTER renderer still untouched: `render_window_xor`'s three v1 call sites and their
  before/after-object ordering are unchanged by `83af471`, and `dragster_window_table_index` still
  passes the constant 6.
- No tracked file was edited while a compare was running.

### Not done in the re-review

Re-ran one frozen compare (the M4-16 primary) rather than all four, one preset's ctest rather than two,
and one capture's pointer agreement rather than three; the code change is confined to the render step
and the tracker's caching, both of which I exercised directly. I did not re-run the DRAGSTER frozen
compares, the other presets, the v1 fixtures, the hidden app runs or the fuzz runner, and I did not
rebuild R-0036's look harness at this candidate either.

### Verdict

**Approve.** The blocking finding is fully fixed rather than papered over: the keep mask is gone, the
compose rule is now "every member covers both objects", and that is both what the originals show and a
strictly better score on every frame that can test it, including 18 DRAGSTER frames the earlier version
did not reach. S1, S2 and A1 are applied correctly, A1's new cache cannot go stale in any live path,
the records no longer assert the retracted finding anywhere, and the gates I re-ran pass with the
expected numbers.
