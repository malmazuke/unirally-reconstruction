# R-0099 - The league podium's music

Status: research result and native change ([PODIUM-AUDIO](../../tasks/PODIUM-AUDIO.md)), 17 October
2026, on main `6c50ac1`. PAL ROM SHA-256
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`; bsnes `7d5aa1e6` with
R-0075's audio observation core v5, Strict synchronization. It closes R-0098's "the podium's own
music" (finding 15 and Not covered) under
[D-0010](../decisions/D-0010-frame-anchored-sound-commands.md), with the method of
[R-0091](R-0091-mode-audio.md) and [R-0096](R-0096-demo-audio.md). Evidence is in main
`local/evidence/podium-audio/`.

Tags: **[L]** read from the static listing (`coverage disassemble` with the five raw coverage
inputs, main `local/evidence/podium-audio/listing/`, and the static map `docs/map/static/`).
**[C]** confirmed in a capture.

## Method

Each schedule is captured twice from a cold start with every dispatcher call site watched
(`cues/cue_captures.sh`, MENU-INPUT's watch lists), and each capture is stripped to its CPU rows.
The cue list is derived (`cues/derive_cues2.py`) and compared, frame waits included, with
`front_end_runner --sound-cues` (`podium_windows.py`: the whole schedule and each podium's
window).

The league's inputs are LEAGUE-TOURS' frozen pads (R-0098), from their manifests in main
`local/evidence/league-tours/` (`cues/schedule.py`):

| Schedule | Source | Frames | Podiums (session frame) |
|---|---|---:|---|
| `tour-cues` | LEAGUE-TOURS' own captures (`organic-cycle-five` with the track cursor at 4) | 16,800 | 11,420 |
| `full-tour` | LEAGUE's `organic-full-tour-turnaround` (a whole CRAWLER tour, no cartridge writes) | 40,264 | 37,234 |
| `pa-t-crawler` | `t-crawler` | 39,395 | 36,065 |
| `pa-tours-b` | `tours-b` (HOPPER, BOUNDER) | 72,034 | 36,728 and 68,704 |
| `pa-tours-a` | `tours-a`, first 64,000 frames (SHUFFLER, RUNNER) | 64,000 | 35,025 and 62,202 |
| `pa-odd-tours` | `odd-tours`, first 56,000 frames (three members, HUNTER) | 56,000 | 54,759 |

The conditional driver check (`cues/cond.sh`, R-0077's method) uses one capture of `tour-cues`
that keeps every row (`tour-cues-full`). The anchored PCM (`cues/measure.sh`, `pcm_window.py`)
reads each schedule's first capture, which keeps the PCM.

## Findings

1. **The podium's routine.** `$83:C9C1` calls `$82:DDCD` from the race's load path when a podium
   is due. `$82:DDEA-DDFA` loads the picture (`$82:DFF4`, `$82:D7C2`, `$82:D71F`, `$82:DF9E`,
   `$82:E103`), then runs the sound session `$82:E06F`. It then enables NMIs (`$82:DE01`) and
   enters the loop at `$82:DE06`. When `$0545` is set (`$83:FBB5-FBB7`, a comparison of
   `$77:000C` with `$83:FB56`), `$82:DDDE-DDE7` skips the picture, the session and the loop
   (`JSL $83:FADC`); that branch is inferred, never observed (Not covered). [L]
2. **The session is the medal award's set.** `$82:E071` calls `$82:8294`, which clears the sound
   queue's pointers `$68`, `$6A` and `$6C`. Then `$82:807E` loads driver 50 (`$82:E079-E07C`, the session's first FF request), tables
   52 (`$82:E082`) and score 58 (`$82:E08B-E08E`). `$82:82A5` then uploads the samples of the slot
   table `$83:FBF5` (`$82:E092-E095`). These are the medal award's driver, tables, score and
   samples (`$83:A624-A640`, R-0077 finding 5), so the podium plays the award's music.
   - LEAGUE-TOURS' cue derivation named the session "ddcd" after its routine `sub_82DDCD`. It is
     this score site (`$82:E08E`).
   - Pack v34 already carries the set (`audio.award-*`), so no pack change is needed.
   - After the samples the session calls the dispatcher eight times (`$82:E099-E0B5`), enqueues
     effect gain 255 (`$82:E0B9-E0BC`, command 8) and calls it sixteen times more
     (`$82:E0C2-E0FE`). It sends no music gain and no music start. [L, C]
3. **The loop starts the music.** Each pass of `$82:DE06-DF02` does four things in order:
   - It waits for the frame (`$82:D497`) and reads the pads (`$82:AA71`).
   - It leaves on its exit test (`$82:DE10-DE5B`, to `$82:DF05`'s RTS; finding 7).
   - It steps the poses (`$82:DF06`, `$83:F0FB`) and makes the copies.
   - At `$82:DEBC-DEFE` it counts its passes in `$1365`: unless the count is 8, it increments
     it, and while the new count is below 4 it enqueues effect gain 255, music gain 127 and the
     music start (`0x08FF`, `0x077F`, `0x0102`), then calls the dispatcher eight times
     (`$82:DEE2-DEFE`).

   `$1365` is otherwise the race's music latch. A race leaves it 0x7F or 0xFF (`$83:CA02`,
   `$83:CA69` and the songs' `$83:CAAE-CBC2`), and only a pause (`$83:F692`) or the demo's fade
   (`$83:E276`) clears it. What zeroes it is a bulk work-RAM clear on the podium's way in, on the
   entry's frame 7, 12 frames before the session (`t-crawler` 36,053: 7,025 bytes of `$0000-1FFF`
   zero after it, `$1363-$1369` among them). The review's `trace_1365.py` over every
   LEAGUE-TOURS probe shows it in all 17 probed podiums: 0x7F (15) or 0xFF (2), then 0 on the
   entry's frame 7, then 1 on the first pass 92 frames later (`podium-back` 11,500, `t-crawler`
   36,145, `tours-a` windows 0-8, `tours-b` 36,808 and 68,784, `odd-tours` 54,839 and 114,560,
   `five` 76,875 and 155,976). So the first three passes send the music, and the count stops at 8.
   The clearing routine is not identified. [L, C]
4. **Its frames.** Over the eight captured podiums:
   - The session's first FF request comes on the podium entry's frame 19, at 340,564-340,822
     master clocks after that frame's boundary (median 340,626).
   - Its last poll comes 79 frames later, at 167,214-223,046 clocks.
   - The loop's three music passes come 80, 81 and 82 frames after the session, which are the
     entry's frames 99 and 100 and the podium's first frame after them.
   - In LEAGUE-TOURS' frames, 11,420 is the session and 11,500-11,502 are the passes.
   - The first poll of each pass sits at its own clock: pass 1 at 84,188-84,240, pass 2 at
     122,582-123,166 and pass 3 at 90,368-90,672 (`anchors.json`). The passes do different work
     before their polls (the pose phases). [C]
5. **What reaches the driver.** The dispatcher sends a queued word only when the driver has
   taken the last one, and the eight polls take about 4,300 clocks. In every podium the first
   pass delivers effect gain 255 and music gain 127. The music start then reaches the driver at
   that pass's eighth poll or at the second pass's first. The second and third passes each
   deliver one more word. No later word reaches the driver before the way back's title session
     (`tour-cues`' and `full-tour`'s delivered commands to the way back).
   - The original delivers the music start in pass 2 in 7 of the 8 podiums and in pass 1 in
     `tour-cues` (`podium_commands.py`).
   - The difference is about one driver poll (1,850 clocks) in the driver's phase, which follows
     the length of the session's upload. [C]
6. **LEAGUE-TOURS' derivation hid half of it.** `derive_cues2.py` skipped up to four "session
   words" after any load, and it counted the loop's polls `$82:DEE2-DEFE` as the session's own.
   The podium's session sends one word, so the skip also took the first pass's three words. The
   original's 7 lines at 11,420 and 11,501-11,502 were therefore 34:
   - the load;
   - for each pass, three words and eight polls.

   The copy here names the session "podium", allows it one session word, and reports the polls as
   the sites `podium-1`, `podium-2` and `podium-3` by pass. Re-derived, LEAGUE-TOURS' captures
   differ from its own list only in those lines. [C]
7. **The loop's exit test, from its first pass** (review round 1). Every pass, the first
   included (the entry's frame 99), reads the pads before the test:
   - `$80:85E8-85FD` (the podium's NMI, set at `$82:DDD5-DDDC`) copies `$4218-$421B` into
     `$0311-$0314` each frame.
   - `$82:AA75-AC4C` turns them into flags. Pad 1 gives A, X, L, R (`$031D`, `$0321`, `$0329`,
     `$032D`) and B, Y, Select, Start (`$0331`, `$0325`, `$0335`, `$0339`), and `$0319` is 0 for
     Left, 2 for Right and 1 for neither. Pad 2 gives the same in `$031F`-`$033B` and `$031B`.
   - Pad 2 is read while `$7E:212C` is set, or `$12E1` and `$0C6D` are clear; pad 1 is cleared when
     `$12DF` is set; `$1335` reverses left and right (`$82:AC5A`). All five are 0 on every frame of
     the eight probed podiums' first 60 passes, so both pads are read as they are.
   - `$82:DE10-DE45` leaves if any of the sixteen button flags is set (a 16-bit OR; the bytes
     between them are 0 in the probes). `$82:DE48-DE5B` leaves unless both `$0319` and `$031B` are
     1, that is while Left or Right is held on either pad.
   - The flags are the pad's state that frame, not its presses, and Up and Down are never tested.

   Two captures of the review show it (main `local/evidence/podium-audio-review/cues/`):
   - `rv-press1` (`tour-cues` with A held on pad 1 over frames 11,490-11,509): the original leaves
     on the first pass. Its way back's title session is at 11,504 and no podium music is sent.
   - `rv-up-pad2` (Up held on pad 1 over 11,520-11,559, then Start on pad 2 over 11,600-11,603):
     the original stays through the Up and leaves on pad 2's Start (11,604).

   Native ran its first pass with no pads and tested pad 1's twelve buttons, Up and Down
   included. It now feeds the first pass the pads and tests the original's predicate. [L, C]

## Native

- `AudioSessionLoad::podium`, the cue log's `L podium`, is the award's set with the anchor
  340,626. After the samples, `end_podium_session` makes the eight calls, enqueues the effect gain
  and makes the sixteen calls.
- The podium entry's frame 19 starts it (`league_podium_entry_frame`).
- `podium_music_pass` is the loop's `$1365` count (`LeagueSetup::podium_music_passes`), which is
  0 at entry. It runs after a pass's poses, and a pass that leaves on its exit test
  (`leaves_podium`: either pad's eight buttons or Left/Right, never Up/Down) returns before it.
  The entry's frame 99 runs the first pass with that frame's pads.
  Each of the first three passes enqueues the three words and makes eight calls at its pass's
  site: `AudioDispatchSite::podium_first_pass`, `_second_pass` and `_third_pass`, logged as
  `podium-1` to `podium-3`, with the anchors 84,221, 123,126 and 90,525 (the medians, D-0010).
- The app's producer was already running through the podium (MODE-AUDIO). It now plays these
  cues.

## Results

- **Cues**, frame waits included [C]:

| Schedule | Lines | Podium windows equal | Frames differing elsewhere (candidate; main) |
|---|---:|---|---|
| `tour-cues` | 19,366 | 1 of 1 (35 lines) | 1; 1 (10,603, R-0095's result way out) |
| `full-tour` | 60,844 | 1 of 1 | 8; 8 |
| `pa-t-crawler` | 60,446 | 1 of 1 | 52; 52 |
| `pa-tours-b` | 114,763 | 2 of 2 | 62; 62 |
| `pa-tours-a` | 99,678 | 2 of 2 | 17; 17 |
| `pa-odd-tours` | 86,132 | 1 of 1 | 2,411; 2,411 |
| `rv-press1` (review) | 13,647 | 1 of 1 (2 lines; main differs on 3 frames) | 6; 11 |
| `rv-press2` (review) | 13,655 | 1 of 1 (13 lines; main 2 frames) | 4; 4 |
| `rv-up-pad2` (review) | 13,578 | 1 of 1 (35 lines; main 8 frames) | 6; 81 |

  - Each podium window runs from the frame before the session to the frame after the way back's
    title session: 35 original lines, all equal. Main differs on 4 frames in each (the session
    and the three passes).
  - Outside the podiums, the candidate's differing frames and lines are main's. The candidate's
    logs add only the 34 podium lines per podium.
  - On the review's three schedules, main also leaves the podium at other frames, so its later
    cues differ more; the candidate's remaining differences there are the way back's (Not covered).
  - The other differences are R-0098's and R-0095's recorded limits: the podium's way back a frame
    late, which shifts the cues after it until the next session; races loading a frame off;
    pairs a frame apart in an odd league; the result's way out.
  - With LEAGUE-TOURS' derivation, `tour-cues` differs only at 10,603 (as on main). Native's log
    is read with that derivation's conventions: the "ddcd" label, the polls left out and the first
    pass's words skipped.
- **Conditional driver check** (`cond.sh tour-cues-full title race-62 title award title race-63`,
  the podium's session as the award's set): all 2,712,437 ordered DSP and SMP port writes and all
  10,763,786 raw stereo pairs are equal. [C]
- **Anchored PCM** (`cues/podium_pcm.sh`). Each window runs from the podium's fade (20 frames
  before its session) to its way back's title session, 773 windows of 20 ms; `tour-cues` has 724.
  [C]

| Podium | Music start's pass, original / native | Within 1 dB | Median, 99th percentile |
|---|---|---|---|
| `pa-odd-tours` 54,759 | 2 / 2 | 768 (99.4%) | 0.02, 0.85 dB |
| `pa-tours-a` 62,202 | 2 / 2 | 761 (98.4%) | 0.02, 1.25 dB |
| `pa-t-crawler` 36,065 | 2 / 2 | 750 (97.0%) | 0.03, 1.46 dB |
| `pa-tours-b` 68,704 | 2 / 2 | 698 (90.3%) | 0.24, 2.90 dB |
| `pa-tours-b` 36,728 | 2 / 2 | 690 (89.3%) | 0.26, 3.20 dB |
| `pa-tours-a` 35,025 | 2 / 2 | 682 (88.2%) | 0.26, 3.22 dB |
| `full-tour` 37,234 | 2 / 1 | 393 (50.8%) | 0.98, 10.40 dB |
| `tour-cues` 11,420 | 1 / 2 | 362 of 724 (50.0%) | 1.00, 10.94 dB |

  - Native delivers the music start in the original's pass in 6 of the 8 podiums
    (`native_commands.sh`). In the other two its music is about a frame apart from the
    original's.
  - The difference is mostly that shift. Moved by about 650 pairs, `tour-cues`' music has 564 of
    673 windows within 1 dB, and `full-tour`'s 588 of 729 (`pcm_shift.py`).
  - Over whole schedules, `tour-cues` has 15,304 of 16,148 windows within 1 dB (94.8%), with 259
    of 259 commands in order and 258 in their frame.
- **The app** (`app_run.sh`): `app-debug` with `--native-title-menu-audio` runs 3,600 updates.
  - It warm-boots from `tour-cues`' cartridge at frame 11,350, which has a podium pending
    (`$77:0742` bit 13).
  - From NOW PLAYING it reaches the podium with no race: the session at 1,720, the music passes at
    1,800-1,802, the way back at 2,704.
  - The producer runs 2,980 cued frames and never stops. Its only drop is the boot's silence (2,815
    pairs, peak 0), with no later drop and no underrun.
  - Its cue log, with rotation cues resolved, equals `front_end_runner`'s (2,414 lines). [C]
  - LEAGUE's `organic-full-tour-turnaround` cannot show this in the app. Its second race (track 1)
    diverges from `front_end_runner`'s at frame 12,619 and never ends, on main `6c50ac1`'s app too.
    That is an app replay limit outside this task.

## Not covered

- **The music start's pass.** The music start reaches the driver at the first pass's last poll or
  at the second pass's first, depending on the driver's phase after the upload. The front end
  does not see that phase: it is D-0010's upload-length variation. Native gets it right in 6 of
  the 8 podiums; in the other two its music is a frame (one pass) apart from the original's.
- **The podium's way back a frame late** (R-0098, PODIUM-RETURN-TIMING). The way back's title
  session keeps its frame, but in about a third of podiums the original's later cues are a frame
  later than native's until the next session (`pa-t-crawler` 36,998-38,554, for example).
- **Presses on the first and second passes** are captured (`rv-press1`, `rv-press2`). A press on
  the third pass, which leaves before its music, is from the listing only.
- **`$1365` at the podium's entry** is 0 in all 17 probed podiums (finding 3), but the routine
  that clears it is not identified. A podium entered with another value would send fewer music
  starts, or none. That is not observed, and native assumes 0.
- **The exit test's other inputs.** If `$12DF`, `$12E1`, `$0C6D`, `$7E:212C` or `$1335` were set
  at a podium, a pad would be ignored or reversed. All are 0 in every probed podium, and native
  assumes so.
- **`$0545`'s branch** at `$82:DDDE-DDE7`, which skips the whole podium (the picture, the session
  and the loop), is inferred, never observed. Native has no such branch.
- **The way back after an early exit.** In `rv-press1`, `rv-press2` and `rv-up-pad2` the cues after
  the way back's title session differ on 5, 3 and 5 frames. These are frame waits and the slide's
  sound a frame apart, R-0098's late way back (main 10, 3 and 80 frames there).
- **PCM** is measured on the six schedules above only, and only from the anchored producer
  (D-0010: not exact).

## Reproduction

From the worktree (local/ linked), through `local/locks/heavy-run.sh`:

1. Captures: `cues/captures.sh pa-t-crawler pa-tours-b pa-tours-a pa-odd-tours full-tour` (the
   schedules from `cues/schedule.py --manifest`), and `cues/capture_full.sh tour-cues`.
2. Derivation: `cues/derive_cues2.py PREFIX 620 NAME.cues.txt` (for `tour-cues` with LEAGUE-TOURS'
   capture prefix).
3. Every measurement: `bash local/evidence/podium-audio/checks.sh OUTDIR` (cues against the
   candidate and main `6c50ac1`, anchors, the driver check, PCM, the app, the league's frozen
   gate).
