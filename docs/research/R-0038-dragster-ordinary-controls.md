# R-0038 - DRAGSTER ordinary controls on the shared race engine

Status: implementation candidate on `task/dragster-ordinary-controls`, not yet
independently reviewed or integrated. Task record:
[DRAGSTER-ORDINARY-CONTROLS](../../tasks/DRAGSTER-ORDINARY-CONTROLS.md).

## Identity and tested domain

PAL ROM SHA-256 `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`;
audited bsnes core SHA-256
`e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b`; two-track
pack v7 `b75539a0...`. One-player MIKE/BRONSEN CRAWLER/DRAGSTER, one lap, fresh
scenario. Every original cold-starts the accepted menu path of
`tests/manifests/replay/race-crawler-dragster-3000.json` (Start presses only,
all before frame 1206) and then presents the case's controller-0 buttons from
frame 1329; frames the case does not name are neutral. Captures record WRAM,
cartridge RAM and video hashes from frame 1159.

## Architecture decision and measurements

Native DRAGSTER had its own early update, `update_movement`, recovered for
riding right from a semantic end-1533 seed. The later ZOOM ZOO engine
(`update_zoom_zoo`) recovered jump, brake, reversal, rolls, rotations,
announcements, pause, finish and result loading from native initialization.
Two observations suggested the engine is shared: R-0037 found the same race
palette NMI, and the 742-byte M4-16 projection is built from the same WRAM and
cartridge-RAM addresses as DRAGSTER's own 333-byte seed.

Both candidates were measured against one exploratory original before either
was extended (`artifacts/dragster-ordinary-controls/explore/mixed1`: B jump,
L rotation, Y brake with Right, a 90-frame Left reversal, B jump with an X roll,
player loss):

| Candidate | Domain compared | Result |
| --- | --- | --- |
| (a) `update_movement` from `classic_crawler_dragster_start` (end-1533) | 333-byte canonical seed projection | All bytes match for 166 updates (1534-1699); aborts at 1700, the first B while moving ("moving brake is outside the recovered movement domain"). It cannot start before 1533. |
| (b) `update_zoom_zoo` from native initialization with DRAGSTER's track, tile columns and tile flags | 742-byte M4-16 projection | All bytes match for 413 updates (1328-1740), through the countdown, the B jump, the L rotation and its landing. First divergence 1741, `player.progress_adjustment` 71 against 72: the ZOOM ZOO speed-limiter bound. |

Decision: (b). The shared routines already reproduce the original far beyond
where (a) stops, and the remaining differences were scenario values, not
missing mechanics. After the three scenario values below the same exploratory
original matched all 742 bytes for all 2,773 updates, 1328-4100. DRAGSTER keeps
`update_movement` and its `URMV` formats unchanged for the accepted historical
gates; the new path is additive (state identity `URDG0001`).

## Recovered facts

- **Initialization boundary.** Fade `$0FF1` is 0 and countdown `$11C5` 270 at
  the end of frame 1328, 48 frames before ZOOM ZOO's 1376 on its menu path
  (`$0FF1` becomes 1 at 1329). The accepted end-1533 seed is the same race 205
  updates later (countdown 69). 48 is a multiple of 2, 3 and 16, so every
  frame-phase clock aligns.
- **Playfield geometry, `$81:A304-A51B`.** Decoded track byte 13 selects a
  playfield arm: 0 takes `$81:A4C1-A4FD` (1,024 x 16 coarse cells, x mask
  `$0D4F = $FFFF`, `$0D51 = $3FF`), `$40` takes `$81:A445-A481` (256 x 64,
  `$3FFF`). The same arm sets the camera and visibility scale: world x is
  shifted left by `$03F1` (0 or 2) before comparison with the follow window
  `$03F3/$03F5` (`$81:9FB0-A05D`: -24/25 or -96/100) and the visible span
  `$0425/$0427` (`$82:AD0F-AD28`: -49/256 or -196/1,024). The other arms are
  unreached and rejected.
