# R-0080 - The split race's HUD: caption places, arrows, colour math and finished views

Status: implemented on `task/split-hud-gaps` ([SPLIT-HUD-GAPS](../../tasks/SPLIT-HUD-GAPS.md)),
5 October 2026; accepted (tier 2, PR #55). PAL ROM SHA-256
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, audited bsnes core
`7d5aa1e656b9171524d01b1b22917197d8121cb4`. It closes most of the split-race picture gaps that
[R-0071](R-0071-two-player-versus.md) and [R-0079](R-0079-two-pad-pause.md) measured; the captures
are R-0079's (main `local/evidence/split-pause-menu/`, the review's four in `review/`), and the
listing reading is main `local/evidence/split-hud-gaps/listing-report.md`.

Tags: **[L]** listing only, **[C]** confirmed in a capture.

## What the original does

A split race's NMI runs `$81:D853` instead of the one-player `$81:E8C9` when `$0DE1` is set
(`$80:8803`). The tilemap is BG3's at VRAM 0x1800, row r at 0x1800 + 32r.

- **Captions.** `$81:E831-E877` writes rider 0's sixteen bytes (`$0EA7`) to rows 5-6 from column 8;
  `$81:E87C-E8C5` writes rider 1's (`$0EC7`) to rows 19-20 from column 8, all sixteen cells. The
  one-player rows 10-11 (`$81:F311`) are not used. [L; C: `twop-plain`, the upper caption]
- **Arrows.** `$82:9822` gives the arrow to whichever rider trails by a pair of transitions, in
  that rider's view, unless it has finished or its transition was rejected. The upper one is
  redrawn after an update with the progress phase set (`$81:D872`, `$0302`), the lower one every
  NMI (`$81:DB10-DDA4`). Side arrows grow from column 28 leftward or column 5 rightward on rows 6-7
  (20-21 below); the up arrow from row 2 (15 below), its middle rows skipped while that rider's
  cells show (`$0D19`, `$0D1B`); the down arrow from row 11 (25 below). The length is
  `classic_arrow_chevrons`'. [L; C for the side arrows: the lower arrows of `twop`, `zz2p`, and the
  review's `zzap`, 3,403 pictures with a side arrow at lengths 1-3 both ways, all equal; the up and
  down rows are the listing's alone]
- **Colour math.** `$2105` = 0x19 puts BG3 in front; `$2130` = 2 makes the sub screen (the objects)
  the math's second operand. Channel 5's table (`$82:D57F-D607`) gives lines 0-110 the upper
  rider's `$82:D4DC` entry and lines from 111 the lower rider's; every entry adds except TONY's
  (rider 9), whose CGADSUB has bit 7 set: where a rider is behind the ink the result is CGRAM 27
  minus the object, a dark red. [L; C: `league`, TONY in the lower view]
- **Finished views.** `$83:E8E0` and `$83:EA72` set a finished rider's view's brightness byte
  (`$7E:2065`, `$7E:2069`) to 7 in every split race, not only a league's. [L; C: `drfin`, 2P]
- **The lower clock and "finish".** Rider 1's last crossing (`$81:824B`) blanks its clock, rows
  15-16 columns 24-30 (`$81:E13B-E1C2`), and the clock is not rewritten while it is finished; the
  lower "finish" starts at column 1, as the upper one does. [L; C: `drfin`]

## Native

- `draw_classic_caption` draws rider 0's caption at row 5 in a split race; `draw_split_hud` draws
  rider 1's, untrimmed, from column 8 of row 19.
- `ClassicRaceHudClock` keeps `lower_arrow` (`classic_race_lower_arrow`), redrawn every update;
  `draw_split_arrow` draws both views' arrows at the split rows and full length.
