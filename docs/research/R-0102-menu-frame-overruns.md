# R-0102 - The league's and RECORDS' frame overruns, and the podium's way back

Status: research result and native change ([PODIUM-RETURN-TIMING](../../tasks/PODIUM-RETURN-TIMING.md)),
27 October 2026, on main `6155f3d2`. PAL ROM SHA-256
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, reference core bsnes
`7d5aa1e6` (Strict). Evidence in main `local/evidence/podium-return-timing/`. It takes up three
frame effects R-0098 and R-0100 left as Not covered, with R-0101's method: a routine's master
clocks against the frame's end.

Tags: **[L]** read from the static map's listing (main `local/evidence/menu-input/listing/`, with
bytes the listing leaves as data decoded by hand where the trace runs them); **[C]** confirmed in a
capture.

## Domain

No new captures of the menus: LEAGUE-TOURS' `tours-a` (nine windows), `tours-b` (two), `odd-tours`
(three members, two windows), `t-crawler`, `podium-back`, `backs`, `picks-l0/1/2`, the review's
`five` (five members, two windows) and `four` (one), and RECORD-SCREENS' `wr-stats`, `wr-stats2`,
`rs-high`, `rs-track` and three `rs-ca-*` results. Each is replayed in process in the reference
core with its instruction ring:

- `trace.py` records, frame by frame, the clock (from vblank's start, line 225; 1,364 master clocks
  a line, 312 lines a frame, 425,568 a frame) of named instructions: the frame wait (`$80:FADF`,
  and `$80:FAF1` after it), `$80:C786`, the count's call (`$80:BF32`), the standings' print
  (`$80:BE57`, `$80:A547`) and the checksum runs (`$83:90F4`). `trace_pairs.sh` traces all 27 way
  outs with a standings print after them (`odd-tours` 9, `four` 4, `five` 14).
- `cycles_records.py` costs `$83:97BD` step by step (each step at the least it took, as R-0101's
  `cycles.py`) at the seven RECORDS entries.
- `podium_upload.py` presses Start on 12 consecutive frames of one podium (organic-cycle-five,
  after LEAGUE-TOURS' review's `podium_sweep.py`) and times the title's sound upload.

`steps.py` measures what the menus show: the arrow's spin `$00C6` counts down once at every frame
wait (`$80:FAF5`), so the lag between the original's spin and native's changes exactly where one
side's work ran past a frame and the other's did not.

## Findings

1. **What a late frame is.** [L, C] The menus' frame wait (`$80:FADF-FAEF`) polls the sound queue,
   then loops while `$4212` shows vblank and then until it shows vblank again. Work that ends
   after the frame's end (425,568 clocks after vblank's start) starts its wait in the next frame:
   that frame has only its NMI (the palette cycle's hook), no arrow step and no menu pass, and
   everything after is a frame later. R-0101's keyboard check is one case; these are three more.

2. **A league pair's way out** (`$80:BF2C-BF35`, then `$80:BE55`). [L, C: 27 way outs]
   - After `$80:F4B8`'s wait, exit frame 1 runs `$80:C786` (the records), `$80:AF54` (the pair's
     count; with another pair to race it ends with a checksum run, `$80:AFEB`, while the awards'
     print follows the count 1,316 clocks later in `odd-tours` 19473) and, with another pair
     to race, `$80:A547` (the standings' print), with no frame wait between them.
   - Measured parts: `$80:C786` is called 24,918-26,660 clocks after vblank; it takes 227,208-227,248
     clocks with no time placed, 452,514-457,036 with one and 676,678-682,184 with two (one
     checksum run `$83:90F4`, about 224,800 clocks, plus one per placing; a lap race's best laps,
     `$80:C868`, add about 4,500 over a one-run race's totals). The count with its checksum takes
     225,654-227,898. The standings' print to the frame wait takes 345,904-351,868 with three
     rows, 388,258-391,474 with four and 429,610-436,246 with five.
   - The print's spread follows its numbers: `$83:8BE7` turns each played and points word into
     five digits by subtracting each power of ten once per unit (`$83:8BFE-8C3E`), so each unit
     of a digit costs about 89 clocks (`pairs_fit.py`: one slope and a constant per league, the
     residuals within 1,706 clocks).
   - The model (`menu_overruns.cpp`, `model.py`): the count's frame is 1 + (25,200 + 227,224 +
     224,765 per placing + 4,480 for a lap race) / 425,568 and the standings' wait frame adds
     226,738 + 346,381 + 41,214 per row over three + 89 per digit unit. It puts all 27 counts and
     all 27 standings' waits in their measured exit frames; its error is -2,007 to +1,732 clocks
     and the nearest boundary 1,453 clocks away.
   - What that gives: in three-member leagues a way out with two placings is late when the
     standings' digits are many (`odd-tours`: the first event's standings, all points 0, end
     1,990 clocks inside exit frame 3; every later one runs into frame 4); with no placing they fit
     in frame 2. In four- and five-member leagues the standings are always a frame later than
     native's old rule (frame 4 after placings, 3 without). One placing counts the pair in frame
     2, not 1 (R-0098's `five` 15,619, 32,500, 60,423 and `odd-tours` 35,575).
   - The awards' way out (the last pair) keeps R-0095's frames (2, or 3 after two placings): the
     awards' print (`$80:8917`) took 243,266 clocks in `odd-tours` 19473, which gives the same
     frames with at least 70,000 clocks to spare.
   - Native printed the standings in the frame the wait comes; the original's print starts in the
     frame before when it runs late, so the text map differs on that one frame (no picture: the
     slide uploads it later).

3. **The standings' print elsewhere.** [C: `five` 6550, 20491, 39036] From PICK TOUR, the slots,
   NOW PLAYING's Y and the awards' way out, `$80:BE57` comes 15,484-25,010 clocks into its frame;
   five rows (429,510-432,052 clocks) always run past the frame, four (about 390,000) and three
   do not. Each such five-member standings was a frame later than native's (`five` 6551, 20492,
   39037, 51636, 65248, 76378, 103049).

