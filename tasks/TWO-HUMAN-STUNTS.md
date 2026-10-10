# TWO-HUMAN-STUNTS - stunt events with two humans: 2P and VS

## Assignment

- Status: **in progress**, claimed 10 October 2026 on main `ef8e440`, in the session the user asked
  to keep working until weekly usage reaches 50% (23% at claim). Queued by
  [COVERAGE-GAPS](COVERAGE-GAPS.md) (R-0089, queue item 5); SAVE-FILES (R-0090) and MODE-AUDIO
  added the two-human restart's records and VS mode 2's counters to it. DATA-COVERAGE runs in a
  parallel session; heavy runs share `local/locks/heavy-run.sh`.
- Worker: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`); coordinator, primary and
  integrator. Branch `task/two-human-stunts` in `.worktrees/two-human-stunts`.
- Milestone: M4 (original game coverage).
- Review tier: **1** (race and result state with two humans).
- Task provider: Anthropic.
- Dependencies: STUNT-EVENTS (R-0066-R-0068), TWO-PLAYER-VS (R-0071), R-0082 (two-human stunt
  notes), TWO-HUMAN-RESTART (R-0084), LEAGUE (R-0073: its paired BOWL), SAVE-FILES (R-0090).

## Why

Native refuses a stunt event's result with two players: the 2P tally, the totals, the best and
the riders' statistics, the second rider's pass and rider 1's score field `$12CB` are not
modelled (R-0066-R-0068, R-0082). Only LEAGUE's paired BOWL is covered. SAVE-FILES also found
the records differing after a two-human restart (`$0422`, `$0486`, `$0829`, `$08F1`) and VS mode
2's counters at `$77:0380-0404` not kept.

## Outcome

- The four stunt events (tracks 2, 7, 12, 17 and their tour repeats) playable with two humans in
  2P and VS, from NOW PLAYING through both riders' passes, the result and its continuation,
  frame-exact against captures of the original, with the cartridge RAM's fields (records,
  statistics, VS counters) equal.
- The two-human restart's records and VS mode 2's counters equal the original's.
