# R-0116 - The picture residuals the records leave

Status: research result and native change ([PICTURE-RESIDUALS](../../tasks/PICTURE-RESIDUALS.md)),
12 November 2026, on main `ce558a3f`. PAL ROM SHA-256
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, reference core bsnes `7d5aa1e6`
(patched, Strict, `bsnes_ppu_fast` on). Evidence in main `local/evidence/picture-residuals/`. It answers
R-0109's queue item 7: the pictures R-0046, R-0071, R-0073, R-0083, R-0084, R-0088, R-0098 and R-0103
leave different from the original's. Each was measured again on main first; four were already equal,
three are fixed here by four native changes, and two remain, recorded with what was learned.

Tags: **[L]** read from the static map's listing (main `local/evidence/static-code-map/static-map/`) or
the ROM's bytes; **[C]** a capture of the original (existing, or a probe made for this task: work RAM
per frame, pictures, and Strict states whose PPU part `ppustate.py` reads: VRAM at state offset
140,527, CGRAM after it, the BG registers before it); **[R]** stated by an accepted record.

## Domain

Main's binaries (`base-ce558a3f/`) and the candidate's, pack v41, against the original's pictures:
- the idle demos: IDLE-DEMO-CYCLES' `cold-102000` (1,006 pictures, every 100th frame, 34 demos) and a
  probe of its first two demos at every frame (`oneview-demos/`, frames 1448-6423, 4,976 pictures);
- split races: SPLIT-HUD-GAPS' review capture `zzap` (6,074 pictures), SPLIT-HUNTER-PICTURES' `demo-pic`,
  `demo-pic-mike`, `vs-4`, `one-67`, TWO-HUMAN-RESTART's `vs-control`, `vs-restart`, `twop-restart`,
  `league-restart` (NOW PLAYING, results and their slides);
- LEAGUE-TOURS' fifteen probe windows (`tours-a` w0-w8, `tours-b` w0-w1, `odd-tours` w0-w1, `five`
  w0-w1; a picture every tenth frame, two humans in two views): every race picture (`race_pictures.py`),
  and main's whole-window `menu_compare.py` runs (`m0/`);
- LEAGUE's frozen gate (`league_frozen.py`, 59 pictures) and the organic league captures;
- TRACK-BREADTH part 3's menu captures of ZOOM ZOO and track 10 (every frame of race updates 190-300).

## Findings, by item

| Item (record) | On main `ce558a3f` | Cause in the original | Now |
| --- | --- | --- | --- |
| The three league pictures at the frozen 14-pixel limit (R-0073) | 0 pixels | - | equal (59 of 59 at 0) |
| NOW PLAYING's arrow and icon animation (R-0071, R-0073, R-0084) | equal | - | equal |
| The result icons and slides after the result (R-0084) | equal | - | equal |
| 470 pixels in the GO letters' area at update 272 (R-0046) | equal | - | equal |
| 41 small sprite pictures in demo races (R-0088) | 13 of 1,006; 177 of 4,974 every frame | the demo leaves `$212D` = 0 | **fixed**: all equal |
| A rider passing under the HUD in split races (R-0083, R-0098) | `zzap` 1; league 34 in 11 windows | `$82:B07E-B153`; the PPU's object priority | **fixed**: all equal |
| Track 42's two-view NEON (R-0098, R-0103) | 314 + 313 + 313 league pictures | colours 0-95 from another scenery's set | **fixed**: all equal |
| An upper-view object at the left edge (R-0103) | 1 picture, 8 pixels, in 3 windows | not recovered | open |
| Line 113 under a six-line mosaic block (R-0103) | 14 pictures, one row each | the BG1 map row above the lower view (hypothesis) | open |

