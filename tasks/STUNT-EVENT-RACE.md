# STUNT-EVENT-RACE - the stunt events in the race engine

## Assignment

- Status: **in progress**. Queued 27 September 2026 (UTC) by the STUNT-EVENTS research (COVERAGE-ROADMAP
  item 9); claimed 27 September 2026 at 02:20Z by the Claude Code desktop session that ran
  FIFTH-WIN-COMPLETION, on `94b3034`. The implementation worker started on `e01aaaa` while
  FIFTH-WIN-COMPLETION was in its gates; its commits were moved onto this claim.
- Milestone: M4 (original game coverage)
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`)
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  **tier 1**: it changes race state.
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): at claim the 5-hour window was 37% used and the weekly window 64%. The
  user's allowance: continue until the weekly window reaches 80%.
- Reviewer (primary automatically spawns fresh model/effort, isolated checkout; no user
  trigger): a fresh Anthropic subagent.
- Dependencies and evidence of acceptance: the race engine on all 36 race tracks (R-0046, R-0050,
  R-0052), the riders and opponents (R-0061), the STUNT-EVENTS research
  (`local/evidence/stunt-events/decode/stunt-events.md`).
- Base commit: the `main` tip at claim.
- Branch and isolated worktree: `task/stunt-event-race` in `.worktrees/stunt-event-race`.
- Owned paths and shared interfaces: the race engine's scenarios, rules and state, the content
  pack (profile v24), the laboratory's track_reference tooling, native tests, a research record,
  this record, `docs/STATE.md`, `tasks/README.md`, `tasks/NEXT_SESSION.md`.
- Claim/lease/heartbeat/checkpoint location: this record.
- Session time limit, concurrency allocation and actual spend authorization if relevant: one
  research worker, one implementation worker, the review subagent; no monetary spend.

## Outcome and boundaries

Each tour's third track is a stunt event: a solo run against the clock for points, with a
qualifying score. Native refuses all nine. Run them in the race engine exactly as the original
does, from the boundary to the result load: the clock, the opponent switched off, the countdown,
the finish, the scoring and captions, the trick tallies, all saved in the race state. Not in scope:
the stunt event's race picture (STUNT-HUD) and its result screen, records and way back to the
menus (STUNT-RESULT); the menus keep their notice for a stunt event until then.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Stunt events | `track_reference explore` on the research's captures (BOWL, HILL CLIMB), new JUMPS and DOWNER riding captures, and the nine tracks' idle captures | Every race row exact from the boundary through the result load | logs |
| State | Save and restore of a stunt event's state | Round trip, restore checks pass | tests |
| Nothing moves | The gates of FIFTH-WIN-COMPLETION | The race tracks unchanged | logs |

## Handoff

- Exact next experiment/command: integrate the implementation worker's commits, run the gates.
