# R-0103 - HUNTER's effects in a two-view race's picture

Status: research result and native change ([SPLIT-HUNTER-PICTURES](../../tasks/SPLIT-HUNTER-PICTURES.md)),
28 October 2026, on main `9980a38a`. PAL ROM SHA-256
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, reference core bsnes
`7d5aa1e6` (Strict). Evidence in main `local/evidence/split-hunter-pictures/`. It takes up
R-0094's open two-view gap: the race state of a split race in HUNTER mode matched, its picture
did not.

Tags: **[L]** read from the static map's listing (`artifacts/static-map/bank-8x.lst` in the main
checkout); **[C]** confirmed in a capture.

## Domain

In-process replays of the original (`probe.py`, CREDITS-NAME's): work RAM `$0000-$3FFF` and
cartridge RAM per frame, and every picture of the race, in `cap/` (`captures.sh`):

- `demo-pic`: CREDITS-NAME review's `demo-faedine-0`, the split ZOOM ZOO idle demo from the image
  with rider 0 named "faedine" (`faedine-split0.bin`); pictures 1446-3699. Its tags: invisible
  track (1894), wobble mode (2602), hedgehog speed (3117), screen flip as "invisible unis" (3447).
- `twop-104`, `vs-124`, `vs-4`: 2P and VS ZOOM ZOO races from the same image, both pads driven by
  `search.py`'s random hold and release segments (seeds 104, 124 and 4), chosen by a native search
  of 200 seeds per mode for their tags (R-0094: native's race state matches). Pictures 1900-7999.
  Their tags: `twop-104` hedgehog speed, slow motion, barf mode, invisible track, invisible unis,
  power bounce, barf mode; `vs-124` invisible unis, barf mode, hedgehog speed, power bounce,
  invisible unis, wobble mode, invisible unis; `vs-4` invisible unis, wobble mode, hedgehog speed
  (twice), wobble mode, invisible track, control reversed, slow motion.
- `one-67`: a one-player ZOOM ZOO race against the computer after a cold start that types
  "faedine" (CREDITS-NAME's `faedine-zoom`), with slow motion (4406) and hedgehog speed (6985);
  pictures 4350-7999.
- `demo-pic-mike`: the review's same demo with the name "mike" (no HUNTER mode), as captured then.
- LEAGUE-TOURS' `tours-a` windows 2 and 6: a two-human league on HUNTER's tour (tracks 40-43, two
  views), 12 tags each, pictures every tenth frame through the races (LEAGUE-TOURS' `menu_compare.py`).

`compare.py` (CREDITS-NAME's review copy) runs native's front end on the capture's pads, each
race initialized where the original's was, and compares every captured picture.

## Findings

- **Invisible unis.** In a two-view race the screen flip's routine sets the blink `$7E:2054` and
  returns before `JSR $D581` (`$83:D337-D343`, R-0094), so no per-line BG1 table is built and no
  object is turned over. The update's visibility routine, for a race with two views (`$77:0750`
  bit 3), points the riders' four objects (`$150B`, `$150F`, `$1513`, `$1517`: x, y, tile,
  attribute) at the other half of their tiles while the blink the update before left is on:
  tiles `$80` and `$08` for `$88` and `$00` (`$82:B154-B17F`) [L]. Those tiles are blank: neither
  rider shows in any view [C: `demo-pic` 3448-3645, `$150D` 0x80 and `$1511` 0x08 from 3448].
  BG1 shows as in any split picture. Native drew BG1 upside down and the riders flipped; it now
  draws BG1 unflipped and no rider while the blink is on, from the same history word
  (`hunter_flip_prior`) as the one-view flip's object bit.
- **Wobble mode's mosaic in the lower view.** The PPU counts mosaic rows from the picture's first
  line (bsnes ppu-fast `PPU::main` reloads the counter on line 1 only), so a lower-view block
  starts where the frame's do, not at the view's line 112; BG2 already did so for its tile fetch,
  BG1 and the block start now do too [C: `demo-pic` 2604-3101 lower view, `vs-4`].
- **Rider 1's caption row.** Rider 1's consumer (`$81:BEF1-BF31`) takes an event into `$0EC7` and
  stores it in `$11C3` (`$81:BF0A`); on its next dry look, with `$11C3` set, it clears it and
  copies the HUD message buffer `$128D` to `$0EC7` (`$81:BFB9-BFD0`) [L]. Off the HUNTER tour that
  buffer is blank; on it, it holds the running effect's name, so the lower row shows the name
  again whenever rider 1's queue is idle, as the player's does (R-0052) [C: `demo-pic` 2282-2393,
  2627-3101]. The player's dry look runs first in the same update and may fill the buffer
  (`$81:BF43-BFA2`); an effect's end blanks it afterwards, in the HUNTER routine (`$83:D275`).
- **The front push in rider 1's queue.** HUNTER's front-of-queue push (`$81:C579-C594`) writes at
  the read cursor and steps it back; native's HUD read any cursor change as a take, so the lower
  row showed the entry behind the cursor ("ROLL" on `demo-pic` 2605 and 3119) [C]. A take steps
  the cursor on. A take and an effect's end in one update leave the cursor where it was, with the
  take's display time in the cooldown and the end's event over the slot the take read [C: `vs-4`
  5327, rider 1's hint 43 taken and overwritten by 35].
- **Skipped updates.** Hedgehog speed and slow motion skip the race update (`$83:CC9A` to
  `$83:CDAA`, R-0052), so its clock (`$81:C6D1-C6DC`) asks for no lower clock and `$82:9822`
  writes no arrow word. Native asked for the lower clock on every skipped update holding the
  tick's subframe, which kept the split chain from reaching the captions for up to five frames
  [C: `demo-pic` 3414-3424], and recomputed the arrow from the marker of the last update that ran
  rather than the one before it, which turned the upper arrow round [C: `twop-104` 2787-2821].
- **The idle demo's caption rows.** The demo's title writes `$212D` = 0 (`$80:942D`, R-0082), so
  both caption rows, like its HUD, are flat ink over the riders. Native drew the upper caption under
  them with the colour math of a race from the menus, which showed a rider's pixels through the
  ink. This was on main as well (the "mike" demo's 125 unequal pictures, 1719-2103) [C].
- **Race state.** The original's tags fall where native's search placed them, and the race rows
  agree (`race_check.py`) on `demo-pic` (2,201 of 2,201) and `vs-4` (5,973 of 5,973). In `twop-104`
  and `vs-124` they agree up to power bounce: from frame 5548 (update 3521) and 4981 (update 2954),
  under effect 2, rider 1's velocity differs. Native gives the landing matrix 0 to rider 0 only
  (`race_update.cpp`, `index == 0`, after `$81:94B9`). The original gives it to rider 1 as well in
  a two-view race: at `$81:94A8-94B2` a two-view race skips the test that excludes rider 1, so
  both riders land with matrix `$0572` under `$132B`, and at both divergence frames the
  original's RAM shows a two-view race with power bounce on (review round 1). Native's own comment
  at `vertical_contact.cpp` already says so. Not changed here (race state, tier 1); queued as
  TWO-VIEW-POWER-BOUNCE.

## Native

`presentation.cpp`: `race_scroll` flips only in a one-view race; `draw_race_backgrounds` takes
the mosaic block from the picture's line; `invisible_riders` hides both riders of a two-view race
while the blink is on; the idle demo draws its caption with its HUD after the riders.
`race_hud.cpp`: `request_split_fields` takes on a forward step only (or a take and push in one
update), copies `hud_message_at_consumers` on rider 1's dry look and asks for no lower clock on a
skipped update; `redraw_arrow` keeps each rider's marker (`arrow_markers_`) through skipped
updates. No state, format or pack change.

## Results

Pictures equal to the original's, candidate against main `9980a38a`, both on pack v41
(`checks.list`, `shp_one.sh`):

| Capture | Pictures | Main | Candidate |
| --- | ---: | ---: | ---: |
| `demo-pic` | 2,254 | 1,194 | 2,245 |
| `demo-pic-mike` | 2,254 | 2,129 | 2,254 |
| `twop-104` (to 5547) | 3,648 | 2,658 | 3,648 |
| `vs-124` (to 4980) | 3,081 | 2,567 | 3,081 |
| `vs-4` | 6,100 | 4,398 | 6,095 |
| `one-67` | 3,650 | 3,650 | 3,650 |

Every candidate picture that differs, differs on line 113 only (the limit below).

The league windows (`shp_league.sh`; whole windows, then the pictures taken while an effect runs):

| Window | Pictures | Main | Candidate | While an effect runs | Main | Candidate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `tours-a-w2` | 3,889 | 2,235 | 2,557 | 664 | 139 | 459 |
| `tours-a-w6` | 3,890 | 1,706 | 2,032 | 664 | 137 | 460 |

No picture differs in more pixels than on main. Of the effect pictures the candidate leaves
unequal, all but one are on track 42 (NEON with two views), whose split picture differs before
its first tag too (thin lines along the track's edges, 200-1,200 pixels; main the same); the other
(`tours-a-w2` 70040, track 40) is 8 pixels at the lower view's left edge, the same on main.

The review's replay of the gate corpus (`review-1/replay.json`) found the change alters only
pictures, in 24 runs, and none is worse than main against the original: the idle demo goes from
963 to 991 of 1,006, and non-HUNTER two-view league windows gain too (9 pictures each in
`tours-a-w8`, `tours-b-w1` and `odd-tours-w1`), most likely from the idle and caption-row changes;
the cause there is not traced.

## Not covered

- **Line 113 under a six-line mosaic block.** With the mosaic size 6 the block of picture rows
  108-113 starts in the upper view, so the lower view's first row samples BG1 four lines above
  its camera, at bottom scroll. The original shows track there that native's world-drawn BG1 does
  not. Hypothesis, not measured (no capture here reads VRAM): that tile-map row lies outside the
  rows the race streams for the lower camera and holds older map data, which native, not modelling
  BG1's map streaming, cannot draw. All 9 `demo-pic` cases have mosaic size 6, but most size-6
  pictures match. The next experiment is a VRAM capture of BG1's map at one of these frames.
  9 pictures of `demo-pic`, 5 of `vs-4`, one row each. Sampling the upper camera's scroll there instead was tried and is wrong (159 more
  unequal pictures on `demo-pic`).
- **Power bounce in a race with two humans** changes rider 1's state (Race state above); the
  pictures of `twop-104` and `vs-124` after the divergence are not compared.
- The league's two-view races are compared on LEAGUE-TOURS' sampled pictures only (every tenth
  frame), and track 42's split picture (NEON) differs from the original's with or without HUNTER's
  effects; not taken up here.
- `tours-a-w2` 70040: 8 pixels at x 0-2 of lines 113-115 (an object at the lower view's left edge,
  the original's, under slow motion), the same on main.
- The NMI's channel 4 under the blink (`$80:8715-8753`: the tables at `$7E:2332`/`$7E:24F8`, BG1's
  vertical scroll, zero in a two-view race) is not reconciled with the unflipped BG1 the pictures
  show; native's BG1 is equal on all of `demo-pic`'s and the 2P and VS captures' invisible-unis
  pictures.

## Reproduction

```sh
zsh local/evidence/split-hunter-pictures/captures.sh demo-pic twop-104 vs-124 vs-4 one-67
local/locks/heavy-run.sh SPLIT-HUNTER-PICTURES zsh local/evidence/split-hunter-pictures/run_checks.sh OUT
python3 local/evidence/split-hunter-pictures/race_check.py twop-104 BINARY PACK --boundary
python3 local/evidence/split-hunter-pictures/search.py 2p 1 200
```
