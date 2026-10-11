# TWO-HUMAN-STUNTS - stunt events with two humans: 2P and VS

## Assignment

- Status: **accepted** (tier 1, PR #72, approved in review round 3 at `0c70896`), claimed 10 October 2026 on main `ef8e440`, in the session the user asked
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
- Gates: `local/evidence/two-human-stunts/gates.sh` (from CREDITS-NAME's; since review round 2 it also runs `pass4/checks4.sh`, the review's W1-W4b and the two-human restore checks) against main `ef8e440`'s
  binaries (`base-ef8e440/`), pack v37 for main and v38 for the candidate, under the shared lock.

## Review round 1 (PR #72, approved with should-fix)

- Fixed in the third pass, each reproduced on the review's captures (`local/evidence/two-human-stunts/pass3/`):
  pad 2 on NOW PLAYING, PICK TOUR and PICK TRACK through the `$0742` gate (W2's pad-2 B at 9850);
  Y from a screen the continuation opened (W1 equal on every menu frame); VS CHAMPIONS' row frame
  from the number of records placed (W2b's VS CHAMPIONS equal); `front_end_runner --restore-check`
  for layout M (the review's 11 checks pass, now in `pass3/checks3.sh`).
- Advisories: the league limit is now exact (R-0095's Not covered: a one-scanline table print
  overrun in organic-three's prefix; the awards press and league scoring fixed), and the second
  race's result words are recorded as the `$0618` league-total write. organic-stunt-tricks,
  -three-events and -two-race league frames are equal; organic-full-tour-turnaround 31/35 (main 28).
- Reruns on the final code: race sweep 432 runs 0 differences; front-end sweep 112/179 equal, the
  same 67 differing, none further from the original but the four organic-three manifests; league
  frozen 0 px; R-0076 cues identical to main; this task's five cue schedules equal the original.

## Handoff (round 2)

- Candidate `6c3cf32`: the third pass (`pass3/`) on `3825120`, plus main merged (no `src/` change).
  Pass 3's checks ran on the uncommitted third pass, which is the same code as `6c3cf32`'s `src/`;
  the gates at `6c3cf32` (`gates-6c3cf32.out`) rerun them through `pass3/checks3.sh`.

## Review round 2 (returned) and the fourth pass

- Blocker fixed: after a VS challenger pick the pad gate holds through the continuation and its
  screens and opens (`$80:C0E0`) only on NOW PLAYING's Race or QUIT (`$80:C0DD JSR $ADE3`); W4
  equal to its end (9250/9250 pictures).
- Should-fix fixed: the menus' slide word `$00AC` is kept at its writers, so the challenger menu
  slides forward after a continuation Y (`$80:CB42-CB48`, `$80:ADFA`); W4b 6800/6800.
- Advisories: NEXT TRACK wraps and skips unopened tours (`$80:AE96-AECC`, `$83:9F14`; new capture
  `twop-next-wrap`); OPTIONS' delay, hidden markers and DEFINE PLAYER's track fixed (W2b equal but
  15672); `$77:10B1`'s 57-frame window recorded in R-0095.
- Fourth-pass results in `local/evidence/two-human-stunts/pass4/final/`: race sweep 0 differences;
  front-end sweep 61/179 equal against main, the 51 more differing in hidden state only (OPTIONS,
  league slots), pictures as main's, none further from the original but the recorded league ones.

## Review round 3 (approved)

- Approved at `0c70896` (https://github.com/malmazuke/unirally-reconstruction/pull/72#pullrequestreview-5483242370):
  W4 and W4b equal, the slide word's writers and NEXT TRACK's loop checked against the ROM, and a
  new capture W5 confirming the OPTIONS change (6 of 51 OPTIONS manifests closer, none further).
  Gates at `0c70896` passed (`local/evidence/two-human-stunts/gates-0c70896.out`).
- Advisories handed to [MENU-INPUT](MENU-INPUT.md): OPTIONS' hidden markers after the rename
  keyboard, and DEFINE PLAYER's pick writing `$017D` (W5).
