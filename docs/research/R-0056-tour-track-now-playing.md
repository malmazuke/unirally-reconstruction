# R-0056 - PICK TOUR, PICK TRACK and NOW PLAYING

Status: recovered and implemented on `task/front-end-1p-setup-2` (FRONT-END-1P-SETUP part 2),
26 September 2026. PAL ROM `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`,
audited bsnes core `e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b`. Follows
R-0055 (PICK YOUR UNI) and R-0054 (the main menu).

After PICK YOUR UNI, the one-player handler (`$80:BB9C`) runs three more screens, then the race:
- PICK TOUR: the tours in two columns of four, HUNTER below, each with its badge.
- PICK TRACK: the tour's five tracks, the medal to race for, and twelve pictures of the tour.
- NOW PLAYING: the rider against the opponent, the race's kind and track, the record, and Race
  or Exit.

Native now does the same, frame for frame (`src/core/tour_menu.cpp`, `track_menu.cpp`,
`now_playing.cpp`), to Race's fade. The app then starts the chosen race when a race scenario
exists for it.

## The handler

`$80:BBF7-BC56` after a rider is chosen:

| Step | Address | What it does |
| --- | --- | --- |
| 1 | `$80:BBF4` | The screen exit `$80:F4E9` (R-0055), frames c to c + 3 |
| 2 | `$80:BBF7` | `$80:A858` loads assets 35 and 36 (the base palette's halves, colours 0 and 0x40) on c + 4 and c + 5; `$80:A82B` sends asset 68's last 1,920 bytes (`$84:A378`, the badges' tiles the main menu overwrote) to VRAM 0x3D80 on c + 6 |
| 3 | `$80:BC07` | PICK TOUR (`$80:E550`) with `$00AC` = 2, a forward slide |
| 4 | `$80:BC12` | Y or X on PICK TOUR: back to PICK YOUR UNI at `$80:BBA3`, `$00AC` = the pad's low byte (not 2, so it slides back in); the logo stays up |
| 5 | `$80:BC1A` | PICK TRACK (`$80:E84E`) |
| 6 | `$80:BC22` | Y or X on PICK TRACK: `$80:BC03` sets `$00AC` = 1 and runs PICK TOUR again from `$80:E550`, sliding back. When the medal to race for is not the rider's best on the tour, `$83:8957` first clears the run's done tracks |
| 7 | `$80:BC45` | NOW PLAYING (`$80:B18D`) |
| 8 | `$80:BC4D` | Y or X on NOW PLAYING: `$00AC` = 2 and PICK TRACK again, sliding back |
| 9 | `$80:BC53` | Race: the fade `$80:9885`, then the race `$80:99A4` |

After a screen returns, the handler tests Y and X again on the same pad word. So back wins over
a choice, after the choice's own effects.

## The menus' latch byte $008F

`$008F` is one byte shared by every menu. The main menu, PICK TRACK and NOW PLAYING write it
whole: 1 after a move, 0 when nothing is held. PICK YOUR UNI clears it on entry and keeps Up and
Down in bits 3 and 2; PICK TOUR keeps those bits and does not clear it. Native keeps the byte's
three flags (`MenuLatches`), and the comparisons check the byte every frame.

## PICK TOUR ($80:E550)

**Set-up**, from the rider's choice at c (defaults: c = 750):
- **c + 6**: `$83:893C` rounds the track `$00CE` down to its tour's first.
- **c + 7**: `$83:94D0` sends the medal object tiles (`$07:D5D8`, 3,072 bytes) to VRAM 0x7A00,
  in a frame wait without an OAM copy.
- **c + 8, c + 9**: assets 35 and 36 again.
- **c + 9**: the text map (`$80:E730`), all in one frame:
  - the map cleared;
  - the lock check on the tour chosen last;
  - the names of the open tours (`$80:E80F` and `$80:E7FB` for levels 2 and 1, then
    `$80:E7C4`: the title and the left column);
  - a 5 x 5 badge per tour 0-7;
  - HUNTER's name (`$80:E824`) and badge from level 3.
- **c + 10**: the map goes to the hidden half, and the forward slide starts.
- **c + 11 to c + 49**: the slide. At c + 49 `$80:974B` shows the medal entries.
- **c + 50**: the medal palettes (assets 32-34 at colours 0x80, 0x90 and 0xA0, `$80:9764`),
  without an OAM copy.
- **c + 51**: the medal objects (`$80:9782`, entries 0-8), without an OAM copy.
- **c + 52**: OAM copied; the loop's head (`$80:E5B4`) aims the arrow at the tour chosen last.

**The badges** (`$83:8D8F`): picture p is 5 x 5 tiles from tile base `$83:8E26[p]`, palette
`$83:8E1C[p]`, a row every 0x32 tiles, with priority. A tour the rider's level does not open
shows picture 9, the "?" badge. The places are `$80:E82E` (HUNTER's `$80:E83E`).

**The level** (`$83:9F14`): in 1P, `$77:10D3 + rider`, 0-3. A tour needs `$80:E842[tour]`
(`00 01 00 01 00 02 00 02 03 03`): level 0 opens CRAWLER, SHUFFLER, WALKER and HOPPER. The test
`$80:E7A1` reads the table with the cursor's low four bits. Past the ten tours that gives 0xFF
or code bytes, above any level, so a move off the grid is refused.

**The medals** (`$80:974B`): entry t (tour t) at `$80:97DD[4t]`, tile 0xAC, with attribute
`$80:9801[medal]` (priority 1, palettes 0-2 for bronze, silver and gold).
- A tour without a medal is hidden by its ninth x bit (`$83:961E`), and keeps whatever attribute
  it had.
- HUNTER's is hidden below level 3.
- On a cold start every medal is 0, so none shows.

**The loop** (`$80:E5B4`, one pass a frame, pad 1 only):
1. Before the wait:
   - the arrow's targets from `$80:E708[cursor]`;
   - the arrow and its shadow mirrored for an even cursor, not for HUNTER (`$80:C1EB`);
   - the medals' tile from `$83:9B27[$0191]` (the decoration animator's step).
2. After it: OAM and the pads (`$80:D1EC`), then the decoration animator `$83:9A1E`.
3. The tests (`$80:E60A`):
   - **Left** toggles to the left column.
   - **Right** toggles to an open right-column tour.
   - **Up** and **Down** (Down counts Select) move by a row to an open tour, once a press (bits
     3 and 2).
   - **B, Start or A** chooses (`$80:E6B1`): 9 (HUNTER from the right) becomes 8; the mirroring is
     cleared; the medals hidden; `$00D0` = the tour; `$77:10D1` = the rider's medal on it.
   - **Y or X** clears the mirroring and hides the medals, and the handler goes back.

   The arrow's shadow is not hidden here (PICK YOUR UNI hides it).

**Paths the captures do not take**:
- levels 1-3 and medals;
- the pending reveal after a tour is completed (`$77:10FD`, `$80:E55B`, `$80:E588`);
- the title code (Up, Left, Up, R, A on the title, `$80:F5E3`), which sets every level to 3.
  The cold start's SRAM set-up runs after the title and clears it again, so a capture needs an
  SRAM that is already set up.

## PICK TRACK ($80:E84E)

**Set-up**, from PICK TOUR's choice at c (defaults: c = 900):
- **c** (`$80:E84F`): the medal step's latch `$77:10D2` = 0; entries 0-23 hidden; the markers'
  animation `$77:10A7` = 0.
- **c + 1** (`$80:E878`): OAM copied; the text map cleared.
- **c + 2**, with no OAM copy:
  - `$83:94FF` sends PICK TRACK's object tiles (`$07:C9D8`) to VRAM 0x7A00;
  - asset 37 goes to colour 0xB0 (the markers) and the rider's palette to 0x80;
  - the text:
    - twelve pictures of the tour (`$83:8D8F`, places `$80:EA40`);
    - the tour's name centred on row 4 (`$80:9BA0` copies it from `$83:A1B4`'s list);
    - the title and the five tracks (`$80:EA58`, each name by the text code F7);
    - in 1P, the medal line (`$80:E906`): BRONZE, SILVER or GOLD for `$77:10D1`.
- **c + 3**: the map goes to the hidden half, and the slide starts: forward, or back when
  `$00AC` = 2 (from NOW PLAYING).
- **c + 4 to c + 42**: the slide.
- **c + 42**:
  - in 1P, the done-track markers (`$80:E948`): entries 64-68, hidden for the tracks not won in
    this run;
  - the first item (`$80:E9AA`): the track chosen last, or the next one not won;
  - the arrow's targets (`$80:BA6A`);
  - the markers turn (`$80:EB23`).

**The loop** (`$80:BAA4`, the generic list menu's). The markers turn before each wait (14 steps,
`$80:EB61`). After it, OAM and the pads, then:
- **Y or X**: back.
- **B, Start or A**:
  - On a track: its number `5 * tour + item` goes to `$00CE`. `$83:9983` derives the race's
    laps, kind and place for the race's SRAM words.
  - On the medal line (1P, `$80:EA8F`): the medal to race for steps up to the rider's best on the
    tour and wraps to 0, once a press. There is no step on HUNTER or in a run already begun. A
    step prints the new word, waits a frame, and sends the map straight to the shown half.
- **Otherwise**: the step's latch is released, and Down (or Select) and Up move, once a press
  (`$008F` = 1). They wrap past either end (`$80:BAC4`).

**The way out** (`$80:E9FC`): the markers hidden; OAM copied on c + 1; the medal tiles back
(`$83:94D0`) on c + 2, without an OAM copy.

## NOW PLAYING ($80:B18D)

**Set-up**, from PICK TRACK's choice at c:

- **c + 3 and c + 4**: assets 35 and 36.
- **c + 4** (`$80:B19A`):
  - the 1P and 2P marks (entries 100-103: y and palettes) are placed;
  - the text map:
    - "NOW PLAYING" (`$80:B4F6`);
    - the race's line on row 13: "racing on", "over N laps on" (`$83:A254`'s laps) or "doing
      stunts on" (`$80:B205`), then the track's name;
    - the tour's picture at `$80:B57C`;
    - the rider's name line on row 6;
    - "VS";
    - on a race, the opponent's line on row 10; on a stunt event, the qualifying score in its
      place (`$80:B53D`, the score `$83:9EEB` by tour and best medal, printed by the text code
      FD, `$80:C456`);
    - the record line on row 21: "record:" or "hi score:", then the holder's line;
    - "Race  Exit".
