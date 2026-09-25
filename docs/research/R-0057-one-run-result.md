# R-0057 - After a one-run race: the result screen, the records and PICK TRACK again

Status: recovered and implemented on `task/front-end-1p-continuation` (FRONT-END-1P-CONTINUATION),
26 September 2026. PAL ROM `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`,
audited bsnes core `e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b`. Follows
R-0056 (PICK TOUR, PICK TRACK and NOW PLAYING).

After a one-player race the original returns from the race (`$83:C8E0`) into the menus' code:
- it reloads the menu machine as at power-on;
- it builds the result screen, fades it in and waits for a press;
- it updates the statistics and the records, scores the race, and runs PICK TRACK again.

Native now does the same for a one-run race (race mode `$77:074B` = 0), frame for frame:
`src/core/race_result.cpp`, on the frame the native race's result load begins. In the app, a
one-run race chosen on NOW PLAYING comes back to the menus. A lap race still ends on the race's
own result screen, and a stunt event is not native (see "Not recovered").

## The race's return

The race's last frame is r: the frame its result load begins (R-0049). Every frame below is
counted from r, and both captures (a win and a loss) agree on every one.

| Frame | Address | What happens |
| --- | --- | --- |
| r | `$80:A09A` | The early loads as at power-on (R-0054): forced blank, NMI off, the OAM buffer reset, asset 5 at CGRAM 0xE0, asset 88 at VRAM 0x7000, the early registers |
| r + 1 to r + 73 | | The sound program's upload; no frame is processed |
| r + 74 | | The upload's last frame: the arrow's spin steps |
| r + 75 to r + 100 | `$80:D20E` | The main menu's screen, as at boot frames 377-403; the objects' layout `$80:D2C1` in r + 100 |
| r + 101 | `$80:D372`, `$80:D377`, `$83:987D`, `$80:98A4` | The OAM copy and NMI on; the menus' words restored; the arrow's targets off the left edge (0xFD00, 0x0700) |
| r + 102 | `$80:A82B` | Asset 68's last 1,920 bytes to VRAM 0x3D80 |
| r + 103, r + 104 | `$80:A858` | Assets 35 and 36 at CGRAM 0 and 0x40; `$83:89BA` resets the text halves and BG2's scroll; `$80:9AAC` puts back `$77:0742` as it was before the race (the logo held up) |

- **What is restored.** `$83:9894` (`$80:9A27`) saves work RAM `$0000-$019D` to `$77:0E6B`
  before the race, and `$83:987D` copies it back. Native keeps the same words (`SavedMenus`):
  - the menus' state, the palette cycle's counters, the logo's offset;
  - the slide, the decorations, the latches;
  - the four one-player screens, the text printer, the arrow's spin.
- **The arrow in r + 101.** The arrow is drawn before the restore, with the spin the race left.
  This spin is the saved one, stepped once at r + 74 and once at r + 75.
- **The text buffer.** `$0200-$09FF` holds the race's work RAM until the result screen clears it.
  The comparisons skip it on these frames; it is not on the screen (forced blank).

## The result screen ($80:951C, type 0 $80:CE90)

Frames from the first (r + 105):

1. **Build.** `$83:94D0` sends the object tiles to VRAM 0x7A00 in a frame wait without an OAM copy.
   Then:
   - CGRAM: the rider's colours at 0x80, the opponent's at 0x90;
   - OAM entries 30 and 31: tile 0xC0, attributes 0x21 and 0x23, high byte `$0C07` = 0xF5;
   - the logo raised (`$80:F53F`: `$00AD` and BG1VOFS 0x52) and the text cleared;
   - the row icons, entries 104-108 at x 0x78, y 0x57 to 0xB7 by 0x18 (`$0C1A` = 0, `$0C1B` =
     0x54); the new-best markers, entries 96-99 at x 0xDB, hidden (`$0C18` = 0x55).
   - The row table: the player, the opponent, the track's three records, three blanks (time
     0xEA62). `$80:F082` sorts it: for k = 7 down to 0 it swaps the *last* largest time among slots
     0 to k into slot k. A computer opponent (`$017F` >= 0x10) gets 0xEA62, so it has no row.
