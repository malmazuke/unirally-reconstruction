# DEMO-PICTURES - the idle demos' remaining picture differences

## Assignment

- Status: **accepted** 9 October 2026 (tier 1, [#65](https://github.com/malmazuke/unirally-reconstruction/pull/65)),
  integrated by merge commit. Claimed 9 October 2026 (04:30 UTC) on main `6be4093`, in the session the
  user asked to keep working until weekly usage reaches 50% (5% at claim). Queued the same day by
  IDLE-DEMO-ROTATION, from R-0087's picture classes.
- Worker: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`); coordinator, primary and
  integrator. Branch `task/demo-pictures` in `.worktrees/demo-pictures`.
- Milestone: M4 coverage of the main menu's idle mode.
- Review tier: **1** (raised at the candidate from the queued 2: the title's held frame and the
  return's pending NMI change front-end timing in `src/core`, though the race words stay exact).
- Task provider: Anthropic (the queuing session's).
- Dependencies: IDLE-DEMO-ROTATION (R-0087).

## Outcome and boundaries

`local/evidence/idle-demo-cycles/cold-102000` retains 1,006 pictures over 34 idle cycles; 906 equal
native's (`pictures.py`, with `--short-demo-title 13 16 18 21 33`). Find and fix the classes:

- the idle menu: 17 of 158 pictures, about 23,300 pixels, also on main (frame 7,000);
- the title's wave: 24 of 151, about 8,500 pixels, also on main (frame 4,100); two title starts
  (cycles 10 and 17, frames 28,400 and 49,700) differ entirely;
- the countdown's GO letters out of phase in some races (19,155 pixels on one-view tracks 5, 8, 10,
  13, 23, 25; 18,889 on split tracks 4 and 16, around update 220-275);
- small sprite differences in races (4-107 pixels).

Outside: the race state (exact), audio, the titles the sound processor shortens (R-0087).

## Checkpoint - 9 October 2026 04:50 UTC

- **GO letters.** Dense capture `local/evidence/idle-demo-cycles/go-c4` (cycle 4, track 5, race start
  10,626; every second frame 10,840-10,920) against native: every frame of updates 214-274
  differs by 19,155 pixels (the original shows the GO window's "G" member where native shows the
  "O"), equal from update 276. `$83:E728` picks the GO table on the `$0300` contact phase; native's
  `race_windows.cpp:107` derives it as `(frame - scenario.initialization_frame) & 1`, which assumes
  the accepted races' phase at the boundary. Next: derive `parity_set` from the race state's
  `movement.contact_phase` (exact in every demo race, R-0087), check which phase value is member 3
  against `go-c4` and the accepted ZOOM ZOO/DRAGSTER window gates, then rerun `pictures.py`.
- Then the title wave (24 of 151, also on main), the idle menu (17 of 158, also on main), two title
  starts (cycles 10 and 17) and the small sprite differences.

## Checkpoint - 9 October 2026 05:05 UTC

- **GO fixed** (`race_windows.cpp`): the GO table's choice reads the race state's `$0300`
  (`movement.contact_phase`), equal to the old updates-since-boundary parity from the menus, and
  right in the idle demos, whose races start with whatever phase the demo leaves. `go-c4` now
  equal on frames 10,840-10,900; the lap's pictures go from 906 to 922 of 1,006 equal (every
  19,155/18,889-pixel GO picture). Not yet gated (the window gates and sweeps must stay equal).
- Remaining: idle menu (about 23,300 pixels, 20 pictures, also on main), title wave (about 8,500,
  24, also on main), two title starts (57,057 at cycles 10 and 17, with 5,265 at title frame 203),
  small sprite differences (1-107 pixels), and 1,536 at 92,500 and 768 at 101,600.

## Checkpoint - 9 October 2026 05:25 UTC (uncommitted source change in the worktree)

- **Idle menu class = the palette cycle's phase.** Frame 7,000's 23,287 pixels are the checkered
  background's colours. Comparing `$00C8/$00C9` (delay/phase) of `cold-102000` with
  `front_end_runner`'s per-frame columns 10-11 over every idle period (exit + 100 to the next
  title): at main (hook enabled at return frame 101 on the timer path) native is one frame behind
  on every frame (6525: original 5/3, native 6/3). With the hook at frame 100 (the uncommitted
  edit in `front_end.cpp`'s `demo_return_frame`) every later return matches except its first
  frame (exit + 101: original 0/0, native 6/3; exit + 102: both 5/3), and the first demo's return
  differs for 497 frames from 3,449.
- So the original's first hook step leaves delay 5, not 6 (or runs twice), on the timer return;
  the first return differs again. Next: read the hook (`$80:FA60`, R-0070's trace at 5103-5117 for
  the interrupted return) and model the first step, then check cycle 1's return, the interrupted
  returns' frozen pictures (`local/evidence/attract-demo/interrupt-*`), and rerun `pictures.py`.

## Checkpoint - 9 October 2026 05:45 UTC

- **Idle menu palette fixed.** The original's hook (`$80:FA60`) steps like native's, but on the
  timer's demo return `$00C8/$00C9` go 0/0, then 5/3: the hook runs twice on its first frame (an
  NMI pending when NMIs are enabled). Exceptions: the first demo's return and every cycle whose
  title was short (cycles 1, 13, 16, 18, 21, 33 of `cold-102000`) step once, so the sound
  processor's timing decides both. Native: `PaletteCycle::pending_nmi` on the timer return unless
  it is the first or the cycle's title was short (`demo_title_was_short`, known only from a
  capture's `--short-demo-title`; the product takes every title as long). All 16,432 idle-menu
  frames' `$00C8/$00C9` equal; pictures 939 of 1,006 equal.
- Remaining: the title wave (about 8,500 pixels around title frames 140-200, 27 pictures), two title
  starts (57,057 at cycles 10 and 17, 5,265 at title frame 203), small race sprite differences
  (1-107 pixels), 1,536 at 92,500 and 768 at 101,600.

## Checkpoint - 9 October 2026 06:00 UTC

- **Title wave fixed.** Native's wave band was a picture ahead in every title after the first: the
  extra frame of those titles is a held frame 133 while the sound program loads, before the wave
  starts at 134 (the short titles of R-0087 lose exactly this hold). Native now holds it (unless a
  laboratory replay marks the title short), and the fade (452) and the choice (451) keep the first
  title's script frames. The lap's race words stay exact (34 of 34); pictures 963 of 1,006 equal.
- Remaining: two title starts (57,057 pixels at 28,400 and 49,700: cycles 10 and 17, title frame
  about 3) and small race sprite differences (1-107 pixels, 41 pictures).

## Review candidate

- Records: [R-0088](../docs/research/R-0088-demo-pictures.md). Pictures 963 of 1,006 equal (from
  906); the lap's race words stay exact on 34 of 34 cycles.
- Gates: `local/evidence/idle-demo-cycles/gates-pictures.sh` against main `6be4093`'s binaries
  (`base-6be4093/`). The GO change must leave every window gate and sweep picture equal: from the
  menus `$0300` is the old parity.
- The window pointer's synthetic test (`presentation_tests.cpp`) now drives `$0300` as the engine
  does (alternating every frame, pauses too); it had relied on the frame parity.

## Gates - head `b45e483` (`local/evidence/idle-demo-cycles/gates-pictures-b45e483.out`)

- Four presets build, ctest 41 of 41 each (ASan presets unavailable on this host; Linux CI covers
  them); synthetic suite, v1 contracts, eight hidden app runs and the front end passed. Fuzz as on
  main (40 seeds, recorded non-pass).
- Eleven differential gates passed with main's row digests; race equivalence sweep 432 runs, 0
  differences; R-0076's seven cue schedules identical to main's; every track's original sweeps
  (track-breadth-2, locked tours) give main's rows.
- Front-end sweep 176 of 179 equal: the three `attract-demo` manifests differ in state, as
  expected (they run through a demo return, whose palette step and title hold this task changes).
  Every split, pause and menu capture's pictures equal (menu 670, countdown 280, split 320,
  fade-in 200, zoom lap 2,260, quit 206, restart 271, lap quit 184); R-0069's split demo pictures
  0 differing.
- Idle demo lap: 34 of 34 cycles exact with trailers, nine demo restores reach their exits,
  pictures 963 of 1,006 equal.
- Tooling tests 554 passed; functions over 80 lines 0; native symbols passed; dirty 0 at the end.

## Review - round 1

- Independent review approved
  ([#65](https://github.com/malmazuke/unirally-reconstruction/pull/65#pullrequestreview-5465921329)),
  withholding `hunter-41-right` and `lap2-150000` (49 cycles). Should-fix: the records did not say
  the `$0300` change also corrects HUNTER races, where a skipped update (`$83:CCA2` to `$CDAA`)
  passes over the toggle at `$83:CCE7` (track 41's winner banner, 104 of 104 pictures, main 49).
  Done in R-0088 and the `race_windows.cpp` comment.
- Advisories done: the runner's `--short-demo-title 1` no longer marks the second title (the first
  title has no hold; a 40,000-frame run is byte-identical with and without it); the hold frame is
  `demo_title_hold_frame`; the fade and choice frames are `constexpr`; comments cite R-0088.
- After the fixes (source only, behaviour unchanged for every gate input): lab-release ctest 41 of
  41, lap 34 of 34 exact, pictures 963 of 1,006.
