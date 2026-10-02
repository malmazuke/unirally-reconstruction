# CLASSIC-PAUSE-MENU - the original's pause menu picture

## Assignment

- Status: ready (prepared 3 October 2026 by the AUDIO-ONE-PLAYER session, from the user's live
  check).
- Milestone: M4 breadth
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: to be recorded at claim
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  the claiming session's model at default effort; **tier 2** (presentation only).
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): sample at claim.
- Reviewer (primary automatically spawns fresh model/effort, isolated checkout; no user
  trigger): a fresh Anthropic subagent with a withheld capture.
- Dependencies and evidence of acceptance: RACE-PAUSE-EXITS (R-0060), M4-16 (R-0035).
- Base commit: the `main` tip at claim.
- Branch and isolated worktree: `task/classic-pause-menu` in `.worktrees/classic-pause-menu`.
- Owned paths and shared interfaces: `draw_race_pause_menu` and its callers in
  `src/core/presentation.*`, the pack rules if new tiles are needed, native tests, a research
  record, this record, `docs/STATE.md`, `tasks/README.md`, `tasks/NEXT_SESSION.md`.
- Claim/lease/heartbeat/checkpoint location: this record.
- Session time limit, concurrency allocation and actual spend authorization if relevant: one
  worker plus the review subagent; no monetary spend.

## Outcome and boundaries

The user could not find a way to quit a race (AUDIO-ONE-PLAYER live check 1). The pause menu's
second choice already ends a race from the menus as the original's QUIT does (R-0060), but native
draws M4-16's authored panel: PAUSED, RESUME, RESTART RACE, a ">" marker and UP DOWN - ENTER
over a halved picture. The original (`$83:CD05`, `$83:F63E-F979`) dims the screen to brightness 7
and shows CONTINUE GAME and QUIT with a "<" in column 24.

Draw the original's pause menu in races from the menus, pixel-exact against captures. Keep the
standalone `--track` race's restart, and give it a label that says what it does. Behaviour, timing
and the frozen race gates do not move.

## Inputs and prerequisites

`local/evidence/race-pause-exits/decode/` (`quit`, `restart`, `lapquit`: frames with the menu
open), the static listing for `$83:F63E-F979`, the current pack.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Picture | Native pause frames against the captures' | Pixel-exact with the menu open, both choices | JSON |
| Nothing else moves | Gates, race sweep, v1 contracts, hidden runs, ctest, synthetic | Unchanged outside pause frames | gate logs |
| Review | Tier 2 | Approved with a withheld capture | review on the pull request |
