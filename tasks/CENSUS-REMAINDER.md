# CENSUS-REMAINDER - the code-bank and data-bank bytes R-0109 leaves unexplained

## Assignment

- Status: **claimed** 8 November 2026 21:33 UTC on main `2a38ec3e`, on the user's instruction
  ("Start CENSUS-REMAINDER"). Queued 8 November 2026 by [COVERAGE-CENSUS](COVERAGE-CENSUS.md)
  (R-0109, queue item 3). Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`):
  coordinator and integrator; a worker subagent runs the census. Run under
  [D-0013](../docs/decisions/D-0013-task-time-and-disk-budget.md): claim to merge against the
  6-hour target, new evidence under 1 GB. Usage at the claim: 5-hour 8%, weekly 26%.
- Branch `task/census-remainder` in `.worktrees/census-remainder`; evidence in main
  `local/evidence/census-remainder/`.
- Task provider: Anthropic.
- Milestone: M4. Review tier: **3** (records, laboratory evidence and a regenerated map).

R-0109 accounts for every byte of banks `$80-$83` but leaves a measured remainder: unknown bytes no
capture reads and no operand names, gap-sweep candidates nothing reaches statically, and small branch
arms of run routines; its data reads come from access records of only 354 of the corpus's runs.
Outside the code banks R-0093's open items stay open (entries `$47`, `$49`, `$4B`, `$4F`, `$51`, the
palettes `$B7-$BE`, `$03`, `$04`, `$3D`, bank `$A2`, `$94:957B`, `$94:9147`, `$97:D9F4`). Outcome:
access records for the rest of the corpus, each unexplained block read from the listing (computed
jumps, long pointers, tables reached through a pointer the record shows) and either reclassified with
evidence or left in a smaller, listed remainder; the data banks' open entries shown read, unreachable
or unused.

## Handoff

Not started.
