# STUNT-HUD - a stunt event's race picture

## Assignment

- Status: **in progress**. Queued 27 September 2026 (UTC) by STUNT-EVENT-RACE; claimed 27 September
  2026 at 09:30Z by the Claude Code desktop session that ran STUNT-RESULT, on `0173cbb`. The
  implementation worker started on STUNT-RESULT's branch while it was in its gates; its commits
  were moved onto this claim.
- Coordinator: the claiming session is coordinator, primary and integrator
- Worker/session/runtime/model: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`)
- Provider quota (D-0004): at claim the weekly window was about 76% used; the user asked to stop
  after this task.
- Reviewer: a fresh Anthropic subagent, isolated checkout.
- Branch and isolated worktree: `task/stunt-hud` in `.worktrees/stunt-hud`.
- Milestone: M4 (original game coverage)
- Tier: 1: presentation, plus a race-state finding on NEON (track 42) made during the work.
- Dependencies: STUNT-EVENT-RACE (R-0066), the race HUD (CLASSIC-RACE-HUD), R-0063.

## Outcome and boundaries

The race picture of a stunt event (the decode's section 1.6): `stunt` in place of `race`, the clock
counting down, the score and qualifying field and its NMI slot, no opponent sprite or arrow, the
countdown digits, `$15A3`, track 37's BG1 fetch with `$0FF7` and track 42's scenery case.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Pictures | Dense frame windows of bowl-lose and hill-win (the start, a few rewards, the clock's end, the finish captions), captured with frame images | 0 differing pixels | logs |
| Nothing moves | The gates of STUNT-EVENT-RACE | Unchanged | logs |