1. **Already equal on main** [C]. The organic league captures' NOW PLAYING pictures (`organic-cycle-five`
   14400 and 15000, R-0073's 885 pixels) and all 38 of its pictures, the frozen gate's 59 pictures (the
   three of `organic-full-tour-turnaround` that had a 14-pixel limit are 0; also 0 on SPLIT-HUNTER-PICTURES'
   base `9980a38a`), every picture of `vs-control` (1,300), `vs-restart` (2,099), `twop-restart` (2,098)
   and `league-restart` (1,434), and the 222 frames of race updates 190-300 on ZOOM ZOO and track 10
   through the menus (`front_end_runner`; the update-272 picture included) and through
   `classic_race_presentation_runner --timeline` (frames 1640 and 1647-1649) are equal. Later tasks'
   changes (DEMO-PICTURES' `$0300` phase, MENU-INPUT, RECORD-SCREENS) are the likely cause; which one
   closed each is not traced.
2. **The idle demo's ink** [L, C]. The demo's title writes `$212D` = 0 (`$80:942D`, R-0082), so in an
   idle demo the objects are not on the sub screen and the HUD's and caption's BG3 ink is flat over the
   riders. SPLIT-HUNTER-PICTURES drew it so for the split demo only; native drew a one-view demo's rider
   under the ink with a race's colour math (red added). Every residual picture of `cold-102000` is a
   rider under a caption ("BIGGER", 5100) or the timer (17200, 91100): the original shows the flat ink.
3. **The lower view's left and right edges** [L, C]. After a two-view race places the lower view's
   entries (`$150B` the opponent, `$150F` the player), `$82:B07E-B153` looks at each x: from 0xF1 to
   0xFF with the ninth bit clear (the right edge) the entry moves to the hidden place (0x30 or 0x1E);
   at 0 or from -15 to -1 (the left edge), when the object's top is above the lower view's first line
   (OAM y below 0x71), the entry moves 16 pixels (left when reflected, `$0BA7`/`$0BA9`, else right) and
   its tile number goes up by 2 (0x02, 0x8A). The tiles that brings in are the blank other half of the
   riders' tiles (R-0103), so the object loses its first two tile columns: with the flip, its right-hand
   16 pixels. `zzap` 6316: `$150F` = 0xEF, tile 2, ninth bit set (x -1 moved to -17), so picture 6317
   lacks the 22 pixels of the player's seat native drew (R-0083's "Not established"). The right edge
   is captured too: TWO-HUMAN-STUNTS' `w1-2p-hill` 2881, 2883, 2887 and 2888, where main drew 4 pixels
   of a lower-view rider at x 255 (lines 180-183) and the original, like the candidate, shows the
   background (review round 1). Native's split
   clip (`dy` from -41) never sets the row clip; the original zeroes `$0C89`/`$0C8B` in a two-view race
   (`$82:AFDA`, `$82:B057`), with no effect in either view.
4. **The objects' priority** [C]. Both riders through a corkscrew together: each object's priority bit
   (`$1516`/`$151A` bit 4, R-0047) toggles on its own update, so for an update one rider is raised
   (priority 3) and the other not. Where the front object (by OAM order) is opaque the PPU takes its
   pixel and its priority alone (bsnes `renderObject`: later tiles of earlier objects overwrite the
   colour and the priority), so where the front rider is not raised and covers a raised one on a BG1
   priority tile, the tile shows. Native tested each rider against BG1 on its own and drew the raised
   one's pixel: `tours-a-w8` 259210 (1 pixel; `$1512` 0x36, `$150E` 0x28), `tours-b-w1` 46630 (1),
   `odd-tours-w1` 67930 (6). With item 3 these are the 10-22 pixel "under the HUD" pictures of R-0098
   (SHUFFLER and HOPPER): `tours-a` w0 7, w1 2, w2 1, w5 4, w7 6, w8 1, `tours-b` w0 3, w1 3, `odd-tours`
   w1 3, `five` w0 3, w1 1.
