# DEMO-AUDIO - the idle demos' sound

## Assignment

- Status: **in review** (tier 1). Claimed 11 October 2026 on main `2c42589` by the session that ran DATA-COVERAGE and
  EVIDENCE-RETENTION (Claude Code desktop app, Claude Opus 5.5 `claude-opus-5-5`; coordinator and
  integrator; a worker subagent implements). Queued 10 October by [MODE-AUDIO](MODE-AUDIO.md)
  (R-0091). Weekly usage at claim 42%.
- Branch `task/demo-audio` in `.worktrees/demo-audio`; evidence in main `local/evidence/demo-audio/`.
  Research record R-0096; pack profile v39 (agreed with the TWO-HUMAN-STUNTS session, which takes v38).
- Parallel work (agreed 11 October): TWO-HUMAN-STUNTS changes no audio file; its `content_pack.cpp`
  change will need a small rebase. Heavy runs take `local/locks/heavy-run.sh`.
- Milestone: M4 (original game coverage).
- Review tier: **1** (the demo's sound cues and the app's audio producer, D-0010).
- Dependencies: MODE-AUDIO (R-0091), IDLE-DEMO-ROTATION (R-0087), AUDIO-ONE-PLAYER (R-0077).

## Why

Native's idle demos report no sound cues, and the app stops its native audio at the first demo
for the rest of the session. The original reloads the title sound set at each demo's title,
plays a slide, and runs the race's dispatches and effects with that set (R-0091's `demo-cold`
schedule: 5,712 frames differ).

## Input from DATA-COVERAGE (R-0093)

The demo's sound set reads ROM data pack v36 does not extract: samples 02 (`$90:8C41`, 2,425
bytes), 23 (`$92:9504`, 2,020) and 34 (`$92:DC80`, 2,911), through `$82:82D4`/`$82:82F0`, and the
block at `$93:F712` (2,899 bytes). Only the corpus runs that reach an idle demo load them.

## Outcome

- The demo's title sessions (anchored as D-0010 asks), its screens' sounds and its races' cues
  equal to the original's over `demo-cold` and further demo cycles (split and one-view races,
  both view toggles, a pad exit).
- The app's producer continues through the demos and back to the menus.

## Attempts

| # | Hypothesis | Experiment | Result | Next |
| --- | --- | --- | --- | --- |
| 1 | The demo's title reloads the title set (R-0091's "L title") | The listing's `JSL $82:807E` sites | No: `$80:9440-9455` loads driver 50, tables 52 (the award's) and score 56 with the slots at `$83:FC35`; R-0091's label came from the next score site its captures watched (`$80:A10B`, the way back's) | A v39 set; watch `$80:9452` |
| 2 | Score 56 is the unpacked block at `$93:F712` (R-0093) | Resource directory | Yes: resource 56 at `$93:F70C`, data from `$93:F712`, 2,899 bytes; its slots add samples 02, 23, 34 | Pack v39 |
| 3 | The native driver plays the demo's score as it is | `race_audio_runner` on native demo cues | No: "unrecovered score control: 148". Control 94 (SPC `$0FE2-$0FEE`) sets the note-space slide (`$0281` amount, `$0280` interval) that `$0C80` and `$0A65-$0AA8` run; native had the slide arithmetic but no control setting it | Controls 93-95 native; the conditional check on a full capture |
| 4 | The remaining differences on `demo-cold` are the demo's own cues | Native cues against R-0091's `demo-cold` capture, relabelled | After the set, slide, start cue and fade-out: the way back's frame 101 wait (timer exits) and the third title's slide (a frame earlier than its wait) | The wait native; the slide's frame measured over more cycles |
| 5 | Pad exits keep the way back's timing | `demo-pad-early`, `-late`, `-port2` | On these three: the session is five frames after the exit, before the pad exit's blank frames; frame 101's wait is in every way back. Not always: review round 1's `rv-late-split` way back held a frame (attempt 9) | Session by frames since the exit (`demo_return_blank`) |
| 6 | Held titles and returns keep the first ones' cue frames | `demo-long` (19 races) | The held returns' two upload waits come a frame later; 13 of 16 held titles play the slide and wait in the hold frame, 3 play the slide a frame before | The returns native; the 3 titles recorded (D-0010's upload variation) |
| 7 | A demo-only way-back anchor improves PCM | 348,212 against 344,450 on `demo-cold` | No: 8,146 against 8,207 windows within 1 dB | Keep the menus' anchor |
| 8 | A pad exit leaves the demo's last inputs held (review finding 1) | The listing (`$83:E25D`, `$83:E2A9` both to `$83:E2BB`; the pad reader `$82:AA75-AC5A`, `$82:AB6F-AB8B` for a one-view race) and `rv-late-split` | Yes: either exit skips the demo's input writes; the riders take that update's pads (rider 2 pad 2 in a split race, released in a one-view one). Native now does; frame 3300's rotation stop equal, the race timelines differ from main only on split races' exit updates | Done |
| 9 | Fixing 8 makes `rv-late-split`'s held way back predictable (review finding 2) | Native cues after the fix; the review's `late_split_shift.sh` | No: the exit update is now equal, but the way back's upload still runs a frame longer (three upload waits, no frame-101 wait) and the next title is short; shifting native's cues a frame from 3402 and replaying the title short leaves only the third wait (3377). Nothing in the game's state that the front end sees predicts it | Recorded in R-0096's "Not covered" |

## Decisions

- The demo set is its own sound set id (`demo`) and session (`AudioSessionLoad::demo`, cue log
  "demo"); its tables are the award's entries, not a copy (pack v39 adds only the score, the slot
  table and three samples).
