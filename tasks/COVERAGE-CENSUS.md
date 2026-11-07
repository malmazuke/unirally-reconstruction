# COVERAGE-CENSUS - how far the native game is from the whole original

## Assignment

- Status: **claimed** 7 November 2026 21:30 UTC on main `dffbcf44`, on the user's instruction
  ("Start", answering whether to start a fresh coverage inventory). Claude Code desktop app,
  Claude Opus 5.5 (`claude-opus-5-5`): coordinator and integrator; a worker subagent runs the
  inventory. Run under [D-0013](../docs/decisions/D-0013-task-time-and-disk-budget.md): claim to
  merge against the 6-hour target, new evidence under 1 GB. Usage at the claim: 5-hour 6%,
  weekly 8%.
- Branch `task/coverage-census` in `.worktrees/coverage-census`; evidence in main
  `local/evidence/coverage-census/`.
- Task provider: Anthropic.
- Milestone: M4 (original game coverage).
- Review tier: **3** (D-0008: records, laboratory evidence and a regenerated map; no `src/core`,
  `src/app`, tool or gameplay change). A queued task carries its own tier. If the inventory needs
  a tooling change, that change moves to a queued task rather than raising this one's tier.
- Dependencies: [COVERAGE-GAPS](COVERAGE-GAPS.md) (R-0089, its corpus scripts and
  `recorded-limits.md`), [DATA-COVERAGE](DATA-COVERAGE.md) (R-0093, the access record and the
  corpus coverage), the static map (R-0045), R-0090 to R-0108.

## Why now

R-0089's queue (9 October) is done: SAVE-FILES through RACE-BRANCH-REMAINDER are integrated, and
no task is queued. The user's goal is the whole game natively ("100% coverage from the ROM").
The static map still classes 31,425 of the code banks' 131,072 bytes (24.0%) as unknown and
11,231 as inferred (decoded but never seen running), and native cites 581 of its 759 routines.
Nobody knows how much of that is data, padding, dead code or real behaviour native lacks. This
task measures it and queues what is left.

## Outcome and boundaries

- **The corpus, brought up to date.** R-0089's corpus plus every replay manifest and coverage
  capture added since (R-0090 to R-0108, warm boots included), so the measure reflects every
  accepted task. Coverage captured only where no existing raw coverage covers a manifest.
- **Every byte of banks `$80-$83` accounted for.** Each unknown and inferred byte is put in one
  class with its evidence: executed code (new captures), data read by the original (R-0093's
  access record and any new one), fill, data reached by a pointer or index the listing shows,
  code shown unreachable, code reachable but never captured (with the feature that reaches it),
  and unexplained. Unexplained is a measured remainder, not a guess.
- **Every routine attributed.** For each of the 759 routines: run by the original and cited by
  native, run but not cited (and whether native implements the behaviour under another citation
  or lacks it), or never run (and why).
- **Outside the code banks.** The data banks' unread bytes (R-0093) and what native does not
  extract, brought up to date; the sound program's status as the records give it.
- **The records' limits, brought up to date.** `recorded-limits.md` refreshed from every record
  accepted since R-0089: what each later task closed, and what each added.
- **The app against the runner.** What the original does that the shipped app (`unirally`) does
  not, as the records state it.
- **Result:** a research record (R-0109) with one headline measure of what is left, what "100%"
  can mean given unreachable code, and a queue of tasks in order, each with its domain and tier.
  Records, evidence and the static map regenerated with any new coverage input only: no native or
  tooling change.

## Handoff

Not started.
