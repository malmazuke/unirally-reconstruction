# TILE-PAIRS-8-12-26 - the tile flag pairs that still stop four locked tracks

## Assignment

- Status: claimed 25 September 2026 by the Claude Code desktop session that ran LOCKED-TOURS
  (after a context compaction), base `0de428f`.
- Milestone: M4 breadth
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`)
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  the claiming session's model at default effort; **tier 1** (simulation state in
  `src/core/movement.cpp` and `vertical_contact.cpp`).
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): at claim (25 September 2026) the 5-hour window was 7% used and the weekly
  window 9%; the user's stop point is 50% weekly.
- Reviewer (primary automatically spawns fresh model/effort, isolated checkout; no user
  trigger): a fresh Anthropic subagent with a withheld capture.
- Dependencies and evidence of acceptance: LOCKED-TOURS (accepted); R-0047, R-0050; pack v12.
- Base commit: the `main` tip at claim.
- Branch and isolated worktree: `task/tile-pairs` in `.worktrees/tile-pairs`.
- Owned paths and shared interfaces: the tile dispatch and contact code, native tests, a new
  research record, this record, `docs/STATE.md`, `tasks/README.md`, `tasks/NEXT_SESSION.md`.
- Claim/lease/heartbeat/checkpoint location: this record.
- Session time limit, concurrency allocation and actual spend authorization if relevant: one
  worker plus the review subagent; no monetary spend.

## Outcome and boundaries

Four locked race tracks stop at unrecovered tile flag pairs (R-0050):
- LAST ONE (15) at update 808, contact pair 26;
- DOWN+UP (25) at 519, contact pair 26;
- JUMPOVER (19) at 773, movement pair 12;
- HIGHROAD (26) at 898, contact pair 8 or 26.

Recover each pair the captures reach, in contact (`$81:9185-91D1`: pair 26 clears probe
penetrations when `$0355,y` = 9; pair 8 sets `$1349` and changes the correction) and in movement
(the table at `$81:82F5`: pair 8 at `$81:8554`, 12 at `$81:8950`, 26 at `$81:837E`). Also
the HUNTER tracks' announcement differences (TWO LOOPS at 1,252, event 57 against 31; HUNTER 44
with Right held at 1,620, 62 against 30; R-0050).
Out of scope: stunt events.

## Inputs and prerequisites

The locked-tour captures under `local/evidence/locked-tours/sweep/` (`sweep.json`) and
`recompare-5.json`; the listing under `artifacts/static-map/`; pack v12.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Pairs named | Native's error and the original's WRAM at each stop | Which pair, which path | research record |
| Match | `recompare --per-track` on the locked sweep; longer captures | Each of the four exact past its stop | JSON |
| Nothing accepted moves | Gates, v1 contracts, hidden runs, fuzz, ctest, synthetic | Unchanged digests | gate logs |
| Review | Tier 1 | Approved with a withheld capture | review on the pull request |

## Handoff

- Exact next experiment/command: `python3 -m tools.unirally_lab.native.track_reference recompare
  --per-track --sweep local/evidence/locked-tours/sweep --binary
  build/lab-debug/src/core/zoom_zoo_runner --pack local/classic-pal-crawler-tracks-v12.pack --out
  <json>`, then read the original's `$0355,y`, `$1349` and `$0DE7` at each stop.
