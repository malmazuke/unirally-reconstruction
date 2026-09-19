# CLASSIC-STUNT-NAMES - independent review

- Candidate: `745c4b87abcc19b317c64f7b3dc5a8f32385c5b3` (documentation on top of the code candidate
  `9e2570e`), base `main` at `260334d`. The five commits under review are `814e399`, `91bc272`,
  `6508698`, `9e2570e` and `745c4b8`.
- Reviewing model: Claude Opus 5, fresh session, no inherited conversation with the primary.
- Checkout: `.worktrees/classic-stunt-names-review`, branch `review/classic-stunt-names`, created at
  the exact candidate. Every command below ran from that directory against a build made there.
  Nothing on `task/classic-stunt-names`, in the primary's worktree or in the main checkout was
  modified; the primary's artifacts were read only.
- ROM `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e` (verified by hashing the
  file behind `local/rom-location.txt`); audited core reported as `e59bf88d4fc922c9...` by every
  access capture I ran.
- Verdict: **return**. Two blocking findings. The recovered mechanism is real and I reproduced every
  part of it independently from the ROM and the audited core - table, encoding, drawing and both
  halves of the blanking rule - and the pack work is byte-exact and genuinely additive. But the
  renderer throws on caption entries that ordinary play can reach (B1), and the caption is
  composited above the channel-6 window effect where the original composites it below (B2). B2 also
  turns out to be the real cause of the one imperfect picture score the task explains away as "the
  start ring, a declared omission": native draws that content itself, so the excuse does not hold
  and the criterion is not met. Four should-fix and five advisory items follow. Both blocking items
  are small, local fixes; nothing in the recovery needs redoing.

## What I ran

```sh
PATH="$PWD/local/toolchain/ninja-1.13.2-darwin-arm64:$PATH" python3 tools/project.py build --preset app-debug
bash artifacts/classic-stunt-names-review/gates-diff.sh    # the eleven accepted differential gates
bash artifacts/classic-stunt-names-review/gates-suite.sh   # presets, synthetic, v1 contracts, hidden, fuzz, pictures
python3 artifacts/classic-stunt-names-review/withheld_case.py --capture
python3 artifacts/classic-stunt-names-review/caption_band_zoom.py
python3 artifacts/classic-stunt-names-review/caption_band_dragster.py
python3 artifacts/classic-stunt-names-review/voice_caption_repro.py
```

Reviewer artifacts live in this checkout's ignored `artifacts/classic-stunt-names-review/`: the two
gate scripts and their logs, `gates/` (the differential reports, the suite logs and the picture
scores), `withheld/` (my own original capture and its five frame images), `acc3280/`, `acc3100/`,
`acc2860/` and `acc-init/` (four access records of my own), and the four scripts above. No reviewer
input points into another worktree except `caption_band_dragster.py`, which renders against the
primary's `pictures-a` frames; the only other things I read from the primary's worktree are its
`access-1590` record and its gate logs, and only to check its claims.

## 1. What I reproduced, item by item

### 1.1 The mechanism, from the ROM and the audited core rather than from the primary's probes

I did not re-run the primary's `queue_probe.py`/`string_search.py`. I read the table out of the ROM
myself and took four access records of my own over an accepted replay manifest the primary never
used for this - `tests/manifests/replay/race-crawler-dragster-12000-continuous-right-fields.json`.

**The table's base and extent - reproduced, at a different entry from the primary's.** Decoding the
ROM at file offset 772596 (`$17:C9F4`, LoROM) with a sixteen-byte stride gives entry 1 `roll`,
14 `wipeout`, 15 `last lap`, 37/38/39 `winner`/`draw`/`loser`, 44-46 the `more stunts / give you /
bigger boosts` sentence, 47 sixteen spaces, and 72 upward the voice lines. Entry 0 is
`e6 0a e7 0a e8 0a e9 0a ea 0a eb 0a ff ff 00 00`, which is not caption text, so excluding it is
right. The primary measured the base from a read of entry 44; my access record shows `$81:BFE9`
reading exactly sixteen bytes at bus `$17:CE24-$17:CE33` on frame 3279, the update my own WRAM
series says consumed event **67** - and `$17:C9F4 + 16*67 = $17:CE24`. Base and stride confirmed
from an entry the primary never touched.

