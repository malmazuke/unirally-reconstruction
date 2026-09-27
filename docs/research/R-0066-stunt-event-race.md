# R-0066 - The stunt event in the race engine (race mode 2)

Status: recovered and implemented on `task/stunt-event-race` (STUNT-EVENT-RACE), 27 September
2026 (written on main `e01aaaa`, integrated on `94b3034`). PAL ROM
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, audited bsnes core
`e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b`. Builds on the STUNT-EVENTS
decode (`local/evidence/stunt-events/decode/stunt-events.md`, sections 1, 3 and 4.1), which this
record supersedes where they differ.

Evidence tags: **[C]** seen in a capture (work RAM, cartridge RAM or the program-counter trace of
the decode's `trace.py`), **[L]** read from the static listing (`artifacts/static-map/bank-8x.lst`),
not seen executing on its own. Every rule below is also confirmed by the native comparison: the
race rows of every capture listed under Evidence are equal from the boundary through the result
load.

## Summary

A stunt event is race mode 2 (`$77:074B` = 2; `$83:99AD-99B8` gives place 2 of every tour, so
tracks 2, 7, 12, 17, 22, 27, 32, 37 and 42). It is a 45-second solo run for points against the
tour's qualifying score. Native now runs it on its own scenario (`classic.track.NN`) with pack
profile v24, which adds the nine tracks' content. The stunt picture (STUNT-HUD) and the stunt
result screen and records (STUNT-RESULT) are not part of this record; the menus still show a notice
for a stunt event until the result exists.

## Setup

- Scenario: no laps (`$0EFB/$0EFD` = 0 + 1, one start-line crossing), not a tour race, BRONSEN in
  the opponent's slot (the medal's opponent, SILVIA or GOLDWYN, after a medal; on HUNTER's tour too,
  as `$80:B351-B35F` skip the ANTI-UNI assignment `$80:B361` in mode 2 [L]; GOLDWYN in the track 42
  capture [C]). HUNTER's stunt event keeps `$131F` = 1 and the tag effects [C].
- Initialization boundaries on the laboratory's menu path [C]: 2 at 1335, 12 at 1343, 22 at 1331,
  32 at 1349 (a cold start), 7 at 1336, 17 at 1330, 27 at 1360, 37 at 1368, 42 at 1399 (LOCKED-TOURS'
  unlocked path).
- Clock: `$82:D7FD-D836` [L] takes the track header's minutes (byte 1) and seconds (byte 2, split
  into tens and units) and counts down (`$0BF9` = 0xFFFF) unless the time is 0:00, when it counts
  up. Every race track's header holds 0:00 and every stunt event's 0:45 (all 45 headers read from
  the ROM), so native takes the direction from the race mode and refuses a header that disagrees.
- AI flag `$0C6D`: cleared at the boundary (`$83:CBD8`) [C]. `$83:CC0B` then skips the opponent's
  tier: level `$1275` 0, catch-up `$1283` 0, and the mode's progress bound `$1281` 0x48
  (`$83:CC72`) [L][C].
- Qualifying score `$77:0753`: `$80:99ED` calls `$83:9EEB`, the word at
  `$83:A218 + 2 * (3 * tour + medal)`, the medal being the rider's best on the tour (`$77:069C` & 3,
  gold counting as silver) [L]; 68 on CRAWLER with no medal, 245 on HUNTER (whose cold-start medal
  byte is 2) [C]. The table is the pack's `front-end.qualifying-scores` (v17). The runner takes
  `--best-medal`; the menus pass the rider's medal.
- The trick tallies `$77:076B-07BA` and the scores `$77:07BB`/`$77:0825` are zeroed at every race's
  setup (`$82:DA75`, `$82:DB1D`, and the tally words beside them) [L].

## The opponent switched off

`$81:8E53-8E5D` (contact) and `$82:8EAB-8EB5` (the rider update: from `$82:8EB8` to `$82:9397`) skip
the opponent while `$0DE1` (a second human) and `$0C6D` are both clear; `$83:E084-E089` skips its AI
[L][C]. The port-2 reader (`$82:AB6A-AB91`) then reads pad 2, which nothing holds [L]. So:
- the opponent keeps its spawn position and zero velocity, never tricks and never contacts [C];
- `$81:8709-8718`, the queue cooldowns' decrement, runs once an update instead of twice: captions
  and rewards last twice as long [C];
