# APP-PARITY - the shipped app plays what the runner plays

## Assignment

- Status: **claimed** 8 November 2026 03:10 UTC on main `68b1c9b4`, on the user's instruction
  ("Start APP-PARITY"). Queued 8 November 2026 by [COVERAGE-CENSUS](COVERAGE-CENSUS.md) (R-0109,
  queue item 1). Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`): coordinator and
  integrator; a worker subagent implements, a fresh subagent reviews. Run under
  [D-0013](../docs/decisions/D-0013-task-time-and-disk-budget.md): claim to merge against the
  6-hour target, new evidence under 1 GB. Usage at the claim: 5-hour 11%, weekly 14%.
- Branch `task/app-parity` in `.worktrees/app-parity`; evidence in main
  `local/evidence/app-parity/`.
- Task provider: Anthropic.
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

## Handoff

Not started.
