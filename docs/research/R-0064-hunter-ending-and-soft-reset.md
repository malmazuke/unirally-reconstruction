# R-0064 - HUNTER's gold ending, the soft reset, and the codes

Status: recovered and implemented on `task/hunter-ending` (HUNTER-ENDING), 26 September 2026. PAL
ROM `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, audited bsnes core
`e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b`. Follows R-0062 (the other tours'
endings) and R-0054 (the boot screens and title).

HUNTER's medal starts at silver, so its first completion, open only at level 3, plays its ending
(`$83:AB9A-AEF5`: the routine and its tables). Unlike the other tours' endings it reads the pads,
ends in a soft reset, and runs no unlock rule and no checksums.

## The ending

- **The fade** is the award's (`$83:A4E9`).
- **Two newspaper pages** in mode 1, BG1 only: DAILY NEWS (assets 0x67 colours, 0x6E map, 0x6B
  tiles), then SPEEDKING (0x68, 0x6F, 0x6C). With the title code's flag `$77:10D0` set, one CHEAT!
  page (0x66, 0x6D, 0x69) takes their place.
- **Each page rolls down** (`$80:E2CF-E3E8`): for 77 frames HDMA writes INIDISP and BG1VOFS a line at a
  time from the tables `$80:E37B` (38 bytes) and `$80:E3A1` (72). On reveal frame n the top 2 + 3n
  rows show the page, the next 31 show it upside down with a rising brightness and a moving offset,
  and the rest are forced blank. The band also shows the menus' map rows 30-31 left in VRAM.
- **The waits.** The first page waits, with no time limit, for either pad to read nonzero on two
  frames in a row (`$80:C202`, `$80:C20C`). The second waits for any of pad 1's twelve buttons or for
  1,201 frames.
- **The credits** ("--WHODUNNIT--", the text `$83:AE01`, printed by `$83:A93A`): 33 objects from
  `$83:AE72`, eleven faces (asset 0x3A, tiles from 0x5D) on eleven unis whose poses cycle through
  `$83:AE12` (0x1350 to 0x1368 and back, two frames each), with a second picture sent to VRAM 0x7500.
  They run until a press, then fade.
- **`JML $80:8858`:** the power-on code again. It clears work RAM and VRAM and rewrites every
  register, but cartridge RAM survives. The records wipe (`$80:8C4E`, boot frame 403) is skipped
  because the save's signature (`$83:8000`) is intact. After the reset `$77:0742` bit 1 and
  `$77:10AD` are cleared, and `$77:1FFF` is set to 0x56 by the memory-size test. Because `$77:10AD` is
  still set when the main menu loads its colours, its arrow is red once.

The ending's routine points are listed in the code at `$83:AB9F`, `$83:ABB3`, `$83:ABB9`,
`$83:ABDA`, `$83:ABFD`, `$83:AC1F`, `$83:AC32`, `$83:AC3B`, `$83:AC6D`, `$83:ACDD`, `$83:AD33`,
`$83:AD67`, `$83:AD9B` and `$83:ADFA`, and the page load at `$80:E2D8`, `$80:E322` and `$80:E369`.

## The reset's timing

Power-on frame f shows at J + f after the reset J, except that the sound program's upload ends
later: frames 97-403 come 3 frames late after a first reset and 2 after a second. So the Nintendo
screen fades in at J + 107 on the long capture. The delay is measured, not derived; native keeps it
as a state field (3 by default), and the laboratory runner takes it from a capture
(`--reset-upload-delay`), as it takes race start frames. The boot's frames from 407 on shift by the
delay less 3. The boot's frame 18 (`$80:B612`) runs while the screen is still blank.

## The codes

- **The title code** Up, Left, Up, R, A (`$80:F5C0-F612`; the fourth word is R, 0x0010) sets
  `$77:10D0`. Every level is then 3 and HUNTER's ending shows the CHEAT! page, until the next boot's
  title (`$80:F564-F580`) puts back the saved levels and clears the flag. A code entered on a cold
  start does not last: the cold start's wipe at boot frame 403 (`$83:FB41`) erases it. Native now
  wipes the records at frame 403 on power-on, and keeps them after a soft reset.
- **The main menu's code** B, Down, L and R jumps into HUNTER's ending after 31 frames, with no
  medal change; the code-route capture reaches the reset at frame 1316.

## Native

- `hunter_ending.cpp`: the pages, their reveal, the waits, the credits and the reset. The renderer
  applies HDMA's per-line INIDISP and BG1VOFS over the CPU's forced blank; it still refuses other
  per-line register changes.
- The boot runs again from the reset frame, with the records kept, the delay, and the red arrow.
- Pack profile v23 adds twelve assets and the five tables.

## Evidence

In `local/evidence/hunter-ending/`: the research (`decode/hunter.md`), two new captures from
power-on (`code-route`: the main menu's code, the ending and the reset; `cheat-route`: the code, a
reset, the title code, the code again, the CHEAT! page and a second reset), this task's copies of
`compare.py` (it restarts the boot's frame-based checks at the reset) and `sram.py`, and their logs.

| Comparison | Frames compared | Pictures equal | Differences |
| --- | --- | --- | --- |
| all-gold (R-0062's run), HUNTER's ending and the reset | 28,487 | 3,007 of 3,007 | none |
| code-route | 1,850 | 1,410 of 1,410 | none |
| cheat-route (two resets) | 2,800 | 1,060 of 1,060 | none |

The first 17 frames after each reset are not compared: the original is still clearing work RAM
there. The records are equal at every compared frame of all-gold. On the code routes `tries`
(`$77:1073`) differs: native's cold start holds 3 where the original holds 0 until the one-player
screens set it, an older difference this task leaves.

## Not recovered

- `$80:B124`, which the title code calls, is not read.
- The page loads' lengths (8 or 9 frames) and the reset's upload delay are measured, not derived.
- A pad-2 press during the first page's wait is not captured.