4. **RECORDS' entry** (`$80:D525-D53C`). [L, C: 7 entries]
   - OPTIONS' choice calls `$83:97BD` (`$80:D527`) 8,994-9,596 clocks after vblank, then prints
     RECORDS' choices; from the return to the frame wait is 229,474 clocks in every entry (1,060
     more when the NMI falls inside).
   - `$83:97BD` clears nine maxima (`$013F-014F`), then for each rider (`$77:0230 + 8 x rider`:
     races, wins and two more words) keeps the largest races, losses (races less wins), wins and
     the two words, then three percentages of the races through `$83:986A` (wins, losses and the
     third word; `$83:9878` skips `$83:9D80` without races, leaving 0), compared against maxima
     that `$83:984F` sets back to 100 after each rider. `$83:9D80` halves both until the part is
     below 655 (`$83:9D85-9DA0`), multiplies by 100 (`$83:956D`) and divides bit by bit
     (`$80:971D`, 16 steps; `$80:973B-973F` when a quotient bit is set).
   - Its CPU time is `32,362 + 34 x stores + 7,704 x percentages + 330 x halvings + 112 x
     quotient bits` master clocks (the per-call part exact on all 39 calls of `wr-stats2`; the
     totals within 40 clocks, one refresh, on all seven entries); elapsed time adds the refresh
     (`x 1364 / 1324`).
   - `wr-stats2`'s statistics (39 percentages) end the wait 593,870 clocks after the choice frame's
     vblank: one frame late; the other six (0 or 6 percentages) end by 321,678. Late means about
     19 or more percentages of riders with races. The model can never give two frames.
   - The print comes after `$83:97BD`, so in a late entry RECORDS' text is printed mostly in the
     late frame; native prints it there whole.

5. **The podium's way back is the sound processor's.** [C: `podium-upload-cf5.json`, 12 presses]
   - The way back calls the title's upload (`$82:82A5`, from `$80:A115`) 21 frames after the
     press (the exit's frames are counted from the press, as native's are). The upload itself takes 23,235,246-23,236,014 clocks (54.6 frames) in every press;
     what moves is its start: 123,796-194,290 clocks into that frame, after the handshake with
     the running driver (`$82:8286-8292`: write port 0, wait until the sound processor echoes
     it).
   - When it starts after about 170,400 clocks the upload ends past frame 75's end and
     `$83:987D` restores the menus on exit frame 104 instead of 103: 4 of the 12 presses (starts
     170,398-194,290 late; 123,796-158,358 on time), in the same clumps as the review's 40-press
     sweep (15 late).
   - The start is the driver's reply to the request, which depends on where the driver is in its
     loop when the request comes: the podium's music, its tick, and everything the driver did
     since the podium's own upload. The front end has none of that, and native's driver is not
     in the original's state there (R-0099: the music start a pass apart in 2 of 8 podiums). Not
     modelled; native keeps R-0095's phase rule, on which the first tour's frozen gate relies.

### Native

- `src/core/menu_overruns.cpp`: the two cost models (`records_statistics_frames_late`,
  `league_exit_frames`) and the standings' print alone (`standings_print_frames_late`). They
  count what the routines run on (the statistics' percentages, halvings and quotient bits; the
  placings, the race mode, the rows and the digits) and turn clocks into frames with the
  measured constants. They are cost models of these routines only, not a CPU model.
