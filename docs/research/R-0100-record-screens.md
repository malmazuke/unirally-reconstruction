# R-0100 - The records screens and the rider word after a result

Status: research result and native change ([RECORD-SCREENS](../../tasks/RECORD-SCREENS.md)), 23
October 2026, on main `ff1efb6`. PAL ROM SHA-256
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, reference core bsnes
`7d5aa1e6` (Strict); sound schedules with R-0075's observation core v5. Evidence in main
`local/evidence/record-screens/`. It closes three items of R-0097's Not covered: HIGH SCORES and
TRACK RECORDS frame by frame, `$00CA`'s writers on the result screens, and `$80:FBEE`'s menu.

Tags: **[L]** read from the static map's listing (main `local/evidence/menu-input/listing/`,
R-0089's recipe) and a linear decode (`disasm.py`) of the routines it leaves as data; **[C]**
confirmed in a capture.

## Domain

Nine probe captures of the original, each a manifest in main `local/evidence/record-screens/`:

| Capture | Start | Frames | What it runs |
|---|---|---:|---|
| `rs-track` | warm: SAVE-FILES' `league_organic-full-tour` | 3,200 | TRACK RECORDS: taps and holds of each pad's four directions, Up+Right, a reversal mid-scroll, pad 2 Y mid-scroll; again, left by Start held from before the loop; HIGH SCORES: B for one frame, pad 2 X then pad 1 A; again with Select in its print and pad 2 L held from before its wait |
| `rs-high` | warm: `front-end-endings_decode_all-gold` | 3,300 | HIGH SCORES: Up during the slide, pad 2 Right for two frames; TRACK RECORDS: Down held past the last tour, the stunt column, pad 2 Up held, X; again, pad 2 B; HIGH SCORES: Start for one frame, pad 2 R held |
| `rs-ca-2p-lap` | cold: SPLIT-RACE-END's `zzap-long` (2P ZOOM ZOO) | 10,453 | the lap result, the continuation's QUIT, OPTIONS, RECORDS, PLAYER SCORES |
| `rs-ca-vs-lap` | cold: VS ZOOM ZOO with `zzap-long`'s riding | 10,768 | the lap result, VS CHAMPIONS, pad 2 picks the challenger, QUIT, PLAYER SCORES |
| `rs-ca-2p-stunt` | cold: TWO-HUMAN-STUNTS' `twop-bowl` (2P BOWL) | 6,394 | the stunt result's two tallies, QUIT, PLAYER SCORES |
| `rs-ca-vs-stunt` | cold: `vs-bowl` (VS BOWL) | 6,809 | the stunt result, VS CHAMPIONS, the challenger, QUIT, PLAYER SCORES |
| `rs-ca-np` | cold: `twop-bowl` | 6,650 | the stunt result, SAME TRACK, NOW PLAYING left with Y, QUIT, PLAYER SCORES |
| `rs-podium-menu` | cold: LEAGUE-TOURS' `podium-back` | 13,300 | the continuation after the podium: Down tapped and held, both wraps, pad 2 Down, Select, Up for one frame, Select+Up, Y |

