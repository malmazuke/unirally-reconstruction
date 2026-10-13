# MENU-INPUT - menu input the listing shows and native lacks

## Assignment

- Status: **claimed** 14 October 2026 on main `e304868` (Claude Code desktop app, Claude Opus 5.5
  `claude-opus-5-5`; coordinator and integrator; a worker subagent implements). Queued 10 October
  2026 by [MODE-AUDIO](MODE-AUDIO.md) (R-0091). Weekly usage at claim 14%.
- Choice: the oldest queued row and native game behaviour still missing (the full-coverage goal);
  ahead of R-0089's LEAGUE-TOURS, CARTRIDGE-OPTION-BITS and RACE-LISTING-BRANCHES and of
  GATES-AGAINST-MAIN, which are larger or infrastructure. No other session holds a claim.
- Branch `task/menu-input` in `.worktrees/menu-input`; evidence in main `local/evidence/menu-input/`.
- Task provider: Anthropic.
- Milestone: M4 (original game coverage).
- Review tier: **1** (menu state in `src/core`: the front end's input handling and its state words).
- Dependencies: OPTIONS (R-0072), LEAGUE (R-0073), TWO-PLAYER-VS (R-0071).

## Why

Placing the menus' sounds (R-0091) read input handling native does not have:

- the keyboard: Start goes straight to OK (`$80:B916`), and held directions repeat;
- the league member picker takes A as well as Start (`$80:B8C4`) and marks with B, Y or X;
- VS refuses the champion as challenger (`$80:C0B1-C0BB`, with `$80:B0FA`'s sound);
- the league awards take a press only after `$80:B051`;
- the two-player continuation menu wraps at its ends (the review's `rv-twop-quit`: a fifth Down
  picks NEXT TRACK in the original, QUIT in native);
- the original's second-player menu does not slide (`$80:CBC3`);
- GROUP TABLES' Up and Down change the tour shown (`$80:DF37-DF77`), which native lacks;
- PLAYER SCORES' Down also takes Select and Up is tested first (`$80:B794`).

## Outcome

Each captured from the original and made native, with the menus' frozen comparisons unchanged
elsewhere.

## Added by TWO-HUMAN-STUNTS (review round 3, 11 October 2026)

- OPTIONS' hidden PICK TRACK markers still differ after returning from the rename keyboard (W5:
  939 frames against main's 1251); the generic list loop's marker turn (`$80:EB23`) is native only
  in OPTIONS and the continuation, not the league slots or RECORDS.
- DEFINE PLAYER's pick writes the rider word `$017D` natively (2) but not in the original (0), so
  the next 1P rider menu starts on another rider (W5 from frame 2943). Capture:
  `local/evidence/two-human-stunts-review/` W5.
