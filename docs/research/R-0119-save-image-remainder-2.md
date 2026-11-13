# R-0119 - The saved image's five remaining differences (R-0117's queue)

Status: research result and native change ([SAVE-IMAGE-REMAINDER-2](../../tasks/SAVE-IMAGE-REMAINDER-2.md)),
13 November 2026, on main `aa647f41` (claim `e51bc84b`). PAL ROM SHA-256
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, reference core bsnes `7d5aa1e6`
(patched, Strict). Evidence in main `local/evidence/save-image-remainder-2/`; the corpus, residual
and comparison are R-0117's (`local/evidence/save-image-remainder/`).

Tags: **[L]** read from the static map's listing (main `local/evidence/static-code-map/static-map/`)
or the ROM's bytes; **[C]** a capture or instruction trace of the original made for this task
(`vtrace.py`: each PC's first execution per frame with its scanline `v` and dot clock `h`;
`wram_watch.py`; `orig_markers.py`; R-0111's change logs); **[N]** native's own models run as a
measurement (the front end's cues, `race_audio_runner`'s title/menu CPU model and sound processor);
**[R]** an accepted record.

## Question and answer

R-0117 left five differences between native's cartridge RAM image and the original's over R-0111's
corpus. For each: can native equal the original at every write frame, and if not, why not and does
it matter? **Answer:**

| Item | Result |
| --- | --- |
| 1. `c385`'s menus after its race | **Fixed.** RECORDS' list loop chooses before it moves (`$80:B983-B99E`); native moved first. |
| 2. The league editor's buffer tail (`c039`) | **Fixed.** The last membership marker's two characters stay at `$00E1-00E2` (`$80:9D66-9D6A`). |
| 3. A saved marker step of 14 or more | **Fixed for the image.** The step is a word; the turns count it down from the saved value. Six warm boots are equal at every frame. |
| 4. The league markers' edges (`c029`, `c032`, `g069`) | **Localized, open.** The title-return sound session after the league's podium runs about 31,000 master clocks longer on these routes, and everything after it moves one frame. Native does not model why. |
| 5. The race start on HUNTER's preloaded tours | **Explained, open.** The warm boot changes the sound processor's phase, and the upload ends 2,256 clocks before the frame's end, too late for the race setup's tail. Native's own sound model predicts the original's start on all 527 runs; the front end does not consult it (D-0010). |

Differing write frames outside the residual drop from 11,879 to 11,594 (725 of 916 routes equal;
`c039`, `c385` and `c434` move, all toward the original).

## Method

- **The corpus and the comparison** are R-0117's (`save-image-remainder/routes.json`,
  `residual.txt`, `sweep.sh`, `compare.py`); `check_image.sh` here compares with this task's
  re-recorded `expected.json`.
- **Behaviour**: `stdout_same.py` (R-0117) compares the menu state of every frame and the sound
  cues of main's runner and the candidate's on all 919 routes (`stdout-main-vs-candidate.json`).
- **Main** is `base/front_end_runner` and `base/race_audio_runner` here, built from the claim
  `e51bc84b` (main `aa647f41` plus records).

## Findings

### 1. `c385`: a choice that comes with a direction [C, L]

After its race the fuzz pads drive RECORDS (`dragster-ordinary-controls` diff-fuzz 155). On 3,527
pad 1 presses B, Right and Up together (`$72` = 0x8900).

- **The original** keeps `$009B` at 0 and opens the first entry's detail. The logo slide (`$0742`
  bit 1) starts on 3,527 (`traces/c385-wram-009B-0072.txt`).
- **Native** moved the arrow up to EXIT first (selection 4), then took B as EXIT's choice and went
  back to the main menu. From there the screens differ.
- **The listing.** The generic list loop `$80:B93C` tests its exits before its moves on each pass
  (`$80:B983-B99E`):
  - Y or X (`$80:B74A`) and B, Start or A (`$80:B71D`) leave with the entry the arrow is on;
  - only then are Down or Select (`$80:B794`) and Up (`$80:B76F`) read.
- **The change.** Native's three loops on it (OPTIONS, the league slots and RECORDS) now move only
  when no exit button is down (`leaves_list`, `options_menu.cpp`).
- **Effect.** `c385`'s image is equal but for four frames of `$0743`'s known bit timing (R-0105).
  `c434`, another fuzz run (209), loses its one differing frame.
