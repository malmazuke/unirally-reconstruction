# R-0058 - After a lap race: the lap result and its graph

Status: recovered and implemented on `task/front-end-lap-result` (FRONT-END-LAP-RESULT),
26 September 2026. PAL ROM `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`,
audited bsnes core `e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b`. Follows
R-0057 (the one-run result).

A lap race (race mode `$77:074B` = 1) returns to the menus exactly as a one-run race does, and
leaves its result the same way. What differs is the result screen:
- a graph of every lap's time, drawn by twenty dots that fly into place;
- the best laps, which become the personal bests and the track's records;
- a press on any frame leaves it: there is no release wait.

Native now does the same (`src/core/lap_result.cpp`), so every race of a tour but the stunt event
comes back to PICK TRACK in the app.

## Frames

Frames count from r, the race's last frame; the return, r to r + 104, is R-0057's.

| Frame | Address | What happens |
| --- | --- | --- |
| r + 105 | `$83:94D0`, `$80:951C`, `$80:8D6E-8FF8` | The common part (R-0057), then the graph's build: OBSEL 0x03, the scale, the dots, the record line, the holder's colours |
| r + 106 | `$80:8FFD-910E` | Asset 0x22 at CGRAM 0xB0, entry 112; the best laps and personal bests; the streams. No OAM copy, and the work runs past the frame's end |
| r + 107 | `$80:9579-958F` | The tail, without a frame wait before it: the text to VRAM 0x1000, `$0089`, `$008B` and `$0192` cleared, the decorations step once |
| r + 108 to r + 114 | `$80:9869` | The fade, brightness 2 to 14 |
| r + 114 | `$80:98B3` | `$0089` = 3 and the graph's first pass, in the fade's last frame |
| r + 115 on | `$80:98C9-997B`, `$80:B6D3` | Each frame: the OAM copy and the pads, the exit test, then a pass |

- **Why r + 107 has no wait.** r + 106's work overruns its frame, so the tail starts inside
  r + 107 and the fade's first frame wait ends it. The arrow, which steps in each frame wait,
  misses a step there. The captures show it: the spin stays put at r + 107.
- **The exit** (`$80:B6D3`): any of pad 1's twelve buttons (`& 0xFFF0`), on one frame. Pad 2 is
  ignored in one-player play, where `$77:0742` bit 10 is set. A button still held from the race
  therefore ends the result at r + 115.
- **From the press, q.** `$80:9805` parks entries 0-29 and sets `$77:0742` bit 8 (the graph loop
  clears it on every pass), and `$80:F4B8` hides entries 96-115 and 120-123. The decorations do
  not step on q. From q + 1 the way out is R-0057's, one frame earlier than a one-run race's
  because there is no second press frame.

## The build (r + 105)

- **High table** (`$80:8D70-8D9A`): `$0C18` = 0xFF (the markers large and pushed off), `$0C19` =
  0x55, entries 0-19 shown and small.
- **The scale** (`$83:904A`). The floor is the fastest of the ten lap slots of both riders and the
  track's record (`$77:0422 + 2 x track`), ignoring 0xEA60; the top is the slowest. If they are
  less than 200 hundredths apart, the floor becomes top - 200. The graph's range is top - floor.
- **A time's line** (`$80:8E0D-8E2F`, the division `$83:8D5D`): 0xB8 - 128 x (time - floor) /
  range, in eight bits; the opponent's dots use 0xB9, a line lower.
- **The dots** (`$80:8DD9-8EC4`): lap k's dot starts at (0x80, 0x6E) and flies to column 0x2B +
  16(k + 1), at its lap's line. A lap slot not run (0xEA60) keeps line 0xF0, off the screen.
  - The player's dots are entries 0-9: tile 0x08, attributes 0x11.
  - The opponent's are 10-19: attributes 0x13, tile 0x0A for a computer opponent.
- **The record line** (`$80:8F4C-8FA6`): entries 20-29, tile 0x8E, attributes 0x15, at columns
  0x27 + 16(k + 1) and the record's line. The graph never moves them.
- **The markers**: entries 96-99 at x 0xE5, y 0x16 (the player's) and 0x26 (the opponent's),
  attributes 0x17; `$83:9A1E` animates their tiles.
- **The holder** (`$80:8FA8-8FF8`): the record holder's colours at CGRAM 0xA0.
  - With a record, the stream `$80:918F` prints the holder, the icon (entry 106) and the time,
    and the line and icon show (`$0C1A` = 0x6A, `$0C1C` = 0x56, `$0C05-0C07`).
  - Without one, `$0C1A` = 0x5A: entries 106 and 107 hidden.
  - `$80:918F` reads the holder through a word whose high byte is the next track's holder;
    `$80:9B2F` masks it.

## The streams and bests (r + 106)

- Entry 112 at (0xE7, 0xAB), attributes 0x17, beside the record; asset 0x22 at 0xB0.
- **The player's best lap** (`$80:9017-905A`) is the smallest of the ten slots. Below the rider's
  best on the track (`$77:0829`), it becomes the new best and the player's markers show
  (`$0C18 &= 0xEE`).
- **The streams**, through the text printer (R-0053):
  - `$80:910F`: player, total and best lap; the graph's axes; its top and floor times, cut to
    whole seconds by the axis glyphs printed over them; TIME down the side; "laps on" and the
    track's name.
  - `$80:91A1` and `$80:91B9`: each rider's name, icon (entries 104, 105), total and best lap.