- RECORDS' entry (`options_menu.cpp`) waits the late frame with no frame wait and prints in it.
- The way out (`race_result.cpp`, `league_menu.cpp` `time_league_exit`): on exit frame 1, after
  the records, the count's frame and the standings' frame come from the model; the awards keep
  R-0095's frames. `enter_league_table` gives a five-row print its late frame.

## Results

The candidate against the original; main `6155f3d2`; both on pack v41 (`checks.list`, run once):

| Capture | Pictures equal: candidate / main | Arrow's lag changes outside the limits: candidate / main | League races, league fields |
|---|---|---|---|
| `odd-tours` w0, w1 (3 members) | 5,678 / 4,912 of 6,459; 6,010 / 4,934 of 7,003 | 0 / 4; 0 / 4 | all equal; fields 0 differing (main 1 in w0) |
| `four` w0 (4) | 5,153 / 4,352 of 5,160 | 0 / 4 | all equal; 0 |
| `five` w0, w1 (5) | 7,334 / 5,465 of 12,377; 3,022 / 2,950 of 9,600 | 0 / 24; 0 / 883 (w1 to 127,125) | all equal; w0 0 (main 2), w1 7,200 both (another race result from 127,125, R-0098) |
| `tours-a` w0-w8, `tours-b` w0-w1 (2 members) | equal to main in every window (e.g. w0 3,096 of 3,206) | 0 / 2 per podium (all at the podium's way back) | all equal; 0 |
| `t-crawler`, `podium-back`, `backs`, `picks-l0/1/2` | (lag only) | 0 / 0 | - |
| `wr-stats2` (RECORDS, dense) | 800 / 517 of 800 | 0 / 1 (799 frames off) | - |
| `wr-stats` (RECORDS, none) | 800 / 800 of 800 | 0 / 0 | - |

The remaining differences: the podium's way back (finding 5) in every two-member window and after
each podium; a race's return two frames apart in `five` 19,780, as on main (after the race R-0098 lists as
loading a frame off at 16,596; R-0077), which leaves `five` w0's later menus two frames apart; HUNTER's two-view pictures (SPLIT-HUNTER-PICTURES) and the odd league's
computer result objects (R-0098); one text-map frame per late print. `model.py`: 27 of 27 way outs
and 7 of 7 RECORDS entries in their measured frames. LEAGUE's frozen gate (first tour, pack v40):
passed.

## Not covered

- **The podium's way back** (finding 5): about a third of podiums restore the menus a frame late
  in the original, by the sound driver's reply; after them the arrow's spin and the palette cycle
  stay a frame off to the league's end (each podium of `tours-a` and `tours-b` but the late
  ones). Predicting it needs the sound processor's position in its loop when the request comes,
  which only an exact sound side from boot could give.
- **The model's reach.** Its constants come from three leagues of the default riders 0, 2, 4, 6
  and 8: other names change the print's cost by an unmeasured amount per letter, and a league's
  name (the standings' title) too. The model's error over the 27 way outs is up to 2,007 clocks,
  so a three-member way out whose standings' wait falls within about 2,000 clocks of exit frame
  3's end may come out a frame off; `odd-tours`' first event is 1,990 clocks inside it. The
  `$80:C786` start varies by 1,742 clocks with the NMI's work and is taken as 25,200. Six to eight
  members, and stunt events that place a score, are extrapolated.
- **One text-map frame** where the original's print starts in the frame before its late wait
  (the standings after a late way out; RECORDS' entry, frame 900 of `wr-stats2`). No picture.
- The awards' `$83:A923`-less way out still polls the sound queue in the frame before the awards
  as native did (the original polls only in the wait), unchanged and unmeasured here.
- `five` window 1 from 127,125 (R-0098's landing multiply: another race result) and the races
  that load a frame off (R-0077) stay as they were.

## Reproduction

From a worktree of the candidate with `local/` linked, under `local/locks/heavy-run.sh`:

```
E=local/evidence/podium-return-timing
zsh $E/trace_pairs.sh                      # trace-pairs-*.json: the 27 way outs
python3 $E/pairs_summary.py $E/trace-pairs-*.json
python3 $E/table_digits.py league-tours odd-tours-w0 $E/trace-pairs-odd-tours.json   # and each window
python3 $E/pairs_fit.py                    # the print's slope; pairs-data.json
python3 $E/cycles_records.py local/evidence/record-screens-review/wr-stats2.json 900 OUT.json
python3 $E/podium_upload.py local/evidence/league/organic-cycle-five.json 12100 12200 12 OUT.json
python3 $E/model.py                        # both models against every measurement
zsh $E/run_checks.sh OUTDIR                # checks.list
```

`steps.py DIR CAPTURE` lists where the arrow's lag changes; `find_calls.py MANIFEST ADDRESS` finds
the frames a routine runs in.
