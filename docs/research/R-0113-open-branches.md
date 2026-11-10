# R-0113 - The reachable branches no capture of the original had run

Status: research result ([OPEN-BRANCHES](../../tasks/OPEN-BRANCHES.md)), 10 November 2026, on main
`19ecb59f`. PAL ROM SHA-256 `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`,
reference core bsnes `7d5aa1e6` (patched, `tools/locks/emulators.json`, Strict). Evidence in main
`local/evidence/open-branches/`. It answers R-0109's "code, reachability open" and "never-captured
branches" and the named arms R-0112 left to this task. One native change: DEFINE PLAYER's erase now
takes the rider out of their leagues (`$83:9B60-9B82`, `$83:9C47-9D7F`).

Tags: **[L]** read from the listing (`coverage-census/listing/`, the static map's eight inputs);
**[S]** a scan of the ROM's own data; **[C]** a run of the original made for this task, with a PC trace
(`orig.py`: the core's instruction ring drained every frame, every execution of the watched PCs with
A, X and Y); **[R]** stated by an accepted record.

## Domain

- **Routes** are real pad inputs: cold boots, a warm boot from LOCKED-TOURS' unlocked image
  (`track_reference.unlocked_sram`: the cold menu's formatted RAM at frame 600 with `$77:1000` and
  `$10C0-10FF` set to `$FF`, written before frame 0; presses from 404 on 3 frames earlier), and one
  corpus run reused (COVERAGE-CENSUS run 280). Each route was first built on native's
  `front_end_runner` where that helps, then run on the original with a PC trace showing the branch.
- **Compared** (each check names the counts of pictures, images and sound commands it compares, and
  fails on fewer; `compare.py`, `rows/race_compare2p.py`, `sounds.py`): every picture of a window
  against native's (video digests), the cartridge RAM after chosen frames outside R-0111's residual
  bytes (`save-image-exact/residual.txt`, the bytes no later boot reads before they are rewritten),
  for two-pad races every race row, its trailer and a stunt event's tallies (RACE-BRANCH-REMAINDER's
  `race_compare2p.py` with two changes, below), and for one-player races every sound command the
  original queues (`JSL $82:8000`, A = command << 8 | parameter) against native's `--sound-cues`
  enqueues, frame for frame, leaving out the session commands 1, 7 and 8 that native's sessions issue.
- Native: main `19ecb59f` plus this task's change, pack v41, `build/lab-release`.

## Findings