and MENU-INPUT's review capture `rv-records` (cold: TRACK RECORDS and HIGH SCORES with each pad).
The `rs-ca-*` manifests come from `make_ca.py`, which places each tail's presses from native's
screens. Five sound schedules: `rs-track`, `rs-high`, `rs-ca-2p-lap`, `rs-ca-vs-stunt` and
`rs-podium-menu` (the last derived with PODIUM-AUDIO's `derive_cues2.py`, copied as
`derive_cues_podium.py`, which names the podium's session).

Each probe capture holds work RAM `$0000-$1FFF` and cartridge RAM every frame from power-on and a
picture every frame from its first listed frame (`probe.py`). `compare.py` (MENU-INPUT's, with
`--rider-word`) compares native's front end with it on every menu frame (the arrow, the palette
cycle, the logo, the slides, the decorations, the menus' words, the OAM buffer, the text map and,
new, `$00CA` from the runner's `--rider-word` log) and every picture. `pcs.py` places routines on
frames with the core's instruction ring. A warm boot writes the saved image before frame 0
(`probe.py --image`; native `--cartridge-in`); its sound schedule gives the observation worker,
which starts from an empty cartridge RAM (`$FF`), the image's other bytes as writes after frame 0:
a probe so written equals the `--image` probe on every frame from 1 (1,300 frames of `rs-track`
checked). Each schedule is captured twice (`cue_captures.sh`) and compared with native's cues
frame by frame, frame waits included (`diff_cues.py --waits`).

## Findings

1. **The frame model.** A frame of the original is the code between two returns from the frame
   wait `$80:FADF`; a frame that never reaches one (a long print) runs into the next, which then
   starts without a frame wait, and the work RAM sample at its end catches the print half done.
   [L, C: `pcs.py` on `rs-track` and `rv-records`]
2. **TRACK RECORDS' entry** (`$80:D5E6`, `$80:EB76`, 88 frames after the choice's). [L, C:
   `rs-track` 1100-1188 and 2050-2138, `rv-records` 1100-1188 and 1950-2038]
   - RECORDS' choice slides the logo up (`$80:D54B`, `$80:F51B`) for every category, and
     `$80:D5E6` starts with a frame wait. Native raised the logo at once (`$80:F53F`).
   - Frame 1: the medals' colours (assets 34, 33, 32 at 0xB0, 0xC0, 0xE0), the forward slide's
     sound, the HDMA tables copied to `$0C70` (`$80:EB7B-EBA2`: `$80:ECD1`, or `$80:ECF7` with the
     shown half at word 0x1400: its BG2HOFS entries are 0x100), HDMA channels 5 (BG2VOFS) and 6
     (BG2HOFS) on, `$008F` cleared, the text cleared. The title's band starts with no lines, so the
     tables write nothing yet.
   - Frame 2 (`$80:EBE2`): the cleared text to both of the hidden half's maps, the title
     (`$80:D729`) printed; frame 3: the title to the shown half's second map, the OAM, the arrow's
     targets (0x0600, 0x0740), the wipe's first step.
   - Frames 4-60, the wipe in (`$80:A031`, each pass `$80:A04C-A091`): 57 passes, each after a frame
     wait and an OAM copy. The grid's step `$00A1` (`$80:A039-A047`) rises by 1 a pass for 8 passes,
     holds for 24 and falls by 1 for 25 (a negative step adds nothing: 256 pixels in all) and is
     added to both grid entries' BG2HOFS; the title's band takes a line from the grid's upper part a
     pass for 39 passes, and the lower part gives one to the bottom lines until it is 64 lines.
   - `ADC $A1` reads a word: `$00A2` holds 0x4C from the boot's `$80:A1A1-A1A4` (`STX $A2` of
     0x044C) in every capture, so each pass also adds 0x4C00, which BG2HOFS's ten bits do not
     show. [L, C: `$00A2` = 0x4C from frame 228 in `rs-track`, `rv-records`, `kb-input`]
   - Frame 61 (`$80:EC20`): the grid's offset's low bytes cleared (`STZ` in 8-bit mode), the scroll
     0x157; frame 62: assets 29 and 30 to colours 0x00 and 0x10, the text cleared; frame 63: the
     first half of the grid (tours 0-4, `$80:ED1D`), which runs past the frame (no frame wait at its
     end: 1163 in `rs-track`); frame 65: that half to the hidden half's first map, the text cleared;
     frame 66: the badge tiles (`$80:A82B`); frames 67 and 68: the base colours (`$80:A858`), the
     forward slide's sound again; frames 69-86: the grid's 18 passes of 8 pixels down to 0x1E7, the
     last one followed by the second half (tours 5-8), which runs past its frame (1186); frame 88
     (`$80:ECAE-ECD0`): the second half sent, the result's sound, the medal objects (entries
     112-114) and the printed icons (entries 104-106, raised 5 pixels), and the loop's first pass.
   - `$80:ED1D` leaves the chosen track `$00CE` on 25 after the first half and 45 after the
     second (5 a tour); native left it as it was. A later PICK TRACK starts from it.
3. **TRACK RECORDS' loop** (`$80:D682-D71C`). [L, C: `rs-track` 1188-1903, `rs-high`]
   - A pass after its frame wait: the colours of the last print's three holders (`$00BC`, `$BE`,
     `$C0`, asset holder + 6 at 0x80, 0x90, 0xA0), the text's rows 22 on to the shown half's
     second map (`$80:93CB`, 0x1FF bytes: the last word's high byte stays), the OAM and the pads
     (`$80:D1EC`), the tests, `$80:E3E9`, then the print of the three records under the grid and
     the decorations' step (`$80:D6D5`).
   - Either pad through the gate: a choice (B, Start, A) or Y or X leaves at once, held or not
     (`$80:D70F-D714`; `rs-track` 2138: Start held from 2130 leaves on the loop's first poll).
   - The scroll (`$80:E3E9-E52F`): `$0000` the offset, `$0002` a step 0-10 into `$80:E536`'s
     speeds (-4, -3, -2, -2, -1, 0, 1, 2, 2, 3, 4), `$0078` the drive (0 faster down, 1 easing, 2
     faster up). At rest (offset & 15 = 7) Down drives down and Up up (Up wins); at a row's end
     (& 15 = 15) with neither held the drive eases to the next rest, from step 10 going down or
     step 0 going up; offsets off that rhythm are moved a pixel back onto it first. A tap moves
     16 pixels, a held direction 4 pixels a frame until released at a row's end. Native moved 32
     pixels for any press, on a fixed table.
   - Left and Right, each latched in `$008F` (bits 1 and 0), play the move's sound on every press
     and move the column within 0-4 without wrapping (`$80:E483-E4E4`): Left first, then Right.
     Native wrapped, shared one latch and played the sound only on a move (`rs-track` 1620, 1760:
     a press at an end plays it).
   - The print (`$80:D682-D6D3`): the text cleared, the template `$80:D777`, then for the tour
     under the arrow (below tour 9) each rank's line: a race's time at column 6 (`$80:C30E`) or,
     in the stunt column 2, the score as five digits after two blanks at column 7 (`$80:C269`),
     then "_by_" and the holder's name. Native printed a time in every column (`rv-records`
     1381-1529).
4. **TRACK RECORDS' way out** (`$80:D71F`, `$80:9F7A`, 62 frames). [L, C: `rs-track` 1903-1965,
   2139-2201]
   - The leaving frame: the logo down (`$80:F52B`), the printed icons hidden (`$80:D420`), the
     medals hidden, the forward slide's sound, the text cleared. Then the cleared text to both
     hidden maps (`$80:9F8B`); the menu's text tiles (`$80:A877`); the base colours (`$80:A8A8`),
     the scroll's entries 0 (`$80:9FB1`, which BG2VOFS keeps once HDMA is off); the wipe's first
     step (`$80:9FBF`, the step `$80:9FC7`); 57 passes of the wipe out (`$80:9FDA-A01D`: the grid
     moves right, the title's band shrinks to one line and then clears the edge line's high byte;
     the lower part grows every pass); then HDMA off (`$80:A029`), `$80:D55F`'s logo down and
     RECORDS' loop from its first pass (`$80:B93C`), without a new print. Native did all of it in
     one frame and printed and slid the RECORDS menu in again.
