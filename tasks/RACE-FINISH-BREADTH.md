# RACE-FINISH-BREADTH - finishes and result screens of the cold-start race tracks

## Assignment

- Status: ready (prepared 24 September 2026 by the RACE-GUARDS session).
- Milestone: M4 breadth
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: to be recorded at claim
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  the claiming session's model at default effort. Review tier under D-0008: **tier 1** if the
  finish slowdown or result state in `src/core/movement.cpp` changes (expected), otherwise
  tier 2 for the laboratory tooling alone.
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): sample at claim and record here.
- Reviewer (primary automatically spawns fresh model/effort, isolated checkout; no user
  trigger): a fresh Anthropic subagent at the exact candidate, with a withheld finish of its own.
- Dependencies and evidence of acceptance: RACE-GUARDS (accepted); R-0046, R-0047, R-0048;
  pack v11.
- Base commit: the `main` tip at claim.
- Branch and isolated worktree: `task/race-finish-breadth` in `.worktrees/race-finish-breadth`.
- Owned paths and shared interfaces: the finish and result paths of `src/core/movement.cpp`
  (`apply_finish_slowdown`, `update_zoom_finish`, result loading), `track_reference` (a
  projection that continues through the finish and the result load), native tests, a new
  research record `docs/research/R-0049-race-finish-breadth.md`, this record, `docs/STATE.md`,
  `tasks/README.md`, `tasks/NEXT_SESSION.md`, R-0046's matrix. The 742-byte ZOOM ZOO and DRAGSTER
  layouts are read-only.
- Claim/lease/heartbeat/checkpoint location: this record.
- Session time limit, concurrency allocation and actual spend authorization if relevant: one
  active worker plus the review subagent; no monetary spend.

## Outcome and boundaries

Every one of the 16 cold-start race tracks now plays to its finish in native with no guard
(R-0048), but no new track's finish, winner banner or result load has been compared with the
original: `track_reference explore` stops at the first finish ("result projection is outside
this exploration"). R-0046 names one likely fault: the finish slowdown counts the absolute
frame modulo 3, both accepted tracks start on a frame congruent to 2, and 9 of the 14 new
tracks do not.

Done means, for at least one one-run track and one lap race among the new tracks (and each
track whose boundary differs mod 3 from the accepted ones, if the rule turns out to depend
on it), native matches the original update for update from the boundary through both finishes
and the result load to the stable result screen, as the M4-15/M4-16 contracts do for ZOOM ZOO
and DRAGSTER. Out of scope: the authored lap-race result screen's pictures (declared), stunt
events, locked tours, audio.

## Inputs and prerequisites

- ROM, pinned core, pack v11, as for R-0047. Held-Right captures reach finishes: EAST's player
  at frame 5,581 (R-0046 observation 18), HYBRID's opponent at frame 3,703
  (`local/evidence/race-guards/hybrid-released`).
- The accepted finish/result comparison is `zoom_zoo_playable` / `dragster_playable`; their
  projection of the result load (`result_updates`, graph extrema, published totals) is the
  model for extending `track_reference`.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Finish rule | Listing of the finish slowdown (`$83:E90D-E932`) and its frame counter; original WRAM around a new track's finish | The counter the slowdown uses, cited | R-0049 |
| Projection | `track_reference` continues past a finish through the result load | Tool test; ZOOM ZOO and DRAGSTER captures still reproduce their accepted rows | tool test, JSON |
| Match | Captures to the stable result on the chosen tracks | Exact through both finishes and the result load | explore JSON |
| Nothing accepted moves | The eleven differential gates, v1 contracts, hidden runs, fuzz, ctest, synthetic suite | All pass, unchanged digests | gate logs |
| Review | Tier as above | Approved, with a withheld finish | review on the pull request |

## Capability and coverage checkpoint

- Native capability delivered / still missing: to be filled.
- Frozen exact-match interval, field set and reference/seed identity: new captures.
- Dynamic captured inputs still consumed (must be zero for autonomy): the landing matrices.
- Relevant branches/transitions exercised, including independent variations: to be filled.
- First divergence and cheapest next discriminating experiment: see the handoff.
- Trial-wide usage baseline/current, reserve, reset authorization/outcome or none: at claim.

## Evidence and attempts

| Attempt | Hypothesis | Experiment | Observation | Next decision |
| --- | --- | --- | --- | --- |

## Handoff

- Current base/head commit and uncommitted state: not claimed; prepared on `task/race-guards`.
- Verified findings: none yet.
- Current hypothesis and failed approaches: the finish slowdown's modulo-3 counter is race
  relative (like `$0300`), not the absolute frame.
- Commands executed, outcomes and report hashes: none yet.
- Unavailable/skipped checks: the ASan presets are unavailable on this host.
- Exact next experiment/command: capture EAST with Right held to frame 7,000 (tour row 3,
  position 3, `--hold 1415 right`) and read `$0300` and the finish slowdown's inputs in WRAM
  around the player's finish at 5,581; then read the listing at `$83:E90D`.
- Remaining dependencies: none outside the project.
- Runtime needs (network, build time, fixtures, memory): about 45 minutes for the gates. Put
  the gate directory under the worktree's `artifacts/` (`presentation-check` refuses others),
  and copy `local/toolchain` into the worktree rather than linking it (a linked toolchain
  lets the worktree's bootstrap rewrite the main checkout's manifest).
- Aggregate parent/child time, provider usage before/after (or unknown), other-account-work
  caveat: to be recorded.
- Accepted outcome, review/fix rounds and next routing decision: to be recorded.

## Review and integration

- Reviewer and independent reproduction/withheld-case results:
- Required changes or acceptance rationale:
- Exact merge candidate and required-check results:
- Integrated commit and evidence location:
- Remote synchronization: pushed ref(s), verified local/remote commit IDs, or exact push failure:
- Scope still unverified:
