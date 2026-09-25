# FRONT-END-1P-CONTINUATION - after a one-player race

## Assignment

- Status: **in progress**. Claimed 26 September 2026 at 20:55Z by the Claude Code desktop session
  that ran FRONT-END-1P-SETUP, on base `7d92810`.
- Milestone: M4 (original game coverage: menus)
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`)
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  **tier 2** for the screens; **tier 1** for anything that changes how a race is scored or
  started.
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): at claim the 5-hour window was 6% used and the weekly window 34%. The
  user's allowance: continue until the weekly window reaches 50%.
- Reviewer (primary automatically spawns fresh model/effort, isolated checkout; no user
  trigger): a fresh Anthropic subagent.
- Dependencies and evidence of acceptance: FRONT-END-1P-SETUP (the one-player screens, R-0055,
  R-0056); the race and its result screen (R-0046, R-0050, R-0053).
- Base commit: the `main` tip at claim.
- Branch and isolated worktree: `task/front-end-1p-continuation` in
  `.worktrees/front-end-1p-continuation`.
- Owned paths and shared interfaces: `src/core/front_end*` and the screen files, the app's race
  end, pack rules and content for the screens, native tests, a research record, this record,
  `docs/STATE.md`, `tasks/README.md`, `tasks/NEXT_SESSION.md`.
- Claim/lease/heartbeat/checkpoint location: this record.
- Session time limit, concurrency allocation and actual spend authorization if relevant: one
  worker plus the review subagent; no monetary spend.

## Outcome and boundaries

After a one-player race the original scores it, then returns to the tour (`$80:BC59-BC98`):
- `$83:879A` compares the times, or the stunt score with the qualifying score;
- a win marks the track done (`$77:1075`), and five done tracks raise the tour's medal and the
  rider's level (`$83:8827`, `$83:8853`);
- a loss sets `$77:0742` bit 12, which PICK TRACK counts down in `$77:1073`;
- then PICK TRACK comes again, with the done markers and the next track.

Make native do the same from the native race's result:
- the result's way back;
- the records: done tracks, medals, levels, best times and record holders;
- the tour's end: the pending reveal on PICK TOUR (`$77:10FD`);
- the game over after the tries run out.

Out of scope:
- saving the records across power cycles (SRAM persistence), unless the task finds it small;
- the stunt events' own rules (STUNT-EVENTS);
- the result screen's icons (RESULT-ICONS);
- audio.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Pictures and state | Every-frame captures of the original through a won and a lost race and back into the menus, against native | Pictures match to the pixel and the menus' words and records agree frame by frame, or each residue is explained | pictures, logs, research record |
| Records | The records after each captured race | Equal to the original's SRAM words, from a save-state or access capture of bank `$77` | log |
| Playable | The app from power-on through a tour's races | The menus follow each race as the original's do | report |
| Nothing moves | ctest, the synthetic suite, the v1 contracts, hidden runs, the differential gates, the equivalence sweep, the front end's comparisons | Unchanged | logs |

## Handoff

- Findings so far (listing only): R-0056's "What follows a race" notes and the PICK TRACK report
  (section 4.4: the scoring by race kind, the forced completion with Select + X + R after a
  race).
- Exact next experiment/command: a capture of the original racing DRAGSTER from the defaults and
  finishing (win and loss), with per-frame work RAM and bank `$77` accesses, over the race's end
  and the return to PICK TRACK.
