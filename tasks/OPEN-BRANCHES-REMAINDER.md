# OPEN-BRANCHES-REMAINDER - the branches and differences OPEN-BRANCHES left open

- Status: **queued** 10 November 2026 by [OPEN-BRANCHES](OPEN-BRANCHES.md) ([R-0113](../docs/research/R-0113-open-branches.md)).
- Milestone: M4. Review tier: **1** (race and front-end state).

What R-0113 leaves open, each to be captured on the original and compared with native, or argued
unreachable: the rider sprite-tile queue's ends (whether the two riders share one queue past 45 tiles,
whether a clipped or hidden rider queues fewer than 16; R-0113 reads one rider's bound as 30), a rider
pose of 4096 or more (the override tables `$82:962D` and `$82:A45E` to bound), the loop descriptor facing
away (R-0108), a stunt score of 1000 and track 13's cell past its flag table (R-0107), and the sounds of
the cheat page's reveal, the slow-checkpoint clear and BRONSEN's praise (R-0076, R-0077); and the
differences R-0113 observed but did not trace: the result screen 2 frames later in the original after a
race ends at the 9:59.9 clock limit (`clock-warning`), the DEFINE PLAYER confirm frame's arrow one step
late after a league removal (608 pixels, likely CPU time), league-three's pictures from 15900 (1-2% of
pixels), the idle demo's way back after its 32nd race (frames 98346-98351), and `$83:9C47`'s reaches past
a slot, implemented from the listing and never run by a capture.
