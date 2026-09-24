# R-0047 - The special tiles: lift, mud, corkscrew and the jump-driven tile

Status: recovered and implemented on `task/special-tile-response`
(SPECIAL-TILE-RESPONSE), 24 September 2026. PAL ROM
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, audited
bsnes core `e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b`.
Extends [R-0011](R-0011-contact.md), [R-0024](R-0024-zoom-zoo-vertical-contact.md)
and [R-0025](R-0025-zoom-zoo-reflected-vertical-contact.md) (the vertical contact), and
answers next experiment 2 of [R-0046](R-0046-track-breadth-matrix.md) (the
special-tile stop, the commonest in the track matrix).

## What stopped native

TRACK-BREADTH left native stopping at `vertical contact reaches a special response
tile` whenever a rider's selected tile carried a flag outside {0, 2, 6, 7, 18, 20}.
With a temporary print in that guard, on the part 2 captures (controller released),
every stop was the opponent, on three flag values:

| Flag | Tracks (update of the stop) |
| ---: | --- |
| 10 | MONSTER (361), MEGAJUMP (531), SHORT CUT (1,483), WARIO PAINT (1,719) |
| 14 | SWITCHER (384), DRAGRACE (1,353) |
| 25 | PINGPONG (1,260), CROCK (1,731), HAIRPIN HILL (302) |

Once those were recovered, a longer WARIO PAINT capture reached flag 16 at update
2,184. No other flag outside the old set is touched in any capture of this task.

## Two dispatches on the flag pair

The original never tests a flag value directly: it takes the pair `flag & $FE` of
the selected tile (`$0DE7`, the flag byte of the tile named by the selected word
`$0F13`) and dispatches twice per update.

1. **Vertical contact, `$81:9185-91D1`** (after `$81:8F9A` has snapshotted the
   incoming unsupported count):

   | Pair | Effect |
   | ---: | --- |
   | 2 | `$0F39` = `$0F41` = 1 (already in native) |
   | 8, 16 | `$1349` = 1 |
   | 24 | `$0F33` (unsupported count) and `$0FBF` (duration) cleared as whole words |
   | 26 | if `$0355,y` = 9, the probe penetrations `$28`/`$2C` are cleared (never executed) |
   | any other | nothing |

   So flags 10 and 14 have no contact response of their own: native's old guard
   was wider than the original's branches. `$1349` is read only at `$81:9685`, on
   `$81:966F-9690`, which continued contact (`$81:9610`, entered only from
   `$81:92C5`/`$81:92D0`) reaches only for an angle magnitude of 31 or more. The
   response has already taken every such magnitude at `$81:9286`, and no coverage
   capture executes the path (the static map classes it inferred). For pair 16
   `$1349` is therefore inert; pair 8 also changes the correction (`$81:92D9`,
   `$81:96FF`, `$81:97E6`, `$81:9930`) and stays guarded, as does pair 26.

2. **Movement, the table at `$81:82F5`**, called through `$81:82B7` from the
   movement driver (`$82:8C45` player, `$82:912F` opponent) after the per-update
   reset `$81:858E` and unless the auxiliary flag `$0EF1` is set (or `$0B8E,y`,
   which only pair 4 sets). `JSR ($82F5,X)` with X = pair; 15 entries:

   | Pair | Handler | Native |
   | ---: | --- | --- |
   | 0 | none (`BNE` skips the call) | - |
   | 2 | `$81:871C` boost | already recovered |
   | 4 | `$81:875C` | guarded (TO AND FRO' only) |
   | 6 | `$81:84EC` | already recovered |
   | 8 | `$81:8554` | guarded |
   | 10 | `$81:87C2` corkscrew | **recovered here** |
   | 12 | `$81:8950` | guarded (MARATHON, JUMPOVER only) |
   | 14 | `$81:8999` mud | **recovered here** |
   | 16 | `$81:89F7` jump-driven push | **recovered here** |
   | 18, 22 | `$81:84AC`, a bare `RTS` | nothing |
   | 20 | `$81:8050` checkpoint tile | already recovered (`update_zoom_checkpoint`) |
   | 24 | `$81:84B2` lift | **recovered here** |
   | 26 | `$81:837E` | guarded |
   | 28 | `$81:8316` | guarded |
   | 30-58 | through the bytes after the table (`CPX #$3C; BPL` skips only 60 and up) | guarded |

   Native used to handle pairs 2 and 6 and silently ignore every other pair here;
   it now throws on the guarded ones, naming the pair.

The per-update reset `$81:858E-871B` runs before the dispatch and carries the
counters the handlers leave: `$0F45` counts down (else `$0F47` is cleared, with sound
`$0213`); with `$0F49` = 0 it ends the corkscrew step `$0FA7`, clears a `$0600-$060F`
pose override `$0F59`, and clears `$0F4B` unless the contact is inverted; `$0F49`
counts up from a negative value and is otherwise cleared; `$0547,y` counts down;
`$0F5B` is always cleared (`$81:8622`); and while surface mode `$0F23` was clear
`$0F2F` counts down (`$81:859D`). `$0F3B`, `$0F3F`, `$0F27`, `$0F39`, `$0F41` and
`$11F1` are cleared.

## The recovered tiles

**Lift (pair 24, `$81:84B2-84EB`).** Away from leading support: launch override
`$11F1` = `$50`, x moves one unit along the descriptor's facing (`DEC $A5` when
`$0F13` bit 14 is set), velocity y falls by `$40` but not below `$FDE0` (`CMP #$FDE0;
BPL`, an N-flag compare), and `$0F39` = `$0F41` = 1. With the contact dispatch
clearing the unsupported counters, a rider on it stays at count 1. Checked on
HAIRPIN HILL's WRAM at update 302 before implementing (x `$1A01` to `$1A00`,
velocity y `$FD4F` to `$FDE0` then gravity to `$FDF3`).

