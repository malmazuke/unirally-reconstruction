# STUNT-HUD - a stunt event's race picture

## Assignment

- Status: **ready** after STUNT-EVENT-RACE. Queued 27 September 2026 (UTC) by STUNT-EVENT-RACE.
- Milestone: M4 (original game coverage)
- Tier: 2 (presentation).
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
