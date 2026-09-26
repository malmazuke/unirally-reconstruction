# FRONT-END-LAP-RESULT - the lap race's result, and quit and restart

## Assignment

- Status: **in review**. Queued 25 September 2026 (UTC) by FRONT-END-1P-CONTINUATION; claimed
  25 September 2026 at 23:50Z by the same Claude Code desktop session, on FRONT-END-1P-CONTINUATION's
  head `689fefb` (pull request #30, to be rebased onto `main` once it merges).
- Milestone: M4 (original game coverage: menus)
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`)
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  **tier 2** for the screen; **tier 1** for anything that changes how a race is scored or ended.
  Kept at tier 2: the win test and the race engine are unchanged; the records' change for a lap
  race (the best laps, `$80:C868`) is a direct transcription, checked against the listing by the
  worker and the reviewer, against cartridge RAM at four frames (`sram.py`), and by native tests
  of a tie and a no-time race (the reviewer's condition for not escalating).
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
| Pictures and state | `lap-won`, `lap-lost`, `lap-record` and `lap-record-frames`: the native races return on the original's frames (6725, 7659, 13416); no difference in the menus' words, OAM buffer or text map on any frame from r + 101 to the end; 4,104 pictures equal |
| Records | `sram.py`: the kept cartridge RAM words equal the original's at lap-won 7610 and 8399 and lap-lost 8610 and 9399 |
| Playable | Every race the app starts (MIKE against BRONSEN, not a stunt event) comes back to the menus. The runner's comparisons cover the lap race; the app cannot replay a driven lap race (its race input is a fixed mask), so its hidden runs cover the one-run hand-over and the shared code |
| Nothing moves | Gates on `003a7b3` (`gates-003a7b3.out`; the head after it changes only records): three presets build, ctest 26 of 26; the synthetic suite; both v1 contracts; the hidden runs, and cont-win's and cont-loss's pads through the app; the eleven differential gates (the same rows digests); the equivalence sweep, 351 runs with 0 differences; both recompares identical; the main menu's, the one-player screens', the one-run and the lap results' comparisons all equal; the records equal; 0 functions over 80 lines; the address index current |

Quit and restart: a known difference, not done here. The original's restart goes back to NOW
PLAYING and its quit is scored (R-0058 "Not recovered"); the native race's pause menu restarts
inside the race and has no quit. Queued as [RACE-PAUSE-EXITS](RACE-PAUSE-EXITS.md).

## Handoff

- Findings: R-0058 and `decode/lap-result.md`, which checks the graph rule against every pass of
  `lap-won`.
- The race's loading varies by a frame (R-0039): the laboratory's `compare.py` aligns each native
  race to the capture's own initialization (`--race-initialization`).
- Next: [FRONT-END-TOUR-END](FRONT-END-TOUR-END.md), then [RACE-PAUSE-EXITS](RACE-PAUSE-EXITS.md)
  (the race's pause menu in one-player play, a known difference). Its first captures (`forced-bronze`,
  `forced-bronze-long`) and a decode of the completion, the award screen and PICK TOUR's return
  are ready in `local/evidence/front-end-tour-end/`.