5. **HIGH SCORES** (`$80:D7CD`). [L, C: `rs-track` 2350-2541, 2650-2736, `rs-high`, `rv-records`]
   - The choice's frame clears the text, finds the leaders and prints `$80:D9D8` with the eight
     printed objects hidden; the print runs past that frame and ends in the next, which has the
     first frame wait (`$80:D908`). Then the OAM, the text to the hidden half, the arrow off and the
     forward slide (`$80:D90B-D914`) (39 frames); then one frame wait each for the leaders' colours:
     wins, losses and races at 0x80-0xA0, the best and worst win averages and the total score at
     0xB0-0xD0, the best and worst score averages at 0xE0-0xF0 with the objects shown; then
     `$80:C206`.
   - `$80:C206`: any button of either pad (the whole words, directions included) on two frames
     running, each frame a frame wait, the pads and the decorations' step; pads may differ
     (`rs-track` 2500-2501: pad 2 X then pad 1 A leaves). A press on one frame does nothing
     (2450). A button held from before leaves on `$80:C206`'s second frame (2694-2696).
   - The way out (`$80:D9B5-D9D7`): the printed objects hidden (`$80:D420`); after a frame wait the
     OAM, the object colours (`$83:91F7`, assets 28 and 5 at 0xD0, 0xE0) and the back slide
     (`$80:E27E`); on its last frame the logo down and RECORDS' loop from its first pass. Native
     left on pad 1's B, Y, X, Start or A at once and printed the RECORDS menu again.
   - The leaders (`$80:D7D5-D8F8`), exactly: every leader starts on rider 15 (`$80:D8C3` turns the
     loops' counts into riders) and the worst averages at 7000. Races and losses (races less
     wins) go to the last rider with `$83:97BD`'s largest, wins to the first. The win averages are
     `$83:9D80`'s percentages of the riders with races (the halving below 655, and the last one
     left in `$77:10F9`/`10FB`); best is the last of the highest, worst the first of the lowest.
     The total score is the last of the highest; the score averages are score / races
     (`$80:9719`) over the riders with a score. With no race at all both worst averages are 0 and
     every leader SOMEONE. Native used 100 x wins / races, took the first of a tied best and
     ranked the score averages differently (`rs-track` 2352-2501: the worst win average's rider).
6. **`$00CA` after a result.** [L, C: `rs-ca-*`]
   - The one-run result's rows (`$80:CF60-CF8E` dispatch by row kind): the player's row writes
     `$017D` (`$80:CFA3-CFA9`), the opponent's row `$017F` (`$80:D009-D00F`, before its test for a
     computer); the last row printed stays.
   - The lap result writes the rider (`$80:903E-9041`) and then the opponent (`$80:90D6-90D9`,
     every opponent, a computer's too); the stunt result writes the rider at the end of its tally
     (`$80:F224-F227`) and a second human's at the end of theirs (`$80:F2A9-F2AC`); the 1P
     scoring writes the rider (`$83:879B-879E`). Those words are stored in 16-bit mode (`$00CB`
     takes `$017E` or `$0180`), which native does not keep.
   - NOW PLAYING pushes `$00CA` as its print begins (`$80:B194`) and pulls it back on Race or Y
     or X (`$80:B4BF-B4C0`), not on Exit. Race's value lasts the 7 frames until the race's setup
     writes the first rider (`$80:99E8`).
   - After a VS challenger pick, the continuation's NEXT or SAME TRACK prints NOW PLAYING
     (`$80:B18D`), whose name lines write `$00CA` (`$80:B5C0`): native since MENU-INPUT's round 1
     (`note_named_riders`); the captures confirm it.
   - So after a 2P or VS lap result PLAYER SCORES starts on the second rider, after a 2P or VS
     stunt result on the last tally's rider; main started on the first.
7. **PLAYER SCORES' mask.** `$80:DACF-DAD6` masks `$00CA` to 0-15 in RECORDS' choice frame,
   before its first frame wait; native masked it a frame later (MENU-INPUT review's `rv-ca` 3450:
   135 for 7 after RENAME PLAYER's Y). [L, C: `pcs.py` on `rv-ca`]
8. **The league's continuation after the podium** (`$80:FBEE`) runs the generic list loop: the
   hidden markers turned, the move's sound, the wrap, Y or X acting like a choice. Native since
   LEAGUE-TOURS (R-0098 finding 5); this task's capture confirms each input. [L, C:
   `rs-podium-menu`]
9. **A warm boot's markers.** `$77:10A7`, the done-track markers' step (`$80:EB23`), is cartridge
   RAM: a warm boot goes on from the saved value, where native started from 0, so every OPTIONS
   and RECORDS frame's hidden marker tiles differed on a warm boot (`rs-track` from 660, as main).
   Native now takes it from the image. [L, C]

## Results

`checks.sh` at `2a15a8c` (pack v40 on both sides; main is `ff1efb6`'s runner, `base-ff1efb6/`).
"Other" is the candidate's differing menu frames outside the pictures.

| Capture | Pictures equal (candidate / main) | Other (candidate) | Sound schedule (candidate / main) |
|---|---|---|---|
| `rs-track` | 2,200 of 2,200 / 487 | the four grid prints' and two HIGH SCORES prints' frames (text, `$00CE`, OAM) | 2,630 lines from 620: 0 frames differ / 20 |
| `rs-high` | 2,300 of 2,300 / 50 | the same prints (1050, 1463, 1486, 2263, 2286, 2500) | 2,712: 0 / 31 |
| `rv-records` | 2,100 of 2,100 / 1,141 | the same (1163, 1186, 1700, 2013, 2036) | - |
| `rs-ca-2p-lap` | 2,153 of 2,153 / 1,878 (PLAYER SCORES 10090-10364) | the lap result's first frame (8412, as main) | 16,414: 0 / 0 |
| `rs-ca-vs-lap` | 2,568 of 2,568 / 2,293 | the result's and VS CHAMPIONS' first frames (as main) | - |
| `rs-ca-2p-stunt` | 1,694 of 1,694 / 1,419 | none | - |
| `rs-ca-vs-stunt` | 2,109 of 2,109 / 1,834 | VS CHAMPIONS' first frame (as main) | 9,289: 0 / 0 |
| `rs-ca-np` | 1,950 of 1,950 / 1,675 | none | - |
| `rs-podium-menu` | 1,000 of 1,000 from 12300 / 1,000 | the podium's own frames 11408-12305, as main (fields the podium does not keep) | 14,297: 1 frame (10603, the awards' frame wait) / 1 |

`$00CA` is equal on every compared frame of every capture above and of MENU-INPUT's (below).
MENU-INPUT's captures and its review's, candidate and main on pack v40, are as MENU-INPUT
recorded: `w5` 3,800 of 3,800, `kb-input` 2,100, `league-pick` 2,400, `league-name` 2,143 of 2,400
(the long-name lag), `opt-backs` 2,300, `warnings` 3,200, `records-input` 1,650, `scores-input`
1,950, `scores-carry` 3,100, `vs-challenger` 700 of 700 from 6600, `twop-quit` 1,000,
`awards-press` 700 of 700 from 10500; `rv-ca` 3,400, `rv-scores` 2,300, `rv-bars` 1,400,
`rv-pick` 2,200, `rv-warn` 6,700, `rv-kb` 1,879 of 2,300, `rv-ca2` 2,600, `rv-ca3` 2,700,
`rv-figs` 2,300; their sound schedules 0 frames differing but `league-name` 5 and `awards-press` 1
(as recorded). Main's PLAYER SCORES pictures differ in every `rs-ca-*` capture because it starts on
the first rider.

## Not covered

- **Prints that run past their frame.** The frames where the original's print is still running
  at the work RAM sample: TRACK RECORDS' two grid halves (the text and `$00CE` on frames 63 and
  86 of its entry: `rs-track` 1163, 1186, 2113, 2136) and HIGH SCORES' leaders (the choice's frame:
  the text and the first printed objects, 2350 and 2650). Native prints each whole in one frame;
  no picture differs.
