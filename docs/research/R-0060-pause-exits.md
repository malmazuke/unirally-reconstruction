# R-0060 - The race's pause menu in one-player play: quit and restart

Status: recovered and implemented on `task/race-pause-exits` (RACE-PAUSE-EXITS), 26 September 2026.
PAL ROM `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, audited bsnes core
`e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b`. Follows R-0057 (the one-run
result) and R-0058 (the lap result).

The race's pause menu (`$83:CD05`, `$83:F63E-F979`) offers CONTINUE GAME and QUIT. In the original,
QUIT ends a one-player race through the menus:
- **During the start countdown** (`$11C5` not 0), it writes 0xEA62 into the player's total
  `$77:0769`: a restart. The menus fade a blank screen in and run NOW PLAYING again, with nothing
  scored.
- **After it**, it writes 0xEA61 there and zeroes the player's stunt score `$77:07BB`: a quit. The
  menus show the ordinary result, where the player's row reads QUIT, and score it as a loss
  without a time.

Native's race already had the menu itself (R-0035, R-0038: Start opens it, Up and Down choose,
Start pressed again after a release confirms). There, the second choice restarted the race in
place, which the race on its own still does. Now a race started from the menus ends for them
instead (`update_race_for_menus`), and the front end handles both totals.

## The pause menu

- **Opening.** Start on pad 1 held (`$0339`), while the player has not finished, opens it. The race,
  its clocks, the countdown and the AI wait while it is open. The screen dims to brightness 7.
- **Choosing.** Up chooses CONTINUE GAME and Down chooses QUIT; the choice follows the held
  direction. A "<" in column 24 marks it.
- **Confirming.** Start must be released (`$0EF5` = 1), then pressed again. CONTINUE GAME clears
  the menu. QUIT tests `$11C5` (`$83:F8DA-F90B`); the labels never change.
- The race ends in the confirming update: the teardown `$83:F97C` runs and `$83:C8E0` returns in
  that frame, with no finish display and no result load. So **r**, R-0057's return frame, is the
  frame of the confirming press, and its picture is black.

## The menus' side

From r the return is R-0057's, r to r + 104. Then, by the player's total:

- **Quit (0xEA61).** `$80:9A50` adds one to the opponent's stunt tally (`$77:0825`, `$0815`,
  `$0817`), words only a stunt event reads. The result is the ordinary one:
  - one run: the track's records, then MIKE and QUIT with the 1P mark;
  - a lap race: total QUIT, best lap NO TIME, and every dot below the graph.
  - Leaving it scores a loss without a time: races +1, `$77:0234` +1, `$77:0742` bit 12. No record
    is placed.
- **Restart (0xEA62), `$80:88DD`.**

| Frame | Address | What happens |
| --- | --- | --- |
| r + 104 | `$80:88F8` | The text buffer cleared (0x004C) |
| r + 105 | `$80:8903-890A` | The logo up (`$80:F53F`), the blank text sent to VRAM 0x1000 |
| r + 106 to r + 112 | `$80:9869` | The fade, brightness 2 to 14, on the checkered background |
| r + 112 | `$80:8914`, `$80:BC36-BC45` | NOW PLAYING (`$80:B18D`) |

  Nothing is scored; only `$77:0769` holds 0xEA62. The next race initializes as the first did,
  a frame sooner or later (R-0058's loading).

## The exit's frame after a placed record

Leaving a result, `$80:C786` updates the statistics and the records, and `$83:879A` scores after
`$83:A923`'s frame wait. `$80:C9D7` recomputes the checksums (`$83:90F4`) when it places a time
in the track's top three, and `$80:C786` does again at its end. With a time placed, the work runs
past its frame, and the scoring and everything after it come a frame later: PICK TRACK on the
exit's fifth frame, as R-0057 found. With none placed, as after a quit, PICK TRACK comes on the
fourth frame, and the arrow moves on the third. Native now counts that frame by whether a time
was placed (`result_scoring_frame`).

## Evidence

Captures of the original in `local/evidence/race-pause-exits/decode/` (bsnes, Strict), from power-on
with the defaults' menu presses and work RAM every frame:
- `quit`: DRAGSTER; Start 1700, Down 1760, Start 1820 (quit, r = 1820); Start 2200 on the result.
- `restart`: DRAGSTER; Start 1450 (during the countdown), Down 1500, Start 1550 (restart,
  r = 1550); Start 1800 on NOW PLAYING, the second race.
- `lapquit`: ZOOM ZOO; Right 1600-2199, Start 2200, Down 2260, Start 2320 (quit, r = 2320);
  Start 2700 on the lap result.
- The listing work is `decode/pause-exits.md`.

`compare.py` runs the native front end with the captures' pads and the native race between the
menus, and compares the menus' words, the OAM buffer and the text map on every frame, and every
picture.

| Capture | r | Frames compared | Differences | Pictures equal |
| --- | --- | --- | --- | --- |
| quit | 1820 | to 2399 | none | 184 of 184 |
| restart | 1550 | to 1807 (the second race) | none | 179 of 179 |
| lapquit | 2320 | to 2899 | none | 166 of 166 |

`sram.py`: the kept cartridge RAM words equal the original's at `quit` 2210 and 2399, `restart`
1662 and 1807, and `lapquit` 2710 and 2899.

## Not recovered

- **The pause menu's picture.** Native draws its own overlay (PAUSED, RESUME, RESTART RACE; R-0035)
  where the original prints CONTINUE GAME and QUIT in the race's text layer at brightness 7.
- **Pad 2's pause** and the other modes' pause messages (`$83:F6FD`): not one-player play.
- A quit after a lap was run, which also puts that lap into the records (`$80:C8E8`), is not
  captured.