**The ASCII-to-tile arithmetic - reproduced on all three runs, which the primary's captions could
not do.** `$81:C019` writes the buffer at `$0EA7-$0EB6`. Reading those writes out of three access
records of my own:

| frame | event | entry | buffer `$0EA7-$0EB6` | R-0042's arithmetic |
| --- | --- | --- | --- | --- |
| 2860 | 61 | `    try it      ` | `80 80 80 80 2e 2c 43 80 23 2e 80 80 80 80 80 80` | exact match, including `y` (n=24) -> `$43` in the third run |
| 3100 | 64 | `press button b  ` | `2a 2c 0f 2d 2d 80 0c 2f 2e 2e 29 28 80 0c 80 80` | exact match, including `b` (n=1) -> `$0c` and `e` (n=4) -> `$0f` in the first run |
| 3280 | 67 | `        to roll ` | `80 x8, 2e 29 80 2c 29 26 26 80` | exact match, middle run |

R-0042 records the rule as `$0B + n` for `n < 5`, `$20 + n - 5` for `5 <= n < 21`, `$40 + n - 21`
for `n >= 21`, space `$80`. All three runs and the space are confirmed. (The writes are 16-bit
stores at consecutive byte addresses, so each buffer byte is the low half of its store; the record's
byte-level statement is the right one.)

**The two tilemap rows - reproduced.** On frame 3280, `$81:F31D` points `$2116` at words 6472-6487
and `$81:F327` writes sixteen words; `$81:F337` points at 6504-6519 - 32 words on - and `$81:F345`
writes sixteen more. The values are exactly `$3800 | tile` and `$3800 | (tile + $10)`:

```
row 1 ($81:F327): 0x3880 0x382e 0x3829 0x382c 0x3826     (space, t, o, r, l)
row 2 ($81:F345): 0x3890 0x383e 0x3839 0x383c 0x3836
```

`$81:C057` raises `$0EE7`, `$81:F30C` reads it once a frame and `$81:F352` clears it: all three
appear in my record with those program counters. The `$3800` attribute is palette 6 with priority.

**Both halves of the blanking rule - reproduced.** Half one: entries 47, 51, 54, 55, 57 and 62 of the
ROM table are sixteen spaces, so a hint sentence ends by publishing a blank entry. Half two: on my
own render of the accepted M4-16 ZOOM ZOO original, native's `empty_display` is 1 at updates 3207,
4839 and 6483 and the caption band is `2304/2304` identical to the original, which is blank there.
Removing the `empty_display` guard is what the primary's attempt 14 corrected; the corrected
behaviour is what I measured.

### 1.2 The pack

- The caption bytes: `sha256(rom[772612:772612+4080]) = 1d5530ca0737ee3e87fdcc2ea26f020caac9d420cd824f29532c426b52ab6f28`,
  exactly the manifest's digest, and `772612 = $17:C9F4 + 16` - entry 1, as claimed.
- Additive: diffing the v8 rules (`745c4b8~5`) against the v9 rules, **nothing was removed and
  nothing changed** - 56 entries -> 57, the shared prefix in the same order, the same `source_rom`
  and `description`. Every v8 entry keeps its size and digest, so every v8 payload is byte-identical
  in v9.
- Byte-exact: rebuilding the pack from the ROM through the tracked rules gives 1290589 bytes,
  `sha256 d7c96d89882d2489da3c1414c7a7575097666a08237b3406d5c99532178bb1be`, **identical** to the
  supplied `local/classic-pal-crawler-two-tracks-v9.pack`.
- `two_track_rules_sha` in `content_pack.cpp` equals `sha256` of the tracked rules file
  (`67e47e33144fbec0706791b0535d22b132d0efe3a38fd7c0dc5736ed304b17ac`).
