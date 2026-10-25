# TASK-EFFICIENCY - shorter tasks and less disk

## Assignment

- Status: **in review**, 25 October 2026. Claimed on main `17aee63` (Claude Code desktop app,
  Claude Opus 5.5 `claude-opus-5-5`; coordinator, primary and integrator), on the user's
  instruction.
- Task provider: Anthropic.
- Milestone: M4 (process).
- Review tier: **2**.
  - Changed: laboratory tooling (`tools/compact_evidence.py`), a tooling test's timeout, and
    records (D-0013, AGENTS.md, the workflow).
  - Unchanged: `src/core`, the gates, the packs and every reference baseline.
- Dependencies: [D-0011](../docs/decisions/D-0011-evidence-retention.md);
  [D-0012](../docs/decisions/D-0012-selective-gates-against-main.md), which arrives with
  [#84](https://github.com/malmazuke/unirally-reconstruction/pull/84).
- Branch `task/task-efficiency` in `.worktrees/task-efficiency`.

## Why

On 24 October 2026 the user said: "This isn't sustainable. Come up with a plan for drastically
reducing both the task time and the disk space usage." Then: "Go ahead and do it now."
[D-0013](../docs/decisions/D-0013-task-time-and-disk-budget.md) gives the measurements and the
plan.

## Outcome

1. **D-0013**, adopted, with pointers in AGENTS.md and `docs/AGENT_WORKFLOW.md`.
2. **`tools/compact_evidence.py`** and its tests.
   - Identical files become APFS clones. The rest get transparent filesystem compression.
   - Every swap is checked byte for byte, and readers need no change.
   - Run once over main's `local/evidence` and `artifacts`.
3. **`local/locks/heavy-run.sh`** (local, not tracked), installed by atomic rename:
   - a stall watchdog that measures each process's CPU, so a finished child takes nothing away;
   - keep-awake for the length of each run;
   - a 15 GB free-disk floor.
4. **The synthetic suite's timeout probe.** `test_native_reports.py` passed `--test-timeout 1`,
   which became every ctest test's default timeout, so any test slower than a second under load
   timed out. That made concurrent heavy runs fail spuriously. The test now passes 30 seconds. The
   probe `lab_probe_expected_timeout` still times out on its own `TIMEOUT 1` property: it sleeps
   10 s.

## Measurements behind D-0013

Taken by the coordinator on 24 and 25 October 2026.
- **Claim to merge.** RECORD-SCREENS took 23 h 09 min: claim commit `ce33f1e` at 22 October
  23:50:57 UTC; #83 merged 23 October 23:00:00 UTC (`gh pr view 83`).
- **The worker.** 3 h 06 min (harness, 11,146,621 ms) and its round-1 fixes 65 min (3,929,022 ms).
  Recorded in RECORD-SCREENS' gates section.
- **The reviewer.**
  - Round 1 ran 5 h 29 min by the harness (19,744,219 ms). Its capture script was written at
    03:14 UTC, and it took the heavy-run lock when the gates released it at 08:16 UTC, after
    4 h 48 min of waiting. That leaves about 41 min of its own work.
  - Round 2 ran 14 min (863,833 ms).
- **The gates.**
  - 5 h 18 min at `b6fa277`. The run at `3f8a1ab` took 13 h 01 min, including about 7 h asleep:
    `pmset -g log` shows the lid closed until 05:11 local.
  - Both runs are in RECORD-SCREENS' record.
- **Disk** (`du`, 25 October 2026):
  - `local/evidence` 112 GB.
  - Summing the files of 1 MB and more by extension: `.bin` 47.7 GB and `.wram` 36.4 GB (84 GB of
    RAM dumps), `.s16le` 5.2 GB, `.json` 4.4 GB.
  - 49 files are over 480 MB, 39.6 GB in all.
- **Compression ratio.** zlib level 6 on the first 50 MB of `record-screens/probe-rs-track/series.bin`
  gives 144.2×. The filesystem's own compression of that file gives 51,200 KB to 1,476 KB on disk.

## Review round 1 (returned on `c86e8990`)

[Review](https://github.com/malmazuke/unirally-reconstruction/pull/85#pullrequestreview-5640917528).
The coordinator stopped the queued compaction before it started; it had touched nothing. The tool
was rewritten:
1. **Clone pairs.** Source and copies now come from one sorted list. Identical files are grouped
   before compression, so only the first of each group is compressed and the rest clone it. Test:
   `z/` and `m/` both end compressed.
2. **Interrupted runs.**
   - Scratch now lives in one `.compact-scratch` directory per root. It is removed at the start of
     a run and never counted as evidence.
   - Each step's scratch is removed in a `finally`.
   - Test: a leftover copy is not counted.
3. **A writer during the run.** The swap now checks that the original still has the size, mtime and
   inode it had when it was read, and refuses otherwise. Test: an append between the read and the
   swap is kept.
4. **Masked skips.** The macOS tests are defined only on macOS, and the refusal test only
   elsewhere, so none passes while running nothing.
5. **Clone metadata.**
   - A clone keeps the copy's own mode, access time and mtime.
   - A copy whose mode or extended attributes differ from the first's is not cloned (filesystem
     bookkeeping attributes aside).
   - Test: `b`'s mtime is kept, and `c` (mode 600) is left alone.
6. **Repeated work.**
   - Already-compressed files and files over `--max-compress` (480 MiB) are not tried.
   - Each compression needs `--min-free` GB (15) plus the file's size free.
   - D-0013 says the tool runs through `heavy-run.sh`.
   - Test: a second run compresses nothing.
7. **Records.**
   - D-0013's figures now have their sources (above).
   - 5 h 18 min is used throughout.
   - The reviewer's work is about 55 min over two rounds.
   - The repeatability processes' 1-second cap is noted.
8. **Minor.**
   - The default roots are this checkout's, whatever the working directory. Test: a run from
     another directory.
   - A root not on APFS is refused, since clones would be full copies.
   - `ditto -c` is always waited for.

## Results

(The compaction of main's evidence follows the re-review.)
