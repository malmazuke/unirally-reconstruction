# R-0114 - The waits that follow the sound processor

Status: research result and native change ([SOUND-PROCESSOR-TIMING](../../tasks/SOUND-PROCESSOR-TIMING.md)),
10 November 2026, on main `e036458e`. PAL ROM SHA-256
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, reference core bsnes `7d5aa1e6`
(patched, Strict). Evidence in main `local/evidence/sound-processor-timing/`. It answers R-0109's queue
item 5: the front end's waits that the records attribute to the sound processor (R-0077, R-0087,
R-0088, R-0090, R-0096, R-0098, R-0099, R-0102), taken from the native driver's state where that is
better than the measured lengths. One native change, and it is not a sound wait: the menu's fade-out
before the idle demo's title (`$80:93FB`, `$80:9885-98A3`), which two of the listed cases were.

Tags: **[L]** read from the listing or a disassembly of the ROM; **[C]** a run of the original made for
this task (bsnes, a PC trace or work RAM per frame); **[M]** measured from the existing audio captures'
CPU port writes against an anchored native run; **[R]** stated by an accepted record.

## Domain

- **Sessions** ([M]): every sound session of 21 existing audio captures of the original (`sweep.list`:
  DEMO-AUDIO's five schedules and its review's five, MODE-AUDIO's five, PODIUM-AUDIO's six; the
  kind-5 CPU port writes of their `-full` or `-disp1` captures), 190 sessions once the schedules that
  share a prefix count each session once. Native: the candidate's `front_end_runner` writes the
  schedule's cue log (with the laboratory replays the task recorded, so its frames follow the
  original's), and `race_audio_runner` plays it through the native IPL, driver and DSP at the D-0010
  anchors with events. `sessions.py` aligns the two port-write streams by the sessions' FF writes and
  compares, per session, the request (the first FF to port 0, `$82:808F`), the reply (the first CC
  header write, `$82:80C9`, once the driver has stopped and the IPL is ready, `$82:8097`), the length
  to the sample upload's closing write (`$82:8321`), and the frames from the request's frame to the
  end's. That frame count is what the front end's waits follow, and it does not depend on where the
  schedule's frames are.
- **Pictures** ([C]): `window_compare.py` runs the original over a window and compares every picture
  with native's; the idle demo lap's retained pictures and race words (R-0087, R-0088); SAVE-FILES'
  warm power-ons (three saved images, seven scripts, every picture).

## Findings

1. **What the CPU waits on at a session.** `$82:807E` writes FF to port 0 and waits for the IPL's
   `$BBAA` (`$82:8097`). A running driver answers FF at its next port poll (SPC `$0572-0596`) with its
   stop sequence (SPC `$0487-04B9`, native `TitleMenuAudioDriver::stop_for_ipl`): it lowers both
   master volumes by 8 on each pulse of timer 2 (target 133 of 64 kHz, about 44,200 master clocks) and
   returns to the IPL at 0. So the reply takes the poll's latency plus one timer-2 pulse for every 8 of
   master volume, on the timer-2 grid that started with the driver. The upload that follows runs at
   the CPU's pace (the IPL echoes each byte), then the samples go through the driver's own loader. [L, M]
2. **Native's driver is exact when its inputs are.** Before any race's or demo race's commands (the
   boot's session, the first demo title's, the first race's from the menus), native's reply is within
   102 master clocks of the original's, absolute, in all 8 such sessions of four captures (`chain`);
   the boot sessions are equal to the clock. A native request up to 15,872 clocks early still gets
   the original's reply, because the driver answers on its own grid. [M]
3. **After the first race it is not.** Over the 189 later sessions native's reply-wait error (from each
   side's own request) is -94,980 to +72,880 master clocks; its absolute error (request error plus
   reply-wait error, as `chain` and `carry.py` count it) is -96,272 to +233,772, the largest where a
   race's request is 172,018 clocks off its anchor (`pa-odd-tours` session 9, race-66). In 38 of 46 demo-title sessions and
   25 of 42 race sessions it equals, within 1,000 clocks, the previous session's end error: the driver
   carries its start's error on its timer-2 grid. After a race it does not (6 of 93 title sessions):
   the way back's FF arrives while the race's fade is lowering the master volume, and the fade's
   commands reach the driver at D-0010 anchors (`$83:E82C` and the other race dispatch sites run where
   the race's CPU work puts them), so native's volume at the request, and the number of pulses, differ.
   Each later session inherits that error through its own timer-2 grid (`carry.py`). [M]
4. **The idle demo's title session runs with interrupts.** During it the title interrupt runs every
   frame (`$00:8587` wrapper, `$80:F622` scroll, `$80:FA60` palette cycle, `$80:B0F0` restore; 34
   instructions, 76 when the palette steps; trace of `demo-cold` frames 1000-1001). The cued audio
   model turns NMIs off after a menu exit (`cue_frame`), so its demo session is 78,108 to 92,202 clocks
   shorter than the original's (R-0096's commands 82,000-97,000 clocks early). The front end's frames
   do not use that length, so this changes no picture; it is a follow-up for the audio. [C, M]