- The font: `presentation.classic.font.v1` decodes from file offset 376000, 2048 bytes, 128 2bpp
  tiles, and only pixel values 0 and 3 occur - the one-bit font R-0042 describes. Sheet tile `$2E`
  is the top half of `t` and `$3E` its bottom half, so the `+$10` half relation holds in the sheet
  the renderer indexes.

### 1.3 The primary's picture scores

Re-derived from my own build, my own renders and the accepted originals:

| case | primary's claim | my measurement |
| --- | --- | --- |
| ZOOM ZOO 1649 (`more stunts`) | ring occludes, a declared omission | band `2188/2304` reproduces, but the attribution does not - see B2: those 116 pixels are channel-6 window member 4, which native draws itself |
| ZOOM ZOO 3208, 4840, 6484 | blank | band `2304/2304` |
| ZOOM ZOO 1450, 1583 | no caption yet | band `2304/2304` |
| M4-16 queue probe | 93 consumptions, events 14, 15, 37, 44-59 | identical: 93 consumptions, the same event set |

The four DRAGSTER scores I re-derived with my own build, my own renders and my own comparison
(`caption_band_dragster.py`; the original frames are the primary's `pictures-a` capture, read only):

```
1601 more stunts    2188/2304 identical; original [(255, 206, 206)]
1637 give you       2304/2304 identical
1652 bigger boosts  2304/2304 identical
1685 wipeout        2304/2304 identical
```

Identical to the primary's `gates/caption-pictures.log`. The 116 pixels at 1601 are B2, not a ring.

## 2. My withheld case

The primary compared four DRAGSTER frames (entries 44, 45, 46, 14) and six ZOOM ZOO frames
(entries 44, 14, 15), and named entries above 59, the voice range above 71 and the letters `j`, `k`,
`q`, `x`, `z` as unexercised. I chose a case that reaches the first of those.

I captured the original myself from the ROM through the audited core over the accepted manifest
`race-crawler-dragster-12000-continuous-right-fields.json` (12000 frames, Right held, no trick
input, so the hints are never cleared and the hint group reaches 5 and 6). A WRAM series of
`$0CA0-$0CFF` over the whole run gives 26 consumptions, reaching events **60-67** - `go on`,
`try it`, blank, `pull a stunt`, `press button b`, `to jump`, `and button r`, `to roll`. Entry 65
`        to jump ` contains **`j`**. I then captured frame images at five of those caption windows
and compared native's band against them:

| frame | event | entry | caption band (128x18 = 2304 px) |
| --- | --- | --- | --- |
| 2820 | 60 | `     go on      ` | **2304/2304 identical** |
| 3120 | 64 | `press button b  ` | **2304/2304 identical** |
| 3180 | 65 | `        to jump ` | **2304/2304 identical** (letter `j`) |
| 3240 | 66 | `and button r    ` | **2304/2304 identical** |
| 3300 | 67 | `        to roll ` | **2193/2304** - 111 pixels where the original shows `(255,38,38)` and native draws caption ink `(231,0,0)` |

Four of five reproduce the claim on entries and a letter the primary never exercised, which is real
independent support for the table, the arithmetic and the position. The fifth is blocking finding
B2 below.

The trigger rule holds on this case too, end to end: running the native engine over the same inputs
for all 12000 frames gives **26 consumptions** carrying exactly **events 37 and 44-67**, the same
count and the same set as the original's own WRAM series. The race finishes at update 3339 (event
37), which is why hint group 7 - entries 68-71 - is never reached.

I also extended the ZOOM ZOO comparison from the primary's six kept frames to **every** picture
frame of the accepted M4-16 original. Frames 6724, 6725 and 6800 - the `winner` caption, which the
primary never compared against a picture - are `2304/2304` identical, and frame 6800 is identical
over the **whole 256x224 frame** (0 of 57344 pixels differ). Frames 6900 and later differ over the
whole frame; that is the result screen, a declared omission, and not attributable to this change -
though note that native's `empty_display` stays 0 with event 37 held from update 6723 to the end of
that original, so native does draw `winner` in the caption band on its result screen. Nothing can be
concluded about whether the original does, because that screen differs entirely for other reasons.

## 3. The claims that are asserted rather than derived

**The tilemap base of `$1800` - I could confirm it, and it is now measured rather than inferred.**
R-0042 says the base is inferred from the word addresses. It need not be: an access record over the
race setup (frames 1240-1340 of the same manifest) shows `$82:DBF6` writing **`$2109 = $18`** on
frame 1249, which is BG3SC - tilemap base `$18 >> 2 << 10 = $1800` words, 32x32. The same routine
writes `$2105 = $19` (mode 1, BG3 priority high), `$2107 = $0C` (BG1 at `$0C00`), `$2108 = $73`
(BG2 at `$7000`, 64x64), `$210B = $12` and `$210C = $00` (BG3 character base 0). So the caption is
BG3, 2bpp, tilemap base `$1800`, and words 6472/6504 are rows 10 and 11, columns 8-23, exactly as
the record reasons. R-0042 can state this as measured.

**The ink as race CGRAM colour 22 - the index is right, and the primary's reasoning is honest about
what it does not know.** The index is empirically correct: with it, four DRAGSTER
frames of my own withheld case (2820, 3120, 3180, 3240), the ZOOM ZOO `winner` frames 6724, 6725 and
6800 - 6800 over the whole 256x224 frame - and every unoccluded glyph pixel of ZOOM ZOO 1649 and
DRAGSTER 1601 match the original exactly, across roughly 6000 frames of palette cycling. It is *not* derivable from the attribute, as
the review brief suspected: the layer is now known to be BG3 in mode 1, which is 2bpp, so palette 6
means CGRAM 24-27 and a pixel value of 3 (the only ink value in the sheet) means CGRAM **27**, which
in the DRAGSTER race palette is `$000d` = `(106,0,0)` - plainly not the caption's red. CGRAM 22 is
`$001c` = `(230,0,0)`, and CGRAM 16-28 is a red ramp that the palette cycle drives. The code comment
("the attribute-to-CGRAM derivation behind that index is not recovered") and R-0042's "measured from
the original's frames rather than derived from that attribute" are accurate and appropriately
modest. I could not close the gap either; see section 6.

**The start ring composing above the caption - I could not confirm it, and the measurement says it
is something else.** This claim is the task's and R-0042's entire excuse for the one imperfect
score, and it is wrong. At ZOOM ZOO 1649 and DRAGSTER 1601 the published channel-6 window member is
**4** on both tracks, and native paints `(255,206,206)` - `colour(cgram,0)` - over **9144** pixels of
the frame while the original paints it over **9260**. The difference is **116**: exactly the
differing pixels, exactly the caption glyphs. Native is not omitting an object there; it is drawing
the same member and then drawing the caption on top of it. See B2.

**Entries 1-255 as the pack's extent - right.** Entry 0 is not caption text, and an event id is a
byte, so 1-255 is the correct closed superset. The engine's own deserializer admits player queue
events below 88 and opponent events 200-215, so the pack is generous rather than wrong.

## 4. Findings

### Blocking

**B1. The renderer throws on caption entries the engine's own voice events can select, ending the
run.** `classic_caption_tile` accepts only `'a'`-`'z'` and `' '` and throws
`std::invalid_argument("unsupported Classic caption glyph")` on anything else, and
`draw_classic_caption` calls it for every non-space byte of the selected entry. Three entries in the
player's own voice range hold `"` (`$22`):

```
 75 |   thrashin"    |
 79 |    jammin"     |
 80 | head bangin"   |
```

