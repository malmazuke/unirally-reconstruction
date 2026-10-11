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
- **Which pad the menus read.** Every menu test goes through the `$77:0742` gate (`$80:B6D3-B82B`,
  `$80:B8F1`). The main menu opens both pads (`$80:ABC8`); a rider pick from it takes pad 1 alone
  (`$80:BBA5` 1P, `$80:BCC8` 2P, `$80:BF4E` VS), 2P's and VS's second pick pad 2 alone (`$80:BCEB`,
  `$80:BF6B`), and both pads open again after it (`$80:BD08`, `$80:BFB7`). So NOW PLAYING, PICK
  TOUR and PICK TRACK take either pad with two humans or in a league, pad 1 in 1P. After a VS
  challenger pick (`$80:C087`, `$80:C09C`) the picking pad alone drives the continuation and every
  screen it opens: `$80:C0DD JSR $ADE3` returns only on NOW PLAYING's Race or on QUIT, and only then
  does `$80:C0E0` open both pads (the review's W4: pad 2's Y at 5450 and its later presses are
  ignored). [L, C]
- **The slide word `$00AC`.** The VS challenger's menu slides back in unless `$00AC` is 2, then
  forward, stepping the decorations (`$80:CB42-CB48`). The 2P and VS loops leave it at 2 from a
  rider pick into PICK TOUR (`$80:BBFF`, `$80:BFBD`), NOW PLAYING's Y (`$80:BC4F`, `$80:BFEA`,
  `$80:AF0E`, `$80:AF47`), the continuation's Y (`$80:ADFA`) and SELECT TOUR (`$80:AF1D`), and at 1
  from a tour chosen (`$80:BC10`, `$80:BFCE`, `$80:AF2E`), PICK TRACK's Y (`$80:BC05`, `$80:BFC3`,
  `$80:AF3A`), the continuation's entry (`$80:ADF8`) and SELECT TRACK (`$80:AEF5`). The review's W4b
  (pad 1's Y on NOW PLAYING, then SAME TRACK, a race, VS CHAMPIONS and the challenger at 9504) shows
  the forward slide; its unplaced race does not matter. [L, C]
- **NEXT TRACK** (`$80:AE96-AECC`) takes the next track, the first after the last (44), and goes on
  until its tour (`$83:99A8`, track / 5) is open against `$80:E842` at `$83:9F14`'s level, playing
  the select sound for each track tried. `$83:9F14-9F46` gives the rider's level in 1P
  (`$77:10AD` = 1) and the highest of all riders' otherwise. Captured from track 4 with tour 1
  locked (twop-next-wrap: tracks 5-9 skipped, track 10, tour 2; 800 of 800 pictures equal from
  3800). [L, C]
- **OPTIONS after a session.** `$0089`, the main menu's idle count, is also the decorations'
  delay: from OPTIONS' choice (`$80:B626-B639`) its low byte counts down until it goes negative
  and only then restarts from 2 (W2b: 180, then 2; W4: 120, 119, ...), and a rider menu from
  OPTIONS starts from that delay, not from an idle count. OPTIONS runs the generic list loop
  (`$80:B63C JSR $B93C`), which turns the PICK TRACK markers (`$80:EB23`) each frame. DEFINE
  PLAYER's confirm leaves the chosen track at 0 (`$83:9BB5`, its best times' loop). The review's
  W2b is equal from 9900 to 15999, W4 to its end. [L, C]
- **Y from a screen the continuation opened** returns to the continuation (`$80:ADFA`): NOW
  PLAYING after NEXT or SAME TRACK (`$80:AECE-AEDA`, `$80:AED3`), PICK TRACK after SELECT TRACK
  (`$80:AEF3-AF03`, `$80:AEFC`), PICK TOUR after SELECT TOUR (`$80:AF1B-AF2B`, `$80:AF24`). The
  menu prints in the frame the screen returns and slides back in without palette reloads, the
  arrow where it was. SELECT TOUR opens PICK TOUR with a forward slide and without `$80:A858`. [L, C]
- **When a print runs past its frame.** `$80:C786` starts about 19 scanlines into its frame; the
  checksum routine `$83:90F4` takes about 165 scanlines and runs once at the end plus once per
  time or score placed in the records; the header, sort and first row take about 181 more before
  `$80:FADF` waits. With exactly one placing VS CHAMPIONS' first row falls in the header's frame,
  with none or two a frame later. In a league about 176 scanlines of points and awards printing
  come first, so two placings push the scoring a frame on. The awards press counts only on two
  frames running after the result sound (`$80:C206`); after it a frame wait, the palettes and an
  OAM copy, and the table prints in the next frame (`$80:B05B-B079`, then `$80:A547`). [L, C]
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
- League: after organic-three's first pair the original ends `$80:A547`'s table print at scanline
  224, dot 742, one scanline before vblank, misses that frame's wait and starts the table slide a
  frame late (the same prefix in organic-three, -clean, -complete and organic-odd-resume, which
  explains their frames 11200, 15400 and 13400). Predicting it needs CPU cycle counting. Main's
  medal bug used to cancel it, so these four manifests are a frame further from the original on
  those frames; organic-stunt-tricks, -three-events and -two-race are now equal on every frame.
- A 2P or VS result also writes the league totals: `$80:F88D` stores at `$0618 + 16 x slot + 2 x
  the pair cursor`, the cursor read as a word from `$0678` whose high byte is the next slot's.
  Native leaves `$77:0618`/`$061A` at 0, so a second two-human race's result words differ.
- One-frame print overruns: VS CHAMPIONS' header frame with two placings and the lap results'
  first frame (race RAM in the text buffer); W2b frame 15672 (the rename keyboard, 256 px).
- Tracks 7, 17, 27, 37 and 42 with two humans (locked tours) and a race tie are not captured;
  nor is NEXT TRACK's wrap after track 44 (it needs HUNTER's tour open).
- `$77:10B1`, the race count, advances in the race's setup (`$83:CA08`), 64 frames after the race
  is chosen; native counts it as the race's fade ends (choice + 7). The values agree again once the
  race has loaded; only cartridge RAM read in those 57 frames differs (the review's W2 at 9900).
- League slots, RECORDS and the other generic list menus (`$80:A9CA`, `$80:D53F`, `$80:FBEE`) also
  turn the hidden PICK TRACK markers; native turns them in OPTIONS and the continuation only (OAM
  entries 64-68's tile bytes, hidden; organic-stunt-tricks from 1060).
- `$073E-0741` and `$10A7` are not kept; `$106C` differs after a VS result (twop-restart at 8399,
  the review's W2, W2b and W3), and `$1116` at W1's frame 4000.
