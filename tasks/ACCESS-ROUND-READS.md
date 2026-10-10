# ACCESS-ROUND-READS - a discarded resolution round's ROM reads

## Assignment

- Status: **claimed** (tier 2). Claimed 10 October 2026 08:12 UTC at the user's request, on
  `task/data-coverage` `fefe661` (PR #71, DATA-COVERAGE, open and approved at claim). The branch
  moves onto main once #71 merges. Weekly usage at claim 31% (5-hour window 66%).
- Worker: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`); coordinator, primary and
  integrator. Branch `task/access-round-reads` in `.claude/worktrees/competent-albattani-fce0d6`.
- Milestone: M4 (original game coverage), laboratory tooling.
- Review tier: **2** (tooling): the change is in `tools/unirally_lab/access/derive.py` and its
  tests; no `src/core`, `src/app`, pack, gate or baseline change. One independent round.
- Task provider: Anthropic.
- Dependencies: [DATA-COVERAGE](DATA-COVERAGE.md) (`--resolve-rmw`, R-0093 and its corpus scripts).
- Parallel work: heavy runs take `local/locks/heavy-run.sh`; only this task's rows of the shared
  records are edited.

## Why now

The tier-2 review of PR #71 (finding 4) reproduced a defect older than DATA-COVERAGE: `AccessDrain`
records ROM reads (`rom_reads`, `rom_bitmap`) inside `_record` during every resolution round, so a
round discarded for a conflict, or a resolution dropped by `_drop_conflicting`, leaves its ROM
reads in the record. R-0093 uses `rom_read_ranges` as its measure of what the original reads.

## Outcome and boundaries

1. ROM-read recording and every other per-round side effect (register shadow and DMA log, stored
   values, labels, watch log, unresolved work RAM code counts) apply only to the kept round's kept
   resolutions.
2. A synthetic test in `tests/tooling/test_access.py` that fails before the fix: a frame whose
   first round resolves a pointer to a ROM address that a later write in the same round changes.
3. The number of corpus frames that needed more than one round, and R-0093's totals re-measured
   with `local/evidence/data-coverage/captures.sh` and `aggregate.py`, with any difference recorded.

Out of scope: what a resolution cannot see (writes through unresolved pointers, DMA into work RAM).