These are reachable, not hypothetical. `update_zoom_landing_rewards` (`src/core/movement.cpp:1176`)
publishes `voice = 72 + (rider.motion.x & 15)` to the **player's** queue - twice - on any landing
whose trick combination is recognised, and 221 of the 625 entries of `zoom.trick-combinations` are
recognised (the simplest, index 3, is three completed rolls). The deserializer explicitly admits
player queue events below 88 (`movement.cpp:1825`), so this is inside the engine's own validated
domain. Three of the sixteen reachable voice ids crash the render. `sdl_main.cpp`'s `main` is a
function-try-block that prints and returns non-zero, so live play terminates.

Reproduction (self-contained, in this checkout):

```sh
python3 artifacts/classic-stunt-names-review/voice_caption_repro.py
# player announcement queue at byte 585 of the 742-byte state
#   event  72 |    rockin      | -> exit 0: rendered
#   event  75 |   thrashin"    | -> exit 1: unsupported Classic caption glyph
#   event  79 |    jammin"     | -> exit 1: unsupported Classic caption glyph
#   event  80 | head bangin"   | -> exit 1: unsupported Classic caption glyph
```

The script runs the DRAGSTER engine over the accepted 12000-frame manifest's inputs, writes the
voice id into the player's queue entry the read cursor points at on the update the renderer draws
from, and renders that frame with `classic_race_presentation_runner`. Nothing else is changed.

Note that event 72 *renders*: native draws `rockin` on screen. R-0042 records that `$81:C0CE-C18A`
diverts the range above 71 and explicitly leaves "whether they reach this display" out of scope, so
native is asserting behaviour in a domain the research says is unknown. Skipping events above 71 in
`draw_classic_caption` would both remove the crash and stop native claiming an unmeasured display;
if the primary would rather keep drawing them, it needs a measured frame of the original showing a
voice caption, and `classic_caption_tile` then needs a glyph for `"` (and `!`, which entries 88
upward use). Do not make the function silently skip an unknown glyph: that would draw a caption with
a hole in it.

Neither the fuzz (40 seeds, 382535 updates, 7696 renders) nor the hidden runs
(`--fixed-controller-mask 128`) reach a voice event, which is why the matrix is green. I also drove
the engine over 24 random-input ZOOM ZOO races and 24 deliberate jump/roll programs without reaching
one; it needs a landed trick combination, which those inputs do not produce. The fuzz renders every
64 updates and a voice caption stays up for at least the 40-update cooldown, so it would very likely
have aborted had it reached one - the gap is in what its input generation produces, not in its
render cadence.

**B2. The caption is composited above the channel-6 window effect; the original composites it
below.** `render_classic_race` calls `render_window_xor(...)` and then `draw_classic_caption(...)`
(`src/core/presentation.cpp:1716` and `:1723`). The comment immediately above the window call states
the accepted rule from R-0040 and ZOOM-ZOO-WINDOW-EFFECTS: "Every member covers both objects as well
as the backgrounds: inside the window the original shows the flat window colour and nothing else."
The caption is not excluded from "nothing else", and the original agrees.

Reproduction - frame 3300 of my withheld case, where a window member crosses the caption band:

```sh
python3 artifacts/classic-stunt-names-review/withheld_case.py --capture
# frame 3300 event  67 |        to roll |  band 2193/2304 identical;
#   original [(255, 38, 38)] where native draws [(231, 0, 0)]
```

The 111 differing pixels are the glyph pixels of `to ro` that lie under the member (band columns
131-161, rows 83-93). Drawn out, the
original's band shows the member as a solid block with the letters hidden, and native's shows the
same member with the letters punched through it:

```
original y88   ...........##.@@@@@@@@@@@@@@@@@@@@@@@@@@@@#..##..##......##
native   y88   ...........##.@@@##@@##@@@@@@@@@@#####@@@##..##..##......##
                             ^ the member, (255,38,38) = colour(cgram,0)
```

`classic_race_presentation_runner PACK --window-index` on the same timeline publishes member **14**
at frame 3300 (and member 20 at 3240, whose region misses the caption band, which is why that frame
still matches).

