# R-0054 - Boot, title and main menu

Status: recovered and implemented on `task/front-end-main-menu` (FRONT-END-MAIN-MENU),
26 September 2026. PAL ROM `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`,
audited bsnes core `e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b`. Follows
COVERAGE-ROADMAP's map of the game's modes.

From power-on the original shows the Nintendo screen, then the title, then the main menu. There
it waits for a choice among 1P, 2P, VS, LEAGUE and OPTIONS, or starts the demo after 480 idle
frames. Native now does the same, frame for frame (`src/core/front_end.cpp`).

## Timeline of a cold start

From every-frame images of two captures, `cold` (no input) and `cursor` (Start at 300, then
cursor moves):

| Frames | Screen |
| --- | --- |
| 0-103 | Black: initialisation and the sound program's upload |
| 104-109, 110-221, 222-227 | The Nintendo screen fades in, holds, fades out |
| 228-250 | Black: the title loads |
| 251-256, 257-368, 369-374 | The title (logo, rider art, "TM 1994 NINTENDO") fades in, holds, fades out |
| 375-409 | Black: the main menu loads |
| 410-415, from 416 | The main menu fades in; the arrow flies in from 420 |

Start pressed during the title changes nothing. The two captures' frames 0-419 are identical.
With no input the demo starts at frame 900.

## The top level

Reset (`$80:8858`) initialises and calls, in order:
- `$80:A09A`: the first loads; the sound program's upload ends by frame 97.
- `$80:B08C`: the Nintendo screen.
- `$80:A16A`: the direct-page state of a screen.
- `$80:F55F`: the title.
- `$80:D20E`: the main menu's screen.
- `$80:ACD5`: the menu's text.

It then enters the main loop at `$80:8881`, which runs the main menu `$80:ABC8` and dispatches
the mode `$9B` (COVERAGE-ROADMAP).

## Loads

Assets come from the directory at `$82:B332`, and all these are raw. `$82:B1DB` copies to VRAM,
from a word address. `$82:B183` copies to CGRAM, from a colour.

| Frame | Routine | CGRAM | VRAM (word address) |
| --- | --- | --- | --- |
| 24 | `$80:A09A` | 5 at 224 | 88 at 0x7000 |
| 99 | `$80:B08C` | base palette at 0 (DMA, `$80:A8A8`/`$80:A8D0`), then 31 at 0 | 80 at 0x1000, 74 at 0x2000 |
| 228 | `$80:F55F` | 27 at 0 (256 colours) | 78 at 0, 72 at 0x1000 |
| 377 | `$80:D20E` | base palette at 0, 1 at 112, 2 at 240 (`$83:91F7`), 28 at 208 | 89 at 0x6000, 91 at 0x7D00, 77 at 0, 70 at 0x2000, 68 at 0x31A0, 69 at 0x3D80 |
| 407 | `$80:ACD5` | - | the printed map (work RAM `$0200`) at 0x1000 |
| 408, 418 | `$80:A8A8` | base palette at 0 | - |
| 409, 419 | `$80:A877` | - | asset 69's first 1,920 bytes at 0x3D80 (DMA from `$84:AAF8`) |

- The base palette is 216 bytes at `$80:A8D4`, colours 0-107.
- `$83:91F7` loads a rider's colours at 240 in place of asset 2 when an SRAM flag is set. On a
  cold start it is clear.
- The main menu's assets 77, 70, 68 and 69, with 89, are the result screen's base VRAM pieces
  (R-0053): the two screens share their background and fonts.

## Registers

Every screen uses mode 3: BG1 8bpp, BG2 4bpp.

- **Nintendo**: `$80:A09A` sets the registers at 97. The screen is BG2 only (TM 2 from 103),
  map at word 0x1000, tiles at 0x2000.
- **Title** (228): BG1 only, map at 0, tiles at 0x1000 (BG12NBA 1).
- **Main menu** (377):
  - BG1 carries the logo: map at 0 (32 x 64), tiles at 0x3000.
  - BG2 carries the checks and the text: map at 0x1000 (64 x 64), tiles at 0x2000.
  - Objects use OBSEL 0x63: 16 x 16 and 32 x 32 at word 0x6000.
  - TM is BG1, BG2 and OBJ; TS is OBJ.
  - Colour math adds the subscreen at half on every layer (CGWSEL 2, CGADSUB 0x7F), which is how
    the arrow's shadow is drawn.

Fades: `$80:9869` sets INIDISP to 2, 4, ..., 14 on seven frame waits, and `$80:9885` to 13, 11,
..., 1 and then 0x80. The screens hold at brightness 14, and each holds for 111 frame waits.