| Item | Verdict | Route or argument | Original against native |
| --- | --- | --- | --- |
| HUD lap fields of 10 and more (`$81:D255`, `$81:D6C0`, `$81:DEBB`, `$81:DFEC`, `$81:EC9A`; arms `$81:D218`, `$81:D692`, `$81:DE90`, `$81:DFC1`, `$81:EC6F`) | **Unreachable** [L, S] | laps are at most 8 (below) | - |
| `$77:0749 = $FF` (`$80:9075-9090`, R-0058) | **Unreachable** [L] | `$FF` reaches `$017F` only until NOW PLAYING replaces it (below) | - |
| `$83:9B49`'s arm (`$83:9B70-9B78`) and `$83:9C47-9D7F` | **Captured; native changed** | `erase-colin`, `league-erase`: DEFINE PLAYER erases a league member; `multi-league` (review round 1's route): the reaches past a slot | images equal after the change (6, 14 and 55 bytes differ before it); pictures 1,399/1,400 and one at 608 pixels, and 500/500 (below) [C] |
| HUNTER's effects in two views: `$83:CEF3`, `$83:D343` | **Captured** | `twop-hunter-next`: 2P, track 44, a tag at 2626 | 4,609/4,609 pictures; images equal [C] |
| HUNTER's effects in two views: `$83:D1D4` | **Unreachable** [L, C] | NEON's lighting flag is never set in two views and does not outlive its race (below) | - |
| 2P and VS on the locked tours' tracks | **Captured** | `twop-hunter-next` (2P, track 44), `vs-neon` (VS, track 42, two views: `$82:D9AC`, inferred before) | `vs-neon`: 2,780/2,780 race rows, trailers and tallies; 1,319/1,319 pictures outside the race; the race's pictures carry R-0103's two-view NEON residual [C] |
| A race tie | **Captured** | `vs-dragster-tie`: VS DRAGSTER, pad 2's Right one frame later, totals 3426/3426 | race 2,160/2,160 rows, the rematch 172/172; 4,497/4,498 pictures, the other a black hand-over frame the runner does not emit; images equal [C] |
| The VS count's tie arm `$83:9296-92AF` | **Unreachable** [L, C] | `$83:99C2` sends a tie past `$83:9229` (below) | - |
| NEXT TRACK's wrap after track 44 (`$80:AE9F`, R-0095) | **Captured** | `twop-hunter-next`: NEXT TRACK from 44 at 4600, track 0 | as above [C] |
| Demo cycles past the twentieth title (R-0096) | **Captured** (the wrap) | `demo-wrap`: cold idle demo, the 32nd race wraps `$77:10C8` from 40 to 0 at 95797 (`$80:94A5`) | the race 2,601/2,639 pictures, the other 38 R-0088's small sprite differences (1-79 pixels); image equal [C] |
| A corkscrew's ejection sound (effect 27, R-0077) | **Captured** | `corkscrew-t14`: COVERAGE-CENSUS run 280, 1P track 14, Right held; `$81:87D6` at 2973 | 69/69 sound commands; 2,100/2,100 pictures; image equal [C] |
| The race clock's 9:5x warning (R-0077) | **Captured** | `clock-warning`: 1P DRAGSTER, pads released to the 9:59.9 limit | 151/151 sound commands to 31900, the eleven warnings (31069-31574) among them; pictures 30000-31917 equal; the result screen then comes 2 frames later in the original (open) [C] |
| The track menu's signed offsets (`$80:BA7C`, `$80:BB1E`, `$80:BB72`) | **Unreachable** [L, S] | the routine's only table is `$80:EA35` (below) | - |
| The sprite-tile queue's last slots and other end | **Open** | a reading of the writer (below); no bound | - |
| A rider pose of 4096 or more (`$81:9E2B`) | **Open** | the pose's composition read (below); the special poses' tables not bounded | - |
| The loop descriptor facing away (`$81:83C4`, `$81:83EC`, R-0108) | **Open** | not attempted beyond R-0108's 16,187-schedule search | - |
| A stunt score of 1000 (R-0107) | **Open** | not attempted | - |
| Track 13's cell past its flag table (R-0107) | **Open** | not attempted | - |
| The cheat page's reveal, the slow-checkpoint clear, BRONSEN's praise (R-0076, R-0077) | **Open** | not attempted | - |

### Unreachable, with the argument

