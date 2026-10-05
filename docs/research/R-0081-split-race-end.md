# R-0081 - When a two-human race ends: both finishes, and VS's forced finish

Status: implemented on `task/split-race-end` ([SPLIT-RACE-END](../../tasks/SPLIT-RACE-END.md)),
5 October 2026. PAL ROM SHA-256
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, audited bsnes core
`7d5aa1e656b9171524d01b1b22917197d8121cb4`. The listing reading is main
`local/evidence/split-race-end/listing-report.md`. The captures are in main
`local/evidence/split-race-end/`:

- `vs-idle`: VS DRAGSTER, pad 2 idle, 5,200 frames, pictures 3900-5199.
- `zzap-long`: the SPLIT-HUD-GAPS review's 2P ZOOM ZOO run extended to 8,800 frames, pictures
  8000-8799.
- `league-idle`: a league pair on DRAGSTER, pad 2 idle, 9,500 frames.

All three hold every frame's work RAM `$0000-$1FFF`.

Tags: **[L]** listing only, **[C]** confirmed in a capture.

## What the original does

- **The finish display waits for both riders in a split race.** `$83:E7C3-E7E4` counts `$0F0F`
  (native `finish_delay`) only once `$0EFF` is set and, when `$0DE1` (two views) is set, `$0F01`
  too. A one-view race counts from the player's finish. The result loads at 240.
  [L; C: `zzap-long` counts from 8068, the update after rider 1's finish at 8067, not from rider
  0's at 7615]
- **Each finished rider runs a banner driver.**
  - The drivers are `$83:EA11` (rider 0: `$0F03` index, `$0F07` life) and `$83:EBA3` (rider 1:
    `$0F05`, `$0F09`). They run on every update of the rider's finish block, from the update after
    its finish. The block itself skips its brake and pose work on phase-1 updates.
  - A driver waits while the other rider's life is not zero.
  - While its index is 0, the life is set to 360. A live driver counts its life down once per
    update. On odd updates (`$0300`), it steps the index 8..24 and then 7..24.
  - It writes the channel-6 window request `$11FD`. This is the banner that native's
    `ClassicWindowPointer` already follows (R-0040).
  - [L; C: `vs-idle` shows 359 on 3987, the first odd update after the finish on 3985, and 0 on
    4346]
- **VS forces the other rider finished.**
  - When a driver finds its life spent, a race with `$77:0750` bit 2 set (VS) sets the other
    rider's finished flag to 0xFFFF. `$83:EA5D-EA69` does this for rider 0's driver,
    `$83:EBCB-EBD7` for rider 1's.
  - Nothing else is written: no lap time, no total, no digits. The forced rider keeps 60000 (NO
    TIME) and takes the loser's pose and caption.
  - Rider 0's block runs before rider 1's in an update. So when rider 0 forces rider 1, rider 1's
    block runs in that same update: its view dims, it brakes, and its driver starts. When rider 1
    forces rider 0, rider 0's block waits for the next update.
  - Every reader in banks 80-83 tests the flag only for zero.
  - [L; C: `vs-idle` 4347, rider 1 forced, `$0F09` = 359 and `$0E11` = 1 the same update; `$0F0F`
    counts 1 on 4348 and 240 on 4587; the result loads on 4588, totals 3601/60000]
- **2P waits.** With bit 2 clear, a spent driver writes nothing, and the race goes on until the
  second rider finishes, the 10:00 clock finishes both, or the pause menu quits. [L; C: `zzap-long`,
  rider 0's driver spent on 7976, rider 1's finish on 8067]
- **League pairs wait too.** A league pair's `$77:0750` takes bit 2 only from `$77:0742`
  (`$80:9A05-9A0B`) [L], and a league pair does not set it. [C: `league-idle`: rider 0 finishes on
  8960, its driver is spent on 9320, and rider 1 is still unfinished at 9499]
- **The clocks.** The clock routine asks for each rider's clock to be rewritten only while that
  rider is unfinished:
  - rider 1's through `$81:C6D1-C6DC` (`$0F01`, `$034F`);
  - the player's through `$81:C6F0-C6F8` (`$0EFF`, `$034D`).

  A forced finish therefore leaves the forced rider's last clock written, the one before the
  force, frozen there. [L; C: `vs-idle` shows rider 1's clock at 0:43:2 from 4347; the review's
  `vs-opp`, rider 1 first, shows the player's at 0:43:2 from 4348]

## Native

- `ZoomZooState::versus`: the front end sets it for a VS race (`front_end_runner`, the app). A
  restart keeps it.
- `ZoomZooRaceState::banners`: the two drivers. Race state follows them only in a VS race, the one
  case where they change race state.
- `update_finish` changes:
  - It counts the display once both riders have finished in a split race.
  - It runs `drive_banner` after each finished rider's `finish_rider` (outside its phase-1 return)
    in a VS race.
  - `forced_finish` is 0xFFFF.
- Serialization: a VS split state appends the two drivers (index, life; 8 bytes) after the split
  trailer and before the lower-view byte (R-0079).
  - Their presence is the VS flag, by size: 792, 844, or one more with the lower view, sizes no
    other layout takes. Other states keep their bytes.
  - Reading accepts 0xFFFF and laps left only in a VS state.
  - It refuses the following: an index outside 0 and 7..24; a life over 359; a driver for an
    unfinished rider; two live drivers; a forced finish whose other driver has not ended.
  - Writing refuses drivers outside VS, and VS outside a two-pad race.
- Picture:
  - `dim_finished_views` dims a forced rider 1's view from the forcing update's picture.
  - `ClassicRaceHudClock` keeps the lower clock text as the forcing update's predecessor left it
    (`ClassicHudPublished::lower_clock`), and `draw_split_hud` draws that. Its queue stops
    rewriting the player's clock once the player is forced (review round 1).
  - `ClassicWindowPointer` starts a forced rider 1's driver in the forcing update. The picture's
    window follows its own copy of the drivers in every race, as before.

## Evidence

`split_compare.py` (main `local/evidence/split-pause-menu/`, now comparing `$0F0F` too) and
`boxes.py` (main `local/evidence/split-race-end/`, differing pictures by place):

| Capture | Race frames | Word differences | Race end (native = original) | Pictures equal | Differing pictures, by class |
| --- | ---: | ---: | --- | ---: | --- |
| `vs-idle` | 2,610 | 0 | 4588 (S+601) | 296 of 1,300 | 496 in rider 1's caption rows (its tutorial captions and their ink over the rider); 4 split upload timing (3931, 3986-3988); 504 the results' icons |
| `zzap-long` | 6,282 | 0 | 8308 | 403 of 800 | 11 caption rows; 2 split upload timing at rider 1's finish (8068-8069); 384 the results' icons |
| `league-idle` | 2,471 | 0 | none by 9499, as the original | 0 of 3 | 3 caption rows |
| review `vs-opp` (VS, pad 2 first, pad 1 idle) | 2,610 | 0 | 4588 | 231 of 1,300 | 562 caption rows; 3 split upload timing (3931, 3986-3987); 504 the results' icons |

- On main, `zzap-long` ended at 7856 and `vs-idle` 241 frames after 3985.
- Restore: a VS state saved at 3990, 4347, 4348, 4400 or 4587 resumes byte-identical to the end
  (`--restore-check`), 792 bytes, layout H. So does the league pair's at 9000 and 9400.
- R-0079's eight captures and the earlier reviews' five are still 0 word differences:
  `twop`, `twop2`, `twop-plain`, `vs`, `vs2`, `vs-now`, `league`, `league-restart`, `drfin`,
  `leaguecpu`, `leaguefin`, `zz2p`, `zzap`.
- Tests: `dragster_race_tests` covers the following:
  - 2P waits 600 updates;
  - VS forces at 361 or 362 updates;
  - the forced rider's driver starts the same update, and the display counts the next;
  - the opponent-first order;
  - the 792-byte state's round trip and its refusals.
- `presentation_tests` covers the frozen lower clock.

## Not covered

- **Rider 1 forcing rider 0** was captured by the tier-1 review. Its `vs-opp` capture is in the
  reviewer's scratchpad; it is withheld, not part of this record's evidence. On `ba07f88` its
  race words matched, but 238 pictures (4350-4587) showed the player's clock running on. The
  player's clock gate (`$81:C6F0`) fixed that, and the numbers above are after the fix.
- **A second rider finishing on its own while the first driver lives**, then being forced over
  its real finish (`$83:EA5D` writes 0xFFFF over 1). This is inferred and not captured; native
  does the same.
- **A restored state without picture history** (`classic_window_table_index`,
  `classic_opponent_finish_frame`) still derives the banner from finish frames. A forced finish
  has no finish time, so after a forced finish such a state shows the banner the finish frames
  give. It also draws the running lower clock, since the frozen text is presentation history.
- `ClassicWindowPointer` keeps its own copy of the drivers. Making it read race state needs the
  drivers in every race's state, which changes the one-player layouts.
- The remaining picture classes are SPLIT-CAPTIONS' (rider 1's tutorial captions, HUD ink over the
  riders, split upload timing) and the results' icons (R-0071).
