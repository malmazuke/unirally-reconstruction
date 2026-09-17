# DRAGSTER-ORDINARY-CONTROLS - Playable DRAGSTER controls

## Assignment

- Status: in_progress
- Milestone: follow-up to accepted DRAGSTER gameplay (M2-M3, M4-01); not an M4 milestone gate
- Coordinator: Claude Opus 5 primary session (Claude Code desktop)
- Task provider: Anthropic (Claude Opus 5), per D-0004
- Worker/session/runtime/model: primary and subagent workers, all Claude Opus 5
- Provider quota: plan telemetry is available through the app usage tool. Session start 2026-09-17T14:57Z: 5-hour window 9% (resets 16:20Z), weekly 6% (resets 2026-09-24T08:00Z), extra usage disabled. **User budget for this overnight run: stop at 100% of the 5-hour window or 50% of the weekly limit, whichever comes first.** No reset, purchase or provider change.
- Reviewer: fresh Claude Opus 5 subagent in an isolated checkout at the exact candidate
- Dependencies: M4-16 (recovered ZOOM ZOO engine), DRAGSTER-PALETTE-CYCLE; both on main at `abc38e6`
- Base commit: `abc38e6`
- Branch and isolated worktree: `task/dragster-ordinary-controls`, `.worktrees/dragster-ordinary-controls`
- Owned paths: DRAGSTER native simulation and its frontend dispatch, DRAGSTER tests/manifests/research, this record, README/STATE DRAGSTER wording
- Checkpoint location: this record

## Outcome and boundaries

Native DRAGSTER must not abort on ordinary controls. Recover, from original
evidence, what the original does with jump while riding, brake, Left and
reversal, and the tricks and rewards they reach, so a player can race DRAGSTER
with keyboard or gamepad like ZOOM ZOO. Accepted DRAGSTER differential,
restore, result and presentation gates must keep passing unchanged. Out of
scope: DRAGSTER's 10:00 limit and window timing/shape (separate follow-ups,
unless the recovery naturally covers them), audio, menus.

## Inputs and prerequisites

Supported PAL ROM and audited bsnes core (identities in docs/STATE.md); the
two-track pack v7 (`b75539a0...`); accepted DRAGSTER replay manifests under
`tests/manifests/replay/` and native cases under `tests/manifests/native/`.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Current behaviour recorded | `artifacts/dragster-ordinary-controls/controls-probe.sh` | aborts listed below | probe summary (done) |
| Original evidence for each recovered control | original-only DRAGSTER timelines, captured twice with identical digests | frozen before native tuning | freezes and research note |
| Native matches the original on those timelines | differential comparison over the declared state projection, with fresh-process restores | every declared byte matches | reports |
| Accepted DRAGSTER gates unchanged | historical matrix (compare, restore, finish, opponent-first, presentation) | all pass with unchanged expectations | gate logs |
| No abort on ordinary controls | the probe, plus random and ordinary input sequences over complete races | no fail-closed abort; any remaining guard documented and unreachable in ordinary play | probe and fuzz logs |
| Live play | user plays DRAGSTER with gamepad and keyboard through result and restart | no abort | parked until the user returns |
| Independent review, CI | fresh reviewer; hosted CI on exact tip | approve; green | review, closeout |

## Evidence and attempts

| Attempt | Hypothesis | Experiment | Observation | Next decision |
| --- | --- | --- | --- | --- |
| 1 | Ordinary controls abort native DRAGSTER | User live playtest with an Xbox controller, then the probe holding each button with Right for 2,200 updates on `abc38e6` | User run: "moving brake is outside the recovered movement domain" after 1.3 s. Probe: Right alone, and Right with Up, Down, Y, A, X, L or R, reach the result (phase 3, player wins); Right+B aborts "unrecovered landing velocity transform"; Left aborts "leftward movement is outside the recovered primary domain"; no input stays at the start | Investigate the source mapping and the shared ZOOM ZOO engine |

Notes on attempt 1:
- **Native DRAGSTER feeds SNES B to both brake and jump.** `update_movement`
  passes B to both `update_horizontal`'s brake and `update_jump`. ZOOM ZOO's
  engine, from `$82:AAE2-AAFB` (R-0032), maps B to jump `$0331` and Y to brake
  `$0325`. So jump (keyboard Z, gamepad South, which is Xbox A) held while
  riding either aborts or reaches the unrecovered landing path. Y does nothing
  in native DRAGSTER, although it is the original's brake. That is a silent
  divergence, not an abort.
- **The accepted DRAGSTER scenarios never press B while moving.** They hold
  Right, sometimes release it, and hold Up, so no gate exercised this path.

## Handoff

- Current head: this record and the README/STATE limitation note.
- Next: compare native DRAGSTER's update with the recovered ZOOM ZOO engine and
  choose between porting DRAGSTER onto that engine and extending DRAGSTER's own
  path, by the first divergence against original DRAGSTER evidence.
