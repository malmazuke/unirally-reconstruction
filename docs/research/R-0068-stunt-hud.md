# R-0068 - A stunt event's race picture, and NEON

Status: recovered and implemented on `task/stunt-hud` (STUNT-HUD), 27 September 2026 (written on
STUNT-RESULT's branch, `16d2789`). PAL ROM
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, the pinned bsnes core.
Follows R-0066 (the stunt event in the race engine), R-0043 (the race's HUD), R-0063 (the BG3
upload order and the arrow), R-0036 (the rider look) and R-0040 (the window members). Builds on the
STUNT-EVENTS decode (`local/evidence/stunt-events/decode/stunt-events.md`, section 1.6), which this
record supersedes where they differ. Pack profile v26. Tier 1: NEON's rules change race state.

Evidence tags: **[C]** seen in a capture (pictures, work RAM, or the PPU's CGRAM and VRAM read from
the core's serialized state by `ppu_probe.py`), **[L]** read from the static listing
(`artifacts/static-map/bank-8x.lst`) and not seen on its own. Every rule a capture reaches is also
confirmed by the native comparison: the pictures under Evidence are equal pixel for pixel.

## Summary

A stunt event's race now draws as the original's: `stunt` in the left field, the clock counting
down and stopping at 0:00.0, the score and the qualifying score bottom right, the score written in
its place in the NMI's upload order, the countdown's digits without their waits, no opponent and
no arrow, and the finish banner from the finish display. The opponent's head point stays at zero
for the look. HUNTER's stunt event, track 42, is NEON: its own scenery, a black screen lit by the
palette under the player, and race rules of its own: no tag effects, and an empty cell of palette
7 read with another angle. NEON is recovered in full and its ridden capture is exact.

## The HUD

- Left field [L; C]: the setup's `$81:D771-D816` writes `stunt` (table indices 0x1C, 0x1D, 0x1E,
  0x17, 0x1D) from column 2 and sets `$053F`; a stunt event's riders never cross the line for
  good (`$0EFB` stays 1), so `finish` never replaces it.
- Clock [L; C]: the usual field. The tick after 0:00.0 (`$81:C7EC` sets `$034D`, `$81:C830` clamps
  the minutes, `$81:C86A` skips the cells `$0E2F-$0E3B`) makes the NMI rewrite 0:00:0: the digits
  stay and the update is spent, so a caption taken on it waits a picture. [C] 0:00:0 on every
  picture from the clock's end to r in bowl-lose and hill-win.
- Score field [L; C]: the setup's `$81:CD41-CE6B` writes `0` at column 25, `/` at 26 and the
  qualifying score from 27 (rows 24-25, attribute 0x38): hundreds, tens and units, or below 100 the
  tens and units and `$80:822A`, which the pictures show blank (`$81:CDB8-CE0F`; the tens digit
  is always written).
  `$81:C357-C3D0`, the tail of the queue consumers, compares the score `$77:07BB` with the one it
  last split (`$12B9`, 0 at the start [C]) and on a change splits it into `$12BD/$12C1/$12C5`, with
  `$12FB` (hundreds less one) and `$12F7` (hundreds plus tens, less one) negative for cells not to
  write, and sets `$12C9`. The one-player NMI's stunt branch (`$81:F28B-F303`) writes the cells
  at columns 23-25 and spends the update; it comes after the riders' cells and before the caption
  (R-0063). So the score, paid on the update a trick's caption is taken, shows a picture before
  the caption. [C] `$12C9` set for one frame after each reward, two when a tenth's tick was due.
  Each digit indexes the character table `$80:81F4`, so a score of 1000 would show a letter [L].
- The countdown [L; C]: `$83:E5B7-E5D5` splits the phases above 220, 160 and 100; a stunt event's
  tests (`$83:E5E2`, `$83:E634`, `$83:E686`, `$83:E6E8`) go straight to each phase's digit
  (members 0, 1, 2) and GO (members 3 and 4 on the `$0300` parity) from 100: no transition member.
  [C] the big `3` from hill-win's first visible pictures (1348).
- No arrow [L]: `$82:98B7-98C8` blanks `$0FD5` every update in race mode 2.
- No opponent [L; C]: `$82:D789-D797` sets `$15A3` = 0xE5, entry 99's ninth x bit, and the
  opponent's object writer is part of its skipped update, so it stays at the setup's x 0x60 - 256.
- The finish banner [L; C]: `$83:E8E0`, which runs the riders' finish drivers, runs only once
  `$0FE9` is set (`$83:E898`), for both riders at once, the player's first; the drivers are armed
  by the update that sets `$0FE9` (as a race's are by a rider's finish) and not by the finishes at
  the clock's end. [C] bowl-lose's `loser` hand at 3900-4043.
- The look [C]: the opponent's contact is skipped, so its head point `$1267/$1268` keeps the
  setup's zero (bowl-lose and hill-win, every frame), while both riders' look steps run (`$0D4B`
  moves in hill-win). Native took the opponent's current pose; hill-win's seat differed on 248
  pictures.
- The fade [C]: where a rider covers the ink, the race's colour math adds red to the rider; the
  fade scales that sum too (bowl-lose 1340-1341, the score field under the rider in the fade-in).

## Track 37's BG1 fetch [L]

`$81:AD1D-AD20` and `$81:AD71-AD74`: with `$0FF7` set (the 65,536-unit playfield, R-0066) the BG1
map fetch skips the row test and a row takes y's sixteen bits. Native's picture wraps the row the
same way. No capture shows a row outside the playfield there: track 37's idle capture looks at
rows near y 8,000 (`$040B` 0x7D) and is exact with the rule and without it.

## NEON (track 42 in one-player play)

- The flag [L; C]: `$82:D98F-D9A7` (the race setup) and `$82:DC22-DC3A` (the scenery loader) set
  `$12D1` when the track `$77:074A` is 0x2A and `$77:0750` bit 3 (two players) is clear; nothing
  else stores it, and the work RAM clearing before the race zeroes it [C: set at 1249, cleared at 1313,
  set again at 1314]. Track 42 is place 2
  of HUNTER's tour, a stunt event in one-player play; with two players it sets `$131F` instead.
  Native's scenario carries it as `neon_lighting`.
- Scenery [L; C]: scenery 14, past the thirteen `track mod 14` gives: BG2 tiles asset 0x7E, map
  0x90 and palette row 0xA1, and asset 0xA6 in all six rows of the palette block
  (`$82:DC7C-DCB4`). [C] CGRAM rows 0-5 and the BG2 map equal these assets; the map is tile 1 of
  palette 7 everywhere, and tile 1 is colour 1 throughout: BG2 is colour 113.
- The NMI [L; C]: `$80:87A2-87D7` with `$12D1`: HDMA channel 5 (the rider's colour math on the ink)
  off, `$2131` = 0x92 (subtract; objects and BG2), `$212D` = 0x02 (BG2 on the sub screen only),
  `$212C` = 0x15 (BG1, BG3 and the objects on the main screen), colour 0 black and colour 113
  `$12D5`; `$80:8816` skips the palette cycle `$82:D37E`. So the backdrop is black, the ink is
  colour 27 in front of the rider, and the player's object shows less BG2's colour 113, channel by
  channel. The fixed colour is 0 [C].
- The lighting [L; C]: `$83:CECB` calls `$83:D1CA` in each race update: the player's object
  palette becomes 7 (`$1516`, `$83:D1DC-D1E3`), and `$12D3` moves towards `$83:D1C3[$12CF]` by a
  quarter of the distance, at least one, in eight-bit arithmetic (`$83:D1EE-D247`; the table's
  eighth byte is the next routine's first opcode, read for a palette of 7); `$12D5` = red 15, blue
  30 and green `$12D3` (`$83:D24A-D271`). `$12CF` is the lowest palette (a cell word's bits 10-12)
  among the ten cells the player's contact sampled, from 7 (`$81:8B75-8BB3`). [C] `$12CF`,
  `$12D3` and `$12D5` every frame of the start (4, 3, 2, 1, 0 as the rider settles; `$12D3`
  5, 9, 0xC ... 0x13 then down to 0), and colour 113 in CGRAM equal to `$12D5`. Native keeps
  `$12CF` beside the state (`player_contact_palette`, not serialized) and the level in the
  picture's history; a timeline carries the palette in a side file (`--contact-palettes`).
- Colours 96-111 [C]: never loaded or cycled in NEON's race, they keep the menus': NOW PLAYING's
  base palette (asset 36 from colour 64) for 96-107 and the front end's cycle (`$80:FA60`) for
  108-111 at its phase (phase 0 on the laboratory's route, [C] from 1240). The app passes the
  front end's own; the renderer alone takes phase 0.
- No tag effects [L; C]: with `$12D1` set, `$83:CECB-CED3` runs `$83:D1CA` and returns through
  `$83:D103`: the tag, its latch `$1325` and the eight effects (`$83:D104`, R-0052) never run on
  NEON. R-0066's "HUNTER's stunt event keeps the tag effects" held only while the riders' counts
  stayed together. [C] the ridden capture: native latched at update 221, the original never does.
- An empty palette-7 cell [L; C]: an empty cell (tile bits 0) of palette 7 is a probe of
  penetration 0x7F (`$81:8BD9-8BE0`), whose angle `$81:8CDD` reads from `$7E:A001 + X`. Every pass
  of the probe loop loads X from `$12D1` at `$81:8BA0` (16-bit X), and an empty cell's paths
  (`$81:8BC4-8BE0`) do not change it, so on every track the angle is the tile-column table's byte
  1 + `$12D1`: byte 1 off NEON, byte 2 on NEON. It is negated (`$81:8CE1`) when `$0202` is set,
  the latest tiled probe's mirror bit (`$81:8BEB`). Byte 1 is 0x00 and byte 2 is 0xA0 in every
  track's table (all 45: tracks 02-44, DRAGSTER's and ZOOM ZOO's; `angle_scan.py`), so the angle is
  0 off NEON and 0xA0, or 0x60 after a mirrored tile, on NEON.
- The first probe [L; C]: it is selected (`$0F13`, `$02EC`, `$02BE`) only when its angle is not
  below the 0xE0 `$02BE` starts from (`$81:8FBD-8FC2`, `$81:900E-9025`): an eight-bit signed test,
  the difference angle - 0xE0 not negative, so angles 0xE0-0xFF and 0x00-0x5F are selected and
  0x60-0xDF are not. The rule is the original's on every track, but it changes nothing off NEON: no
  column with a height other than 0xA0 in any track's table has an angle, plain or negated, in
  0x60-0xDF (`angle_scan.py`), and the empty probe's angle there is 0. [C] the ridden capture at
  update 1576: probe 0 of penetration 0x7F and angle 0xA0, `$0F13` 0; native had selected the word
  0x1C00. Native now reads the table's byte 1 + `$12D1` for every empty palette-7 probe.

## Evidence

Captures, compared with `local/evidence/stunt-hud/pictures.py` (native's pictures from the
capture's own controller timeline, against its frame images or, without one, its video digests)
and `track_reference explore --scenario auto` (the race rows). Logs:
`local/evidence/stunt-hud/checks-<commit>/`.

| Capture | What | Result |
| --- | --- | --- |
| stunt-events/decode/bowl-lose (frames: STUNT-RESULT's capture.py pictures) | BOWL, tricks, 49 < 68 | every picture 1336-4043 (2,708): 0 differing pixels |
| stunt-events/decode/hill-win (same) | HILL CLIMB, tricks, 121 >= 90 | every picture 1332-4110 (2,779): 0 |
| the nine idle stunt captures (track-breadth-2 row0-3-pos2; locked-tours jumper-2 to hunter-2) | released, to 2900 | every picture from the boundary to 2899 (1,500 to 1,569 a track, 13,940 in all), by digest: all equal |
| stunt-hud/captures/neon-start | NEON's start, pictures 1400-1440, 1600, 1800 | 0 differing pixels |
| stunt-hud/captures/neon-ride | NEON ridden (hill-win's inputs moved to its boundary) | rows 2,808 of 2,808 through the result load (4102); pictures 1400-4101 (2,702; 331 with images, 2,371 by digest): all equal |

STUNT-RESULT's pictures of the same manifests were checked against the track_reference captures'
video digests (identical where both exist). From the race's return r the front end's comparison
(`local/evidence/stunt-result/acceptance.sh`) covers the pictures.

The ridden NEON capture, before the two race rules: rows exact to update 221 (native latched the
tag), then, with the tag skipped, to 1576 (the selected word); its pictures differed on 183 frames
before the ink was put in front of the rider.

Nothing else moves (`checks-b69a83d.out`, on the review's corrections; `checks-f31aae8.out` gave the
same numbers on the commit that first added NEON's race rules). The acceptance rows above are
the same in both:
- the stunt rows (`stunt-event-race/compare.sh`): all 16 captures exact on every row, as before
  (bowl-explore 2,831, bowl-lose 2,814, hill-win 2,885, bowl-brake 2,808, hill-brake 2,902,
  jumps-ride 2,815, downer-ride 2,952, the four cold and five locked idle captures);
- R-0061's seven race captures and silvia-runner-25's consecutive frames: every row exact and 0
  differing pixels on all 336 pictures;
- the per-track recompare of both sweeps, main `e131469`'s runner (pack v24) against the
  candidate (pack v26): race tracks identical 16 of 16 and 20 of 20 (HUNTER's 40, 41, 43, 44
  among them), the stunt rows exact;
- HUNTER-EFFECTS' 48 held captures (R-0052; tracks 40, 41, 43, 44): every one the same as main's
  runner gives, all exact to their ends;
- the front end around a stunt event (STUNT-RESULT's acceptance, pack v26, on `b69a83d`):
  bowl-lose, hill-win, bowl-quit, hill-complete and bowl-press equal on every frame from power-on
  and on every picture from 400 (2,165, 2,298, 2,289, 3,898 and 2,165), the records at every
  checked frame;
- builds lab-release, lab-debug and app-debug with no warnings, ctest 29 of 29 on each; no
  function over 80 lines; native-symbols passed.

## Listing only, and not covered

- Track 37's rows outside the playfield; a score of 1000 or more (a letter), a qualifying score
  below 10; the `draw` caption.
- `$0202` before a tiled probe in a contact: it is scratch that many routines write (among them
  `$82:83FD`, `$82:841B`, `$82:8628`, `$83:F12E`, `$83:8DE9`), so an empty palette-7 probe met
  before any tiled probe of its contact is negated or not by whatever the last of them left; native
  takes it clear. Off NEON the angle is 0 either way; on NEON it can change race state (a first
  probe of angle 0x60 is selected, one of 0xA0 is not, and the angle feeds `$02BE` at
  `$81:90CC-90E0`). Not covered by any capture.
- NEON's colours 96-111 in the app: the front end's, not compared with a capture from power-on;
  whether a BG1 tile of palette 6 showed in the compared NEON pictures was not checked.
- NEON's two race rules are shown by one ridden capture; the empty palette-7 probe was reached
  there once as a first probe (update 1576).
- A single restored state (no history): NEON's level is taken as the level of the palette under
  the player, the score cells from the score.
- Two players: the second score field (`$12BB`, `$12CB`), `$131F` on track 42, the two-player NMI.
