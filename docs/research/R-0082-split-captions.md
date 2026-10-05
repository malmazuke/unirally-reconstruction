# R-0082 - Rider 1's tutorial hints and the split HUD's field chain

Status: implemented on `task/split-captions` ([SPLIT-CAPTIONS](../../tasks/SPLIT-CAPTIONS.md)),
5 October 2026. PAL ROM SHA-256
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, audited bsnes core
`7d5aa1e656b9171524d01b1b22917197d8121cb4`. It closes the caption differences that
[R-0080](R-0080-split-hud.md) left in two-human split races.

Sources (paths are under main `local/evidence/`):
- The listing reading: `split-captions/listing-report.md`.
- The captures, all with every frame's work RAM:
  - R-0079's: `split-pause-menu/`, and the review captures in `split-pause-menu/review/`;
  - the SPLIT-HUD-GAPS review's `zzap`: `split-hud-gaps/review/`;
  - R-0081's, and the review captures in `split-race-end/review/`.

Tags: **[L]** listing only, **[C]** confirmed in a capture.

## What the original does

### Rider 1's tutorial hints

- **Setup.** `$82:D930-D96F` gives each rider its own hints, from its own bit in the tutorial
  mask `$77:1116`. The player's are `$12E3`; rider 1's are `$12E5`, from `$12E9 = 1 << $77:0749`.
  - A computer opponent (character 16 and up) never has hints.
  - Neither does a rider 1 who is MIKE. `$82:D93E-D94C` skips the store of `$12E9`: the branch at
    `$82:D943` jumps past it. [L]
- **Groups.** `$83:CE43-CEAF` runs after the player's block `$83:CDBC-CE2A`, with its own counter
  (`$12ED`, from 0) and group (`$12F1`).
  - Every 300 updates it queues hints 40+4g … 43+4g into rider 1's queue (`$0CEB`, cursors
    `$0D11`/`$0D13`).
  - Rider 1's groups therefore come 30 updates after the player's, whose counter starts at 30.
  - [C] `twop-plain`: queued on 2278, 2578 and 2878; taken on 2279, 2339, 2399 and later frames.
- **Hold.** A take holds 120 while the hints run (`$81:C0A5-C0CA`), as the player's do.
- **End.** Queueing an event ends the hints when bit 7 of the byte `event - 22` is set: scoring
  events and voices from 150 (`$81:C5D5-C5E1`). It zeroes the hold and clears `$12E5`. [C:
  `zzap` 3452]
- **Write-back.** `$83:CEB1-CEC5` writes rider 1's bit into the cartridge in every update while
  its hints are off, outside the demo. [C: `zzap`, from 3452]
- **The split demo** runs the same code. Its rider 1's first scoring event (frame 1740) comes
  before its first group would (1748), and the demo writes nothing back. [C]

### The split HUD's field chain

The split NMI `$81:D853` services one field a frame. Order, with each field's request flag:

| # | Field | Request flag | Notes |
| --- | --- | --- | --- |
| 1 | Left field (both views) | `$0D17` | Always cleared |
| 2 | Top clock | `$034D` | |
| 3 | Bottom clock | `$034F` | See below |
| 4 | Top cells | `$0349` | |
| 5 | Bottom cells | `$034B` | |
| 6 | Stunt score | `$12C9` / `$12CB` | Stunt events only |
| 7 | Top caption | `$0EE7` | |
| 8 | Rider 1's caption | `$0EE9` | Last |

- **Bottom clock.** `$81:C6D1-C6DC` asks for it the update after the top clock's tick, while
  rider 1 is unfinished. Rider 1's last crossing asks for its blank.
- **Rider 1's caption.** Its consumer `$81:BEF1-BF31` takes an event into `$0EC7`
  (`$81:C05C-C0A0`). On its first dry look after a take (`$11C3`), it copies the blank message
  (`$81:BFB9-BFD0`).
- **Check.** The listing reader's predictor matched every serviced NMI of fourteen captures
  (10,326) [C].
- **Bottom cells' sign.** The chain's cell writer `$81:E49F-E6F3` reads the sign `$11BD` in both
  layouts, so rider 1's split shows its own sign. The one-player writer always shows a minus.
  [C: `zzap` 7155]

### CONTINUE in a split race

- `$83:F915-F92C` zeroes 129 words from the row where the menu last opened (`$130F`). Pad 1's menu
  opens at 0x18A8, pad 2's at 0x1A68.
- The clear blanks that view's caption until its next upload, and its side arrow until the NMI
  redraws it.
- It repeats while Start is held.
- [C: `leaguefin` 9050-9053; `league` 7540-7578 and 7640-7652]

### The HUD over the riders

- `$80:D241` makes the objects the sub screen (`$212D` = 0x10) on the way into a race from the
  menus. So the split HUD's ink is CGRAM 27 plus or minus whatever object lies under it, as the
  one-player HUD is.
