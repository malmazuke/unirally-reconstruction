# PODIUM-RETURN-TIMING - frame overruns in the league's menus

## Assignment

- Status: **queued** 16 October 2026 by [LEAGUE-TOURS](LEAGUE-TOURS.md) (R-0098). Unclaimed.
- Milestone: M4 (original game coverage).
- Review tier: **1** (menu timing in `src/core`).
- Dependencies: LEAGUE (R-0073), LEAGUE-TOURS (R-0098), D-0010 (upload timing).

## Why

After LEAGUE-TOURS every league race equals the original, but most later-tour menu pictures
differ by a frame, from two effects R-0098 records and native does not model:
- **The podium's way back.** In 4 of 11 podiums, and in 15 of 40 of the review's shifted exits,
  the original restores the menus a frame later. The title sound upload ends in one of two
  frames, with no pattern in the podium's phase or frame parity. After it the arrow's spin, and
  after a late one the palette cycle, stay a frame off until the league ends.
- **An event's pairs a frame apart.** In every observed event of a league with more than one
  pair (15 of 15, leagues of 3, 4 and 5), the standings after each pair but the last start a
  frame later. Native also counts some later pairs a frame early.

## Added by RECORD-SCREENS (24 October 2026)

- **RECORDS' entry with dense statistics.** `$80:D527` calls `$83:97BD`, whose 48 `$83:9D80`
  percentages run past RECORDS' entry frame when the riders have many races, so the original's
  menus are a frame behind native's from there (review capture `wr-stats2` from 901, main
  `local/evidence/record-screens-review/`; on main too). The same frame-cycle model would predict
  it ([R-0100](../docs/research/R-0100-record-screens.md) Not covered).

## Outcome

- Model what predicts each effect (the sound processor's handshake, or the frame's cycle count),
  or a fixed rule if the captures support one.
- Check each model against LEAGUE-TOURS' captures `tours-a`, `tours-b`, `odd-tours`, `five` and
  `four`, with the first tour's frozen gate unchanged.
