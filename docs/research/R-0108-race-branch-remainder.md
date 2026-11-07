# R-0108 - The race branches R-0107 could not capture

Status: research result ([RACE-BRANCH-REMAINDER](../../tasks/RACE-BRANCH-REMAINDER.md)), 7 November
2026, on main `788efdaf`. PAL ROM SHA-256
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, reference core bsnes `7d5aa1e6`
(Strict). Evidence in main `local/evidence/race-branch-remainder/`. It closes R-0107's Not covered
list, except the items R-0107 left out of RACE-BRANCH-REMAINDER (a stunt score of 1000, track 13's read
past its flag table, whole-SRAM and audio).

Tags: **[L]** read from the listing (`race-listing-branches/listing-after/`, the static map with
R-0107's six inputs); **[C]** a capture of the original made for this task; **[N]** native's behaviour
read through a temporary trace build of the runners (`trace_patch.py`, never committed) where native
equals the original on every compared row and picture of the same route, so native's states are the
original's.

## Summary

| Branch | Result | Route | Original against native |
| --- | --- | --- | --- |
| Two-view arrow, upper view's up arrow (`$81:D9E8-DA4C`) and its erase (`$81:D907-D933`) | **Captured** | `t11-upper`: 2P, track 11; rider 0 trails rider 1 by about 23 transitions and is steered into the up markers at x 8960-9136, y 1600-1904 | 1,699/1,699 pictures; up arrow on 79 frames, 0, 1, 2 and 3 chevrons, with the cells (62) and without (17) [C] |
| Lower view's up arrow without the cells: rows 16-17 (`$81:DC92-DCCA`), the empty step (`$81:DC8D`), the up erase beside the cells (`$81:DBB5-DBCA`) | **Captured** | `t11-lower`: the same pads swapped | 1,699/1,699; up arrow on 56 frames, 0-3 chevrons, without the cells on 15 (2 and 3 chevrons among them) [C] |
| Upper view's down arrow, three chevrons (`$81:DACE-DAE1`) and the empty step (`$81:DAC4`) | **Captured** | `t34-upper`: 2P, track 34; rider 0 rides rider 1's route 651 frames later and falls through the down markers about 20 transitions behind | 2,299/2,299; down arrow on 6 frames: 0, 1, 2 and 3 chevrons [C] |
| Lower view's down arrow (`$81:DD4D-DDA4`) and its erase (`$81:DB75-DB9C`) | **Captured** | `t34-lower`: rider 1 rides the route 514 frames later | 2,169/2,169; down arrow on 6 frames: 0, 1, 2 and 3 chevrons [C] |
| Rider 1's stunt score of 100 or more (`$81:C3EA-C3F0`) | **Captured** | `vs-hill-rider1`: VS, HILL CLIMB; pad 1 idle, pad 2 a route built for rider 1's score | 2,949/2,949 pictures; 2,792/2,792 race rows, second camera and rider 1's tallies; `$77:0825` reaches 100 at frame 3870 and 152 by the end [C] |
| The lift on leading support (`$81:84B7` taken) | **Captured** | `t34-seed155`: 1P, track 34, a random schedule; the mirrored lift with `$0F5D` set on updates 1915 and 1916 | 604/604 rows, 602/602 pictures [C, N] |
| A loop entry with a pose override inside `$0610-$061F` (`$81:83A6`) | **Unreachable** by construction (below) | - | - [L, N] |
| A loop descriptor facing away from the rider (`$81:83C4`, `$81:83EC`) | **Not reached**; reachability open | 16,187 native schedules, 2,000 perturbations, none (below) | - |

No native code changed: native already drew every form and ran every arm the original shows here.
The pack profile and the race state's serialized layout are unchanged.

## How the routes were built

Random two-pad schedules give riders that interfere and make little progress (R-0107). The routes here
are built on native, which equals the original on everything these captures compare, and only the
finished routes run on the original.

- **A trace build.** `trace_patch.py` patches a copy of `src/core` (in the worktree's ignored
  `artifacts/`) so the runners print, with `RBR_TRACE` set, each race frame's transition counts,
  marker words, positions and both stunt scores, each up or down two-view arrow the HUD draws (view,
  direction, chevrons, cells), rider 1's split score writes and the lift and loop-entry arms.
- **Riding routes by receding-horizon search** (`mpc_build.py`). Each step tries constant holds and
  run-ups (one way, then the other) over the next 160 frames on native, keeps the best by the rider's
  signed transition count and then its progress along the marker's direction, and commits the first
  20 frames. A route for track 11 (2150-6510, 263 transitions) and one for track 34 (2150-4410, 102)
  came from it with the other pad idle. The track 11 route jumps over the marker pit at x 8700-9136;
  the trailing rider's route uses the same search with a target objective (the nearest approach to
  x 9040, y 1750) from frame 3300 to drop into it. The same search did not take the trailer from there on to track 11's
  down markers (x 8704-8944, y 1984-2160) in 420 frames, so the down arrows use track 34.
