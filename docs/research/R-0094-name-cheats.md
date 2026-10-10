# R-0094 - The rider-name cheats: "credits" and "faedine"

Status: research result and native change ([CREDITS-NAME](../../tasks/CREDITS-NAME.md)), 10 October
2026, on main `6b6790a`. PAL ROM SHA-256
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, reference core bsnes
`7d5aa1e6`. Evidence in main `local/evidence/credits-name/`.

Tags: **[L]** read from the ROM's bytes, **[C]** confirmed in a capture or replay.

## Domain

A gated cold-start capture `credits` (OPTIONS' keyboard renames rider 0, then a 1P DRAGSTER race:
frame images, work RAM per frame, a sound schedule) and in-process replays of the original
(`probe.py`: work RAM `$0000-3FFF` and cartridge RAM per frame) for `credits-2p`, `faedine`,
`faedine-zoom`, `faedine-count` and, from saved images with the name already set
(`make_image.py`), `warm-demo-credits`, `warm-2p-credits` and `warm-hunter-faedine` (track 40).

## Findings

- **The test.** Every race's setup (`$83:C8E0`, all modes and the idle demo, entered at
  `$83:C8E4`/`$83:C9E4`) calls `$83:FB8A` at `$83:C99E`: `$77:111A` = its value less one, kept within 0-3
  (`$83:FB8C-FB9B`, `$83:FB95`); `$0545` = 0; rider 0's name (`$77:000C`) is compared over 7 bytes
  with "credits" (`$83:FB56`, `$83:FBAE`): a match sets `$0545` = 1 (`$83:FBBA`) and the name
  becomes "mike____" (`$83:FB66`). Otherwise "faedine" (`$83:FB70`, `$83:FBD5`) sets `$77:111A` = 3
  (`$83:FBDC`) and the same rename (`$83:FB80`, `$83:FBE2`). The demo clears `$77:111A-111B` first
  (`$83:C914`) and still runs the test. The names' checksum `$77:016C` is left stale; native
  writes a fresh one. [L, C]
- **"credits".** The race's count is skipped (`$83:CA08`) and the player's total set to `$EA62`;
  `$82:DDD5-DDDE` calls `$83:FADC`/`$83:FAE0`: forced blank, BG mode 1 with BG1 alone, assets
  `$C1` (colours), `$C0` (map) and `$BF` (tiles) loaded through the copier (`$83:FB03`,
  `$83:FB1D`), shown through the race's NMI hook (`$80:85A4`, `$80:FAF8`): brightness 1 at C+16
  (C the first blank frame after the menu's fade), 2-14 to C+23, then the hook's count to 15,
  held 501 frames (`$83:FB31-FB39`), faded out C+525-531; `$82:DF05` then ends the race as
  R-0060's restart. `$82:D6E7` and `$82:D7C2` run before the test on every setup; the race's load
  after it (`$82:D4C4`) is skipped.
  After a demo's credits picture the main menu returns a frame later than after a timed exit. [C]
- **"faedine".** `$82:D978` reads `$77:111A` as a word: nonzero sets `$131F` = 1 on any track
  (zero: tracks 40 and up only), so for three races every track runs HUNTER's tag effects (R-0052)
  and its opponent tier (R-0050: level 3, `$40`, `$60`). `faedine-count`: `$77:111A` 3, 2, 1, 0
  and `$131F` 1, 1, 1, 0 over four setups. [L, C]
- **Two views.** In a race with two views (`$77:0750` bit 3: the split demo, 2P, VS, a two-view
  league) an announcement pushed to the front of the player's queue goes to the front of the
  opponent's too (`$81:C579-C594`, entries `$0CEB`, cursors `$0D11/$0D13`), and the screen flip is
  named "invisible unis" (`$24`, `$83:CEE8-CF0F`), R-0052's open variants. No split race ran
  HUNTER's effects before this cheat; the review's `demo-faedine-0` (a split ZOOM ZOO demo with
  tags) found both. [L, C]
- **Native** (`src/core/credits_name.cpp`): the test at each race start (NOW PLAYING's fade, the
  demo's title), the picture screen, the restart, `OnePlayerRecords::hunter_races` (`$77:111A`,
  saved in the cartridge image) and `ZoomZooState::hunter_tour` (`$131F`) in place of the track
  lookup; a forced race's save state wraps the format in `URHF0001`. Pack profile v37 adds the
  names (`front-end.name-cheats`) and the picture's assets. [C]
- **Results.** `credits`: 800 of 800 pictures from frame 3400 (860 of 860 in the replay), the
  menus' words from 3400 and its 3,676 sound cue lines equal; the app's cue log and save equal
  the runner's. Race rows: `faedine` 1,671 of 1,671, `faedine-zoom` 3,023 of 3,023 (a tag
  included; 3,299 of 3,300 pictures, the one a race's first frame the runner draws none of),
  `faedine-count` 1,274 of 1,274, `warm-hunter-faedine` 1,571 of 1,571. The 2P and demo credits
  pictures and restarts equal. Without the names, eleven schedules' rows, cues and errors equal
  main's binary. [C]

## Not covered

- Native renames the rider when the race's setup begins, the original seven frames later; nothing
  reads the name between.
- Two-view races in HUNTER mode: the review's split demo is equal through its race but for its
  last update (the demo's exit: the camera and `$2054`); no 2P, VS or league race with a tag is
  captured. Only `credits` has a sound schedule. `front_end_runner --restore-check` refuses a
  two-pad race forced into HUNTER mode (its layouts H and F have no wrapper); the one-player and
  demo wrappers restore exactly.
- The demo's one-frame-later return after a credits picture is measured on one warm boot.
- Differences these captures show that main also has: OPTIONS' hidden OAM bytes and rename return
  text, the idle demo's start after OPTIONS or a warm boot, the demo's rider word `$017D`, 2P menus
  on R-0071's schedule from frame 900, a second pause-restart's fade.
