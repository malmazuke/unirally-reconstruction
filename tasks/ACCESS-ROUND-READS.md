# ACCESS-ROUND-READS - a discarded resolution round's ROM reads

## Assignment

- Status: **in review** (tier 2). Claimed 10 October 2026 08:10 UTC at the user's request, on
  `task/data-coverage` `fefe661` (PR #71, DATA-COVERAGE, open and approved at claim). #71 merged
  at that head (main `2b1bd99`), and the branch was rebased onto it. Weekly usage at claim 31% (5-hour window 66%).
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

## What changed

`tools/unirally_lab/access/derive.py`:

- **Only the kept round is recorded.** `_resolve_round` records nothing; it returns each
  resolution (the work RAM bytes it read, its label and its accesses) and the writes those
  accesses make. After the last round, `drain_frame` passes the kept round's kept resolutions
  through `_record` once. A round discarded for a conflict, or a resolution `_drop_conflicting`
  drops, now leaves no ROM read (`rom_reads`, `rom_bitmap`), stored value, label, register shadow,
  DMA log or watch log entry, and the work RAM code's unresolved count is taken from the kept round
  only (it was counted once per round).
- **A round is stable when it reproduces its inputs** (decision, recorded here). Before, a frame
  stayed in conflict while any resolved write touched a byte some resolution read, even when the
  next round resolved it consistently, so every such frame ran to `MAX_RESOLVE_ROUNDS` and dropped
  the resolutions. Now each round resolves against the frame's recorded writes plus the previous
  round's resolved writes, and is kept when, at every byte a resolution read (including the old
  values of computed RMWs), its own resolved writes equal those it was resolved against. The first
  round's conflict test is unchanged, so "needed more than one round" means the same before and
  after. Why fix it here: without it the leak fix would turn every such frame into dropped,
  unresolved accesses; the review's reproduction (store, `INC`, load through `[$63]`) now resolves
  to `$01:9001`, its value at the load.
- **An indirect access of resolved work RAM code** counts the code's bytes as read, so it is
  dropped with the code.
- **`resolution_rounds`** in the access record: frames by the rounds they took (additive field,
  schema version unchanged). The store-list part of `_record` is now the module function
  `append_writes`, shared with the rounds.

## Results

- **Tests** (`tests/tooling/test_access.py`, `ResolutionRoundTests`, 6 new; commit `f295cb6`
  adds them before the fix, where all six fail):
  - the task's case: `LDA [$63]` before `STA [$80]` writes the pointer. The first round takes the
    end-of-frame pointer `$01:9000`; the record kept it beside the correct `$01:8000` (before:
    `rom_read_ranges` `[[0x8000, 2], [0x9000, 2]]` and the load dropped). After: `[[0x8000, 2]]`,
    resolved in 2 rounds, 0 unresolved;
  - the same frame with one round allowed: the dropped load leaves no ROM read (before:
    `[[0x9000, 2]]`);
  - a discarded round's store through a pointer to `$00:420B` leaves no DMA log entry and no row;
  - the review's reproduction with `--resolve-rmw`: reads `$01:9001` only; dropped, it reads
    nothing (before: both `$01:8001` and `$01:9001`);
  - frames without a conflict take one round.
- `python3 -m unittest discover -s tests/tooling -t tests/tooling`: 569 tests OK (two need the
  checkout's `artifacts/` directory to exist).
- **Corpus re-measure** (`local/evidence/access-round-reads/`, tools at `d5e24cc`
  (run as `d2692bb` before the rebase; `tools/` and `tests/` identical), R-0089's
  manifests, 10 jobs under `heavy-run.sh`, 09:56-11:09 UTC):
  - 236 runs kept (004 fails by design; 103-106 kept under `followup.sh`'s rule), 1,666,993 frames;
  - **frames that needed more than one round: 0.** Every frame resolved in one round, with 0
    conflict bytes, 0 dropped resolutions and 0 unresolved accesses;
  - every reduced run (`runs/NNN.json`) is byte-identical to R-0093's;
  - `aggregate.py` with R-0093's inputs (profile v36 rules from `6b6790a`) writes
    `aggregate.json` and `aggregate.out` byte-identical to R-0093's: 1,562,774 of 2,097,152 ROM
    bytes read (CPU 904,803, DMA 678,967, HDMA 22,748), 1,551,791 of 1,966,080 in the data banks.
  - **Difference: none.** The leak needed a multi-round frame, and the corpus has none. R-0093's
    note on the leak now says so.

## Reproduction

From the task checkout with `local/` linked to the main checkout's:

```
local/locks/heavy-run.sh ACCESS-ROUND-READS local/evidence/access-round-reads/captures.sh 10
python3 local/evidence/access-round-reads/compare.py
git show 6b6790a:tests/manifests/content/classic-crawler-tracks-pack.json > pack-v36.json
python3 local/evidence/access-round-reads/aggregate.py <ROM> pack-v36.json local/evidence/access-round-reads/aggregate.json
cmp local/evidence/access-round-reads/aggregate.json local/evidence/data-coverage/aggregate.json
```

`aggregate.py` and `track_assets.json` are copies of DATA-COVERAGE's (identical by `cmp`);
`aggregate.py` reads the runs beside it.

## Not covered

- A corpus frame that exercises the multi-round path: none exists, so the new round logic is
  checked by the synthetic tests only.
- What resolution cannot see stays as R-0093 states: writes through unresolved pointers, DMA into
  work RAM, and HDMA tables in work RAM.
