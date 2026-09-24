# R-0049 - New tracks' finishes: the finish phase counts race updates

Status: recovered and implemented on `task/race-finish-breadth` (RACE-FINISH-BREADTH), 24
September 2026. PAL ROM `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`,
audited bsnes core `e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b`.

## The finish slowdown's phase

After a rider finishes, `$83:E8E0-EA72` brakes it by 10 on two updates of every three. It
skips the third while bit 0 of `$0304` is set (`$83:E90D-E915` for the player, `$83:EA9F` for
the opponent). `$0304` counts 0, 1, 2 once per race update (`$83:CCAB-CCB5`), and nothing
resets it at race setup. On all 20 part 2 captures it is 0 at the initialization boundary
and 1, 2, 0 on the next three updates. So it is the update's number from the boundary, mod
3.

Native read `(frame + 1) mod 3` from the absolute frame. That agrees only when the boundary
frame is 2 mod 3, as it is for DRAGSTER (1,328) and ZOOM ZOO (1,376), but not for 9 of the 14
other cold-start tracks (R-0046). Native now takes the phase from the scenario's
initialization frame (`race_update_phase`). DRAGSTER's legacy path keeps its frame-based form,
which its boundary makes equal.

Limit: `$0304` also runs through a whole session, so a race that is not the first after power-on
may start at another phase. Native's Race Again restarts the same clean scenario at phase 0,
as it did before (the M4-16 restart scope).

## Comparing through the result

`track_reference` used to stop at the first finish. It now keeps projecting through both
finishes and, from the update on which the player's finish display reaches 240 (the result
load), archives the last race row as `zoom_zoo_playable` and `dragster_playable` do. After that
only the load clock (to 115 for a lap race, 226 won or 242 lost for a one-run race),
the graph extrema and the published totals advance, and the lap and total slots are checked
against SRAM on every update. It reports the first visible result frame the same way (after
load + 75, the first picture unlike the black one). `explore` now drives native with the
capture's own controller, update by update. `capture --hold` repeats, giving a schedule of
segments.

## Evidence

Captures under `local/evidence/race-finish-breadth/`, compared with `explore`:

| Track (boundary mod 3) | Input | Finishes (frame) | Result load | Result (exact rows) | Old rule |
| --- | --- | --- | --- | --- | --- |
| EAST (2) | Right | both 5,597 | 5,838, visible 5,946 | **6,598 of 6,598** | identical |
| WARIO PAINT (1) | Right and B | player 5,381, opponent 5,141 | 5,622, visible 5,730; lost, 242 | **4,911 of 4,911** | diverges at 5,142 |
| FLAT FUN (0) | Right, then Left from 3,760 | player 5,661, opponent 5,670 | 5,902, visible 6,010; won | **5,209 of 5,209** | diverges at 5,662 |
| HYBRID (0) | Right | opponent 3,703 | - | **6,615 of 6,615** | diverges at 3,705 |
| MONSTER (0), lap race | Right | opponent 6,991 | - | **6,582 of 6,582** | diverges at 6,993 |
| SHORT CUT (1), lap race | Right | opponent 6,939 | - | **6,599 of 6,599** | diverges at 6,941 |
| DRAGRACE (1) | Right | opponent 3,965 | - | **6,641 of 6,641** | - |
| WARIO PAINT (1) | Right | opponent 5,141 | - | **6,611 of 6,611** | - |

"Old rule" is the same native with the frame-based phase restored for the run. Each first
visible result is load + 108, as on the accepted tracks. The part 2 recompare is unchanged
(all 16 exact over their windows); none of its windows reaches a finish. In the app, a hidden
7,000-update run on WARIO PAINT with Right and B reaches a stable result (242 load updates) with
0 fallback frames.

## Limits

- No new lap race's player finish or result is captured. With simple held inputs (Right,
  Left, with or without jump) the player does not complete three laps on MONSTER, MEGAJUMP,
  SHORT CUT, HAIRPIN HILL or HYBRID in 9,000 updates of native search. The lap result load
  is the accepted ZOOM ZOO path, and its phase is the one recovered here.
- The result screens' pictures are outside this comparison: lap races keep their authored
  result screen (declared), and one-run results were checked for EAST and FLAT FUN in R-0046
  observation 18.
- A session's later races may start at another `$0304` phase (above).

## Reproduction

```sh
local/evidence/race-finish-breadth/captures.sh
python3 -m tools.unirally_lab.native.track_reference explore --reference local/evidence/race-finish-breadth/<capture> \
    --binary build/lab-debug/src/core/zoom_zoo_runner --pack local/classic-pal-crawler-tracks-v11.pack \
    --scenario classic.track.NN --out <json>
```
