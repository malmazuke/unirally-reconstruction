# R-0040 — DRAGSTER countdown and winner window effects

Status: **verified finding; native implementation in
[DRAGSTER-WINDOW-EFFECTS](../../tasks/DRAGSTER-WINDOW-EFFECTS.md)**, with one
named residual (the opponent-won banner beyond its 120-update counter).

ROM SHA-256 `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`;
audited bsnes core `e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b`.

## Question

[R-0015](R-0015-native-presentation.md) ("Additive window-effect prerequisite
freeze") froze two channel-6 window HDMA tables, `$15:8A89` from frame 1600 and
`$15:BF36` from frame 3453, as `presentation.effect.go-window.v1` and
`presentation.effect.winner-window.v1`, and drew each when the rider pose pair
equalled a hard-coded value. [R-0037](R-0037-dragster-race-palette-cycle.md)
gave those shapes the right fill colour but recorded that "when the windows
appear and their shape" was not established: natively the GO window appeared
only at frame 1600, and at 3322 the original's winner shape covered 2,810
pixels against native's fixed 5,001-pixel table. When does the original enable
these effects, and what table does each frame use?

## The mechanism

All addresses are LoROM: file offset = `(bank & $7F) * $8000 + (address - $8000)`.

**One table pointer per frame.** The race vblank routine reads a 24-bit pointer
from work RAM and writes it to channel 6:

| Site | Instruction | Effect |
| --- | --- | --- |
| `$80:868E` | `LDA $11FD` (16-bit) | the frame's window table address |
| `$80:8691` | `STA $4362` | channel-6 `A1T6L`/`A1T6H` |
| `$80:8696` | `LDA $11FF` (8-bit) | its bank |
| `$80:8699` | `STA $4364` | channel-6 `A1B6` |
| `$80:86E9`/`$80:8786` | `LDA $11FD; CMP #$DB4E` | already the sentinel? |
| `$80:86F3`/`$80:8790` | `LDA #$BE` / `#$3C` | enable mask **without** channel 6 |
| `$80:86FA`/`$80:8796` | `LDA #$DB4E; STA $11FD` | reset the request |
| `$80:8702`/`$80:879E` | `LDA #$FE` / `#$7C` | enable mask **with** channel 6 |
| `$80:87A9` | `STA $420C` | HDMA enable |

`$82:D572` sets the channel's `DMAP=$04` (mode 4: `$2126-$2129`, WH0..WH3) and
`BBAD=$26`, as R-0015 recorded. So **channel 6 is on for exactly the frames in
which the game logic wrote a table pointer to `$00:11FD`/`$00:11FF`**; the
vblank consumes the request and resets it to the sentinel `$DB4E`. The setup
runs at the start of the frame, so the table a frame displays is the one the
*previous* frame's logic chose. `$80:8691` does not execute at all before the
race vblank starts, DRAGSTER initialization frame 1328 plus 6.

**The table family.** Every writer of `$11FD` in the ROM is in bank `$83`
(`$83:E5AC`, `E601`, `E61E`, `E653`, `E670`, `E6A5`, `E6D2`, `E730`, `E746`,
`E770`, `EA58`, `EA6F`, `EBDD`, `EC01`, `EC10`, `EC24`, besides the vblank's own
`$80:86FD`/`$80:8799`), and each one loads `$83:E55C,X` and adds `$8000` with
bank `$15`. `$83:E55C` (file offset 124252) holds 16-bit offsets in an exact
arithmetic sequence of stride 899: `$0000, $0383, $0706, ... , $6CDD`. The first
**25** of those (`$15:8000`-`$15:D7CA`, file offsets 688128-710602, 22,475
bytes) are well-formed HDMA tables: two repeat-mode runs of 127 and 97 lines,
224 four-byte rows of WH0/WH1/WH2/WH3, 898 bytes, then a `$00` run terminator.
Entries 25-31 are not; the code paths below never select them, and `$DB4E`
(which would be entry 26) is only the "nothing requested" sentinel. Members 3
and 18 of the family are byte for byte the two entries R-0015 froze:
`33f19daed02ec2f9...` and `b6fddc697a55984d...`.

**The countdown driver `$83:E59C`,** called from `$83:CD6B` only while
`$11C5 != 0` (`$83:CD66`). `$82:D841` sets `$11C5 = 270` at race initialization
and `$83:E789`/`E73D`/`E753` decrement it once per frame. `$83:E59E` requires
`$0FF1 >= 5`, then dispatches on `$11C5`:

| `$11C5` | Selected member | Site |
| --- | --- | --- |
| `>= 250` | 6 | `$83:E60D-E627` |
| 221-249 | 0 | `$83:E5FE-E60A` |
| 190-220 | 6 | `$83:E65F-E679` |
| 161-189 | 1 | `$83:E650-E65C` |
| 130-160 | 6 | `$83:E6C1-E6DB` |
| 101-129 | 2 | `$83:E6A2-E6AE` |
| 70-100 | 6 | `$83:E759-E782` |
| 1-69 | 3 when `$0300 != 0`, else 4 | `$83:E728-E756` |

The transition member is `5 + $1229` (`$83:E611`, `E663`, `E6C5`, `E763`);
`$83:CC08` loads `$1229` from `$0BA7`, which is 1 in every captured DRAGSTER
race, so every transition draws member 6. `$0300` is toggled every frame by
`$83:CCED` (`LDA $0300; INC; AND #$01; STA $0300`), so the GO letters alternate
between two members on the frame parity. The same dispatch also holds the riders
at the line: only the "1-69" path skips the block at `$83:E78C-E7BD` that keeps
`$0325`/`$0327` set.

**The winner-banner drivers `$83:EA19-$83:EA5B` (player) and `$83:EBB0-$83:EC13`
(opponent)** are structurally identical and differ only in their counters
(`$0F03`/`$0F07` and `$0F05`/`$0F09`). On the driver's first update the index
counter is zero, so it sets the life counter to `$0168` (360) and takes member
7; afterwards each update decrements the life counter and, **only when `$0300`
is set**, advances the member, wrapping 25 back to 7 (`$83:EA42-EA4B`). The
member is therefore held for two frames at a time and cycles through 7..24, a
36-frame loop. When the life counter reaches zero the driver stops requesting a
table (`$83:EA5D`/`$83:EBCB`) and channel 6 goes off. Only the winner's driver
runs: the opponent's counters are untouched in a player-won race and the
player's in an opponent-won one.

## Evidence

Four fresh access captures of original DRAGSTER on the audited core, each
watching `$80:8691` (which logs `A` = the frame's table address) and, for the
first pair, `$00:11FD`, `$00:11FF`, `$00:11C5`, `$00:1229`, `$00:0F01`-`$00:0F09`
and `$00:0300`. Ignored under `artifacts/dragster-window-effects/`.

| Capture | Scenario | Frames | `access.json` SHA-256 |
| --- | --- | --- | --- |
| `win-a` | `race-crawler-dragster-12000-continuous-right-fields` | 1200-3700 | `67dc78cce01021aa...` |
| `win-b` | the same, fresh process | 1200-3700 | `67dc78cce01021aa...` (byte-identical) |
| `lose-a` | `race-crawler-dragster-12000-release-3000-3299-fields` | 1200-3860 | `c7bef6574e5e06bb...` |
| `rel3213` | `race-crawler-dragster-12000-review-release-3213-fields` | 1300-3700 | `0732500347b06931...` |

The observed selections, identical in all three scenarios for the countdown:

- nothing before frame 1334; member 6 at 1334-1354; 0 at 1355-1383; 6 at
  1384-1414; 1 at 1415-1443; 6 at 1444-1474; 2 at 1475-1503; 6 at 1504-1534;
  then 4 and 3 alternating from 1535 (4 on odd frames) through 1603; nothing
  from 1604. `$11C5` reads 270 at frame 1333 and one less each frame, which
  predicts every boundary above.
- the banner: `win-a` finishes the player at 3213 and shows member 7 from frame
  3215; `rel3213` is the same; `lose-a` finishes the opponent at 3214 and shows
  member 8 from 3216 (its first driver update falls on a set `$0300`, so the
  member advances before it is first displayed). In each the member advances
  every second frame and wraps 24 to 7.

`$11FD` is only ever `$DB4E` between 1604 and the banner, so **the original
draws no window at all during the body of the race**.

## Native implementation

`dragster_window_table_index` in `src/core/presentation.cpp` returns the member
a state's frame draws, and `dragster_window_table` takes its 898 scanline bytes
out of the frozen family. `render_dragster` draws members 0-6 where the GO
window was drawn (before the riders) and members 7-24 where the winner window
was (after them), in the cycled colour 0 of R-0037. The pose-pair gate is gone
whenever the pack carries the family.

The rule, for a state whose `frame` is `n` and whose race began at
initialization frame `i` (1328 for DRAGSTER):

- the countdown's `$11C5` for frame `n` is `270 - (n - (i + 6))`, and the table
  is chosen from the dispatch table above; outside 1..270 there is none;
- the banner's first driver update is the one after `record_finish`, so with
  `u` updates since that finish the member is
  `7 + ((n / 2 - (n + 1 - u) / 2) mod 18)` for `2 <= u <= 361`, integer
  division standing in for the `$0300` parity;
- `u` is `player_finish_delay` when the player won, and `120 -
  finish_animation_countdown[1]` when the opponent did;
- during result loading the frame used is the loading-start frame, because the
  race vblank runs for the last time on loading update 1, exactly as
  `apply_dragster_palette_cycle` freezes the palette phase. The review added the
  part that makes this right rather than merely declared: `$420C` is never
  rewritten after 3454, so the original keeps showing the member chosen on that
  last vblank (member 19 in the primary capture) through the fade.

**Content.** `presentation.effect.classic.window-tables.v1`, 22,475 bytes at
file offset 688128, SHA-256
`238ff3fc38357b359e99ae3da0647fdf61cb4a6a3680b83faa1b3a8a6a4a2cf0`, enters the
two-track pack through the existing extraction rules. The profile becomes
`classic.pal.crawler.two-tracks.v8` with 56 entries and rules SHA-256
`f1e9c2580ef0fe26213068ed3021bc326981b55ae2f6f533307577689446778b`. Every v7
entry is unchanged. DRAGSTER v1 packs do not carry the family and keep the
accepted pose-keyed placement, so the accepted v1 contracts and the historical
matrix are untouched.

## Measurements

**Against the original's own pointer.** Rendering is not needed to score the
rule: `dragster_window_table_index` was evaluated on all 2,147 native states of
the release-3213 sweep (frames 1533-3679, from the DRAGSTER-PALETTE-CYCLE review
artifacts) and compared with the member `$80:8691` published in the `rel3213`
capture at the same frame. **1,922 of 1,922 frames from 1533 to 3454 agree**,
including every frame with no window; frame 3679 agrees that there is none. The
224 differences are result-loading updates 2 and later, where the original's
race vblank no longer runs and native holds update 1's member, which the
review confirmed is what the original does, since `$420C` is untouched from
3455 and channel 6 stays enabled with that table. The
same rule, given each capture's own winner-finish frame, reproduces `win-a`
(241 frames) and `lose-a` (345 frames) with no disagreement.

**Against original pictures.** 132 frames that have both a native state and a
recaptured original were rendered with the previous implementation (pack v7 and
a `live_presentation_runner` built from the parent commit) and with this one
(pack v8), and scored over the frozen rectangle (0, 28, 256, 196)
(`artifacts/dragster-window-effects/window_compare.py`, `window-compare.json`):

| Segment | Frames | Pixels the two draw differently | matching the original before | after |
| --- | --- | --- | --- | --- |
| countdown transition 1510-1534 | 2 | 18,556 | 0 | **18,556** |
| GO 1535-1603 | 44 | 384,328 | 0 | **384,328** |
| winner banner 3214-3453 | 46 | 164,753 | 0 | **164,276** |
| result loading 3454-3677 | 14 | 36,414 | 14,080 | 17,374 |
| all 132 | 132 | 604,051 | 70,885 | **584,534** |

Rectangle mismatches over the same 132 frames fall from 1,327,728 to 757,274 of
6,623,232 pixels; 100 frames improve, 30 are unchanged and 2 (3600 and 3677, by
13 and 17 pixels) are marginally worse, both inside the already declared
difference that native draws the race while the original fades in the result
screen. Frames the old code left blank now match exactly: the whole GO segment,
and 2,810 of 2,810 pixels at 3215 and 4,034 of 4,034 at 3450. The accepted
contract frames are untouched: at 3452 and 3453 both implementations draw
member 18 and the rectangle mismatch stays at 653.

## Established later: the opponent-won banner over its full length

CLASSIC-PRESENTATION-UNIFICATION moved the selection onto the shared race
state (`classic_window_table_index`). When the player won, `race.finish_delay`
counts the whole banner as `player_finish_delay` did. When the opponent won,
its finish frame is presentation history kept by `ClassicRaceHistoryTracker`
(the frame on which `race.riders[1].finished` became set), like the rider look
state; a single restored state without history recovers it once the player has
also finished, from the two finish times: `finish_centiseconds` advances two
per frame plus the frame parity, so `total[0] - total[1] = 2(fa - fb) + (fa &
1) - (fb & 1)` has exactly one solution for `fb` given the player's finish
frame `fa = frame - finish_delay` (during result loading, the delay last
advanced on the frame before loading started). The relation holds on all four
original races with both finishes (continuous-Right, primary-a, reversal-a,
random-1-a).

Checked against `lose-a` (opponent 3214, player 3318, loading 3559) with the
runner's `--window-index` mode: with the history the member equals the
original's `$80:868E` read on **2,226 of 2,226 frames from 1334 to 3559**,
including the 226 banner frames beyond the legacy 120-update counter; without
history the derivation agrees on every frame from the player's finish through
loading and selects nothing on the 102 frames before it. The read on frame n is
the table frame n shows (the driver of n-1 chose it); an earlier working note
in the unification task had the alignment one frame off.

## Not established

- **Whether `$1229` is ever anything but 1**, which would move the transition
  member off 6. `$0BA7` was 1 in every captured DRAGSTER race; what sets it was
  not examined.
- **`$0FF1`**, the `>= 5` gate at `$83:E59E`, and `$77:0750`/`$77:074B`, which
  the drivers read for the two-player and stop paths. One-player DRAGSTER always
  took the paths above.
- **ZOOM ZOO's own window content.** The mechanism is shared: the drivers,
  the pointer and the family are not DRAGSTER-specific, and ZOOM ZOO's recorded
  omissions (start ring, GO, hint text, finish banner) are the same channel-6
  effect. Which members its countdown and banner select, and whether its other
  captions use this path at all, was not captured. The unified renderer binds
  no window family for ZOOM ZOO until that is measured.
- **Whether `$1229` is ever anything but 1**, which would move the transition
  member off 6. `$0BA7` was 1 in every captured DRAGSTER race; what sets it was
  not examined.
- **`$0FF1`**, the `>= 5` gate at `$83:E59E`, and `$77:0750`/`$77:074B`, which
  the drivers read for the two-player and stop paths. One-player DRAGSTER always
  took the paths above.
- **ZOOM ZOO's own window content.** The mechanism is shared: the drivers,
  the pointer and the family are not DRAGSTER-specific, and ZOOM ZOO's recorded
  omissions (start ring, GO, hint text, finish banner) are the same channel-6
  effect. Which members its countdown and banner select, and whether its other
  captions use this path at all, was not captured; it is a follow-up, not part
  of this task.

## Bounds that are defensive, not observed

`winner_window_frames` is 360, from `$0F07`'s `$0168`, but `deserialize_zoom_zoo`
rejects a `player_finish_delay` above 240, so no restorable state reaches the
bound, and for the parity the captured race takes the ROM driver leaves a frame
earlier than 360 would. The presentation test therefore asserts only reachable
states (review finding C). `frame + 1 - since` can wrap for hand-made states
with tiny frame numbers; it is defined, bounds-checked and unreachable for real
states (finding D).
