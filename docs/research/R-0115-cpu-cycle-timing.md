# R-0115 - The race and menu effects that depend on CPU cycles

Status: research result and native change ([CPU-CYCLE-TIMING](../../tasks/CPU-CYCLE-TIMING.md)),
11 November 2026, on main `a804372d`. PAL ROM SHA-256
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, reference core bsnes `7d5aa1e6`
(patched, Strict). Evidence in main `local/evidence/cpu-cycle-timing/`. It answers R-0109's queue
item 6: the differences the records put down to the CPU's cycle count rather than its instructions,
the multiply helper `$81:B67C` hit by an HDMA write (R-0095, R-0098, R-0102) and the prints that run
past their frame or sit on a knife edge (R-0095, R-0100, R-0102). One native change: TRACK RECORDS'
grid halves now stop where the original's print stands when its frame ends (15 of 16 traced prints
equal). The multiply cannot be placed by any cost model of native's state; the other prints are
measured and queued.

Tags: **[L]** read from the static map's listing (main `local/evidence/menu-input/listing/`) or the
ROM's bytes; **[C]** a run of the original made for this task (an instruction trace with each
instruction's scanline and dot, or work RAM per frame); **[R]** stated by an accepted record.

## Domain

- **Instruction traces** ([C]): `trace_frames.py` replays a manifest in the reference core (a warm boot
  writes its image before frame 0) and records every instruction of the chosen frames with its clock:
  master clocks from vblank's start (line 225; 1,364 a line, 312 lines, 425,568 a frame), and the work
  RAM at each frame's end, the sample every comparison reads. `print_cut.py` finds the text printer's
  stores (`STA abs,X` in `$80:C3BC-C6FF`) on each side of a frame's end.