## The frame model

Image f shows what frame f's vblank wrote.

- **The frame wait** (`$80:FADF`) runs the arrow update `$80:FAF5` each time. It runs on frames 98,
  99, 104-228, 251-377 and 403 (`$80:D36F`), and on every frame from 407. `$80:D1EC` copies OAM (`$80:9318`)
  and reads both pads into `$72`/`$74`.
- **The NMI hook** (`$53` = `$80:F622`, installed by `$80:A16A`) runs from frame 250, once
  NMITIMEN is 0x81 (`$80:F5B8`). It runs on every frame from then on, loads included.
  - It may scroll BG1 vertically by 2 a frame. That is gated by `$77:0742` bit 1, which is clear
    in these captures.
  - It runs the palette cycle `$80:FA60`. `$C8` counts 6 to 0; at each wrap `$C9` counts 3 to 0,
    and CGRAM 111, 110, 109 and 108 take the words at `$80:FACD + 2*$C9` and on (`$80:FA82`).
    The four colours rotate through CGRAM 108-111: the result screen's cycle.
  - `$80:A16A` zeroes both counters (228, 377).
- **The arrow**: OAM 119, and its shadow OAM 127.
  - `$C6` counts 31 to 0 and wraps. The tile of both sprites is `$80:FBC5[$C6 >> 1]`
    (`$80:FB08`).
  - x moves `$0C60 += ($0C62 - $0C60) >> 2` as a 16-bit arithmetic shift, when the gap is not 0.
    A positive gap under 4 moves 0, so the arrow settles three sixteenths short of its x target
    (0x4FA for 0x4FD). A negative gap always moves.
  - OAM x is `$0C60 >> 4`, and the shadow's is 7 more. The ninth bits come from the signs of
    `$0C60` and `$0C60 + 0x70`.
  - y moves the same way from `$0C68` to `$0C6A`. OAM y is `($0C68 >> 4) - 0x12`, and the
    shadow's is 7 more.
  - `$83:99F6` parks it at (0xFD00, 0x700) at 99 and 402 (`$83:99FA`). `$80:A16A` (228, 377) and
    the main menu (419) aim it at the 1P entry (0x4FD, 0x580). It flies in on the title too,
    where TM leaves objects off.
- **The menu's object layout** at 402 (`$80:D2C3-D36C`):
  - Entries 100-103 wait off the right edge.
  - Entries 104-111 wait below the screen, with tiles `$80:9B31` and attributes `$80:D383`.
  - The arrow is palette 7 at priority 3, and its shadow palette 5 at priority 0; both are large.
- **The map buffer**: `$80:ACD5` fills work RAM `$0200` with 0x004C, prints `$80:AD1F`
  (`$80:ACF1`: `F9 07`, then `FC row` and each entry) through the text printer (R-0053), and
  copies the map to VRAM. OAM is copied again at 403 (`$80:D372`).
- **The text printer** (R-0053), as native ports it (`src/core/text_printer.cpp`):
  - Bytes from 0xEE are control codes, dispatched by 0xFF - code (`$80:C3C6-C3D9`).
  - A backquote prints as an underscore, and bytes from 0x8C as `?` (`$80:C3FE-C40C`).
  - The menus use FF (end), FE (column, row), FC (centre on a row), FB (nothing) and F9
    (attribute: `$B0` = its argument << 10). Native refuses the others until a screen needs
    them.

## The main menu ($80:ABC8)

- **Initialisation** at 419: `$9B = 0`, the idle count `$89 = 480` and the arrow's targets. The
  loop (`$80:ABE3`) starts at 420.
- **Each frame**:
  - the frame wait, then `$80:D1EC`;
  - the two codes below;
  - the idle count. `$89 - 1` is stored only while it is not negative
    (`$80:AC0F-AC14`); otherwise the mode is 5, the demo;
  - Choose (`$80:B71D`): B, Start or A on either pad;
  - Down (`$80:B794`): Down or Select;
  - Up (`$80:B76F`);
  - with neither Up nor Down, the latch `$8F` clears.
- **A move** needs `$8F` clear. It plays sounds 0x087F and 0x0203 (`$80:B178`), sets `$8F`, sets
  `$89` to 1,500, and steps `$9B` with the wrap: Down past OPTIONS sets the y target to 0x400,
  Up past 1P to 0xD00. Then the x target is `$80:88BE[$9B] << 7` (10, 10, 10, 6, 5 columns),
  and the y target moves by 0x180, which is 24 pixels.
