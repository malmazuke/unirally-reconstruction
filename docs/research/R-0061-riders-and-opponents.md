# R-0061 - The one-player race's riders and opponents

Status: recovered and implemented on `task/race-riders-opponents` (RACE-RIDERS-OPPONENTS),
26 September 2026. PAL ROM `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`,
audited bsnes core `e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b`. Follows
R-0052 (the HUNTER tour's opponent) and R-0055 to R-0060 (the one-player menus).

Until now the race scenarios were MIKE's against BRONSEN (ANTI-UNI on HUNTER's tracks), and the
app showed a notice for any other rider chosen on PICK YOUR UNI, and for SILVIA (0x12) and
GOLDWYN (0x13), whom NOW PLAYING brings once the rider holds a bronze or a silver on the tour
(`$80:B31F-B346`). The menus copy the rider `$017D` to `$77:0748` and the opponent `$017F` to
`$77:0749` at `$80:99E8-9A01`. This record lists everything the race reads from those two bytes.

## The rider changes no physics

Over a whole DRAGSTER race with the same inputs, ANDREW's work RAM `$0000-$1FFF` equals MIKE's on
every frame from the boundary to the finish. At the boundary only `$017D`, `$12E7` (1 << rider) and
menu temporaries differ. The rider changes four things:

- **Its voices.** `$82:9D47-9D5E` makes a combination's voice event 72 + 16 * (character >> 1)
  + (x & 15): each pair of characters shares sixteen voices. MIKE and ANDREW have 72-87, MARTIN and
  MELISSA 88-103, up to STEVE's 184-199. All of them take the voice path (caption only, no reward).
- **Its tutorial hints.** `$82:D94C-D96F` starts the hints (`$12E3` = 1) only when the cartridge
  RAM's `$77:1116` lacks the rider's bit (`$12E7`, 16-bit). When a scoring event ends the hints,
  `$83:CE2C` sets that bit and clears `$12E3` (unless `$7E212C` is nonzero, which one-player play
  never has). So once a rider's hints have ended, that rider's later races, and a restart of the
  same race, start without them.
- **Its sprite colours.** `$82:DD90-DD9A` loads OBJ palette 3 (CGRAM 0xB0) from asset 6 + rider.
  The tiles are the same for every rider.
- **The ink of the HUD and captions.** `$82:D57F-D5FB` sets up HDMA channel 5 from the rider's
  four bytes of `$82:D4DC`: three COLDATA writes (bits 5, 6 and 7 choose red, green and blue, bits
  0-4 the intensity) and CGADSUB (0x04, or 0x84 to subtract). With CGWSEL 0x02, the BG3 ink is
  CGRAM 27 (`$000D` on every race palette) plus that fixed colour, clamped per channel. MIKE's
  (15,0,0) gives (28,0,0), which is also CGRAM 22's colour, the one native drew before. ANDREW's
  (0,0,31) gives (13,0,31), and TONY's subtraction gives black. Where a rider covers the ink, the
  original still adds CGRAM 27's 13 to the sprite's red (R-0042), whatever the rider.

## The opponent sets the AI's tier

| Site | What it does |
| --- | --- |
| `$83:CC2B-CC56` | `$1275` (the AI level) = opponent - 16: BRONSEN 1, SILVIA 2, GOLDWYN 3. `$1283` (the catch-up term) = 0 for level 1, the track's byte of `$83:C8B3` + 0x20 for level 2 and + 0x40 for level 3. On HUNTER's tracks `$83:CC0B-CC29` sets 3, 0x40 and 0x60 instead (R-0052). |
| `$83:CC59-CC7C` | `$1281` (the progress adjustment bound) = 0x60 (one-run race) or 0x48 (lap race) minus `$1283`, 8-bit; the base when bit 7 of the result is set. |
| `$82:A793` | The opponent's speed cap gains `$1283` * 2 while the player leads. |
| `$83:E135` | At a jump marker under four updates in the air, any level but 1 keeps the jump. |
| `$83:E16B-E1C8` | The launch once the opponent has risen four updates off the ground (below). |

The launch rule by level, with `$1277` the suppression word it leaves:

- **Level 1** (`$83:E1A8`): the player must let it launch (no feature total yet, or a lead of 3
  progress transitions or more): 30, else 0 and no launch.
- **Level 2** (`$83:E17D-E1A5`, SILVIA): `$1277` = the player's lead + 15. A negative sum leaves 0
  and no launch. Otherwise no trick launches when the animation counter `$04C7` ends in 7, though
  the word stays.
- **Level 3** (`$83:E175`, GOLDWYN and ANTI-UNI): always, with 60.

The opponent's voices follow its character: BRONSEN's 200-215, SILVIA's and GOLDWYN's 216-231
(one pair) and ANTI-UNI's 232-247. Past the class table they take the reward path and read the
learned bank past its end, `$7E21D9-21E8` for SILVIA and GOLDWYN, which is zero. So, as for
BRONSEN (R-0035), they reward nothing. `$82:DDB0-DDBC` loads OBJ palette 4 (CGRAM 0xC0) from asset
6 + opponent. The loading time before the race does not depend on the pairing.

## Native

- The scenario carries the pairing (`RacePairing`) and the hints' start. The race state carries the
  pairing and the opponent's tier (`OpponentTier`), which the setup computes from the pairing and
  `$83:C8B3`. The layouts serialize neither, so deserializing another pairing's state takes them
  as parameters.
- `opponent_ai.cpp` has the level-2 rule, and no level throws any more.
- `announce_combination` takes each rider's own character. Deserialization admits each rider's
  sixteen voices.