2. **Rows** (after `$80:CF56`'s frame wait). The kind table `$80:CF91` prints the first five
   sorted rows (`$80:CF5B-D0E6`): each row's rider or holder and time go into `$00B2 + 4n`, and
   its rider's colours to 0x80 + 16n (`$80:D163`):
   - The player's row: `$80:CFB9` writes a time below the rider's best on the track as the new
     best (`$77:0829 + 2 x (50 x rider + track)`), and shows the new-best markers (`$0C18 &=
     0xEE`). The 1P marker, entries 100 and 102, moves to the row: y = 24n + 0x58, attributes 0x11
     | 2n.
   - A record row prints its holder and time.
   - Then the stream `$80:D187`:
     - the track's name in capitals on row 2 (`F7 CE 00 FC 02 EE`), and COMPLETE on row 5;
     - `player time`;
     - four rows of `F8` (a rider's name, `$80:C5D3`) and `F1` (a time, `$80:C6BB`).
   - A computer opponent then hides the 2P marker (`$0C1B` = 0x55, entries 101 and 103 at y 0xEF,
     `$80:D0FE`). `$0C19` = 0.
3. **Icons** (after `$80:D110`'s frame wait, `$80:D113-D13C`): assets 0x22, 0x21 and 0x20 at
   CGRAM 0xD0, 0xE0, 0xF0. Entries 112-114 come from `$80:D17B` (`$0C1C` = 0x40). `$80:F88D`
   writes the session slot. Then the common tail (`$80:9579-958F`): the text goes to VRAM
   0x1000, `$0089`, `$008B` and `$0192` are cleared, and the decorations step once
   (`$83:9A1E`).
4. **Frames 4-10**: the fade, brightness 2, 4, ..., 14 (`$80:9869`).

Then two waits:
- **Release** (`$80:C24C`): each frame waits, copies the OAM and steps the decorations, until
  both pads read 0. A button held from the race holds the result on screen. The last frame sets
  `$0089` = 3 (`$80:98B3`).
- **Press** (`$80:C206`): any button on either pad, seen on two frames running. A one-frame tap
  does not leave.

The printer gains three codes for the stream:
- **F8** prints a rider's name: the 16-byte record of the rider in the next direct-page word, to
  its 0xFF.
- **F1** prints the next word as a race time (`_m:ss.cc`, `$83:8C7B`; `_no_time` and `_quit___`
  for 0xEA60 and 0xEA61).
- **EE** after F7 or F8 prints the name in capitals: a-z less 0x20, digits plus 0xE6 (R-0053's
  upper case).

The pack's profile v18 (314 rules entries) adds `front-end.result-text` (`$80:D187`, 84 bytes)
and `front-end.result-icons` (`$80:D17B`, 12 bytes).

## Leaving the result ($80:BC68-BC7E)

From the press's second frame, q:

| Frame | Address | What happens |
| --- | --- | --- |
| q | `$80:C236-C245`, `$80:9805` | Entries 30-35 hidden; entries 0-29 parked at (1, 1) and hidden, unless `$77:0742` bit 8 is already set (it is then set) |
| q + 1 | `$80:F4B8` | Entries 96-115 and 120-123 hidden, the OAM copy; the object palette (`$83:91F7`, the rider's colours at 0xF0), asset 28 at 0xD0, OBSEL 0x63. Then `$80:C786` (the statistics and the records) and `$77:1073` = 3 |
| q + 2, q + 3 | `$83:879A` | Its frame waits (`$83:A923`) leave the arrow alone. Then the scoring |
| q + 4, q + 5 | `$80:A858` | Assets 35 and 36; then PICK TRACK (`$80:E84E`) with `$00AC` as restored |

The press at 3800 gives PICK TRACK's tiles at 3808 and its text at 3809, as native. There is no
fade: PICK TRACK slides in over the result at brightness 14.

- **Statistics** (`$80:C786`, one-run path `$80:C7C4`). Each rider has four words at `$77:0230 +
  8 x rider`: races, wins, losses without a time, stunt points. There are 16 riders:
  `$77:02B0` is these words' checksum (`$83:90F4`).
  - The player: races +1 (`$80:CA74`); a win or a tie adds a win and `$77:10A9` +1 (`$80:CA83`).
  - A rider opponent (under 16) counts the same way (`$80:CAAA`, `$80:CAC5` with `$77:10AB`). A
    computer opponent keeps no counts and no records.
- **Records.** `$80:C9D7` inserts a time into the track's top three:
  - times at `$77:0422`, `0486` and `04EA` + 2 x track;
  - holders at `$77:0550`, `0582` and `05B4` + track.
  - The player's time goes in after a win or a tie (`$80:C81C`; a tie is `$80:C7EE`). After a
    loss it goes in unless it is no time (0xEA60), which counts a loss without a time instead
    (`$80:C800`, `$80:CA9B`).
  - A rider opponent's time goes in the same way (`$80:C82F` after a win, `$80:C850` after a loss
    or a tie).
- **Scoring** (`$83:879A`). A win needs the player's total strictly below the opponent's: a tie
  is a loss.
  - A win marks the track done (`$77:1075 + track`, the track's low six bits, `$83:9EC8`). The tour's fifth done track completes it (not
    recovered; native refuses it).
  - A loss sets `$77:0742` bit 12. PICK TRACK's exit (`$80:EA14`) clears it and takes one from
    `$77:1073`.
  - Nothing reads `$77:1073`, and `$83:87E9`, a jump to the main loop, has no caller. **There is
    no game over.**
- **PICK TRACK again.** Its entry sets `$77:1073` = 3 again (`$80:E857`). Its cursor search
  starts from the raced track and skips done ones. So
  after a win the cursor is on the next track, and after a loss it stays.

## One player

`$77:10AD` is the menus' mode, which `$83:91F7` tests for one-player play (1) to choose the
object palette. PICK YOUR UNI's choice sets it to 1 (`$80:BBEE`), and its Y clears it
(`$80:BC9B`), as the main menu does (`$80:AD18`). The other modes store 2 to 5 (`$80:BD1F`,
`$80:BE3F`, `$80:BF9F`, `$80:99B2`); native keeps only whether it is 1.

## A new run

Choosing a rider on PICK YOUR UNI starts a new run (`$80:BBC3-BBE5`):
- the opponent `$017F` = 0x10;
- `$77:0742` bit 3 is cleared;
- all fifty done-track bytes `$77:1075-10A6` are cleared, and `$77:1073` = 3 (`$80:BBD6-BBE5`).

So after backing out to PICK YOUR UNI and choosing again, PICK TRACK starts on the tour's first
track. Native does the same (the review found it missing).

## The cold start's medals

The format (`$80:8C4E`) clears the 160 medal bytes (`$83:9454`), then sets HUNTER's 16 to 2
(`$83:9464`); its record times are R-0056's (`$83:936E`). Native's cold start did not; nothing
on the recovered screens reads them.

## Evidence

Laboratory captures in `local/evidence/front-end-1p-continuation/` (bsnes, Strict), from power-on
with the defaults (MIKE, CRAWLER, DRAGSTER):
- `cont-win`: Right held 1500-3299, Up 2200-2259. MIKE 0:33.57 against BRONSEN 0:33.58: a win
  by one hundredth. Start at 3800 leaves the result. Then ZOOM ZOO is chosen (Down, Up, A),
  PICK TOUR again (Y, Y), CRAWLER, ZOOM ZOO and Race (4800).
- `cont-loss`: Right held 1500-1999 and 3000-4399, so the result waits for the release. MIKE
  0:47.18: a loss. Start at 5000; DRAGSTER again at 5200; Race at 5350.
- Both have work RAM `$0000-$1FFF` every frame. Pictures run from 3700 and 4300 to the end, and
  `cont-win-early` and `cont-loss-early` add 3440-3709 and 4120-4409.

`compare.py` runs the native front end with the capture's pads between the menus and the native
race (`front_end_runner`). The race starts at the fade's end + 121 frames, DRAGSTER's loading on
this path; the runner times no other track. The comparison checks every frame from r + 101 on:
the arrow, the palette cycle, the logo, the slides, the decorations, the menus' words, the OAM
buffer and the text map.

| Capture | Race returns (r) | Result | Exit | Frames compared | Differences | Pictures equal |
| --- | --- | --- | --- | --- | --- | --- |
| cont-win | 3454 | 3558 | 3801 | to 4807 (the next race) | none | 1,108 of 1,108 |
| cont-loss | 4135 | 4239 | 5001 | to 5357 (the next race) | none | 1,058 of 1,058 |
| cont-win-early | 3454 | 3558 | - | to 3709 | none | 256 of 256 |
| cont-loss-early | 4135 | 4239 | - | to 4409 | none | 275 of 275 |

The native race returns on the original's frame: the native DRAGSTER race's result load begins
at 3454 and 4135, as the captures' `$80:A09A` does.

**Records.** `sram.py` replays a capture in the core (original side only) and checks its work
RAM against the capture's series every frame. It then compares cartridge RAM after chosen frames
with the runner's records (`--records`), written at their `$77` addresses:
- the statistics, the records and holders, the medals, the bests;
- `$0742` bit 12, `$1073`, the done tracks, `$10A9` and `$10AB`, the tour levels.

All equal at `cont-win` 3808 and 4700, and at `cont-loss` 5008, 5210 (after PICK TRACK's exit:
bit 12 clear, `$1073` = 2) and 5356. Native keeps no other cartridge RAM word:
- the checksums (`$83:90F4`);
- `$80:F88D`'s session slots at `$77:0618`;
- the race's own words `$0744-$07D7`;
- `$0742`'s other bits: bit 8 is `$80:9805`'s guard, and the objects it parks agree;
- PICK TRACK's marker step `$10A7` (in the menus' state, compared as their OAM).

**The app.** The hidden app with `cont-win`'s pads for the menus and Right held in the race
reports the race returned at front-end frame 3455, totals 3357/3358, and ZOOM ZOO chosen at
front-end frame 4808, as the runner's.

## Not recovered

- **The lap result** (`$80:8D6E-910E`): the lap graph, the best lap as the personal best, and
  `$80:98B3`'s graph animation, which has no release wait. A lap race in the app ends on the
  race's own result screen.
- **The stunt result** (`$80:F0EE-F2E9`) and the stunt events themselves (STUNT-EVENTS).
- **The tour's completion**: the medal award (`$83:AEF6`), the endings (`$83:88FD`), the unlock
  levels (`$77:10D3`, `$77:10FD`) and PICK TOUR's reveal, and the forced completion (pad 1 exactly
  Select + X + R on `$83:879A`'s frame, which native scores as an ordinary race without a
  word). Native refuses a fifth done track.
- **Quit and restart** (0xEA61, 0xEA62 from the race's pause menu): `$80:9A50`'s quit scoring
  and `$80:88DD`'s restart to NOW PLAYING. The native race's pause menu restarts the race
  itself.
- **A human opponent's rows** (kind 2, `$80:D007`, the 2P marker) do not occur in one-player
  play.