- **Race mode `$77:074B`.** DRAGSTER's scenario bytes `$77:0744/074A/074B/074C`
  are 0 (ZOOM ZOO: 3, 1, 1, 1). The ROM has 47 long reads of `$77:074B`; an
  exploratory DRAGSTER race reaches 15 of them (`access-mixed1`). Those in the
  race update compare with 1 or 2 so that modes 0 and 1 take the same arm
  (for example the countdown `$83:E5D8-E6DE`, the finish `$83:E808-EAC6` and the
  wrong-way warning `$82:9751/9781`). The mode-0 differences are set up before
  the race or appear at the result:
  - laps: `$82:DB96-DBB2` stores 1 + 1 in `$0EFB/$0EFD` (one lap plus the
    initial line crossing); `$81:D673` copies it to the lap base `$0D15`, which
    `$81:8139-81A3` uses to skip the initial crossing's slot, display and
    final-lap announcement;
  - the speed-limiter bound `$1281` (`$83:CC59-CC7C`): `$60` for mode 0, `$48`
    for mode 1, minus `$1283`, which is 0 on every authenticated frame of both
    tracks (guarded);
  - the final-lap announcement (`$81:81AE`), unreachable in a one-lap race;
  - result loading: mode 0 publishes no lap-graph extrema (`$83:904A-90F0`),
    and `$80:F88D-F8A5` publishes the totals at load update 108 instead of 107
    (observed at 3687 for load 3580).
- **Finish, result and stable screen.** The lap routine `$81:8050-82B6` is
  DRAGSTER's finish (R-0013's crossing writes are its lines `$81:80C1-823B`).
  The first new result picture is load update 109 on both tracks. The legacy
  stable counts stay: winner 226, loser 242. Equal times, both riders crossing
  on one update, count as won: the player is processed first, and the tie
  original's picture at load 225 matches the native winner picture as closely
  as the accepted winner does (961 of 57,344 pixels, time row exact).
- **Countdown releases A and X, `$83:E7A2-E7BF`.** Besides forcing both brakes
  and clearing both jumps it clears both riders' A (`$031D/$031F`) and X
  (`$0321/$0323`) every countdown update. The shared engine missed this;
  pressing X during the initial drop started a roll the original never starts
  (fuzz seed 1, original divergence at 1343). ZOOM ZOO references never press A
  or X there, so its gates are unaffected.
- **Landing clears held rotations.** A landing's reward pass (`$829B69-9D97`)
  clears held rotations on the update a released roll counts its hold, so the
  original reaches hold 1 with 0 rotations (fuzz seed 31, frame 1623). The
  M4-16 restore bound rejected that state; it now admits it only with zero
  rotations while supported or with a landing response.
- **Physical D-pad.** The reference core's gamepad reports `up & !down` and
  `left & !right` (bsnes `sfc/controller/gamepad`): a SNES rocker cannot press
  both. random-1 held Up with Down at 2207 and the original saw neither.
  DRAGSTER's runner and app drop opposing directions the same way
  (`with_physical_dpad`); ZOOM ZOO's accepted input path is unchanged.
- **Result font.** The small result font is contiguous from `.` at `$A8`:
  digits `$A9-$B2`, then letters from `A` at `$B3` without `O`. Ordinary finish
  times reach every digit; 1, 2, 4 and 9 were missing and aborted the result
  screen. Original result frames with 0:33.61, 0:52.96, 0:43.30 and 0:36.09
  match the native time row exactly (`explore/*-frames`).

## Frozen evidence

Each case was captured twice with identical per-frame WRAM, cartridge RAM and
video digests and frozen before native evaluation, except the two regressions,
which were frozen after the fixes they drove.