- **A computer opponent's best lap** (`$80:90AF-90F2`) goes to `$77:0829 + 100 x opponent + 2 x
  track`, past the 16 riders' table.
  - For opponents 0x11-0x13 that address lies in `$77:0E6B-1008`, the pre-race save of work RAM
    (`$83:9894`). The save is taken again before the next race, so the write does nothing later.
  - The opponent's markers show when its best lap is under the saved word there (`$0C18 &=
    0xBB`). For BRONSEN on CRAWLER's five tracks those are the words `$0062-$006B` that the
    menus leave: 0, 0x1300, 0, 0x9E00, 0, the same in every capture. On ZOOM ZOO (0x1300,
    0:48.64) all four markers show.
  - Native keeps those five words, and takes 0 (no markers) elsewhere, which is not recovered.

## The graph ($80:98B3)

Each pass (`$80:98C9-997B`) steps the decorations (`$83:9A1E`), then for each of the twenty
dots:
- it writes the dot's OAM position from its position before the step (12.4 pixels, over 16);
- it adds the velocity to the position;
- it moves the velocity two towards the desired velocity. That is the distance to the target
  over 8, arithmetically, clamped to 40 either way (`$80:9980`).

The velocities stay even, so a dot can settle a pixel short of its target (x 0x3A for 0x3B).
In `lap-won` the dots move from r + 116 and settle by r + 203.

## The records ($80:C868)

The statistics, the records and the scoring are R-0057's, with one difference: the values put
into the records are the best laps, not the totals. Each is the smallest lap slot that is not 0,
from 0xEA62. Winning is still decided on the totals (`$83:88D3`).
- After a win (`$80:C8BA`), the player's best lap goes in (`$80:C902`), and a rider opponent's
  unless it is no time (`$80:C913`).
- After a loss (`$80:C8C5`), a rider opponent's goes in (`$80:C932`), and the player's unless it
  is no time (`$80:C8E8`).
- A tie (`$80:C8D0`) puts both in.

## The pack (profile v19)

Profile v19 (318 rules entries) adds the four streams: `front-end.lap-result-text` (`$80:910F`,
128 bytes), `front-end.lap-result-record` (`$80:918F`, 18), `front-end.lap-result-player`
(`$80:91A1`, 24) and `front-end.lap-result-opponent` (`$80:91B9`, 24).

## The race's loading

The laboratory's runner starts a native race after NOW PLAYING's fade, when the track's loading
ends: DRAGSTER's 121 frames and ZOOM ZOO's 169 on the cold start's path. The original's loading
varies by a frame with the sound program's handshake (R-0039): `lap-record`'s second ZOOM ZOO
race initialized after 168. `compare.py` therefore reads each race's initialization from the
capture (`$0FF1`'s first step with the countdown `$11C5` at 270) and passes it to the runner
(`--race-initialization`). The app labels the front end's frames with the measured loading.

## Evidence

Captures in `local/evidence/front-end-lap-result/` (bsnes, Strict), from power-on with ZOOM ZOO
chosen on PICK TRACK:
- `lap-won`: M4-16 boundary-a's driving. MIKE 1:38.02 against BRONSEN 1:38.10, best laps 0:32.50
  each. Start at 7600 on the graph. Pictures 6700-8399.
- `lap-lost`: M4-16 idle late-start-a's driving. MIKE 1:56.70 against 1:38.10, but with the
  better best lap, 0:31.65. Start at 8600. Pictures 7700-9399.
- `lap-record`: `lap-won`, then ZOOM ZOO again, with the same driving shifted to the second race.
  MIKE 1:37.86, and the result shows MIKE's record line. Pictures every tenth frame from 7600;
  `lap-record-frames` adds every frame of 13440-13839 and 14290-14419.
- All have work RAM `$0000-$1FFF` every frame. The listing work is `decode/lap-result.md`, which
  checks the graph rule against all 761 of `lap-won`'s passes.

`compare.py` runs the native front end with the capture's pads and the native race between the
menus. It checks the arrow, the palette cycle, the logo, the slides, the decorations, the
menus' words, the OAM buffer and the text map on every frame from r + 101 on, and every picture.

| Capture | Races return (r) | Frames compared | Differences | Pictures equal |
| --- | --- | --- | --- | --- |
| lap-won | 6725 | to 8399 | none | 1,675 of 1,675 |
| lap-lost | 7659 | to 9399 | none | 1,700 of 1,700 |
| lap-record | 6725, 13455 | to 15099 | none | 199 of 199 |
| lap-record-frames | 6725, 13455 | to 14419 | none | 530 of 530 (every frame of the second result's build, fade, graph and exit) |

`sram.py` compares the records native keeps with the original's cartridge RAM. They are equal at
`lap-won` 7610 and 8399 and `lap-lost` 8610 and 9399, including the best-lap record and the
personal best. Not kept: the opponent's write into the save (`$77:0ECF` here), the session
slots and the checksums (R-0057).

## Not recovered

- The opponent's saved word for any opponent or track but BRONSEN's on CRAWLER (no markers).
- `$77:0749` = 0xFF, a path one-player play does not take.
- The stunt events' result (`$80:F0EE-F2E9`, STUNT-EVENTS).
- Quit and restart (0xEA61, 0xEA62). The native race's pause menu restarts the race itself, so
  neither reaches the menus.