- `RaceObjectMath` carries each view's CGADSUB; `rider_pixel` subtracts where the view's rider's
  entry says so (in one-player play too: TONY as the player, untested there). In a split race it
  reaches only the ink drawn before the riders (rider 0's caption, the pause menu): the split HUD,
  rider 1's caption among it, is still drawn over the riders as flat ink (below).
- `dim_finished_views` dims a finished view in every split race; the lower clock is blank after
  rider 1's crossing and the lower "finish" starts at column 1.

## Evidence

`classify.py` over R-0079's captures and the review's (each picture classed by where its
differing pixels lie), state words unchanged (0 differences on every race frame):

| Capture | Paused: equal | caption cells only | other | Racing: equal | caption cells | other |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `twop`, `vs` | 155 each | 0 | 0 | 322 each | 181 each | 352 each (the result's icons) |
| `twop2` | 100 | 0 | 0 | 153 | 105 | 352 (the result) |
| `vs2` | 40 | 0 | 0 | 159 | 0 | 311 (NOW PLAYING after the restart) |
| `league` | 105 | 120 | 0 | 305 | 180 | 0 |
| `league-restart` | 145 | 0 | 0 | 459 | 0 | 6 (NOW PLAYING after the restart) |
| `twop-plain` | | | | 238 | 292 | 0 |
| review `zz2p` | 170 | 0 | 0 | 698 | 149 | 263 (the result and the menus after it) |
| review `drfin` | 0 | 25 | 0 | 108 | 88 | 579 (the result; 3967-3968) |
| review `leaguefin` | 50 | 0 | 0 | 10 | 54 | 7 (8961-8963; 9050-9053) |
| review `leaguecpu` | 50 | 0 | 0 | 41 | 0 | 0 |

Before this task (R-0079): 731 paused pictures equal, 197 caption-only, 187 other; now 970, 145
and 0. The review's withheld `zzap` (2P ZOOM ZOO to both finishes, no pause, main
`local/evidence/split-hud-gaps/review/`): 5,830 race frames with 0 word differences; 4,940 of 6,074
pictures equal (main's renderer: 1,272), none worse than main's. Without a pause, `twop-plain` goes from 177 to 238 equal pictures and loses every class
but captions. The split demo's 326 retained pictures (R-0069) stay equal (`demo_recheck.py`), as
do R-0078's one-player pause captures.

## Not covered

- **The remaining caption differences**, two causes, queued as
  [SPLIT-CAPTIONS](../../tasks/SPLIT-CAPTIONS.md) (tier 1): rider 1's tutorial hints, which a human
  rider 1 gets in its own queue and native's engine queues only in the demo (race state); and the
  split chain's upload order (left fields, top clock, bottom clock, top cells, bottom cells, stunt
  scores, top caption, bottom caption), so that a caption lands a picture off.
- **Upload timing** at a finish (`drfin` 3967-3968, `leaguefin` 8961-8963) and of the centred
  cells and the lower clock (`zzap`, 15 pictures): the lower fields follow the split chain, not the
  state's own picture; **the split CONTINUE clear** (`leaguefin` 9050-9053: the lower arrow's cells
  blank on the closing pictures); **the split HUD over the riders**: drawn as flat ink where the
  original adds or subtracts the object behind it (`zzap`: rider 1's WIPEOUT, 5115-5129; drawing it
  before the riders mends 68 pictures and breaks 6 of the split demo's, so the rule for two humans
  is not recovered); and **the lower cells' sign** (`zzap` 7155-7271: "+" in the original, "-" in
  native, as on main). All with SPLIT-CAPTIONS.
- **A two-human race's end**: native ends `zzap` 241 frames after rider 0 finishes (7856), the
  original races on until rider 1 finishes (8067); queued as [SPLIT-RACE-END](../../tasks/SPLIT-RACE-END.md)
  (tier 1, as on main).
- The lower clock at a 10:00 time-out (the listing: the top shows 9:59:9, the bottom keeps its
  digits) is not captured.
- The results' icons (R-0071) and NOW PLAYING after a two-human restart (TWO-HUMAN-RESTART).
- A split race with `$77:0750` bit 3 clear outside the demo, where the lower view uses the
  player's colour math entry; the stunt score fields in a split race (rows 11-12 and 25-26).
