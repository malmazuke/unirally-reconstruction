# R-0101 - The keyboard's name check and its run time

Status: research result and native change ([NAME-CHECK](../../tasks/NAME-CHECK.md)), 27 October
2026, on main `1cf871fa`. PAL ROM SHA-256
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, reference core bsnes
`7d5aa1e6` (Strict); sound schedules with R-0075's observation core v5. Evidence in main
`local/evidence/name-check/`. It closes the first two items of R-0097's Not covered: the name
check and long names' lag.

Tags: **[L]** read from the static map's listing (main `local/evidence/menu-input/listing/`) and a
decode of the routines it leaves as data; **[C]** confirmed in a capture.

## Domain

Three cold-start probe captures of the original (`probe.py`: work RAM `$0000-1FFF` and cartridge
RAM every frame, a picture every frame from the keyboard on), each with its sound schedule
captured twice (`cue_captures.sh`, stripped to CPU rows):

| Capture | Frames | What it types (`make_names.py`) |
|---|---:|---|
| `nc-refuse` | 2,496 | RENAME PLAYER (review `rv-kb`'s way in): a listed word at the start (6 letters), one in the middle (7), two overlapping words (7), a word ending a full-length name (8), each refused, erased; then an 8-letter near miss (a word's first four letters at the end), accepted |
| `nc-lengths` | 4,692 | RENAME PLAYER eight times: names of 1 to 8 letters, accepted, with first letters many words share |
| `nc-league` | 2,900 | the league keyboard (MENU-INPUT's `league-name` to frame 1999): 13 letters no word starts with and a word (17, refused), erased; 13 letters ten words start with and the same word (17, refused), erased; 15 of those letters, accepted |

The words are named here by their place in the list, not spelled: the list is ROM content. With
MENU-INPUT's keyboard captures (`league-name`, `kb-input`, `w5`, `warnings`, `league-pick`, review
`rv-kb`) the check ran 24 times. `cycles.py` traces each call in the core's instruction ring
(scanline and dot of every instruction) from OK's JSL to the frame wait's first read of `$4212`;
`timing.py` places the calls. `compare.py` (RECORD-SCREENS') compares native's front end with each
capture on every menu frame and every picture; `diff_cues.py` compares the sound schedules from
frame 620, frame waits included.

## Findings

1. **The check** (`$80:A4AA-A4B6`, `$83:84D9-8799`). [L, C: `nc-refuse`, `nc-league`]
   - OK, after the select sound (`$80:A495`) and the name's two 0xFF (`$80:A4A2-A4A7`), sets
     `$0094` to 1 and calls `$83:84D9` with the buffer's address (`$0087`). `$83:84DD` sets the
     data bank to `$83`, so the name is read through bank `$83`'s mirror of work RAM.
   - For each position from the first until the name's 0xFF (`$83:84EE-84FB`), each of the 71
     words is compared in table order (`$83:8501-8525`: the pointer read at `$83:8505` from
     `$83:8537`, the next word at `$83:851B-8525` while below 71, `$83:851E`; the next position at
     `$83:8528`) by `$83:8779`:
     characters are taken while equal (`$83:877E-878C`) and the word matches at its 0xFF. The
     name's 0xFF never equals a word's character, so a word cut short by the name's end does not
     match (`nc-refuse`'s near miss).
   - A match returns at once with Z set (`$83:850F-851A`); the end of the name returns through
     `$83:852D` with the caller's flags (Z clear: the length is not 0).
   - The words are 71 strings of 3 to 11 lower-case letters; the keyboard stores letters in lower
     case (R-0097), so only letters take part in a match.
   - On a match OK prints `$80:A4C9` (a centred line on row 19), saving and restoring the
     printer's place `$009F`, clears `$0094` and goes back to the keyboard's loop
     (`$80:A4B8-A4C6`): the keyboard stays open with the name and its length, and the line stays
     until the keyboard closes. Every refusal in the captures (a word at the start, the middle,
     overlapping another, ending a full-length name, and the 17-letter league names) shows the
     line in the next frame's picture; the select sound is the same as an accepted OK's.
2. **The run time** (`cycles.py`, `model.py`). [C: all 24 calls]
   - The check's CPU time in master clocks is exactly
     `484 + 266 x positions + 720 x words + 174 x characters - 116 if it matched`
     (positions read, the name's end included; words compared; characters matched before a
     difference). 22 calls match to the clock, two within 40 (one refresh the step costing gives
     the wrong side).
   - Elapsed time adds the work RAM's refresh, 40 clocks in each 1,364-clock line (so
     `cpu x 1364 / 1324`), and 1,082 clocks for each NMI it runs through (one NMI in
     `nc-league` 2560 took 2,242).
   - OK's JSL comes 30,020-33,096 clocks after vblank's start (lines 247-249; the keyboard's pass
     after its frame wait). The frame wait's first `$4212` read (`$80:FAE3`) comes 1,060-1,100
     clocks after the check's return, or 26,886-26,926 after a refusal, whose print comes first.
   - A PAL frame is 312 lines, 425,568 clocks, starting at vblank (line 225). A read past the
     frame's end finds vblank over or not yet begun and waits for the next: each frame boundary
     the read passes delays everything after by a frame.
   - The model (start 31,600) puts each of the 24 reads in its measured frame; the nearest a read
     came to a boundary was 7.9 lines (`nc-league` 2560), the start's spread is 2.3 lines.
3. **What that gives.** [C, and the model's search]
   - A rider's name (up to 8 letters): 1-7 letters end inside OK's frame, 8 always one frame
     late (`nc-lengths`, `rv-kb`, `nc-refuse`'s near miss). The heaviest 7-letter name the model
     finds (91 matched characters) still ends 3.8 lines before the boundary, the lightest 8-letter
     one 23.8 lines after it.
   - A league name (up to 17): 8-14 letters one frame late (`league-name`: 12), 15 one or two by
     its letters (`nc-league`: fifteen of a letter ten words start with, two; fifteen of a letter
     no word starts with would be one, by 16.5 lines), 16-17 two.
   - A refusal stops at its match: the 17-letter refusals in `nc-league` matched at position 13
     and were one frame late; a full-length rider name refused at position 4 was not late.
4. **The late frames.** [C: `nc-lengths`, `nc-league`, `league-name`, `rv-kb`, sound schedules]
   OK's frame ends without a frame wait, so the select sound queued in it is dispatched at the
   next wait. Each late frame has only its NMI: no arrow step (the frame wait's `$80:FAF5`), no
   keyboard pass. The frame the check ends in ends with the frame wait; the next starts with the
   arrow and the keyboard's pass, which uploads the refusal's line or, with `$0094` still 1, leaves
   the keyboard for the commit (`$80:A256-A25E`).

### Native

- `src/core/name_check.cpp` counts the check from the name and the pack's word list as
  `$83:84DD-8534` runs it and turns the counts into the frames it runs late with finding 2's
  constants. It is a cost model of this routine only, not a CPU model.
- The keyboard's OK (`options_menu.cpp`) plays the select sound, writes the name's end, runs the
  check, then either commits, refuses (the line printed, the printer's place kept) or waits the
  late frames first; `waits_for_frame` gives those frames no frame wait.
- Pack profile v41 appends `front-end.name-check-words` (`$83:8537`, 578 bytes: the 71 pointers
  and the words) and `front-end.name-refused-text` (`$80:A4C9`, 18 bytes), both read from the ROM
  when the pack is built (`front_end.py` `v41_new_entries`). No earlier payload changes. A v40 or
  older pack has neither: every name is accepted at once, as on main.

## Results

The candidate on pack v41 against the original; main `1cf871fa` on v40 (main cannot read v41).

| Capture | Pictures equal (candidate / main) | Other differing frames (candidate) | Sound schedule (candidate / main) |
|---|---|---|---|
| `nc-refuse` | 1,206 of 1,206 / 119 | text 2158 (OK's OPTIONS print) | 2,236 lines equal / 161 frames differ |
| `nc-lengths` | 3,402 of 3,402 / 3,203 | text on 8 frames, one per commit (the same print) | 4,489 equal / 5 frames differ |
| `nc-league` | 910 of 910 / 139 | none | 2,579 equal / 97 frames differ |
| `league-name` | 2,400 of 2,400 / 2,143 | text 2501 (GROUP TABLES' first print) | 2,273 equal / 5 frames differ |
| review `rv-kb` | 2,300 of 2,300 / 1,879 | text, one frame (OK's print) | no schedule |
| `kb-input`, `w5`, `warnings`, `league-pick` | all equal / all equal | as main: OK's print | `kb-input` 1,945 and `w5` 3,636 equal, as main |

The text frames are R-0097's one-frame text buffers (Not covered there and here). `model.py`:
24 of 24 calls in their measured frame.

## Not covered

- **The start's spread.** The model starts every check 31,600 clocks after vblank; the captures
  spread 30,020-33,096, from the keyboard pass's work and the frame wait's polling, which native
  does not time. With the NMI's spread a read within about 2 lines (2,700 clocks) of a frame
  boundary may be put a frame off. No rider name comes that close; a 15-letter league name can.
- **A refusal's print across a boundary.** When the check ends just before a frame's end and the
  print runs past it, the original's text map holds part of the line at that frame's end; native
  prints it whole in the later frame. Not captured.
- **One-frame text buffers** after OK (OPTIONS' print) and GROUP TABLES' first print, as R-0097.
- The gate catalogue's front-end checks use pack v40, so they run without the check; this task's
  checks (`local/evidence/name-check/checks.list`) use v41.

## Reproduction

From a worktree of the candidate with `local/` linked, under `local/locks/heavy-run.sh`:

```
E=local/evidence/name-check
python3 $E/make_names.py                               # nc-refuse, nc-lengths, nc-league manifests
python3 $E/schedule.py NAME --manifest $E/NAME.json    # each schedule's script and inputs
zsh $E/captures.sh                                     # the probe captures and sound schedules
zsh $E/cycles_all.sh                                   # cycles-*.json: every call, traced
python3 $E/model.py $E/cycles-*.json                   # the model against them
zsh $E/nc_one.sh compare DIR NAME FIRST OUT            # pictures and state, candidate and main
zsh $E/nc_one.sh cue DIR NAME OUT                      # sound schedules
python3 tools/project.py content pack --rules tests/manifests/content/classic-crawler-tracks-pack.json --out local/classic-pal-crawler-tracks-v41.pack
```

`timing.py MANIFEST FIRST LAST` lists the calls, returns, frame waits and NMIs frame by frame.
