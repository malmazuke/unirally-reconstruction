# PICTURE-RESIDUALS-REMAINDER - the two split-race picture residuals R-0116 leaves

- Status: **queued** 12 November 2026 by [PICTURE-RESIDUALS](PICTURE-RESIDUALS.md) ([R-0116](../docs/research/R-0116-picture-residuals.md)).
- Milestone: M4. Review tier: **2** (presentation).

R-0116 makes every compared race picture of the idle demos, `zzap`, the two-human captures and the
league windows equal but two kinds, both in two-view races:

- **Objects across the split** (R-0116 finding 6): one moment of a tour's first race (`tours-a-w2`
  70040, `odd-tours-w0` 11910, `five-w1` 95090) shows 8 pixels of the upper view's opponent (`$1517` x
  -13, y 0x64) at x 0-2 of lines 113-115. In `zzap` 218 frames have a lower-view object reaching above
  line 112 and 108 an upper-view one below it, and native's clip at line 112 matches all of them, so
  the original hides objects across the split by some means not yet read: find it (the split's HDMA
  channels, OBSEL, the OBJ window or the main screen per line around line 112; a per-line register
  record of one such frame, e.g. by tracing `$2101`/`$2123`-`$212F` writes and the HDMA tables) and
  why the left edge escapes it.
- **Line 113 under wobble mode's mosaic** (R-0103, R-0116 finding 7): `demo-pic` 9 pictures and `vs-4`
  5, one row each. BG1 is one 32-by-32 map of 16-pixel tiles shared by both views (word 0x1C00); the
  block from line 108 makes line 113 sample the row above the lower view. Confirm what that map row
  holds (VRAM at the picture's own time, with the line's scroll from the wobble table) and, if it is
  an older row, model BG1's map streaming for two views (`$81:B31C`'s gather, R-0009) or show it
  cannot be done from native's state.
- **Two-view NEON in VS** (R-0116 finding 5, listing and ROM only): native takes colours 0-95 from the
  set of sceneries 1, 3, 4, 7, 8 and 10 (`$83:9E1E` = 10 with X = 0xC20C), not scenery 12's own; no
  capture shows a VS race on track 42. Capture one (VS, NEON chosen) and compare its race pictures.
- Not a picture residual, seen on the way: `organic-cycle-exit` (LEAGUE) 14400 and 15000 differ in the
  whole picture on main too: native's second race starts later than the original's; find where the
  route parts.
- Tools: main `local/evidence/picture-residuals/` (`ppustate.py` reads VRAM, CGRAM and the BG registers
  from a probe's Strict state; `race_pictures.py`, `pictures.py`; `check.sh line-113` pins the 14).