- `$0E7B` (the drive target latch) is left as the player's pass wrote it [C];
- its finish test, its finish routine and its announcement queue still run (below).

## The countdown

`$83:E59C-E7C0` [L][C]: a race's phases 3 and 4 wait for 0x82 and 0x46; a stunt event's tests
(`$83:E686`, `$83:E6E8`) skip the waits. Phase 3 (`$11C5` 160 down to 101) therefore drops the
start boost of a rider not braking at once, and phase 4 (100 down) spends it and returns without the
hold (`$83:E704-E740`), so the riders are free from 100 (a race: 70). The brake-forcing path
`$83:E759` never runs [C].

## The clock and the finish

- `$81:C7CE-C867` [L][C]: once the countdown is below 0x44 (`$81:C6A0`), the digits count down a
  tenth every five updates (`$81:C7DE`); each digit borrows below 0 (`$81:C7F4-C82E`). The tick
  after 0:00.0 clamps the minutes at 0 (`$81:C830`), leaving the other digits at 5, 9 and 9 (the HUD
  keeps 0:00.0), and sets `$0BF5` (`$81:C864`). From then on (`$81:C7D2`) every update runs the
  finish test `$81:C836-C861` alone: a rider finishes (`$0EFF`/`$0F01`) when its unsupported count
  `$054B/$054D` is below 2 and its vertical velocity is at least -256 (`$81:C838-C846`). Times stay
  60000.
- Before the race's finish routine, `$83:E7C1-E8DD` [L][C], new in this record: each finished rider
  that stands (unsupported count 0, `$83:E85C`, `$83:E87A`) sets `$12DF`/`$12E1`; from the next
  update its vertical velocity is held at 128 (`$83:E856`, `$83:E874`) and its controls are released
  (the player's reader `$82:AA7D-AAA1` clears A, X, the rotations, jump, brake and Start and centres
  the horizontal axis, leaving the vertical one; the opponent's `$82:AB62`). Once both have
  finished and stand, `$83:E8B6` clears the tutorial hints (`$12E3`, `$12E5`); once both
  announcement queues are also empty, `$83:E8DA` sets `$0FE9`. Only from the next update do the
  finish poses and captions (`$83:E8E0`) run and the finish display `$0F0F` count towards the result
  load (`$83:E803-E81D`). BOWL [C]: the opponent finishes at 3796 and stands, its `$04C1` is 128 from
  3798, the player finishes at 3802, `$0FE9` is set at 3803, and the opponent's `loser` is queued
  from 3806.
- The result load begins when `$0F0F` reaches 240, as a race's does; the race's return then runs
  until the stunt result (`$80:F0EE`) takes over at load 106. Native's stable result is load 105.

## Scoring and captions

- The player's queue consumer `$81:C0CE-C18A` [L][C]: for an event below 0x48 whose class
  `$81:C50A[e - 1]` is not 0xFF, the tally byte `$77:076B + 2c` counts it (`$81:C0FF-C116`) before
  the weight is read, so a trick of weight 0 still counts; a non-zero weight adds to `$77:07BB`
  (`$81:C12A-C132`) and to the column's points word `$77:076B + 2c + 2` (`$81:C173-C184`), then
  halves. Columns are four bytes (count, zero, points); rows ROLL, FLIP, TWST, ZEE, MEGA are 16
  bytes apart. The class table is the pack's `physics.reward.rotation-class`. Native keeps the
  player's tallies in a stunt event only (every race writes them too, but only the stunt result
  reads them); the opponent's (`$77:07D5`) stay 0, as it shows no trick.
- Captions `$83:E940-E957` (player) and `$83:EAD2-EAE7` (opponent) [L][C]: the score against the
  qualifying score, a 16-bit difference: equal is `draw`, negative `loser`, else `winner`; the
  opponent's score `$77:0825` is 0, so it announces `loser`. The win test of the scoring
  `$83:88E1-88F6` counts equal as a win [C, decode].

## Two findings on single tracks

