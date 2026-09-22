# R-0044 - The centred HUD fields: crossing times and signed splits

Status: recovered and implemented on `task/classic-split-time`
(CLASSIC-SPLIT-TIME), 22 September 2026. PAL ROM
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, audited
bsnes core `e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b`.
Extends [R-0043](R-0043-classic-race-hud.md), which recovered the HUD's layer,
font, colour and redraw queue and left the two centred fields drawn only at
the finish.

## What the fields show

The two seven-cell fields at columns 13-19 of BG3 rows 5-6 (the player) and
rows 20-21 (the opponent) are one mechanism with two layouts. Each rider's
field is requested on every crossing of that rider but the initial start-line
one, and blanked 118 updates later:

| Crossing | Layout | Content | Example |
| --- | --- | --- | --- |
| a lap or the finish (`$119F,y` = 0) | `M:SS:th` | the clock as the lap routine stored it at the crossing, hundredths included (`$0E43,y`, `$0E47,y`, `$0E4B,y`, `$0E4F,y`, `$0E3F,y`; native `time_digits`) | `0:32:50` on the M4-16 primary's update 3208, `1:38:02` at its finish |
| a checkpoint the other rider has already passed (`$119F,y` = 1-3, slot seen) | `sM:SS:t` | the clock as this update read it minus the clock the first rider through stored, digit by digit; `s` is `+` for the player and always `-` for the opponent | `+0:01:3` on update 2077, `-0:00:1` on 3881 |
| a checkpoint first (slot unseen) | nothing | the slot is marked seen and this rider's clock stored; the request is cleared. The opponent's cells are blanked at once | 2008 (opponent), 3628 (player) |

The blank 118 updates after a crossing is skipped for a rider who has
finished, which is why the finish times stand.

## The mechanism, from the ROM

**Requests.** `$81:C910-CB23` runs once per update for each rider (Y = 0, 2
through `$0260`), after the lap routine `$81:8050-82B6` and before the clock
ticks. It reads the rider's display countdown `$0FFF,y`, which the lap routine
sets to 120 (`$81:8251`, `$81:8288`) on every accepted crossing but the initial
one and `$81:86EF` decrements at the start of each update:

