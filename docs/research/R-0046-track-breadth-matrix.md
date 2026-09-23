# R-0046 - Every track through the shared engine: the matrix

Status: part 1 (inventory, producers, playfield shapes and the native idle matrix),
23 September 2026, [TRACK-BREADTH](../../tasks/TRACK-BREADTH.md). PAL ROM
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`. The reference
and match columns are not measured yet; no track beyond the two already accepted is
claimed to match the original. Static readings come from the
[R-0045](R-0045-static-code-map.md) listing and are labelled as such under
[D-0008](../decisions/D-0008-static-map-track-breadth-review-tiers.md).

## Observations

Observations 1, 2 and 4 are verified by execution; 3 is verified for the header
values and the `$00`/`$40` arms and is a static reading for the other arms; 5 is a
static reading.

1. **Track *i* is asset `$C2 + i`.** `$82:E140-E152` loads the byte at SRAM
   `$77:074A`, adds `$C2` and calls `$82:B2DD` with it (observed in the race
   captures). `$82:B2DD` resolves a five-byte entry of the asset directory at
   `$82:B332` (R-0008 finding 3). Entries `$C2`-`$EE` are the only compressed
   entries in the run (`$C1` and `$EF` are uncompressed), and they point, in
   order, at the 45 `RNC\x01` headers a scan of the ROM finds, from `$98:8000` to
   `$9F:BB4C`. Every stream decodes with the register-level port of `$81:B8E2`,
   produces exactly the unpacked length its header states and consumes exactly
   the length its directory entry states. Streams 0 and 1 equal the pack's
   DRAGSTER and ZOOM ZOO track data. `content rnc-inventory` reproduces this and
   the tracked manifest
   [track-streams.json](../../tests/manifests/content/track-streams.json)
   (locations, header fields and digests, no payload bytes) byte for byte on two runs.
2. **The per-track tile producer is general.** Walking a track's tile-set list
   (header bytes 11-12, up to `$FF`) through the tile directory at `$82:B7DD` and
   the table directory at `$17:A000` (R-0008 finding 4, R-0021) reproduces all
   eight per-track pack entries of the two accepted tracks byte for byte: DRAGSTER's
   track data, tile columns (640), tile flags (20) and BG1 tiles (2,560), and ZOOM
   ZOO's (50,665, 6,496, 203, 25,984). Every one of the 45 lists terminates and
   resolves.
3. **Six playfield shapes.** Header byte 13 is a quarter of the column count
   (mod 256). Across the 45 tracks it takes six values:

   | Byte 13 | Arm of `$81:A304` | Columns x rows | Tracks | Arm observed under capture |
   | --- | --- | --- | ---: | --- |
   | `$00` | `$81:A4C1` | 1,024 x 16 | 4 | yes (DRAGSTER) |
   | `$80` | `$81:A483` | 512 x 32 | 3 | no |
   | `$40` | `$81:A445` | 256 x 64 | 22 | yes (ZOOM ZOO) |
   | `$20` | `$81:A406` | 128 x 128 | 11 | no |
   | `$10` | `$81:A3C7` | 64 x 256 | 4 | no |
   | `$08` | `$81:A388` | 32 x 512 | 0 | no |
   | `$04` | `$81:A343` | 16 x 1,024 | 1 (track 37) | no |

   Header byte 14 is a quarter of the row count (mod 256) on all 45 tracks, which
   agrees with the table. Every arm stores the same fields with a regular progression (`$0D4F` =
   columns x 64 - 1, `$03F1` the screen shift 0-6, `$03F3/$03F5` the follow window
   and `$0425/$0427` the visible span doubling per step). The five arms other
   than `$00` and `$40` are **static readings**: the static map left four of
   them as unknown bytes, and the table above was decoded by hand from them. The
   `$04` arm also stores 1 in `$0FF7`, which the sampler (`$81:8A2C`, a clamp of a
   negative row) and the BG1 map fetch (`$81:AD1D-ADA7`) test. Any other value
   falls through to a `BRK` at `$81:A342`.
4. **Native idle matrix** (`content track-idle-matrix --updates 1200`, declared
   before the run: the 270-update countdown and 930 of riding). Each track runs
   from `classic_race_start` on the ZOOM ZOO scenario with the v9 pack's engine
   tables and its own track data, tile columns and tile flags
   (`zoom_zoo_runner --track-override`), the controller released throughout. The
   override with ZOOM ZOO's own bytes reproduces the plain pack run (1,200 rows,
   identical digest), and two runs of the matrix are identical.

   | Result | Tracks |
   | --- | --- |
   | completes 1,200 updates (16) | 0, 1, 12, 13, 14, 19, 21, 23, 24, 29, 30, 31, 33, 36, 41, 44 |
   | stops at `vertical contact reaches a special response tile` (19); in parentheses the updates completed before the stop | 2 (7), 3 (383), 4 (360), 6 (760), 9 (700), 11 (530), 16 (403), 17 (220), 18 (525), 25 (451), 26 (444), 27 (301), 28 (560), 34 (301), 38 (604), 39 (1,168), 40 (243), 42 (17), 43 (525) |
   | stops at `unrecovered coarse-grid edge branch` (9); updates completed as above | 5 (330), 7 (738), 8 (562), 10 (359), 15 (461), 20 (452), 22 (776), 32 (459), 35 (438) |
   | refused in its first update: shape `$04` (1) | 37 |

   Both stops are native guards on branches no accepted track reached: the
   response to a tile flag outside {0, 2, 6, 7, 18, 20} in vertical contact
   (`src/core/vertical_contact.cpp`), and the sampler at the right column edge or a
   negative row (`src/core/track_sampling.cpp`; the listing shows the original's
   negative-row clamp at `$81:8A31-8A3B`, gated by `$0FF7`). The tile tables of the
   45 tracks hold the unrecovered flag values 1, 4, 8, 10, 12, 14, 16, 25, 27 and
   28; which one each stopping rider touched is not yet measured.

5. **Track names (static reading).** Bank `$83` holds 45 `$FF`-terminated track
   names in lowercase ASCII from `$83:9FFA` (DRAGSTER, ZOOM ZOO, BOWL, SWITCHER,
   MONSTER, WOBBLE, ...), followed by five `unavailable` entries and nine tour names
   (CRAWLER, JUMPER, SHUFFLER, BOUNDER, WALKER, RUNNER, HOPPER, SPRINTER, HUNTER).
   Names 0 and 1 agree with the two tracks whose index is verified, and the first
   five agree with the Crawler PICK TRACK screen (R-0006 finding 8). That the name
   table is indexed by track number for the rest is not yet observed; the table
   below uses it as a label.

## The matrix

| Track | Name (static) | Tour (hypothesis) | Stream | Unpacked | Shape | Idle 1,200 updates (native) | Reference | Match |
| ---: | --- | --- | --- | ---: | --- | --- | --- | --- |
| 0 | DRAGSTER | CRAWLER 1 | $98:8000 | 33,815 | 1024x16 | completes | DRAGSTER-ORDINARY-CONTROLS originals | exact (six frozen gates) |
| 1 | ZOOM ZOO | CRAWLER 2 | $98:8183 | 50,665 | 256x64 | completes | M4-16 and opposing-input originals | exact (five frozen gates) |
| 2 | BOWL | CRAWLER 3 | $98:9B4A | 35,810 | 256x64 | special-tile guard after 7 updates | not yet | not yet |
| 3 | SWITCHER | CRAWLER 4 | $98:A07E | 63,013 | 1024x16 | special-tile guard after 383 updates | not yet | not yet |
| 4 | MONSTER | CRAWLER 5 | $98:C70E | 52,234 | 128x128 | special-tile guard after 360 updates | not yet | not yet |
| 5 | WOBBLE | JUMPER 1 | $98:E62E | 47,619 | 64x256 | edge guard after 330 updates | not yet | not yet |
| 6 | TWINPEAK | JUMPER 2 | $98:FEDB | 48,168 | 256x64 | special-tile guard after 760 updates | not yet | not yet |
| 7 | SKIER | JUMPER 3 | $99:977C | 35,617 | 128x128 | edge guard after 738 updates | not yet | not yet |
| 8 | LOOPBACK | JUMPER 4 | $99:9CD3 | 56,372 | 512x32 | edge guard after 562 updates | not yet | not yet |
| 9 | SMALL CUT | JUMPER 5 | $99:C1DF | 45,036 | 256x64 | special-tile guard after 700 updates | not yet | not yet |
| 10 | LOOPER | SHUFFLER 1 | $99:D307 | 63,397 | 64x256 | edge guard after 359 updates | not yet | not yet |
| 11 | MEGAJUMP | SHUFFLER 2 | $9A:81D9 | 45,899 | 256x64 | special-tile guard after 530 updates | not yet | not yet |
| 12 | JUMPS | SHUFFLER 3 | $9A:96AC | 38,625 | 128x128 | completes | not yet | not yet |
| 13 | FLAT FUN | SHUFFLER 4 | $9A:A206 | 49,156 | 512x32 | completes | not yet | not yet |
| 14 | INFINITY | SHUFFLER 5 | $9A:BBEC | 38,219 | 256x64 | completes | not yet | not yet |
| 15 | LAST ONE | BOUNDER 1 | $9A:C3FC | 54,382 | 256x64 | edge guard after 461 updates | not yet | not yet |
| 16 | MARATHON | BOUNDER 2 | $9A:E545 | 53,331 | 256x64 | special-tile guard after 403 updates | not yet | not yet |
| 17 | CIRCLE | BOUNDER 3 | $9B:838B | 34,490 | 256x64 | special-tile guard after 220 updates | not yet | not yet |
| 18 | PLINKEY | BOUNDER 4 | $9B:8668 | 49,258 | 128x128 | special-tile guard after 525 updates | not yet | not yet |
| 19 | JUMPOVER | BOUNDER 5 | $9B:9F15 | 52,178 | 256x64 | completes | not yet | not yet |
| 20 | DRAGRACE | WALKER 1 | $9B:BCED | 42,663 | 512x32 | edge guard after 452 updates | not yet | not yet |
| 21 | PINGPONG | WALKER 2 | $9B:CDCB | 46,893 | 256x64 | completes | not yet | not yet |
| 22 | HILL CLIMB | WALKER 3 | $9B:E38D | 34,622 | 128x128 | edge guard after 776 updates | not yet | not yet |
| 23 | HYBRID | WALKER 4 | $9B:E720 | 47,906 | 256x64 | completes | not yet | not yet |
| 24 | SHORT CUT | WALKER 5 | $9B:FF01 | 47,078 | 256x64 | completes | not yet | not yet |
| 25 | DOWN+UP | RUNNER 1 | $9C:9454 | 50,764 | 128x128 | special-tile guard after 451 updates | not yet | not yet |
| 26 | HIGHROAD | RUNNER 2 | $9C:AF5E | 47,215 | 128x128 | special-tile guard after 444 updates | not yet | not yet |
| 27 | SPINE | RUNNER 3 | $9C:C5B8 | 39,267 | 256x64 | special-tile guard after 301 updates | not yet | not yet |
| 28 | BOO! | RUNNER 4 | $9C:D0D8 | 53,001 | 128x128 | special-tile guard after 560 updates | not yet | not yet |
| 29 | FIRE ESCAPE | RUNNER 5 | $9C:ED41 | 44,483 | 256x64 | completes | not yet | not yet |
| 30 | WARIO PAINT | HOPPER 1 | $9C:FB63 | 49,932 | 1024x16 | completes | not yet | not yet |
| 31 | CROCK | HOPPER 2 | $9D:9405 | 52,171 | 256x64 | completes | not yet | not yet |
| 32 | DOWNER | HOPPER 3 | $9D:B1DA | 34,497 | 64x256 | edge guard after 459 updates | not yet | not yet |
| 33 | EAST | HOPPER 4 | $9D:B5D4 | 48,099 | 1024x16 | completes | not yet | not yet |
| 34 | HAIRPIN HILL | HOPPER 5 | $9D:CD3F | 43,340 | 256x64 | special-tile guard after 301 updates | not yet | not yet |
| 35 | VERTICAL | SPRINTER 1 | $9D:DE54 | 57,267 | 64x256 | edge guard after 438 updates | not yet | not yet |
| 36 | FLASH | SPRINTER 2 | $9E:8241 | 41,512 | 128x128 | completes | not yet | not yet |
| 37 | LITTLE DIPPER | SPRINTER 3 | $9E:907B | 35,364 | 16x1024 | refused in its first update: shape | not yet | not yet |
| 38 | FRUITBAT | SPRINTER 4 | $9E:952D | 47,242 | 256x64 | special-tile guard after 604 updates | not yet | not yet |
| 39 | 123 JUMP | SPRINTER 5 | $9E:AAED | 51,273 | 256x64 | special-tile guard after 1168 updates | not yet | not yet |
| 40 | GRILLER | HUNTER 1 | $9E:C756 | 65,354 | 128x128 | special-tile guard after 243 updates | not yet | not yet |
| 41 | TWO LOOPS | HUNTER 2 | $9E:FC2C | 43,463 | 256x64 | completes | not yet | not yet |
| 42 | NEON | HUNTER 3 | $9F:8CD5 | 48,453 | 256x64 | special-tile guard after 17 updates | not yet | not yet |
| 43 | HAMSTER | HUNTER 4 | $9F:A098 | 49,607 | 128x128 | special-tile guard after 525 updates | not yet | not yet |
| 44 | TO AND FRO' | HUNTER 5 | $9F:BB4C | 44,592 | 256x64 | completes | not yet | not yet |

"Completes" means only that no native guard fired with the controller released
on ZOOM ZOO's scenario; it is not a comparison with the original.

## Hypotheses and limits

- **Tours.** Header byte 2 is `$2D` on exactly tracks 2, 7, 12, ..., 42 and 0
  elsewhere, the menu lists five Crawler tracks (R-0006 finding 8), and the ROM
  names nine tours. That fits nine tours of five tracks, index = 5 x tour +
  position, with a different kind of track third in each tour. Not verified:
  confirm with NOW PLAYING per track.
- **`$0FF7` may outlive track 37.** In banks `$80`-`$83` its only writer besides the
  power-on clear (`$80:92F7`) is the `$04` arm, which stores 1. If no race setup
  clears it, every track raced after LITTLE DIPPER in the same power-on session
  skips the negative-row clamp at `$81:8A31`, and the engine's six arms (which
  assume 0) would not describe it. Not yet checked; found by the part 1 review.
- "Completes 1,200 updates" means only that no native guard fired with the
  controller released. It is **not** a match with the original. The scenario
  (laps, race mode, initialization frame) is ZOOM ZOO's for every track, which
  the original will not use on most of them.
- The landing-response matrices (`zoom.landing-response-matrices`) are a
  captured input. Both accepted tracks use the same bytes, and R-0030 says they
  are built from ROM coefficient words by `$81:9A4D-9E12`; whether any track
  changes them is not yet measured.

## Next experiments

1. A reference capture per track through the menu, released controller, with
   consecutive frames across race start: Down on PICK TRACK reaches Crawler
   tracks 2-4 from a cold start; other tours may be locked. Record NOW PLAYING
   (rider, opponent, laps, track name) to fix the scenario and confirm the tour
   hypothesis.
2. A generic projection of the 742-byte state from a capture of any track (the
   existing `zoom_zoo_playable` reads only the M4-16 manifest), then the match
   column: exact updates from race start and the first divergence.
3. The two guards: a watch capture on track 2 (stops at update 7) and track 5
   (330) names the flag value and the edge case each hits.

## Reproduction

```sh
python3 tools/project.py content rnc-inventory --expect tests/manifests/content/track-streams.json
python3 tools/project.py build --preset lab-debug
python3 tools/project.py content track-idle-matrix --out artifacts/track-breadth/idle-1200 --updates 1200
```
