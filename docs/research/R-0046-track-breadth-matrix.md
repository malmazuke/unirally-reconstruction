# R-0046 - Every track through the shared engine: the matrix

Status: part 1 (inventory, producers, playfield shapes and the native idle matrix) and
part 2 (references for the 20 tracks a cold start reaches, and the match column) and
part 3 (native selection of those race tracks by id, pack profile v10), 23 September 2026, [TRACK-BREADTH](../../tasks/TRACK-BREADTH.md). PAL ROM
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`. Four tracks beyond
the two accepted ones match the original exactly over about 1,500 updates with the
controller released; that is a laboratory measurement, not the frozen-gate acceptance
the two accepted tracks carry. Since part 3, the fourteen cold-start race tracks beyond
the first two start natively by id, from the pack, on their own scenario. Static readings come from the
[R-0045](R-0045-static-code-map.md) listing and are labelled as such under
[D-0008](../decisions/D-0008-static-map-track-breadth-review-tiers.md).

## Observations

Observations 1, 2, 4, 6-11 and 13-16 are verified by execution. Observation 3 is
verified for the header values and, since part 2, for every arm but `$08` and `$04`.
Observation 5 is a static reading, confirmed for the 20 reachable tracks by
observation 6. Observation 12 is verified for palette classes 0, 1 and 2 (1 through
FLAT FUN's pictures, observation 15).

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
   | `$80` | `$81:A483` | 512 x 32 | 3 | yes since part 2 (FLAT FUN, observation 10) |
   | `$40` | `$81:A445` | 256 x 64 | 22 | yes (ZOOM ZOO) |
   | `$20` | `$81:A406` | 128 x 128 | 11 | yes since part 2 (MONSTER) |
   | `$10` | `$81:A3C7` | 64 x 256 | 4 | yes since part 2 (LOOPER) |
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

   Since R-0047 and R-0048 the same matrix completes on 41 of the 45 tracks; the four stops
   are listed in R-0048.

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

### Part 2: references and the match column

6. **Twenty tracks are reachable from a cold start, and their indices are observed.**
   PICK TOUR lists CRAWLER, SHUFFLER, WALKER and HOPPER (tours 0, 2, 4 and 6 of the
   name table), each opening a PICK TRACK screen of five tracks. `track_reference sweep`
   drives the accepted ZOOM ZOO menu prefix with one Down per PICK TOUR row and per
   PICK TRACK position, then releases the controller. On all 20 captures SRAM
   `$77:074A` at the boundary equals 10 x row + position, the name at that index is the
   one NOW PLAYING shows (read on tracks 2, 3, 4, 21 and 33, and on the SHUFFLER list),
   and a second fresh capture is identical in WRAM, SRAM and video. For ZOOM ZOO the
   generic path reproduces the M4-16 idle original byte for byte over frames 1207-2649.
7. **Three race kinds, one per race mode.** SRAM `$77:074B` is 0 for "RACING ON ..."
   (one run, like DRAGSTER), 1 for "OVER n LAPS ON ..." with n in `$77:0744` (3, 5 or
   7 here), and 2 for "DOING STUNTS ON ...", a solo event with a qualifying score and
   no opponent. The stunt events are exactly the tracks with header byte 2 = `$2D`
   (2, 12, 22, 32 among those captured). The opponent is BRONSEN in every race.
8. **The initialization boundary differs per track** (1,328 to 1,419 across the 20),
   found as the end of the frame before fade `$0FF1` first advances with countdown
   `$11C5` at 270; the rule gives 1,328 and 1,376 on the two accepted tracks.
9. **The match column** (the matrix above). Six tracks match the original exactly over
   the whole captured window (horizon frame 2,900, about 1,500 updates): DRAGSTER,
   ZOOM ZOO, FLAT FUN, WARIO PAINT, CROCK and EAST. Seven match exactly until a native
   guard stops (special-tile: SWITCHER 384, MONSTER 361, MEGAJUMP 531, SHORT CUT 1,483;
   edge: LOOPER 360, DRAGRACE 453, HYBRID 1,326). PINGPONG matches 1,068 updates and
   then differs in `opponent.response_b`; it is the only compared race in which a
   state field differs before a native guard stops (native later stops at the
   special-tile guard after 1,260 rows). INFINITY (7 laps) and HAIRPIN HILL (5 laps) differ from update 0 only in the
   two `laps_remaining` fields: with those masked, HAIRPIN HILL matches all 302 native
   updates and INFINITY matches until its first lap crossing at update 379, where the
   lap count's consequence (`checkpoint_seen13`) follows. The four stunt events are not
   compared. Counts are rows from the boundary row inclusive, so "exact 384" is the
   boundary state and 383 updates, the same stop the idle column gives as "after 383
   updates".

   The original also leaves the accepted ZOOM ZOO guard domain on new tracks, and the
   sweep records where (`guard_violations`). Most are the shape and mode constants, or
   special-tile state one update after a native guard fires, which supports those
   stops. Two fall inside windows reported exact: the opponent's X press (`$0323`) on
   HYBRID from row 348, and its A press (`$031F`) on SHORT CUT from row 953. The
   accepted guard inventory held both at 0 on every frame, so these matches are the
   first time native's opponent A/X path meets the original (part 2 review).
10. **Three static arms are now observed.** The captured `$03F1`, `$03F3`, `$03F5`,
    `$0425`, `$0427` and `$0D4F` equal the part 1 constants on MONSTER (`$20`, 128 x
    128), LOOPER (`$10`, 64 x 256) and FLAT FUN (`$80`, 512 x 32), and those tracks
    match update for update while they run. Only `$08` (no track) and `$04` (LITTLE
    DIPPER, not reachable) remain static readings.
11. **The captured landing matrices are track-independent across the reachable set.**
    WRAM `$0572-$0B59` at the boundary equals the pack's
    `zoom.landing-response-matrices` on all 20 captures, stunt events included.

### Part 3: native selection by track id

12. **The loader's scenery producer reproduces the accepted presentation entries.**
    `$82:DC20-DD84` takes scenery s = track mod 14 (track 42, NEON, has an extra
    case not used here). BG2 tiles are asset `$70 + s` (VRAM `$1000`), the
    BG2 map `$82 + s` (VRAM `$7000`), and a palette row `$93 + s` (CGRAM `$70`). A
    six-row palette block follows, chosen by the class byte at `$82:DC12 + s`:
    class 0 is assets `$A4-$A9` (observed), class 2 is `$AA-$AF` (observed), and
    class 1 is `$B0-$B5` (read statically, then observed through FLAT FUN's pictures).
    Four more rows complete the palette. They are the same on both accepted tracks,
    and the loader chooses them by the rider selection (`$77:0748`/`$0749`), which is
    MIKE throughout.
    Generated as pack pieces, these give byte for byte the v9 entries of DRAGSTER (s
    0, class 0) and ZOOM ZOO (s 1, class 2): BG2 tiles, map and the 352-byte palette.
    With observation 2, all seven per-track presentation and engine entries of both
    accepted tracks now come from general producers.
13. **Pack profile v10** (`classic.pal.crawler.tracks.v10`, 147 entries). It keeps
    v9's 57 entries unchanged and adds, for each of the fourteen other cold-start race
    tracks, `track.NN.data`, `.tile-columns`, `.tile-flags` and `.bg1-tiles`. It adds
    `scenery.SS.bg2-tiles`, `.bg2-map` and `.palette` for the eleven sceneries those
    tracks use, and the name table (`$83:9FFA`). `tracks.v10_new_entries` generates
    the added entries, and a unit test holds the loader's compiled table equal to the
    rules file.
14. **Each track's own scenario matches the original, from the pack.** Native starts
    any of the fourteen with `classic.track.NN` (runner) or `--track NN` (app). It
    uses the observed race mode, lap count and initialization frame, and takes the
    track's content from the pack with no laboratory override.
    `track_reference recompare --per-track` on the part 2 captures gives:
    - FLAT FUN, WARIO PAINT, CROCK and EAST stay exact over their whole windows.
    - HAIRPIN HILL (5 laps) is now exact for all 302 rows before its special-tile guard.
    - INFINITY (7 laps) is exact for 379 rows. At its first lap crossing native stops
      at a new guard, `race checkpoint index invalid`: a checkpoint layout the two
      accepted tracks never showed.
    - Every other row is unchanged from observation 9.
15. **Pictures.** Native frames rendered by `classic_race_presentation_runner
    --timeline` were compared with the original's own frames, whole 256 x 224 picture.
    - At six sampled race frames (updates 20 to 900), FLAT FUN, WARIO PAINT and CROCK
      differ by 0 to 36 pixels, and the accepted ZOOM ZOO by 0 to 72.
    - Sampled frames hid a real fault. The part 3 review found the GO letters swapped,
      18,739-19,191 pixels per frame over updates 205-285, on the six tracks whose
      boundary frame is odd. The window drivers took `$0300`'s parity from the
      absolute frame, but `$0300` counts from race start.
    - After the fix, every consecutive frame of updates 190-300 on CROCK, LOOPER and
      EAST (odd boundaries), and on FLAT FUN and ZOOM ZOO (even), shows the same
      profile as ZOOM ZOO: 36 pixels or none on every frame, one frame of 72 on
      LOOPER, and 470 at update 272 on all five, ZOOM ZOO included.

      The comparison covers the race and the countdown only. No new-track finish,
      winner banner or result screen has been captured.
16. **The sampler's playfield edges are recovered** (`$81:8A2A-8AC4`, read from the
    listing, confirmed against the original). With `$0FF7` clear, a negative y samples
    coarse cell (0, 0). In the last column the right-hand neighbours are column 0 of
    the same two rows, so the playfield wraps horizontally. Both replace the native
    guard `unrecovered coarse-grid edge branch`, and each is exercised:
    - LOOPER and HYBRID (opponent in the last column, 63 of 64 and 255 of 256) now
      match to the end of their windows: 1,484 and 1,515 rows.
    - DRAGRACE (opponent y `$FFFB`) matches 1,353 rows, 900 past its old stop, then
      reaches the special-tile guard.

    **Eight tracks now match the original over the whole captured window**: DRAGSTER,
    ZOOM ZOO, LOOPER, FLAT FUN, HYBRID, WARIO PAINT, CROCK and EAST.
17. **The BG1 picture wraps horizontally, as the sampler does** (found by the user
    in live play on LOOPER: near the end of a race the track seemed to vanish below
    the rider).
    - **Setup.** Captured LOOPER holding Right from frame 1,688 (race update 271).
      Native matches the original on all 1,284 rows to frame 2,700. The player rides
      to x = 4,086 on the 4,096-unit-wide playfield, and the camera to about 3,900.
    - **Before the fix.** Native drew nothing past the playfield's right edge. At
      frame 2,617 it differed from the original by 1,790 pixels, 1,718 of them in
      screen columns 176-255, where the original continues the floor from column 0.
      The original's map fetch masks the column with `$0D51` (`$81:AD05`).
    - **The fix.** Native now masks BG1's world x with the playfield mask. Over 200
      consecutive frames at the edge (updates 1,083-1,282), 198 frames differ by 0,
      36, 72 or 108 pixels: the declared off-screen rider arrow. Two differ by 549,
      where native's hint caption differs from the original's for one frame. The
      review found this repeats every 60 updates (LOOPER 1,172, 1,232, 1,292 and on,
      and HYBRID 1,352), and is sometimes the next hint sentence one frame early
      rather than a caption the original lacks. The caption state is the engine's,
      which matches, so its display timing is an open presentation question that
      predates this fix.
    - **A second track.** The review withheld HYBRID (256 x 64). With Right held its
      player wraps from x 16,172 to 36, and native matches all 1,385 rows. Frames
      2,720-2,736 differed by up to 3,486 pixels before the fix and by 0 after.
    - **Scope.** The vertical rule is unchanged: native blanks rows off the playfield.
      The original's row fetch (`$81:AD22-AD2C`) tests the screen's bottom row
      against the playfield height whichever way the camera scrolls; no capture
      reaches that case. DRAGSTER and ZOOM ZOO, whose cameras never reach an edge in
      the accepted races, are unchanged: the v1 contracts and hidden runs pass.
18. **A one-run track's result title is its own name, centred** (found by the user
    in live play: EAST and LOOPER showed DRAGSTER).
    - **The cause.** The result title came from the 16 bytes the DRAGSTER result
      assets carry at `$83:9FFA`, the first entry of the name table.
    - **The fix.** Native now takes the track's own entry from the pack's name table,
      and centres it by its width in tiles (two per letter, one per underscore):
      start tile = 16 - width / 2. That gives DRAGSTER tile 8 as before, EAST 12 and
      FLAT FUN 9. The rule is fitted to those three names.
    - **Setup.** Captured EAST holding Right from race update 271. The player finishes
      at frame 5,581 and wins, and native matches the original on all 4,178 rows to
      that finish.
    - **Result.** On the stable result screen (frames 6,200, 6,400 and 6,598) the
      title reads EAST at the original's position, and no title-glyph pixel differs.
      The remaining 26,500 or so differing pixels are the checkerboard shading and the
      result sprites (1P arrow, unicycles, medals). DRAGSTER's accepted result screen
      shows the same residue under the same check: 26,244-26,376 pixels, 4,417 in the
      title rows against EAST's 4,746.
    - **The title font.** Its layout is recovered from the 12 letters observed in
      "dragster" and "complete": digits 0-9 at 2 x digit, "o" sharing the zero,
      a-n from `$14` and p-z two tiles lower. The review withheld FLAT FUN (Right
      from frame 1,663, then Left from 3,760; native exact on all 4,269 rows to the
      finish). Its result confirmed f, u and n, and showed that an underscore is a
      one-tile space. After that fix FLAT FUN's title rows differ by 0 pixels on the
      frames where the checkerboard is in phase (6,200 and 6,480). Letters b, h, i,
      j, k, q, v, w, x, y and z, and the digits, remain the layout's reading, never
      compared with the original. SWITCHER, HYBRID and WARIO PAINT use some of them.
    - **Still unverified.**
      The original's title writer is the text interpreter at `$80:C3C0`, whose
      big-font table is not yet located. The table at `$80:C709` drives the small font.

## The matrix

| Track | Name | Tour | Stream | Unpacked | Shape | Idle 1,200 updates (native) | Reference (NOW PLAYING kind, laps) | Match with the original, controller released |
| ---: | --- | --- | --- | ---: | --- | --- | --- | --- |
| 0 | DRAGSTER (observed) | CRAWLER 1 (observed) | $98:8000 | 33,815 | 1024x16 | completes | captured, boundary 1328; race (one run) | **exact, 1,573 of 1,573 updates** |
| 1 | ZOOM ZOO (observed) | CRAWLER 2 (observed) | $98:8183 | 50,665 | 256x64 | completes | captured, boundary 1376; race (laps), 3 laps | **exact, 1,525 of 1,525 updates** |
| 2 | BOWL (observed) | CRAWLER 3 (observed) | $98:9B4A | 35,810 | 256x64 | special-tile guard after 7 updates | captured, boundary 1335; stunt | not compared: no native stunt event |
| 3 | SWITCHER (observed) | CRAWLER 4 (observed) | $98:A07E | 63,013 | 1024x16 | special-tile guard after 383 updates | captured, boundary 1418; race (one run) | **exact, 1,483 of 1,483 updates** since the special tiles (R-0047); exact 384 before |
| 4 | MONSTER (observed) | CRAWLER 5 (observed) | $98:C70E | 52,234 | 128x128 | special-tile guard after 360 updates | captured, boundary 1419; race (laps), 3 laps | **exact, 1,482 of 1,482 updates** since the inverted AI marker (R-0048); 809 after R-0047, 361 before |
| 5 | WOBBLE | JUMPER 1 (hypothesis) | $98:E62E | 47,619 | 64x256 | edge guard after 330 updates | not reachable from a cold start | - |
| 6 | TWINPEAK | JUMPER 2 (hypothesis) | $98:FEDB | 48,168 | 256x64 | special-tile guard after 760 updates | not reachable from a cold start | - |
| 7 | SKIER | JUMPER 3 (hypothesis) | $99:977C | 35,617 | 128x128 | edge guard after 738 updates | not reachable from a cold start | - |
| 8 | LOOPBACK | JUMPER 4 (hypothesis) | $99:9CD3 | 56,372 | 512x32 | edge guard after 562 updates | not reachable from a cold start | - |
| 9 | SMALL CUT | JUMPER 5 (hypothesis) | $99:C1DF | 45,036 | 256x64 | special-tile guard after 700 updates | not reachable from a cold start | - |
| 10 | LOOPER (observed) | SHUFFLER 1 (observed) | $99:D307 | 63,397 | 64x256 | edge guard after 359 updates | captured, boundary 1417; race (one run) | **exact, 1,484 of 1,484 updates** since the edge recovery (part 3, observation 16) |
| 11 | MEGAJUMP (observed) | SHUFFLER 2 (observed) | $9A:81D9 | 45,899 | 256x64 | special-tile guard after 530 updates | captured, boundary 1368; race (laps), 3 laps | **exact, 1,533 of 1,533 updates** since the special tiles (R-0047); exact 531 before |
| 12 | JUMPS (observed) | SHUFFLER 3 (observed) | $9A:96AC | 38,625 | 128x128 | completes | captured, boundary 1343; stunt | not compared: no native stunt event |
| 13 | FLAT FUN (observed) | SHUFFLER 4 (observed) | $9A:A206 | 49,156 | 512x32 | completes | captured, boundary 1392; race (one run) | **exact, 1,509 of 1,509 updates** |
| 14 | INFINITY (observed) | SHUFFLER 5 (observed) | $9A:BBEC | 38,219 | 256x64 | completes | captured, boundary 1376; race (laps), 7 laps | **exact, 1,525 of 1,525 updates** since the 80 checkpoint flags (R-0048); 379 before |
| 15 | LAST ONE | BOUNDER 1 (hypothesis) | $9A:C3FC | 54,382 | 256x64 | edge guard after 461 updates | not reachable from a cold start | - |
| 16 | MARATHON | BOUNDER 2 (hypothesis) | $9A:E545 | 53,331 | 256x64 | special-tile guard after 403 updates | not reachable from a cold start | - |
| 17 | CIRCLE | BOUNDER 3 (hypothesis) | $9B:838B | 34,490 | 256x64 | special-tile guard after 220 updates | not reachable from a cold start | - |
| 18 | PLINKEY | BOUNDER 4 (hypothesis) | $9B:8668 | 49,258 | 128x128 | special-tile guard after 525 updates | not reachable from a cold start | - |
| 19 | JUMPOVER | BOUNDER 5 (hypothesis) | $9B:9F15 | 52,178 | 256x64 | completes | not reachable from a cold start | - |
| 20 | DRAGRACE (observed) | WALKER 1 (observed) | $9B:BCED | 42,663 | 512x32 | edge guard after 452 updates | captured, boundary 1360; race (one run) | **exact, 1,541 of 1,541 updates** since the special tiles (R-0047); 1,353 before |
| 21 | PINGPONG (observed) | WALKER 2 (observed) | $9B:CDCB | 46,893 | 256x64 | completes | captured, boundary 1367; race (laps), 3 laps | **exact, 1,534 of 1,534 updates** since the rotation fix of R-0047; 1,068 before |
| 22 | HILL CLIMB (observed) | WALKER 3 (observed) | $9B:E38D | 34,622 | 128x128 | edge guard after 776 updates | captured, boundary 1331; stunt | not compared: no native stunt event |
| 23 | HYBRID (observed) | WALKER 4 (observed) | $9B:E720 | 47,906 | 256x64 | completes | captured, boundary 1386; race (one run) | **exact, 1,515 of 1,515 updates** since the edge recovery (part 3); a longer capture is exact for 2,317 rows, to the opponent's finish (R-0048) |
| 24 | SHORT CUT (observed) | WALKER 5 (observed) | $9B:FF01 | 47,078 | 256x64 | completes | captured, boundary 1402; race (laps), 3 laps | **exact, 1,499 of 1,499 updates** since the special tiles (R-0047); 1,483 before |
| 25 | DOWN+UP | RUNNER 1 (hypothesis) | $9C:9454 | 50,764 | 128x128 | special-tile guard after 451 updates | not reachable from a cold start | - |
| 26 | HIGHROAD | RUNNER 2 (hypothesis) | $9C:AF5E | 47,215 | 128x128 | special-tile guard after 444 updates | not reachable from a cold start | - |
| 27 | SPINE | RUNNER 3 (hypothesis) | $9C:C5B8 | 39,267 | 256x64 | special-tile guard after 301 updates | not reachable from a cold start | - |
| 28 | BOO! | RUNNER 4 (hypothesis) | $9C:D0D8 | 53,001 | 128x128 | special-tile guard after 560 updates | not reachable from a cold start | - |
| 29 | FIRE ESCAPE | RUNNER 5 (hypothesis) | $9C:ED41 | 44,483 | 256x64 | completes | not reachable from a cold start | - |
| 30 | WARIO PAINT (observed) | HOPPER 1 (observed) | $9C:FB63 | 49,932 | 1024x16 | completes | captured, boundary 1390; race (one run) | **exact, 1,511 of 1,511 updates** |
| 31 | CROCK (observed) | HOPPER 2 (observed) | $9D:9405 | 52,171 | 256x64 | completes | captured, boundary 1397; race (laps), 3 laps | **exact, 1,504 of 1,504 updates** |
| 32 | DOWNER (observed) | HOPPER 3 (observed) | $9D:B1DA | 34,497 | 64x256 | edge guard after 459 updates | captured, boundary 1349; stunt | not compared: no native stunt event |
| 33 | EAST (observed) | HOPPER 4 (observed) | $9D:B5D4 | 48,099 | 1024x16 | completes | captured, boundary 1403; race (one run) | **exact, 1,498 of 1,498 updates** |
| 34 | HAIRPIN HILL (observed) | HOPPER 5 (observed) | $9D:CD3F | 43,340 | 256x64 | special-tile guard after 301 updates | captured, boundary 1407; race (laps), 5 laps | **exact, 1,494 of 1,494 updates** since R-0048; 531 after R-0047, 302 before |
| 35 | VERTICAL | SPRINTER 1 (hypothesis) | $9D:DE54 | 57,267 | 64x256 | edge guard after 438 updates | not reachable from a cold start | - |
| 36 | FLASH | SPRINTER 2 (hypothesis) | $9E:8241 | 41,512 | 128x128 | completes | not reachable from a cold start | - |
| 37 | LITTLE DIPPER | SPRINTER 3 (hypothesis) | $9E:907B | 35,364 | 16x1024 | refused in its first update: shape | not reachable from a cold start | - |
| 38 | FRUITBAT | SPRINTER 4 (hypothesis) | $9E:952D | 47,242 | 256x64 | special-tile guard after 604 updates | not reachable from a cold start | - |
| 39 | 123 JUMP | SPRINTER 5 (hypothesis) | $9E:AAED | 51,273 | 256x64 | special-tile guard after 1168 updates | not reachable from a cold start | - |
| 40 | GRILLER | HUNTER 1 (hypothesis) | $9E:C756 | 65,354 | 128x128 | special-tile guard after 243 updates | not reachable from a cold start | - |
| 41 | TWO LOOPS | HUNTER 2 (hypothesis) | $9E:FC2C | 43,463 | 256x64 | completes | not reachable from a cold start | - |
| 42 | NEON | HUNTER 3 (hypothesis) | $9F:8CD5 | 48,453 | 256x64 | special-tile guard after 17 updates | not reachable from a cold start | - |
| 43 | HAMSTER | HUNTER 4 (hypothesis) | $9F:A098 | 49,607 | 128x128 | special-tile guard after 525 updates | not reachable from a cold start | - |
| 44 | TO AND FRO' | HUNTER 5 (hypothesis) | $9F:BB4C | 44,592 | 256x64 | completes | not reachable from a cold start | - |

"Completes" in the idle column means only that no native guard fired on ZOOM ZOO's scenario. The
match column compares the 742-byte state (836 bytes for any track but DRAGSTER and ZOOM ZOO since
R-0048) from the original's own initialization boundary, update by update, on the native scenario of the same race mode (DRAGSTER's for one-run races, ZOOM ZOO's
for lap races), with the controller released after the menu (part 2, `track_reference sweep`,
horizon frame 2,900 declared before the run). Bytes 0-11 (state magic and frame label) are
excluded, since native labels frames from its own scenario. It is not the frozen-gate acceptance
the two accepted tracks carry.

## Hypotheses and limits

- **Tours.** Observed for the four tours a cold start offers (observation 6): index
  = 5 x tour + position, with the stunt event third. For the other five tours
  (JUMPER, BOUNDER, RUNNER, SPRINTER, HUNTER) the same rule is a hypothesis: they
  are not on the cold-start PICK TOUR screen, and how the original unlocks them is
  not recorded.
- **`$0FF7` may outlive track 37.** In banks `$80`-`$83` its only writer besides the
  power-on clear (`$80:92F7`) is the `$04` arm, which stores 1. If no race setup
  clears it, every track raced after LITTLE DIPPER in the same power-on session
  skips the negative-row clamp at `$81:8A31`, and the engine's six arms (which
  assume 0) would not describe it. Not yet checked; found by the part 1 review.
- "Completes 1,200 updates" means only that no native guard fired with the
  controller released. It is **not** a match with the original. The scenario
  (laps, race mode, initialization frame) is ZOOM ZOO's for every track, which
  the original will not use on most of them.
- The landing-response matrices remain a captured input (observation 11 shows they
  do not vary across the 20 reachable tracks; a ROM producer for `$81:9A4D-9E12`
  would remove the capture).
- **No cold-start race stops at a native guard any more** (R-0047, R-0048): all 16 match the
  original over their windows, the longer captures match to frame 4,400 or to a finish, and
  every one runs 4,000 held-input updates clean in the app. Tile flag pairs 4, 8, 12, 26 and
  28 remain guarded; BOWL (a stunt event) and two locked tracks reach 8 and 26.
- **The 470-pixel frame at update 272** is on every track checked, DRAGSTER and ZOOM
  ZOO included, and predates this task. It sits in the GO letters' area. It is
  unexplained and open.
- **Done in RACE-FINISH-BREADTH** ([R-0049](R-0049-race-finish-breadth.md)): the finish
  phase counts race updates, and new tracks' finishes and one-run results match. Before it:
  the finish slowdown in `movement.cpp` counted the absolute frame modulo 3. Both accepted tracks start on a
  frame congruent to 2 (mod 3), but 9 of the 14 new tracks do not. If the original
  counts from race start there, as `$0300` does, those finishes will be out of phase.
  Capturing a new-track finish decides it (part 3 review).
- The lap graph of the authored result screen spaces 5 and 7 laps by `165 / laps`.
  That is native layout, not recovered.
- The match column holds for a released controller only. Riding inputs, finishes and
  the result screen of the new tracks are not compared.

## Next experiments

1. **Done in part 3**: a native scenario per track and selection by id (observations
   12-15). **Done in RACE-GUARDS**: INFINITY's checkpoint guard; the flags are 80 bytes
   ([R-0048](R-0048-race-guards.md)).
2. **Done in SPECIAL-TILE-RESPONSE** ([R-0047](R-0047-special-tiles.md)): the lift,
   mud, corkscrew and jump-driven tiles (flag pairs 24, 14, 10 and 16). Every
   compared race now runs past its old special-tile stop; pairs 4, 8, 12, 26 and 28
   stay guarded, since no capture touches them.
3. **Done in part 3**: the sampler edges (observation 16). **Done in RACE-GUARDS**: the
   inverted AI marker (R-0048).
4. **Done in SPECIAL-TILE-RESPONSE**: PINGPONG's `opponent.response_b` at update
   1,068 was the rotation clearing its step under surface mode (R-0047).
5. **Done in LOCKED-TOURS** ([R-0050](R-0050-locked-tours.md)): the locked tours open in the
   laboratory with a preloaded cartridge RAM; 15 of their 20 race tracks match. TILE-PAIRS-8-12-26
   ([R-0051](R-0051-loop-and-tile-pairs.md)) recovered the tile pairs that stopped four more;
   TWO LOOPS waits on the HUNTER tag effects, recovered in HUNTER-EFFECTS
   ([R-0052](R-0052-hunter-effects.md)): all 36 race tracks now match.
6. **Stunt events**: a separate mode (solo, qualifying score), out of this task's race
   scope.

## Reproduction

```sh
python3 tools/project.py content rnc-inventory --expect tests/manifests/content/track-streams.json
python3 tools/project.py build --preset lab-debug
python3 tools/project.py content track-idle-matrix --out artifacts/track-breadth/idle-1200 --updates 1200
python3 -m tools.unirally_lab.native.track_reference recompare --per-track --sweep <sweep dir> \
    --binary build/lab-debug/src/core/zoom_zoo_runner --pack local/classic-pal-crawler-tracks-v11.pack --out <json>
python3 -m tools.unirally_lab.native.track_reference sweep --core <pinned bsnes core> \
    --out artifacts/track-breadth-2/sweep --binary build/lab-debug/src/core/zoom_zoo_runner \
    --pack local/classic-pal-crawler-two-tracks-v9.pack --horizon 2900
```
