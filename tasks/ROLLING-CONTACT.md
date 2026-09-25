# ROLLING-CONTACT - a rolling rider's second contact after a long shoulder rotation

## Assignment

- Status: ready (prepared 25 September 2026 by the HUNTER-EFFECTS session, from its review).
- Milestone: M4 breadth
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: to be recorded at claim
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  the claiming session's model at default effort; **tier 1** (`src/core/vertical_contact.cpp`,
  movement).
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): sample at claim.
- Reviewer (primary automatically spawns fresh model/effort, isolated checkout; no user
  trigger): a fresh Anthropic subagent with a withheld capture.
- Dependencies and evidence of acceptance: HUNTER-EFFECTS (R-0052).
- Base commit: the `main` tip at claim.
- Branch and isolated worktree: `task/rolling-contact` in `.worktrees/rolling-contact`.
- Owned paths and shared interfaces: the contact and pose code, native tests, a research
  record, this record, `docs/STATE.md`, `tasks/README.md`, `tasks/NEXT_SESSION.md`.
- Claim/lease/heartbeat/checkpoint location: this record.
- Session time limit, concurrency allocation and actual spend authorization if relevant: one
  worker plus the review subagent; no monetary spend.

## Outcome and boundaries

HUNTER-EFFECTS' review capture `w-rev-buttons` (TWO LOOPS, Right from 1,585 with Y, A, L, R, B,
X and Left under effect 7) matches to update 1,737 (frame 3,128). There the player's velocity y
is 72 natively and 0 in the original. The published inputs are equal through the divergence.

It is the second contact of a rolling rider (surface mode 1, orientation 37 to 32) after a
32-update airborne rotation on the shoulder buttons. No HUNTER word is read on that path, so the
cause is in the shared contact code.

Find the branch native misses, and make the capture exact past 1,737.

## Inputs and prerequisites

`local/evidence/hunter-effects/review-withheld/w-rev-buttons` (in the main checkout after
HUNTER-EFFECTS' closeout), pack v14, the listings under `artifacts/static-map/`.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Cause named | WRAM around frame 3,128 and the contact listing | The branch and its words | research record |
| Match | `track_reference explore` on the capture; a withheld rolling capture | Exact past 1,737 | JSON |
| Nothing accepted moves | Gates, v1 contracts, hidden runs, fuzz, ctest, synthetic | Unchanged digests | gate logs |
| Review | Tier 1 | Approved with a withheld capture | review on the pull request |

## Handoff

- Exact next experiment/command: `python3 -m tools.unirally_lab.native.track_reference explore
  --reference local/evidence/hunter-effects/review-withheld/w-rev-buttons --binary
  build/lab-debug/src/core/zoom_zoo_runner --pack local/classic-pal-crawler-tracks-v15.pack
  --scenario classic.track.41 --out <json>`, then read the contact words (`$0F33`, `$0F2B`,
  `$0F55`, `$0F57`, `$0FAB`) on frames 3,120-3,128.