- **Leading and trailing on purpose.** The leader rides the route from the race's start; the trailer
  rides it later (the leader's route shifted 400 frames on track 11 until the steering starts, 651 or
  514 frames on track 34). After the first frames the riders are far apart and do not interfere.
  On track 34 the down markers are 16 cells on a drop the riders fall through in 2 or 3 frames per
  lap, so which chevron count shows depends on the blink step when the trailer falls: a scan of the
  shift on native (`dscan.py`, 400-800 frames) found 651 (upper view) and 514 (lower view) to show
  all four counts over two laps.
- **Rider 1's score.** Hill-win's one-player riding (R-0068, 121 points) on pad 2 alone gives rider 1
  at most 80 points over 31 shifts (630-690 frames). The search above with the score as objective,
  from frame 3000 and following hill-win's riding where nothing scores more, reaches 152.
- **Tile arms** (`tile_hunt.py`): the trace build of zoom_zoo_runner on random schedules (4-80 update
  holds) and on schedules with long Left holds, 16,187 runs on tracks 7, 15, 21, 25, 31, 34, 35, 39
  and 40. The lift on leading support came in 1,532 updates (track 7's right-column tour, 1,464;
  track 34, 68); track 34's seed 155 has it earliest, at update 1915 of a cold start, so
  `capture_hunt.py` captured it.

## The arrow forms

The original's words: `$0FD5`/`$0FD7` (the upper and lower arrow, written by `$82:9822`), the
transition counts `$0FCD`/`$0FCF`, the blink step `$1255` and the cells flags `$0D19`/`$0D1B`.
`$1253` is the NMI's scratch for the chevron count; by the end of a frame the lower view's pass has
overwritten it, so `pictures_2p.py` computes the count each NMI draws from the lead and the next NMI's
`$1255` ($81:E9AC-E9DB: under 10 one chevron, under 20 `2 - step` with 2 on step 3, else `3 - step`).
Those counts equal the trace build's, frame for frame, on all four routes. The NMI that draws an arrow
erases it on the next race NMI through `$0FD9`/`$0FDB` (`$FFFE` up, `$FFFF` down), so each drawn up or
down arrow is also an erase of that form.

| Route | Window | Forms the original shows (frames) |
| --- | --- | --- |
| t11-upper | 2000-3699 (init 2018) | upper up 79: 0 chevrons 16, 1 23, 2 24, 3 16; cells shown 62, not 17 |
| t11-lower | 2000-3699 (init 2018) | lower up 56: 0 16, 1 17, 2 12, 3 11; without the cells 15 (2 chevrons 8, 3 2) |
| t34-upper | 2000-4299 (init 2007) | upper down 6: 0 1, 1 2, 2 1, 3 2; the cells never show |
| t34-lower | 2000-4169 (init 2007) | lower down 6: 0 1, 1 2, 2 1, 3 2 |

Pictures are compared with the core's per-frame video digest (XRGB8888, SHA-256), every frame of
the window but the race's initialization frame, which native's runner gives to the race's setup.
The two-pad states of these races have no serialized layout (native refuses a two-view state on a
track other than ZOOM ZOO, DRAGSTER or a stunt event), so only pictures are compared; the stunt race
is compared row by row as well.

## The loop's two arms