- **The grid prints** ([C]): `grid_prints.py` traces all 16 TRACK RECORDS grid halves in the existing
  captures that enter TRACK RECORDS (RECORD-SCREENS' `rs-track`, `rs-high`, its review's `wr-track`,
  MENU-INPUT review's `rv-records`; four each, two entries of two halves): each write of the print with
  its clock, each record dot's path, the half's start and whether the palette cycle stepped in that
  frame's NMI (`grid-prints.json`). `grid_model.py` is native's model in Python, with `--fit` the step
  costs from the traces.
- **The multiply** ([C]): LEAGUE-TOURS' `hv-odd-30250.txt` and `hv-five-121489.txt` (R-0098) and
  TWO-HUMAN-STUNTS' `downer/trace-3558.json` (R-0095) for the three known hits; `multiply_hits.py`
  checks every call of `$81:B67C` over a window, the product read back against the clean product, and
  was run over the whole track-41 race of `odd-tours` (frames 27,300-31,300).
- **The other prints** ([C]): the frames RECORD-SCREENS' `compare.py` still finds different
  (`rs-track`, `rs-high`, `rv-records`, `wr-stats`, `rs-ca-vs-lap`, `rs-ca-2p-lap`, `rs-ca-vs-stunt`),
  each traced over its frame and the next.

## Findings

1. **What decides the work RAM at a frame's end.** [C] A print that runs past its frame leaves the
   text buffer (`$0200-$09FF`) as far as its last store before vblank's start, where the frame's
   sample is taken; the NMI runs after it. The printer writes a letter as two words, its top and
   bottom (`$80:C58F`, `$80:C596`, about 80 clocks apart), and letters come at least 1,059 clocks
   apart, so the cut can be placed by a cost model whose error at the frame's end is under the gap
   around it: in the 16 grid prints the original's last store is 124 to 1,842 clocks before the end
   and the next 2,488 to 5,382 after it.
2. **Where a menu frame's work starts.** [C] The menus' frame wait (`$80:FADF-FAEF`) polls the sound
   queue (`JSL $82:8035`) and reads `$4212` in a loop of about 640 clocks. Vblank comes somewhere in
   that loop, and when it comes inside the poll the rest of the poll runs after the NMI: the same
   print starts 25,856 clocks into its frame in one entry and 26,358 in another (`rs-track` 2113 and
   1163; the instruction streams differ only by the poll). Over the 16 grid prints the start spreads
   over 502 clocks (first halves) and 560 (second halves) without a palette step. The loop's phase at
   vblank is the previous frame's exact clock modulo the loop, so no model of one frame's work can know
   it: every cost model that starts after a menu frame wait carries this spread, R-0102's league way
   outs included (part of their -528 to +510).
3. **TRACK RECORDS' grid halves** (`$80:ED1D-EEDE`). [L, C]
   - Entry frames 63 and 86 print the grid's first half (tours 0-4) and second (5-8) with no frame
     wait after (R-0100). Each tour: `$83:8D8F` draws its picture, five rows of five words
     (`$83:8DE5-8DF5`); `$80:EDAC-EDB1` adds 5 to `$00CE` (set to 0 or 25 at `$80:ED34-ED44`); then
     for each rank the medal's name through the printer (`$80:EEDF`'s strings) and five record dots
     (`$80:EEF2`: two printer calls, the left column and then the flipped right). A holder below 16
     takes `$80:EEFA-EF13`'s path (335 clocks more). Every tour's picture calls the level check
     (`$83:8DA4 JSL $83:9F14`: `$83:8DA0`'s `CMP #$08; BEQ` branches to the next instruction, so both
     paths reach it), part of each tour's head; the second half tests the tour at `$80:ED70-ED7C`, and
     for tour 8 (`$95` = 9) calls it once more at `$80:ED7E` (2,140 clocks more; review round 1's trace
     of `rs-track` 1186: `$80:ED7E` on tour 8 only).
   - A half is 52 writes a tour (5 picture rows, `$00CE`, 16 letters, 30 dot columns): 260 and 208.
     About 124,000 clocks a tour; when its frame ends the original has done 170 or 171 of the first
     half's writes (into tour 3) and 176 of the second's (into tour 8). The rest are done early in the
     next frame (frame 64 or 87, which has no frame wait before it).
   - The cost model (`grid_half_steps_in_frame`): the half's start (26,110 or 9,164 clocks, the mean of
     the traced starts, plus R-0102's 1,100 when that frame's NMI stepped the palette cycle), then the
     CPU clocks before each write (each the mean of its kind over the traces with the refresh and HDMA
     taken out), turned into elapsed clocks line by line: 40 for the refresh at dot 538 of every line,
     and 24 for the grid's two HDMA channels at dot 1,104 of the visible lines (fitted: with one rate
     for all lines the error swung by 1,400 clocks between the visible lines and vblank). The writes
     before the frame's end are done in frame 63 or 86.
   - Fitted on `rs-track`'s four prints, it places 15 of the 16 (`grid_model.py`): its start could move
     by 1,000 to 3,070 clocks either way without changing them, against the start's spread of about
     500. The sixteenth, `rs-high` 1463, is a knife edge: its frame's NMI stepped the palette cycle,
     and the original's last letter lands 124 clocks before the frame's end; the model's start is 50
     clocks later than the traced one (the step taken as R-0102's 1,100) and it places that letter
     after the end. Within the poll's spread of finding 2 it cannot be placed either way.
4. **The multiply hit by HDMA** (`$81:B67C`). [L, C]
   - `$81:B680-B6A0` writes M7A (`$211B`) low then high (`$81:B682`, `$81:B686`, with `XBA` between),
     M7B (`$211C`) at `$81:B689` and reads the product from `$2134-2136`. M7A and BG1HOFS/BG1VOFS
     share the PPU's mode-7 latch: an HDMA write to one of them between the two stores leaves its own
     byte in the latch, and M7A becomes the HDMA's high byte under the written high byte.
   - The three known hits: `odd-tours` 30,250 (track 41, ANTI-UNI's landing, race frame 2,809;
     `$81:9585`'s product, R-0098): the stores start at dots 1,058 and 1,100 of line 42, the race's
     channel 4 (BG1VOFS, a line each) writes at dot 1,104 between, and `$01C4 x $C6` gives `$FFC552`
     instead of `$FF9998`; 177,014 clocks and 6,705 instructions into the frame. `five` 121,489 (race
     frame 2,867): line 68, dots 1,070 and 1,112, 212,490 clocks and 8,325 instructions in. DOWNER in
     two views (`vs-downer` 3558, R-0095): line 112, the split line, first store at dot 1,086 and the
     second at 1,284 (the split's own HDMA between them), M7A `$021C` for `$0202`, 272,522 clocks and
     10,544 instructions in.
   - The whole track-41 race (`odd-tours` 27,300-31,300, `multiply-odd-tours.json`): 76 calls in 18
     frames, the four landing products (`$81:9585`, `$81:95A2`, `$81:95C3`, `$81:95E0`) of 19
     landings, all on visible lines 33-89; one hit (30,250). One other product's stores also span dot
     1,104 (29,597: dots 1,094 and 1,190), but its M7B is 0, so its product is 0 whatever M7A became
     and the scan cannot say whether its timing missed the HDMA (review round 1). The window's evidence
     is the three hits: a product is hit when its first store starts in a window of about 30 clocks of
     the line's 1,364 (1,058 to 1,086 in the hits seen), on a line where a latch-sharing HDMA write
     comes.
   - The race's frame starts from a two-instruction wait (`$82:D4A0-D4A2`, about 38 clocks a pass;
     3,802 passes in `odd-tours` 30,249): the NMI comes at whatever point of it vblank finds, which is
     the previous frame's exact clock modulo the loop.
   - So placing a hit needs the update's clock to within about 15 clocks after 150,000 to 270,000
     clocks of work (the NMI's DMA, every routine of both riders' updates before the landing, the
     HDMA and refresh of every line), and the frame's start phase, which needs the previous frame's
     clock as exactly: a cycle-exact count of every frame from the race's start. That is a model of
     the 65816 in all but name, which the project does not build (D-0003, D-0008), and a counted-work
     model is several orders too coarse. No native change. Native takes the clean product, which is
     the original's in 75 of the 76 products of the track-41 race.
5. **HIGH SCORES' leaders** (`$80:D7CD-D905`). [C] The choice frame computes the leaders (`$83:9D80`
   percentages, `$80:9719` divisions) and prints the screen's stream, which runs into the next frame:
   in `rs-track` 2350 `$80:D7CD` starts 10,958 clocks into the frame, the leaders take about 156,500
   (two riders with races) and the print the rest, its frame ending after 387 letter stores. Six
   traced: the original's last store 472 to 2,750 clocks before the frame's end and the next 1,406 to
   3,910 after (`rs-track` 2350, 2650; `rs-high` 1050, 2500; `rv-records` 1700; `wr-stats` 1050). Text
   and OAM differ on those frames (the printed objects of the second half of the stream); no picture.
   Native prints it whole. Not modelled: it needs the leaders' loops (R-0102's percentage costs cover
   part) and the generic printer's costs for every code of the stream (letters, numbers, names,
   object places), with a margin of about 500 clocks against finding 2's spread. Queued.