**The same bug is what the primary attributed to the start ring.** At ZOOM ZOO 1649 and DRAGSTER
1601 - the primary's own two imperfect frames - the published member is 4 on both tracks. Native
paints `(255,206,206)` over 9144 pixels of the frame and the original over 9260; the 116-pixel
deficit is precisely the 116 differing pixels, and the band drawn out shows the original covering
`more stunts` completely while native punches `n`, `t` and `s` through the member:

```
original y88  ................#######..##..##..#####...####.............#####...@@@@@@@@@@
native   y88  ................#######..##..##..#####...####.............#####...@##@@@@##@
```

So all three imperfect caption frames - DRAGSTER 1601, ZOOM ZOO 1649 and my 3300 - are this one
ordering error, and there is no declared omission involved in any of them. Correcting the order
should take every one of them to `2304/2304`. `window_colour` is `colour(cgram,0)`, the cycled backdrop, so the member's colour is
native's own - this is an ordering error, not an omitted object. Moving `draw_classic_caption` above the
`render_window_xor` call is the whole fix, and it is consistent with the caption being BG3 (section
3) rather than something drawn last.

This falsifies the task's acceptance criterion as written - "every pixel matches except where the
start ring, a declared omission, occludes the caption" - and R-0042's "Those are the only pixels of
any measured caption frame that differ". Both need correcting along with the code. The primary's ten
frames happened to have a clear band; a frame it did not choose does not.

### Should-fix

**S1. The documented command inventory still names the v8 pack, and every one of those commands now
fails.** `docs/BUILD_AND_VALIDATION.md` - which AGENTS.md sends every agent to for the implemented
command inventory - names `local/classic-crawler-two-tracks-v8.pack` in about fifteen commands and
states "profile classic.pal.crawler.two-tracks.v8, 56 entries" (lines 453, 454, 490-502, 529, 531,
570, 576, 594). `src/app/README.md:26` names `-v8.pack`, and `docs/STATE.md` lines 14 and 90 still
say v8/56 entries. With this change a v8 pack is refused outright:

```sh
python3 tools/project.py frontend run --track zoom-zoo --pack local/classic-crawler-two-tracks-v8.pack --preset app-debug --updates 20 --hidden
# [ failed] classic_pack (required): ... it records profile classic.pal.crawler.two-tracks.v8 and
#           this launch needs classic.pal.crawler.two-tracks.v9 ...
```

The primary updated the error string in `sdl_main.cpp` and `TWO_TRACK_PROFILE` in `pack.py` but not
the documentation. The rejection itself is correct and matches the v7 -> v8 precedent; the stale
inventory is not.

**S2. Nothing tracked exercises the new code.** `ctest` is 23 tests before and after, on all five
presets, and none of them touches `draw_classic_caption` or `classic_caption_tile` - the task's own
owned-path list includes `tests/native/presentation_tests.cpp` and the candidate does not change it.
The only checks on the caption are ad-hoc scripts in the primary's ignored worktree artifacts, which
neither `ctest` nor hosted CI runs. A caption regression would pass the whole matrix. B1 is a direct
consequence: a three-line unit test over the 255 table entries would have found it. At minimum the
glyph mapping and the `empty_display` rule deserve a tracked test.

**S3. The ZOOM ZOO picture score is claimed but not retained.** The acceptance table requires
"`caption_pictures.py` **and** `zoom_captions.py` ... on both tracks" with "picture scores in the
gate logs". `gates-a.sh` runs only `caption_pictures.py`; `zoom_captions.py` was written after that
script and there is no log of its output anywhere under `artifacts/classic-stunt-names/`. The ZOOM
ZOO half of the acceptance criterion rests on prose in the task record. I reproduced it myself
(section 1.3 and section 2), so the numbers are sound - but the artifact the criterion asks for does
not exist.

