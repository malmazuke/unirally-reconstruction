# R-0051 - The loop and the other tile flag pairs; the HUNTER tag effects

Status: recovered and implemented on `task/tile-pairs` (TILE-PAIRS-8-12-26), 25 September 2026.
PAL ROM `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, audited bsnes
core `e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b`. Continues R-0047 (the
special tiles) and answers R-0050's open tile stops. Addresses name the working copies the race
update uses: `$0F13` selected word, `$0F23` surface mode, `$0F27` tile mode, `$0F2F` reflection
lock, `$0F3B` drive step, `$0F3D` animation delta, `$0F41` tile pose, `$0F4B` float, `$0F51`
reflected, `$0F53` orientation, `$0F59` pose override, `$0F5B` contact skip, `$0F5D` leading
support, `$0FA9`/`$0FAB` velocity, `$A5` x. `$82:8A0D-8AB4` loads them from each rider's words.

## The loop (flag pair 26)

Movement's table entry for pair 26 is `$81:837E-84AB`. Three new words per rider carry it:
`$0351,y` (direction), `$0355,y` (step, signed) and `$0359,y` (cooldown).

- **Entry, step 0.** Refused if the rider is rising, has leading support, or carries a pose
  override outside `$0610-$061F`; a refused entry stores step `$FFFE`, which the per-update reset
  (`$81:85AE-85B7`) counts back up to 0. Otherwise the cooldown is set to 3.
  - The rider must face the descriptor's way (`$4000` in `$0F13` with `$0F51` set, or neither);
    if not, nothing else happens.
  - It stores the direction (0 against a mirrored descriptor), pose `$0610`, x moved 10 along
    the direction, velocity x 0, and step 1.
- **Steps 1-15.** Each update sets pose `$0610 + step`, both response words and velocity x to 0,
  velocity y `$1CE`, reflection lock 6, surface mode, float (gravity off), contact skip and tile
  pose, and `$0F51 = 1 - direction`.
  - x moves by the step's word from the table at `$81:834C` (17 signed words; pack entry
    `zoom.loop-offsets`, profile v13).
  - Step 8, the top, clears float, contact skip and tile pose for that update. Contact then
    runs with the step at 9.
- **Step `$10`** stores 0 and returns.
- **The cooldown** (`$81:85B9-861A`, after the reflection lock) falls by one per update. At 0 it
  clears the step. On the update it reaches 1 it ends the loop's float, pose override and angle
  sentinel. It then poses the rider rolling (`$0F15` = 1, `$0F4D` = 2), orientation `$24`
  (`$14` for a mid-loop, unreflected rider). At step 9 it also restores `$0F51` from the
  direction. While the cooldown is nonzero the jump is refused (`$82:A8CD-A8D5`).
- **Contact** (`$81:919E-91B2`): on pair 26 with `$0355,y` = 9, both probe penetrations `$28`
  (vertical) and `$2C` (horizontal) are cleared. So at the loop's top neither the boundary test
  (`$81:91E3`) nor the correction (`$81:97E4`) moves the rider.

LAST ONE's capture shows the opponent riding the loop twice (updates 809-825 and 826-842). Each
step's x change is the table's word, velocity y is `$1CE` until step 8 adds gravity's 5, and the
cooldown counts 3, 2, 1, 0 after each ride.

## Flag pair 8

- **Movement** (`$81:8554-858D`) sets the tile mode (`$0F27`) and `$0F2D`. It also counts
  `$0D3D,y` up by one; the per-update reset (`$81:8594`) counts it down by one. At 8, velocity x is
  held to +-`$20` instead (N-flag compares). Because both run every update, the counter stays 0
  or 1 in practice.
  - `$0F2D` skips the slope y nudge after integration (`$82:A6C0`).
  - It also makes the pose target velocity x shifted once instead of five times (`$83:EFA1`).
- **Contact.** A re-contact (`$81:92D9-92F0`) clears the unsupported count and duration and
  goes straight to the correction, as a steep surface (28-30) does. No landing matrix is read.
  - Under surface mode, continued contact keeps velocity y (`$81:96FA-970B`).
  - The correction applies the vertical penetration even when the horizontal one is larger
    (`$81:97E6`, already native).
  - `$1349` is inert as for pair 16 (R-0047).

## Flag pair 12

`$81:8950-8998` sets the drive step `$0F3B` to 1 (mud's is 4) and `$0FB1`. With the D-pad held it
also sets the animation delta `$0F3D` to 2 against the held direction, seen from the rider's
facing.

- A drive step also replaces the low-speed damping (`$82:A5FC`).
- `$0FB1` sends the brake path through mud's branch (`$82:9981`): throttle and announcement
  drop.
- The stationary override (`$82:A069-A0B6`) returns without a store when the rider moved or the
  D-pad is centred. So the tile's delta survives it; native now keeps the earlier value in that
  case.
- `$0FB1` is also copied to `$0FED`/`$0FEF` (`$82:8C99`, `$82:9187`), which nothing in banks
  `$80-$83` reads. On JUMPOVER the opponent is on the tile for 24 updates from 773.

## Flag pair 28

`$81:8316-834B`: unless the reflection lock is set, velocity x moves by `$20` and x by 8 along
the descriptor's facing. DOWN+UP reaches it at update 546 once its loop passes.

Pair 4 (`$81:875C`) is still unrecovered, and the table (`$81:82F5`) has no entry above pair
28; native still throws on both.

## State

The four words join the special-tile block, now 12 words per rider:

- The other tracks' states are `URTRnn05` (854 bytes).
- DRAGSTER's and ZOOM ZOO's are `URDG0004`/`URZZ000E` (794 bytes), but only while a word is
  live, so their 742-byte states are unchanged.
- `classic_race_layout.special_tile_bytes` projects `$0351`, `$0355`, `$0359` and `$0D3D` (+2 for
  the opponent).

## The HUNTER tag effects (not implemented; HUNTER-EFFECTS)

TWO LOOPS' divergence at update 1,252 is not an announcement fault. The HUNTER tour (`$131F`
nonzero) runs `$83:CEC9` every update, which native lacks.

- **The trigger** (`$83:D104`) tests whether the riders' boxes overlap. It runs only while no
  effect is running (`$1323`), neither rider has finished, and once the progress words `$0FCD`
  and `$0FCF` have differed by 2 or more (`$1325`). An overlap picks one of eight effect flags
  by the player's x & 7 (`$1327 + 2*(x & 7)`).
- **Each effect**, when its flag becomes 1:
  - pushes its announcement to the front of the player queue (`$81:C55B`: written at the read
    cursor, which then steps back);
  - clears the other flags and runs a routine, most with a 500-update timer that ends with
    event `$23`.
- **The two observed differences are both effects.** At 1,252 the original writes event `$1F`
  (effect `$1327`) over the slot just shown and steps the read cursor from 14 to 13. HUNTER 44
  with Right held (R-0050) shows event 30, effect `$132B`.

The effect routines' words feed consumers not yet traced. `tasks/HUNTER-EFFECTS.md` carries the
reading.

## Evidence

`track_reference recompare --per-track`, pack v13, `lab-debug`:

| Sweep | Result |
| --- | --- |
| locked tours (20 race tracks, `local/evidence/locked-tours/sweep`) | 19 exact over the whole window; 41 TWO LOOPS exact to 1,252 (HUNTER tag) |
| cold start (16, `local/evidence/track-breadth/track-breadth-2/sweep`) | 16 exact, unchanged |

LAST ONE, JUMPOVER, DOWN+UP and HIGHROAD moved from 808, 773, 519 and 898 to their whole windows
(1,512, 1,486, 1,507, 1,517). New captures with Right held from frame 1,500 and released at
2,600 (`local/evidence/tile-pairs/right`) are exact over their whole windows too. On DOWN+UP
the player rides the loop, steps 1-16. Pair 8 is reached only by the opponent, once, on
HIGHROAD.

**The app.** Hidden `frontend run` with Right held: LAST ONE 2,000 updates, and JUMPOVER and
HIGHROAD 4,000 each, all clean with no rider-pose fallback frames.

**Pictures.** On DOWN+UP with Right held, 17 native frames from 1,920 to 1,952 (the player's
first loop) match the original with 0 differing pixels. For most of them the rider is inside the
tube and hidden; it reappears at 1,952. The render needed a local build with the result-title
check below relaxed; that change was not committed.

**A presentation defect found here.** The result-title font (`result_title_tile`) covers
0-9 and a-z. Three track names use other characters: `down+up` (25), `boo!` (28) and
`to_and_fro'` (44). For a one-run race (25 and 28) the native presentation checks the name when
it loads the track's content and refuses it. So since LOCKED-TOURS the app cannot start
DOWN+UP or BOO!, though their simulation is exact. The font's slots after z hold a blob, two
arrows and two unclear shapes (`local/evidence/tile-pairs/glyphs.py`); which tile the original
draws for `+` and `!` is not known yet ([RESULT-TITLE-GLYPHS](../../tasks/RESULT-TITLE-GLYPHS.md)).

## Limits

- Pair 4 is unrecovered; no capture reaches it.
- Pair 12 has been met by the opponent only (JUMPOVER, 24 updates). Pair 8's counter is never
  seen above 1.
- The HUNTER tag effects are unimplemented (HUNTER-EFFECTS), so HUNTER races diverge after the
  first tag.
- The stunt events are not compared.
- The app refuses DOWN+UP and BOO! at load: their names' `+` and `!` have no known title glyph
  (RESULT-TITLE-GLYPHS).

## Reproduction

```sh
python3 tools/project.py content pack --rules tests/manifests/content/classic-crawler-tracks-pack.json \
    --out local/classic-pal-crawler-tracks-v13.pack
python3 -m tools.unirally_lab.native.track_reference recompare --per-track --sweep local/evidence/locked-tours/sweep \
    --binary build/lab-debug/src/core/zoom_zoo_runner --pack local/classic-pal-crawler-tracks-v13.pack --out <json>
local/evidence/tile-pairs/captures.sh   # the Right-held captures
```
