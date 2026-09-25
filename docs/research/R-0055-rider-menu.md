# R-0055 - PICK YOUR UNI, the one-player rider menu

Status: recovered and implemented on `task/front-end-1p-setup` (FRONT-END-1P-SETUP part 1),
26 September 2026. PAL ROM `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`,
audited bsnes core `e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b`. Follows
R-0054 (the boot and the main menu) and R-0053 (the text printer).

1P on the main menu starts mode 0 (`$80:BB9C`). Its first screen is PICK YOUR UNI:
- 16 riders in two columns of eight, each with a unicycle icon in the rider's colours;
- the red arrow on the rider chosen last.

The d-pad moves the arrow. B, Start or A chooses; Y or X goes back to the main menu. Native
now does the same, frame for frame (`src/core/rider_menu.cpp`). What follows a choice (PICK TOUR,
PICK TRACK and NOW PLAYING) is R-0056.

## Timeline

1P chosen on frame c (the main menu's loop reads the choice and enters `$80:BB9C` in the same
frame):

| Frames | What happens |
| --- | --- |
| c | `$80:F51B` sets `$77:0742` bit 1: from c + 1 the NMI hook slides the logo (BG1) up 2 lines a frame to 0x52, 41 frames. `$83:9543` makes the menu read pad 1 only. `$80:CB04` starts: `$009B` = the last rider (`$017D`, `$80:CB07`), the Up/Down latches `$008F` cleared (`$80:CB0C`) |
| c + 1 | OAM copied; `$80:95AB` clears the text map `$0200` (`$83:8B51`) and prints the 16 names; `$80:C3BC` prints the title `$80:BCAF` |
| c + 2 | OAM copied; `$80:937B` copies the map to BG2's hidden half (`$00A8`); `$80:98A4` sends the arrow off the left; `$83:8E3A` builds uni picture 0x12EE |
| c + 3 | `$80:F818` copies the picture to VRAM word 0x7000; the forward slide `$80:E233` starts |
| c + 4 to c + 42 | The slide's 39 passes, one a frame; at c + 42 the halves swap, then `$80:CB50-CBC6` lays out the icons (`$80:9625`) and aims the arrow |
| c + 43 on | The menu's loop `$80:CBC8`, one pass a frame |

Captured: c = 620 (`defaults`), 430 (`moves`, `back`), 700 and 900 (`back`, second and third
entries) and 420 (`held`); the timeline is the same from each.

## The text

`$80:95AB` prints each name at column 5 (even riders) or 19 (odd riders), on text row
4 + 3 * row, left column first. The text code F8 (`$80:C5D3`) copies the rider's 16-byte record
from SRAM `$77:000C + 16 * rider` (`$80:9B2F`) and prints it up to its 0xFF. A cold start's
records are the ROM's `$83:800C` (22 records), which `$83:92CC` copies into SRAM when the SRAM
signature check (`$80:8C4E`) fails.

The printer keeps its attribute `$00B0` between prints. The names and the title therefore
print in the main menu's `F9 07` (palette 7).

## The slides ($80:E233, $80:E27E)

BG2's map is 64 x 64 tiles at VRAM word 0x1000. Its left and right 32-column halves are at
0x1000 and 0x1400: `$00AA` is the half shown, `$00A8` the hidden one. A screen's new text goes
into the hidden half, which then scrolls into view:
- The countdown `$0076` runs from 0x26 to 0: 39 passes.
- The speed `$00A1` rises by 1 while the countdown is 0x1F or more, and falls by 1 below 7. So
  it runs 1..8, holds at 8, then 7..1, 256 pixels in all.
- Each pass waits for a frame, copies OAM and writes `$0090` to BG2HOFS.
- The halves swap at the end.
- The forward slide (`$80:E233`) adds; the way back (`$80:E27E`) subtracts.

`$0090` gains 0x4C00 each pass besides the speed. `$00A2`, the high byte of the word add, holds
0x4C from `$80:A1A1`. 0x4C00 is a multiple of 0x400, so BG2's offset is unchanged. Native keeps
the offset alone, and the comparison masks `$0090` to its low 10 bits.