**S4. The task record's handoff is stale and its candidate hash is wrong.** The Handoff block still
reads "registered at `260334d`; no work started" and "Remaining dependencies: none" after eleven
recorded attempts and five commits, and its "Exact next experiment" is an experiment already done -
AGENTS.md requires the handoff to carry the actual commit, commands, results and next experiment so
a fresh agent can resume without the conversation. The Assignment block names the candidate as
`9e2570e`, but the candidate sent to review is `745c4b8`; the matrix in `gates-a.log`/`gates-b.log`
was in fact run at `9e2570e` (documentation-only on top, so the result carries, but the record
should say so). The attempts table also lists attempt 12 after attempt 14.

### Advisory

**A1. The "captions are then omitted" comment describes a fallback the code does not have.**
`presentation.hpp` says the caption span is "Empty only when the pack does not carry it; the
captions are then omitted, as they were before v9", and `draw_classic_caption` guards on
`content.captions.empty()`. `classic_race_presentation_content` fetches it with `pack.entry`, which
throws when absent, and the entry is a required member of the v9 profile, so the span is never
empty and a v8 pack never gets that far. The guard is dead and the comment is aspirational. (The
adjacent `window_tables` comment has the same shape, so this is a pre-existing house habit, not a
new one.)

**A2. `event > 255` is unreachable.** `RewardQueueState::entries` is `std::array<std::uint8_t,32>`,
so the guard can never fire. Harmless, but it reads as though a wider domain were being handled.

**A3. `classic_caption_tile` and `draw_classic_caption` have external linkage and no declaration.**
Neither appears in `presentation.hpp` nor in an anonymous namespace. They are only used inside
`presentation.cpp`; `static`/anonymous-namespace would match the file's other helpers and keep the
symbol out of the library.

**A4. `-1` as the space sentinel from an `int`-returning tile function.** `std::optional<unsigned>`
would say what it means and remove the `int`/`unsigned` mixing in the draw loop. D-0003 readability
only; the arithmetic itself is fine and `(event-1U)*16U` stays inside the 4080-byte span for every
admissible event.

**A5. The ink index is pinned while the palette around it cycles.** CGRAM 16-28 is a red ramp driven
by `presentation.zoom.race-palette-cycle.v1`, and `draw_classic_caption` takes a fixed index 22 out
of the cycled CGRAM. That matched on every frame I measured, from ZOOM ZOO 1649 to 7600 and DRAGSTER
2820 to 3300, so the phase behaviour is right for those; but because the index is measured rather
than derived, a caption held across a cycle phase the measured frames miss is not covered.

## 5. No accepted contract moved

Everything below ran at `745c4b8` from this checkout, on the v9 pack, with a clean tracked tree
(`dirty 0` in both gate headers; the only untracked addition is this report). The compares were run
one at a time, as the differential gates require.

The eleven accepted differential gates (`gates-diff.log`, 12:08:49Z - 12:43:12Z):

| gate | result | restores | primary's restores |
| --- | --- | --- | --- |
| m4-16-primary | passed | 757 | 757 |
| m4-16-idle (late start) | passed | 801 | 801 |
| opposing-ride | passed | 801 | 801 |
| opposing-axes | passed | 781 | 781 |
| opposing-edges | passed | 757 | 757 |
| dragster-primary | passed | 379 | 379 |
| dragster-random-1 | passed | 567 | 567 |
| dragster-reversal | passed | 327 | 327 |
| dragster-random-3 | passed | 493 | 493 |
| dragster-regression-landing-held-roll | passed | 181 | 181 |
| dragster-regression-countdown-actions-tie | passed | 179 | 179 |

Every restore count is identical to the primary's `gates-b.log`. The suites (`gates-suite.log`,
12:43:28Z - 12:48:43Z):

- Five preset builds and `ctest`: **23/23 on all five** (lab-debug, lab-release, lab-sanitize,
  app-sanitize, app-debug). Unchanged from `main`, and unchanged in *count* - see S2.
- Synthetic suite (lab-debug): `status=passed`, 50.4s.
- Both v1 presentation contracts on the legacy DRAGSTER renderer with the `dragster.v1` pack:
  `winner status=passed`, `loser status=passed`. The v1 contracts did not move.