5. **NEON with two views** [L, C]. `$82:DC22-DC4D` (R-0098, R-0105): on track 42 with `$77:0750` bit 3
   set, A keeps the whole word, 0xC208 (`$0751` is the defaults' 0xC2 for good, R-0105); the mod-14 loop's
   `BMI` takes it as negative at once and it is stored as the scenery `$0230`. The asset loaders take
   A's low byte, so BG2's tiles, map and colours 96+ are scenery 8's [C: VRAM `$2000`/`$E000` equal
   `scenery.08.bg2-tiles` and `-map` at `tours-a` 81100]. Colours 0-95 come from one of three sets
   (`$82:DCBB-DD84`: assets 0xA4-0xA9, 0xB0-0xB5 or 0xAA-0xAF) chosen by the byte at `$82:DC12` + X with
   X = 0xC208: a long index carries into bank $83, so the byte is `$83:9E1A`'s 0, the set of sceneries 0,
   2 and 5 [L, ROM bytes], not scenery 8's (`$82:DC12`'s table: 0 for 0, 2, 5; 1 for 6, 9, 11, 12, 13;
   2 for 1, 3, 4, 7, 8, 10). The original's CGRAM at 81100 equals `scenery.00.palette` in colours 1-95
   and no scenery-8 value there [C]. Native drew scenery 8's colours 13-15 of each row: the track's edge
   lines, 200-1,190 pixels on every NEON race picture. In VS the word is 0xC20C [R-0105], the byte
   `$83:9E1E`'s 10, so the set of sceneries 1, 3, 4, 7, 8 and 10 (not 12's own) [L only: no VS NEON
   capture].
6. **An upper-view object at the left edge** [C], open. `tours-a-w2` 70040, `odd-tours-w0` 11910 and
   `five-w1` 95090 are the same moment of the same race (a tour's first race, 4,681-4,683 frames in,
   under slow motion): the original shows 8 pixels at x 0-2 of lines 113-115 where native shows the
   lower view's background. Its work RAM gives the upper view's opponent entry `$1517` x 0xF3 with the
   ninth bit (-13) and y 0x64, so the object crosses the split. In `zzap` 218 frames have a lower-view
   object reaching above line 112 and 108 an upper-view one reaching below, and all their pictures equal
   native's clip at line 112, so the original hides objects across the split in some way this task did
   not recover; at the left edge it leaves these 8 pixels.