**Mud (pair 14, `$81:8999-89F6`).** On the first update (`$0F45` = 0) velocity x
halves by an arithmetic shift and velocity y is zeroed (sound `$0212`). Every update
on it sets `$0F45` = `$0F47` = 4 and `$0F27` = 1, then, unless velocity x is already
within 48 of zero (`$0030`/`$FFD0`, N-flag compares), brakes it by 5 and sets `$0F3F` =
velocity x and `$0F3B` = 4. The consumers:
- the drive routines `$82:A9B3`/`$82:AA10` step by `$0F3B` instead of 24;
- the low-speed damping `$82:A5FA` is skipped while `$0F3B` is set;
- with the brake held, `$82:998B` drops the throttle `$0F5F` and the charge latch;
- the pose target `$83:EFAD` follows `$0F3F` instead of velocity x.

**Corkscrew (pair 10, `$81:87C2-894F`).** Entry needs `$0F49` not negative, no
reflection step (`$0F61`), an upright descriptor, a previous unsupported count
(`$0F4F`) below 3, and facing and velocity along the descriptor (reflected: velocity
x not negative and bit 14 set; otherwise velocity x not positive and bit 14 clear).
Anything else ejects: `$0F49` = -4, the boost handler with `$40` added to its push,
step `$FFFF` (sound `$021B` on the first). Entered, `$0F49` = 1, and while the step is
below `$30` each update:
- velocity x is set to `$01CE` (reflected) or `$FE32`, velocity y to 0;
- `$0547,y` = 8, `$0F4B` = `$0F5B` = `$0F41` = 1, `$11F1` = `$50`;
- the step advances and the pose override is `$0600 + (step / 2 mod 16)`;
- y moves by the step's entry in a 48-byte signed table, `$00:8088`, or
  `$00:80B8` for a rider whose rolling flag `$0F15` is set;
- at steps 1, `$12`, `$20` and `$2F` the object attribute `$0FAF` has bit 4
  toggled (OBJ priority 2 and 3), and step 0 resets it to `$26`;
- `$0EA3` is set to 1 by **absolute address** (see below).

At step `$30` the rider leaves reflected the other way (orientation `$15` or `$2B`),
upside down (`$0F31` = `$80`), with surface mode set and the reflection locked for 6
updates (`$0F2F`); step `$31` then only holds the launch override.

Its words reach seven other routines: `$0F49` skips the completed-turn hold
(`$82:A241`), the reflection transition (`$82:A40B`, with `$0F2F` locking it whole at
`$82:A35D`) and the rotation, which it clears (`$82:A4CC`); `$0547,y` skips the whole
drive routine (`$82:98D6`), the speed limiter (`$82:A6FD`) and, with `$0F4B`, gravity
(`$82:A971`); and `$0DFB`/`$0DFD` skip that rider's whole contact update
(`$81:8CFC`, `$81:8E43`).

**Jump-driven tile (pair 16, `$81:89F7-8A29`).** With jump (`$0F1F`) held, velocity
x moves 4 the way the D-pad points (none when neutral); otherwise `$0F39` = 1.

## Three findings beyond the handlers

- **The opponent's corkscrew sets the player's rolling flag.** `$81:8949 STA $0EA3`
  writes the player's persistent word even while the opponent is processed. The
  player's own update has already stored its flag, so the 1 survives to the next
  update. MONSTER and MEGAJUMP diverged on `player.rolling` until native did the
  same, and only on steps that reach `$8949` (not an ejection, step `$30` or `$31`).