6. **VS CHAMPIONS' first frame** (`$80:C786`, the header). [C, R] Native already gives its frames
   (R-0095's placings rule); what differs is the text and OAM at the first frame's end, where the
   original's header print stands: `rs-ca-vs-stunt` 5343 (56 letter stores done; the last 956 clocks
   before the end, the next 1,138 after) and `rs-ca-vs-lap` 9302 (the last 124 before, a knife edge).
   The start of the header comes after `$80:C786`'s records and one checksum run (`$83:90F4`) per
   placing, which R-0102 costs to within about 500 clocks; with the header's own print costs it might
   place 5343 but not 9302. Not modelled; queued.
7. **The lap result's first frame is not a cycle case.** [C] In result frame 0 (`rs-ca-2p-lap` 8412,
   `rs-ca-vs-lap` 8322) the original only waits on the sound queue (`$82:80xx`, from `$80:FADF`); the
   text clear (`$83:8B51`) comes in frame 1, 46,578 clocks in. Until then `$0200-$09FF` holds the
   race's work RAM (7 of its 1,024 words are the cleared tile), which native does not keep there.
   It is the shared buffer's content, not a print's timing (RECORD-SCREENS' `compare.py` says as
   much); it leaves no picture.

## The listed cases

| Case | Records | Verdict |
| --- | --- | --- |
| DOWNER in two views, rider 1's landing | R-0095 | cannot: the hit needs the update's clock within about 15 clocks (finding 4); no change |
| Track 41's landing against ANTI-UNI (`odd-tours` 30,250, `five` 121,431 and 121,489) | R-0098, R-0102 | cannot, as above; 1 of the race's 76 products is hit |
| TRACK RECORDS' grid halves past their frames (`$00CE` and text) | R-0100 | **native changed**: 15 of 16 traced prints equal at the frame's end, every capture's other frames as before; `rs-high` 1463 a knife edge |
| HIGH SCORES' leaders past their frame (text, OAM) | R-0100 | measured (six prints, margins about 500 clocks), not modelled; queued |
| VS CHAMPIONS' header frame (text, OAM) | R-0095, R-0100 | measured (two prints, one a knife edge), not modelled; queued |
| The lap results' first frame | R-0095, R-0100 | not a cycle case: race RAM in the text buffer (finding 7) |
| W2b frame 15672, the rename keyboard (256 px) | R-0095 | not judged: on main's runner and the candidate alike every menu field of the review capture `w2b-vs-jumps` differs from 13,024 on (the second race returns at 12,923 in native; the comparison notes its loading), so the frames after it are a lag, not one print; queued |
| The three-member league's knife edge | R-0102 | unchanged: R-0102's model and band; finding 2 explains part of its error (the poll's spread) |

## Native change

- `src/core/menu_overruns.cpp`, `.hpp`: `grid_half_steps_in_frame`, the writes of a grid half done
  when its frame ends (finding 3), with its constants and the line-by-line refresh and HDMA. It reuses
  R-0102's palette step (`after_first_nmi`).
- `src/core/track_records.cpp`: `print_grid_half` prints its writes in the original's order and stops
  after a number of them; entry frames 63 and 86 print the modelled part, frames 64 and 87 the whole
  half (the same words again, then the rest). `record_dot` writes a column at a time; `$00CE` follows
  each tour. `src/core/tour_menu.cpp`: `draw_tour_picture` can stop after its first rows.
- Every frame from 64 (87) on is as before, so no picture moves and nothing after the entry changes.
  The race runners (`lab_runner`, `movement_runner`, `split_demo_runner`, `zoom_zoo_runner`,
  `classic_race_presentation_runner`, `presentation_runner`) are byte-identical to main's build;
  `front_end_runner`'s races do not pass through TRACK RECORDS.

## Results

`checks.list` (`check.sh`), the candidate against the original, and with main's runner
(`main-runner/`, `reverted.out`):

| Check | Candidate | Main's runner |
| --- | --- | --- |
| `grid-rs-track` (2,781 frames, 2,200 pictures) | differing only HIGH SCORES 2350, 2650 (text, OAM) | also `$00CE` 1163, 2113 and text 1163, 1186, 2113, 2136 |
| `grid-rs-high` (2,881, 2,300) | HIGH SCORES 1050, 2500; text 1463 | also `$00CE` 1463, 2263, text 1486, 2263, 2286 |
| `grid-wr-track` (1,731, 1,150) | none | `$00CE` 1163, 1663, text 1163, 1186, 1663, 1686 |
| `grid-rv-records` (2,081, 2,100) | HIGH SCORES 1700 | also `$00CE` 1163, 2013, text 1163, 1186, 2013, 2036 |
| `grid-model` (16 traced prints) | 15 of 16 placed | the same (the runner is not used) |

Every picture equal on both. Unit tests (`front_end_tests`): the model's counts for no icons, all
icons, a palette step, `rs-track` 1163's eight icons, and a second half with every icon, nine icons
and nine with a step.

## Limits

- **The start's spread** (finding 2) bounds every cost model that starts after a menu frame wait to
  about 500 clocks; a print whose cut lands closer to the frame's end than that (`rs-high` 1463,
  `rs-ca-vs-lap` 9302) cannot be placed from native's state.
