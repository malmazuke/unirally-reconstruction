# R-0104 - Power bounce for both riders in a two-view race

Status: research result and native change ([TWO-VIEW-POWER-BOUNCE](../../tasks/TWO-VIEW-POWER-BOUNCE.md)),
28 October 2026, on main `5f161c9c`. PAL ROM SHA-256
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, reference core bsnes
`7d5aa1e6` (Strict). Evidence in main `local/evidence/two-view-power-bounce/`. It closes R-0103's
race-state gap: in a two-view race under HUNTER's power bounce, rider 1's velocity diverged.

Tags: **[L]** read from the static map's listing (`artifacts/static-map/bank-81.lst` in the main
checkout); **[C]** confirmed in a capture.

## The condition

At `$81:9495` every landing path of the vertical contact routine has its matrix pointer in `$A3`
(`$FFFF` for none). Before `$81:94C3` reads it [L]:

```
$81:94A8  LDA $77:0750 / AND #$00FF / BIT #$0008 / BNE $94B9   ; two views: skip the rider test
$81:94B4  LDY $0FF9 / BNE $94C3                                 ; rider 1: keep $A3
$81:94B9  LDA $132B / BEQ $94C3                                 ; power bounce off: keep $A3
$81:94BE  LDA #$0572 / STA $A3                                  ; matrix 0
```

- `$0FF9` is the rider being resolved: `$81:8DB0-8DB3` stores 0 before rider 0's contact
  (`$81:8CFC`), `$81:8F04-8F07` stores 2 before rider 1's (`$81:8E43`) [L].
- `$132B` is HUNTER effect 2's word (1 on the update it is picked, 2 while it runs; R-0052), read
  16-bit [L].
- So a landing takes matrix 0 when power bounce is on and either the race has two views
  (`$77:0750` bit 3) or the rider is the player. In a one-view race rider 1 never does. Two views
  covers every race with bit 3: 2P, VS, the split idle demo (`split_demo_options`, R-0069) and a
  two-view league race. Native keeps bit 3 as `split_screen`, which it already passes to contact
  as the options word `0xC20A`.

Native gave the matrix to rider 0 only (`race_update.cpp`, `index == 0`). It now passes the effect
for both riders, and `lands_with_power_bounce` (`vertical_contact.cpp`) decides as the listing
does. A test (`zoom_zoo_trial_tests.cpp`) checks the condition's four cases and lands a rider
through `resolve_vertical_contact` in each; with native's old condition (the player only) it fails.

## Evidence

- **Native search** (`landings.py`, SPLIT-HUNTER-PICTURES' `search.py` schedules 1-200 from the
  faedine-split0 image; VS seeds replay the same races as 2P). Power bounce runs in 70 of 200
  races; rider 1 lands under it (unsupported count 9 to 0) in 67, 631 landings. The candidate's
  race differs from main's in 64, each first at one of those landings; none differs elsewhere. In
  3 races (seeds 25, 62, 173) rider 1's landings under power bounce leave the state as main's.
- **The original** [C], race rows (`race_check.py`, bytes 12-564 and the HUNTER effects per
  update), candidate against main:

| Capture | Power bounce (frames) | Rider 1's landings under it | Candidate | Main |
| --- | --- | ---: | --- | --- |
| `twop-104` (2P) | 5423-5921 | 11 | 5,973 of 5,973 | 3,521, then from 5548 (update 3521) |
| `vs-124` (VS) | 4888-5386 | 8 | 5,973 of 5,973 | 2,954, then from 4981 (update 2954) |
| `twop-67` (2P, new) | 5054-5552, 6725-7223, 7578-7999 | 23 | 5,973 of 5,973 | 3,030, then from 5057 (update 3030) |

  The original's power bounce frames in each capture (`power_frames.py`) are those native's
  search predicted. At each divergence main differs in rider 1's velocity x and y.
- **One view keeps the `$0FF9` test** [C]. A native search of SPLIT-HUNTER-PICTURES' one-player
  schedules 1-150 (cold start typing "faedine", 9,000 frames) against a mutant without the rider
  test (`mutant-either-rider/`): power bounce runs in 26 races, rider 1 lands under it in all 26
  (76 landings), and the mutant differs in all 26, each first at such a landing; candidate and main
  agree. The original's `one-88` (seed 88, power bounce 5543-6041, rider 1 lands at 5636, 5841,
  5951 and 5998): candidate and main equal on all 4,023 rows, the mutant on 1,659, from 5636
  (rider 1's velocity).
- **Unchanged** (race rows, candidate and main equal on every row): one-view HUNTER races
  CREDITS-NAME's `faedine` (1,671), `faedine-zoom` (3,023), `faedine-count` (four races, 1,274) and
  `warm-hunter-faedine` (1,571), SPLIT-HUNTER-PICTURES' `one-67` (3,699, from its first dumped
  frame), and the split demo `demo-pic` (2,201) and `vs-4` (5,973). None of these has power bounce
  during its race (`warm-hunter-faedine`'s `$132B` is set at 741-1406, before its race at 1628).

## Pictures

SPLIT-HUNTER-PICTURES' `compare.py`, every captured picture 1900-7999 (6,100 per capture),
candidate against main:

| Capture | Candidate | Main |
| --- | ---: | ---: |
| `twop-104` | 6,100 | 3,650 (unequal from 5550) |
| `vs-124` | 6,099 | 3,083 (unequal from 4983) |
| `twop-67` | 6,089 | 3,148 (unequal from 5059) |

The candidate's 12 unequal pictures (`vs-124` 7044; `twop-67` 2727-4753, before power bounce and
the same on main) differ on line 113 only, R-0103's six-line mosaic limit.

## Not covered

- A two-view league race under power bounce (LEAGUE-TOURS' `tours-a` windows) is not replayed here;
  it takes the same `split_screen` path.
- The split idle demo under power bounce: `demo-pic` has no power bounce, and the demo's effects
  come from its AI's riding, not searched here.
- Why rider 1's landings under power bounce leave the state unchanged in 3 searched two-view races
  is not traced (the matrix may give the same velocities there).
- Line 113 under a six-line mosaic block (R-0103's limit) is unchanged.

## Reproduction

```sh
zsh local/evidence/two-view-power-bounce/captures.sh twop-67 one-88
python3 local/evidence/two-view-power-bounce/landings.py 2p 1 200 --main local/evidence/two-view-power-bounce/base-5f161c9c/front_end_runner
python3 local/evidence/two-view-power-bounce/landings.py 1p 1 150 --frames 9000 --main local/evidence/two-view-power-bounce/mutant-either-rider/front_end_runner
local/locks/heavy-run.sh TWO-VIEW-POWER-BOUNCE zsh local/evidence/two-view-power-bounce/run_checks.sh OUT
```
