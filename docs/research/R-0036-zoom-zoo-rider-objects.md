# R-0036 — ZOOM ZOO rider objects and look animation

Status: **recovered and implemented on `codex/m4-16-rider-art`; unreviewed.**
Task: [M4-16](../../tasks/M4-16.md), answering visual-review defects D1-D3
(`tasks/M4-16-review.md` at `bfd41ed`). Presentation only: the serialized
`URZZ000B` state (742 bytes) is unchanged. DRAGSTER keeps its accepted
`presentation.rider.mike.race-tiles.v1` atlas and gates.

Identity: PAL ROM SHA-256
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, audited
bsnes core `e59bf88d...699a91b`. Addresses are the ones the code uses
(FastROM `$8x` for code; data banks as loaded).

## Summary

A rider is one 64x64 OBJ whose tiles are rebuilt every race update from its
pose index, optionally with an upper-body overlay frame. The previous renderer
drew five DRAGSTER pose pairs, always mirrored, and held the last pair for any
other pose. The original instead:

1. composes the object from a **pose frame** (a 5x6 tile-slot mask and tile
   references) plus an **overlay frame** chosen by a presentation-only look
   animation;
2. publishes OAM with **horizontal flip = the rider's reflected flag**, X/Y from
   position minus camera, and a row clip near the vertical screen edges;
3. shows in picture N the objects and OAM built in update N-1.

All three are recovered from code, validated against the original's own WRAM
(its upload queue, OAM shadow and look variables) on eight frozen timelines,
and implemented natively from packed ROM data.

## Recovered mechanism

### Object composition — `$83:F0FB` (JSL) / `$83:F0FF-$83:F2D9`

- Inputs: published poses `$0FF3/$0FF5` (native `pose.pose_index`), overlay
  poses `$0D45/$0D47` enabled by `$0D21/$0D23`, row clip `$0C89/$0C8B`.
- `$83:F2DA`: pose P reads three bytes at `$20:8000 + 3P`: frame address word
  and bank byte + `$23`. 5,171 poses; entry 5171 points at the all-`$FF` end
  marker `$26:E47C`. Frames occupy `$23:8000-$26:E47B`.
- Frame layout (`$83:F2FF-$83:F3F8`): a four-byte header of five six-bit row
  masks (rows 0-4; in each masked byte bits 7..2 are columns 1..6):
  row0 = b0&FC, row1 = (b0&3)<<6 | (b1&F0)>>2, row2 = (b1&0F)<<4 | (b2>>6)<<2,
  row3 = (b2&3F)<<2, row4 = b3&FC. Then one tile word per set bit, row-major.
- Tile word (`$83:F253-$83:F26B`): bank `$27 + (low&FC)>>2`, address
  `$8000 | (low&3)<<13 | high<<5` (1024 32-byte 4bpp tiles per bank; references
  reach `$27-$3F`; poses `$1432/$1433` reference an unmapped bank `$66`).
- Slot loop (`$83:F1F2-$83:F2C3`): rows 0-4, columns 1-6. An overlay bit takes
  the overlay's next word and consumes (skips) the pose word under it; otherwise
  a pose bit takes the pose's next word. A slot used last update but not now
  receives the all-zero `$27:8000` tile, so unused slots are transparent.
- Queue `$15AB` (bank), `$164F` (address), `$16F3` (VRAM word
  `$6000 + row*$100 + slot*$10`); the NMI loop at `$82:B8AA-$82:D195` adds
  `$800` for slots 8-15, placing the opponent (slots 9-14) at tile base `$88`.
- Clip (`$83:F311`, `$83:F412`, `$83:F3E9`): code 1 drops row 0 and skips its
  words; code 2 drops row 4. Both apply to pose and overlay.

### OAM — `$82:ACA8` / `$82:ACAC-$82:AE5E` (one-player race, `$0DE1 = 0`)

- Attribute bit 6 is cleared each update and set when `$0BA7`/`$0BA9` (native
  `pose.reflected`) is nonzero: `$26/$66` player, `$28/$68` opponent
  (palettes 3/4, OBJ priority 2). Tile bases stay 0/`$88` (`$82:B154` alternate
  layout inactive: `$77:0750` bit 3 clear).
