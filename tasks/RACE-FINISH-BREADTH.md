# RACE-FINISH-BREADTH - finishes and result screens of the cold-start race tracks

## Assignment

- Status: review. Claimed 24 September 2026 about 14:52Z by the session that closed RACE-GUARDS.
- Milestone: M4 breadth
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: Claude Opus 5.5 (`claude-opus-5-5`), Claude Code desktop, coordinator, primary and integrator
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  the claiming session's model at default effort. Review tier under D-0008: **tier 1** if the
  finish slowdown or result state in `src/core/movement.cpp` changes (expected), otherwise
  tier 2 for the laboratory tooling alone.
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): weekly all-models 5% and five-hour 12% at claim (14:52Z). Tier 1: the
  finish phase is race state.
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

- Native capability delivered / still missing: the finish phase from race start; comparison through finishes and the result load. Missing: a new lap race's player finish (R-0049 limits).
- Frozen exact-match interval, field set and reference/seed identity: new captures.
- Dynamic captured inputs still consumed (must be zero for autonomy): the landing matrices.
- Relevant branches/transitions exercised, including independent variations: boundaries 0, 1 and 2 mod 3; player won and lost; one-run and lap races (opponent finishes); the old rule as a control.
- First divergence and cheapest next discriminating experiment: see the handoff.
- Trial-wide usage baseline/current, reserve, reset authorization/outcome or none: at claim.

## Evidence and attempts

| Attempt | Hypothesis | Experiment | Observation | Next decision |
| --- | --- | --- | --- | --- |
| 1 (14:53Z) | The finish slowdown's third-update skip is not the absolute frame | Listing `$83:E90D`, `$83:CCAB`; `$0304` at the 20 boundaries | `$0304` counts race updates from 0 at every boundary | Phase from the boundary |
| 2 (14:57Z) | - | HYBRID's opponent finish with the new projection; old rule restored as a control | New exact 3,015; old diverges at 3,705 | Captures with finishes |
| 3 (15:01Z) | Right held finishes | Six captures to frame 8,000 | All exact; only EAST's player finishes | Native search for finishing inputs |
| 4 (15:10Z) | - | Search (Right, Left, jump patterns) on eight tracks | WARIO PAINT Right+B finishes; no lap race does | Capture WARIO PAINT and FLAT FUN (R-0046 input) |
| 5 (15:15Z) | - | Both through the result; old rule on all | Both exact through the stable result; old rule diverges after each first finish on every boundary not 2 mod 3 | Records, gates |

## Handoff

- Current base/head commit and uncommitted state: base `522a1ac` (`main` after #17); on
  `task/race-finish-breadth`.
- Verified findings: [R-0049](../docs/research/R-0049-race-finish-breadth.md).
- Current hypothesis and failed approaches: simple held inputs do not finish a new lap race.
- Commands executed, outcomes and report hashes: `captures.sh` and `explore` reports under
  `local/evidence/race-finish-breadth/`; recompare unchanged; tool tests 5/5; hidden 7,000-update
  WARIO PAINT run to a stable result. Gates: see the closing section.
- Unavailable/skipped checks: the ASan presets (host); the hosted Linux job covers them.
- Exact next experiment/command: none; [LOCKED-TOURS](LOCKED-TOURS.md) is next.
- Remaining dependencies: none.
- Runtime needs (network, build time, fixtures, memory): the 8,000-frame captures are about
  0.9 GB each.
- Aggregate parent/child time, provider usage before/after (or unknown), other-account-work
  caveat: primary from 14:52Z; figures in the closeout.
- Accepted outcome, review/fix rounds and next routing decision: see Review and integration.

## Review and integration

- Reviewer and independent reproduction/withheld-case results:
- Required changes or acceptance rationale:
- Exact merge candidate and required-check results:
- Integrated commit and evidence location:
- Remote synchronization: pushed ref(s), verified local/remote commit IDs, or exact push failure:
- Scope still unverified:
