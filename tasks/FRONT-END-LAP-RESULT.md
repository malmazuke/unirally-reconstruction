# FRONT-END-LAP-RESULT - the lap race's result, and quit and restart

## Assignment

- Status: **in progress**. Queued 25 September 2026 (UTC) by FRONT-END-1P-CONTINUATION; claimed
  25 September 2026 at 23:50Z by the same Claude Code desktop session, on FRONT-END-1P-CONTINUATION's
  head `689fefb` (pull request #30, to be rebased onto `main` once it merges).
- Milestone: M4 (original game coverage: menus)
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`)
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  **tier 2** for the screen; **tier 1** for anything that changes how a race is scored or ended.
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): at claim the 5-hour window was 36% used and the weekly window 38%. The
  user's allowance (25 September 2026): continue until the weekly window reaches 80%.
- Reviewer (primary automatically spawns fresh model/effort, isolated checkout; no user
  trigger): a fresh Anthropic subagent.
- Dependencies and evidence of acceptance: FRONT-END-1P-CONTINUATION (R-0057, the one-run result
  and the race's return); the lap race and its graph extrema (R-0038, R-0049).
- Base commit: the `main` tip at claim.
- Branch and isolated worktree: `task/front-end-lap-result` in `.worktrees/front-end-lap-result`.
- Owned paths and shared interfaces: `src/core/race_result.cpp` and the front end's files, the
  app's race end, pack rules and content for the screen, native tests, a research record, this
  record, `docs/STATE.md`, `tasks/README.md`, `tasks/NEXT_SESSION.md`.
- Claim/lease/heartbeat/checkpoint location: this record.
- Session time limit, concurrency allocation and actual spend authorization if relevant: one
  worker plus the review subagent; no monetary spend.

## Outcome and boundaries

A lap race's result is the menus' too (`$80:951C` type 1, `$80:8D6E-910E`, R-0057's listing
report):
- the lap graph: twenty objects flying to each lap's time (`$83:904A`'s extrema, `$83:8D5D`);
- the record line (`$80:918F`), the rows (`$80:910F`, `$80:91A1`, `$80:91B9`);
- the best lap as the personal best;
- `$80:98B3`'s animation until any button, with no release wait.

Make native do the same, so every race of a tour but the stunt event comes back to PICK TRACK in
the app. Also the race's quit and restart as the menus see them:
- `$80:9A50`'s quit scoring (0xEA61);
- `$80:88DD`'s restart to NOW PLAYING (0xEA62);
- if the native race's pause menu can reach them.

Out of scope: the stunt events (STUNT-EVENTS) and the tour's end (FRONT-END-TOUR-END).

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Pictures and state | Every-frame captures of the original through a won and a lost lap race (ZOOM ZOO) and back to PICK TRACK, against native (`compare.py` with the native race between the menus) | Pictures match to the pixel and the menus' words agree frame by frame, or each residue is explained | pictures, logs, research record |
| Records | `sram.py` after each captured race | The kept words equal the original's cartridge RAM | log |
| Playable | The app from power-on through a tour's races | Each race but the stunt event comes back to PICK TRACK | report |
| Nothing moves | ctest, the synthetic suite, the v1 contracts, hidden runs, the differential gates, the equivalence sweep, the front end's comparisons | Unchanged | logs |

## Evidence

R-0058 and `local/evidence/front-end-lap-result/` (`NOTES.md`, `decode/lap-result.md`,
`compare.py`, `sram.py`).

| Criterion | Result |
| --- | --- |
| Pictures and state | `lap-won`, `lap-lost`, `lap-record` and `lap-record-frames`: the native races return on the original's frames (6725, 7659, 13455); no difference in the menus' words, OAM buffer or text map on any frame from r + 101 to the end; 4,104 pictures equal |
| Records | `sram.py`: the kept cartridge RAM words equal the original's at lap-won 7610 and 8399 and lap-lost 8610 and 9399 |
| Playable | Every native race comes back to the menus in the app. The runner's comparisons cover the lap race; the app cannot replay a driven lap race (its race input is a fixed mask), so its hidden runs cover the one-run hand-over and the shared code |
| Nothing moves | The gates (below) |

Quit and restart: the native race's pause menu restarts the race itself, and nothing hands 0xEA61
or 0xEA62 to the menus, so there is nothing to score. They stay with the pause menu's own
recovery.

## Handoff


- The native race's loading time between NOW PLAYING's fade and its initialization is known for
  DRAGSTER only (121 frames on the laboratory's path). `front_end_runner` times no other track.
  Measure ZOOM ZOO's from a capture (the first frame `$0FF1` moves).
- `$77:0742` bit 8 matters here. `$80:9805` parks entries 0-29 (and `$0CF0`, `$0DE0`) only
  while the bit is clear, and sets it. The first result after a cold start sets it, and it stays
  set (it is in `$0742`, across races) until the lap result's graph loop clears it
  (`$80:98CB`). So later one-run results skip the parking. Native keeps neither the bit nor
  those tables and always parks; `$80:A09A` has already parked those entries, so the OAM agrees.
  The lap graph uses entries 0-19.
- Holding Right alone never finishes ZOOM ZOO: `lap-explore` circles the loop at 0 of 3 laps.
- The capture exists: `local/evidence/front-end-lap-result/lap-won` (8,400 frames). It uses
  M4-16 boundary-a's inputs, which drive three laps to 6724: MIKE 1:38.02 against BRONSEN
  1:38.10, a win. Start at 7600 leaves the lap result, and PICK TRACK shows ZOOM ZOO done with
  the cursor on BOWL. It has work RAM every frame and pictures 6700-8399.
- Exact next experiment/command: run the native race between the menus for ZOOM ZOO in
  `front_end_runner` (measure its loading frames from the capture), then compare with a copy of
  the continuation's `compare.py`.
