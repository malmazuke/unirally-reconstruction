# DEMO-PICTURES - the idle demos' remaining picture differences

## Assignment

- Status: **ready** (queued 9 October 2026 by IDLE-DEMO-ROTATION, from R-0087's picture classes).
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
