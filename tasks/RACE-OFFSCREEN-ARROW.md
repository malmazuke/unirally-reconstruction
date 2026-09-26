# RACE-OFFSCREEN-ARROW - the race's off-screen rider arrows

## Assignment

- Status: **ready**. Queued 26 September 2026 (UTC) by RACE-RIDERS-OPPONENTS.
- Milestone: M4 (original game coverage)
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: recorded at claim
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  **tier 2** (the race's picture), unless the arrow needs race state the engine does not carry.
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): recorded at claim.
- Reviewer (primary automatically spawns fresh model/effort, isolated checkout; no user
  trigger): a fresh Anthropic subagent.
- Dependencies and evidence of acceptance: the race HUD (R-0043), the riders and opponents
  (R-0061).
- Base commit: the `main` tip at claim.
- Branch and isolated worktree: `task/race-offscreen-arrow` in `.worktrees/race-offscreen-arrow`.
- Owned paths and shared interfaces: the race's HUD and picture (`race_hud.cpp`,
  `presentation.cpp`), native tests, a research record, this record, `docs/STATE.md`,
  `tasks/README.md`, `tasks/NEXT_SESSION.md`.
- Claim/lease/heartbeat/checkpoint location: this record.
- Session time limit, concurrency allocation and actual spend authorization if relevant: one
  worker plus the review subagent; no monetary spend.

## Outcome and boundaries

When the other rider is off screen, the original draws an arrow in the HUD's ink, in the race's text layer (BG3),
at rows 14-15: columns 5-7 when it is behind (on the left) and 26-28 when it is ahead (on the
right). Native has never drawn it; R-0043 declared it an omission. R-0061's captures show both
sides: SILVIA and GOLDWYN get ahead of the player, and the arrows differ on 26 to 32 of those
races' 43-51 kept frames. They are the only difference left in those races' pictures. Find its writer,
what it reads (the riders' positions, and maybe the distance), and when it appears and goes, and
draw it frame for frame.

Out of scope: two-player play.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Pictures | `local/evidence/race-riders-opponents/race/pictures.py` on R-0061's six captures, and the M4-16 primary's kept frames | 0 differing pixels, or each residue explained | logs, research record |
| Timing | Consecutive frames around the arrow's appearance and disappearance, both sides | Equal to the pixel | pictures |
| Nothing moves | The gates of RACE-RIDERS-OPPONENTS | Unchanged apart from the arrow's pixels, each accounted for | logs |

## Handoff

- Exact next experiment/command: find the BG3 writes at rows 14-15 in the listing (the tilemap's
  words for columns 5-7 and 26-28), starting from R-0043's writers (`$81:D1B2-$81:D30C`,
  `$81:EB44-$81:EB83`), then capture consecutive frames from `silvia-dragster` around frame 1790.