- **c + 5**: the rider's palette at 0x80.
- **c + 6**, with no OAM copy:
  - the opponent's palette at 0x90 and the holder's at 0xA0;
  - the map to the hidden half;
  - the forward slide.
- **c + 7 to c + 45**: the slide. At c + 45 (`$80:B42F`):
  - the 2P mark is hidden for a computer opponent;
  - the opponent's icon is hidden on a stunt event;
  - the arrow goes to Race (`$80:B457`).

**The name line** (`$80:B57E`):
- the name, to its first blank (`$80:933C`), then a blank;
- the text code EF n (`$80:C6D5`), which puts object 104 + n one pixel up and left of the cursor;
- two blanks;
- for a real rider (below 16), the best on this track in brackets:
  - races: `m:ss.cc` by `$83:8C7B`: 6,000 hundredths a minute, from minute `5` above 0x7FFF;
    0xEA61 is "quit" and 0xEA60 "no time" (`$80:FC5C`);
  - stunt events: the score by `$83:8BE7` without its blanks.

**The opponent** (`$80:B31F`): in 1P, BRONSEN, SILVIA or GOLDWYN (0x11-0x13) for the medal to race
for. On HUNTER's races it is ANTI-UNI (`$80:B361`).

**The record holder** is `$77:0550 + track`.

