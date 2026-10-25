# D-0013 - Task time and disk budget

- Status: adopted on the user's instruction, 25 October 2026 (TASK-EFFICIENCY, Claude Opus 5.5)
- Related: [D-0011](D-0011-evidence-retention.md) (evidence retention),
  [D-0012](D-0012-selective-gates-against-main.md) (selective gates against main, in review on
  [#84](https://github.com/malmazuke/unirally-reconstruction/pull/84)),
  [TASK-EFFICIENCY](../../tasks/TASK-EFFICIENCY.md)

## Problem

On 24 October 2026 the user said: "This isn't sustainable. Come up with a plan for drastically
reducing both the task time and the disk space usage."

- **Time.** RECORD-SCREENS (#83) took about 23 hours from claim to merge, for about 5 hours of work:
  - the worker's captures and code took 3 h 06 min, and its review fixes 65 min;
  - the reviewer's own work took about 30 min;
  - two full gate runs took 5 h 18 min and 13 h, the second with about 7 h of laptop sleep;
  - the reviewer waited 4 h 48 min on the heavy-run lock behind the gates.

  The next task, GATES-AGAINST-MAIN, lost another 4 h to a job hung at 0% CPU, which nobody noticed
  because every agent was idle, waiting to be told the job had finished.
- **Disk.** `local/evidence` held 112 GB, and the user saw 3.8 GB free during a gate run.
  - 84 GB of it is raw RAM dumps: work RAM `$0000-$1FFF` every frame (`series.bin`,
    `wram-series.bin`, `memory.wram`). These compress about 144× (zlib, measured on `rs-track`).
  - Many captures are stored twice: the a/b repeatability runs are byte-identical by design.

## Decision

### Time

1. **Gates.** Gate through the selective runner of D-0012:
   - **Measured.** On #84 it took 34 min on RECORD-SCREENS' change (5 h 17 min serially) and 39 min
     on PODIUM-AUDIO's.
   - **After a review round,** rerun only the steps whose inputs the fix changed, with the previously
     gated head as the base, plus the task's own checks. Never rerun the full set.
   - **Records-only commits** skip the gates; CI's docs fast path covers them.
   - **The full set** runs only before a milestone, or after a change to the gates themselves.
2. **Order.** Within a task: worker, then reviewer, then gates.
   - The reviewer's captures take the heavy-run lock first. The gates start when they finish.
   - Nobody queues behind a multi-hour job.
   - The worker's own checks run under the lock as before.
3. **Liveness.** `local/locks/heavy-run.sh` stops a job with no progress for 30 min (exit 124):
   no new process, and under 5 s of CPU gained by its processes.
   - An agent waiting on a job or another session for more than 30 min checks its progress (log
     growth, CPU) rather than only waiting for its completion notice.
   - The agent tells the user up front when a run will take more than an hour.
4. **Keep-awake.** `heavy-run.sh` keeps the Mac from idle sleep for as long as each run lasts
   (`caffeinate -i`).
   - A closed lid still sleeps the machine, so the coordinator warns the user before a long run.
5. **Target.** About 6 hours from claim to merge for a task the size of RECORD-SCREENS.
   - Each handoff records the worker's capture time and implementation time separately, to find the
     next saving there.

### Disk

1. **Compaction.** `tools/compact_evidence.py` shrinks evidence without changing a byte any reader
   sees, so no reader changes:
   - identical files are stored once, as APFS clones;
   - the rest get macOS's transparent filesystem compression;
   - every replacement is checked byte for byte before an atomic swap.

   It runs on all evidence once, and then at every task's closeout. macOS does not compress files
   larger than about 500 MB this way; they are only stored once where identical.
2. **Free-disk floor.** `heavy-run.sh` refuses to start a run with less than 15 GB free (exit 75).
   - D-0012's runner caps its own temporary use at 6 GB and checks free disk before each step.
3. **Per-task budget.** After compaction, a task's new evidence should stay under 1 GB.
   - The closeout records the size, and the reason for any excess.
4. **Captures.** Capture only what the checks read:
   - a WRAM range and a frame window, not all of work RAM every frame, unless the question needs
     it;
   - pictures only for the frames compared.
5. **Closeout.** The closing session deletes worktrees, their build output and gate output
   directories, keeps the logs, then runs the compaction. D-0011's retention rules are unchanged.

## Not done

- **Two heavy runs at once.** This was proposed and dropped: each gate run already uses every core
  through the runner's pool, so two at once would finish no sooner. The timeout probe that made
  concurrent runs fail spuriously is fixed anyway: `test_native_reports.py` gave ctest a 1-second
  default timeout for every test, and the expected-timeout probe now relies on its own `TIMEOUT 1`
  property.
- **Files over about 500 MB** (49 files, 39.6 GB before compaction) stay uncompressed. Compressing
  them needs either a reader change (`.zst` with a reading helper) or deleting them under D-0011.
  The compaction results in TASK-EFFICIENCY say what remains.