- countdown 120: `STZ $119D` (the sign flag), `$0349,y` = 1 (a draw request
  in the redraw queue of R-0043), then by `$119F,y`, the mode the lap routine
  set (0 at the start line, the checkpoint number otherwise):
  - mode 0 (`$81:CAAD`): the five crossing digits are copied through the
    character table `$80:81F4` into `$11BB,y` (minutes), `$11AB,y` (tens),
    `$11AF,y` (seconds), `$11B3,y` (tenths), `$11B7,y` (hundredths), and
    `$0F0B,y` is set.
  - otherwise, with X = `$0EFB,y` * 4 + `$119F,y` (laps remaining times four
    plus the checkpoint - a slot shared by both riders):
    - `$114D,x` negative, the slot unseen (`$81:CA38`): clear the flag, write
      the digit `0` to the four digit bytes and `+` (`$80:821F`, tile `$4C`) to
      `$11BB,y`, **clear `$0349,y`** (so nothing is drawn), and if `$0C6D` is
      set and Y is the opponent, set `$1001` = 2 and `$034B` = -1 (the
      opponent's field is blanked on this very update). Then, for either
      rider, store the clock digits `$0E19/$0E1D/$0E21/$0E25` (minutes, tens,
      seconds, tenths) in `$100D + 16 * laps + 4 * checkpoint`.
    - slot seen (`$81:C94F`): subtract the stored digits from the clock's,
      tenths first, each borrow added to the stored digit above (`ADC #$0A`,
      `ADC #$0A`, `ADC #$06`); a negative minute takes `EOR #$FF` and sets
      `$119D`. Non-negative: the four digits and `+`. Negative
      (`$81:C9F4-CA33`): tens become `5 - T`, seconds `9 - s`, tenths
      `10 - t` with ten shown as zero, and `-` (`$80:8220`, tile `$4D`). The
      slot is not rewritten.
- countdown 2: `$0349,y` = -1, a blank request.

**Writers.** The queue services one field per update in the order of R-0043:
left field, clock, player's field (`$81:EDDC`), opponent's (`$81:F033`), then
the caption. A positive request with `$119F,y` zero takes the finish layout
(`$81:EE89-EF4F`: cells 13, 15, 16, 18, 19 from the digit bytes, `:` at 14
and 17); with `$119F,y` set, the split layout (`$81:EF5E-F030`: cells 13, 14,
16, 17, 19, `:` at 15 and 18). The opponent's split writer `$81:F1B5-F288`
takes its first cell from the **constant** `$80:8220` (`-`), never from
`$11BD`, so the opponent's field reads `-` whatever the sign; the player's
reads `$11BB`. A negative request blanks the seven cells with the space tile
unless `$0EFF,y` (the rider has finished) is set, in which case the handler
jumps on to the next field without clearing the flag or spending the update
(`$81:EDE9`, `$81:F040`).

**Which clock.** The split reads the clock digits before the update's own
tick: on the primary's update 2077 the clock steps 0:09.8 to 0:09.9, the
stored slot is 0:08.5, and the original publishes `+0:01:3`. Checked against
every capture's WRAM with `split_probe.py` (below): all 509 splits match the
previous update's clock and only 448 the current one; all 260 first-seen
stores hold the previous update's clock and only 208 the current one.

**The engine already runs the rest.** Native's `update_zoom_checkpoint`
(R-0034) keeps the countdown (`checkpoint_display_countdown`, 120, cut to 2
for the opponent's first-seen slot), the mode (`checkpoint`), the crossing
digits (`time_digits`), the next checkpoint and the shared first-seen flags
(`checkpoint_seen`, 20 bytes at `$114D`). What the 742-byte state does not
carry is the slot store `$100D`: the clock the first rider through each slot
stored. It is presentation history, kept by `ClassicRaceHudClock` from the
update on which it observes the first crossing of a slot; a queue that did
not observe that crossing leaves the cells alone.

## Native

`ClassicRaceHudClock` (R-0043) now carries a request per field. A crossing is
recognised by the rider's next-checkpoint step with a nonzero countdown -
the countdown alone cannot name the opponent's first-seen crossing, which the
engine sets to 120 and cuts to 2 within one update. A mode-0 crossing requests
`classic_hud_crossing_text(time_digits)`; a seen checkpoint requests
`classic_hud_split_text(previous update's clock, stored slot)`, with the first
cell forced to `-` for the opponent; a first-seen checkpoint stores the clock
and requests nothing. A countdown of 2 requests a blank. Each update the queue
services the first pending field in the dispatcher's order; a blank for a
finished rider is dropped without spending the update. The renderer draws the
cells' text through `draw_bg3_text` at columns 13 of rows 5 and 20, with `+`
added to the caption glyph map as `$4C`.

## Measurement

`hud_compare.py` (R-0043) scores native renders against the original's own
pictures in four boxes: the HUD rows (y 15-30), the rows the authored bar once
covered (y 0-14), and the two centred bands (x 104-159, y 39-54 and y 159-174).
Numbers are differing pixels; "before" is the CLASSIC-RACE-HUD tip `0ead6d3`.

| Pictures | Frames | player band before -> after | opponent band before -> after | HUD rows | whole picture |
| --- | --- | --- | --- | --- | --- |
| M4-16 primary, kept every 20 | 274 | 6,347 -> TBM | 9,880 -> TBM | 0 | 18,601 -> TBM |
| brake (loser) race, kept | 274 | TBM | TBM | 0 | TBM |
| trick-long, kept | 192 | TBM | TBM | 0 | TBM |
| primary consecutive 1670-1690, 3200-3215, 4835-4850, 6475-6500 | 79 | TBM | TBM | 0 | TBM |
| primary consecutive 2004-2012 (opponent first, cut), 2075-2082 (split with a tick), 2193-2198 (blank), 3873-3886 (player first, opponent's `-0:00:1`), 4631-4642, 6275-6285 (first with a tick) | 60 | TBM | TBM | 0 | TBM |
| compound-reverse-a lap change 3205-3222 (both riders' lap times) | 18 | TBM | TBM | 0 | TBM |
| compound-reverse-a finish 6465-6500 | 36 | TBM | TBM | 0 | TBM |
| down-a crossing 3200-3216 | 17 | TBM | TBM | 0 | TBM |
| DRAGSTER primary-a consecutive 2378-2392 (checkpoint 2: player first, opponent's split), 2496-2502 (blank), 3205-3216 (finish) | 34 | TBM | TBM | TBM | TBM |

Before the opponent's constant `-` was applied, every kept primary frame on
which the opponent's split shows differed by exactly 14 pixels, the
difference between the `+` and `-` glyphs; that measurement is what found the
constant.

**WRAM check of the rule** (`split_probe.py` over 77 captures with whole WRAM
per frame: M4-16, idle, opposing-input and DRAGSTER originals): 1,145 crossing
requests; 509 splits, all matching the rule from the previous update's clock,
none negative; 260 first-seen stores; 376 mode-0 displays, all equal to the
crossing digits; 1,145 blanks, all at countdown 2 (the finished riders'
skipped). DRAGSTER runs the same mechanism with one checkpoint (2) and its
finish.

## Domain and limits

- Measured on both tracks with the ROM and core above. DRAGSTER's single
  checkpoint is crossed by the player first in every original; no DRAGSTER
  original shows the player's own split.
- **The negative split path is implemented as read, not measured.** With one
  shared clock and the first rider through a slot storing the earlier time, a
  later rider's difference cannot be negative, and no capture shows one
  (`negative: 0`). In that path the original's `$81:CA13` stores the seconds
  digit to the absolute `$11B3`, the player's byte, for either rider; native
  does not reproduce that misplaced store.
- A split whose minute digit would exceed nine (a difference of ten minutes
  or more) would index a letter in the original's character table; native
  refuses it, and the 10:00 race limit keeps it out of reach.
- `$0C6D` (the AI mode, held at 1 in this product's reference guard) gates the
  opponent's first-seen cut; native applies the cut unconditionally, as its
  engine already did.
- The slot store is presentation history: a frozen scene or a restore that
  begins after the first crossing of a slot cannot show that slot's split. The
  finish sequence and the mode-0 displays need no history.
- Still declared: the off-screen rider arrow, which accounts for the
  whole-picture residual outside the bands.

## Scripts

Under `local/evidence/classic-split-time/classic-split-time/` after
integration: `split_probe.py` (the WRAM check), `hud_compare.py` and
`recapture.py` (R-0043's, unchanged), `split-probe.log`, `compare-*.txt`,
`orig-primary-splits/` and `orig-dragster-splits/` (the consecutive originals,
asserted against the frozen digests), `gates.sh` and `gates-<commit>/`.