**The loop** (`$80:B467`): the decoration animator before the wait (it turns the icons' tiles and
sways the 1P mark), then OAM and the pads, then:
- Right to Exit, Left to Race, Select to the other (`$80:B8F1`), once a press (`$80:B471`,
  `$80:B4D4`);
- **Y or X**: back;
- **B, Start or A**, by the arrow's target:
  - Race: the icons hidden (`$80:D420`), then the fade;
  - Exit: to the main loop, which prints the main menu and slides it back in, as after PICK YOUR
    UNI's Y (R-0055). Captured: the main menu from c + 42.

**The fade** (`$80:9885`): seven frames of brightness 13, 11, ..., 1, then forced blank in the
seventh. The race (`$80:99A4`, `$83:C8E0`) starts on that frame, and its work RAM is the race's
from then on.

## Cold-start SRAM

A cold start fails the SRAM signature check (`$80:8C4E`) and sets SRAM up. What the screens read,
as native's `cold_start_records`:
- **levels, medals and done tracks**: 0;
- **every rider's best on every track**: 9:59.99 (0xEA5F); on the stunt events (place 2), 0
  (`$83:9340`);
- **every record holder**: 0x10, SOMEONE (`$83:93D8`).

So the defaults show MIKE (9:59.99) against BRONSEN, with the record held by SOMEONE.

