# CPU-CYCLE-TIMING - the race and menu effects that depend on CPU cycles

- Status: **queued** 8 November 2026 by [COVERAGE-CENSUS](COVERAGE-CENSUS.md) (R-0109, queue item 6).
- Milestone: M4. Review tier: **1** (race arithmetic and frame timing).

Two classes of difference need the CPU's cycle count, not just its instructions: the multiply helper
`$81:B67C` hit mid-way by an HDMA write sharing the mode-7 latch (DOWNER in two views, R-0095; track
41's landing against ANTI-UNI, R-0098, R-0102), and prints that run past their frame or sit on a
knife edge (R-0095, R-0100, R-0102). Outcome: a cycle-cost model of the race update and of the
front end's prints good enough to place each listed case, checked on the original's captures, with
native's results equal on those routes.
