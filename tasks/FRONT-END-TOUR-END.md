# FRONT-END-TOUR-END - a tour's completion: the award, the endings and the unlocks

## Assignment

- Status: **ready**. Queued 26 September 2026 by FRONT-END-1P-CONTINUATION.
- Milestone: M4 (original game coverage: menus)
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: recorded at claim
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  **tier 2** for the screens; **tier 1** for anything that changes the scoring.
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): recorded at claim.
- Reviewer (primary automatically spawns fresh model/effort, isolated checkout; no user
  trigger): a fresh Anthropic subagent.
- Dependencies and evidence of acceptance: FRONT-END-1P-CONTINUATION (R-0057); PICK TOUR's
  levels and badges (R-0056); the locked tours' preload (R-0050).
- Base commit: the `main` tip at claim.
- Branch and isolated worktree: `task/front-end-tour-end` in `.worktrees/front-end-tour-end`.
- Owned paths and shared interfaces: the front end's files, pack rules and content for the
  screens, native tests, a research record, this record, `docs/STATE.md`, `tasks/README.md`,
  `tasks/NEXT_SESSION.md`.
- Claim/lease/heartbeat/checkpoint location: this record.
- Session time limit, concurrency allocation and actual spend authorization if relevant: one
  worker plus the review subagent; no monetary spend.

## Outcome and boundaries

The fifth done track of a tour completes it (`$83:879A`, `$83:881B-88D2`, R-0057's listing
report):
- the done flags cleared;
- the medal raised by one, up to gold;
- the award screen (`$83:AEF6`) or, at gold, the tour's ending (`$83:88FD`);
- the rider's level (`$77:10D3`) and the pending reveal (`$77:10FD`), by exact counts;
- PICK TOUR called from inside the scoring, with the newly opened tours revealed;
- then PICK TRACK with no test of Y or X.

Pad 1 exactly Select + X + R on the scoring's frame forces the completion for any race kind. This
gives captures a way to reach it without winning five races. A preloaded cartridge RAM with four
done tracks is the other (R-0050's method).

Native refuses a fifth done track today (`score_race`). Make it do what the original does.

Out of scope: the stunt events (STUNT-EVENTS); the endings' music (audio).

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Pictures and state | Every-frame captures of the original through a forced completion (bronze, then silver, then gold with the ending) and through a completion by a fifth win | Pictures match to the pixel and the menus' words agree frame by frame, or each residue is explained | pictures, logs, research record |
| Records | `sram.py` after each completion | The medals, levels and reveal equal the original's cartridge RAM | log |
| Playable | The app through a completion | The award, the reveal on PICK TOUR, and the next tour | report |
| Nothing moves | ctest, the synthetic suite, the v1 contracts, hidden runs, the differential gates, the equivalence sweep, the front end's comparisons | Unchanged | logs |

## Handoff

- Exact next experiment/command: `cont-win`'s inputs (`local/evidence/front-end-1p-continuation`)
  with pad 1 holding exactly Select + X + R through the press that leaves the result (3799-3801).
  Capture with work RAM every frame to the award screen and PICK TOUR's reveal.
