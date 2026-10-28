# CARTRIDGE-OPTION-BITS - what sets the cartridge's option words, and what they do

## Assignment

- Status: **claimed** 28 October 2026 14:10 UTC on main `a6a1b385`, on the user's instruction
  (Claude Code desktop app, Claude Opus 5.5 `claude-opus-5-5`; coordinator and integrator; a worker
  subagent implements, a fresh subagent reviews). Queued 4 October 2026 by
  [COVERAGE-GAPS](COVERAGE-GAPS.md) (R-0089, queue item 7); the record is written at the claim.
  Run under [D-0013](../docs/decisions/D-0013-task-time-and-disk-budget.md): claim to merge against
  the 6-hour target, new evidence under 1 GB.
- Branch `task/cartridge-option-bits` in `.worktrees/cartridge-option-bits`; evidence in main
  `local/evidence/cartridge-option-bits/`.
- Task provider: Anthropic.
- Milestone: M4 (original game coverage).
- Review tier: **1** (cartridge RAM and the state the bits gate).
- Dependencies: R-0052 (HUNTER), R-0054 (boot and main menu), R-0089, R-0090 (save files),
  R-0093 (data coverage), R-0094 and R-0104 (option bit 3 is two views).

## Why

R-0089 listed the cartridge option words' unrecovered paths as a gap: what sets `$77:0742`'s
bits and their effects (bits 1, 4, 9 and 10 in R-0054; bit 13 in R-0090 and R-0099), HUNTER's
`$12D1` palette mode (R-0052), the save image's `$10D0` and the unmodelled bytes (R-0090), and
the `$82:DC2E-DC3D` path that picks the never-read `scenery.14` backdrop when `$77:0750` bit 3 is
clear (R-0093). Since then R-0094 and R-0104 showed that `$77:0750` bit 3 ("HUNTER's option bit
3") is the race's two-view flag, so that part is recovered.

## Outcome

- A census of `$77:0742` and `$77:0750`: every bit, every writer and reader in the static map's
  listing, whether the shipped game can set it, and whether native models it. Recorded in a
  research record with addresses.
- Each bit the shipped game can reach is modelled natively and checked against a capture of the
  original that exercises it (preloaded cartridge RAM or a warm boot where a path needs one).
- `$12D1` and the `scenery.14` path: reached and implemented, or shown unreachable from the
  listing with the reason.
- Bits no path sets are recorded as such, not implemented.