- Y = `y - $0421`; hidden (X=Y=`$70`, ninth X bit set) unless `-41 <= Y < 225`
  by the original N-flag compares. X = `x - $041D`; scaled `X << $03F1` must lie
  in [`$0425`, `$0427`); X byte = `(X & $0D4F) & $FF`, ninth bit = sign of the
  scaled value. ZOOM ZOO uses the `$81:A445` set selected by decoded track
  header byte `$0D == $40`: `$03F1=2`, `$0425=$FF3C`, `$0427=$0400`,
  `$0D4F=$3FFF` (constant 1377-6724 on both primary timelines).
- Clip `$82:AD91-$82:ADAB`: Y < -32 → 1, Y >= 208 → 2. Opponent requires
  `$0C6D != 0`.
- Native `race.camera.x/y` equal `$041D/$0421` at every checked frame.

### Look animation — `$82:8337` / `$82:836D-$82:8926` (presentation-only)

Nothing in gameplay reads it, which is why the serialized race omits it.
Per rider: head `$0D49`, target `$1259`, looking-back `$1269`, distance step
`$126D`, glance timer `$1271`, sequence cursor/end/delay/target `$0D5F/$0D63/
$0D67/$125D` and sequence number `$0D6F` (opponent at +2).

- One rider per update: player when contact phase `$0300 != 0`, else opponent.
- Head points: rider position plus collision point 0 of its current pose
  (`$1265-$1268`, written by `$81:9FA6`; native `collision_points(...)[0]`).
- Other rider within ±256 rows and ahead in the facing direction: step through
  `$82:833B` distances, then `$82:834B` height steps, giving target 1..17.
  Behind: the glance timer runs to 180 (player) or 60 (opponent), looking back
  along `$82:835B` (targets 18..25 reflected, 34..41 otherwise), then rests for
  `(update_counter & $3F) + 1` updates.
- No target while `idle_pose.cycle_latched` (`$0D5B/$0D5D`, copied through
  `$0F7D`) is set: scripted glances from `$17:C606` ranges and `$17:C614`
  target/delay bytes (8-bit stores). Only the opponent's copy calls `$82:8927`
  to start a sequence. Both no-target paths store `$11` into the **player's**
  `$126D`.
- `$82:87C9-$82:8926` moves the head one step along arcs 1..17 (9 neutral,
  stored 0), 18..25, 26..33, 34.., joined at 9/18, 2/26, 7/34.
- Pause-diverted updates (pause selection set before or after the update) do
  not run it (pause-a frames 6000-6010).

### Overlay selection — `$83:EC8E-$83:ED76`

With the head from **before** the update's look step and the updated state's
`$0DE9` (native `pose.reflected_orientation`) and `$0DEF` (native
`reflection.pose_override`): none when the override is set or the head is 0;
orientation < `$10` → `$0AEC + 32(h-1) + o`; >= `$31` → `... + o - $21`;
`$10-$18` / `$28-$30` → `$83:EC2E[h-1] + (o - $10 | o - $1F) + $100C`;
`$19-$27` → none.

### Timing

Picture N shows update N-1's objects and OAM (colour-consistency scoring of
decoded objects against 225 recaptured primary frames, `wram-model/sweep.py`: 94.1% for N-1 against
58.9% for N and 58.7% for N-2; the remainder is overlay occlusion). This is
the same latency the HUD already models; the BG uses the prior camera.

## Content (Classic two-track pack v6)

Additive entries; all 50 v5 entries unchanged. Rules SHA-256
`c3db255d5cd50e25d2762d21aaac3370a827d967f8849100bd0f2a6453cdf4e7`; 54 entries.

| Entry | ROM file offsets | Bytes | SHA-256 |
| --- | --- | --- | --- |
| `presentation.rider.pose-pointers.v1` | `0x100000` | 15,513 | `089b5743...53d8a9` |
| `presentation.rider.pose-frames.v1` | `0x118000`, `0x120000`, `0x128000` (32,768 each), `0x130000` (25,724) | 124,028 | `f985fa7e...157c51` |
| `presentation.rider.object-tiles.v1` | 25 banks `0x138000-0x1FFFFF` | 819,200 | `a1e3f892...f8f55cb` |
| `presentation.rider.look-tables.v1` | `0x1033B` (48), `0x1EC2E` (96), `0xBC606` (14), `0xBC614` (436) | 594 | `3ea809fc...986029` |

Two fresh extractions from the user ROM produced byte-identical packs
(1,263,225 bytes, SHA-256 `adfa974b0c9d51031f7c87df8c79776fac77441482a34b7e0eb7b00c5c685aea`).

## Validation

