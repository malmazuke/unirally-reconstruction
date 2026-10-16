# PODIUM-AUDIO - the league podium's music

## Assignment

- Status: **claimed** 17 October 2026 on main `6c50ac1` (Claude Code desktop app, Claude Opus 5.5
  `claude-opus-5-5`; coordinator and integrator; a worker subagent implements, a fresh subagent
  reviews). Queued 16 October 2026 by [LEAGUE-TOURS](LEAGUE-TOURS.md) (R-0098). Weekly usage at
  claim 14%.
- Choice: the smallest player-visible gap in the queue (the league's podium is silent), with its
  original schedule already captured (`tour-cues`); ahead of PODIUM-RETURN-TIMING (needs a
  cycle-count model), RECORD-SCREENS, NAME-CHECK, SPLIT-HUNTER-PICTURES and GATES-AGAINST-MAIN.
  No other session holds a claim (the `task/player-hint-voices` worktree is from 7 October). The
  user asked for the next task.
- Branch `task/podium-audio` in `.worktrees/podium-audio`; evidence in main
  `local/evidence/podium-audio/`.
- Task provider: Anthropic.
- Milestone: M4 (original game coverage).
- Review tier: **1** (sound cues and their frame timing in `src/core`, possibly the pack, D-0010).
- Dependencies: MODE-AUDIO (R-0091), LEAGUE-TOURS (R-0098).

## Why

Native plays nothing on the league's podium, where the original starts `$82:E08E`'s sound session
and its music. That shows in `tour-cues` at 11420 and 11501-11502. The menu fade before the podium
and the title set after it are already native.

## Outcome

- The podium's session and music equal to the original's cue schedule, frame waits included.
- The app's audio producer runs through the podium.
