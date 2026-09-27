# STUNT-RESULT - a stunt event's result, records and way back to the menus

## Assignment

- Status: **ready**. Queued 27 September 2026 (UTC) by STUNT-EVENT-RACE.
- Milestone: M4 (original game coverage)
- Tier: 2 (screens and their records), as the earlier result tasks.
- Dependencies: STUNT-EVENT-RACE (R-0066), the one-run and lap results (R-0057, R-0058), the
  fifth-win route (R-0065).

## Outcome and boundaries

After a stunt event's race, the menus' stunt result (`$80:F0EE-F2E9`, the decode's section 2): the
screen and its tally animation, the records written at the result (the rider's best, the score
history, checksums), the shared exit wait, the statistics and top three (`$80:C948`, `$80:8CCB`),
the win test (`$83:88E1`) with the done or loss flag, and PICK TRACK. Also the pause QUIT's stunt
words, the removal of the app's notice for a stunt event, and native's cold-start `tries`, which
holds 3 where the original holds 0 until the rider's choice (R-0064).

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Result | Front-end comparisons from each race's return to PICK TRACK on bowl-lose and hill-win | 0 differing pixels, equal menu state, OAM buffer and text map | logs |
| Records | `sram.py` at the result, the tally's end, the exit and the end | Equal to the original's cartridge RAM | logs |
| Completion | A tour completed by a stunt win (R-0065's route) | The award frame for frame | logs |
| Nothing moves | The gates of STUNT-EVENT-RACE | Unchanged | logs |

## Handoff

- Research: `local/evidence/stunt-events/decode/stunt-events.md` section 2, and the captures
  `bowl-lose`, `hill-win` (both leave the result with a press).