- **Behaviour changes on four routes** (`stdout-main-vs-candidate.json`):
  - `c385` from 3,527 and `c434` from 4,368;
  - `g078` (1,950) and `g079` (1,750): A, Select and Y together on OPTIONS. Select no longer moves
    the arrow before Y leaves. The original's `$009B` stays 0 there, as the candidate's does.

### 2. `c039`: the league editor's buffer tail [C, L]

- **What the bytes are.** A league's membership picker prints each mark through the text printer's
  scratch `$00DC` (`$80:9D3E-9D49` copies the stream `$80:9E0F` there in 16-bit words, 9 bytes
  to `$00DC-00E4`). `$80:9E3C` then sets
  its column and row. An unmarking stores two underscores at `$00E1-00E2` (`$80:9D66-9D6A`) where
  the stream has the X and its stop (0xFB).
- **Why they reach the image.** The league editor (`$80:9E57`) types over the scratch from its first
  byte. A one-letter name's commit copies the rest of the scratch into the league record.
- **The capture.** `c039`'s last press before naming (1,940, pad 2's X) unmarks a rider, so the
  record gets `5F 5F` at `$0173-0174` (`traces/c039-wram-00DC.txt`). The tail alternates between
  `58 FB` and `5F 5F` with each mark and unmark.
- **The change.** Native kept the column and row of the last marker (R-0073) but always the X. It
  now keeps the marker's two characters (`LeagueSetup::marker_characters`) and puts them in the
  editor's scratch.
- **Effect.** `c039`'s 170 differing write frames drop to 1: the league pair's 3-frame write at
  1,970 (R-0111's timing class).

### 3. A saved marker step of 14 or more [L, C]

**Where the step is read.** `$80:EB23` turns the markers each pass of a list loop (`$80:B97A`) and
on PICK TRACK. Each turn:

- loads the word `$77:10A7` and decrements it; when bit 15 comes out set it becomes 13
  (`$80:EB28-EB32`);
- reads the tiles at `$80:EB61` + step (`$80:EB39`) and + step + 7 (`$80:EB52`).

**What the original does** [C] (`orig_markers.py`; c175's preloaded image with the word set, pads
of `c039`, which go from the main menu to OPTIONS):

- The OPTIONS loop counts a saved 0x0020 down from 0x1F at 660 and reaches 13 at 678. On the way
  it reads past the 21-byte table (`$80:EB61-EB75`) into the code after it: tiles `F0`, `14`, `A0`,
  `E0`, ..., the bytes at `$80:EB80`, `$80:EB7F`, `$80:EB85`.
- 0x8000 counts on from 0x7FFF: the decrement leaves bit 15 clear. R-0117's "0x8000 and up give 13
  at the first step" holds from 0x8001 only.
- On PICK TRACK the step is stored 0 first (`$80:E872`), so a saved step reaches only the list
  loops before a PICK TRACK.
- The entries 64-68 are hidden there. Booted with 0x0020 and with 0, the original's pictures are
  equal on every frame to 1,100 (`marker-step/pictures-c039-pads.json`); its OAM buffer and
  `$10A7` differ. Review 1 added boots with 0x8000, which keeps the step past the table for the
  whole run, against 0: pictures equal on every frame on `c039`'s pads to 2,600 (OPTIONS, the
  league slots and their warning, the rider picker, the keyboard) and on `c075`'s to 1,700
  (RECORDS and a detail) (`marker-step/pictures-8000-*.json`). On `c078`'s and `c081`'s pads (the
  two-player and VS menus, to 1,550) no turn ran. The league's continue screen and the local modes'
  loops are not checked.

**The change.** Native loaded the step only when its low byte was below 14, and stored a byte.

- `TrackMenu::marker_step` is now the word. A boot loads it whole (`cartridge_ram.cpp`).
- Each turn decrements it as `$80:EB28-EB32` does and stores both bytes (`store_marker_step`).
- A tile index past the pack's 21-byte `front-end.marker-tiles` leaves that entry's tile unchanged
  (`turn_markers`). Those entries are hidden on every screen the captures turn such a step on.

**Result.** Six warm boots (`marker-step/routes.json`: 0x0014, 0x0020 and 0x8000 past the table;
0x000E, 0xFFF4 and 0x0020 on PICK TRACK as controls) are equal to the original's change logs at
every frame outside the residual (`check_marker_step.sh`). Main differs on the first three for
640 frames each.

**The ending's counter.** It moves from 0xFFF4 up to 0x0020 (R-0117), so the game's own saves give
steps 14 to 0x20 or 0xFFF4-0xFFFF. All are now modelled for the image.

### 4. The league markers' edges: a sound session a frame longer [C, N]

