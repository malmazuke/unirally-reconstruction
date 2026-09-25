# R-0054 - Boot, title and main menu

Status: in progress on `task/front-end-main-menu` (FRONT-END-MAIN-MENU), 25 September 2026.
PAL ROM `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, audited bsnes
core `e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b`.

DRAFT NOTES (to be rewritten as the record):

## Timeline of a cold start (no input), from every-frame images (`cold`, `cursor` captures)

- 0-103 black; 104-109 fade in; 110-221 Nintendo screen ("(c) 1994 NINTENDO/DMA DESIGN");
  222-227 fade out; 228-250 black; 251-256 fade in; 257-368 title (unirally logo, rider art,
  "TM 1994 NINTENDO"); 369-374 fade out; 375-409 black; 410-415 fade in; 416-419 main menu
  without the arrow; from 420 the menu with the arrow, changing every frame (palette cycle).
- Start at 300 (during the title) changes nothing: the `cursor` capture's frames 0-419 equal
  `cold`'s.
- With no input the demo starts at frame 900 (main menu idle 480 frames from 420).

## Screen loads (asset ids through `$82:B2DD`, VRAM copier `$82:B1DB`, CGRAM `$82:B183`)

All raw (uncompressed). Word addresses.

| Frame | Screen | CGRAM | VRAM |
| --- | --- | --- | --- |
| 24 | (first load) | asset 5 at CGADD 224 | asset 88 at 0x7000 |
| 99-100 | Nintendo | asset 31 at 0 | 80 at 0x1000, 74 at 0x2000 |
| 228-229 | title | asset 27 at 0 | 78 at 0x0000, 72 at 0x1000 |
| 377-394 | main menu | 1 at 112, 2 at 240, 28 at 208 | 89 at 0x6000, 91 at 0x7D00, 77 at 0, 70 at 0x2000, 68 at 0x31A0, 69 at 0x3D80 |

Plus DMA: CGRAM `$00:A8D4` (ROM 0x28D4) 216 bytes at 99, 377, 408, 418 (pc $80:A8D0);
VRAM from ROM `$04:AAF8` 1920 bytes at 409, 419 (pc $80:A8A0); VRAM word 0x1000 from WRAM
`$00:0200` 2048 bytes at 407 (pc $80:939D, a tilemap built in WRAM); OAM from WRAM `$0A00`
544 bytes every frame from 99 (pc $80:9338).

The menu's assets 77, 70, 68, 69 are the result screen's base VRAM pieces (same ROM ranges).

## Registers (writes; every screen mode 3)

- Nintendo (97): BGMODE 3, BG1SC 2, BG2SC 0x13, BG12NBA 0x23, OBSEL 0x63, TM 0x11, TS 0x10,
  CGWSEL 2, CGADSUB 0, windows 0; at 103 TM 2, TS 0.
- Title (228): BGMODE 3, BG12NBA 1, TM 1, OBSEL 0x63.
- Menu (377): BGMODE 3, BG1SC 2, BG2SC 0x13, BG12NBA 0x23, OBSEL 0x63, TM 0x13, TS 0x10,
  CGWSEL 2, CGADSUB 0x7F.
- Fades: INIDISP 2, 4, ..., 14 one frame each ($80:987A), out 13, 11, ..., 1 ($80:9898), then
  0x80.
- Menu palette cycle ($80:FA74-FAC6, from frame 250 to 994 in `cursor`, 108 writes each):
  CGRAM 108-111 take the four colours 0x52B5, 0x4A56?, ... rotating (the result screen's cycle,
  R-0053).

## Main menu code ($80:ABC8)

- $9B = 0, $8F = 0, $A6 = 0; idle $89 = 480 (0x1E0); arrow sprite y $0C6A = 0x580, x $0C62 = 0x4FD.
- Loop: JSR $FADF (wait frame?), JSR $D1EC; $72/$74 == $02B0 -> $A9B4; == $8430 -> $F0D6;
  $89 -= 1, when negative: $9B = 5, $A6 = 1, return (demo).
- $B71D nonzero: $A6 = 1, return (Start: choose). $B794: Down? ($9B += 1, wrap 5 -> 0 with
  $0C6A = 0x400); $B76F: Up? ($9B -= 1, wrap -1 -> 4 with $0C6A = 0xD00). A move: JSR $B178
  (sound?), $8F = 1, $89 = 1500 (0x5DC), $0C62 = table[$AE + $9B] << 7 ($AE = $88BE: 10, 10, 10,
  6, 5), $0C6A -+ 0x180. $8F = 1 blocks a repeat until released?

## The arrow object ($80:FAF5, called from the frame wait $80:FADF every frame)

- `$C6` counts down 31..0 and wraps; tile = `$80:FBC5[$C6 >> 1]` for both sprites (OAM 119 at
  `$0BDC`, 127 at `$0BFC`: the arrow, palette 7 priority 3, and its shadow, palette 5 priority 0).
- x: `$0C60 += (($0C62 - $0C60) >> 2)` as a 16-bit arithmetic shift (LSR twice, then bit 13
  sign-extended); skipped when equal. A positive gap under 4 moves 0, so x settles 3 short
  (0x4FA for target 0x4FD); a negative gap always moves. OAM x = `$0C60 >> 4` (low byte), the
  shadow +7; the ninth bit of each from the sign of `$0C60` and of `$0C60 + 0x70` (`$0C1D`,
  `$0C1F` bit 6).
- y the same with `$0C68`/`$0C6A`; OAM y = (`$0C68 >> 4`) - 0x12, the shadow +7.
- The menu setup ($80:D20E at 375-402) places it at x 0, y 0xF00 and sets the targets to
  0x4FD, 0x580 (`$80:A16A`), so it flies in from the left; the title does the same from 251
  with OBJ off the main screen (invisible). The reference per-frame WRAM is the `wram-cursor`
  series (0x0000-0x1FFF, frames 0-999).

## Title ($80:F55F)

- Loads CGRAM asset 27 at 0, VRAM 78 at 0 and 72 at 0x1000; BG12NBA 1, TM 1 (BG1 only), mode 3;
  fades in; then 111 frames (`LDY #$6E`) of: frame wait, `$80:D1EC` (OAM DMA and the joypad
  read into `$72`/`$74`), and a compare of `$72` with a five-word sequence at `$80:F618` (a code;
  on a match it saves `$77:10D3-10E2` and sets `$77:10D0`).

## Menu setup ($80:D20E)

- Frame wait, `$80:A8A8`, force blank, `$80:A16A` (direct-page state and the arrow targets), the
  registers above, CGRAM 1 at 0x70, `$83:91F7`, CGRAM 28 at 0xD0, VRAM 89, 91, 77, 70, 68, 69,
  the arrow reset, OAM cleared to y=1, `$0C00` filled with 0x55, the arrow's OAM entries, then
  small sprites from `$80:9B31`/`$80:D383` (OAM 104-111 at `$0BA1`...).
- Layers: BG1 8bpp, map at word 0 (32x64), tiles at word 0x3000; BG2 4bpp, map at word 0x1000
  (64x64; the WRAM tilemap `$0200` built by the text printer is its first page), tiles at word
  0x2000; OBJ base word 0x6000 (OBSEL 0x63: 16x16 and 32x32). TS = OBJ, CGWSEL 2 (subscreen),
  CGADSUB 0x7F (add, half, all layers): the arrow's shadow on the subscreen.
