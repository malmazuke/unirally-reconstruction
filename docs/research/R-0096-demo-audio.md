# R-0096 - The idle demos' sound

Status: research result and native change ([DEMO-AUDIO](../../tasks/DEMO-AUDIO.md)), 11 October
2026, on main `2c42589`. PAL ROM SHA-256
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`; bsnes `7d5aa1e6` with
R-0075's audio observation core v5, Strict synchronization. It extends
[R-0091](R-0091-mode-audio.md) and [R-0077](R-0077-one-player-audio.md) under
[D-0010](../decisions/D-0010-frame-anchored-sound-commands.md). Evidence in main
`local/evidence/demo-audio/`.

Tags: **[L]** listing only, **[C]** confirmed in a capture.

## Method

R-0091's, with its scripts copied. Each schedule is captured twice from a cold start
(`captures.sh`, which now strips each capture to its CPU rows as soon as it ends). The cue list
is derived (`derive_cues2.py`, which now names the demo's session by its score site `$80:9452`
and the race's fade-out `$83:E26E` as `demo-fade`) and compared with `front_end_runner
--sound-cues` frame by frame (`cues.sh`, `diff_cues.py --waits`, frame waits included). One more
capture keeps every row (`capture_full.sh`). It serves the conditional driver check (`cond.sh`,
R-0077's method) and the anchored PCM measurement (`measure.sh`, R-0091's).

| Schedule | Inputs (cold power-on, Start on frames 300-305) | Frames | Demo cycles |
|---|---|---:|---|
| `demo-long` | none after Start | 60,000 | 19 races (tracks 1-25), split and one-view in turn, the held returns after tracks 20 and 24, short titles 13, 16, 18 |
| `demo-cold` | the same, first 10,000 frames (R-0091's schedule) | 10,000 | 3 |
| `demo-pad-early` | A on pad 1, frames 5000-5003 (one-view race, before its warning) | 9,000 | 2 and a pad exit |
| `demo-pad-late` | Left on pad 1, frames 6400-6403 (after the warning) | 10,000 | 2 and a late pad exit |
| `demo-pad-port2` | B on pad 2, frames 8000-8003 (split race) | 11,000 | 3 and a second-pad exit |
| `rv-late-split` (review, `local/evidence/demo-audio-review/`) | Y on pad 1, frames 3300-3303 (the first split race after its warning, the computer rider rotating) | 10,000 | 3 and a late pad exit |
| `rv-early-p2` (review) | X on pad 2, frames 1460-1463 (the first split race's countdown) | 10,000 | 1 and an early exit |
| `rv-long` (review) | none after Start | 99,000 | 32 races, short titles 13, 16, 18, 21, 33 |

`demo-long`'s first 10,000 frames derive to exactly R-0091's `demo-cold` cues (with its labels
renamed), so the original is deterministic over these schedules.

## Findings

- **The demo's own sound set, not the title's.** R-0091 read the demo title's session as a
  title reload because its captures watched only the way back's score site (`$80:A10B`), the
  next one. The demo title (`$80:93FB`) has its own session at `$80:9440-9455`: driver 50, tables
  52 (the medal award's), score 56, then the samples of the slot table at `$83:FC35`
  (`$80:9459-945C`). It then sends the music start, effect gain 255 and music gain 127
  (`$80:9460-9471`) [L, C].
  - Score 56 is R-0093's unpacked block: the resource at `$93:F70C`, its data from `$93:F712`
    (2,899 bytes).
  - The slots add samples 02, 23 and 34 to the title, race, award and ending sets. Pack v39
    carries the score, the slot table and those three samples.
- **The session's frame.** The session starts on the title's frame 57 (`demo-long`: 20 of 20
  titles) [C]. The demo's race skips its own session (`$83:C9F6-CA05`, R-0087) and plays with
  this set to its end.
- **The reveal's slide.** `$80:9477` (`JSR $E2D3`) starts the title's HDMA reveal with the forward
  slide's sound (`$80:B139`: effect gain 79, effect 2), then its first frame wait (`$80:E322`).
  The session's upload decides the frame:
  - On the first title the slide and wait come on its last blank frame.
  - On every title after it the upload ends a frame later. That is R-0087/R-0088's hold of
    frame 133: the slide and wait come in the hold frame (13 of `demo-long`'s 16 held titles).
  - On 3 of the 16 the slide comes a frame before its wait; on the short titles (R-0087's
    replay) there is no hold [C].
- **The race's start cue plays.** The start's countdown cue (`$82:D84A`, effect 15) runs the
  track's `race_start_cue_leads` frames before the race's initialization. On the menus' path the
  session's queue reset drops it (R-0077); in the demo nothing does [C: 19 of 19 races].
- **The race's fade-out.** From the warning (the demo clock `$1387` at 0x714 or more), every
  update enqueues a music fade (command 3, 0x80) and calls the dispatcher twice
  (`$83:E267-E272`). It runs before the pad test, so a late pad exit's update fades too; the
  timer exit's update does not (`$83:E25D`). Native reports the site as `demo-fade` [L, C].
- **An exit leaves the riders the update's pads.** Both exits branch to `$83:E2BB` (`$83:E25D`
  the timer, `$83:E2A9` a pad) and skip the demo's input writes (`$83:E2C4` on), so the riders
  keep what the pad reader (`$82:AA75-AC5A`) gave them that update: the player pad 1's; the second
  rider pad 2's in a split race, and in a one-view race the released controls `$82:AB6F-AB8B`
  writes when `$0C6D` is set (the opponent AI then runs). The review's `rv-late-split` shows it:
  the exit update's Y releases the computer rider's held rotation, and its sound stops
  (`E 6 43, E 2 12`) on that update. Native's exits kept the demo's last inputs; they now take the
  pads (the race timelines change only on the split races' exit updates) [L, C].
- **The way back.** The title set's session (`$80:A0F7`, as after a menus' race) starts five
  frames after the race's exit; a pad exit's one or two extra blank frames (R-0070) follow it.
  - Its two frame waits during the upload are on the way back's frames 74 and 75. After the
    timer exits from tracks 20 and 24 (R-0087's held returns) they are on frames 75 and 76.
  - Its frame 101, where NMIs come on, runs late and enters its frame wait (`$80:FADF`) there.
    That is one wait more than the frames that turn the arrow (at 247,358 master clocks into
    `demo-cold`'s frame 3449) [C].
  - After `rv-late-split`'s exit the upload runs a frame longer again, on track 1 and a pad exit:
    three upload waits (frames 74-76), no frame-101 wait, and everything after a frame later; the
    next title is then a short one. Moving native's cues after frame 3401 one frame later and
    replaying that title short (the review's `late_split_shift.sh`) leaves only the third upload
    wait (frame 3377). The front end cannot predict it (below) [C].
- **The score's note-space slide.** The demo's score uses control 94, which the native driver
  had not met (`unrecovered score control: 148`). SPC `$0FE2-$0FEE` stores two inline bytes, the
  slide's amount (`$0281`) then its interval (`$0280`); `$0FF1` (control 95) clears the interval;
  `$0FD4` (93) is the release at an absolute count. A note copies the interval into its
  countdown (`$0C80`), and `$0A65-$0AA8` moves the note towards its target by the amount each
  time the countdown runs out. Native already had that arithmetic (`update_note_slide`) with the
  interval always 0; controls 93-95 now set it [L, C: the conditional check below].
- **The app** keeps its cued producer through the idle demos and back to the menus. Its audio
  model's menu idle expiry (`AudioCpuMenuAction::attract`) enters the cued scenes like a menu
  choice. A pack before v39 stops the producer at the demo, and the app reports it
  (`native_audio_cued_stopped=1`; it reported 0 before the review).

## Results

- **Cues** (frame waits included): [C]

| Schedule | Lines | Frames differing | What differs |
|---|---:|---:|---|
| `demo-long` | 103,162 | 6 | three titles' slide a frame before its wait (frames 7155, 10210, 34592) |
| `demo-cold` | 16,158 | 2 | the same at 7155 |
| `demo-pad-early` | 13,194 | 0 | equal |
| `demo-pad-late` | 16,067 | 2 | the next title's slide (7133) |
| `demo-pad-port2` | 17,480 | 4 | the slides at 7155 and 8734 |
| `rv-early-p2` | 15,018 | 2 | a held title's slide (5269) |
| `rv-long` | 171,170 | 12 | six held titles' slides of 27 (7155, 10210, 34592, 77219, 86361, 92426) |
| `rv-late-split` | 15,964 | 183 | the way back's extra frame after the exit (3377 on); the exit update itself equal |

- **Anchors** (D-0010 calibration, `anchors.py` on `demo-long`):
  - The demo session's first FF request: median 123,288 master clocks after its frame's boundary
    (20 sessions, 123,008-123,574).
  - The fade-out's `$83:E26E`: median 30,027 (1,672 calls, 25,590-41,952).
  - The way back's title session keeps the menus' anchor (344,450). Its 19 timer returns have a
    median of 348,212 (346,092-353,292), but a demo-only anchor of that value measured worse
    PCM on `demo-cold` (8,146 against 8,207 windows within 1 dB), so it is not used.
- **Conditional driver check** (`cond.sh demo-cold-full title demo title demo title demo title`):
  the original's CPU port writes drive the native IPL, driver and DSP from zero. All 1,872,341
  ordered DSP and SMP port writes and all 6,406,928 raw stereo pairs are equal, including the
  demo score's slides [C].
- **Anchored PCM** (`measure.sh demo-cold`, D-0010; three demo titles, races and returns): [C]

| Commands in frame | Identical pairs | Windows | Median level difference | 90th / 99th percentile | Within 1 dB |
|---|---|---|---|---|---|
| 721 of 727 (6 a frame late, the fade-outs near two exits) | 982,056 of 6,406,928 | 9,301 | 0.30 dB | 1.09 / 2.29 dB | 88.2% |

  The PCM is identical up to pair 663,424 (frame 1035), just after the first demo title's
  session. The commands that follow its upload (music start, gains, slide) reach the driver
  about 82,000-97,000 master clocks earlier than in the original. That is the upload-length
  variation R-0077 records under D-0010: the native upload ends earlier than the original's.
  After each later session the PCM is identical again for a while (22 of 200 seconds fully identical).
  `twop-next`'s 95.7% (R-0091) is the closest comparison.
- **The app** (`app-debug`, `--native-title-menu-audio`, 13,000 updates of `demo-long`'s inputs):
  12,100 cued frames, the producer never stops. Only the boot's silence is dropped (2,815 pairs,
  peak 0; no later drops). Its cue log, rotation cues resolved, equals `front_end_runner`'s
  through the four demo races it reaches.

## Not covered

- A way back whose upload runs a frame longer after a pad exit (`rv-late-split`, track 1). R-0087
  found the timer exits from tracks 20, 24 and 36 hold every time, and native applies that rule;
  this hold, like the short titles, follows the sound processor's state, which the front end
  does not see. The pad exits of `demo-pad-early`, `-late`, `-port2` and `rv-early-p2` do not hold.
  Native's cues after such a hold are a frame early until the next way back.
- The slide's frame on held titles whose upload ends near the frame boundary (3 of `demo-long`'s
  16, 6 of `rv-long`'s 27): native
  keeps the hold frame for both cues. Which way it falls depends on the sound processor's state
  through the race, which the front end does not see (D-0010's upload variation).
- A pad press during the demo's title: the original leaves at `$80:9487` (`BNE $94D1`), fades,
  reloads the title set (`$80:94D4`, `JSR $A09A`) and returns to the menu. Native's title takes
  no press (R-0087's picture domain), so neither its picture nor its sound is covered.
- Short titles need the laboratory's `--short-demo-title` replay (R-0087). Native does not
  predict them, so a short title's slide comes a frame late in the app.
- Demo cycles past the twentieth title (`demo-long` ends at frame 60,000, track 25), the second
  lap's tracks 26-39 and 0, and a cartridge whose demo counters hold other values.
- PCM measured on `demo-cold` only.
