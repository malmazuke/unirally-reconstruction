# R-0048 - The last two race guards: long races' checkpoints and the inverted AI marker

Status: recovered and implemented on `task/race-guards` (RACE-GUARDS), 24 September 2026.
PAL ROM `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, audited bsnes
core `e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b`. Answers R-0046's
next experiments 1 and 3.

After [R-0047](R-0047-special-tiles.md), native stopped on four of the sixteen cold-start
races, at two guards:
- INFINITY (7 laps) at update 379 and HAIRPIN HILL (5 laps) at 531, on `race checkpoint
  index invalid`;
- MONSTER at 809 and HYBRID at 2,053, on `inverted AI marker is unrecovered`.

## Checkpoint-seen flags are 80 bytes

A rider crossing checkpoint *c* with *l* laps remaining indexes the shared first-seen flags at
`l * 4 + c`. Native held 20 flags, from the 742-byte state's `checkpoint_seen` at `$114D`. A
race of *n* laps starts with `n + 1` laps remaining, so five and seven laps index up to 27
and 35.

- **Observed.** On INFINITY's capture, `$116A`, `$116B` and `$116C` clear at updates 379,
  481 and 588, then `$1166-$1168` and `$1162-$1164`, exactly like the first twenty flags.
  At the boundary `$114D-$119C` holds `$FF` on every capture, and `$119D` holds 0.
- **Listing.** Race setup fills 80 bytes: `$81:CD25 LDY #$004F; LDA #$FF; STA $114D,Y; DEY;
  BPL`. The only other writers are the crossing's `STA $114D,X` stores (`$81:CA3C`,
  `$81:CC17`), and no instruction addresses `$1161` onward directly.
- **The HUD agrees.** The split-time slot store is `$100D + 16 * laps + 4 * checkpoint` (R-0044,
  four bytes a slot). 80 slots end at `$114C`, right before the flags.

Native now keeps all 80 flags. DRAGSTER's one lap and ZOOM ZOO's three index at most flag 19,
so their 742-byte layouts are unchanged. The other tracks' state becomes `URTRnn03` (836
bytes): the 742-byte layout, R-0047's 34 special-tile bytes, then flags 20-79. The HUD's slot
store widens to 80 slots to match. `track_reference` appends `$1161-$119C` to the original's
projection, so the flags are compared.

## An inverted marker returns before the AI sets any input

The opponent's AI (`$83:E082-E253`) reads the marker `$0FC7` and on bit 15 returns at once
(`$83:E0A7-E0AF`). Native refused that case. The first attempt kept the opponent's direction
from the previous update; MONSTER then differed at update 809, where the original's direction
`$031B` was 1, neutral. The port-2 reader explains it: with the AI enabled (`$0C6D`), every
update `$82:AB6F-AB8B` releases all the opponent's inputs, A (`$031F`) and X (`$0323`)
included, and sets `$031B` = 1, before the AI runs. So after an inverted marker the opponent
has every input released and its direction neutral for that update. The selector `$0C75`,
countdown `$0C6F` and suppression `$1277` keep their values.

Native already cleared the brake, jump and rotations. It now also sets the direction to
neutral and masks the A and X it derives from the selector's bits 1 and 2 for that update.

## Evidence

`track_reference recompare --per-track` on the part 2 sweep (horizon frame 2,900):
**all 16 compared races are exact over their whole windows.** Before this task, INFINITY was
exact for 379 rows, HAIRPIN HILL 531 and MONSTER 809.

New captures (`local/evidence/race-guards/`), each compared with `explore`:

| Track | Released to frame 4,400 | Right held to frame 4,400 | Path exercised |
| --- | --- | --- | --- |
| INFINITY | exact 3,025 of 3,025 | exact 3,025 of 3,025 | flags 21-31 cleared |
| HAIRPIN HILL | exact 2,994 of 2,994 | exact 2,994 of 2,994 | flag 22 cleared; 10 inverted-marker updates |
| MONSTER | exact 2,982 of 2,982 | exact 2,982 of 2,982 | 3 inverted-marker updates |
| HYBRID | exact 2,317 rows, to the opponent's finish | exact 2,317 rows, likewise | 1 inverted-marker update (2,053) |

Hidden 4,000-update runs with Right held complete with 0 fallback frames on all four. With
R-0047's runs, **every one of the 16 cold-start race tracks** now does. The idle matrix
(`content track-idle-matrix --updates 1200`, ZOOM ZOO's scenario) completes on 41 of 45
tracks. The four stops are BOWL (pair 8), LAST ONE and DOWN+UP (pair 26) and LITTLE DIPPER
(shape).

## Limits

- An inverted marker lasts one update in these captures: 1, 3 or 10 updates in all.
  Longer runs of it follow the same code, but no capture shows one.
- The opponent's HUD split for flags past 19 is covered only through the flags themselves.
  No picture of a long race's split has been compared.
- Races of more than 19 laps would index past the 80 flags. None exists among the tracks
  read so far (laps 1, 3, 5 and 7 on the cold-start set), and native still guards it.

## Reproduction

```sh
python3 -m tools.unirally_lab.native.track_reference recompare --per-track \
    --sweep local/evidence/track-breadth/track-breadth-2/sweep \
    --binary build/lab-debug/src/core/zoom_zoo_runner --pack local/classic-pal-crawler-tracks-v11.pack --out <json>
local/evidence/race-guards/captures.sh      # eight captures, about 12 s each
python3 -m tools.unirally_lab.native.track_reference explore --reference local/evidence/race-guards/<capture> \
    --binary build/lab-debug/src/core/zoom_zoo_runner --pack local/classic-pal-crawler-tracks-v11.pack \
    --scenario classic.track.NN --out <json>
```