**`$81:83A6` is unreachable by construction** [L]. At a loop strip's step 0 the entry is refused
unless the pose override `$0F59` is 0 or inside `$0610-$061F` (`$81:8397-83A6`). The writers of
`$0F59` are the loop's own (`$81:83D0`, `$81:83F8`: `$0610`; `$81:841E`: `$0610` plus the step),
the corkscrew's (`$81:8911`: `$0600-$060F`), the turn's (`$82:A461`: `$0620` plus the reflection pose
table word, 0 to `$01F0`, plus a step of 1-15, so `$0621` and up), the spin's (`$82:9630`: `$09A0` and
up), the per-rider loads (`$82:8AA3`, `$82:8FAB`) and clears (`$81:85CD`, `$81:86CA`, `$81:8888`,
`$82:9556`, `$82:96E7`, `$82:A49B`). So a loop pose comes only from a loop step, and every loop
step stores the cooldown `$0359,Y` = 3 (`$81:849D`). The step returns to 0 only on step 16
(`$81:8414` to `$81:84A5`, which leaves the cooldown) or when the cooldown has run out (`$81:861C`).
The per-update reset before the tile dispatch (`$81:85B9-85CD`, R-0107's ordering) counts the cooldown
down and clears `$0F59` when it reaches 1. Step 16 comes at least one reset after the last refresh,
so the next update's reset takes the cooldown to 1 or below and clears the pose before the strip is
tested at step 0; and the cooldown cannot run out without passing 1. Native's `update_loop_tile` and
`update_loop_cooldown` read the same way and never saw the arm in 56,467 entries [N].

**`$81:83C4`/`$81:83EC` (a rider facing away from the descriptor) was not reached.** Every loop
strip is a vertical shaft: the rider falls through it in 16-update cycles, contact suspended, so the
selected descriptor stays the one of the cycle's first update.
- Tried: 16,187 schedules (11,396 on tracks 15, 25 and 35, 9,000 of them with 40-300 update Left
  holds) gave 10,881 player entries, 45,586 opponent entries and 3 refusals, none facing away; 2,000
  perturbations of track 25 seed 782's inputs in the 75 updates before its shaft (every move, held
  4-32 updates from every third update) gave only facing entries.
- Why it resists [N]: track 25's first shaft (x 5248-5280) is entered from a corkscrew whose last
  update sets the facing and the reflection lock; the lock counts down only while surface mode is clear,
  and it is set through the fall, so the rider cannot turn before the strip. Each shaft holds a
  column of opposite-facing cells (track 25 x 5264, y 1712-1824 and x 6800, y 2944-3056; track 15
  x 14544, y 3376-3488; track 35 x 80, y 10768-10880). The loop's top step turns the rider to match
  them, and no re-entry at step 0 selected one with the facing the other way.
- Whether a route reaches it (a shaft entered from below or from its far side, or a cycle boundary on
  one of those columns) is open.

## Results

Main `788efdaf`'s binaries (the candidate's are the same: no native change), pack v41. All seven
`checks.list` lines pass; `mutants.txt` shows each fails on a broken native.

| Check | Original | Native |
| --- | --- | --- |
| t11-upper | 1,699 pictures | 1,699/1,699 equal |
| t11-lower | 1,699 pictures | 1,699/1,699 |
| t34-upper | 2,299 pictures | 2,299/2,299 |
| t34-lower | 2,169 pictures | 2,169/2,169 |
| vs-hill-rider1, pictures | 2,949 pictures (1950-4899) | 2,949/2,949 |
| vs-hill-rider1, race rows (`stunt/race_compare2p.py`) | 2,792 updates (1982-4773) | 2,792/2,792 rows, trailer and rider 1's tallies |
| t34-seed155 (`check_race.sh`) | 604 rows, 602 pictures | 604/604, 602/602 |

**Coverage** (`coverage_captures.sh`): the five two-pad routes, each `status=passed`, merged into
`coverage/merged.json` and tracked as `docs/map/race-branch-remainder.map.json`, the static map's
seventh coverage input. t34-seed155 adds no byte: `$81:84B7` (a branch) was already observed, and
the coverage counts instructions, not branch outcomes. What the seventh input moves from inferred
to observed [L] (`listing-after/`, the listing with seven inputs):
- every branch this task names but the loop's two: `$81:C3EA-C3F0`, `$81:D907-D933`, `$81:D9E8-DA4C`,
  `$81:DAC4`, `$81:DACE-DAE1`, `$81:DB75-DB9C`, `$81:DBB5-DBCA`, `$81:DC8D`, `$81:DC92-DCCA` and
  `$81:DD4D-DDA4`;
