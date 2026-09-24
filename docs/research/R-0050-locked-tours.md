# R-0050 - The locked tours: how to reach them, their 20 race tracks, the HUNTER tier

Status: recovered and implemented on `task/locked-tours` (LOCKED-TOURS), 24 September 2026.
PAL ROM `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, audited bsnes
core `e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b`. Answers R-0046's
"Tours" hypothesis and next experiment 5.

## Reaching the locked tours (laboratory, original side only)

- **Cartridge RAM decides.** A probe powers on with patched cartridge RAM and pictures PICK
  TOUR (`local/evidence/locked-tours/probe.py`). A nonzero `$77:1000` lists all nine tours,
  but choosing a locked one also needs bytes in both `$77:10C0-$10DF` and `$10E0-$10FF`:
  - `$1000` plus `$10C0-$10FF` all `$FF` opens every tour;
  - no single pair of bytes in those ranges does.
  - Filling the whole `$1000-$1FFF` also works, but reaches the graph extrema (`$106F`) and the
    tutorial hints, so the preload is kept narrow.
  - How the game itself sets these bytes (progression through the open tours) is not
    recovered.
- **The preload has to be a formatted RAM.** Fresh cartridge RAM is all `$FF`, and the menu
  formats it at frames 403-405, after the first Start, which would clear any unlock. The core
  loads cartridge RAM from `<save dir>/<rom>.srm` at power-on; a write into memory after load is
  lost to the reset the Strict serialization method performs. So `track_reference capture
  --unlock-tours`:
  1. boots once through the cold menu to frame 600, after the format;
  2. takes that formatted RAM and sets the unlock bytes;
  3. writes it as the save file of a fresh power-on.

  The reference records the preload's digest. This is a capture method only: the product
  never executes original code.
- **The menu.** Down walks PICK TOUR's left column (CRAWLER, SHUFFLER, WALKER, HOPPER, then
  HUNTER), and Right takes the right column (JUMPER, BOUNDER, RUNNER, SPRINTER). So
  `menu_events(position, tour_row, tour_column)`.

## The 25 tracks

All 25 captures load track `5 x tour + position`, which confirms R-0046's rule for every tour.
Position 2 is a stunt event in every tour: tracks 7, 17, 27, 37 (LITTLE DIPPER) and 42. The 20
race tracks have laps 1 (one run), 2, 3 or 5, and boundaries between 1,374 and 1,464 on this
menu path. Pack profile **v12** (`classic.pal.crawler.tracks.v12`, 237 rules entries) adds their
data, tile columns, tile flags and BG1 tiles, and sceneries 1, 8 and 12
(`tracks.v12_new_entries`). The scenario table carries each one.

## The HUNTER tier and the opponent's turnaround

- **The HUNTER tier.** On every track but HUNTER's, the AI level `$1275` is 1 and `$1283` is 0.
  On 40, 41, 43 and 44 they are 3 and 64, and the progress adjustment bound `$1281` is 96 even
  on the lap races (72 elsewhere).
  - `$83:CC0B-CC29` sets `$1283` = `$40`, `$1281` = `$60` and `$1275` = 3 directly when `$131F`
    is nonzero, skipping `$83:CC59`. `$131F` is 1 on all five HUNTER captures and 0 on the other
    40 (the review found this).
  - `$82:A77E-A797` raises the opponent's speed cap by `$1283` x 2 while the player leads.
    Native had no term for it; the review's withheld HUNTER 44 capture with Right held diverged
    at update 259, exactly where the player first leads.
  - At `$83:E16B` a level above 2 always launches and suppresses for 60 (`$1277`). Below 2
    (`$83:E1A8`) it keeps native's conditions and 30.
  - At `$83:E135`, with fewer than four unsupported updates, a level other than 1 takes
    `$83:E222` at once, keeping the marker's jump. The rotation check at `$83:E228-E250`
    follows, as on the `$83:E21C` path.
  - Level 2 is on no observed track and stays guarded.
- **The turnaround.** Native lacked `$83:E0C5-E111`, a branch on every tour.
  - In surface mode, on a slope of 26 or more against the marker's direction, with a previous
    x displacement below 3, the AI starts a 30-update turnaround (`$0C73`). While it runs, the
    opponent rides against the marker without jumping.
  - It explains HIGHROAD's and JUMPOVER's first divergences.
  - `$0C73` is new state. It joins the special-tile extension: other tracks' states are now
    `URTRnn04` (838 bytes), and DRAGSTER's and ZOOM ZOO's are `URDG0003`/`URZZ000D` (778) while
    a word is live. `track_reference` projects it too.

## Evidence

`track_reference recompare --per-track` on the 25 captures (`local/evidence/locked-tours/sweep`,
horizon frame 2,900, controller released), the stunt events not compared:

| Result | Tracks |
| --- | --- |
| exact over the whole window (15) | 5, 6, 8, 9, 16, 18, 28, 29, 35, 36, 38, 39, 40, 43, 44 |
| exact to update 1,252, then the player's announcement queue differs | 41 TWO LOOPS |
| stop at an unrecovered tile flag pair | 15 LAST ONE (26, update 808), 25 DOWN+UP (26, 519), 19 JUMPOVER (12 in movement, 773), 26 HIGHROAD (8 or 26 in contact, 898) |

The steps: 12 exact with content and scenarios alone, 13 with the HUNTER level, and 15 with the
level-3 jump. The turnaround moved JUMPOVER from 551 and HIGHROAD from 741 to their tile
guards. The cold-start recompare is unchanged.

The review's withheld captures, each through the unlocked menu with a held input the primary
did not use: HUNTER 40 (Right from 1,500, released at 2,600) exact over 2,537 updates, JUMPER 9
(Right) 2,606, and SPRINTER 36 2,613. HUNTER 44 with Right held diverged at 259, the catch-up
term above; after the fix it is exact to update 1,620, where the player's announcement queue
differs (event 62 against 30), as on TWO LOOPS.

## Limits

- The unlock bytes' meaning, and the progression that sets them in play, are not recovered.
- On HUNTER tracks the player's announcement queue can differ: TWO LOOPS at 1,252 (event 57
  against 31) and HUNTER 44 with Right held at 1,620 (62 against 30). Open.
- Tile flag pairs 8, 12 and 26 are unrecovered, so four locked tracks still stop.
- The five stunt events are not compared (no native stunt event).
- The locked tracks' boundaries label this menu path and the unlocked RAM, not a natural
  progression.

## Reproduction

```sh
local/evidence/locked-tours/captures.sh
python3 -m tools.unirally_lab.native.track_reference recompare --per-track --sweep local/evidence/locked-tours/sweep \
    --binary build/lab-debug/src/core/zoom_zoo_runner --pack local/classic-pal-crawler-tracks-v12.pack --out <json>
```
