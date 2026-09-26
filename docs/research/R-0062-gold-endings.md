# R-0062 - The tours' gold endings and the reveal of new tours

Status: recovered and implemented on `task/front-end-endings` (FRONT-END-ENDINGS), 26 September
2026. PAL ROM `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, audited bsnes core
`e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b`. Follows R-0059 (a tour's
completion, the award, the unlock rule).

A tour's third completion raises the rider's medal to gold. Instead of the award, `$83:883E`
dispatches through `$83:88FD` to the tour's ending, and after it the unlock rule can open new
tours, which PICK TOUR reveals. Native now plays the eight tours' endings and the reveal frame for
frame. HUNTER's ending, which ends in a soft reset, is queued as HUNTER-ENDING.

## Getting there quickly

A forced completion (pad 1 holding exactly Select + X + R as a result is left, R-0059) works on the
result of a race quit through its pause menu (R-0060): Start at race + 520, Down at + 550, Start at
+ 580 (r), then Select + X + R from r + 140 for nine frames; the completion scores at r + 143. So a
tour's gold comes after about 4,700 frames from power-on, and one 42,787-frame run reaches all 25
completions of the game.

## The endings

All eight share one scheme (frames from the completion's scoring frame s):

| Frames | What happens |
| --- | --- |
| s + 1 to s + 16 | `$83:A4E9`: the award's fade out, then forced blank |
| s + 16 | `$83:A507`: the award's set-up (`$83:A614`) with another song |
| s + 89 | `$83:A575-A613` as for the award: NMI off, the object tiles and text cleared, the screen reset, every object hidden, the gold medal's colours at 0x90; then the tour's routine: mode 2, colours 0x57 at CGRAM 0, the rider's at 0x80 (CRAWLER 0xC0), the uni's (0x19) and the tour's own after them, the gold pattern's map 0x53 |
| s + 90 to s + 92 | the pattern's tiles 0x54, the bar's map 0x52 (palette 1, priority) and tiles 0x4C, its colours 0x3C, OBSEL, the tour's object tiles at 0x6000 |
| s + 92 to s + 96 | the tour's first objects and poses, then `$83:A4D2`'s fade in over 15 frames |
| then | the tour's script: a fixed number of steps, each of one or two frames, that reads no pad |
| t' + 1 on | the award's way back (`$83:A4E9`, `$83:A721`, `$83:A4D2`, the unlock rule, PICK TOUR), a frame later than after the award from the menus' screen on (PICK TOUR at t' + 133) |

Each tour's script moves its objects from a table between its code (the objects' first entries,
tiles by step, pose words, CRAWLER's flash colours) and uploads the rider's poses (`$83:8E3A`,
`$80:F814`) to object tiles 0x100, 0x108 or 0x180. The tours: CRAWLER walks past a gold nugget and a
flash of colour math (CGADSUB 0x33, COLDATA from its table) leaves a hole, whose tile is one the
menus left in VRAM; SHUFFLER, WALKER, HOPPER, JUMPER (whose gold pattern scrolls 8 pixels a step
with the slides' word `$0090`), BOUNDER and RUNNER each have their own. SPRINTER loads no object
tiles and draws with the menus'. BOUNDER and SPRINTER build a second picture (`$16F0`) for a second
rider. The script lengths: CRAWLER 226 frames, SHUFFLER 270, WALKER 285, HOPPER 407, JUMPER 298,
BOUNDER 191, RUNNER 212, SPRINTER 248.

Found against the captures, correcting the first decode:
- HOPPER clears object tiles 0x180-0x1FF (VRAM 0x7800, 0x800 words) as CRAWLER does (`$83:C126`).
- BOUNDER's red uni appears at step 0x17 (`$83:B8CB`).
- After WALKER's and JUMPER's endings NMI's hook first runs a frame later than after the others;
  why is not known.
- The menus' screen comes back on the frame the award's way back would bring it plus one, and NMI's
  hook runs in that frame already.

## The reveal

The unlock rule (`$83:8853`) also writes its level to `$77:10FD`, the pending reveal, whenever a
count matches (four tours at bronze or better, six at silver or better, or all 24 golds), even if
the level does not change. PICK TOUR's entry (`$80:E553-E560`) then draws the level below it; after
the slide (`$80:E580-E5A2`) the level is revealed and the pending word cleared, the medal tiles and
`$80:A858`'s two frames run again with the tours printed at the new level, and the text goes to the
shown half: the new tours appear in picture R + 47, and PICK TOUR takes a press at R + 51, four
frames later than without a reveal. If the tour just completed is not open at the lower level,
PICK TOUR's cursor goes to CRAWLER (the rule R-0056 found for PICK TOUR's print).

## Native

- `tour_ending.cpp`: the scheme, CRAWLER's script and each tour's layout;
  `tour_ending_scripts.cpp`: the other seven scripts. `award.cpp` shares the award's reset, fades,
  map loader and way back with them.
- `tour_menu.cpp`: the reveal; the records carry `$77:10FD`.
- Pack profile v22 adds the endings' 16 assets and eight tables.
- The front-end runner keeps a race's own frame label, which its clocks count from, and lines its
  first update up with the menus' frame; it had overwritten the label, which broke a race starting
  before its scenario's frame.

## Where the code is

The award's set-up with the endings' song is `$83:A507-A613`, and the way back `$83:A721-A90E`;
BOUNDER's and SPRINTER's second picture is sent by `$83:AB25`. The eight endings fill
`$83:B1EB-C8A9`, each routine followed by its table: SHUFFLER `$83:B1EB-B4DE` (table
`$83:B4DF-B505`), WALKER `$83:B506-B78F` (`$83:B790-B79B`), the walk's poses `$83:B79C-B7D1`,
BOUNDER `$83:B7D2-BB43` (`$83:BB44-BB7F`), JUMPER `$83:BB80-BE99` (`$83:BE9A-BEB9`) and its scroll
`$83:BEBA-BECF`, SPRINTER `$83:BED0-C0F5` (`$83:C0F6-C11D`), HOPPER `$83:C11E-C457`
(`$83:C458-C49B`), CRAWLER `$83:C49C-C6BF` (`$83:C6C0-C714`), RUNNER `$83:C715-C899`
(`$83:C89A-C8A9`). PICK TOUR's reveal is `$80:E553-E5A2`.

## Evidence

Captures from power-on in `local/evidence/front-end-endings/decode/` (pictures, work RAM every
frame, per-frame register writes), compared by `compare.py` (the menus' words, the OAM buffer and
the text map every frame, and the pictures) and `sram.py` (the records):

| Capture | What it holds | State | Pictures |
| --- | --- | --- | --- |
| crawler-gold | three completions on CRAWLER, its ending, PICK TOUR | no difference on 3,784 frames | 830 of 830 (every frame from 4670) |
| shuffler-gold, walker-gold, hopper-gold | the same for those tours | no difference | 1,030 of 1,030 each |
| reveal | a bronze on each of the four first tours, the level-1 reveal | no difference | 600 of 600; records equal at 6353, 6400, 6799 |
| locked-gold | the reveal, JUMPER's and BOUNDER's golds | no difference on 10,580 compared frames (a 16,300-frame run; the races' frames are the race engine's) | 910 of 910, 770 of 770 |
| all-gold, to frame 39700 | 24 completions: levels 2 and 3 with their reveals, RUNNER's and SPRINTER's golds, the first four tours' golds at level 2 | no difference on 25,400 compared frames | RUNNER 720 of 720, SPRINTER 790 of 790, the reveals 81 and 181 |

A native test completes each tour at gold and checks each ending's length, the frame NMI's hook
comes back and PICK TOUR's return, without the ROM. The records at more frames are in the task
record's gates.

## Not recovered

- HUNTER's ending (`$83:AB9A`): newspaper pictures and the credits, each until a press, then a soft
  reset (HUNTER-ENDING).
- A completion by a fifth win (FIFTH-WIN-COMPLETION): every capture uses forced completions.
- A completion on a tour already at gold replays its ending without a store [listing only].
- The endings' counters `$77:10C9`, `$10CB` and `$10A7` stay in cartridge RAM with values the
  records do not keep.
- Why NMI's hook runs a frame later after WALKER's and JUMPER's endings.
- Not exercised by any capture or test: a reveal on PICK TOUR's forward entry (from PICK YOUR UNI),
  which needs preloaded records; another rider's colours in an ending (every capture rides MIKE).
