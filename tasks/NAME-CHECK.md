# NAME-CHECK - the keyboard's name check

## Assignment

- Status: **claimed** 27 October 2026 04:31 UTC on main `1cf871fa`, on the user's instruction
  (Claude Code desktop app, Claude Opus 5.5 `claude-opus-5-5`; coordinator and integrator; a worker
  subagent implements, a fresh subagent reviews). Queued 14 October 2026 by
  [MENU-INPUT](MENU-INPUT.md) (R-0097). The first task under
  [D-0013](../docs/decisions/D-0013-task-time-and-disk-budget.md): its claim-to-merge time is
  measured against the 6-hour target.
- Choice: the next queued player-visible gap (names with listed words are accepted natively).
  PODIUM-RETURN-TIMING needs the same cycle model this task may build, so it follows.
- Branch `task/name-check` in `.worktrees/name-check`; evidence in main `local/evidence/name-check/`.
- Task provider: Anthropic.
- Milestone: M4 (original game coverage).
- Review tier: **1** (menu state and frame timing in `src/core`).
- Dependencies: MENU-INPUT (R-0097), OPTIONS (R-0072).

## Why

The rename keyboard's OK runs `$83:84D9`, which compares the name with 71 words (`$83:8537`) at
every position and on a match prints `$80:A4C9` and refuses it (`$80:A4B8-A4C6`). Native has no
check and accepts every name. The check also takes about 39 scanlines a character, so from about
eight characters OK's frame ends without its frame wait and everything after comes a frame later
(every full-length rider name; `league-name`, review `rv-kb`).

## Outcome

- The refusal: the word list read from the ROM at pack build time (not committed), the message
  and the keyboard staying open, captured with names that contain listed words.
- The lag: a model of the check's run time that predicts the frame OK ends on, checked against
  captures of names of every length; or, if it cannot be predicted from the front end's state,
  a recorded limit with the evidence.
