# R-0107 - The race branches known only from the listing

Status: research result ([RACE-LISTING-BRANCHES](../../tasks/RACE-LISTING-BRANCHES.md)), 7 November
2026, on main `2d516755`. PAL ROM SHA-256
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, reference core bsnes `7d5aa1e6`
(Strict). Evidence in main `local/evidence/race-listing-branches/`. It closes R-0089's queue item 8:
the HUD fields and tile behaviours R-0089 listed as reached by no capture.

Tags: **[L]** read from the static map's listing, regenerated for this task with the five raw
coverage inputs (`listing/bank-8*.lst` in the evidence folder); **[S]** a scan of the ROM's track
data (`tile_scan.py`, `marker_scan.py`: all 45 tracks, all 16,384 coarse cells, 16 fine cells
each); **[C]** a capture of the original made for this task or kept by an earlier one; **[N]** the
original's behaviour read through native where native's race rows equal the original's on every
update of the capture's window (so native's states are the original's), with a temporary trace
build of the runner (`tile_trace.py`; the trace was never committed).

## Summary

R-0089's two groups are really five behaviours. All but two are reachable in the shipped game,
and native already modelled every reachable one: no native code changes. The captures made here
show each reachable behaviour on the original, and native equals them on every row and every
picture. Not every reachable form is captured: the two-view arrow's other forms, rider 1's stunt
score of 100 or more and three tile arms (Not covered) are queued as
[RACE-BRANCH-REMAINDER](../../tasks/RACE-BRANCH-REMAINDER.md).

- The "lap and split fields drawn for `$FFFF` or `$1253 < 1`" are the direction arrow's **up and
  down** forms (R-0063 recorded them as listing only). `$0FD9`/`$0FDB` hold the arrow to erase:
  `$FFFF` a down arrow, `$FFFE` an up arrow. `$1253` is the chevron count, below 1 on the blink's
  empty step.
- `$81:C3DC` is not "scores of 100 and more": it splits **rider 1's** stunt score (`$77:0825` into
  `$12BB`), which TWO-HUMAN-STUNTS implemented (R-0095).
- **Tile pair 4** (`$81:875C`) is unreachable: no track's map holds a cell of it. Native keeps
  refusing it.