- **Assets 29 and 30** (TRACK RECORDS' entry frame 62, colours 0x00-0x1F) are not in pack v40, and
  native skips them while the pack lacks them; the base colours overwrite them five frames later
  and no picture of the captures differs. A pack profile v41 would add them.
- **The HDMA tables** are checked only through the pictures (every one equal), not byte for byte:
  the runner does not print `$0C70-$0C95`.
- **`$00CB`**, the high byte of the result screens' 16-bit stores of `$00CA`, and `$0000`-`$0078`
  as TRACK RECORDS leaves them (native keeps its own fields), are not compared.
- **A saved `$77:10A7` of 14 or more**, which the menus do not write (HUNTER's ending uses the
  byte as its drop), starts the markers from 0; native's own save still writes the byte as the
  loaded image or the cold start left it.
- The name check and the long names' lag (NAME-CHECK), and every other item of R-0097's Not
  covered but the three above, are unchanged.

## Reproduction

From a worktree of the candidate with `local/` linked, under `local/locks/heavy-run.sh`:

```
E=local/evidence/record-screens
python3 $E/build_manifest.py NAME FRAMES DESCRIPTION SPEC...   # image:PATH for a warm boot
python3 $E/make_ca.py NAME PRESS DESCRIPTION [--vs PAD DOWNS] [--after F] SPEC...  # the rs-ca-* runs
zsh $E/probe_captures.sh NAME:FIRST ...                         # the original's runs
python3 $E/schedule.py NAME --manifest $E/NAME.json
zsh $E/cue_captures.sh NAME                                      # two sound captures, the cue list
zsh $E/checks.sh OUTDIR                                          # every comparison, candidate and main
```

`pcs.py MANIFEST FROM TO BB:AAAA=label... [--image IMAGE]` lists the frames and scanlines a
routine runs on; `peek.py`, `pixdiff.py`, `textrows.py` and `summary.py` show one capture's words,
pictures and text beside native's (`CAPTURE_DIR` reads another task's captures).