- **The rotation keeps its step under surface mode and leading support.** In
  `$82:A49F` both rotations, a shallow supported contact or `$0F49` clear
  `response_b`; then surface mode (`$82:A4DC`) and leading support (`$82:A4E4`)
  return with it unchanged; only after them does no rotation clear it. Native cleared
  it for no rotation before those tests. A rider leaving a corkscrew is inverted on
  pair-6 tiles with surface mode set, and MONSTER and MEGAJUMP diverged there. The
  same fix removes PINGPONG's divergence at update 1,068 (R-0046 next experiment 4).
- **`$0E7B` is one word for both riders.** The drive routine sets it (1, or 0 on
  its small-displacement path) and the pose and idle routines read it. A corkscrew's
  physics hold skips the drive routine, so the rider reads the other rider's value.
  Native carries it in state.

## Native

- `src/core/vertical_contact.cpp`: the contact dispatch (pair 24 clears the
  counters; pairs 8 and 26 guarded).
- `src/core/movement.cpp`: the dispatch by pair with a guard for the unrecovered
  ones, `update_special_tile_counters`, `update_mud_tile`,
  `update_corkscrew_tile`, `apply_boost_tile` (pair 2, and the ejection), the lift and
  pair 16 inline, and each consumer above.
- **State.** `SpecialTileRider` per rider (`$0BCB`, `$0D57`, `$0DF7`, `$0DF3`, `$0DFF`,
  `$0547`, `$0BE7` and bit 4 of `$1516`/`$151A`) and `drive_target_latch` (`$0E7B`).
  The other tracks' state always carries them: `URTRnn02`, the 742-byte layout plus
  34 bytes (776). DRAGSTER and ZOOM ZOO keep their 742-byte layouts (`URDG0001`,
  `URZZ000B`) while every special-tile word is zero, and take the same 776-byte
  extension (`URDG0002`, `URZZ000C`) only while one is live; the extension with every
  word zero is refused as non-canonical. No accepted race reaches a special tile, so
  no frozen contract moves, but ZOOM ZOO's own tile table holds the corkscrew (pair
  10) and a live race there must not abort when its state is saved (review finding
  1: the first candidate refused such a state). `$0E7B` travels with the extension:
  without a physics hold each rider's drive rewrites it before it is read. The words the tiles set for one update only
  (`$0F3B`, `$0F3F`, `$0F5B`) are locals.
- **Pack profile v11** (`classic.pal.crawler.tracks.v11`, 148 entries) adds
  `zoom.corkscrew-heights`, the 96 bytes `$00:8088-80E7`.
- **Presentation.** Bit 4 set, the rider object is drawn above BG1 tiles whose map
  entry has bit 13 set (OBJ priority 3 is above them in mode 1).
- `track_reference` appends the same 34 bytes to the original's projection for any
  track but DRAGSTER and ZOOM ZOO, so the comparisons include them.

## Evidence

All compared with `track_reference` (`recompare --per-track` on the part 2 sweep,
`explore` on new captures), native from its own initialization, bytes 12 onward.

| Track | Capture | Before | After |
| --- | --- | --- | --- |
| SWITCHER | part 2, released | 384, special-tile guard | **exact 1,483 of 1,483** |
| MONSTER | part 2, released | 361, special-tile guard | exact 809, then HYBRID's `inverted AI marker` guard (out of scope) |
| MEGAJUMP | part 2, released | 531, special-tile guard | **exact 1,533 of 1,533** |
| DRAGRACE | part 2, released | 1,353, special-tile guard | **exact 1,541 of 1,541** |
| PINGPONG | part 2, released | 1,068, `opponent.response_b` | **exact 1,534 of 1,534** |
| SHORT CUT | part 2, released | 1,483, special-tile guard | **exact 1,499 of 1,499** |
| HAIRPIN HILL | part 2, released | 302, special-tile guard | exact 531, then INFINITY's checkpoint guard (out of scope) |
| CROCK | new, released to frame 4,400 | (stop at 1,731) | **exact 3,004 of 3,004** |
| WARIO PAINT | new, released to frame 4,400 | (stop at 1,719) | **exact 3,011 of 3,011** (pair 16 at 2,184) |
| SWITCHER | new, Right held from frame 1,430 | - | **exact 1,983 of 1,983**, both riders on mud |
| SHORT CUT | new, Right held from frame 1,414 | - | **exact 1,999 of 1,999**, the player ejected from a corkscrew |
| MEGAJUMP | new, Right held from frame 1,380 | - | **exact 2,033 of 2,033**, the player through a whole corkscrew |
| WARIO PAINT | new, Right held from frame 1,402 | - | **exact 3,011 of 3,011**; the player's corkscrew uses the second height table |
| WARIO PAINT | new, Right and B (jump) held from frame 1,402 | - | **exact 3,011 of 3,011**; the player on pair 16 with jump held for 43 updates |

