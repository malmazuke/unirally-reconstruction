# R-0117 - The saved image's differences R-0111 left without an argument

Status: research result and native change ([SAVE-IMAGE-REMAINDER](../../tasks/SAVE-IMAGE-REMAINDER.md)),
12 November 2026, on main `5c7522ee` (claim `cf15c6b4`). PAL ROM SHA-256
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, reference core bsnes `7d5aa1e6`
(patched, Strict). Evidence in main `local/evidence/save-image-remainder/`. It works through
[R-0111](R-0111-save-image-exact.md)'s Value list: the differences between native's cartridge RAM
image and the original's that R-0111 could not argue away.

Tags: **[L]** read from the static map's listing (main `local/evidence/static-code-map/static-map/`)
or the ROM's bytes; **[C]** a capture of the original made for this task (R-0111's per-frame logs in
`save-image-exact/original/`, the instruction trace `voice_trace.py`, work RAM watches
`wram_watch.py`, pictures, perturbed boots `perturb.py`); **[R]** an accepted record.

## Question and answer

For each Value item: does native's image equal the original's at every write frame of R-0111's
corpus (919 runs), and where not, can the difference matter? **Answer:** the endings' `$10A7`, the
rider names' leftover bytes, the end of `league/organic-full-tour` and the league pair's race words
are closed (three by native changes, the rest argued unread with a perturbation). The hunter-effects
personal bests and `$1116` are explained (a race started a frame early) and closed only in the
comparison, which takes the captures' race starts (D-0010): in play they still reach a save. Five
remain, queued as
[SAVE-IMAGE-REMAINDER-2](../../tasks/SAVE-IMAGE-REMAINDER-2.md).

## Method

- **The corpus** is R-0111's (`save-image-exact/routes.json`) with one change (`routes.py`,
  `routes.json` here): each of the 527 `track_reference` runs whose capture records an
  `initialization_frame` passes it as `--race-initialization`, as every other race check in the
  laboratory does (R-0077; item 1). 484 already started there.
- **The comparison** is R-0111's (`compare.py` against `save-image-exact/original/`), through
  `sweep.sh` here; the residual bytes are R-0111's plus three ranges argued below (`residual.txt`).
  `classify.py` (R-0111) classes what is left as shift or value.
- **Main** is `base/front_end_runner`, lab-release built from the claim `cf15c6b4`.

## Findings

### 1. The personal bests on the hunter-effects routes: a race that started a frame early [C, L, R]

`c175`, `c177`, `c210`, `c220` (HUNTER's tour, track 41, preloaded image with `$77:10B1` = 0, so the
race's song counter, which indexes R-0077's table, is 1):
native added 1 and 2 to `$77:085F` (`$085B` on `c177`) at frames 1973 and 1991, through the
opponent's voice class read past rider 1's tallies (R-0098, `count_opponent_voice`).

- **The original takes no class there: it queues no voice.** Its rider-1 queue (`$0CEB`, cursors
  `$0D11`/`$0D13`) takes one event on the whole run, `0x12` (one z flip) at 1929, and the consumer
  `$81:C219-C226` finds it empty on every pass after (trace 1960-1995). Native's opponent landed at
  1929 with a roll, two twists and a z flip, combination 36, so it queued voice 245 twice.
- **The opponent's landing differs because its race does.** In the original rider 1 brakes at 1928
  (`$82:A3D7-A3FB`: `$031F` A held and `$127D` set, so `$0FA9` halves and `$0F6B`, the air turns,
  clears). Native's opponent's x equals the original's (`$0417`) on every update to 1819, then gains
  speed: native's player counts its fifth track marker at 1820 (`$0FCD`), the original's not by
  1830, so the opponent's catch-up (`$82:A788-A797`, twice `$1283` = 0x40) raises its cap to 0x200.
