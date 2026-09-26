# FRONT-END-ENDINGS - the gold medal's endings and the reveal of new tours

## Assignment

- Status: **ready**. Queued 26 September 2026 (UTC) by FRONT-END-TOUR-END.
- Milestone: M4 (original game coverage: menus)
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: recorded at claim
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  **tier 2** (screens and their records).
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): recorded at claim.
- Reviewer (primary automatically spawns fresh model/effort, isolated checkout; no user
  trigger): a fresh Anthropic subagent.
- Dependencies and evidence of acceptance: FRONT-END-TOUR-END (R-0059, the award and the way back).
- Base commit: the `main` tip at claim.
- Branch and isolated worktree: `task/front-end-endings` in `.worktrees/front-end-endings`.
- Owned paths and shared interfaces: `src/core/award.cpp` and the front end's files, pack rules
  and content for the endings, native tests, a research record, this record, `docs/STATE.md`,
  `tasks/README.md`, `tasks/NEXT_SESSION.md`.
- Claim/lease/heartbeat/checkpoint location: this record.
- Session time limit, concurrency allocation and actual spend authorization if relevant: one
  worker plus the review subagent; no monetary spend.

## Outcome and boundaries

What R-0059 leaves:
- **The gold medal's endings.** A third completion of a tour plays that tour's ending
  (`$83:88FD`), one routine per tour: a gold award screen and a scripted animation, then the
  restore. HUNTER's (`$83:AB9A`) shows picture screens and ends in a soft reset (`$80:8858`).
  Native now leaves a gold completion at once through the award's way out.
- **The reveal.** Whenever the unlock rule's counts match, even with the level unchanged, PICK
  TOUR draws the tours of level - 1, slides, then shows the others four frames later
  (`$80:E588-E5A2`, `$77:10FD`). Native shows the level at once.
- **A completion by a fifth win**, captured from a preloaded cartridge RAM with four done tracks.
  The laboratory runner must then start from those records.

Make native do both, frame for frame. `local/evidence/front-end-tour-end/decode/tour-end.md`
section 5 outlines every ending.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Pictures and state | Captures of the original reaching each tour's gold by forced completions (preloaded cartridge RAM with silver medals, R-0050's method), and a reveal (four bronze medals), against native | Pictures match to the pixel and the words agree frame by frame, or each residue is explained | pictures, logs, research record |
| Fifth win | A capture from a preloaded cartridge RAM with four done tracks, the fifth race won | The same completion as a forced one, frame for frame | pictures, logs |
| Records | `sram.py` after each | The medals, levels and reveal equal the original's cartridge RAM | log |
| Nothing moves | The gates of FRONT-END-TOUR-END | Unchanged | logs |

## Handoff

- Exact next experiment/command: a preloaded cartridge RAM with CRAWLER at silver for MIKE, then
  `forced-bronze`'s inputs: the forced completion plays CRAWLER's ending (`$83:C49C`).