Private evidence under `.worktrees/m4-16-rider-art/artifacts/m4-16-rider-art/`
(ignored). Original series: `.worktrees/m4-16-playable-zoom-zoo/artifacts/m4-16/
<timeline>-a/memory.wram` with matching `<timeline>-a-native.txt`.

1. **Original WRAM, model only** (`wram-model/allcheck.py`, output `allcheck.out`):
   for boundary, brake, loss, pause, start-brake, stop-brake, trick-left and
   trick-long over every race update from 1377 to the last update before result
   loading (6721-6772), the composed tile set equals the original upload queue
   (42,839 updates from 1378), the projection equals OAM entries 98/99 and the
   clip words (85,694 rider-updates), the look model equals `$0D49/$0D4B`, target,
   timer, distance, looking-back and delay words (42,847 updates) and the overlay
   selection equals `$0D45/$0D47` with their enables (85,694). Zero mismatches
   once pause-diverted updates skip the look step (without it pause-a failed 112).
2. **Native C++ look tracker from native states only** (`eval/look_eval`,
   `eval/look_compare.py`): per-update heads and overlays of both riders equal
   the original on all eight timelines, 85,694 checks, zero failures.
3. **Pictures**: `recapture.py` replays the frozen `reference.json` timelines on
   the audited core and asserts every frame's video and WRAM digest equals the
   frozen run (5,514 primary, 5,514 brake, 5,494 trick-long frames). Recaptured
   PNGs at 1450/1649/1700/2501/3208/4840/6484 are byte-identical to the frozen
   `visual-original-a` contract PNGs. `eval/rider_eval` and
   `eval/sweep_metrics.py` compare native renders (via the look tracker) with
   274 + 274 + 191 kept frames. See the task record for the measured counts.

## Limits and open points

- The single-state `zoom_zoo_presentation_runner PACK STATE OUT [PREVIOUS]`
  has no look history and draws pose frames without overlays; `--timeline`
  replays from the native initialization and is exact.
- `$82:857F`/`$82:87AD` clear the gameplay idle-cycle latch when a scripted
  glance sequence ends. Native movement does not model that write; no frozen
  timeline starts a sequence (`$0D6F/$0D71` stay 0), so it is unexercised and
  could diverge in live play.
- Other OBJs (countdown digits, start arrow and ring, off-screen arrows, hints,
  finish flags) remain the declared omissions; where they cover a rider the
  original pixel differs.
- Poses `$1432/$1433` and any index >= 5171 fail closed; the live frontend then
  holds that rider's last drawn pose and counts a fallback frame.
- `build_race_cgram` still switches the colour 96-111 cycle on DRAGSTER pose
  indices `$08D5`/`$04FE`, which ZOOM ZOO riders also use; not investigated here.

## Look state and the idle-cycle latch

Added 17 September 2026 at the coordinator's request: does gameplay depend on
the look animation? Branch `codex/m4-16-rider-art`, based on integrated
`8160392`.

### Writes

A linear disassembly of `$82:8337-$82:8926`, including `$82:87C9` and
`$82:8927`, finds two gameplay-visible stores. Everything else goes to look
words, scratch `$0200/$0202`, or direct page `$00-$06`.

- `$82:857F` stores 0 into `$0D5B`, the **player's** idle-cycle latch (native
  `riders[0].idle_pose.cycle_latched`). This happens when a player look update
  finds no target, `$0D5B` set, the delay `$0D67` has just reached 0, and the
  sequence cursor equals its end.
- `$82:87AD` stores 0 into `$0D5D`, the **opponent's** latch, under the same
  conditions using `$0D69/$0D61/$0D65`.

No other gameplay code reads the look words. The idle routine copies each latch
through `$0F7D` (`$82:8B13`/`$82:8E16` for the player, `$82:901B`/`$82:9304`
for the opponent). The race loop calls the look step last (`$83:CDA6`), after
`$8289C8` updates the riders, so a clear would be seen by the next update. The
only other effect of the latch in gameplay is whether the idle counter keeps
counting (`$82:A0F5`, `$82:A194-A1B2`); rider motion never reads it. The builder
copies the latches into `$0EF7`, and ROM code never reads `$0EF7`.

### Reachability in the one-player race