## The pack (profile v17)

`tools/unirally_lab/content/front_end.py` `v17_new_entries`, all raw ROM:
- **Palettes**: assets 22-26 (the computer riders' and SOMEONE's), 32-36 and 37.
- **Object tiles**: the medal and PICK TRACK sets.
- **PICK TOUR**: the text, the badge places and pictures, the levels, the arrow's targets, and
  the medal places and attributes.
- **Names**: the tour names.
- **PICK TRACK**: the layout, the text, the medal words and the marker tiles.
- **NOW PLAYING**: the race-kind words, the streams and the time words.
- **Race tables**: the laps and the qualifying scores.

The tracks' names are the race's table (`presentation.classic.track-names.v1`).

## Evidence

`local/evidence/front-end-1p-setup/`: every-frame images from 400 and per-frame work RAM.

| Capture | Frames | Inputs |
| --- | --- | --- |
| `tour-moves` | 1,200 | PICK TOUR: Right (locked), Down past the last open row, Left, Up, Select as Down, Up held, Up against the top, A with Y (back wins). Then MIKE again, SHUFFLER, and PICK TRACK |
| `tour-back` | 1,150 | Y on PICK TOUR, MARTIN chosen, X on PICK TOUR, MARTIN again, CRAWLER |
| `tour-code` | 950 | The title code (cleared by the cold start's SRAM set-up), then moves and SPRINTER's lock |
| `track-moves` | 1,450 | PICK TRACK: Down to the medal line and past it, Up past the top, A on the medal line, Select, ZOOM ZOO. NOW PLAYING: Right, Left, Select twice, Y. BOWL (a stunt event), X. SWITCHER, Right, Start: Exit to the main menu |
| `track-race` | 1,260 | SHUFFLER, Y on PICK TRACK, SHUFFLER again, FLAT FUN (track 13), Race |
| `defaults` | 1,400 | The defaults to DRAGSTER and the race |

`compare.py` compares, frame by frame:
- R-0055's words;
- `$00D0`, `$00CE`, each menu's cursor `$009B` and the latch byte `$008F`;
- `$017F` and `$018F` on NOW PLAYING;
- the OAM buffer, the text map and every picture.

Every compared word, the OAM buffer and the text map are equal on every frame, to the race's first
frame. Every picture has 0 differing pixels:

| Capture | Frames compared | Pictures |
| --- | --- | --- |
| `tour-moves` | 1,200 | 800 |
| `tour-back` | 1,150 | 750 |
| `tour-code` | 950 | 550 |
| `track-moves` | 1,450 | 1,050 |
| `track-race` | 1,008 | 608 |
| `defaults` | 1,208 | 608 |

## Not recovered

- The race after NOW PLAYING is native only where a race scenario exists (MIKE against BRONSEN,
  R-0046 and R-0050). For another rider or a stunt event the app shows a notice.
- What follows a race: FRONT-END-1P-CONTINUATION.
- The SRAM words the choices write, and the handler's `$77:1073` and `$77:0742` bit 12.
- Audio: the sounds are queued, not played.
