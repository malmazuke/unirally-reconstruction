# MODE-AUDIO - native audio in 2P, VS, league and OPTIONS

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

## Decisions

- The idle demo's sound is split off as DEMO-AUDIO (queued): native's demo reports no cues at all
  (title set loads at each demo's title, the race's dispatches with that set), a separate piece of
  work from the menus' sounds, which this task covers.
- SAVE-FILES' audio advisory (the warm power-on's title work) stays open: changing it needs a
  warm-boot audio capture to check against, and the cold path's timing is validated as it is.

## Checkpoint - 10 October 2026 15:00 UTC

- Six schedules captured (R-0091's table) with dispatcher and enqueue-site watches; tools in main
  `local/evidence/mode-audio/`.
- Native: the split race's finish fade site (`finish2`, `$83:E7F2-E7FD`); the menus' sounds in
  OPTIONS, the keyboard, the league's slots, members and awards, the picks and the two-player
  continuation, with `$80:B0FA`'s refusal as `MenuSound::refused`; the second player's menu
  slides silently. The placement of the menus' sounds was done by an implementation subagent and
  checked here (the five schedules' cues, eight R-0077 schedules, the runner's rows unchanged
  against main's binary).
- The app keeps the cued producer past any main menu choice but the idle demo.
- Cues: OPTIONS equal; 2P, VS and league differ only where their slides do (recorded residuals).
  PCM: `options-rename` 99.6% and `twop-next` 95.7% of windows within 1 dB.

## Review candidate

- Records: [R-0091](../docs/research/R-0091-mode-audio.md).
- Gates: `local/evidence/mode-audio/gates.sh` against main `a54341a`'s binaries.

## Review - round 1 (returned)

- [Review](https://github.com/malmazuke/unirally-reconstruction/pull/68#pullrequestreview-5471714773)
  on `9f7554d`: return. Blocker: the RECORDS detail screens were silent (TRACK RECORDS' entry,
  table and Left/Right; PLAYER SCORES' Down/Up). Should-fix: the gates' baseline binaries could
  not start (their DSP library lived in the removed build tree; fixed mid-review, the run
  restarted and then stopped for this round); R-0091's wording on PICK CHALLENGER and GROUP
  TABLES. Advisories: the continuation menu's wrap, the refusal comment, the demo stopping the
  app's audio for the session, the queued tasks only in prose, one unformatted helper.
- Done: TRACK RECORDS' and PLAYER SCORES' sounds (the review's schedules now equal); GROUP
  TABLES' Up/Down need input native lacks and go to MENU-INPUT with the continuation's wrap;
  R-0091 corrected; DEMO-AUDIO and MENU-INPUT have records and registry rows; gates.sh refuses a
  baseline that cannot run; the comment and the helper fixed.
