# R-0085 - A player voice from 150 and the end of the tutorial hints

Status: research result, no native change
([PLAYER-HINT-VOICES](../../tasks/PLAYER-HINT-VOICES.md)), 6 October 2026, on main `ec05ea7`.
PAL ROM SHA-256 `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, audited bsnes
core `7d5aa1e656b9171524d01b1b22917197d8121cb4`. The captures are in main
`local/evidence/player-hint-voices/`.

Tags: **[L]** listing only, **[C]** confirmed in a capture.

## The question

SPLIT-CAPTIONS (R-0082) read the hint end at `$81:C5B0-C5BC` (the player) and `$81:C5D5-C5E1`
(rider 1) as a test of bit 7 of the byte `event - 22`. So events 0-21 and also 150-255 end the
hints. Native's `queue_player_announcement` ends the player's hints only for `event < wrong_way`
(22). The only events from 150 a player can queue are the combination voices of characters 10-15,
and of character 9 with x & 15 = 14 or 15 (R-0061). If such a voice could be queued while the
hints run, before any scoring event, native would keep the hints running where the original ends
them.

## Answer: no divergence

The original ends the hints on the trick event queued just before the voice, not on the voice.
Native does the same, so the two tests give the same result on every event sequence the game can
produce.

- **Order of the queue calls.** A landing queues in this order:
  1. the head bounce (16) and the tabletop (17) (`$82:9C6D`, `$82:9CA3`);
  2. one event per kind of trick counted (`$82:9CEA-9D14`): roll 1-4, flip 5-8, twist 9-12,
     z flip 18-21;
  3. the combination voice twice (`$82:9D64`, `$82:9D70`).
  [L]
- **A voice needs a counted trick.** The voice is queued only when the combination table entry
  `$82:9DB6 + index` is not `$FE` (`$82:9D35`). The index is the four counts in radix 5. Entry 0,
  no tricks, is `$FE`, so a voice always comes after at least one trick event below 22 from the
  same landing. [L; ROM byte]
- **A full queue cannot separate them.** The full test at `$81:C5A8` (write cursor equal to read
  cursor) comes before the store. Nothing reads the queue during a landing. So if the queue has
  room for any of the events, the first one stored is a trick event. [L]
- **Other events from 150.** No other caller of the queue routine queues one. The callers queue
  14, 15, 22, 37-39, the hints 40-71, the trick events and the voices.
  [L: the 26 `JSL $81C598` sites in banks `$82`-`$83`, and `$81:81BA`'s `JSR $C59C` with 15]

## Capture [C]

- **The run.** WALKER's fourth track (track 23), rider 12 (voices 168-183), tutorial hints on
  (`$77:1116` = 0). Right is held from 1555, and X with R from 2073 to 2190.
  - A native search on the player's queue found these inputs.
  - Rider 12's six menu presses move the race boundary to 1441, 55 frames later than
    `classic.track.23`'s 1386.
- **`combo-a` and `combo-b`.** `track_reference capture` records every frame's work RAM up to
  frame 2800. The two runs are identical (equal WRAM, SRAM and video digests).
  - The first scoring landing is at 2198. It queues 3 (treble roll), 18 (z flip), 171 and 171.
  - On that update `$12E3` goes 1 to 0 and the hold `$0CA5` is zeroed. The same update then
    takes event 3 (hold 28).
  - The earlier hint groups are 44-47 (1711) and 48-51 (2011).
  - Rider 12's tutorial bit (`$77:1116` bit 12) is set by the end.
- **`access-combo`.** `access capture` of the same inputs as `combo.json`, with every frame's work
  RAM `$0000-$1FFF` and the registers at three PCs.
  - At 2198, `$81:C5B0` runs four times, with A = 3, 18, 171, 171 and Y = slots 9-12.
  - `$81:C5B4` also runs four times. This confirms that 171 passes the `CMP #$16`/`BPL` test as
    the listing says.
  - `$81:C5B9` (`STZ $0CA5`) runs once, with Y = 9: the treble roll's store. The voices then read
    `$12E3` = 0.
- **Native.** `track_reference explore` (`classic.track.23`, rider 12) equals the original on all
  1,360 updates from the boundary to 2800.
  - The compared rows include the player's queue `$0CC1-$0CE0`, its cursors, `$0CA5` and `$12E3`.
  - The player's voice 171 is the first captured player voice past 87. It closes R-0061's open
    "player's voices past 87" item for rider 12.

## Not changed

`queue_player_announcement` keeps `event < wrong_way`. It is equivalent on reachable sequences.
The bit-7 form would read closer to the listing, but it is not needed for accuracy, and changing
`src/core` would need tier 1 review.

Rider 1's `ends_opponent_hints` is equivalent for the same reason. A computer opponent's voices
from 200 come only from `announce_combination` and the same landing sequence.

Not covered by a capture:
- characters 10, 11 and 13-15;
- character 9's voices 150 and 151;
- a full queue at a landing.
The listing argument above covers them.
