# OPEN-BRANCHES - the reachable branches no capture of the original has run

## Assignment

- Status: **claimed** 9 November 2026 23:41 UTC on main `19ecb59f`, on the user's instruction
  ("Start OPEN-BRANCHES"). Queued 8 November 2026 by [COVERAGE-CENSUS](COVERAGE-CENSUS.md)
  (R-0109, queue item 4). Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`):
  coordinator and integrator; a worker subagent captures and implements, a fresh subagent reviews.
  Run under [D-0013](../docs/decisions/D-0013-task-time-and-disk-budget.md): claim to merge against
  the 6-hour target, new evidence under 1 GB. Usage at the claim: 5-hour 1%, weekly 29%.
- Branch `task/open-branches` in `.worktrees/open-branches`; evidence in main
  `local/evidence/open-branches/`.
- Task provider: Anthropic.
- Milestone: M4. Review tier: **1** (race and front-end state).

R-0109's code whose reachability is open and the branches the records list as never captured: the
HUD's fields of 10 and more (R-0043), the `$77:0749 = $FF` path (R-0058), `$83:9B49`'s arm and
`$83:9C47`, the rider sprite-tile queue's last slots (`$82:BFC9-C53C`, `$82:CFF3-D194`), the loop
descriptor facing away (R-0108), a stunt score of 1000 (R-0107), track 13's cell past its flag table
(R-0107), 2P and VS menus on the locked tours' tracks, a race tie, NEXT TRACK's wrap after track 44
(R-0095), demo cycles past the twentieth title (R-0096), and the never-captured sounds (corkscrew
ejection, the 9:5x warning, the cheat page reveal, the slow checkpoint, BRONSEN's praise, R-0077).
Outcome: for each, a route of real inputs captured on the original and compared with native, or a
recorded argument that it is unreachable; native changed only where it differs.

Added by [CENSUS-REMAINDER](CENSUS-REMAINDER.md) ([R-0112](../docs/research/R-0112-census-remainder.md),
`local/evidence/census-remainder/arms.md`), the never-run arms whose feature is named: the other end
of the sprite-tile queue (`$82:B8E3-BF76` and `$82:C53E-CFC7` never stopped at their first 16 and
21 slots: a frame queuing fewer tiles than any capture); two views in HUNTER's effects (`$83:CEF3`,
`$83:D1D4`, `$83:D343`; 2P or VS on the locked tours' tracks, above); a rider pose of 4096 or more
(`$81:9E2B`, which would read bank `$A2`); the track menu's signed table offsets (`$80:BA7C`,
`$80:BB1E`, `$80:BB72`); and the HUD numbers of 10 and more at `$81:D218`, `$81:D692`, `$81:DE90`,
`$81:DFC1`, `$81:EC6F` (with R-0043's fields, above). The other 187 of the 278 small arms left R-0112 groups by
guard and leaves open.

## Handoff

Not started.
