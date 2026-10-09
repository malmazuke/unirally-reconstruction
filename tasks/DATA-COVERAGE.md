# DATA-COVERAGE - which data the original reads

## Assignment

- Status: **claimed** 10 October 2026 (about 21:30 UTC, 9 October) on main `33f3645`, by the
  session the user asked to run beside the WIPE-RAM session (R-0089's queue item 9). Weekly usage
  at claim 15%.
- Worker: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`); coordinator, primary and
  integrator. Branch `task/data-coverage` in `.worktrees/data-coverage`.
- Milestone: M4 (original game coverage).
- Review tier: **3** (D-0008: records, laboratory evidence and regenerated maps). If the read
  measure needs a change to the capture tooling, the task escalates to tier 2 and says so here.
- Task provider: Anthropic.
- Dependencies: [COVERAGE-GAPS](COVERAGE-GAPS.md) (R-0089's corpus), the static code map (R-0045).
- Parallel work: runs beside the lane that holds WIPE-RAM and then CREDITS-NAME. Agreed with that
  session on 10 October: this task changes no `src/core`, `src/app` or pack profile; heavy runs
  (captures, sweeps, gates) take `local/locks/heavy-run.sh`; the static map is regenerated only on
  top of a main that includes WIPE-RAM; each session edits only its own rows of the shared records.

## Why now

R-0089 measured the code banks `$80-$83` but not the 60 data banks or the sound program. Data the
original reads and native does not extract is missing content; data nothing reads is either
unused or reached by paths no capture has taken (the 45 RNC entries at `$82:B332` `$C2-$EE`).

## Outcome and boundaries

- Over R-0089's corpus, which bytes of the ROM outside the code banks the original reads, by CPU
  instruction and by DMA, attributed to banks, directory entries and the content pack's logical
  entries.
- The RNC entries: whether any captured path loads them, and what they hold.
- The static code map regenerated from this corpus, with the shrink in its unknown share.
- Records only, unless the measure needs a tooling change (then tier 2, recorded above).