**Where it starts.** On `c029`, `c032` and `g069` native's hidden markers run one frame ahead of
the original's from the league's continue screen (KEEP SCORES, RESET SCORES, EXIT) to the run's
end. That is 2,048 to 2,748 frames of `$10A7`; R-0111's "value" frames are the ends of that run.

- In the original, the continue screen's loop starts (`$80:FBEE` then `$80:B93C`, then `$80:EB23`)
  one frame after native's: 37,049 against 37,048 on `c029` (`traces/c029.vtrace.txt`).
- Its set-up (`$80:FBD5`) runs at 37,002, where native's podium exit makes it at 37,001.
- Both start the title-return sound session after the podium (`$80:A0F7`) on 36,899.

**Which routes.** 16 corpus routes reach the continue screen (`traces/item4-continue-visits.txt`).
On 11 the original's timing equals native's; on `c029`, `c032`, `g069`, `c100` and `c101` it is a
frame later.

**Why, from `c032` against `g066`.** The two have the same pads and a different league byte poked at
10,500 (`$067E`: 43 against 4).

- Both request the session at the same clock (`$80:A0F7` at 12,204, line 168, dot 864).
- `c032`'s sample upload (`$82:82A5`) is 23 scanlines, about 31,000 master clocks, behind
  `g066`'s at the same step: the resource walk `$82:812A` returns (`$82:8150`) at 12,238 line 226
  against 12,237 line 203 (`traces/c032-all.vtrace.txt`, `g066-all.vtrace.txt`).
- `c032`'s frame waits then run one frame late from 12,277 (`traces/c032-pod.vtrace.txt`), and its
  `$80:FBD5` comes on 12,307 against 12,306.

**Native's sound model does not reproduce it** [N]
(`traces/item4-native-session-ends.txt`). Fed native's cues:

- `c032` and `g066` have identical cues and identical session ends;
- the ends do not separate the five late routes from the others (`c030` and `c029` both end
  12,000 clocks into the 72nd frame; the original is a frame late on `c029` only).

