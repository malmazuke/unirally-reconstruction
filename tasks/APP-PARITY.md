# APP-PARITY - the shipped app plays what the runner plays

- Status: **queued** 8 November 2026 by [COVERAGE-CENSUS](COVERAGE-CENSUS.md) (R-0109, queue item 1).
- Milestone: M4. Review tier: **2** (the app shell and its defaults; a change that reaches `src/core`
  race or front-end state raises it to tier 1).

The lab runner (`front_end_runner`) is checked against the original frame for frame, but the shipped
app (`unirally`) still differs where R-0109's app list says: audio and saves are opt-in
(`--native-title-menu-audio`, `--save-file`), the idle demo's race takes the live pads instead of the
demo's own inputs, a one-player race after a notice is not run, and the app reports no race state per
frame (R-0090, R-0106). Outcome: the app plays with native audio and a default save file unless told
otherwise, the idle demo's race runs as the runner's does, the race after a notice runs, and the app
can write the runner's per-frame race rows so a scripted app run is compared with the runner on rows,
cues and pictures (not only cues and totals).
