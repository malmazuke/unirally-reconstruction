# MENU-INPUT - menu input the listing shows and native lacks

## Assignment

- Status: **queued** 10 October 2026 by [MODE-AUDIO](MODE-AUDIO.md) (R-0091). Unclaimed.
- Milestone: M4 (original game coverage).
- Review tier: **1** (menu state).
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