- **The HUD's two-digit lap fields.** Every field shows `$0D15 - $0EFB` (lap number) or `$0EFB - 1`
  (total laps) [L]. `$0EFB` and `$0EFD` are written once per race, at `$82:DBAF-DBB2`: 2 in race mode
  0, 1 in modes 2 and 3, and `$77:0744 + 1` in mode 1 [L]; `$0D15` copies `$0EFB` at setup
  (`$81:D1F9`, `$81:D673`). After that the only write is `$81:8194`'s decrement on a start-line
  crossing, and `$81:8238` sets the finished flag `$0EFF,Y` when it reaches 0, which `$81:8055` tests
  before any further crossing [L]. `$77:0744` is written only by `$83:999D` (the byte of `$83:A254`
  indexed by the track) and `$83:9493` (the defaults table `$83:8472`: 4) [L]; no indexed, long-indirect
  or bank-`$77` store reaches it [L]. The 45 bytes at `$83:A254` are 0, 2, 3, 5 or 7 [S]. So
  `$0EFB` is at most 8, the lap number and the total at most 8, and no field reaches 10. (`$80:B25D`,
  the race line's "10", is unreachable for the same reason.)
- **`$77:0749 = $FF`.** `$0749` is written by `$80:9A01` (from `$017F`), `$83:95F4` (from `$0748`, a
  rider) and `$83:C994` (`AND #$0F`) [L]. `$017F` takes `$FF` only at `$83:990D` (a league pair whose
  second place is empty), whose only caller `$80:BEE4` goes on to `$83:958C` and NOW PLAYING
  (`$80:B18D`); NOW PLAYING replaces any `$017F` of `$10` or more with a computer rider
  (`$80:B297-B346`, then `$80:B369`), before the race setup at `$80:C00A` copies it to `$0749` [L].
  The other race setups (`$80:94C7`, `$80:BC56`, `$80:BD7D`, `$80:BF0F`, `$80:BEB8`) are reached after
  NOW PLAYING or with `$017F` from the demo (1) or a previous race [L]. The defaults leave `$0749` at
  `$65` (`$83:8472`'s text). So `$80:9075` never runs.
- **`$83:D1D4`.** It runs in `$83:D1CA`, NEON's lighting, which `$83:CED0` calls only while `$12D1` is
  set; `$12D1` is set only at `$82:D9A7` and `$82:DC3A`, both on track 42 with `$77:0750` bit 3
  clear, and `$83:D1D4` needs that bit set [L]. No race writes `$0750` (its writers are menus and the
  demo's setup) [L]. `$12D1` does not outlive its race: `neon-then-43` (warm unlocked, 1P track 42,
  quit, then track 43) runs `$83:CED0` on 535 updates of track 42 and never on track 43, whose 1,426
  updates take `$83:CED6` [C].
- **`$83:9296-92AF`.** `$83:9229` is called only from `$80:C04A`, which `$80:C026` reaches only when
  `$83:99C2` finds the riders' totals (`$0769`, `$07D3`) or, in a stunt event, scores (`$07BB`, `$0825`)
  unequal; `$83:925E-9274` compare the same pairs, so their equal arm cannot run [L]. On
  `vs-dragster-tie` the original takes `$83:99EE` (equal) and `$80:C02C` at 5302 and never `$80:C04A`
  [C]; `vs-neon`'s zero-score tie does the same at 5232 [C].
- **The track menu's signed offsets.** The three arms are in `$80:BA6A-BB93`, the list loop entered
  only from `$80:E9CE` and `$80:E9DF` (PICK TRACK), with `$00AE` = `$80:EA35` set at `$80:E9A8` [L]; no
  instruction writes `$00AE-00AF` in between (the only other writers are seven menus' `STX $AE` and race
  code) [L]. The index `$9B` stays within 0 to `$9D` (4 or 5, `$80:E997-E9A3`), and `$80:EA35-EA3A` are
  3, 3, 3, 3, 3, 6 [S]: no byte has bit 7 set.

### The native change: a league member's erase

- **Observed.** OPTIONS' DEFINE PLAYER, the warning and Select+Y+A erase a rider (`$83:9B49`). For each
  of the six league slots whose member mask `$77:02B2 + 2k` has the rider's bit, `$83:9B68-9B78` clears
  the bit and calls `$83:9C47`, which closes the rider's gap in the slot's standings rows (`$02C0`, by
  rider order), pairings (`$05E8`) and event totals (`$0618`, by pairing order), sets a best-time
  holder that was the rider to `$10`, steps the pair cursor `$0678` back past the rider's place, and
  keeps the place and following count in `$10F5`/`$10F7` [L]. Native kept the rider in the league.
  - `erase-colin` (league-three's league T of COLIN, TONY and CRAIG, then COLIN erased at 6100): the
    original runs `$83:9B70` and `$83:9C47` once [C]; with the change native's image equals the
    original's outside the residual at 6300 and 6599, without it 6 bytes differ (`$02B3`, `$05E8-05EA`,
    `$0686`, `$10F7`) [C].
  - `league-erase` (the same league after its first round, 16,000 frames of league-three, then TONY,
    pairing place 0 and best-time holder, erased at 18980): images equal at 16000, 18979, 19200 and
    19799 with the change, 14 bytes differ without it [C] (`reverted.out`).
  - `multi-league` (review round 1's route, `review-1/`, moved here: `make_image.py`, `multi-league.bin`,
    `manifests/multi-league.json`): a warm boot from `league-erase`'s image at 18979 with COLIN put in
    five slots: slot 0 with cursor 2; slot 1 full (eight members), COLIN at place 7, so the scan reads
    slot 2's first pairing `05` and moves it in, the odd-slot holder word `$0688` 7 and the cursor word
    `$0679` 0x0100 (a borrow); slot 3 with COLIN in no pairing place (place 8), cursor 9; slot 5 with
    COLIN at place 0, holder word `$0690` 7 and slot 0's league track `$067E` 1 above its cursor 0.
    The image is hand-made, so the route checks the routine, not reachability. Erased at 1697, the
    original runs `$83:9B70` 4 times, `$83:9C47` 4, `$83:9C8B` (no place) 1, `$83:9D62` (holder) 2 and
    `$83:9D77` (cursor) 4 [C]; with the change native gives 500/500 pictures (1500-1999, the confirm
    frame included) and both images (1696, 1999) equal, without it 55 bytes differ at 1999 [C]
    (`reverted.out`); `$067D/$067E` go from `00/01` to `FF/00` on both.
- **Implementation** (`options_menu.cpp`, `remove_from_leagues`, `remove_league_member`): on the
  cartridge RAM image, because the routine reaches past the slot's own tables [L], and slot 5's
  reaches land in other slots' fields:
  - the pairing scan reads one byte past the slot's eight, `$05E8 + 8k + 8`: the next slot's first
    pairing, for slot 5 the low byte of slot 0's first event total `$0618`; a full run moves it in and
    writes 0x10 over it;
  - a rider in no pairing place writes 0x10 to that byte (`$83:9D0F`) and clears the total word
    `$0618 + 16k + 16`: the next slot's first total, for slot 5 the pair cursors of slots 0 and 1
    (`$0678-0679`);
  - the holder test reads `$0686 + 2k` (slot `k/2`'s holder for an even slot, slot `(k+1)/2`'s best
    time for an odd one);
  - the cursor is compared and decremented as a word with the byte above it: the next slot's cursor,
    for slot 5 slot 0's league track `$067E` (`league_tracks[0]`). With that byte nonzero a slot-5
    cursor always steps back, and a cursor of 0 borrows from it.
  Changed bytes go back through `store_record_byte`.
- **Not matched.** Frame 6102 of `erase-colin`, the picture after the press: the warning's arrow is one
  animation step behind in the original (608 pixels) [C]. It is not this change's: main's native gives
  the same picture (review round 1). That the removal's CPU time pushes the frame's OAM work past its
  end is a hypothesis, and a weak one: on `multi-league` the original runs `$83:9C47` four times on
  the press frame and every following picture is equal [C]. No cycle count was made.

### Open, with what was read

- **The sprite-tile queue.** `$83:F1F2-F2C9` visits each of the five object rows of a rider and, for
  each of 14 bits of the row's 16-bit masks, queues a tile when the bit is set in the overlay or pose
  mask, queues the blank tile `$27:8000` when it is clear in both but was used the update before (the
  stored complement `$0C79,X`), and queues nothing otherwise; `$FFFF` ends the queue [L]. The pose
  masks have six columns a row (`rider_object.cpp`: rows 0-4, columns 1-6), so one rider queues at
  most 30 tiles from those columns [L, R]. But on `vs-dragster-tie` (two views, the second uploader
  `$82:C53D`) the writer runs once a frame and its end mark lands at queue offsets 40 to 140, 20 to 70
  tiles [C]: more than one rider's 30 (a hypothesis: the 14-bit masks carry more than one object); how the slots map to the two
  uploaders, and whether a frame can queue past either end, is not settled here.
- **Poses of 4096 and more.** The collision index is `$0F85` x 8 (`$81:9E1B`); `$0F85` copies `$0411`/
  `$0413`, which copy `$0FF3`/`$0FF5`, which come from `$0FA1`, set at `$83:EF50` to `$0F8D` x 64 +
  `$0F81` (+ `$0820` or `$08E0` + `$0FE3` x 64 for a rolling rider), at most 2,463 for 24 wheel phases,
  or to an override `$0F59` [L]. The overrides are constants and table sums (`$81:83CD`, `$81:841B`,
  `$81:890E`, `$82:962D` from a table read through the data bank, `$82:A45E` from `$82:A2DB`) [L];
  `$82:962D`'s table and data bank were not resolved, so no bound is given.
- **The clock-limit result.** After `clock-warning`'s race ends at the limit (the player never
  finished), native's result screen fades in 2 frames before the original's (31918 on), and its first
  command (2, 6) comes at 31924 against 31926 [C]. Not traced; main's native gives the same pictures
  (review round 1).
- **Pictures not compared:** `vs-neon`'s race (every picture differs; at 2400 by 859 pixels, R-0103's
  two-view NEON edge lines); `league-erase` from 15900 (most pictures differ by 1-2% of their pixels,
  the checkered background, before the erase too; from 16302 the equal pictures come every other
  frame, which suggests the background animation one frame out of phase; not traced); `demo-wrap` from
  98340 (the return after the race: frames 98346-98351 show the menu fading in the original and black
  in native; not traced). Review round 1 ran main's native on `league-erase`, `clock-warning`,
  `demo-wrap` and `erase-colin` without skips: the same frames differ, and every native picture is
  byte-identical to this change's, so all four differences are pre-existing.