- **The player's race differs from its first moving frame**: native's velocity is 0x88, 0xA0, ...
  where the original's `$04BB` is 0x98, 0xB0, ...: the same as Right pressed one update later.
  Native's race started one frame early: its first update follows R-0077's (track, song) loading
  table (track 41, the race's counter 1: 69 + 80 - 1 frames after the choice), and the original's sound upload
  took 81 frames here. With `--race-initialization 1391`, the capture's frame, the velocities are
  equal.
- **Over the corpus** (`race_starts.py`, `race-starts.json`): native's first update equals the
  capture's on 484 of 527 track_reference runs; it is one frame early on all 34 runs of track 41
  and all 8 of track 43 with `$10B1` = 0 in the image (the race's counter 1; the preloaded HUNTER
  tours, two preloaded starts rather than 42 independent samples), and one frame late on the one run
  of track 8 with `$10B1` = 0. R-0077 records the upload's variation with the sound processor's
  state, and [D-0010](../decisions/D-0010-frame-anchored-sound-commands.md) keeps the table in the
  front end and has the laboratory take a race's start from its capture where they differ; this
  task follows it and does not change the table.
- **In play the difference stays.** A player on those tours still starts a frame early natively, so
  the saved personal bests (`$085F`/`$085B` on `c175`, `c177`, `c210`, `c220`) and the tutorial bits
  `$1116` (`c209`, `c217`, `c218`, `c272`) can still differ from the original's in a save a later boot
  reads. The comparison's race starts take these 8 routes out of `check_image.sh`;
  `check_race_starts.sh` pins native's own start on all 527 runs instead (any move fails), and the
  item is queued as SAVE-IMAGE-REMAINDER-2 item 5.
- With the captures' race starts (`routes.json`), the personal-best increments are gone on all
  four routes, and so is `$1116` on `c209`, `c217`, `c218` and `c272` (R-0111 counted three):
  their hints ended a frame apart for the same reason. No class question remains: when a voice is
  queued, the listing's class read (R-0098, confirmed on `odd-tours` and `five`) stands.
- **Overlap with OPPONENT-POSE-BOUNDS:** none. That task is the presentation's colour maths read past
  its table for opponent 0x14 (`presentation.cpp:243`) on the same routes; the race start and the
  voice path here do not touch it.

### 2. The endings' `$10A7` [L, C]

The gold endings use the markers' word `$77:10A7` as their fall and rise counter: nine stores in
bank `$83` (ROM search for `$77:10A7` operands): word stores with a 16-bit accumulator at
`$83:B649` (WALKER's hit: 0), `$83:B9A3` (BOUNDER's ball: 0), `$83:BA03` (its fall, `INC`),
`$83:C030` (SPRINTER's knock: 0xFFF4), `$83:C0D7` (its tumble, `INC`); byte stores at `$83:B667`
(WALKER's fall, `INC`), `$83:BA7E` and `$83:BB32` (the bound's rise 0x0C), `$83:BAE7` (its `DEC`).
Native modelled the counter (`TourEnding::drop`) with these widths but kept it out of the image.
It now stores each in the image (`store_drop`, `store_drop_low_byte`, `tour_ending.cpp`): `all-gold`,
`locked-gold` and `walker-gold` (`g038`, `g041`, `g045`) are equal outside the residual at every write
frame (276, 79 and 125 write frames before), the high byte included (`0xFFF4` to `0x0020` across
zero on `all-gold` 31208-31278).

A later boot reads a saved `$10A7` as the markers' step (`$80:EB28-EB32`, a word: `DEC`, below 0
becomes 13). A save made during an ending can now hold a step of 14 to 0x7FFF (0x0020 at SPRINTER's
end); the original then counts it down, reading the marker table `$80:EB61,X` and `$80:EB61 + 7,X`
past its 14 bytes into code. Native starts such a step from 0, as R-0100 left it (`cartridge_ram.cpp`);
steps 0x8000 and up behave the same on both sides (13 at the first step). Queued (item 3 below).
Within a session the two users of the word cannot conflict: an ending returns to PICK TOUR, and every
way out of it stores 0 (`$80:BC7B-BC7E`, `JSR $E84E` to `$80:E872`) before any marker turn (review 1's
extended `g045` run: 0x20 to 0); native keeps one shared offset (`marker_step_word`).

### 3. The rest of R-0111's Value list

- **`$1116`, three track_reference runs** [C]: the race start (item 1).
- **The names' checksums `$016C` and `$022E`** [L, R, C] (8 routes differed on `$016C`: the
  "credits" and "faedine" renames leave it stale in the original, R-0094; native rewrites it): their
  only reader, `$83:89D9-89F5` (and `$83:8A15` for `$022E`), has no caller (R-0112's unreferenced
  routines). Perturbed boots (`perturb.py`, `$016C-016D` and `$022E-022F` XOR `0xA5` and 0, on R-0111
  review 1's nine warm routes): picture, work RAM and the rest of cartridge RAM equal to the
  unperturbed run at every frame (`perturb-016C-016D.json`, `perturb-022E-022F.json`; the sound
  digests differ from frame 0 in pool processes, R-0111's known limit). Added to the residual.
- **`$0744-074B`, the race's track, kind and rider words** [R, C] (12 league routes; the league
  pair's swap `$83:95C4`/`$83:95CA`, which native does not model, and timing): read within a session
  (NOW PLAYING `$80:B1D1`, `$80:B1E4`; `$83:9975`; the swap `$83:95E3-95F4`), but each read follows a
  race setup's stores (`$83:9983` from `$80:B18D` and `$80:E9F8`, `$80:99FA`, `$80:9A01`), so every
  race setup rewrites them before any read; R-0090 found no read by a later boot;
  perturbed on the nine routes, equal at every frame (`perturb-0744-074B.json`). Added to the
  residual. (`$074C-074F` are read and stay checked.)
- **The rider names after their terminator** (`rv-warn`, `w5`, `warnings`: `c042`, `c053`, `c054`)
  [C]: these runs rename a rider from DEFINE PLAYER's warning. The keyboard's buffer `$00DC` keeps
  what the text printer last left there, the name at `$00DE`, and a short name's commit copies that
  tail (native already modelled this with STEVE, the picker's last name). The warning prints the
  chosen rider's name (`$80:C12A-C1EA`, its F8 operand), so on this path the tail is that rider's
  name as it was before the warning's confirm cleared it ("t", `FF FF`, then "arti" of MARTIN, "ike" of
  MIKE). Native now keeps the warned name (`FrontEndState::warned_name`, cleared when OPTIONS opens
  a picker): the three routes are equal but for `$0743`'s known one-frame bit timing on `c042`. A
  perturbation of every rider record's bytes after `FF FF` changes no picture on the nine routes
  (work RAM and other cartridge bytes change: the race start's copy and the name buffers).
- **`$10B1` and `$074D` at the end of `league/organic-full-tour`** (`g070`) [C, L]: not a store
  timing (R-0105 read it as one) but a different screen. Left is held on both pads from 32,786 to
  60,764; at the cycle's last LEAGUE TABLE the original prints the table (`$80:A547`), waits in
  `$80:C206-C24B` for any button of either pad on two reads, finds Left, sees no Y or X (`$80:B74A`)
  and goes to the podium at 60,757 (`$80:BE76-BEB5`: `$0742` bit 13 cleared, the three places stored,
  `$80:999F`). Native took only B, Start or A there and waited for the Start at 61,415: 659 frames
  on the table where the original shows the podium (pictures at 61,000). Native now takes any
  button but Y or X (`league_table_frame`). `g070` goes from 255 differing write frames outside the
  residual to 17, all the 3-frame league timing R-0111 records; its podium pictures equal the
  original's two frames later (`g070pics`: 0 differing pixels at the original's 61,002 against
  native's 61,000), the same lead. The other 918 routes' menu states and cues are unchanged
  (`stdout_same.py`, `stdout-main-vs-candidate.json`).
- **Still open** (queued):
  - `$0742` bit 10 and `$10A7` on `c385` (`dragster-ordinary-controls` fuzz run 155): after its race
    the fuzz pads drive the menus, and the original raises the logo (`$0742` bit 1) at 3,527 where
    native does at 3,570 with the pad gate's bit 10; from there the menus differ (the original's
    PICK TRACK markers run 3,678-3,737, native's not). A front-end behaviour difference, not an
    image one.
  - The league names' bytes after their terminator on `c039` (`rv-pick`): the league editor's buffer
    tail `$00E1-00E2` is `5F 5F` in the original where native's start (`$80:9E0F`, R-0097) puts
    `58 FB`. These bytes are read: perturbing every league record's bytes after `FF FF` changes
    `c116`'s picture from frame 6,757 (XOR `0xA5`; zero changes nothing) (`perturb-leagues-after-terminator.json`).
  - A saved marker step of 14 to 0x7FFF (item 2).
  - The league marker's stop and run a frame or a few apart on `c029`, `c032` and `g069` (1 to 5 value
    frames each at the edges of R-0111's timing spans).

## Results

`check_image.sh` here (the corpus with race starts, `expected.json`); write frames from frame 406
differing outside the residual bytes (`residual.txt` here), of 363,007 [C]:

| Runner | Differing write frames | Routes equal (of 916) |
| --- | ---: | ---: |
| main, R-0111's corpus and residual (R-0111) | 14,380 | 709 |
| main, with the race starts, R-0111's residual | 14,364 | 717 |
| main, with the race starts, this residual | 13,986 | 719 |
| candidate | 11,879 | 724 |

Every write frame, with all bytes: 183,128 differ on main, 181,739 on the candidate. Routes that
change, main to candidate: `g038`, `g041`, `g045` (endings), `g070` (the table), `c042`, `c053`,
`c054` (the names); no other route's image moves. R-0111's own check (`save-image-exact/check_image.sh`,
no race starts) on the candidate differs from its `expected.json` on the same seven routes; its
re-recorded result is `expected-save-image-exact.json` here.

## Limits

- The race start follows the capture only in the comparison (D-0010); native's front end still
  starts by R-0077's table, so a player's race on HUNTER's preloaded tours begins a frame early and
  the difference reaches cartridge RAM a later boot reads (the personal bests, `$1116`). Modelling
  the sound processor's upload time is R-0077's limit; queued as SAVE-IMAGE-REMAINDER-2 item 5, and
  native's start is pinned by `check_race_starts.sh`.
- The arguments for `$016C`, `$022E`, `$0744-074B` and the rider name tails rest on the nine warm
  routes' perturbation and the listing; the name tails' only observed readers are copies.
- The warned name is kept for the define-player path alone; another screen printing a rider name
  between the warning and the keyboard is not in the corpus.
- The league table's any-button exit is observed with Left (`g070`); other buttons follow from the
  listing (`$80:C206` tests the whole pad words `$72`, `$74`).
- Behaviour: menu states and cues on all 919 routes; pictures checked on `g070` at chosen frames.

## Reproduction

From the candidate's worktree (`local/` linked to main's), after building lab-release,
`R=local/evidence/save-image-remainder`:

```
python3 $R/routes.py local/evidence/save-image-exact/routes.json $R/routes.json
python3 $R/race_starts.py build/lab-release/src/core/front_end_runner race-starts.json 8
local/locks/heavy-run.sh SIR zsh $R/check_image.sh OUT                     # PASS
local/locks/heavy-run.sh SIR zsh $R/check_image.sh OUT $R/base/front_end_runner   # FAIL, 7 routes
SET=016C-016D python3 $R/perturb.py OUT.json 8 c115 c116 c083 c085 c077 c175 c263 c268 c290
python3 $R/stdout_same.py $R/base/front_end_runner build/lab-release/src/core/front_end_runner OUT.json 8
python3 $R/voice_trace.py c175 1960 1995; python3 $R/wram_watch.py c175 1930 0CEB-0D0A 0D11-0D14
```
