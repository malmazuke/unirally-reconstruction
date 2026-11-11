# CPU-CYCLE-TIMING-REMAINDER - the prints CPU-CYCLE-TIMING measured but did not model

- Status: **queued** 11 November 2026 by [CPU-CYCLE-TIMING](CPU-CYCLE-TIMING.md) ([R-0115](../docs/research/R-0115-cpu-cycle-timing.md)).
- Milestone: M4. Review tier: **2** (presentation: work RAM at a frame's end, no picture differs).

R-0115 places TRACK RECORDS' grid prints at their frame's end with a cost model (15 of 16 traced
prints), shows that a menu frame's work starts with a spread of about 500 master clocks (where vblank
finds the frame wait's sound poll), and shows that the multiply hit by HDMA (`$81:B67C`) cannot be
placed by any model of native's state: it needs every frame's exact clock from the race's start. Open,
each without a picture difference:

- **HIGH SCORES' leaders** (`$80:D7CD-D905`): the text and OAM at the end of the choice frame, six traced
  prints (`rs-track` 2350, 2650; `rs-high` 1050, 2500; `rv-records` 1700; `wr-stats` 1050), the
  original's last store 472 to 2,750 clocks before the frame's end and the next 1,406 to 3,910 after.
  A model needs the leaders' loops (R-0102's `$83:9D80` costs, `$80:9719`'s divisions) and the
  printer's costs per code of the stream (letters, numbers, rider names, object places); the 472-clock
  margins sit near the start's spread, so expect some prints to stay knife edges.
- **VS CHAMPIONS' header frame** (`$80:C786`): text and OAM at the first frame's end, `rs-ca-vs-stunt`
  5343 (margins 956 / 1,138) and `rs-ca-vs-lap` 9302 (124 before the end: a knife edge). R-0102's
  records and checksum costs give the header's start to about 500 clocks.
- **W2b 15672** (TWO-HUMAN-STUNTS review, the rename keyboard, 256 px): RECORD-SCREENS' `compare.py`
  finds every menu field of `w2b-vs-jumps` different from 13,024 on, with main's runner as well (the
  second race returns at 12,923 in native), so that frame cannot be judged until the lag is
  explained; find where the race's return parts from the original's first.
- Tools: `local/evidence/cpu-cycle-timing/trace_frames.py` and `print_cut.py` give any frame's
  instructions and the printer's stores around its end; `grid_prints.py` and `grid_model.py` are the
  grid's pattern to follow.
