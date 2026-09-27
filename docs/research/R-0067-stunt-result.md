# R-0067 - A stunt event's result, its records and the way back to PICK TRACK

Status: recovered and implemented on `stunt-result-work` (STUNT-RESULT), 27 September 2026, on
`task/stunt-event-race` `5dd973f`. PAL ROM
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, the pinned bsnes core.
Follows R-0066 (the stunt event in the race engine), R-0057 (the one-run result, whose return,
waits and exit the stunt result shares), R-0060 (the pause exits) and R-0065 (a completion by a
fifth win). Builds on the STUNT-EVENTS decode (`local/evidence/stunt-events/decode/stunt-events.md`,
section 2), which this record supersedes where they differ.

Evidence tags: **[C]** seen in a capture (work RAM or cartridge RAM every frame, pictures, or the
program counters of `coverage capture` and `capture.py --trace-frames`), **[L]** read from the
static listing (`artifacts/static-map/bank-8x.lst`) or disassembled where it has a gap, not seen
executing. Every rule below is also confirmed by the native comparison: from power-on to the end of
each capture below, no state difference and every picture equal.

## Summary

After a stunt event (race mode 2) the race returns to the menus as after a one-run race (R-0057's
r to r + 104). At r + 105 the result screen's builder dispatches by race mode (`$80:955E-9572`
through `$80:95A5`) to `$80:F0EE`: a table of the player's tricks by family and count, counted up
column by column (`$80:F669`), then the record holder, the rider's score, the rider's best and
QUALIFY. A press leaves it as it leaves a one-run result; the statistics, the track's top three and
the win test are the stunt event's own. Native now does all of this (`src/core/stunt_result.cpp`,
`race_result.cpp`), and the app plays a stunt event through instead of showing its notice.

## The result's frames

From the result's first frame (r + 105) [C]:

| Frame | bowl-lose | hill-win | bowl-quit | What |
| --- | --- | --- | --- | --- |
| 1 | 4149 | 4216 | 2225 | `$83:94D0` (the object tiles), `$80:951C-955A` as every result (R-0057), then asset 0x22 at colour 0xB0 (`$80:F0F3`) |
| 2 | 4150 | 4217 | 2226 | the heads, objects and streams (below); the printing runs past the frame's end |
| 3 | 4151 | 4218 | 2227 | no frame wait: the printing ends, the text to VRAM, the decorations restarted (`$80:F1F4-F202`); NMI does not move the arrow |
| 4-10 | 4152-4158 | 4219-4225 | 2228-2234 | `$80:9869`: brightness 2 to 14; in frame 10 `$80:F209` shows the 1P mark and the tally's first pass runs |
| tally | 4158-4326 | 4225-4385 | 2234-2322 | `$80:F669-F7CB` |
| t + 1 | 4327 | 4386 | 2323 | the total again, the rider's best (`$80:F222-F244`) |
| t + 2 | 4328 | 4387 | 2324 | `qualify   :` and the qualifying score (`$80:F24F-F272`) |
| t + 3 | 4329 | 4388 | 2325 | `$80:F88D`: the score history and checksums |
| from t + 4 | 4330 | 4389 | 2326 | `$80:C24C` and `$80:C206`, the one-run result's waits (R-0057) |

The stream printing of frame 2 ends in the third frame on all three captures, at a point that
depends on the names printed: bowl-lose's `$009F` reads 0x23E at the end of 4150 (the 1P dashes
printed, the rider's line not), hill-win's 0x235 at the end of 4217 (inside the dashes). Native
prints everything in frame 2; the picture is black (forced blank) until frame 4, and the comparison
does not compare the text map of frame 2, nor of frame 0 (r + 104, the race's work RAM still in the
buffer, as for the one-run result).

## The screen (`$80:F0EE-F2E9`)

- Colours [L; C pictures]: asset 0x22 at 0xB0; the rider's head (asset 0x26 + `$017D`) at 0 and the
  opponent's (0x26 + `$017F`) at 0x10 (`$80:F0FC-F112`); the record holder's (`$77:0550 + track`,
  asset + 6) at 0xA0 (`$80:F1A8-F1B9`).