- `$81:84B2` (the lift, pair 24), `$81:831D` (the push, pair 28, unlocked) and `$81:83E7` (the
  loop's entry on an unmirrored descriptor) are reachable. Existing captures and one new one show
  them; native was already exact.
- `$81:8565` (pair 8's clamp at 8) is unreachable by construction.

## Census

| Branch | Condition [L] | Shipped game | Evidence | Native |
| --- | --- | --- | --- | --- |
| One-player arrow up/down: `$81:EA5E-EAC2` (up), `$81:EB2C-EB83` (down), `$81:E94F-E9A9` (their erase), `$81:EA6C` and the down arrow's `$1253 < 1` exits | `$0FD5` from `$82:9822` not negative (the player behind by a transition pair, not finished, no rejected transition) with bit 1 set: the previous update's marker word's bits 15-14 are 2 (up) or 3 (down). The NMI erases the last drawn arrow through `$0FD9` (`$E954`: `$FFFF`, rows 24-26 of column 15; `$E97D`: `$FFFE`, rows 4-6, rows 5-6 left alone while `$0D19` = 1) | Reachable. Up or down markers on tracks 4, 11, 19, 23, 24, 26, 28, 31, 34 and 40 [S] | t23-seed102 (up, 5 updates), t04-seed69 (up, 2), t34-seed124 (down, 20; `$0FD9` = `$FFFF` on 18 frames) [C] | `classic_race_arrow`, `ClassicRaceHudClock::redraw_arrow` (`race_hud.cpp`); unchanged, equal on every picture |
| Two-view arrow up/down: `$81:D8D9-D933` (erase, upper), `$81:D9E8-DA4C` (upper up), `$81:DAB6-DB0D` (upper down), `$81:DB70-DBCA` (erase, lower), `$81:DC7F-DCCA` (lower up), `$81:DD4D-DDA4` (lower down) | The same words for the upper view (`$0FD5`, `$0FD9`) and the lower (`$0FD7`, `$0FDB`, rider 1's marker) in a two-view race (`$0DE1`) | Reachable: two-view races (2P, VS, a league's two-human races, the idle demo) run on the tracks above | Upper down: the idle demo on track 26 (`$77:10C8` = 25, `$1115` = 0 written after frame 600 of a cold start, or a warm boot with them) at frames 2498-2503 [C], and the rotation's own track 26 demo in the cold idle capture `idle-demo-cycles/cold-102000` at 63469-63471, erased through `$0FD9` = `$FFFF` (`$81:D8D9`) [C]; lower up: R-0089's corpus run 069 (a league's two-human race) at 32815 [C] | `draw_split_arrow` (`race_hud.cpp`, R-0080); unchanged, equal on every compared picture |
| `$81:C3DC-C43B`, rider 1's stunt score | `$77:0825` differs from `$12BB` (`$81:C3D7`): split into `$12BF/$12C3/$12C7` and `$12CB` set | Reachable in any two-human stunt event | TWO-HUMAN-STUNTS' twop-bowl, coverage captured here: all of it but `$81:C3EA-C3F0` (rider 1 at 100 or more) [C] | R-0095's split score cells; unchanged |
| Scores of 100 and more, 1000 and more | `$81:C37A-C385` loops while the score is 100 or more; from 1000 the hundreds count is 10 or more and indexes a letter of `$80:81F4` | 100 and more: reachable (hill-win, 121, R-0068). 1000 and more: not established | `$81:C37F-C385` observed [L, C] | `stunt_score_cells`; a letter for 10-35 hundreds (`hud_character`), refused past the table |
| Tile pair 4, `$81:875C-87BF` | The selected word's flag `& $FE` = 4 (`$81:82C2-82F0`) | **Unreachable.** Only track 44's flag table holds pair 4 (tiles 203 and 204); none of the 45 maps has a fine cell that indexes them [S]. `$0F13` is a sampled map word (`$81:8B66`, `LDA $7F800F,X`); the one other source, `$81:9140-9160` (inferred, never observed), swaps a word with bit 0 set for one read from `$7E:2132`, and no fine word in any of the 45 maps has bit 0 set [S] | `tile_scan.json`; `tile_scan.py --check` | Refused (`tile_behavior_under` throws); kept |
| Pairs 30 and more (past the table at `$81:82F5`) | `CPX #$3C; BPL` skips only 60 and up | Unreachable from the tables: no track's flag table holds a pair above 28 [S] | `tile_scan.py --check` | Refused; kept |
| The lift, pair 24, `$81:84B2-84EB` | Pair 24; `$81:84B7`: leading support `$0F5D` returns at once | Reachable: tracks 7, 15, 21, 31, 34, 35, 39 and 40 hold it [S] | Plain on track 7 (locked-tours jumper-2, 167 updates), mirrored on tracks 21 and 34 (4 captures, 158 updates, t34-seed124's 72 among them) [N, C] | `run_lift_tile`; unchanged |
| The push, pair 28, unlocked: `$81:831D-8349` | Pair 28 with the reflection lock `$0F2F` clear | Reachable: tracks 15, 25 and 35 hold it [S] | Earlier captures reach it only locked (DOWN+UP after its loop, 8 updates) [N]; t25-seed43 (DOWN+UP): plain 20, mirrored 26 [N, C] | `run_push_tile`; unchanged |
| The loop's entry on an unmirrored descriptor, `$81:83E7-840C`; its refusal `$81:839C-83AE` and count back `$81:85B3-85B7` | Pair 26 at step 0: refused when rising, on leading support or with a pose override outside `$0610-$061F` (step `$FFFE`, counted back by the reset); else the descriptor's facing (`$4000`) picks the arm | Reachable: tracks 15, 25 and 35 [S] | Plain entries in locked-tours runner-0 (6) [N]; t25-seed43: plain entries 10, one refusal (pose override `$07A1`), 20 count-back updates [N, C] | `update_loop_tile`; unchanged |
| Pair 8's clamp at 8, `$81:8565-8582` | `$0F29 + 1` = 8 | **Unreachable by construction**: `$0F29` comes only from `$0D3D`/`$0D3F` (`$82:8A13`, `$82:8F1B`) and goes back to them (`$82:8D2B`, `$82:9219`); the per-update reset `$81:8594` (one `DEC`, held at 0) runs before the dispatch in both drivers (`$82:8C3A` before `$82:8C45`, `$82:9124` before `$82:912F`, with no branch into the dispatch) [L]. So the count, once 0 or 1, is 0 before the `INC` and at most 1 after. It starts at 0: `$0D3D`/`$0D3F` hold `$FFA0`/`$0080` at power-on until the boot's work-RAM clear, then 0, and never rise above 1 after it in any of the 75 raw work-RAM captures (73 stay at 0, 2 reach 1; `wram_census.py`'s `slow_counter_before_clear` and `slow_counter_max_after_clear`) [C] | No capture (65 exact) and no native schedule (300) counts above 1 [N] | `run_slow_tile` implements the clamp (R-0051); it is dead code there too, left as it is |

Two smaller observations:
- **Track 13 reads past its flag table.** One fine cell of its map (world x 21376, y 928, word
  `$42E6`) indexes tile 187 of a 183-tile table [S]. On the cold route the byte there, `$7E:C0BB`,
  is 0 (track-breadth row1-pos3, every frame) [C]. Native refuses the cell ("tile flag is missing").
  Whether a rider can reach it, and what a stale byte left by an earlier track would do, are open.
  A stale byte cannot be pair 4: no track's flag table holds pair 4 at index 187 (only track 44's,
  at 203 and 204), so only non-table data left there could matter.
- `$81:834C-837C` is not code: it is the loop's 17 offset words (R-0051's `zoom.loop-offsets`).

## How the reachable branches were reached

- **The arrows.** `search_arrow.py` ran native's runner (exact on these cold-start tracks) on
  random schedules: an idle opening of 300 updates, so the opponent leads, then random holds. Of
  900 one-player schedules, 33 gave a down arrow on track 34, 48 an up arrow on track 4, and 8 on
  track 23. `capture_seed.py` then captured the original on three of them. Each arrow appeared in
  the original's `$0FD5` at the update native had predicted.
- **The two-view arrows.** Two-pad 2P schedules on track 34 (`split/search_split.py`: 58 logged in
  `split/search-34*.log`, and 8 earlier tries with no log: a first test, two schedules aligned with
  the one-player ones, and 5 on track 23 whose log a concurrent run removed) found only a 3-frame
  upper down arrow. Rider 1's random riding made little progress,
  and the riders interfere: with pad 2 holding Left, rider 0 passed 6 transitions by update 1300
  instead of the 26 it passes alone. The idle demo was easier. A demo's track and view come from `$77:10C8` and `$77:1115`
  (R-0087), so `split/demo_search.py` set them after a cold start's frame 600, as a manifest's
  `cartridge_ram_writes` do. (The rotation's own track 26 demo in `cold-102000` shows the same
  arrow at 63469; it is a two-view race too, so the one-view down arrow rests on t34-seed124.) Of 64 demos on tracks 4, 11, 24, 26, 28, 31 and 34, track 26's shows
  the arrow; `$10B1` changed nothing in it. The lower view's up arrow was already in R-0089's corpus
  (run 069); the coverage files say where.
- **The tiles.** `tile_search.py` ran a traced native runner on 300 random schedules on tracks
  15, 25 and 35. It counted 2,033 mirrored and 767 plain unlocked pushes, 1,370 plain loop
  entries, 1 refusal and 20 count-back updates. Track 25's seed 43 has all of them, and
  `capture_seed.py` captured it with the tours unlocked.

## Results

Main `2d516755`'s binaries (the candidate's are the same: no native change), pack v41:

| Check | Original | Native |
| --- | --- | --- |
| t23-seed102, track 23 (up arrow) | 701 rows, 699 pictures | rows 701/701 exact; pictures 699/699 equal |
| t04-seed69, track 4 (up arrow) | 1,301 rows, 1,299 pictures | 1,301/1,301; 1,299/1,299 |
| t34-seed124, track 34 (down arrow, lift) | 1,401 rows, 1,399 pictures | 1,401/1,401; 1,399/1,399 |
| t25-seed43, DOWN+UP (push both ways, loop entries, refusal, count back) | 3,002 rows, 3,000 pictures | 3,002/3,002; 3,000/3,000 |
| The idle demo on track 26 from a warm boot (two views, upper down arrow) | 1,906 pictures, boundary 1432 to the exit request 3338 | 1,905/1,905 equal (the race's initialization frame 1438 has no native picture: the runner gives that frame to the race's setup) |
| R-0089's run 069 (league, two humans, lower up arrow at 32815) | 81 pictures, 32780-32860 | 81/81 |
| The cold idle demo (`cold-102000`: the rotation's track 26 demo, two views, upper down arrow at 63469) | 61 pictures, 63440-63500 | 61/61 |
| The 65 kept race captures with full work RAM, through the trace | every row of each | all 65 exact over their whole windows |

Pictures are compared with the core's own per-frame video digest (XRGB8888, SHA-256), so a match
is pixel-exact and a mismatch is not measured.

**The checks fail when the behaviour breaks** (`mutants.txt`). Up and down swapped in the arrow's
direction: t23-seed102 4 pictures differ, t34-seed124 10, the warm demo 2. The plain push moved by
7 instead of 8: t25-seed43 fails. So do the mirrored push moved by -7 and a loop refusal that does
not store `$FFFE`.

**The coverage runs** (`coverage_captures.sh`): manifests from the captures' timelines
(`timeline_manifest.py`), the cold demo route and twop-bowl. The three cold-start runs equal their
track_reference captures on every frame's work RAM and video digest (987, 1,708 and 1,620 frames).
Track 25's manifest cannot preload the unlocked tours, so it writes the same bytes after frame 600.
Its pictures differ only on the menu frames 1100-1221, and every race picture is the same. At frame
2000 its work RAM differs in 29 bytes (direct page and its copies) and its cartridge RAM in 44,
left by the boot's different path (`$77:10D0`, `$77:10FE-10FF` and `$77:1FFF` differ from frame
1150). `coverage merge` joins the six into
`coverage/merged.json`, tracked as `docs/map/race-listing-branches.map.json`, the static map's
sixth coverage input.

What the sixth input moves from inferred to observed in the listed ranges [L]:
- the whole one-player arrow (`$81:E900-EB83`);
- `$81:C3DC-C43B` except `$81:C3EA-C3F0`;
- `$81:D88A`, `$81:D8D9-D906`, `$81:DAB6-DB0D` except `$81:DAC4` and `$81:DACE-DAE1`;
- `$81:831D-8349`, `$81:839C-83AE` except `$81:83A6`, `$81:83E7-840C` except `$81:83EC`;
- `$81:845D-846F`, `$81:84AD-84EB` except `$81:84AD` and `$81:84CB`;
- `$81:85B3-85B7`, `$81:85DD-85E3`, `$81:85F2-85F8`, `$81:86E0-86E1` and `$81:87D1-87F5`.

The static map regenerated with the six inputs (`regen.sh`; `docs/map/static/`): observed bytes
86,837 to 87,882, inferred 12,737 to 11,692, data 74 to 73, unknown 31,424 to 31,425 (24.0%).

## Not covered

Queued as [RACE-BRANCH-REMAINDER](../../tasks/RACE-BRANCH-REMAINDER.md), but the stunt score of 1000,
track 13 and the uncompared state:

- **Two-view arrow forms** no capture shows: the upper view's up arrow and its erase
  (`$81:D907-D933`, `$81:D9E8-DA4C`), the lower view's down arrow and its erase (`$81:DB75-DB9C`,
  `$81:DD4D-DDA4`), the up erase beside the cells (`$81:DBB5-DBCA`), three chevrons (`$81:DACE-DAE1`,
  `$81:DC92-DCCA`) and an empty blink step (`$81:DAC4`, `$81:DC8D`). They are the drawn forms of the
  same words. Native draws them by R-0080's rule; no picture of the original checks them. A
  two-pad route needs a rider 1 that leads or trails on purpose. The 58 logged two-pad schedules
  here (66 with the early unlogged tries) did not give one.
- **Tile arms not reached:** the lift on leading support (`$81:84B7`, none in 22,644 native lift
  calls); a loop descriptor facing away from the rider (`$81:83C4`, `$81:83EC`, none in 1,776
  entries); a loop entry with an override inside `$0610-$061F` (`$81:83A6`). The plain lift's
  `INC` (`$81:84CB`) is shown by track 7's capture, which has no coverage file. Whether the first
  three are reachable is not established.
- Rider 1's stunt score of 100 or more (`$81:C3EA-C3F0`): reachable in a two-human stunt event,
  but twop-bowl's rider 1 stays below 100, and no capture here shows it.
- A stunt score of 1000 or more: the same instructions with a hundreds count of 10 or more. No
  capture or estimate of the largest score was made.
- Track 13's read past its flag table (above).
- Whole-SRAM and audio are not compared; the captures compare race rows and pictures.

## Reproduction

From the worktree, with `local/` linked to the main checkout's (`E=local/evidence/race-listing-branches`):

```
python3 $E/tile_scan.py <ROM> $E/tile_scan.json --check
python3 $E/marker_scan.py <ROM> $E/marker_scan.json
python3 $E/wram_census.py <main>/local/evidence $E/wram_census.json
python3 $E/search_arrow.py build/lab-release/src/core/zoom_zoo_runner local/classic-pal-crawler-tracks-v41.pack 4,11,23,24,31,34 150 3000 300 $E/search/search-1.json
python3 $E/capture_seed.py 34 124 3000 300 2807 1407 $E/captures/t34-seed124   # 23 102 ... 2086 1386; 4 69 ... 2719 1419; 25 43 3000 0 4395 1394
python3 artifacts/trace.py && <build lab-release> && cp the runner to artifacts/trace-bin && git checkout src/core   # the trace build
python3 $E/tile_trace.py artifacts/trace-bin/zoom_zoo_runner build/lab-release/src/core/zoom_zoo_runner <pack> $E/tile_trace.json <captures...>
python3 $E/tile_search.py artifacts/trace-bin/zoom_zoo_runner <pack> 15,25,35 100 3000 0 push.plain,... $E/search/tile-search-1.json
python3 $E/split/demo_search.py 3500 600 26:0 ...; python3 $E/split/warm_demo.py <front_end_runner> <pack> <out>
python3 $E/split/league_arrow.py <front_end_runner> <pack> 32780 32860 <out>
zsh $E/coverage_captures.sh; python3 $E/coverage_same.py t23-seed102 t34-seed124 t04-seed69 t25-seed43
python3 tools/project.py coverage merge --coverage-list $E/coverage/coverage.list --scenario race-listing-branches --out $E/coverage/merged.json
python3 tools/project.py coverage map --coverage $E/coverage/merged.json --out docs/map/race-listing-branches.map.json --summary docs/map/race-listing-branches.md
bash $E/regen.sh   # native-symbols and the static map with six coverage inputs
python3 tools/project.py coverage native-symbols --check
```

`artifacts/trace.py` (the trace patch) is kept beside the evidence as `trace_patch.py`.