- `$82:A81A-A829` [L][C]: on tracks 2 (BOWL) and 42 the vertical speed cap takes its other path
  (`$82:A84F-A86A`): falling is capped at 768 whatever the extra, and rising is not capped at all
  (the listing compares with -(768 + the vertical boost) and, on the path that would store, compares
  again with the same result and branches past the store). R-0011 had left these "cartridge modes"
  unrecovered; `$77:074A` is the track. BOWL [C]: vertical velocity -876 at update 293 of bowl-lose
  where the ordinary cap gives -768.
- `$81:A343-A387` [L]: track 37's playfield (header byte 13 = 0x04) is 16 columns of 1,024 rows,
  continuing the other arms' progression (mask 0x03FF, shift 6, follow window -0x600/0x640, visible
  span -0xC40/0x4000), and sets `$0FF7`, which skips the sampler's clamp of a negative y to cell
  (0, 0) (`$81:8A2C-8A2F`): the 65,536-unit-tall playfield takes y's sixteen bits as its row. The
  BG1 map fetch's use of `$0FF7` (`$81:AD1D-ADA7`) belongs to the picture (STUNT-HUD). The idle
  capture of track 37 [C] is exact; no capture yet reaches a negative y there.

## The state

A stunt event's state is `URTRnn07`, 1,006 bytes: the other tracks' 916 (`URTRnn06`) and 90 more:
the qualifying score, `$0BF5`, `$0FE9`, `$12DF`, `$12E1`, then the 80 tally bytes in the original's
layout. Restore guards: a count's second byte is 0; a stopped clock reads 0:59.9; a rider finishes
only once the clock has stopped, and settles only once finished; the display waits for both
finishes, and the finish poses and the finish display wait for it; the columns' points add up to
the score; the qualifying score is one of the tour's three.

## Evidence

Captures (work and cartridge RAM every frame from 1100), compared with
`python3 -m tools.unirally_lab.native.track_reference explore --scenario auto` (race rows from the
boundary; after the result load begins, rows through load 105):

| Capture | Track | Controller | Rows exact |
| --- | --- | --- | --- |
| stunt-events/decode/bowl-explore | 2 BOWL | trick cycle, no exit | 2831 of 2831 |
| stunt-events/decode/bowl-lose | 2 BOWL | search.py tricks, 49 < 68 | 2814 of 2814 |
| stunt-events/decode/hill-win | 22 HILL CLIMB | search.py tricks, 121 >= 90 | 2885 of 2885 |
| stunt-events/decode/bowl-brake | 2 BOWL | Y held 1500-1699, tricks | 2808 of 2808 |
| stunt-events/decode/hill-brake | 22 HILL CLIMB | Y held 1500-1699, tricks | 2902 of 2902 |
| stunt-event-race/captures/jumps-ride | 12 JUMPS | search.py tricks, 59 >= 58 | 2815 of 2815 |
| stunt-event-race/captures/downer-ride | 32 DOWNER | search.py tricks, 86 < 107 | 2952 of 2952 |
| track-breadth-2/sweep/row0-pos2, row1-pos2, row2-pos2, row3-pos2 | 2, 12, 22, 32 | released, to frame 2900 | 1566, 1558, 1570, 1552 of the same |
| locked-tours/sweep/jumper-2, bounder-2, runner-2, sprinter-2, hunter-2 | 7, 17, 27, 37, 42 | released, to frame 2900 | 1565, 1571, 1541, 1533, 1502 of the same |

Nothing else moves: the native equivalence sweep against main's binaries and the per-track
recompare of the track-breadth and locked-tours sweeps, race tracks identical (the task record's
gates).

## Not covered

- Two players (`$0DE1`, `$77:0750` bit 3, `$83:E958-E964`, `$83:EAEA`): the second rider's pass,
  its score `$77:0825` and captions.
- A start-line crossing in a stunt event (none seen; `$0EFB` stays 1); a draw caption; a negative
  y on track 37; the HUNTER tag touching the frozen opponent on track 42.
- The pause QUIT's stunt words (`$80:9A50-9A79`), the stunt HUD and picture, the result screen,
  the records and scoring: STUNT-HUD and STUNT-RESULT.
