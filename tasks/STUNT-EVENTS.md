# STUNT-EVENTS - reconcile the delivered stunt outcome

## Assignment and result

- Status: fulfilled by the earlier STUNT-EVENT-RACE, STUNT-RESULT and STUNT-HUD
  integrations; queue reconciliation on 30 September 2026 UTC in AUDIO-DECISION.
- Milestone: M4 original game coverage.
- Tier: 3 for this record audit. No game/source/reference changes or new stunt
  gameplay acceptance are made here. The contributing implementations retain
  their own tier-1/tier-2 reviews and exact evidence domains.
- Record owner: AUDIO-DECISION primary, OpenAI. Its base, quota and closeout
  apply; this is not a separate claim or a restarted resource allowance.

The coverage roadmap's item 9 originally called for four stunt tracks' rules
and scoring. Those were the cold-start tracks BOWL, JUMPS, HILL CLIMB and
DOWNER. Before the later local/menu modes, three tasks delivered the stunt
outcome for all nine tour event scenarios:

| Outcome | Accepted integration | Evidence and tested domain |
| --- | --- | --- |
| Native event rules/clock/scoring | [#39](https://github.com/malmazuke/unirally-reconstruction/pull/39), merge `e1314698bf25a32bb12f7b784534a64882d18c9f` | [R-0066](../docs/research/R-0066-stunt-event-race.md): seven ridden captures across the four cold-start tracks, all nine idle event captures; race rows exact |
| Result tally, records and continuation | [#40](https://github.com/malmazuke/unirally-reconstruction/pull/40), merge `0173cbb3d164a6653c782cff522a3c99a70f8cdb` | [R-0067](../docs/research/R-0067-stunt-result.md): five captures from power-on including win/loss, quit, tally press and tour completion |
| Event HUD and NEON | [#42](https://github.com/malmazuke/unirally-reconstruction/pull/42), merge `490302a60739ceb29b98e98ce96c3382463aab80` | [R-0068](../docs/research/R-0068-stunt-hud.md): dense BOWL/HILL CLIMB pictures, nine idle event sweeps, NEON ridden capture |

The primary read those records and retained evidence, verified all three PRs
are merged, and verified these commits are ancestors of synchronized main
`16675d0`. The LEAGUE handoff's paired BOWL capture supplements this earlier
one-player domain; it is not the only starting evidence for stunt events.
Reimplementing the four-track outcome would duplicate accepted work.

## Limits and handoff

The contributing research records still own their limits: equal-score caption,
tally wrap, out-of-playfield/negative-y behavior, some NEON probes, and broader
two-player stunt evidence. LEAGUE adds only its recorded paired BOWL domain.
Nine initialized scenarios and the listed comparisons do not establish every
stunt edge case or whole-game accuracy. No expectations were changed or tests
rerun merely to extend a claim.

The next unmet roadmap outcome is [AUDIO-DECISION](AUDIO-DECISION.md), followed
by [AUDIO-TITLE-MENU](AUDIO-TITLE-MENU.md) under D-0009. ROLLING-CONTACT remains
ready as independent race-engine polish.
