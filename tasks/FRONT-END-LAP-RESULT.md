# FRONT-END-LAP-RESULT - the lap race's result, and quit and restart

## Assignment

- Status: **ready**. Queued 26 September 2026 by FRONT-END-1P-CONTINUATION.
- Milestone: M4 (original game coverage: menus)
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: recorded at claim
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  **tier 2** for the screen; **tier 1** for anything that changes how a race is scored or ended.
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): recorded at claim.
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

## Handoff

- The native race's loading time between NOW PLAYING's fade and its initialization is known for
  DRAGSTER only (121 frames on the laboratory's path). `front_end_runner` times no other track.
  Measure ZOOM ZOO's from a capture (the first frame `$0FF1` moves).
- Exact next experiment/command: a capture from the defaults with Down on PICK TRACK (ZOOM ZOO),
  racing three laps, then the result and a press, with work RAM every frame.
