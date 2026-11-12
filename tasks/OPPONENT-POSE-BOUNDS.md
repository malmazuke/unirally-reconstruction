# OPPONENT-POSE-BOUNDS - a computer opponent's colour maths read past its table

## Assignment

- Status: **claimed** 12 November 2026 23:06 UTC on main `e11cbe11`, on the user's instruction ("next
  task", after SAVE-IMAGE-REMAINDER #105). Queued 9 November 2026 by
  [SAVE-IMAGE-EXACT](SAVE-IMAGE-EXACT.md) (review round 1). Claude Code desktop app, Claude Opus 5.5
  (`claude-opus-5-5`): coordinator and integrator; a worker subagent captures and implements, a fresh
  subagent reviews. Run under [D-0013](../docs/decisions/D-0013-task-time-and-disk-budget.md): claim
  to merge against the 6-hour target, new evidence under 1 GB. Usage at the claim: 5-hour 2%,
  weekly 1%.
- Branch `task/opponent-pose-bounds` in `.worktrees/opponent-pose-bounds`; evidence in main
  `local/evidence/opponent-pose-bounds/`.
- Task provider: Anthropic.
- Milestone: M4. Review tier: **1** (race presentation content for a scenario the menus reach).

A libc++-hardened debug build (`_LIBCPP_HARDENING_MODE_DEBUG`) aborts on the corpus route `c175`
(`hunter-effects`, R-0109): `presentation.cpp:243`
`colour_math.subspan(colour_math_size * scenario.pairing.opponent, colour_math_size)` with the
computer opponent `0x14` (ANTI-UNI), called from `classic_race_presentation_content` from the
runner's `start_race`. The release build reads past the table without a check; a Linux debug run of
that route would abort. The code and the scenario are main's (SAVE-IMAGE-EXACT did not change them).
Outcome: the original's colour maths for opponents past the table recovered from the listing and
used, or the read bounded as the original's is, with a test on opponent `0x14` and the route's
pictures unchanged.

## Handoff

Not started.
