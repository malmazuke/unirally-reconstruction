# GATES-AGAINST-MAIN - check changes against accepted native code

## Assignment

- Status: **claimed** 24 October 2026 on main `ff1efb6` (Claude Code desktop app, Claude Opus 5.5
  `claude-opus-5-5`; a side session started by the RECORD-SCREENS coordinator). Queued 11 October
  2026 by [EVIDENCE-RETENTION](EVIDENCE-RETENTION.md), at the user's request. Weekly usage at claim
  46%, 5-hour window 27%.
- Why now, and why this task rather than a new one: on 24 October 2026 the user said hour-long
  gate checks and a process using hundreds of GB of disk are not acceptable and asked for a better
  way. RECORD-SCREENS' gates took 5 h 18 min serially and grow with every task, because each task
  appends the earlier tasks' `checks.sh`, and their comparison scripts write every picture to temp.
  Comparing against `main`'s native output is this task's design, so the user's request extends it:
  the decision also chooses which checks run on every change and which only when a change touches
  their system, and the task delivers one gate runner that keeps the run under an hour and its temp
  under a few GB.
- Branch `task/gates-against-main` in `.claude/worktrees/elegant-bhaskara-4abb94`; evidence in main
  `local/evidence/gates-against-main/`.
- Task provider: Anthropic.
- Milestone: M4 (original game coverage); project infrastructure.
- Review tier: **1** (it changes differential gates and reference baselines, D-0008).
- Dependencies: [D-0011](../docs/decisions/D-0011-evidence-retention.md).
- Shared state: heavy runs go through `local/locks/heavy-run.sh`; RECORD-SCREENS' final gates hold
  it until about 23:00 UTC on 23 October 2026, so design and tracked code come first.

## Why

- **What the gates do today.** The gates compare each candidate with stored captures of the
  original: the eleven frozen race gates, the front-end and audio cue comparisons. The folders
  they read hold about 122 GB.
- **Why that may be more than needed.** Once a part is accepted, native was proven equal to the
  original on those inputs. A candidate equal to accepted `main` on the same inputs is therefore
  equal to the original on them, for the same compared fields.
- **What the project already does.** The race sweep and front-end sweep already compare against
  `main`'s binary.
- **The user's question (11 October 2026).** Why keep the original's captures once a part is
  migrated?

## Outcome

- **The design.** A decision record proposes which gates compare against `main`'s native output
  rerun from its commit, and which must stay against the original:
  - changes meant to alter behaviour;
  - parts not yet native;
  - known residual differences;
  - the first acceptance of a capability.
  It also says how a task regenerates the original's capture from its manifest when it needs one.
- **The proof.** The switched gates catch a deliberately introduced regression in each gated
  system (race state, pictures, cues), and the remaining original captures are listed with the
  reason each stays.
- **The payoff.** With the decision adopted, the original captures the switched gates no longer
  read can go under D-0011, keeping their recipes.

## Boundaries

- Acceptance of new behaviour still compares with the original.
- No gate is weakened silently. Each switched gate's compared fields and frame ranges are the
  same as before.
