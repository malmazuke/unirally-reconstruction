# DRAGSTER-PALETTE-CYCLE - Frame-driven DRAGSTER start-line palette

## Assignment

- Status: review (implementation complete on `task/dragster-palette-cycle`; independent review pending)
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
| Cycle is right beyond frozen frames | original CGRAM read from strict serialized state on every frame of the winner and loser replays, twice each | every racing frame matches; loading rule matches through loser update 75 | R-0037 table, `artifacts/dragster-palette-cycle/cgram-*.json` |
| Content versioning | decision recorded; v1 packs keep accepted rendering, packs carrying the tables draw the cycle | no new pack version needed (two-track v7 carries the tables); v1 checks unchanged | R-0037, tests |
| Regressions | four-preset tests, DRAGSTER differentials and restores | all pass | gate logs |
| Independent review | fresh reviewer at the exact candidate | approve | review report |

## Evidence and attempts

| Attempt | Hypothesis | Experiment | Observation | Next decision |
| --- | --- | --- | --- | --- |
| 1 | DRAGSTER runs the ZOOM ZOO palette routine | Scan six existing DRAGSTER access captures for `$82:D382-D496` and all `$0B84/$0B92` writes | Routine runs every race frame from 1334; `$0B84` ends frame n at `(n-1333)&15` in every capture; the two fixed arrays are exactly phases 10 and 7; the accepted frames sit on those phases | Implement after M4-16 lands |
| 2 | The cycle holds on every frame, and loading freezes it | Original-only CGRAM probe on the winner and loser scenarios, each twice | Racing frames 1,854/1,854 and 1,959/1,959 match; loading holds the start frame's colours 96-111 with black colour 0 until the result palettes load at loser update 76 | Implement with v1 compatibility |
| 3 | Implementation keeps accepted checks and draws the cycle with tables present | Four presets; v1 winner/loser presentation checks; same cases with pack v7 through the checker's comparison; mutation of the start frame | 22/22 tests on four presets; counts unchanged in both; mutant fails the new test | Independent review |

**Decision (17 September 2026):** no new DRAGSTER pack version. The two-track
pack v7 already carries every DRAGSTER entry and the cycle tables, so DRAGSTER
draws the recovered cycle from it. DRAGSTER v1 packs keep the accepted
pose-keyed palette, so no accepted contract, fixture or historical gate changes.
The default DRAGSTER-only pack is unchanged; launching DRAGSTER with the
two-track pack gives the recovered palette. A later DRAGSTER pack version could
add the tables if the DRAGSTER-only pack remains a supported product path.

## Handoff

- Current head: implementation, tests, R-0037 and this record on `task/dragster-palette-cycle`.
- Verified findings: R-0037.
- Next: fresh independent review at the exact candidate, then integration with CI on the exact tip.
- Not done: a native pixel sweep at non-frozen DRAGSTER frames; the palette after loser loading update 75.