5. **The two long title starts are the menu's fade-out.** When the menu's idle count `$89` runs out
   (480 frames, one a frame), `$80:AC2E-AC36` sets `$9B` to 5 and the main loop's dispatch (`$80:889F`,
   table `$80:88B2`) enters `$80:93FB`, which calls the fade (`$80:93FD JSR $9885`; `$80:9881` is the
   long entry the credits picture uses, `$83:FB3B`). `$80:9885` sets `$00` to 15 and `$80:988C-989C`
   loops seven times: a frame wait (`$80:FADF`, which turns the arrow), the object buffer's DMA
   (`$80:9318`), brightness lowered by 2 and written to `$2100`; after the seventh (1) `$80:989E` forces
   blank at once. So the menu shows at 13, 11, 9, 7, 5 and 3 on the title's frames 1-6, frame 7 is
   blank, and after `$80:D20E`'s frame wait the title's work starts on frame 9 (finding 6). Native
   blanked from frame 1: six pictures differed (57,057 pixels each) at **every** title start, not only
   cycles 10 and 17 (R-0088's two were the ones among the retained pictures) and the warm power-on's
   first (R-0090's "blanking 6 frames later"). The idle count, the fade and the 48 frames of the
   title's decompression are the same length in cycle 10 and cycle 11 (traced: `$89` from 480 at 27,917
   and 30,965; decompression 28,407-28,454 and 31,455-31,502); no sound wait is involved. [L, C]
6. **The palette cycle through the title.** `$00C8/$00C9` step on through the fade. After it `$80:93FB`
   calls `$80:D20E`, which waits a frame and then restarts the screen state (`$80:D219 JSR $A16A`;
   `$80:A1ED-A1EF` clear `$00C8/$00C9`) on the title's frame 9: `demo-cold` frames 909 and 3,956 (in the
   first title a continuous count is also (0,0) there, so a trace of that title alone shows no break;
   review round 1). The interrupt steps the cycle on every frame after it, the held frame 133 included
   (4,080), and `$83:9894` copies the direct page to `$77:0E6B` on the race's setup frame, after that
   frame's step, so the pairing reads `$00C9` 449 steps after the restart, 450 in a held title: 3 both
   ways (`$77:0F34`, frames 1,358 and 4,406). Native reset the cycle on the title's first frame and read
   the phase at the choice (frame 451), 450 steps later, which also gives 3, but the reset put the wrong
   colours on the fading menu (19,520 pixels). Native now keeps the menu's cycle through the fade,
   restarts it on frame 9, steps it on the held frame and pairs at the setup frame: native's delay and
   phase equal the original's `$00C8/$00C9` on every frame from the menu's last to the setup frame in the
   first, the held second and the short thirteenth titles (1,440 frames, `cycle`). The first fix of the
   fade restarted on frame 8 and skipped the held frame's step, two errors that cancelled at the setup
   frame (review round 1). [L, C]

## The listed cases

Native's own driver against the measured length, per kind (`summary.py`, sessions counted once; the
"measured" length is the commonest frame count of the session's key: a race's track and song, a
title session's origin):

| Kind | Sessions | Original's frames, request to end | Native driver gives the original's | Off the measured: native right | Measured right, native wrong |
|---|---:|---|---:|---:|---:|
| boot | 1 | 68 | 1 | 0 of 0 | 0 |
| first demo title | 3 | 76 (2), 77 (1) | 2 | 0 of 1 | 0 |
| later demo title | 43 | 76 (18), 77 (25) | 18 | 18 of 18 | 25 |
| race | 42 | 78-80 by song | 40 | 1 of 1 | 2 |
| title after a demo race | 45 | 69 (35), 70 (6), 71 (4) | 44 | 9 of 10 | 0 |
| podium | 8 | 79 | 8 | 0 of 0 | 0 |
| title after the podium | 8 | 71 (5), 72 (3) | 5 | 3 of 3 | 3 |
| title after a race | 40 | 69 | 40 | 0 of 0 | 0 |

- **The warm power-on's handshakes** (R-0090): **closed.** On main, OPTIONS' track records and high
  scores are equal on every picture of all three images (2,650 and 1,500 pictures each); the first
  idle demo's six pictures were finding 5's fade, which this task's change makes equal. What remains
  in the 21 warm runs is listed in `expect-warm.json`: the runner writes no picture on the frame it
  hands over to a race (the same on a cold boot, `demo-cold` 1,448), league-two-events frame 6,098 (787
  pixels, on all three images) and the all-gold image's second race (below). None follows the sound
  processor.