- Hidden app runs on the v9 pack, 4000 updates each: DRAGSTER and ZOOM ZOO both `rc=0`, rider-pose
  fallback frames **0**.
- Fuzz (lab-release, v9 pack, 40 seeds): `rc=0`, 382535 updates, 79 completed races, 1242 pause
  restarts, 7696 renders, **0 aborts** - byte-identical summary to the primary's.
- Pack profile handling: a v8 pack is refused at launch with the profile message and exit code 3,
  which is the same behaviour the v7 -> v8 bump had. The v9 pack rebuilds byte-identically from the
  ROM and is additive over v8 (section 1.2).

So the code change moves no accepted contract, and I found no changed baseline, weakened
comparison, masked skip, undefined arithmetic or accidental private content. The candidate's diff is
eight files, no binaries, and the pack rules carry only offsets and digests.

## 6. What remains unverified

- **Why the ink is CGRAM 22.** The layer is now known to be BG3 (2bpp) and the attribute is palette
  6 with priority, which should mean CGRAM 27. It is not. CGRAM 16-28 is a cycled red ramp; I could
  not derive 22 from anything and neither could the primary. The index is empirically right on every
  frame either of us measured.
- **Whether the original displays a caption for the voice range 72-87 at all.** No capture reaches
  one, so R-0042's "the voice lines the consumer diverts above 71" is untested as a *display* claim,
  and native's decision to draw them is unsupported either way. This is the domain B1 sits in.
- **The remaining unexercised letters.** My withheld case adds `j` - and entry 65 is the only entry
  below 72 that contains one, so `j` is now fully covered. Checking the whole table: **`q` occurs in
  no entry at all** (so it can never be exercised, and R-0042's list should say so), `x` occurs only
  in entries 73 and 158, both in the voice range, `k` occurs below 72 only in entry 32
  (`invisible track`, a cheat message) and entry 69 (`  easy you know `, hint group 7), and `z`
  occurs below 72 only in the `z flip` family, entries 18-21. Entry 69 needs a race that keeps its
  hints past update ~2050; the 12000-frame DRAGSTER race finishes at 3339, so I could not reach it.
  `z` needs a landed z-flip, which I could not produce from scripted input.
- **The identity of the objects behind the window members.** I established that the pixels the
  primary attributes to "the start ring" are channel-6 member 4, which native draws; I did not
  establish what that member depicts.
- **Whether a caption belongs on the result screen.** Native holds event 37 with `empty_display` 0
  from update 6723 to the end of the M4-16 original and draws `winner` in the band throughout;
  frames 6900 onward differ over the whole frame for declared reasons, so the question is open.
- **Hosted CI on the final tip.** Not run by me; the closeout requirement is unchanged.
- **The primary's `pictures-a` DRAGSTER capture itself.** I re-rendered against it with my own build
  rather than recapturing that exact case; my independent DRAGSTER original is the 12000-frame
  manifest of section 2.

## 7. What the primary needs to do

1. Fix B1 - decide what a voice event should display and make `classic_caption_tile` total over
   whatever entries can reach it. Do not silently skip unknown glyphs.
2. Fix B2 - move `draw_classic_caption` above the `render_window_xor` call, then re-measure
   DRAGSTER 1601, ZOOM ZOO 1649 and my frame 3300; all three should reach `2304/2304`.
3. Correct the "start ring, a declared omission" explanation in the task record, in R-0042's
   "Domain and limits" and in the comment above `draw_classic_caption`. It is not an omission and
   it is not the start ring.
4. S1-S4: refresh the v8 pack references in `docs/BUILD_AND_VALIDATION.md`, `src/app/README.md` and
   `docs/STATE.md`; add a tracked test over the glyph mapping and the `empty_display` rule; retain a
   ZOOM ZOO picture score in the gate logs; bring the handoff and the candidate hash up to date.
5. Re-run the matrix on the corrected candidate and request re-review.

Reviewer scripts and reports for every number above are in this checkout's ignored
`artifacts/classic-stunt-names-review/`.