The forward slide's pass also runs `JMP ($0056)`, which is `$83:9A1E`, the main menu's
decoration animator. Its objects (entries 96-114) are all hidden during the slide, so only the
OAM buffer shows it:
- **Every third pass** (`$0089`'s low byte, 2 down to 0):
  - entries 96-97's and 98-99's tiles step through `$83:9AF9` and `$83:9B01`;
  - entries 112-114's tiles step through `$83:9B27`.
- **Every other pass** (`$008B`):
  - entries 104-111's tiles step through `$83:9B31` (the counts at `$0187`);
  - entries 100-103's columns sway (`$83:9B09`, `$83:9B13`).

The animator's step counter `$0089` is the main menu's idle count's low byte, so its first step
depends on when 1P was chosen. Its other counters (`$0191-$0194`) start at 0 on a cold start and
are never reset. `$80:D2C1` resets only the wave (`$0187` from `$80:D37B`, `$008B` = 1).

## The menu's loop ($80:CBC8)

Each pass, after its frame wait:
- `$0C1F` = 0x55 keeps the arrow's shadow (entry 127) hidden (`$80:CC0C`);
- `$80:D1EC` copies OAM and reads the pads;
- `$80:A705` DMAs riders 0-7's palettes into colours 0x80-0xFE;
- `$80:93A5` sends the text again, to the half it was first shown in (`$005A`, now hidden);
- the HDMA (below) is set up;
- `$80:F818` sends the uni picture built on the last pass;
- the pad is read.

**The icons** (`$80:9625`): each rider's icon is six objects (one 32 x 32 and five 16 x 16)
showing one shared 48 x 40 uni picture, in palette rider & 7:
- the left column is mirrored;
- rider 15 takes OAM entries 0-5 and rider 0 entries 90-95.

**The picture** (`$83:8E3A`): a frame of the race riders' pose directory (`$20:8000`), five rows
of six tiles, uploaded to object tiles 0x100-0x145.
- Frames 663-719 of `defaults`: the uni builds up over pictures 0x12EE-0x130A. The counter
  `$0076` runs from 0x25DC, a picture every two frames.
- Then it loops through 0x1320 down to 0x130D, two frames each (`$80:CD5C`, indexed by `$0190`,
  39 down to 0).

**The palette split** (HDMA, `$80:CC1A-CC47`): 16 riders' colours from 8 object palettes.
- Channel 0 writes CGADD: 0 on line 0, 0x80 on line 64 (`$80:CD47`).
- Channel 1 writes CGDATA (`$80:CD84`): colour 0 = black on line 0, then one colour a line
  from line 64, through colours 0x80-0xFF.
- Palette p holds rider p's colours above line 64 + 16p and rider p + 8's from there. Rider p's
  icon (row p / 2) is above that line, and rider p + 8's (row 4 + p / 2) below it.
- `$80:A72B` (riders 0-7) and the HDMA blocks (riders 8-15) are byte for byte assets 6-21;
  the pack tool checks this. So rider r's palette is asset 6 + r.
- The 255-byte DMA leaves colour 0xFF, which keeps rider 15's last colour.
- bsnes caches CGRAM per line, and the HDMA write of table line n comes at the end of scanline
  n. So it shows from screen row n, which is scanline n + 1. `snes_screen` applies these line
  colours (`SnesLineColour`).
- Where rider 15's icon meets palette 7's band (rows 184-191), the top of STEVE's icon is drawn
  with a partly rewritten palette. The captures' pictures confirm this.

**The arrow**: OAM entry 119, moved by `$80:FAF5` in each frame wait (R-0054).
- Targets: x 0x680 (left column) or 0x780 (right); y (24 * row + 41) * 16 (`$80:CB62`).
- Attributes (`$0BDF`): priority 3, palette rider & 7, mirrored in the left column so it points
  at the name. The arrow takes the selected rider's colours; rows 4-7 get them through the
  split.
- At `$80:CB62` the attribute is written as a word, which zeroes entry 120's x.

**The pad** (`$80:CC52-CD46`, pad 1 only):
- **Left and Right** change the column only when it changes.
- **Up and Down** move once a press (the latches `$008F` bits 3 and 2) and stop at rows 0 and 7.
  - A move up skips the Down test.
  - A move down ends the pass before the choice test.
- **Choice**: B, Start or A, or Y or X for back (Y wins over a choice). `$009B` = 2 * row +
  column, with 0x80 for back.
- A choice needs no release first. Its latch (`$0010` bit 0) is never set on this way, so Start
  held from the main menu chooses MIKE on the menu's first frame (`held`).
- The menu has no idle timeout.

## The way out

**A rider chosen** (`$80:BBB8`):
- The arrow is sent off (`$80:98A4`).
- `$017D` = `$00CA` = the rider; `$017F` = 0x10.
- SRAM writes: `$77:0742` bit 3 cleared, `$77:1075-10A6` cleared, `$77:1073` = 3,
  `$77:10AD` = 1.

**Y or X** (`$80:BC9B`): `$77:10AD` = 0.

Both then run `$80:F4E9`:
- **c**: the HDMA stops at once, so that frame's picture is unsplit.
- **c + 1**: `$80:D2C1` hides every object, lays out the main menu's objects and parks the
  arrow.
- **c + 2**: OAM copied.
- **c + 3**: OAM copied, then CGRAM 0xF0 is loaded (`$83:91F7`): the chosen rider's asset
  6 + rider, or asset 2 after Y. Then asset 28 at 0xD0 and asset 5 at 0xE0 (`$80:F502`).

After a choice, PICK TOUR follows (R-0056). After Y, `$83:9558` lets both pads count
again, and the main loop's `$80:ACD5` runs with `$00A7` set:
- **c + 3**: `$80:D1FA` clears the map and the main menu is printed.
- **c + 4**: the map goes to the hidden half; `$009B` = 0; the slide back `$80:E27E` starts.
- **c + 5 to c + 43**: the slide back. At c + 43 `$80:F52B` clears `$77:0742` bit 1, so the
  logo slides back down from c + 44.
- **c + 44**: `$80:A8A8` reloads the base palette.
- **c + 45**: `$80:A877` reloads the text tiles; the main menu starts (idle 480, on 1P).

## The pack (profile v16)

`tools/unirally_lab/content/front_end.py` `v16_new_entries`, all raw ROM:
- assets 6-21, the riders' palettes;
- `front-end.rider-names` (`$83:800C`, 352 bytes);
- `front-end.pick-rider-title` (`$80:BCAF`);
- `front-end.decoration-frames` (`$83:9AF9`, 76 bytes).

The uni pictures come from the race's pose tables (`presentation.rider.*`).

## Evidence

`local/evidence/front-end-1p-setup/`:
- **Captures** (every-frame images from 400, per-frame work RAM 0x0000-0x1FFF):
  - `defaults`: images from 600. Start at 300, 620, 750, ...: the defaults to the race.
  - `moves` (1,100 frames): 1P at 430, then:
    - Down held during the slide (ignored);
    - Right, Down twice, Left;
    - Up four times, the last two against the top;
    - Down nine times, the last two against the bottom;
    - Right, Up;
    - Left with Right, Up with Down (the pad's rocker cancels both);
    - Right held 30 frames, Select, L with R;
    - A at 1000: ROBBIE (13).
  - `back` (1,200 frames):
    - 1P at 430, Down, Right, controller 2's Down (ignored), Y at 540: the main menu from 585;
    - Down and Up there, 1P again at 700, Down, X with Start at 790 (back wins);
    - 1P at 900, Right, B at 980: ANDREW (1).
  - `held` (700 frames): Start held 419-478: 1P at 420, MIKE at 463.
- **`compare.py`** runs native's `front_end_runner` against a capture. Every frame it compares:
  - the arrow, the palette cycle, the logo `$00AD`, the map halves and BG2's offset;
  - the decoration animator's counters, `$009B`, and `$0089` (the main menu's idle count on its
    own frames, the animator's delay otherwise);
  - the slide's countdown and speed; the rider menu's row, latches, picture counters and
    `$017D`;
  - the OAM buffer and the text map `$0200-$09FF`;
  - and the pictures.
- **Results**: every compared word, the OAM buffer and the text map are equal on every frame of
  the four captures, which run on into PICK TOUR (R-0056):
  - `held`: 700 frames;
  - `moves`: 1,100 frames;
  - `back`: 1,200 frames;
  - `defaults`: 1,208 frames, to the race.

  Pictures: 300, 700, 800 and 608, all 0 differing pixels.

## Paths the captures do not take

- **The 2P handler** re-enters the loop at `$80:CBC3`, with `$0076` = 0, so it skips the intro
  (inferred).
- **`$0010` bit 0**: its setters are in untraced code at `$80:9D0B-9E07`.

## Not recovered

- The meaning of `$017F` = 0x10 and of the SRAM words written on a choice. Their readers are the
  later screens and the race.
- Audio: the move, choose and slide sounds (`$80:B178`, `$80:B124`, `$80:B139`, `$80:B14E`) are
  queued, not played.
