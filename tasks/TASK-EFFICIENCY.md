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

## Results

(Filled in below as the runs finish.)
