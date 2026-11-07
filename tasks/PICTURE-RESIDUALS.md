# PICTURE-RESIDUALS - the picture differences the records leave, by system

- Status: **queued** 8 November 2026 by [COVERAGE-CENSUS](COVERAGE-CENSUS.md) (R-0109, queue item 7).
- Milestone: M4. Review tier: **2** (presentation).

The records list pictures that still differ from the original after every task: split pictures where
a rider passes under the HUD, track 42's two-view NEON, line 113 under a mosaic block, 41 small sprite
pictures in demo races, 470 pixels in the GO letters' area, NOW PLAYING's arrow and icon animation,
the result icons and three league pictures (R-0046, R-0071, R-0073, R-0083, R-0084, R-0088, R-0098,
R-0103). Outcome: each traced to its cause in the original's PPU state and fixed in native, or
recorded with the reason it cannot be (a hardware effect native does not model), with every
compared picture of the routes involved equal.