- **Player.** Only the opponent's copy calls `$82:8927`, so the player's
  sequence number `$0D6F`, cursor and end stay 0. The player's delay starts at
  0, and a clear needs 65,536 decrements. Every no-target update that is
  unlatched, or where the head is nonzero, resets it. Look updates run only in
  race updates. The 10:00 race clock (`$81:C73E-C75B`) then finishes both riders.
  About 205 countdown updates plus 30,000 clock updates plus the 240-update
  finish delay allow at most about 15,220 player look updates. That is fewer
  than 65,536, so the clear cannot happen.
- **Opponent.** A sequence lasts at least 144 opponent look updates (the
  smallest `$17:C614` delay sum). Its latch is only set while it idles. In every
  capture that happens only in the countdown: latched at 1472, released at 1551
  when the countdown brake starts at 100. Mid-race its AI always drives (horizontal
  0 or 2). After its finish the finish-pose override keeps idle off. That leaves
  at most about 40 opponent updates, so the clear cannot happen.

### Original evidence (ordinary controller timelines, cold start, captured twice)

Each case was captured twice from a cold start, and the two runs have identical
per-frame video, WRAM and SRAM digests. The capture tool
`zoom_zoo_playable_reference --case` gains `"idle": {"from", "frames"}`:
release every button from a frame for a span, or to the horizon, then resume
the primary controller stream.

| Case | Controller | Result | Look and latch facts |
| --- | --- | --- | --- |
| `idle-late-start` | neutral 1650-2649, then the primary shifted by 1000 | player finishes 7418, opponent 6488; loading 7659, visible 7767 | player latched 1472-2649; delay 65535 down to 65012; cleared by moving off (idle code), not by `$82:857F` |
| `idle-stop-timeout` | primary until 3240, then neutral to 32300 | player rolls to rest in the x≈12,618 basin; the clock limit finishes both at 31582; loading 31823, visible 31933 | player latched 1472-1649, 4448-4456 and 4544-31582; delay only down to 52542 (12,993 of 65,536); opponent latched 1472-1550 only; `$0D6F/$0D71` stay 0 |
| existing `start-brake` a/b | Y+Right held 1450-1590 | primary result | opponent sequence 1 starts at 1526 (cursor 82, end 128, delay 5); the countdown releases its latch at 1551 mid-sequence; the sequence is abandoned at 1596 (selector stays 1) |

The WRAM model (`wram-model/headsim.py`), checked in full against each update,
covers heads, delays, cursors, ends, selectors, sequence targets and glance
timers. It matches 6,282 updates on late-start, 30,446 on stop-timeout and
5,348 on each start-brake run. It matches every overlay, and it never predicts
a latch clear.

### Decision

Gameplay does not depend on look state in the one-player race, because neither
clear can happen within the clock limit. The serialized state keeps
`URZZ000B`; no look words are added.

The same captures exposed a real gameplay gap. Native ignored the 10:00 limit.
It now sets both finish flags when the clock caps, and restore validation
accepts a finish with laps still remaining only when both riders are finished
at the held 9:59.9 clock, with the no-time total. Two limits apply:

- The limit is what bounds the player's look updates, so the latch argument
  needs the fix. Without it native would race on past 10:00.
- The time-out result loads two updates later in the original. Traced loading
  (`artifacts/m4-16-idle/trace-*`) spends updates 5-7 in the `$82:8088-809A`
  SPC700 reset wait (`CMP $2140` against `$BBAA`): 7,122 loop iterations against
  292 in the primary. The graph extrema are published at load update 108 instead
  of 106, totals at 109 instead of 107, and the picture becomes visible at
  loading+110. Audio is excluded, so that case stays an incomplete inventory.
  Native matches it from 1376 through the finish, the finish delay and loading
  to 31927, then publishes two updates early.

The opponent argument rests on the recovered AI. Mid-race its horizontal input
is always 0 or 2, and `update_zoom_throttle` then sets the throttle target
whenever displacement is below 4. The case not ruled out by code is a rider on
a ±31 wall tile with no horizontal displacement. It is not observed: across
nine timelines and the 30,000-update idle run, the opponent's idle counter
never leaves 0 after 1650.

Gates on `51c05f6` (pack v7): primary 6225 states with 757 restores and the
fresh restart on app-debug and app-sanitize; idle-late-start 6725 states with
801 restores and restart on both; M4-15 matrix 8/8; 30 existing
original captures (24 complete, 6 incomplete) match every native row; four
presets at 406/406; DRAGSTER winner and loser presentation unchanged; time-out
case 13/13 fresh-process restores (app-sanitize) through 31927.