| Case | Frames | Rows SHA-256 | Finish (player/opponent) | Outcome | Role |
| --- | --- | --- | --- | --- | --- |
| primary | 1328-3900 | `dac612cc...` | 3211/3214 | won | untuned |
| reversal | 1328-5000 | `f142311e...` | 4292/3325 | lost | untuned |
| random-1 | 1328-4340 | `7df2f728...` | 3636/3214 | lost | tuned (D-pad) |
| random-2 | 1328-4400 | `de456f5d...` | 3700/3214 | lost | untuned |
| random-3 | 1328-4140 | `391f008e...` | 3438/3214 | lost | withheld until the final candidate |
| regression-countdown-actions-tie | 1328-4000 | `3a68c246...` | 3226/3226 | won (tie) | regression |
| regression-landing-held-roll | 1328-4400 | `8c3022d0...` | 3213/3214 | won | regression |

primary: start boost from a countdown brake, held and tapped B jumps, short and
long X rolls, L, R and A+R in the air, Up, Down, Select and a Y brake while
riding. reversal: B and Left during the countdown, a 450-frame Left ride that
reaches the wrong-way warning, B, R and Y while reversing, two Start pauses.
random-1..3: seeded Right-biased ordinary input without Start (generator in the
task record). The regressions are the two fuzz races above.

Originals, captures with frame images and the access records
(`access-load`, `access-mixed1`, `access-fuzz-seed1`) stay private under
`artifacts/dragster-ordinary-controls/` in the task worktree.

## Reproduction

```sh
python3 -m tools.unirally_lab.native.dragster_playable_reference --core local/emulators/bsnes/bsnes/out/bsnes_libretro.dylib --case tests/manifests/native/dragster-ordinary-primary.case.json --horizon 3900 --out artifacts/FRESH-primary-a
python3 -m tools.unirally_lab.native.dragster_playable freeze --reference artifacts/FRESH-primary-a --repeat artifacts/FRESH-primary-b --out artifacts/FRESH-primary.freeze.json
python3 -m tools.unirally_lab.native.dragster_playable compare --reference artifacts/FRESH-primary-a --repeat artifacts/FRESH-primary-b --contract tests/manifests/native/dragster-ordinary-primary.freeze.json --binary build/app-debug/src/core/zoom_zoo_runner --pack local/classic-crawler-two-tracks-v7.pack --out artifacts/FRESH-primary-compare.json
build/lab-release/src/app/dragster_fuzz_runner --content-pack local/classic-crawler-two-tracks-v7.pack --first-seed 1 --seeds 3000 --races 3 --max-updates 40000 --failure-cases artifacts/FRESH-fuzz-failures
```

The compare gate checks the frozen inventory, runs native from initialization
twice, restarts from the final result state, and restores in fresh processes at
every controller change, state transition, announcement, roll, pause and result
boundary. DRAGSTER guards are the ZOOM ZOO race guards from end-1601 with the
nine recovered overrides in `tests/manifests/native/dragster-race-guards.reference.json`.

## Presentation and product limits

- DRAGSTER keeps its accepted M3 renderer through `dragster_presentation_state`.
  Rider poses outside its five recovered pairs hold the last recovered art, as
  before; jumps, rolls and reversal therefore show held art.
- The fade-in scales the converted picture with the output curve, an
  approximation of the original's CGRAM brightness.
- The pause menu is the authored ZOOM ZOO overlay; RESTART RACE restarts
  natively where the original retires to the tour.
- A rider reversing behind x 894 holds the view at the start area; the accepted
  BG1 map reads before the track otherwise.
- The result background is the accepted static picture; the original keeps
  animating it. The original also leaves the result screen for the track menu
  on other buttons (random-1 at 4118); native waits for Start (Race Again).
- The 25-entry DRAGSTER pack lacks the shared tables. `frontend run` uses a
  valid two-track pack beside it or extracts one with `--rom`; the app refuses
  the DRAGSTER-only pack before gameplay.

Not established: the 10:00 time limit on DRAGSTER (no case reaches it), opponent
reward events beyond the reached ones, arbitrary restore states, and live play by
the user.
