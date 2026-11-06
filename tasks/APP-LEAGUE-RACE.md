# APP-LEAGUE-RACE - the app's league races, and the podium's audio in the app

## Assignment

- Status: **in progress**. Claimed 6 November 2026 22:21 UTC on main `6797457b`, on the user's
  instruction ("Next task"; Claude Code desktop app, Claude Opus 5.5 `claude-opus-5-5`;
  coordinator and integrator; a worker subagent implements, a fresh subagent reviews). Queued
  29 October 2026 by [CARTRIDGE-OPTION-BITS](CARTRIDGE-OPTION-BITS.md) (R-0105, gates round 1).
  Run under [D-0013](../docs/decisions/D-0013-task-time-and-disk-budget.md): claim to merge against
  the 6-hour target, new evidence under 1 GB. Usage at the claim: 5-hour 0%, weekly 0%.
- Branch `task/app-league-race` in `.worktrees/app-league-race`; evidence in main
  `local/evidence/app-league-race/`.
- Task provider: Anthropic.
- Milestone: M4 (original game coverage).
- Review tier: **1** (the app's race path).
- Dependencies: PODIUM-AUDIO (R-0099), LEAGUE (R-0073), CARTRIDGE-OPTION-BITS (R-0105).

## Why

In a league race the app and `front_end_runner` part ways on the same save and inputs:
- PODIUM-AUDIO (R-0099) found LEAGUE's `organic-full-tour-turnaround` second race diverging at
  frame 12,619 in the app and never ending.
- CARTRIDGE-OPTION-BITS' gates found the app logging pause cues from frame 3180 of the league race
  on `podium-audio/app_run.sh`'s route (`local/evidence/cartridge-option-bits/gates-round-1/`),
  on main too.

`front_end_runner` matches the original there. Since R-0105 the original's podium cannot be
reached by a warm boot (the league slots clear the owed podium), so the app's only podium check,
`app-podium-audio`, now stops before the race and covers no podium.

## Outcome

- The app's league race equals `front_end_runner`'s (and so the original's) on both routes.
- `app-podium-audio` covers a podium again, reached by racing a league's last event in the app.