The upload's length here depends on something before the session that native's cues do not carry:
the CPU's work around the request or the sound processor's state after the anchored scenes (D-0010
says the product's sound is not exact after the first anchored command). Left open.

**What it does to a later boot.** From the continue screen on, the image's `$10A7` is one step off
the original's, so a save made after it holds a step one off (both within 0-13). A later boot turns
its hidden markers one step apart on the list loops until PICK TRACK stores 0 (`$80:E872`) before
its first turn; nothing else reads the word, and the markers are hidden there (item 3's captures).

### 5. The race start on HUNTER's preloaded tours [C, N, L]

R-0117 found native starting these races a frame early (track 41) or late (track 8) against
R-0077's (track, song) table. This task finds what makes the original's start move.

**The original, `c175`** [C] (`traces/c175*.vtrace.txt`):

- The race session's first FF request is at `$83:CA77` on 1,311, 214,682 master clocks after the
  frame boundary. Native's anchor for track 41 is 215,139 (`race_load_anchors`).
- The four polls end at `$83:CBF2` on 1,390, line 223 dot 472: 2,256 clocks before the next
  frame's boundary (line 225).
- The race setup's tail (`$83:CBF2-CC83`, the colour maths at `$82:D51E-D6DF`) reaches the frame
  wait `$00:8587` at line 227 of 1,391, 5,214 clocks after `$83:CBF2`. So the wait ends on 1,392,
  and the race's first update is 1,392: 81 frames after the request, where the table has 80.

**Native's sound model** [N]: `audio_starts.py` and `race_audio_runner`.

- How it runs: `race_audio_runner` runs the title/menu CPU model from power-on with the run's image
  (new `UNIRALLY_CARTRIDGE_IN`, the boot's signature check as for the app's save file), then
  native's cues at their anchored clocks. It reports each load cue's end clock (new
  `UNIRALLY_LOAD_ENDS`).
- On `c175` the polls end 2,652 clocks before the frame's end, against the original's 2,256.
- Without the image (a cold boot's defaults work) they end 7,404 clocks before it, and the race
  would start on 1,391.
- **The cause** is the warm boot. Its skipped defaults work changes the sound processor's phase
  when the request arrives, and the upload ends that much later.

**Over the corpus** (`audio-starts.json`, `check_audio_starts.sh`), the rule is: first update = the
frame after the one the polls end in, plus one more when fewer than 5,214 clocks of that frame are
left.

| Runs | Native's table | The sound model and the rule |
| --- | ---: | ---: |
| All 527 track_reference runs | 484 equal | 527 equal |
| Track 41 with `$10B1` = 0 (34 runs) | 1 early | equal: 2,652 clocks left, so +2 |
| Track 43 with `$10B1` = 0 (8 runs) | 1 early | equal: the polls end 15,472 clocks into the 80th frame |
| Track 8 with `$10B1` = 0 (1 run) | 1 late | equal: the polls end a frame sooner |

The runs with +1 leave at least 18,598 clocks; those with +2 leave 2,652. The tail threshold is
measured on `c175` only, and no run falls between those margins.

**Why native does not start the race this way.** Native's front end takes the race start from R-0077's
table, because "the game's frames do not depend on the audio" (D-0010). Following the sound model
would need two things:

- the front end and the laboratory runner run the title/menu CPU model and the sound processor for
  every run: about 1.8 s of CPU per 1,400 frames here, where the runner takes well under that;
- a decision that the front end's frames may depend on the sound model, amending D-0010, and the
  app's `--no-audio` path running the model silently.

That is a design change beyond this task. It is queued as
[SAVE-IMAGE-REMAINDER-3](../../tasks/SAVE-IMAGE-REMAINDER-3.md). Until then the personal bests on
`c175`, `c177`, `c210`, `c220` and `$1116` on `c209`, `c217`, `c218`, `c272` can still differ in
play, as R-0117 records. The sound model reproduces the start only where the cues before the
session are exact: here, the cycle-modelled title and menus (D-0010). Item 4 shows a session after
anchored scenes where it does not.

## Results

`check_image.sh` here, differing write frames from frame 406 outside the residual bytes, of 363,007 [C]:

| Runner | Differing write frames | Routes equal (of 916) |
| --- | ---: | ---: |
| main (R-0117's candidate) | 11,879 | 724 |
| candidate | 11,594 | 725 |

Routes that change, main to candidate:

- `c039`: 170 to 1;
- `c385`: 119 to 4;
- `c434`: 1 to 0.

No other route's image moves. The corpus's menu states and cues change on `c385`, `c434`, `g078`
and `g079` only.

## Limits

- **Item 5** is explained and checked, not changed in the product. Native's start still follows
  R-0077's table, one frame early on HUNTER's preloaded tours. The 5,214-clock tail is measured on
  one run; the margins of the 527 runs leave a wide gap around it.
- **Item 4** is localized to the title-return session after the league's podium. Its cause is not
  found, and native's sound model does not reproduce it.
- **Item 3**:
  - The tiles past the pack's table stay as they were. The original writes code bytes from
    `$80:EB76-EB87` into hidden OAM entries; native's OAM buffer differs from the original's
    there for up to 19 frames after such a boot.
  - Adding the bytes would need pack profile v42, which every gate and session would pick up as
    the newest pack.
  - Pictures are checked equal on OPTIONS, the league slots and their warning, the rider picker,
    the keyboard and RECORDS; the league's continue screen and the local modes' loops are not
    checked.
  - Steps 0x0021-0x7FFF other than the ending's are not written by the game. They count down
    the same way and leave hidden tiles stale until 13.
- **Item 1**: the loop order is changed on OPTIONS, the league slots and RECORDS. Other loops on
  `$80:B93C` (the league's continue screen, the local modes) already test their exits first.

## Reproduction

From the candidate's worktree (`local/` linked to main's), after building lab-release,
`H=local/evidence/save-image-remainder-2`:

```
zsh $H/check_image.sh OUT                    # PASS; with $H/base/front_end_runner: FAIL, 3 routes
zsh $H/check_marker_step.sh OUT              # PASS; with base: FAIL, 3 routes
zsh $H/check_audio_starts.sh OUT             # PASS; with base runners: FAIL
python3 $H/marker_routes.py $H/marker-step/routes.json
python3 local/evidence/save-image-exact/original_trace.py $H/marker-step/routes.json $H/marker-step/original 6
PADS_FROM=c039 python3 $H/orig_markers.py c175 0020 900
PADS_FROM=c039 python3 $H/orig_marker_pictures.py c175 0020 0000 1100
PCS="80FAF1 80FBD5 80A0F7" python3 $H/vtrace.py c032 12195 12310 OUT.txt
python3 $H/vtrace.py c175 1389 1392 OUT.txt
python3 $H/ends.py c032 12300
python3 local/evidence/save-image-remainder/stdout_same.py $H/base/front_end_runner build/lab-release/src/core/front_end_runner OUT.json 8
```
