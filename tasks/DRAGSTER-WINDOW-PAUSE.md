# DRAGSTER-WINDOW-PAUSE - Windows through a pause, and the banner's life

## Assignment

- Status: in progress (started 18 September 2026 from `main` at `db042ef`, from the user's live play on the integrated CLASSIC-PRESENTATION-UNIFICATION build)
- Milestone: follow-up to CLASSIC-PRESENTATION-UNIFICATION and DRAGSTER-WINDOW-EFFECTS
- Coordinator: main session
- Task provider: Anthropic (unchanged)
- Worker/session/runtime/model: Claude Code, Claude Fable 5.1
- Base commit: `main` at `db042ef`
- Branch and isolated worktree: `task/dragster-window-pause`, `.worktrees/dragster-window-pause`
- Reviewer: fresh independent subagent in an isolated checkout at the exact candidate

## Why

Playing DRAGSTER on `b50dd83` the user found that pausing during the countdown
does not stop the countdown digits, although the underlying countdown does
pause. The user also asked whether the winner banner playing from the
opponent's finish, while the player is still racing, is what the original
does. The second is established: R-0040's opponent-won capture shows the
original's pointer cycling the banner from two frames after the opponent's
finish while the player rides on, and native equals it frame for frame.

The first is a real defect with a specific cause. The window selection was
computed from the frame number (`frame - 1334` for the countdown, the frame
parity for GO and the banner steps), and the frame keeps advancing through a
pause: the original's NMI runs through a pause, so the palette cycle is
right to follow the frame, but the countdown and banner drivers are race
logic that the pause menu diverts.

## Recovered

Two originals captured with the user's ROM (`dragster_playable_reference`,
twice each, byte-identical repeats), under ignored
`artifacts/window-pause/`:

- `countdown-pause`: continuous Right with the pause menu opened on 1400
  and resumed on 1501, and again 1561 to 1601 during GO. Player won at 3359.
- `banner-pause`: the release-3000 race (opponent finishes first, 3214) with
  the pause menu opened on 3240 and resumed on 3301 during the banner; the
  player finished at 3411.

Read from each capture's WRAM series: the pointer `$11FD` the vblank setup
`$80:868E` publishes (frame n shows the value at the end of frame n-1), and
the shared engine's own rows for the same timelines (identical on every
frame of both captures, including the pauses). Findings:

1. **A pause disables the window rather than holding it.** The channel is
   off from the update that opens the menu through the update that resumes
   (no window on 1401-1502 for a pause opened on 1400 and resumed on 1501),
   and the drivers choose again from the first update after that.
2. **The countdown digits follow `$11C5` as the update read it**, before the
   update's own decrement; the state's `movement.countdown` after update
   n-1 plus one is what the driver of update n-1 read. A paused update
   neither decrements nor selects.
3. **GO alternates on the update's `$0300` parity, which is the frame
   parity even across a pause** (the clock is sampled before the menu
   diverts the update, R-0038): an odd driver update chooses member 3,
   shown on the even frame after it.