## Limits

- `$83:9C47`'s reaches past a slot run on `multi-league` (a hand-made image): a full slot, a rider in
  no pairing place, odd-slot holder words, a cursor borrow and slot 5's cursor over `$067E`. Not run:
  a full slot 5, whose scan reads `$0618`, and a slot-5 rider in no pairing place, which clears
  `$0678-0679`.
  The holder test compares the word `$00CA-00CB` with the rider; `$00CB` is taken as 0 (both captures
  agree, not proved).
- Sound commands are compared, not PCM; audio past `demo-long`'s twentieth title is not compared.
- The race rows of `twop-hunter-next` are not compared: the runner's race timeline does not serialize
  a two-human lap race on track 44 ("split race state requires native ZOOM ZOO, local DRAGSTER, a
  two-pad stunt event or a split idle demo"); its pictures are.
- The static map keeps its eight inputs: this task's PC traces are not coverage files, so the census
  classes are not regenerated.
- The race-row comparison masks one thing more than RACE-BRANCH-REMAINDER's: a later race's
  published totals and graph extrema (`$77:106F-1072`, `$77:0618-061B`) while they equal the race
  before's (R-0049: a race state holds 0 there until its result), as TWO-HUMAN-STUNTS' race_compare.py
  does off a warm boot; and it passes a warm boot's image to native.

## Reproduction

From a checkout of main (`local/` linked to the main checkout's), `E=local/evidence/open-branches`:

```
python3 $E/make_unlocked.py $E/unlocked.bin
python3 $E/orig.py $E/manifests/NAME.json $E/runs/NAME.json [--image $E/unlocked.bin] --watch PC... --digests LO HI --sram F...
python3 $E/rows/probe.py $E/rows/NAME.json $E/rows/probe-NAME --frames N --dump-from F --wram-bytes 0x4000 [--image ...]
zsh $E/check.sh NAME OUT     # each line of checks.list, about 1 min for all nine
bash $E/regen.sh; python3 tools/project.py coverage native-symbols --check
```

The manifests' descriptions give each route's presses; `menu.py` writes a manifest from a press list.
