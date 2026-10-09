# DEMO-PICTURES - the idle demos' remaining picture differences

## Assignment

- Status: **in progress**, claimed 9 October 2026 (04:30 UTC) on main `6be4093`, in the session the
  user asked to keep working until weekly usage reaches 50% (5% at claim). Queued the same day by
  IDLE-DEMO-ROTATION, from R-0087's picture classes.
- Worker: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`); coordinator, primary and
  integrator. Branch `task/demo-pictures` in `.worktrees/demo-pictures`.
- Milestone: M4 coverage of the main menu's idle mode.
- Review tier: **2** (presentation on recovered state; the race words and timing are exact).
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