4. **Two drivers, one per rider, each with a 360-update life.** Each
   rider's finish arms that rider's driver (`$0F03`/`$0F07` and
   `$0F05`/`$0F09`); it first runs on the next race update the menu does not
   divert, and the earliest-armed live driver owns `$11FD`. Per run: with
   its index still zero the life is set to 360; a life of zero stops the
   driver for good and the next armed one runs in the same update; otherwise
   the life counts down once per driver update of either parity and, on an
   odd frame, the index steps 8..24 then 7..24 (member 7 for index zero).
   So the banner ends 360 driver updates after the driver's first odd
   update: random-1 (opponent 3214, first driver update 3215 odd) is blank
   from 3576, reversal (opponent 3325, first driver update 3326 even) from
   3688; a second finish restarts nothing (banner-pause: `$0F09` counts on
   through the player's finish at 3411); the other rider's driver starts at
   index zero once the first stops. This was the reviewer's reading, from
   the counters themselves and from a capture with an odd number of diverted
   updates (banner-pause-odd, 61): without a pause 360 updates are ten
   member cycles, so my first reading, "180 odd steps from the latest finish
   with the phase continuing", fitted six captures and failed that one on
   15 frames (member 8 where the original steps back to 7 on 3637).

## Implementation

- `ClassicWindowPointer` (`presentation.hpp`) keeps the channel-6 pointer
  the way the original does, from the shared race state alone and without a
  pack: the two drivers above, the countdown driver
  (`classic_countdown_window`, from the countdown word before the update and
  the update's parity), the vblank publishing the previous update's choice
  from initialization + 6 until result-loading update 1, and a diverted
  update publishing nothing. `ClassicRaceHistoryTracker` composes it;
  `ClassicRaceHistory` carries `window_published` and `window_table`, and
  the renderer draws that member when the history has been observed, as the
  app and the runner's `--timeline` mode do.
- The frame-based `classic_window_table_index` remains for a single restored
  state (exact when no pause diverted a race update since the first finish)
  and follows the same two-driver rule from the first and the latest finish.
  `classic_opponent_finish_frame` accepts either finish order. The legacy
  `dragster_window_table_index` uses the same helper with its single finish;
  the frozen v1 frames and the release-3213 sweep are unchanged.
- `classic_race_presentation_runner --window-index` prints the published
  member beside the two frame-based selections.

## Measurements

`artifacts/window-pause/verify_pause.py`, the tracker's published member
against the original's `$11FD` on every frame from initialization + 6 to
loading update 1:

| capture | frames | published (`ClassicWindowPointer`) |
| --- | --- | --- |
| countdown-pause | 2,267 | 2,267 |
| banner-pause | 2,319 | 2,319 |
| banner-pause-odd (the reviewer's case, regenerated) | 2,318 | 2,318 |
| continuous-right (the reviewer's case, regenerated) | 2,121 | 2,121 |
| primary-a | 2,119 | 2,119 |
| random-1-a | 2,544 | 2,544 |
| reversal-a | 3,200 | 3,200 |

The frame-based fallback (the runner's other two columns) disagrees only
where it cannot know: through a pause, and before the player has finished
when the opponent finished first (no history, no second finish time).

## Gates

`artifacts/window-pause/gates.sh` on the candidate's tree (the tracker fix
for a second finish on the driver's starting update was re-measured after it,
below):

| Gate | Result |
| --- | --- |
| five presets ctest | 23/23 each |
| `test --suite synthetic` (lab-debug) | passed |
| v1 contracts, v1 pack | winner 36/697/279/445/653/962/961, loser 1,073 (unchanged) |
| stage_c A (frozen contract frames, unified) | 36/358/279/322/777/962/961 (unchanged) |
| stage_c B (release-3213, 132 frames) | 680,807 rectangle mismatch, 114 better / 4 worse (unchanged; a first draft published no banner on 3215 because the second rider finished on the update the driver starts on, which the timeline-driven frame 3215 caught: 3,314 against 504) |
| stage_c C (ten M4-16 ZOOM ZOO scenes) | pixel-identical to the before-change renderer |
| hidden app runs, both tracks, 4,000 updates | 0 rider-pose fallback frames |
| `dragster_fuzz_runner` (lab-release), 40 seeds | 79 completed races, 1,242 pause restarts, 7,696 renders, 0 aborts |
| capture agreement (table above) | 100% on all five captures after the fix |
| hosted CI | the first push `5b21f38` failed on Ubuntu GCC (`-Werror=range-loop-construct` on two test loops), fixed with the second commit |

## Mistakes

- The first tracker draft treated a second finish that lands on the very
  update the driver would start on as "not started": no capture covers it,
  but the release-3213 picture measurement does (frame 3215), and the
  frame-based fallback already had it right. The pictures caught what the
  five index-level captures could not.
- The structured-binding loops copied on GCC (`-Werror=range-loop-construct`),
  which the local Clang build accepts; the project's memory note about
  dispatching CI before integration exists for this and was still needed.

## Handoff

- Candidate: see `git log` on `task/dragster-window-pause`. Review: fresh
  independent reviewer in an isolated checkout; then integration, final-tip
  CI and the closeout under ignored `artifacts/window-pause-integration/`.
- Not done: ZOOM ZOO's own windows (still no family bound; the same tracker
  would drive them once its members are captured).
