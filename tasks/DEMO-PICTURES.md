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