- The idle demo's title writes `$212D` = 0 (`$80:942D`), so the demo's ink stays flat. [C: the
  register writes; the pixels in `zzap` and the demo]

### Pause messages

- `$83:F752` loads `$80:8224` as a word, so `"` becomes tile 0x261, past the font, and shows
  nothing. [C: `league`, rider 9's "mellowin"]
- `!` (`$80:8223`, tile 0x160) works the same way but is not captured.

## Native

- **Race state.** `ZoomZooState::opponent_hints` (`$12E5`, `$12ED`, `$12F1`) replaces both the
  demo-only flag and the league-only "hints over".
  - `classic_local_race_scenario` takes rider 1's bit, and `update_tutorial_hints` runs both
    riders' groups.
  - The race update ends rider 1's hints as an ending event is queued
    (`announcement::ends_opponent_hints`).
  - `update_opponent_announcements` holds a take for 120 while the hints run.
  - The front end writes rider 1's bit back in every two-human mode.
- **Serialization.** The flag keeps the bytes the two old fields had: the split trailer's, and the
  league wrapper's last (as "over").
  - The counter and group are derived on reading from the race clock while the hints run, as the
    player's are checked (`check_hint_timeline`).
  - The reader reads the flag before the base layout's checks (`TrailerFlags`), so the opponent
    queue's hold may be 120. It refuses hints for a rider 1 who is MIKE, or for a computer.
- **The HUD.** `ClassicRaceHudClock` runs the split chain.
  - Its pieces are `request_split_fields`, `service_split_clock` and `service_cells`, and rider
    1's caption is serviced last.
  - It publishes the lower clock and rider 1's caption. `draw_split_hud` draws them, from the
    state only without history.
  - `clear_after_pause` follows the split clear from the view the menu last opened in.
- **The ink.** A split race with `demo_ai` clear draws its HUD before the riders, into the colour
  math's mask.
- **The sign.** `crossing_cell` keeps the minus for one view only.
- **Pause messages.** A message's `"` draws nothing.

## Evidence

- **Words.** `split_compare.py --opponent-queue` compares rider 1's whole queue (`$0CEB-$0D13`,
  `$0CA7`) as well as the earlier words. It finds 0 differences on every race frame of all
  eighteen split captures; main differs on `twop-plain`'s rider 1 queue on 622 frames.
- **Restores.** The following are byte-identical to the end: `twop-plain` 2290, 2600 and 2850;
  `league` 7400; `zzap` 3000, 3500 and 5600. The runner's two-pad restore check now takes ZOOM
  ZOO's layouts F and G too.
- **Pictures.** `measure.py` (main `local/evidence/split-captions/`) compares pictures, the
  candidate against main 6d8d823. No capture has a picture worse than main.
  - Equal pictures now:
    - `twop-plain`: 530 of 530 (main 238);
    - `league`: 710 of 710 (main 410);
    - `leaguefin`: 120 of 121 (main 60);
    - `zzap`: 6,064 of 6,074 (main 5,093);
    - `twop` and `vs`: 658 of 1,010 (main 477);
    - `zz2p`: 1,014 of 1,280 (main 868);
    - `vs-opp`: 796 of 1,300 (main 231);
    - `vs-late`: 421 of 851 (main 184).
  - During the race, R-0079's captures differ only in:
    - one picture of `leaguefin` (3 pixels);
    - three of `zz2p` (a 2-pixel strip of the rider under the ink, below).
  - The other differing pictures follow the race:
    - the results' icons (R-0071);
    - NOW PLAYING after a two-human restart (TWO-HUMAN-RESTART).
- **The split demo.** Its 326 retained pictures stay equal (`demo_recheck.py`).
- **Tests.**
  - `race_pairing_tests`: setup, the MIKE quirk, both riders' groups and the ending test.
  - `dragster_race_tests`: the flag's bytes and their refusals.
  - `presentation_tests`: the chain's lower clock and its blank.

## Not covered

- **The riders under the ink.** These are not HUD text:
  - a pad-2-idle rider 1's look (seat and head) differs from the original's in `vs-idle` (446
    race pictures) and `league-idle` (3);
  - a 2-pixel strip of a rider's upper body under the ink is drawn without the ink's colour math
    in a few pictures (`zzap` 9, `zz2p` 3, `leaguefin` 1);
  - rider 0's sprite shows near the split line in one `zzap` picture (6317).
- **The player's hint ending.** It uses `< 22`, where the listing's test also ends the hints on
  voices from 150. This is not captured for one-player play, so the player's test is unchanged;
  a follow-up was flagged.
- **Rider 1's stunt score field** (`$12CB`) in a split stunt event is not modelled; no capture
  reaches one.
- **A pause message's `!`** keeps the caption's glyph.
- **Whether the chain runs during a pause** is not established. No field was pending in any
  captured pause.