The other nine compared tracks are unchanged (exact over their windows, or stopped at
the same out-of-scope guard). The WRAM of these captures agrees with the new state
words on every row, since they are part of the compared projection.

**Pictures.** On MEGAJUMP with Right held, 16 frames from 1,904 to 1,962 (the
player's corkscrew) match the original with 0 differing pixels. With the priority bit
ignored, 6 of 8 of those frames differ by 3 to 160 pixels, the rider hidden behind the
tube. On MONSTER (opponent's corkscrew) and SHORT CUT the only differences are the
declared off-screen rider arrow (36 to 108 pixels).

The independent review re-ran the recompare and made four captures of its own, none
used here as evidence before: WARIO PAINT with Right held and with Right and jump held,
CROCK with Right held and DRAGRACE with Right held. All four are exact to their ends
(3,011, 3,011, 3,004 and 2,605 rows). The two WARIO PAINT cases were then recaptured
into this task's evidence directory: they are the table's last two rows.

**Idle matrix.** `content track-idle-matrix --updates 1200` (every track on ZOOM ZOO's
scenario, controller released) now completes on 39 of the 45 tracks, against 16 in R-0046
observation 4. It stops on BOWL (pair 8 in contact, update 7), LAST
ONE and DOWN+UP (pair 26 in contact, 807 and 518), MONSTER and HAIRPIN HILL (the AI-marker
guard, 808 and 773, on this scenario), and LITTLE DIPPER (its shape).

**Live.** Hidden 4,000-update runs with Right held (`frontend run --hidden
--fixed-controller-mask 128`) complete with 0 fallback frames on WARIO PAINT, CROCK,
SWITCHER, MEGAJUMP, DRAGRACE, PINGPONG and SHORT CUT. MONSTER stops at the AI-marker
guard and HAIRPIN HILL at the checkpoint guard, both out of scope.

## Which tracks hold which pairs

Read statically from each track's tile-flag table (`tracks.tile_content`); a pair in a
table is not a pair a rider touches.

| Pair | Tracks holding it | Status |
| ---: | --- | --- |
| 8 | 20 tracks, 10 reachable (BOWL, MONSTER, MEGAJUMP, WARIO PAINT, CROCK, ...) | guarded; no capture touches it |
| 28 | 38 tracks, DRAGSTER and ZOOM ZOO included | guarded; no capture touches it |
| 26 | LAST ONE, DOWN+UP, VERTICAL (none reachable) | guarded |
| 12 | MARATHON, JUMPOVER (not reachable) | guarded |
| 4 | TO AND FRO' (not reachable) | guarded |

## Limits

- Audio is a declared omission: the sounds `$0212`, `$0213` and `$021B` are not played.
- The corkscrew's ejection is exercised once, on SHORT CUT (player, Right held).
- Pairs 4, 8, 12, 26 and 28 in movement, and 8 and 26 in contact, stay guarded: a
  track that touches one still aborts, with the pair named.
- The static reading that `$81:966F-9690` is unreachable rests on the listing and on
  its absence from every coverage capture.

## Reproduction

```sh
python3 tools/project.py content pack --rules tests/manifests/content/classic-crawler-tracks-pack.json \
    --out local/classic-pal-crawler-tracks-v11.pack
python3 tools/project.py build --preset lab-debug
python3 -m tools.unirally_lab.native.track_reference recompare --per-track \
    --sweep local/evidence/track-breadth/track-breadth-2/sweep \
    --binary build/lab-debug/src/core/zoom_zoo_runner --pack local/classic-pal-crawler-tracks-v11.pack --out <json>
python3 -m tools.unirally_lab.native.track_reference capture --core <pinned core> --tour-row 1 --track 1 \
    --horizon 3400 --hold 1380 right --out <fresh dir>          # MEGAJUMP, Right held
python3 -m tools.unirally_lab.native.track_reference explore --reference <that dir> \
    --binary build/lab-debug/src/core/zoom_zoo_runner --pack local/classic-pal-crawler-tracks-v11.pack \
    --scenario classic.track.11 --out <json>
```

The captures of this task are under `local/evidence/special-tile-response/` in the
main checkout, with `captures.sh` and `gates.sh`.
