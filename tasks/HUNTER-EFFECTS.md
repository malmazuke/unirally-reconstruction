# HUNTER-EFFECTS - the HUNTER tour's tag effects

## Assignment

- Status: claimed 25 September 2026 by the Claude Code desktop session that ran
  TILE-PAIRS-8-12-26, base `47708b4`.
- Milestone: M4 breadth
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`)
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  the claiming session's model at default effort; **tier 1** (simulation state in
  `src/core/movement.cpp` and the announcement queue).
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): at claim the 5-hour window was 7% used and the weekly window 12%; the
  user's stop point is 50% weekly.
- Reviewer (primary automatically spawns fresh model/effort, isolated checkout; no user
  trigger): a fresh Anthropic subagent with a withheld HUNTER capture.
- Dependencies and evidence of acceptance: TILE-PAIRS-8-12-26 (accepted); R-0050, R-0051;
  pack v13.
- Base commit: the `main` tip at claim.
- Branch and isolated worktree: `task/hunter-effects` in `.worktrees/hunter-effects`.
- Owned paths and shared interfaces: the race update in `src/core/movement.cpp` and its state
  layout, the announcement queue, presentation of any effect that changes the picture, native
  tests, a new research record, this record, `docs/STATE.md`, `tasks/README.md`,
  `tasks/NEXT_SESSION.md`.
- Claim/lease/heartbeat/checkpoint location: this record.
- Session time limit, concurrency allocation and actual spend authorization if relevant: one
  worker plus the review subagent; no monetary spend.

## Outcome and boundaries

On the HUNTER tour (`$131F` nonzero; tracks 40-44) the race runs `$83:CEC9` every update
(from `$83:CDB1`), which native lacks. R-0051 read it statically:

- `$83:D104` tests whether the two riders' boxes overlap (player x+8 to x+40 and y to y+40
  against the opponent's). It runs only while no effect is running (`$1323`), neither rider
  has finished, and once the riders' progress words `$0FCD` and `$0FCF` have differed by 2 or
  more (latched in `$1325`). An overlap sets `$1321`, `$1323` and one of eight effect flags,
  `$1327 + 2*(player x & 7)`.
- Each effect flag, the update it becomes 1, pushes its announcement to the front of the
  player queue (`$81:C55B`: written at the read cursor, which then steps back), plays sound
  `$021F`, sets `$12AF`, clears the other flags and runs its routine: `$1327` event `$1F`
  (`$83:D474`), `$1329` `$1C` (`$83:D530`), `$132B` `$1E` (`$83:D4AE`), `$132D` `$1B`
  (`$83:D2C9`), `$132F` `$20` (`$83:D3FC`), `$1331` `$1D` (`$83:D4E8`), `$1333` `$21`
  (`$83:D349`), `$1335` `$22` (`$83:D28A`).
- Most routines run a 500-update timer and end the effect with event `$23` (`$83:D275` also
  resets `$128D-$12AD`); the timed words (`$0557`, `$055B`, `$055D`, `$055F`, `$0561`, `$12B5`,
  `$12B7`, `$12CD`, `$7E:2052`, `$7E:2054`, `$1285-$128B`) and a blink table at `$83:D3BC`
  feed consumers elsewhere that are not yet traced. `$12D1` selects a palette fade instead
  (`$83:D1CA`).

Recover the trigger, each effect the HUNTER captures reach, and their consumers; add the state
to the race layout; show the effects in the presentation. Out of scope: stunt events.

## Inputs and prerequisites

The locked-tour captures (`local/evidence/locked-tours/sweep/hunter-*`), the listing under
`artifacts/static-map/` (`bank-83.lst` `$83:CEC9-D600`; the extract is
`local/evidence/tile-pairs/hunter-83CEC9.txt`), pack v13. Held-controller captures are needed
to reach more effects (the player's x at the tag picks the effect).

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Effects named | Static reading and WRAM at each tag | Trigger, each reached effect, its consumers | research record |
| Match | `recompare --per-track` on the locked sweep; held captures on 40-44 | TWO LOOPS and HUNTER 44 exact past 1,252 and 1,620 | JSON |
| Nothing accepted moves | Gates, v1 contracts, hidden runs, fuzz, ctest, synthetic | Unchanged digests | gate logs |
| Review | Tier 1 | Approved with a withheld capture | review on the pull request |

## Handoff

- Exact next experiment/command: `python3 -m tools.unirally_lab.native.track_reference explore
  --reference local/evidence/locked-tours/sweep/hunter-1 --binary
  build/lab-debug/src/core/zoom_zoo_runner --pack local/classic-pal-crawler-tracks-v13.pack
  --scenario classic.track.41 --out <json>` (diverges at update 1,252, the first tag: effect
  `$1327`, event 31); then read `$1321-$1345`, `$12AF` and the effect's timer at each tag.
