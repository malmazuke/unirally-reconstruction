# LEAGUE-TOURS - league beyond the first CRAWLER tour

## Assignment

- Status: **claimed** 14 October 2026 on main `ceb50cf` (Claude Code desktop app, Claude Opus 5.5
  `claude-opus-5-5`; coordinator and integrator; a worker subagent implements, a fresh subagent
  reviews). Queued by [COVERAGE-GAPS](COVERAGE-GAPS.md) (R-0089, queue item 6). Weekly usage at
  claim: recorded in the handoff.
- Choice: R-0089's next queue item and the largest player-visible gap left there; ahead of the
  smaller RECORD-SCREENS and NAME-CHECK, SPLIT-HUNTER-PICTURES and the infrastructure task
  GATES-AGAINST-MAIN. No other session holds a claim. The user asked for the next task after the
  second evidence cleanup (#80).
- Branch `task/league-tours` in `.worktrees/league-tours`; evidence in main
  `local/evidence/league-tours/`.
- Task provider: Anthropic.
- Milestone: M4 (original game coverage).
- Review tier: **1** (league state, scoring and cartridge RAM in `src/core`).
- Dependencies: LEAGUE (R-0073), TWO-HUMAN-STUNTS (R-0095), SAVE-FILES (R-0090), MODE-AUDIO
  (R-0091), MENU-INPUT (R-0097).

## Why

LEAGUE (R-0073) accepted a league over the first CRAWLER tour: its setup, pairings, races,
scoring, standings, podium and continuation. R-0073 leaves the rest outside: the league's other
tours and their events (all 44 played events), tour progression and what the original does when
a league's tour is finished or a later tour is picked. R-0089 lists "league is covered for the
first CRAWLER tour only" among the whole features native lacks.

## Outcome

- A league played natively through every tour the original offers it, frame-exact against
  captures of the original: PICK TOUR's choices and locks for a league, each tour's events (lap,
  stunt and special tracks) with league pairing and scoring, the standings and podium after each
  tour, and the cartridge RAM's league fields (membership, played, points, pairing, cursor).
- What native does differently today is measured first (a capture of a second tour against main).

## Boundaries

- The league's first tour stays as accepted (R-0073's frozen gate unchanged).
- Presentation residuals R-0073 and R-0095 record stay with them unless this work fixes them.
