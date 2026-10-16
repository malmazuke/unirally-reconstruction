# PODIUM-AUDIO - the league podium's music

## Assignment

- Status: **queued** 16 October 2026 by [LEAGUE-TOURS](LEAGUE-TOURS.md) (R-0098). Unclaimed.
- Milestone: M4 (original game coverage).
- Review tier: **1** (sound cues, D-0010).
- Dependencies: MODE-AUDIO (R-0091), LEAGUE-TOURS (R-0098).

## Why

Native plays nothing on the league's podium, where the original starts `$82:E08E`'s sound session
and its music. That shows in `tour-cues` at 11420 and 11501-11502. The menu fade before the podium
and the title set after it are already native.

## Outcome

- The podium's session and music equal to the original's cue schedule, frame waits included.
- The app's audio producer runs through the podium.
