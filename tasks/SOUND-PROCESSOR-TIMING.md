# SOUND-PROCESSOR-TIMING - the front end's waits that follow the sound processor

- Status: **queued** 8 November 2026 by [COVERAGE-CENSUS](COVERAGE-CENSUS.md) (R-0109, queue item 5).
- Milestone: M4. Review tier: **1** (frame timing of the front end and races).

The original's CPU waits on the sound processor at uploads and handshakes, so some frames depend on
the driver's state; native uses measured lengths. The records list the cases: the warm power-on's
handshakes, short titles and the two long title starts, held titles' slides, the longer way back
after a demo pad exit, the podium's way back a frame late in about a third of podiums and its music a
pass off in 2 of 8, a race's loading varying by a frame (R-0077, R-0087, R-0088, R-0090, R-0096,
R-0098, R-0099, R-0102). Native now runs its own driver (D-0009) whose port writes equal the
original's. Outcome: the front end takes these waits from the native driver's state instead of
measured lengths, and every listed case equals the original frame for frame, or is recorded with the
reason it cannot.