- Held titles put the slide in the hold frame, the more common case (13 of 16); the other 3
  stay a recorded residual, since the front end does not see the sound processor's timing.
- The way back keeps the menus' title-session anchor (attempt 7).
- A pad press during the demo's title stays out of scope: native's title has no pad exit
  (R-0087's picture domain), recorded in R-0096.
- Captures: two stripped captures per schedule (CPU rows only) and one full 10,000-frame
  capture, gzipped after the conditional check and PCM measurement read it.

## Results

- Records: [R-0096](../docs/research/R-0096-demo-audio.md).
- Cues against the original, frame waits included: `demo-long` 103,162 lines, 6 frames differ;
  `demo-cold` 16,158 lines, 2; `demo-pad-early` 13,194, equal; `demo-pad-late` 16,067, 2;
  `demo-pad-port2` 17,480, 4. Every differing frame is a held title's slide one frame before
  its wait in the original.
- Conditional driver check on `demo-cold-full` (title, demo and title sessions three times):
  1,872,341 ordered DSP/SMP writes and 6,406,928 raw pairs equal.
- Anchored PCM on `demo-cold`: 721 of 727 commands in frame (6 a frame late), 88.2% of 9,301
  windows within 1 dB, median 0.30 dB, 90th/99th percentile 1.09/2.29 dB.
- The app with native audio through four demo races: 12,100 cued frames, never stopped, no drops
  after the boot's silence.

## Evidence

Main `local/evidence/demo-audio/`: schedules (`*.script.json`, `*.inputs.txt`,
`*.frontend-inputs.txt`), stripped captures `*-disp1`/`*-disp2` with their command records,
derived cues `*.cues.txt`, `demo-cold-full` (events gzipped, `SHA256SUMS`), `cond-demo-cold-full.json`,
`anchored-demo-cold.json`, `pcm-demo-cold.json`, `anchors*.json`, the scripts (`captures.sh`,
`capture_full.sh`, `capture_batch.sh`, `derive_cues2.py`, `cues.sh`, `diff_cues.py`, `cond.sh`,
`measure.sh`, `anchors.py`, `resolve_app_cues.py`), `sets/` (the demo set's sound RAM images),
`base-2c42589/` (main's binaries for the gates) and `gates.sh`.

## Review - round 1 (returned)

- [Review](https://github.com/malmazuke/unirally-reconstruction/pull/77#pullrequestreview-5482336818)
  on `5b6ab8d`: return. Captures and scripts in main `local/evidence/demo-audio-review/`.
- Finding 1, a pad exit drops a race sound (`race_demo_controls.cpp`): confirmed from the listing
  and `rv-late-split` (attempt 8). Fixed: `exit_demo` releases the second rider and the race
  update gives it pad 2 in a split race and the player pad 1's A and X. `rv-late-split`'s frame
  3300 is equal.
- Finding 2, the held way back after that exit: not predictable from the front end (attempt 9).
  Recorded in R-0096; attempt 5's claim corrected.
- Finding 3, `demo_sound.cpp`'s comment: it now says script frame 57.
- Finding 4, `front_end.hpp`: formatted.
- Optional: control 93 has a unit test; with a pack before v39 the app now reports
  `native_audio_cued_stopped=1` (checked: 0 cued frames, stopped 1).

## Gates - head `a5c496c` (main `local/evidence/demo-audio/gates-a5c496c.out`)

Run under `local/locks/heavy-run.sh` with `gates_and_synthetic.sh`, against main `2c42589`'s
binaries (`base-2c42589`, built from `git archive 2c42589`):
- Four presets build, ctest 41 of 41 each (ASan presets unavailable on this host; Linux CI covers
  them). Synthetic suite passed on app-debug and on lab-debug. v1 contracts, eight hidden app runs
  and the front end passed. Fuzz as on main (40 seeds, 40 aborts: recorded non-pass).
- Eleven differential gates passed with main's row digests. Race sweep 432 runs, 0 differences;
  front-end sweep 179 of 179 equal to main's runner. Every pause, two-pad, race-end, look,
  restart and restore comparison as recorded; the original's track sweeps give main's rows.
- The idle demo lap 34 of 34 exact, 9 demo restores continue, pictures 963 of 1,006 equal (as
  the other recent gates). CREDITS-NAME's and WIPE-RAM's captures as recorded, their schedules
  equal.
- DEMO-AUDIO: the cue results above; the conditional check equal; the app's producer 12,100
  cued frames, never stopped, and its cue log equals `front_end_runner`'s through frame 12,999.
- MODE-AUDIO: `options-rename` and `options-league` equal; `twop-next` 2, `vs-champions` 4 and
  `league-two` 2 differing frames (R-0091's residuals). Twelve R-0077 schedules equal; R-0076's
  seven identical to main; the cold cartridge image equal.
- Tooling tests 564 passed; functions over 80 lines 0; native symbols current.

## Handoff

- Commits: `c61cb41` (native change), `a5c496c` (way back, anchors, R-0096, maps), then this
  record. Working tree clean.
- Shared files with TWO-HUMAN-STUNTS, kept small: `front_end.cpp` (include `race_sound.hpp`; one
  call each in `demo_title_frame` and `demo_return_frame`; the start cue's three lines beside the
  race switch; the hold frame's call; two lines in `ends_with_queue_wait`; one in
  `begin_demo_return`), `front_end.hpp` (`demo_return_blank`), `front_end_screens.hpp` (the
  `demo_sound.cpp` declarations). `content_pack.cpp`: the v39 profile, rules digest, variant and
  `demo_audio_required`. On rebase onto v38, insert v38 before v39 (variant order, `>=` on
  credits_name/v38), recompute the manifest's digest in `demo_audio_rules_sha`, set
  `PREVIOUS_TRACK_PROFILE` to v38, update `V37_ENTRIES`/the v37 inventory digest in
  `test_tracks.py`, rebuild `local/classic-pal-crawler-tracks-v39.pack` (the local one is v37 plus
  the demo entries) and rerun the cue schedules.
- The static map and native symbols are regenerated at `a5c496c`; regenerate after the rebase.
- Next: the demo title's pad exit (picture and sound) and a model of the short titles, if wanted.
