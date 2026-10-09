# MODE-AUDIO - native audio in 2P, VS, league, OPTIONS and the idle demos

## Assignment

- Status: **in progress**, claimed 10 October 2026 on main `a54341a`, in the session the user asked
  to keep working until weekly usage reaches 50% (10% at claim). Queued by
  [COVERAGE-GAPS](COVERAGE-GAPS.md) (R-0089, queue item 2).
- Worker: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`); coordinator, primary and
  integrator. Branch `task/mode-audio` in `.worktrees/mode-audio`.
- Milestone: M4 (original game coverage).
- Review tier: **1** (it changes the sound cues the front end and races report and the app's audio
  producer; D-0010's frame-anchored commands).
- Task provider: Anthropic.
- Dependencies: AUDIO-ONE-PLAYER (R-0077, D-0010), TWO-PLAYER-VS (R-0071), LEAGUE (R-0073),
  OPTIONS (R-0072), IDLE-DEMO-ROTATION (R-0087), SAVE-FILES (R-0090: the audio model's warm
  power-on).

## Why

With `--native-title-menu-audio` the app plays native audio through the one-player game, but 2P,
VS, league, OPTIONS and the idle demos stop the cued producer and go silent (R-0077's limit).

## Outcome

- Cue schedules captured from the original (R-0077's method: two dispatcher-watch captures each)
  for a 2P race, a VS race, league events, OPTIONS' screens and idle demo cycles; native's
  `front_end_runner --sound-cues` equal to them line for line.
- The app's producer runs through those modes; anchored PCM measured on some of them (D-0010).
- The audio model's warm power-on reads the save's cheat flag and `$10AD` like its soft reset
  (SAVE-FILES' advisory).