- The front end's records carry `$77:1116`. The race's scenario comes from the menus
  (`one_player_race_scenario`), and the return sets the rider's bit when the race's hints have
  ended.
- The race picture loads OBJ palettes 3 and 4 from the front end's assets 6 + character, and draws
  the ink through the rider's colour math (`classic_race_ink`), faded after it.
- Pack profile v21 adds `race.opponent-catch-up` (`$83:C8B3`, 45 bytes) and
  `race.rider-colour-math` (`$82:D4DC`, 64 bytes).
- The app, `front_end_runner` and `zoom_zoo_runner` (`--rider`, `--opponent`,
  `--tutorial-hints`) start any pairing the one-player menus can choose;
  `classic_race_presentation_runner --timeline ... --pairing` draws one.

**A correction to R-0042.** `$81:BEA8-BEF1` blanks the caption when the player's queue runs dry.
The blank reaches the screen one picture later than native showed it: the picture after the
update that found the queue dry still shows the last caption, and the blank follows from the next
picture. Native now blanks once the dry wait's cooldown has started to run down. This was measured
on ANDREW's DRAGSTER race, whose captions are MIKE's: frames 2736-2748 around the end of a hint
group.

## Evidence

**Race state and pictures.** These are `track_reference` captures from power-on (bsnes, Strict;
full work RAM and cartridge RAM every frame), in `local/evidence/race-riders-opponents/race/`.
`--rider` presses Right and Down on PICK YOUR UNI. `--sram` preloads a medal on the tour
(`$77:069C` + rider), which makes NOW PLAYING choose SILVIA or GOLDWYN, or the rider's tutorial
bit. Nothing in the ROM verifies the save's checksums (R-0059). DRAGSTER holds Right from 1500,
with Up 2200-2259; ZOOM ZOO holds Right from 1500, with Up at 2600 and 3600. Each capture was taken
twice, with identical memory.

| Capture | Pairing | Updates equal from the boundary | Pictures |
| --- | --- | --- | --- |
| andrew-dragster | ANDREW v BRONSEN | 2,473 of 2,473 | 43 of 43 equal |
| silvia-dragster | MIKE v SILVIA | 2,473 of 2,473 | 43; 28 differ only by the off-screen arrow |
| goldwyn-dragster | MIKE v GOLDWYN | 2,473 of 2,473 | 43; 26 differ only by the off-screen arrow |
| mike-hints-off-dragster | MIKE v BRONSEN, `$77:1116` = 1 | 2,473 of 2,473 | 43 of 43 equal |
| silvia-zoom-zoo | MIKE v SILVIA | 4,825 of 4,825 | 51; 32 differ only by the off-screen arrow |
| goldwyn-zoom-zoo | ANDREW v GOLDWYN | 4,825 of 4,825 | 51; 32 differ only by the off-screen arrow |

- The boundary words match the rules above. For example, SILVIA on ZOOM ZOO has `$1283` 0x21 and
  `$1281` 0x27, and GOLDWYN on ZOOM ZOO has 0x41 and 0x07.
- In SILVIA's DRAGSTER race, `$1277` took the level-2 values 6 to 15.
- GOLDWYN's ZOOM ZOO race queued voice 219.
- In the hints-off race `$12E3` starts at 0.
- Every differing pixel lies in BG3 rows 14-15, columns 5-7 or 26-28. That is the red off-screen
  rider arrow, a declared omission since M4-16 (R-0043). SILVIA and GOLDWYN get ahead of the player,
  so it shows on the right as well as the left. It is queued as RACE-OFFSCREEN-ARROW.

**The menus around the races.** These are front-end captures (pictures from frame 400, work RAM
`$0000-$1FFF` every frame), compared by `compare.py` in `local/evidence/race-riders-opponents/
front-end/`: the menus' words, the OAM buffer and the text map on every frame outside the races,
and every picture.

| Capture | Races (native, between the menus) | Differences | Pictures equal |
| --- | --- | --- | --- |
| andrew | ANDREW v BRONSEN | none | 1,054 of 1,054 |
| forced-silver (R-0059's) | MIKE v BRONSEN, MIKE v SILVIA | none | 582 of 582 |
| goldwyn | MIKE v BRONSEN, v SILVIA, v GOLDWYN | none | 6,321 of 6,321 |

`sram.py`, now with `$77:1116`: the kept cartridge RAM words equal the original's at `andrew` 3690
and `goldwyn` 5600, 10500 and 13390.

## Not recovered

- **The player's voices past 87.** No capture has a player trick combination (no
  `track_reference` capture in the evidence queues a player voice), so riders 2-15's voices are
  from the listing.
  Native computes them with the same expression as the opponent's, which GOLDWYN's 219 confirms.
- **`$7E212C`**, which stops the hints ending, has no recovered writer in one-player play.
- **The cartridge RAM class counters** that the opponent's out-of-table voices increment
  (`$81:C253-C258`) are not modelled, as for BRONSEN (R-0035).
- **ANTI-UNI against another rider** on HUNTER's tracks has no capture. Its parts (the rider's
  voices, hints, palette and ink) are each measured on other tracks.
- **The race's own result pictures** (`result_screen.cpp`, the authored lap screen) still print
  MIKE and BRONSEN. Only a race started with `--track`, which is always MIKE's, draws them. After a
  race from the menus the front end draws the result, with the riders' names from the records.
- **Two players and the stunt events** are other tasks (SPLIT-SCREEN-RACE, STUNT-EVENTS).