- Objects `$80:F116-F19C` [L; C OAM]: entries 96-99 at x 0xC4, y 0xB0, 0xC0, 0xB0, 0xC0, attributes
  0x17, hidden (`$0C18` = 0x55); entries 100-103 at y 0xAF, 0xC0, 0xAF, 0xC0, attributes 0x11, 0x13,
  0x11, 0x13, hidden (`$0C19` = 0x55); entries 104-106 at x 0x7F, y 0xAF, 0xBF, 0x9F (`$0C1A` =
  0x44: 104 and 106 shown); entry 112 at (0xC4, 0xA1), attributes 0x17 (`$0C1C` = 0x54). Their tiles
  and the columns of 100-103 are the decoration animator's (`$83:9A1E`). `$80:F209` shows 100 and
  102 (`$0C19` &= 0xCC).
- Streams, printed with the printer's attribute as the menus left it (palette 7) [L; C text map]:
  `$80:F2EA` (the track's name in capitals, a rule, `x1`-`x4`, `1p 2p`, the rows `roll:` to `mega:`
  with a 0 in every cell), `$80:F471` (the holder's name and the record, `$77:0422 + 2 * track`, at
  row 20), and in one-player play (`$77:10AD` = 1) `$80:F3E0` (dashes over the 2P cells) and
  `$80:F491` (the rider's name and `:    0` at row 22, palette 7). The nine streams from `$80:F2EA` to
  `$80:F4B4` are the pack's `front-end.stunt-result-text` (profile v25): the table, the dashes, the
  totals at rows 22 and 24 (`$80:F44F`, `$80:F456`), the qualifying line (`$80:F45D`), the record
  line, the riders' lines (`$80:F47F`, `$80:F491`, `$80:F4A3`).
- The printer's F0 (`$80:C474`) [L; C text map]: the word as five digits (`$83:8BE7`), printed from
  the fourth: the last two, the tens a blank below 10. The tally's cells `$80:F7E7-F7FA` are five
  `F0 w FF` streams, w = `$00B2` to `$00BA` (the pack's `front-end.stunt-tally-cells`).

## The tally (`$80:F669-F813`)

Column by column, x1 to x4 (`$0076` 3 down to 0), `$00C0` the running total from 0 [L; C work RAM,
text map]:
- A pass (`$80:F680-F751`): the cell of row n is at text row 9 + 2n, column 7 + 6 x column
  (`$80:F68F-F6A6`; 3 more for the 2P column). Each row whose shown count (`$00B2 + 2n`) is below its
  tally (the count byte `$77:076B + 16n + 4 x column`, R-0066) shows one more and is printed; a row
  whose shown count is 0 prints its 0 again.
- A pass that raised a row waits (`$80:F755-F77C`): after its frame wait the text goes to VRAM and the
  decorations step (`$80:F765-F76C`), then `$80:F7FB` with Y = 6 (`$80:F775`): 7 more frames, each
  copying the OAM and reading the pads (`$80:D1EC`) and stepping the decorations. The next pass runs in
  the eighth frame: bowl-lose's passes at 4158, 4166, 4174 [C].
- A pass that raised none (`$80:F77F-F7A2`) adds the column's five points words (`$77:076D + 16n + 4 x
  column`) to `$00C0`, prints it at row 22, column 19 (`$80:F44F`), and waits the same way with Y = 12
  (`$80:F7AD-F7BC`): the next column's first pass 14 frames later (`$80:F7C4`). bowl-lose: x1's total
  31 at 4246, x2's first pass at 4260 [C].
- A press cuts a wait short (`$80:B6D3`: any of pad 1's twelve buttons; pad 2 is ignored while
  `$77:0742` bit 10 is set) [L]: the next pass runs in that frame. A press during the wait of a pass
  also only skips a sound (`$80:F758-F75D`, `$80:B178`); the column's sound is `$80:B124`. Native plays
  no sound. The first pass's check reads the pad word the menus saved before the race (0x1000, NOW
  PLAYING's Start, on all four captures), so its sound is skipped [C: `$80:F75D` first runs at 4166 on
  bowl-lose]; every wait reads the pads afresh (`$80:D1EC`) before it tests them.
- After the last column (`$80:F7CB-F7D7`, a frame wait, the text, the decorations, `$80:D1EC`): the
  total printed again (`$80:F21C`), the rider's best `$77:0829 + 2 x (50 x rider + track)`
  (`$83:9E47`) replaced by the score `$77:07BB` when the score is higher, unsigned, and the new-best
  markers 96 and 98 shown (`$0C18` &= 0x66, `$80:F222-F244`). bowl-lose: `$77:082D` 0 -> 49 at 4327;
  hill-win: `$77:0855` 0 -> 121 at 4386 [C].
- Then (a frame wait, `$80:F249`) with a computer opponent (`$77:0749` >= 0x10) and `$77:10AD` != 4:
  the qualifying score (`$83:9EEB` into `$00B2`) printed by `$80:F45D`; a frame wait (`$80:F26F`);
  the text; `$80:F88D` with X = the score and Y = 0xFFFF: `$77:0618 + 16 x $CC + 2 x $77:0678[$CC]` =
  the score, and the checksums (`$83:90F4`). bowl-lose: `$77:0618` = 49 at 4329 [C].
- `$80:9575` returns to `$80:9579`, which skips the one-run result's tail for mode 2
  (`$80:957D-957F`), and `$80:9599-95A1` runs `$80:C24C`: the release and press waits are the one-run
  result's (R-0057). bowl-lose's release at 4330, the press 4700-4701 [C].

`$83:9E47` also writes `$0195`/`$0197`, work RAM the comparison does not read.

## Leaving: the statistics, the top three and the scoring

The exit is the one-run result's (`$80:C236`, `$80:9805`, `$80:F4B8`; R-0057): q is the press's
second frame. `$80:C786` dispatches by mode to the stunt branch `$80:C948-C9D6` [L; C cartridge
RAM]:
- races +1 for the rider (`$80:CA74`) and a rider opponent (`$80:CAAA`);
- the score `$77:07BB` against the opponent's `$77:0825` (0 in one-player play): higher or equal adds
  the rider's win and `$77:10A9` (`$80:CA83`), lower or equal a rider opponent's win and `$77:10AB`
  (`$80:CAC5`);
- the stunt points `$77:0236 + 8 x rider` += the score (`$80:C984`), and a score that is not 0 goes
  into the track's top three (`$80:8CCB-8D6D`): scores `$77:0422/0486/04EA + 2 x track`, holders
  `$77:0550/0582/05B4 + track`, a slot taken only by a strictly higher score (an equal score goes
  below it), the checksums after a placing; a rider opponent's the same (`$80:C9AB`), the loser's
  after the winner's;
- the checksums again (`$80:C97E`).

When a score was placed the scoring comes on q + 3, otherwise on q + 2 (the one-run result's rule,
R-0060): bowl-lose and hill-win (placed) q = 4701, statistics 4702, scoring 4704; bowl-quit (score
0, nothing placed, only `$80:C97E`'s checksums) q = 3001, statistics 3002, scoring 3003 [C].

The scoring (`$83:879A`, R-0065) runs the stunt event's win test `$83:88E1-88F6` through `$83:88F7`
[C trace 4704]: `$83:9EEB`'s qualifying score, a win when `$77:07BB` >= it (unsigned), so an equal
score wins. A win marks the track done and adds the tour's five done bytes (R-0065); a loss sets
`$77:0742` bit 12 [C: `$77:0743` 05 -> 15 at 4704 on bowl-lose, `$77:108B` = 1 on hill-win].

`$83:9EEB` takes its medal through `$83:9EB4` (X = 16 x `$00D0` + `$00CA`) and `$83:9EF3`
(`$77:069C,X` & 3, gold counting as silver) [L]; native's `tour_qualifying_score`.

## A tour completed by a stunt win

hill-complete [C]: hill-win's inputs with WALKER's other four done bytes (`$77:1089`, `108A`, `108C`,
`108D`) written after frame 2000, during the race (R-0065's route). The win at 4704 makes the fifth
done track and runs the completion (`$83:881B`): the award (native `tour_award` from 4704), the press
at 6301, PICK TOUR, WALKER chosen at 6601. Native's completion needed no change.

## The pause menu's QUIT in a stunt event

bowl-quit [C]: the race's pause menu's second choice after the countdown (0xEA61 in `$77:0769`,
R-0060) at 2120. On the return `$80:9A50-9A79` [C]: the score `$77:07BB` = 0, and the opponent's
score `$77:0825`, its MEGA x1 count `$77:0815` and points `$77:0817` each +1 (at 2224, r + 104). The
tallies stay: the result counts the tricks and shows their points (23 on bowl-quit), but the score
is 0, which is not the rider's best, places no record, and loses both the statistics' test (0 < 1:
no win) and the qualifying test. An opponent's quit (`$80:9A7D-9AA4`) is its mirror, from pad 2's
pause (not in one-player play). Native: `apply_quit_scores` at r + 104; the opponent's tallies are
not kept.

## The cold start's tries

`$77:1073` holds 0 from the records' wipe at boot frame 403 until a rider is chosen, when
`$80:BBE3-BBE5` sets 3 [C: bowl-lose, 0 from 403, 3 at 750]. Native's cold start held 3 (R-0064's
"older difference"); it now holds 0, and the rider's choice sets 3 as before.

## Native

- `stunt_result.cpp`: the screen, the tally (`StuntTally`: the column, the shown counts, the total,
  the frames waited), the best score, the qualifying line, then the one-run result's waits
  (`wait_for_result_press`, shared). A new screen, `FrontEndScreen::stunt_result`.
- `race_result.cpp`: the stunt event's scores and tallies in `RaceTimes` (from the race's
  `feature_total`s and `StuntEvent::tallies`), the quit's words, `update_stunt_records`
  (`$80:C948`), `insert_stunt_record` (`$80:8CCB`) and the win test `race_won`; the result's common
  first frame `start_result_screen`.
- The text printer's F0.
- `src/app/frontend.cpp`: a stunt event is a native race; the notice is gone.
- Pack profile v25 (429 entries): assets 0x26-0x39 (the heads' colours) and the two tables above.
- Native keeps no score history (`$77:0618`) and no checksums, as for the other results (R-0057):
  nothing in one-player play reads them.

## Evidence

In `local/evidence/stunt-result/`: the manifests and captures (`decode/`: `capture.py`'s work RAM and
cartridge RAM every frame from power-on and pictures from 400; `project/`: the same manifests
through `coverage capture` and `access capture`, whose work RAM series equal `capture.py`'s on every
frame and whose pictures from the race's return are identical: 1,357, 1,490, 1,481 and 3,090),
`compare.py` and `sram.py` (this task's copies), `acceptance.sh` and its logs, and `NOTES.md`. The
coverage captures executed the branches cited [C] above: `$80:F0EE-F27F`, `$80:C474`, the tally's
passes and waits, `$80:C948-C97E` (`$80:C967` on the wins, `$80:C95C` on bowl-quit), `$80:8CCB` on
bowl-lose and hill-win only, `$80:9A5B-9A79` on bowl-quit, `$83:88E1` with `$83:88F1` (hill-win,
hill-complete, then `$83:881B`) or `$83:88F4` (bowl-lose, bowl-quit); not `$80:F283` (two players).

| Capture | Route | Frames compared | Differences | Pictures equal |
| --- | --- | --- | --- | --- |
| bowl-lose | CRAWLER, BOWL, 49 < 68, A at 4700, PICK TRACK | 2,565 (power-on to 5400) | none | 2,165 of 2,165 |
| hill-win | WALKER, HILL CLIMB, 121 >= 90, A at 4700, PICK TRACK with HILL CLIMB done | 2,698 (to 5600) | none | 2,298 of 2,298 |
| bowl-quit | BOWL ridden to 1989, the pause menu's QUIT at 2120, A at 3000 | 2,689 (to 3600) | none | 2,289 of 2,289 |
| hill-complete | hill-win with WALKER's other four done, the award, PICK TOUR | 4,298 (to 7200) | none | 3,898 of 3,898 |

Frames compared exclude the race's own frames (R-0066 compares them); the pictures are every frame
from 400 outside the race. The records (`sram.py`: the statistics, the records and holders, the
medals, the bests, bit 12, the tries, the done tracks, the wins, the levels) equal the original's
cartridge RAM at 500 (a cold start's tries), 1000, the result's first frame, the tally's end, the
best, the history, the statistics, the scoring, after it and the end: 10 frames each on bowl-lose,
hill-win and bowl-quit, 11 on hill-complete (the award at 4800, PICK TOUR at 6400 and the end).

## Not covered

- Two players: the 2P tally (`$80:F283-F2E9`: the second pass from `$77:07D5`, X = 0x6A), its total
  and line (`$80:F456`, `$80:F4A3`, `$80:F4B5`), the 2P rider's best (`$80:F2AE-F2C9`), a rider
  opponent's statistics and records; `$77:10AD` = 4. Native refuses a stunt result for two players.
- A press during the tally (`$80:B6D3` in `$80:F7FB`): read from the listing and tested ROM-free, not
  captured.
- The score history `$77:0618` and the checksums are written [C] but not kept natively.
- The opponent's tallies (`$77:07D5-0824`) and an opponent's quit (`$80:9A7D-9AA4`).
- The sounds `$80:B124` and `$80:B178`.
