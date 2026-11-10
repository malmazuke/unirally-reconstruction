# SOUND-PROCESSOR-TIMING-REMAINDER - what SOUND-PROCESSOR-TIMING left open

- Status: **queued** 10 November 2026 by [SOUND-PROCESSOR-TIMING](SOUND-PROCESSOR-TIMING.md) ([R-0114](../docs/research/R-0114-sound-processor-timing.md)).
- Milestone: M4. Review tier: **1** (audio timing).

R-0114 shows that a session's wait is the running driver's stop sequence (SPC `$0487-04B9`: the master
volume lowered by 8 on each timer-2 pulse), that native's driver reproduces it to the clock while its
inputs are exact, and that after a race the fade commands' D-0010 anchors put native's reply wait up to
about 95,000 master clocks off (the absolute reply, with the requests' anchor errors, -96,272 to
+233,772), so the short titles, held titles' slides, `rv-late-split`'s way back, the
podium's way back and music pass and a race's loading a frame off stay on measured lengths. Open:

- **The idle demo title session's interrupts.** The cued audio model runs the demo session with NMIs
  off, 78,108-92,202 clocks shorter than the original (R-0114 finding 4; the title interrupt `$00:8587`,
  `$80:F622`, `$80:FA60`, `$80:B0F0` every frame). Model it in `audio_cued_scenes.cpp` with the palette
  cycle's phase the front end has at the session (`$00C8/$00C9`), and trace the award's, endings' and
  podium's sessions for the same. Measured gain: the commands after the first demo title's session
  reach the driver at the original's clock (R-0096's 82,000-97,000 clocks early).
- **The waits after a race.** Exact only with the race's fade commands at their exact clocks, which
  needs the race's CPU work (CPU-CYCLE-TIMING's model, or D-0010 revisited). Until then, re-measure
  with `sweep.sh` and `summary.py` whenever anchors or the driver change: a predictor goes into the
  front end only if it gives the original's frames in more sessions than the measured lengths.
- **The warm runs' remaining differences**: league-two-events frame 6,098 (787 pixels, all three
  images) and the all-gold image's second race (R-0077's race loading) are not traced; the runner's
  missing picture on a race's first frame (cold and warm) is a runner output gap.
