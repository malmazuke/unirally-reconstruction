# R-0078 - The one-player race's pause menu picture

Status: implemented on `task/classic-pause-menu` ([CLASSIC-PAUSE-MENU](../../tasks/CLASSIC-PAUSE-MENU.md)),
5 October 2026; accepted (tier 2, PR #53). PAL ROM SHA-256
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, audited bsnes core
`7d5aa1e656b9171524d01b1b22917197d8121cb4` (library `e59bf88d4fc9`), Strict serialization. Private
captures and scripts are in main `local/evidence/classic-pause-menu/`. It completes the picture
half of [R-0060](R-0060-pause-exits.md), which recovered what the menu's choices do.

Tags: **[L]** listing only, **[C]** confirmed in a capture below.

## What the original draws

The menu routine is `$83:F63E-F979`, run by `$83:CD05` on every paused update (R-0060). Its
picture, in a one-player race:

- **Brightness 7.** The NMI writes INIDISP from the fade (`$80:883F-8849`: `$0FF1` - 15, so 15
  once the fade is done). A paused update then writes 0x87 (forced blank, `$83:F695-F69C`) while
  it writes VRAM, and 0x07 before it returns (`$83:F962-F979`), all inside the vertical blank, so
  the picture of every paused update shows at brightness 7. The update that confirms CONTINUE GAME
  writes 0x0F instead (`$83:F945-F956`) and its picture is at full brightness, even during the
  fade-in; the next NMI goes back to the fade's value. [C: in `menu` the Start press on 1700 dims
  picture 1700 itself, mean level 140 to 46; the confirming press on 1920 restores picture 1920;
  in the review's `fadein`, CONTINUE GAME on 1346 with the fade at 8 shows at full brightness]
- **The words.** Fifteen BG3 map words from `$83:F616` at VRAM 0x18A8 (the map at 0x1800: row 5,
  column 8), " CONTINUE GAME " with a blank 0x79 at each end, and four from `$83:F636` at 0x18ED
  (row 7, column 13), "QUIT"; each glyph's lower half, tile + 0x10, goes 0x20 words further on
  (`$83:F794-F7FA`). The words are 0x38xx: palette 6 and the priority bit. Their tile numbers are
  the caption alphabet's (`c` 0x0D, `o` 0x29, `q` 0x2B, R-0042), so native prints them with the
  caption's font and ink. [L; C: every paused picture below]
- **The cursor.** `$83:F831-F8A7` writes tile 0x46 (the left chevron, "<") and its lower half
  0x56 in column 24 beside the chosen line, and 0x59, blank, in both halves beside the other.
  The choice follows `$0EF3` (native `pause.selection`: 1 CONTINUE GAME, 0xFFFF QUIT). [C]
- **The HUD under them.** The menu's words replace whatever the HUD had in those cells: a split
  time on rows 5-6 is hidden while the menu is open. [C: `split` 2590-2649, `+0:03:6` under it]
- **The window members over them.** The words are in BG3, so a window member covers them as it
  covers the HUD (ZOOM-ZOO-WINDOW-EFFECTS). [C: the review's `fadein` 1440, the countdown digit
  over "CON" and the "<" on the picture that opens the menu]
- **CONTINUE GAME clears its rows.** `$83:F915-F92C` writes 129 zero words from `$130F`, 0x18A8 to
  0x1928: row 5 from column 8 to row 9 column 8. Tile 0 shows nothing, so the player's centred
  cells and an up arrow's rows 5-6 stay blank until the HUD queue writes the cells again or the
  NMI redraws the arrow (`$81:E8E8-EB83`). `$0D19`, which keeps the up arrow off rows 5-6 while
  the cells hold text, is not touched. [C: `split`: the split is blank from 2650, the confirming
  press, while the queue still holds it; its blank on 2745 changes nothing on screen. The arrow
  half is the listing's: no capture pauses under an up arrow]
- A paused update that reopens and confirms at once because Start is still held (R-0060) also
  writes the menu and clears it, so its picture is at full brightness with the rows cleared. [C:
  `menu` 1920-1923, Start held after the confirming press]

Not drawn by this routine in a one-player race: pad 2's half (`$83:F6D9-F6F3`, VRAM 0x1A68 and
0x1AAD, the lower view) and the menu modes' pause messages (`$83:F6FD-F791`, eight sixteen-letter
lines at `$83:F516`, chosen by the pauser's rider). Both are split-screen behaviour; native's split
races keep M4-16's authored panel (below).

## Native

- `render_classic_race` draws a one-player race whose update was paused at brightness 7, and the
  picture of the update that closed the menu at 15 (`classic_pause_menu_closed`: the menu's count
  of updates moved on and no choice is left, which a HUNTER effect's skipped update and the
  standalone restart are not). It removes the HUD's and caption's ink from the menu's cells
  (`classic_pause_menu_cells`), draws the menu there (`draw_classic_pause_menu`) in the caption's
  font and ink, then the riders and the window members as before. Native draws the words in the
  HUD's place, under the riders; no capture shows a rider under them.
- `ClassicRaceHudClock` follows the clear: the update that closes the menu (the same test) marks the player's
  cells cleared on that picture (`player_cells_cleared`) and takes the up arrow's rows 5-6 down;
  the queue's next write of the player's cells, and the NMI's next arrow redraw, put them back.
- The second line is the original's "quit" in a race from the menus, where it ends the race as
  the original's QUIT does (R-0060). The standalone `--track` race restarts in place from the
  same choice (M4-16), so its second line reads "restart", centred under the first (columns 12-18).
  That label is authored: the original has no standalone race.
- Split-screen races (2P, VS and league pairs) keep M4-16's authored panel: PAUSED, RESUME,
  RESTART RACE over a halved picture.
- The laboratory's `front_end_runner` now writes one-player race pictures too (`--picture`), with
  the race's picture history kept only when pictures are asked for.

## Evidence

Captures of the original (`local/evidence/classic-pause-menu/`, `captures.sh` and
`captures-split.sh`), cold start with the defaults' menu presses (R-0060), DRAGSTER, a picture
and work RAM every frame:

| Capture | Presses after the menus | What it shows |
| --- | --- | --- |
| `menu` | Start 1700, Down 1760 (20 frames), Up 1800, Down 1830 (30), Up 1880, Start 1920; Start 2000 held 30 frames, Start 2060; Start 2150, Down 2180, Start 2220 | opening, the cursor both ways and held, CONTINUE GAME, Start held through the opening, a quit |
| `countdown` | Start 1450, Down 1480, Up 1500, Start 1520, Start 1540, Down 1560, Start 1580 | the menu during the start countdown, CONTINUE GAME there, then QUIT as a restart |
| `split` | Right 1750 (1,100 frames), Start 2590, Down 2610, Up 2630, Start 2650 | the menu over the split `+0:03:6`, CONTINUE GAME and the cleared cells |

`pictures.py` runs the native front end with each capture's pads, each race initialized where the
original's was (R-0060's `compare.py` rule), and compares every captured picture pixel for pixel:

| Capture | Pictures compared | Menu open | QUIT chosen | Equal |
| --- | ---: | ---: | ---: | ---: |
| `menu` (1690-2359) | 670 | 350 | 130 | 670 |
| `countdown` (1440-1719) | 280 | 110 | 40 | 280 |
| `split` (2580-2899) | 320 | 60 | 20 | 320 |
| R-0060 `quit` | 206 | 21 | 15 | 206 |
| R-0060 `restart` | 271 | 28 | 23 | 271 |
| R-0060 `lapquit` (ZOOM ZOO lap race) | 184 | 17 | 11 | 184 |
| review `fadein` (1320-1519) | 200 | | | 200 |
| review `zoomlap` (ZOOM ZOO lap race, 1340-3599) | 2,260 | | | 2,259 |

The primary's six captures: 1,931 of 1,931 pictures are equal: 586 with the menu open (239 with
QUIT chosen), the race's around them and the menus' after the quits. The reviewer's two withheld
captures (`review/`): `fadein` (DRAGSTER: the menu opened during the fade-in, CONTINUE GAME there
with Start held, the cursor across the fade's end, the menu opened under a countdown digit, QUIT)
and `zoomlap` (ZOOM ZOO lap race: nine pauses with the cursor moved down and up and CONTINUE GAME,
then QUIT). `zoomlap`'s one difference, 4 pixels on the unpaused picture 1801, is the left arrow's
chevron drawn over the rider's wheel where the original shows the wheel; main's renderer draws the
same 4 pixels (checked with the runner change alone), so it is an existing layering gap of the
arrow, not this change's. Before the change the 350 paused pictures of `menu` all differed (about 55,700
pixels each) and the unpaused 320 were already equal; before the clear was followed, `split`
differed on the 95 pictures 2650-2744, in the split's cells only. The first review found three
more: on `fadein` the window digit drawn under the words (1440, 214 pixels) and CONTINUE GAME
during the fade-in at the fade's brightness (1346-1347, every pixel); and, from the listing, a
HUNTER effect's skipped update taken for the menu closing, which blanked a split time (a native
probe; `$83:CC9A-CCA2` skips the whole update, menu included).

## Not covered

- **Split-screen pauses.** Pad 2's menu in the lower view, the menu modes' pause messages, and
  the dimming of a split race are not drawn; native's split races show the authored panel. Native
  also lets only a league pair's second pad pause (`run_pause_menu`), where the listing reads pad
  2's Start in every two-pad race (`$83:CD05`, R-0060): a state change, outside this tier-2 task.
- **The up arrow.** Its cells under the menu and its rows cleared by CONTINUE GAME follow the
  listing and a native test; no capture pauses while it shows.
- **HUNTER's skipped updates** with a split time showing are covered by a native test only. A
  pause while a HUNTER effect is skipping updates is not captured: if the original skips the menu
  on those updates too, its picture there would show at the fade's brightness, where native draws 7.
- **The left arrow over a rider** (`zoomlap` 1801): an existing gap, outside the pause.
- The `$0545` name cheat's 500-frame picture (R-0060).
