# R-0076 - native audio through the first ordinary race

Status: in progress for [AUDIO-FIRST-RACE](../../tasks/AUDIO-FIRST-RACE.md),
1 October 2026 UTC. PAL ROM SHA-256
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`; bsnes
`7d5aa1e656b9171524d01b1b22917197d8121cb4` with R-0075's opt-in audio
observation core v5 (SHA-256 `3483e8bc...4539`), default options, Strict
synchronization, fresh private save directory per process. Private captures
and scripts are in main `local/evidence/audio-first-race/`.

## Tested domain

`primary.script.json` is FRONT-END-1P-CONTINUATION's cold `cont-win` path
truncated to 4,000 frames: power-on, Start through the title and the four 1P
setup screens with their defaults, the first CRAWLER race on DRAGSTER won by
MIKE (Right held 1500-3299, Up 2200-2259), the result left with Start at 3800
and PICK TRACK. Two fresh captures (`primary-a`, `primary-ctl`) give identical
raw PCM (2,562,673 stereo pairs, SHA-256 `c926b53b...5da0`) and identical event
counts; later captures add SMP or CPU watches only.

## Verified observations

1. **Three sound program sessions.** The CPU uploads through the IPL three
   times: at boot (frames 29-38), before the race (1249-1260) and after it
   (3459-3468). Each session sends the same 4,445-byte driver (resource 50).
   The race session sends resource 54 (ROM `0x9ED13`: 1,583 data bytes to
   `$1600`, plus the next record's six header bytes) and resource 62 (ROM
   `0xA1F65`: 2,457 data bytes to `$1D00`, plus six), and the samples selected
   by the 64-byte slot table at `$83:FC75` instead of the title's `$83:FCF5`.
   The post-race session returns to the title set.
2. **Race song selection.** Static `$83:CA08-CBC8` (bank-83 listing) increments
   cartridge byte `$77:10B1` modulo 6 and selects the race score from resources
   64, 62, 63, 64, 65, 66. A cold cartridge gives 1 for the first race, so the
   first race plays resource 62 (`race-song-1`). The other songs are outside
   this domain.
3. **Commands on the played path** (port 2/3 pairs written at `$82:806D`):
   menu effects 2/4 with gains (command 8) through setup; command 3 with
   parameter `0x90` at frame 1207 before the race load; in the race, effect 15
   at 1354/1414/1474 (countdown), commands 11 and 6 with parameters 20-43
   paired with effects 12-17, and command 3 `0xA0` on 61 consecutive frames
   from 3393; after the reload, the title's music start and menu effects.
4. **Driver semantics recovered for the race set.** Readings of the uploaded
   driver with a laboratory SPC700 decoder (`spc_disasm.py`; a reading aid, not
   product content):
   - Command 3 (`$063B`) stores `sign_extend(parameter) * 8` as a signed
     per-update rate. The music pass (`$068F-$06B5`) adds it to the volume word
     `$DF/$E0`; the DSP master volume is `$E0`. Leaving `0000-7FFF` stops the
     rate with the high byte 7F (rising) or 00 (falling) and the sum's low byte
     kept. Music and effect starts reset the word to `7F00` but not the rate.
   - Commands 6/11 (`$0656`/`$0663`) clear/set flag `p`: bit `p & 7` of byte
     `p >> 3` in an eight-byte table (`$033C`). Score controls A5/A6 set/clear
     a flag; A7/A8 jump like control 81 when the flag is set/clear and otherwise
     skip the two-byte target.
   - Control 86 sets the fixed note duration (`$0230`).
   - Control 8C starts a table-driven gain envelope (period, table pointer).
     `$0B42` steps it every `period` updates: a positive byte is the DSP GAIN,
     `0x80` jumps back to the loop index, another negative byte holds; at the
     release point the position and loop move past the table's first negative
     byte. `$0B8A` skips the software envelope while it is active; controls
     97, 9B and A2 end it; a note restarts it unless control 9C holds it.
5. **Envelope correction.** The software envelope's decay and release phases
   (`$0BFE`, `$0C3A`) branch at `$0C13`/`$0C4A` on the flags of `MOV Y,A`,
   because `POP` sets no flags. The branch is never taken there, so a zero
   decay or release length still divides, with the hardware's divisor-zero
   quotient. AUDIO-TITLE-MENU's native code skipped that division; a race voice
   with release length 0 (frame 2866) exposed it. An SMP watch at every branch
   of `$0B8A-$0C65` in `primary-env` shows the taken path and the resulting
   GAIN `0x81`. The corrected native code reproduces all six of
   AUDIO-TITLE-MENU's frozen title/menu/HUNTER comparisons unchanged
   (`regression-1.json`).
6. **Conditional exact race audio.** With the original's CPU port writes
   supplied at their SMP ticks, the native IPL, driver, score and DSP model
   started from zero reproduce all 703,154 ordered DSP-register and SMP-port
   writes and all 2,562,673 raw stereo pairs of `primary-a`, through the race,
   its finish fade, the post-race reload and the return to PICK TRACK
   (`cond-race-2.json`). No original DSP event, snapshot or post-entry RAM write
   is an input. This is SMP/DSP evidence; the CPU-side producers and their
   timing are the remaining part of the outcome.

## Timing of race commands

The race loop calls the dispatcher `$82:8035` at `$83:CD6E` and `$83:CD9F`
(bank-83 listing), and other sites send the countdown effects. In `primary-a`
those two sites run 7,250-42,168 and 167,452-239,482 master clocks after the
previous frame boundary, while the driver polls port 2 about every 178 SMP
ticks (about 1,850 master clocks). Which poll first sees a command, and so
every later DSP write time, depends on the CPU work of the race body before
the dispatch.

## Frame-anchored transport (D-0010)

The race's dispatch sites and the setup screens' frame waits are not cycle-modelled, so
[D-0010](../decisions/D-0010-frame-anchored-sound-commands.md) delivers commands at declared
frame-anchored clocks after the exact title/menu model's 1P exit. Frame n's work follows the
vertical-blank boundary at line 225 that ends frame n-1, observed at 306,900 + n * 425,568
master clocks from power-on in every one of `primary-a`'s 4,000 frames. Calibration constants
are the medians of the dispatcher call sites watched in `primary-disp1/2` (every `JSL $82:8035`
site in banks 80-83): the setup screens' `$80:FADF` 13,082, the race's `$83:CD6E` 27,920 and
`$83:CD9F` 207,728, the countdown's `$83:E739`/`$83:E74F`/`$83:E785` 31,564, the finish fade's
`$83:E82C` 26,324, NOW PLAYING's `$80:99BE` 9,908, and the first FF requests of the race load
(169,072) and post-race reload (344,450). `$83:A923` is a second frame-wait routine that also
polls the queue. A dispatch whose anchor has passed runs at once; a frame wait in a frame the
CPU has already left (an upload) is skipped.

Driven by cues derived from the original's watches (`derive_cues.py`), `race_audio_runner`
delivers all 120 commands in the original's frames and order; arrival clocks differ by
-43,072 to +26,596 master clocks, and the race load's first command is exact (`anchored-2`).

## Native producers

Static listings name every enqueue (`JSL $82:8000`) and dispatch site on the played path;
`primary-enq1/2` and `primary-disp1/2` watch them all. The native front end and race engine
emit the same operations in program order:

- Menus: the slides' sounds at `$80:E233` (forward: volume 79, effect 2) and `$80:E27E`
  (back: effect 1); a choice's `$80:B124` (volume 63, effect 4) from the rider menu
  (`$80:BBB8`), NOW PLAYING's Race/Exit (`$80:B4A0`, `$80:B4B1`; Back is silent) and a tour
  reveal (`$80:E595`); NOW PLAYING's arrow moves play the navigation sound (`$80:B4DE`,
  `$80:B4F1` to `$80:B178`); the result's `$80:B10F` (volume 63, effect 6) at `$80:9596` on
  the fade's last frame; NOW PLAYING's Race fades the menu music (`$80:99B7`, command 3 with
  `0x90`) and dispatches at once (`$80:99BE`).
- Frame waits: every front-end frame that ends in `$80:FADF`/`$83:A923`, taken from the
  existing `waits_for_frame` timing plus the `$83:A923` waits that leave the arrow alone
  (award, ending, and the wait before `$83:879A`'s scoring).
- Loads: the race start's countdown cue (`$82:D84A`) 85 frames before the native
  initialization (the upload's queue reset drops it), the race set's upload 79 frames before
  it (`$83:CA72`), and the title set's reload on the race return's fourth frame (`$80:A0F7`,
  `$80:A0FC`).
- Race: the countdown (`$83:E59C-E785`) beeps when `$11C5` first falls below 250, 190 and
  130, sounds GO below 70 and dispatches on every published update; the finish fade
  (`$83:E7F2-E830`) from the finish display's 180th update; the skid flag follows the brake
  latch `$0D53`/`$0D55` (`$82:999A-9A49`); checkpoints set or clear flag 20 by speed
  (`$81:828E-82B2`); rotation flags 42/43 follow the `$1003`/`$1005` latches, which the audio
  side keeps (`$82:A507-A5F3`); each read announcement plays its voice from `$81:C441`
  (`$81:C191-C216` player, `$81:C2D0-C357` opponent; the 72-byte table is pack entry
  `audio.announcement-voices`); then `$83:CD6E` and `$83:CD9F`.

`front_end_runner --sound-cues` on the primary schedule writes 5,791 cues for frames
620-3,999, equal line for line to `primary.cues.txt` (4,757 without the frame waits); the
native race initializes at 1,328 as before. The stunt event's beeps follow the static reading
only; no stunt event, pause or other mode is compared yet.