- **Opposing directions**: the reference emulator models the pad's rocker, so Up with Down, or
  Left with Right, reads as neither. The `buttons` capture pressed Up and Down together, and the
  menu did not move. Native applies the same rule (`physical_pad`, as `with_physical_dpad` does
  for the race, R-0041).
- **The codes**:
  - On the title, while it holds, `$72` is compared with Up, Left, Up, R, A (`$80:F618`). A
    match saves `$77:10D3-10E2` and sets `$77:10D0`.
  - On the main menu the codes are tested first, as exact pad words on either pad, WIPE RAM's
    first (`$80:ABEB-AC0A`). Left+A+L+R (0x02B0) opens a two-entry menu, WIPE RAM and MAIN MENU
    (`$80:A9B4`, text at `$80:A9FA`).
  - B+Down+L+R (0x8430) waits 31 frames and jumps to `$83:AB9A` (`$80:F0D6`). This is not read
    yet.
  - None of the three is implemented natively: their effects are SRAM content, and a mode not
    yet recovered.

## The SNES screen

`src/core/snes_screen.cpp` draws a picture from VRAM, CGRAM, OAM and the registers as the
reference emulator's fast PPU does (`sfc/ppu-fast`). It covers the backgrounds in modes 0, 1
and 3, the objects with their 32-per-line and 34-tile limits, colour math and the brightness.
Screen row 0 is scanline 1, as in native's race backgrounds, and an object is drawn one line
after its OAM y, as bsnes stores it. Windows, mosaic, HDMA (per-line register changes), OAM
priority rotation, hires, mode 7 and offset-per-tile are not modelled: a screen that uses one
must set `SnesVideoRegisters::unmodelled_features`, and is then refused. The original turns
HDMA on at frame 743 of the `buttons` capture, once 2P is chosen, so the 2P screens will need
it.

## Evidence

`local/evidence/front-end-main-menu/`:

- **Captures**:
  - `cold`, `cursor`, `buttons`: manifests, coverage captures with frame images;
  - `wram-*`: per-frame work RAM 0x0000-0x1FFF;
  - `access-*`: register and port accesses;
  - `provenance-cold`: the DMA and port inventory;
  - `ppu-writes.txt`: every PPU register write of `cursor` with its frame.
- **`compare.py`**: native's `front_end_runner` against a capture. It compares, frame by frame,
  the arrow (`$C6`, `$0C60`/`$0C62`/`$0C68`/`$0C6A`), the menu (`$89`, `$9B`, `$8F`), the palette
  cycle (`$C8`, `$C9`) and the whole OAM buffer (`$0A00-$0C1F`), and the pictures.
- **`cursor`**, 1,000 frames: Down six times (the cursor wraps past OPTIONS), then Up three times
  (it wraps past 1P).
  - Every compared word and the OAM buffer are equal on all 1,000 frames.
  - All 1,000 pictures match, with 0 differing pixels.
- **`cold`**: equal on frames 0-900; native starts the demo at 900, as the original does. All 901
  pictures match.
- **`buttons`**: Select as Down; Up and Down together, which move nothing; Down held 61 frames,
  which moves once; controller 2's Up; controller 2's A, which chooses 2P at 700 as the
  original's mode 1 handler starts.
  - Equal on frames 0-700.
  - Pictures 400-700 match.

## Paths a cold start does not take

These were read in the listing but not exercised by the captures. Native follows the cold
start's path:
- **Pad reads**: the main menu's tests read pad 1 only while `$77:0742` bit 9 is clear, and
  pad 2 only while bit 10 is clear (`$80:B71D`, `$80:B76F`, `$80:B794`). Both are clear on a
  cold start.
- **Entering the main menu** (`$80:8889-8897`): when `$77:0742` bit 4 is set, it is cleared and
  `$80:D503` runs.
- **Reset's SRAM routines** `$83:8AF7`, `$80:8C4E` and `$83:8B23` run at 403-405; their effects
  are SRAM content (persistence).
- **`$80:ACD5` prints the menu's text only while `$00A6` is clear**. It is clear on the first
  pass; a return from a mode sets it and keeps the map.

## Not recovered

- The demo (mode 5), and the modes 2P, VS, LEAGUE and OPTIONS (later tasks).
- The three codes' effects. Native reports the main menu's two codes as their own outcomes
  (`FrontEndMode::wipe_ram_code`, `unread_code`), so an exact code never chooses a mode; the app
  shows a notice.
- The NMI's BG1 scroll gated by `$77:0742` bit 1.
- Audio: the sound commands (`$82:8000`) are not played.
- Why the loads take the frames they do. The boot uses the measured frames, which are fixed for
  a cold start under the reference emulator.