- `$81:E7B7-E7D9`, the split chain's write of rider 1's hundreds cell;
- `$82:9866` (the lower arrow word `$0FD7` = 3, a down marker), and small branches the routes pass on
  the way: `$82:9524`, `$82:AB9A-AB9F`, `$83:E895`, `$83:E8B3`, `$83:E8C8` and `$83:EAF6`.

`$81:83A6`, `$81:83C4` and `$81:83EC` stay inferred. The static map regenerated with the seven inputs
(`regen.sh`; `docs/map/static/`): observed bytes 87,882 to 88,343, inferred 11,692 to 11,231, data 73,
unknown 31,425 (24.0%), unchanged.

## Not covered

- A loop descriptor facing away from the rider (`$81:83C4`, `$81:83EC`): not reached, as above.
- From R-0107, not part of this task: a stunt score of 1000 or more, track 13's read past its flag
  table, whole-SRAM and audio.
- The two-pad arrow races compare pictures only; their state has no serialized layout.

## Reproduction

From the worktree, with `local/` linked to the main checkout's (`E=local/evidence/race-branch-remainder`).
The search scripts (`$E/search/`, `$E/tile_hunt.py`) run the trace build; copy them to
`artifacts/rbr/` (they look for `artifacts/rbr/trace-tree/build/src/core/`) or adjust `TR`.

```
mkdir -p artifacts/rbr/trace-tree && cp -R src third_party tests CMakeLists.txt CMakePresets.json artifacts/rbr/trace-tree/
python3 $E/trace_patch.py artifacts/rbr/trace-tree   # then configure (-DBUILD_TESTING=OFF) and build front_end_runner and zoom_zoo_runner there
python3 $E/search/make_manifest.py t11 2p 9000 --tour-downs 1 --track-downs 1        # menu paths; t34: --tour-downs 3 --track-downs 4
python3 $E/search/mpc_build.py $E/manifests/menu-t11.json 0 2150 6510 OUT.json        # the leader routes (logs in search/logs)
python3 $E/search/mpc_build.py $E/manifests/menu-t11.json 0 3300 3800 OUT.json --other $E/routes/t11-leader.json \
  --prefix $E/routes/t11-leader.json --shift 400 --objective target --target 9040,1750  # routes/t11-trailer.json
python3 $E/search/dscan.py $E/manifests/menu-t34.json $E/routes/t34-leader.json 4400 400 800 1 [swap]
RBR_TR=<trace front_end_runner> python3 $E/search/mpc_build.py <vs-hill menu, pad 2 hill-win at 650> 1 3000 4750 OUT.json \
  --objective stunt --prefix $E/routes/hill-win-650.json --follow $E/routes/hill-win-650.json --horizon 200  # routes/hill-rider1.json
python3 $E/tile_hunt.py 7,15,21,25,31,34,35,39,40 0 1500 3000 OUT.jsonl random; python3 $E/tile_hunt.py 15,25,35 0 3000 3000 OUT.jsonl back
python3 $E/search/away_probe.py 25 782 random 1394 1840 1915
python3 $E/capture_hunt.py 34 155 random 3000 2010 $E/captures/t34-seed155
python3 $E/route_manifest.py <menu> <route1|-> <route2|-> FRAMES $E/manifests/NAME.json "<description>"
python3 $E/stunt/probe.py $E/stunt/vs-hill-rider1.json $E/stunt/probe-vs-hill-rider1 --wram-bytes 0x4000 --dump-from 1950
zsh $E/coverage_captures.sh
python3 tools/project.py coverage merge --coverage-list $E/coverage/coverage.list --scenario race-branch-remainder --out $E/coverage/merged.json
python3 tools/project.py coverage map --coverage $E/coverage/merged.json --out docs/map/race-branch-remainder.map.json --summary docs/map/race-branch-remainder.md
bash $E/regen.sh   # native-symbols and the static map with seven coverage inputs
zsh $E/mutants.sh OUTDIR; python3 $E/mutants_summary.py OUTDIR $E/mutants.out
```

`trace_patch.py` is kept beside the evidence; the trace build was never committed.
