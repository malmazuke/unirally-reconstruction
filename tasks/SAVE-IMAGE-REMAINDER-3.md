# SAVE-IMAGE-REMAINDER-3 - Sound sessions that move a front-end frame

## Assignment

- Status: **queued** 13 November 2026 by [SAVE-IMAGE-REMAINDER-2](SAVE-IMAGE-REMAINDER-2.md)
  ([R-0119](../docs/research/R-0119-save-image-remainder-2.md), items 4 and 5). Not claimed.
- Milestone: M4. Review tier: **1**. It would change when a race and a menu loop start, and a
  record a later boot reads. It also needs a decision record.

[R-0119](../docs/research/R-0119-save-image-remainder-2.md) leaves two items open. In both, a sound
session's upload ends late enough in its frame to move the next screen a frame, and native's front
end does not see it.

1. **The race start on HUNTER's preloaded tours** (R-0119 item 5, R-0117 item 1).
   - The cause is found. The warm boot's skipped defaults work changes the sound processor's phase.
     The upload's polls then end 2,256 master clocks before the frame's end, and the race setup's
     5,214-clock tail misses the frame wait.
   - Native's own sound model (`race_audio_runner` with the run's image) predicts the original's
     race start on all 527 track_reference runs (`check_audio_starts.sh`). R-0077's table, which
     the front end uses (D-0010), is right on 484.
   - In play the difference reaches saved records a later boot reads:
     - the personal bests on `c175`, `c177`, `c210`, `c220` (`$085F`/`$085B`);
     - `$1116` on `c209`, `c217`, `c218`, `c272`.
   - **The work:**
     - decide, in a decision record amending D-0010, whether the front end's race start may
       follow the sound model's session end;
     - if so, run the title/menu CPU model and the sound processor beside the front end in the
       app (with `--no-audio` too) and in `front_end_runner` (measured: about 1.8 s of CPU per
       1,400 frames);
     - take the start as the frame after the polls' frame, plus one when fewer than 5,214 clocks
       are left.
   - The rule's tail is measured on one run (`c175`). Measure it on a few tracks before relying on
     it.
2. **The league's continue screen a frame late** (R-0119 item 4).
   - On `c029`, `c032`, `g069`, `c100` and `c101` the original's title-return session after the
     podium (`$80:A0F7`) runs about 31,000 master clocks longer than on the other 11 corpus routes
     that reach the screen. Everything after it moves a frame, among it the hidden markers' step
     `$10A7` for the rest of the run.
   - Native's sound model, fed native's cues, gives the same session end on `c032` and `g066`,
     whose original sessions differ.
   - **The work:** find what differs before the request (the CPU work around it, or the sound
     processor's state after the anchored scenes), with the original's port writes captured on
     `c032` and `g066`.

Evidence and tools: main `local/evidence/save-image-remainder-2/` (`audio_starts.py`,
`audio-starts.json`, `ends.py`, `vtrace.py`, `traces/`).

Outcome: the race start and the continue screen's frame equal to the original's on the corpus by a
native model, or a recorded decision why not.

## Handoff

Not started.
