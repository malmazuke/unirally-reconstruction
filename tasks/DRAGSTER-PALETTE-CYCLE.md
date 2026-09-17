# DRAGSTER-PALETTE-CYCLE - Frame-driven DRAGSTER start-line palette

## Assignment

- Status: in_progress (finding verified; native implementation waits for M4-16 integration)
- Milestone: follow-up to accepted DRAGSTER presentation (M3/M4-01); not an M4 milestone gate
- Coordinator: Claude Opus 5 primary session that is also integrating M4-16
- Task provider: Anthropic (Claude Opus 5), as recorded in D-0004 for the current work
- Worker/session/runtime/model: same primary session, Claude Code desktop, Claude Opus 5
- Actual model/reasoning effort, routing rationale: primary; follow-up the user started from an M4-16 finding
- Provider quota window: no percentage telemetry under this provider; UTC wall clock recorded instead
- Reviewer: fresh Claude Opus 5 subagent in an isolated checkout at the exact candidate, before integration
- Dependencies: M4-16 integration. It adds the race palette cycle helper and the two-track pack v7 entry, and edits the same renderer and pack registry, so implementation starts from main after M4-16 lands.
- Base commit: `f599565` (origin/main at 17 September 2026)
- Branch and isolated worktree: `task/dragster-palette-cycle`, `.worktrees/dragster-palette-cycle`
- Owned paths: DRAGSTER branch of `src/core/presentation.cpp` palette construction, DRAGSTER content rules and pack registry (additive version), related tests, this record, R-0037
- Claim/checkpoint location: this record
- Spend authorization: none needed

## Outcome and boundaries

Replace DRAGSTER's pose-keyed late-finish palette with the original race NMI
palette cycle `$82:D382-D496` (R-0037), so DRAGSTER's checkered start/finish
line animates as in the original on every race frame. Out of scope: DRAGSTER's
10:00 clock limit (separate follow-up), ZOOM ZOO (done in M4-16), audio.

## Inputs and prerequisites

Supported ROM (see R-0037 hashes); the private DRAGSTER access captures listed
in R-0037; the accepted DRAGSTER presentation fixtures and
`tests/manifests/presentation/classic-crawler-dragster-v1.json` and
`-loser-v1.json`.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Original rule | R-0037 capture analysis | `(n-1334)&15` predicts every first `$0B84` write in six captures | R-0037 (done) |
| Accepted contracts do not regress | `native presentation-check` winner and loser | counts unchanged or lower at every frame | reports |
| Cycle is right beyond frozen frames | digest-checked original recapture of DRAGSTER race frames across all sixteen phases | start-line colours equal the original at every sampled frame | comparison report |
| Content versioning | fresh extraction twice; old pack behaviour explicit | byte-identical packs; compatibility documented and tested | pack reports, tests |
| Regressions | four-preset tests, DRAGSTER differentials and restores | all pass | gate logs |
| Independent review | fresh reviewer at the exact candidate | approve | review report |

## Evidence and attempts

| Attempt | Hypothesis | Experiment | Observation | Next decision |
| --- | --- | --- | --- | --- |
| 1 | DRAGSTER runs the ZOOM ZOO palette routine | Scan six existing DRAGSTER access captures for `$82:D382-D496` and all `$0B84/$0B92` writes | Routine runs every race frame from 1334; `$0B84` ends frame n at `(n-1333)&15` in every capture; the two fixed arrays are exactly phases 10 and 7; the accepted frames sit on those phases | Implement after M4-16 lands |

## Handoff

- Current head: this record and R-0037 only; no code change yet.
- Verified findings: R-0037.
- Exact next step: after M4-16 is on main, merge main here, add the cycle to the DRAGSTER renderer from an additive DRAGSTER content version, and run the acceptance table.
- Remaining dependencies: M4-16 integration.
