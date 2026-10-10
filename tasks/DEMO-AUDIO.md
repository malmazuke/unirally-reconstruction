# DEMO-AUDIO - the idle demos' sound

## Assignment

- Status: **claimed** 11 October 2026 on main `2c42589` by the session that ran DATA-COVERAGE and
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
| 5 | Pad exits keep the way back's timing | `demo-pad-early`, `-late`, `-port2` | The session is five frames after the exit, before the pad exit's blank frames; frame 101's wait is in every way back | Session by frames since the exit (`demo_return_blank`) |
| 6 | Held titles and returns keep the first ones' cue frames | `demo-long` (19 races) | The held returns' two upload waits come a frame later; 13 of 16 held titles play the slide and wait in the hold frame, 3 play the slide a frame before | The returns native; the 3 titles recorded (D-0010's upload variation) |
| 7 | A demo-only way-back anchor improves PCM | 348,212 against 344,450 on `demo-cold` | No: 8,146 against 8,207 windows within 1 dB | Keep the menus' anchor |

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
