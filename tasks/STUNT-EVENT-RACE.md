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

## Result

[R-0066](../docs/research/R-0066-stunt-event-race.md). The nine stunt events run in the native race
engine as race mode 2: the header's 45-second clock counting down and its finish rule, the
opponent switched off (its pass skipped, so the queue cooldowns fall once an update), the
countdown's stunt phases, a finish sequence that waits for both riders to stand and both queues to
empty, captions by score against the qualifying score, the trick tallies, and two single-track
rules (BOWL's and track 42's vertical speed cap, track 37's 16-column playfield). A stunt event's
state is `URTRnn07`. Pack profile v24 adds the nine tracks' content. The laboratory's
track_reference compares a stunt event on its own scenario. The menus still show their notice for a
stunt event until STUNT-RESULT. A research worker decoded it; an implementation worker wrote it;
the primary integrated.

## Evidence

`local/evidence/stunt-events/decode/` (the research and its captures) and
`local/evidence/stunt-event-race/` (the two new riding captures, `compare.sh`, `nothing_moves.sh`,
`NOTES.md`).

| Criterion | Result |
| --- | --- |
| Stunt events | Every race row exact from the boundary through the result load on the seven riding captures (BOWL four times, HILL CLIMB twice, JUMPS, DOWNER: 2,808 to 2,952 rows each) and the nine idle captures of the nine tracks (1,502 to 1,571 rows each). |
| State | ROM-free tests round-trip a stunt event's state and refuse broken ones (`stunt_event_tests`). |
| Nothing moves | The gates: pending. |

## Handoff

- Exact next experiment/command: after the review and the merge, STUNT-RESULT (the result screen,
  records and way back to the menus), then STUNT-HUD.