7. **Line 113 under the mosaic** [C], open. `demo-pic` (9 pictures) and `vs-4` (5) still differ on
   line 113 only. A Strict state after `demo-pic` 2611 gives BG1 as one 32-by-32 map of 16-pixel tiles
   (word 0x1C00, 512 by 512 pixels) shared by both views, with BG1 and BG2 mosaic on. A six-line block
   starting at line 108 makes line 113 sample the map row above the rows the lower view shows. Drawing
   that row from the state's VRAM does not reproduce the original's line 113 (wobble mode's scroll
   changes per line, and the state holds only the last line's registers), so R-0103's hypothesis (an
   older map row that native, which draws BG1 from the track, cannot know) is neither confirmed nor
   refuted. Fixing it means modelling BG1's map streaming for two views.

## Native change

`presentation.cpp` (`render_classic_race` and its helpers; no state, format, pack or runner change):
- `hud_over_riders` is every idle demo (`state.demo_ai`), not the split demo only (finding 2).
- `lower_view_edge` and `drop_first_tile_columns`: a lower-view entry at the right edge is not
  drawn; one at the left edge with its top above the lower view loses its first two tile columns
  (finding 3).
- `layer_rider_object` builds the riders' layer, the front object over the back one with its priority;
  `draw_race_riders` then decides each pixel against BG1 by its object's priority (finding 4).
- `ClassicRacePresentationContent::background_palette_set` (`presentation.hpp`): for track 42 with two
  humans and no NEON lighting, colours 0-95 (the first 192 bytes) of `scenery.00.palette` (2P and a
  league's pair) or `scenery.08.palette` (VS); `race_cgram` and `race_ink_rgb` read them (finding 5).

## Results

Candidate `3cca204a` against main `ce558a3f`, pictures equal to the original's (`checks.list`):

| Check | Pictures | Main | Candidate |
| --- | ---: | ---: | ---: |
| `oneview-demos` (every frame) | 4,974 | 4,797 | 4,974 |
| `idle-lap` (`cold-102000`) | 1,006 | 993 | 1,006 |
| `zzap` | 6,074 | 6,073 | 6,074 |
| `league-a` race pictures (`tours-a` w0-w8) | 17,725 | 17,076 | 17,724 |
| `league-b` race pictures (`tours-b`, `odd-tours`, `five`) | 19,466 | 19,138 | 19,464 |
| `two-human` | 6,931 | 6,931 | 6,931 |
| `league-frozen` | 59 | 59 at 0 | 59 at 0 |
| `go-letters` | 222 | 222 | 222 |
| `line-113` (open) | 8,354 | 8,340 | 8,340 |

The league pictures the candidate leaves unequal are the three of finding 6 (8 pixels each, the same
on main). On main, by window (race pictures differing): `tours-a` w0 7, w1 2, w2 316 (314 NEON), w5 4,
w6 313 (NEON), w7 6, w8 1; `tours-b` w0 3, w1 3; `odd-tours` w0 314 (313 NEON), w1 3; `five` w0 3, w1 2.
Outside these checks, TWO-HUMAN-STUNTS' `w1-2p-hill` goes from 5,046 to 5,050 of 5,050 (frames 2881,
2883, 2887 and 2888, 4 pixels each at x 255: finding 3's right-edge hide), and LEAGUE-TOURS' review window `four-w0` changes 7
pictures, each to equal. The gate corpus replay (536 cases) finds pictures alone change, in 44 cases;
every changed picture of them is closer to the original (the task record lists them), and the race
sweep's 2,538 pictures are unchanged.

## Limits

- The two open items (findings 6 and 7) are queued in
  [PICTURE-RESIDUALS-REMAINDER](../../tasks/PICTURE-RESIDUALS-REMAINDER.md).
- League windows are compared every tenth frame (the probes keep no more); `five-w1`'s races from
  120,013 are left out: its ANTI-UNI landing on track 41 at 121,431 takes the multiply hit by HDMA
  (R-0098, R-0115) and native's race differs from there.
- Two-view NEON in VS (finding 5) is read from the listing and the ROM only (no VS NEON capture); the
  lower view's right-edge hide (finding 3) has one capture (`w1-2p-hill` 2881-2888).
- The priority rule (finding 4) is the PPU's; it also applies to one-view races, where the player is
  the front object. The gate corpus replay compares retained pictures only. The review's every-frame
  probe (a build counting each picture where the new rule and the old differ, `review-1/priority-probe.out`)
  over 40 corpus runs, 624,727 frames, finds 5 changed pictures, all in the `five` drive: 4 two-view
  (7864, 12472, 84176, 90879; 1-10 pixels) and one one-view, 101732 (7 pixels at x 70-71, lines
  128-131). The reviewer captured the original there (`review-1/original-five-101732/`): the candidate
  is equal, main 7 pixels off. So the rule reaches one-view pictures, rarely, and the one checked moves
  toward the original.
- No unit test: each change needs the private pack's content; the checks compare with the original.
- `organic-cycle-exit` 14400 and 15000 (organic league captures) differ in the whole picture on main and
  here: native's second race there starts later than the original's (not a picture residual; noted in
  the remainder).

## Reproduction

```sh
local/locks/heavy-run.sh PICTURE-RESIDUALS zsh local/evidence/picture-residuals/run_checks.sh OUT
RUNNER=local/evidence/picture-residuals/base-ce558a3f/front_end_runner zsh local/evidence/picture-residuals/run_checks.sh OUT
python3 local/evidence/league-tours/picdiff.py tours-a-w2 81100 --out DIR --pack local/classic-pal-crawler-tracks-v41.pack
python3 local/evidence/picture-residuals/ppustate.py STATE   # BG registers from a probe's --state-out
```