- The grid model's step costs are means over four prints (checked on sixteen); the HDMA's 24 clocks a
  visible line are fitted, not counted. The validation set is narrow: the 16 prints cover four record
  layouts, the first halves with 0, 8 or 9 icons (all in tour 0) and the second halves with none. The
  palette step's 1,100 is R-0102's fitted value, and the one traced print here with a step is the
  miss (`rs-high` 1463, its traced start 27,160 against the model's 27,210): consistent with it, not
  confirmed by it.
- Review round 1 traced three record layouts the model had not seen (holder and time writes added to
  `wr-track`: every record with an icon, icons only in tours 5-8, icons in tours 1-3 ranks 0-1; main
  `local/evidence/cpu-cycle-timing/review-1/`): the model placed 7 of 8, the second halves with icons
  (170 writes) among them. The miss, `wx-dense` 1163, did 165 writes in the original and 166 in the
  model; the same inputs at 1663 did 166 in the original. Only the start differs (+248 and -254
  against the model's mean) and the model's slack there is 20 clocks: finding 2's spread. With dense
  records the first half's cut is where that spread decides it, and native cannot be right for both
  entries. Other records change the dots' paths only; other levels change tour
  8's picture (the level check is the same call). A grid half's letter is written whole: a frame end
  between a letter's two stores (about 80 clocks) would leave its bottom unwritten in the original.
- The multiply's verdict rests on three hits and one scanned race. Other tracks' HDMA tables (which
  lines write a latch-sharing register) were not surveyed; the race sweeps' equal rows say hits are
  rare, not absent.
- HIGH SCORES, VS CHAMPIONS' header and W2b are measured or listed, not modelled
  ([CPU-CYCLE-TIMING-REMAINDER](../../tasks/CPU-CYCLE-TIMING-REMAINDER.md)).

## Reproduction

From a worktree with `local/` linked, under `local/locks/heavy-run.sh`:

```
E=local/evidence/cpu-cycle-timing
python3 $E/grid_prints.py OUT.json local/evidence/record-screens/rs-track.json:1163,1186,2113,2136 ...  # the 16 prints
python3 $E/grid_model.py $E/grid-prints.json [--fit]     # the model against them; the step costs
python3 $E/trace_frames.py MANIFEST FIRST LAST OUT       # any frames' instructions and work RAM
python3 $E/print_cut.py OUT.trace FIRST FRAME            # the printer's stores around FRAME's end
python3 $E/multiply_hits.py local/evidence/league-tours/odd-tours.json 27300 31300 OUT.json
zsh $E/run_checks.sh OUT_DIR [RUNNER]                    # checks.list
```
