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

## Checkpoint - 10 October 2026 (implementation subagent's first pass, committed as WIP)

- Implemented: rider 1's stunt tallies (`StuntEvent::opponent_tallies`, `$77:07D5-0824`), its
  release once settled (`$82:AB62-AB8E`), the split HUD's two scores, save layout `URTRnn0M`, the
  stunt result rewritten after `$80:F0EE-F2E9`/`$80:F669` (two riders' columns; it also fixes main's
  one-player stunt result regression from LEAGUE: STUNT-RESULT's five captures now equal), both
  riders' quit words, VS counters `$77:0380/0382/0400` and `$77:10F9/10FB`, VS CHAMPIONS ranking,
  REMATCH on a tie (pack v38: `front-end.vs-rematch-text`), the challenger pick, every mode's race
  loading from the song table (R-0090's two-human restart record differences came from a one-frame
  loading error and are now equal).
- Evidence: main `local/evidence/two-human-stunts/` (nine captures, `checks.sh`,
  `checks/run1.out`, `base-ef8e440/`), sound schedules in `two-human-stunts-audio/`.
- Results: race rows equal on six captures (2P/VS BOWL, JUMPS, HILL CLIMB, a zero-score tie);
  result text/objects, continuation, VS CHAMPIONS, challenger, REMATCH equal; cartridge RAM record
  fields equal; five sound schedules equal.
- Open before review:
  1. VS DOWNER (track 32): rider 1's landing at frame 3558 (vertical velocity -288 native, -308
     original; no landing-matrix row gives -308).
  2. Frozen league picture `organic-full-tour-turnaround` frame 38500 now differs (the podium exit's
     arrow a frame early, previously hidden by the old stunt result's late arrow); the league
     frozen gate needs the v29 pack (missing locally).
  3. 2P/VS pictures' animated objects (result, NOW PLAYING, VS CHAMPIONS) differ by 105-1369 pixels:
     native's second-rider pick slides; the original reprints PICK ANOTHER in place
     (`$80:BCDB-BCEF`). Probably MENU-INPUT's (`$80:CBC3`) item.
  4. Rider 1's horizontal input while pad 2 has paused the race (48 frames).
  5. The front-end sweep against main (24 of 179 differed before the last fixes) must be rerun and
     classified; R-0095 must cite the 31 addresses the native-symbols check lists.

## Checkpoint - second pass

- The subagent's second pass: the second pick in place (`$80:CBC3`), PICK TOUR/TRACK and the
  continuation for two humans, the result's entry 108, pad 2's words during a pause, the league
  podium arrow and POINT AWARDS' decorations. Item 1 (DOWNER) is a hardware timing effect
  (R-0095's Not covered). Results and sweeps in `checks/pass2/` (R-0095).

## Review candidate

- Records: [R-0095](../docs/research/R-0095-two-human-stunts.md). Pack v38.
- Gates: `local/evidence/two-human-stunts/gates.sh` (from CREDITS-NAME's) against main `ef8e440`'s
  binaries (`base-ef8e440/`), pack v37 for main and v38 for the candidate, under the shared lock.
