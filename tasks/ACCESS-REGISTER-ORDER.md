# ACCESS-REGISTER-ORDER - register stores in instruction order

## Assignment

- Status: **in_progress**. Claimed 10 October 2026 20:01 UTC (11 October local) at the user's request, on
  `task/access-round-reads` `1375c9d` (PR #74, ACCESS-ROUND-READS, accepted in its record and
  not yet merged at claim; this branch takes main once #74 merges). Weekly usage at claim 36%
  (5-hour window 10%).
- Worker: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`); coordinator, primary and
  integrator. Branch `task/access-register-order` in `.claude/worktrees/nostalgic-matsumoto-42e59c`.
- Milestone: M4 (original game coverage), laboratory tooling.
- Review tier: **2** (tooling): the change is in `tools/unirally_lab/access/derive.py` and its
  tests; no `src/core`, `src/app`, pack, gate or baseline change. One independent round.
- Task provider: Anthropic.
- Dependencies: [ACCESS-ROUND-READS](ACCESS-ROUND-READS.md) (records only the kept resolution
  round; its corpus scripts), [DATA-COVERAGE](DATA-COVERAGE.md) (`extract.py`, `aggregate.py`, R-0093).
- Parallel work: heavy runs take `local/locks/heavy-run.sh`; only this task's rows of the shared
  records are edited.

## Why now

The tier-2 review of PR #74 (finding 4) found a defect older than ACCESS-ROUND-READS:
`AccessDrain._record` applies register stores to the register shadow `_regs`, and logs
MDMAEN/HDMAEN stores in `dma_log` with the channel parameters from that shadow, as each access is
recorded. Direct accesses are recorded in instruction order in `drain_frame`'s first pass, but
resolved accesses (stores through indirect pointers, work RAM code) only after the resolution
rounds. So a DMA enable stored through a resolved pointer logged the frame's last channel
parameters, not those at its own store, and a resolved channel-parameter store landed after a
direct enable later in the frame. R-0093 reads the DMA sources from `dma_log`.

## Outcome and boundaries

1. Register stores reach the shadow in sequence order across direct and kept resolved accesses,
   and `dma_log` entries are in sequence order.
2. A synthetic test in `tests/tooling/test_access.py` that fails before the fix: a resolved
   channel-parameter store before a direct MDMAEN store, and a resolved MDMAEN store before a
   direct parameter change.
3. R-0093 re-measured: the corpus access captures re-run, reduced with DATA-COVERAGE's
   `extract.py`, compared with ACCESS-ROUND-READS's runs and aggregated with profile v36's pack
   rules; any difference recorded in R-0093.

Out of scope: what a resolution cannot see (writes through unresolved pointers, DMA into work
RAM, HDMA tables in work RAM).

## What changed

`tools/unirally_lab/access/derive.py`: `_record` no longer writes the shadow. It appends each
register store (seq, pc, offset, width, value) to the frame's list, and `drain_frame` applies the
list sorted by seq (a stable sort, so an instruction's own stores keep their order) once the kept
round is recorded. Only the kept round's kept resolutions reach `_record`, so a discarded round
still leaves no register store.

## Results

- **Tests** (`RegisterOrderTests`, 2 new; commit `9707f2d` adds them before the fix, where both
  fail):
  - `STA [$80]` to `$00:4302` (`$34`), then `STA $420B`: the MDMAEN entry's channel 0 `A1T` is
    `$34` (before: 0, the resolved store applied after the enable);
  - `STA $4302` (`$11`), `STA [$84]` to `$00:420B`, `STA $4302` (`$22`), `STA $420B`: two MDMAEN
    entries, seq 1 with `A1T` `$11` and seq 3 with `$22` (before: seq 3 first, both `$22`).
- `python3 -m unittest discover -s tests/tooling -t tests/tooling`: 574 tests OK (two need the
  checkout's `artifacts/` directory to exist).
- **Corpus re-measure**: running (`local/evidence/access-register-order/captures.sh`).

## Reproduction

From the task checkout with `local/` linked to the main checkout's:

```
local/locks/heavy-run.sh ACCESS-REGISTER-ORDER local/evidence/access-register-order/captures.sh 8
```

## Review and integration

- Tier 2, one independent round: pending.
