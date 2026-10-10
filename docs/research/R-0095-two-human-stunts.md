# R-0095 - Stunt events with two humans, and the two-player menus around them

Status: research result and native change ([TWO-HUMAN-STUNTS](../../tasks/TWO-HUMAN-STUNTS.md)),
10 October 2026, on main `ef8e440`. PAL ROM SHA-256
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, reference core bsnes
`7d5aa1e6`. Evidence in main `local/evidence/two-human-stunts/` (captures, `checks.sh`,
`checks/pass2/`) and `two-human-stunts-audio/`.

Tags: **[L]** read from the ROM's bytes, **[C]** confirmed in a capture or probe.

## Domain

Nine cold-start captures and in-process probes of the original: 2P and VS on BOWL, JUMPS, HILL
CLIMB and DOWNER (track 32), a zero-score VS tie, a 2P pad-2 QUIT, the VS challenger, lap races in
2P and VS, a one-player BOWL loss; three sound schedules; the league's frozen pictures.

## Findings

- **The race.** Rider 1 keeps its own trick tallies (`$77:07D5-0824`, `$81:C238-C2C3`,
  `$81:C24E-C2C3`), is released once it has settled (`$82:AB62-AB8E`), and the split HUD draws
  both scores (`$12C9`/`$12CB`, `$81:C3D3-C43B`, `$81:E705-E82E`); the stop tick asks for the
  lower clock (`$81:C7EE-C7F1`). While pad 2 has paused the race, the opponent's words come from
  port 2 as the pad reader leaves them (`$82:AB5C-AB91`). [L, C]
- **The result** (`$80:F0EE-F2E9`, `$80:F283-F2E9`, `$80:F669`, `$80:F69D-F6A3`, `$80:F4B5`): with
  a second human the player's tally, then rider 1's three columns, each with its best and
  markers; two-rider results run a frame earlier; pad 2 counts during the tally outside 1P;
  entry 108 is hidden only against a computer opponent (`$80:D0EF-D108`). Both riders' quit
  words apply (`$80:9A50-9AA4`, `$80:9A86-9AA4`). This also repairs the one-player stunt result,
  which LEAGUE had broken on main (STUNT-RESULT's five captures equal again). [L, C]
- **VS.** Counters `$77:0380`, `$0382`, `$0400` (`$83:9229`, `$83:925E`, `$83:92B0`), cleared at
  boot (`$83:8B23`) and by DEFINE PLAYER (`$83:9BE1`); percentage scratch `$77:10F9/10FB`
  (`$83:9D80`). VS CHAMPIONS ranks by today's wins (`$80:F02B`) and prints a row a frame
  (`$80:F8E7`, `$80:F905`, `$80:F949`, `$80:F974`, `$80:F9A5`, `$80:F9DB`, `$80:F9E4`). A tie shows
  REMATCH (`$80:C120`, pack v38's `front-end.vs-rematch-text`) and returns to NOW PLAYING with
  nothing counted (`$83:99C2`, `$80:C02C`, `$80:C026`). The challenger: the loser picks with its
  own pad and the winner is refused (`$80:C048-C0E0`, `$80:C04A`, `$80:C051`, `$80:C054`,
  `$80:C087`, `$80:C09C`, `$80:C0C0`, `$80:C0B1-C0BB`), the menu sliding back in (`$80:CB42`); pad
  gates `$0742` bits 9/10 (`$83:952E`, `$83:9543`, `$83:9558`, `$83:956D`). [L, C]
- **The menus around them.** The second rider's pick reprints "PICK ANOTHER" in place and
  re-enters the loop, no slide (`$80:BCDB-BCEF`, `$80:CBC3`; MENU-INPUT's item). PICK TOUR shows
  medals only in 1P (`$80:E5A5-E5AD`; `$10AD` = 4 after it in league, `$80:BE45`); PICK TRACK with
  two humans has no medal line, no done-track markers and four items (`$80:E8FE-E904`,
  `$80:E93D-E945`, `$80:E997-E9A3`). The continuation (`$80:ADE3`, `$80:ADE5-ADF4`, `$80:ADFC`,
  `$80:AE12-AE1A`, `$80:A858`) runs the generic list loop (`$80:B93C`, `$80:B962-B975`,
  `$80:B983-BA5E`, `$80:BA1F`, `$80:B71D`, `$80:B74A`, `$80:B178`): arrow columns `$80:AE81`,
  wrapping Down/Up with one latch, Y/X ignored, the markers turning. `$80:9719`, `$80:BD98` set up
  the two-human races. League: the podium's arrow restores a frame later on phases 2 and 3, and
  POINT AWARDS step the decorations only from script frame 45 (`$80:8B28`). [L, C]
- **Race loading.** Every mode's race loading follows the song table (`$83:CD14-CD1C`), which
  explains R-0071's and R-0073's measured lengths and R-0090's two-human restart record
  differences (a one-frame loading error). [C]
- **Results.** Race rows equal on 2P/VS BOWL, JUMPS, HILL CLIMB, the zero-score tie and the pad-2
  QUIT (1,074 of 1,074); the front end equal on every probe frame but VS CHAMPIONS' entry frame
  5353 (the original's header print runs past the frame) and the lap results' first frame (race
  RAM in the text buffer); cartridge RAM's record fields equal; five sound schedules equal; the
  league's frozen pictures 0 px; the front-end sweep against main better on every 2P/VS manifest;
  the race sweep 0 differences. [C]

## Not covered

- **DOWNER (track 32) with two views:** rider 1 lands at frame 3558 with -288 against -308. The
  multiplier helper `$81:B67C` (from `$81:8F7C`) writes `$211B` low then high; at scanline 112 the
  split HDMA writes a register sharing the mode-7 latch between the two, so M7A becomes `$021C`
  instead of `$0202`. Reproducing it needs CPU cycle timing.
- League: the table's entry slide and arrow start two frames early after the awards (main's medal
  bug used to cancel it); four league manifests are a frame further from the original on one
  frame each.
- Tracks 7, 17, 27, 37 and 42 with two humans (locked tours) and a race tie are not captured.
- `$073E-0741` and `$10A7` are not kept; twop-restart's `$106C` at 8399 differs.
