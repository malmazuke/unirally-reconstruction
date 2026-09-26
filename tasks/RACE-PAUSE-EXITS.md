# RACE-PAUSE-EXITS - the race's pause menu in one-player play: restart and quit

## Assignment

- Status: **in review**. Queued 26 September 2026 (UTC) by FRONT-END-LAP-RESULT's review (S1);
  claimed 26 September 2026 at 04:52Z by the same Claude Code desktop session, on `4b66150`.
- Milestone: M4 (original game coverage: menus)
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`)
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  **tier 1**: it changes how a race ends and is scored.
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): at claim the 5-hour window was 30% used and the weekly window 43%. The
  user's allowance: continue until the weekly window reaches 80%.
- Reviewer (primary automatically spawns fresh model/effort, isolated checkout; no user
  trigger): a fresh Anthropic subagent.
- Dependencies and evidence of acceptance: FRONT-END-LAP-RESULT (R-0058), FRONT-END-1P-CONTINUATION
  (R-0057; the listing report `local/evidence/front-end-1p-continuation/post-race.md` 1.1-1.2).
- Base commit: the `main` tip at claim.
- Branch and isolated worktree: `task/race-pause-exits` in `.worktrees/race-pause-exits`.
- Owned paths and shared interfaces: the race's pause menu and its restart, the front end's return
  and result, the app's race end, native tests, a research record, this record, `docs/STATE.md`,
  `tasks/README.md`, `tasks/NEXT_SESSION.md`.
- Claim/lease/heartbeat/checkpoint location: this record.
- Session time limit, concurrency allocation and actual spend authorization if relevant: one
  worker plus the review subagent; no monetary spend.

## Outcome and boundaries

In the original's one-player play the race's pause menu ends the race through the menus:
- its restart writes 0xEA62 into the quitter's total (`$83:F901`, `$83:F90B`), and `$80:88DD`
  then fades back to NOW PLAYING without scoring (8 frames);
- its quit writes 0xEA61 (`$83:F8DA`, `$83:F8EB`), zeroes the quitter's stunt score, and
  `$80:9A50` scores the quit (the `_quit___` row, the stunt tallies' counters);
- `$83:C9AC` also writes 0xEA62 at the race's start when `$0545` = 1 (meaning unknown).

The native race's pause menu restarts inside the race and has no quit, so in a 1P tour the app
skips NOW PLAYING on a restart and cannot quit. Make both go through the menus as the original's
do, frame for frame.

Out of scope: the stunt events (STUNT-EVENTS).

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Pictures and state | Captures of the original pausing a one-run and a lap race and choosing each option, against native (the front end's `compare.py` with the native race between the menus) | Pictures match to the pixel and the words agree frame by frame, or each residue is explained | pictures, logs, research record |
| Records | `sram.py` after a quit | The kept words equal the original's cartridge RAM | log |
| Nothing moves | ctest, the synthetic suite, the v1 contracts, hidden runs, the differential gates (the pause restarts they replay), the equivalence sweep, the front end's comparisons | Unchanged, or each change explained | logs |

## Evidence

R-0060 and `local/evidence/race-pause-exits/` (`NOTES.md`, `decode/` with the captures,
`pause-exits.md`, `compare.py`, `sram.py`).

| Criterion | Result |
| --- | --- |
| Pictures and state | `quit`, `restart` and `lapquit`: no difference in the menus' words, OAM buffer or text map on any frame from each race's end; 529 pictures equal (R-0060) |
| Records | `sram.py`: the kept words equal the original's cartridge RAM after each |
| Nothing moves | The gates, when run |

The race engine is unchanged: `update_race_for_menus` runs the ordinary update and, when the pause
menu's second choice restarted the race, keeps the race as it was and reports the pause menu's
total. The differential gates and the equivalence sweep, which replay pause restarts, therefore
see the same race.

## Handoff

- Findings: R-0060 and `decode/pause-exits.md`.
- Next: [FRONT-END-ENDINGS](FRONT-END-ENDINGS.md), or the other modes on the main menu
  (COVERAGE-ROADMAP).