- **The two long title starts** (R-0088, cycles 10 and 17): **closed**, finding 5; every picture of the
  first, second, tenth and eleventh titles' starts and of the all-gold warm power-on's first equal
  (1,717 of 1,717; main 1,687), and the lap's retained pictures 993 of 1,006 (main 991; the other 13 are
  R-0088's small race sprite differences).
- **Short titles** (R-0087) **and held titles' slides** (R-0096): **cannot, under D-0010.** The title's
  slide comes in its session's end frame; the session ends either in frame 76 after the request (at
  396,654-399,996 clocks: a short title, no hold; after about 402,000: the slide a frame before its
  wait) or in 77 (held). The original's ends cluster within 45,000 clocks of the boundary, and native's
  reply carries the way back's error (finding 3). Even with finding 4's length made exact, native's
  driver would give the original's frame in 20 of 43 later titles against the hold rule's 25
  (`main-sweep`), so the front end keeps the rule and the laboratory's `--short-demo-title`.
- **The longer way back after a demo pad exit** (R-0096, `rv-late-split`): **cannot.** The original's
  way back ends 70 frames after its request, at 420,930 clocks; native's driver says 71 (its reply
  69,030 clocks late). The front end's rules give every other captured way back's frames; native's
  driver gives 44 of 45, missing this one too.
- **The podium's way back a frame late** (R-0098, R-0102): **cannot.** The original's reply is either
  1,199,740-1,203,010 clocks (the menus on time) or 1,229,446-1,230,606 (late), a different count of
  timer-2 pulses on a grid set by the podium session's end. Native's driver gives the original's frame
  in 5 of 8 podiums (all 3 late ones, and 2 of the 5 on time); the front end's rule (always on time)
  also 5 of 8. Native's podium sessions end 48,348 clocks early to 56,370 late, which is the grid's
  phase.
- **The podium's music a pass off in 2 of 8** (R-0099): **unchanged.** Native already plays it from its
  own driver (6 of 8); the pass follows the podium session's end, inherited as above.
- **A race's loading by a frame** (R-0077, R-0098): **cannot.** A race's first update comes one frame
  after its session's end, or two when the end is past 410,669 clocks (39 and 3 of 42 races; fitted on
  the original). Applied to native's driver that gives the original's start in 38 of 42 races; the
  measured (track, song) table gives 40 (`race_loading.py`). It keeps the table.

## Native change

`src/core/front_end.cpp`: the idle demo's title shows the menu's fade-out on its frames 1-7
(`demo_title_start_frame`, the file's `fade` as for `$80:9885` elsewhere: 13 down to 3, blank on 7, the
object buffer copied each frame, the palette cycle running on) and restarts the screen state and loads
the title on frame 9 (`$80:D219`); the held frame 133 steps the palette cycle; the demo's riders are
paired at the race's setup frame (`pair_idle_demo_riders`, from `set_up_demo_race`). The track, view
and counter are still chosen on frame 451.

## Limits

- The verdicts rest on 190 sessions of 21 captures: one cold lap's demos and their review's, five
  menu-mode schedules and six league tours (8 podiums). The podium's on-time/late split is 5/3 here,
  against R-0102's 7/5 and 25/15 sweeps.
- The native runs use D-0010's anchors as they are; better anchors (the podium way back's request is
  349,374 clocks into its frame in 6 of 8, native's title anchor 344,450) do not help, because the
  reply follows the driver's grid, not the request (finding 2).
- Finding 4's interrupt work is measured on the demo title only; other cued sessions whose screen has
  NMIs on (the award's, an ending's, the podium's) were not traced. Their lengths are within 6,134
  clocks of the original's.
- The palette cycle is compared frame by frame over three cold titles (first, held, short); the other
  31 cycles of the lap are checked through their pairing (race words) and retained pictures, and the
  warm power-ons through every picture.
- The warm runs' remaining differences (league-two-events 6,098, the all-gold race) were not traced.

## Reproduction

```
S=local/evidence/sound-processor-timing
zsh $S/sweep.sh OUT && python3 $S/summary.py OUT && python3 $S/carry.py OUT   # findings 2-3, the table
python3 $S/race_loading.py $S/sweep.list OUT                                   # a race's loading
python3 $S/trace_session.py local/evidence/demo-audio/demo-cold.script.json 1000 2   # finding 4
python3 $S/warm_waits.py INPUTS FIRST LAST [IMAGE]                             # where a boot waits
python3 $S/window_compare.py RUNNER PACK INPUTS FIRST LAST [--image IMAGE]     # pictures, finding 5
grep -v '^#' $S/checks.list | while read -r line; do eval "$line"; done        # the checks
```
