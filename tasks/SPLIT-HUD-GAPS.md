# SPLIT-HUD-GAPS - the split race's captions, lower arrow and lower colour math

## Assignment

- Status: **claimed** 5 October 2026 by the session that closed SPLIT-PAUSE-MENU, on main
  `52f7a79` after that task's merge (main equal to `origin/main`, closeout written). Prepared the
  same day from R-0079's split picture classes and R-0071's residual.
- Milestone: M4 breadth
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`); one
  session is coordinator, primary and integrator; a read-only Claude Opus 5.5 subagent reads the
  split HUD listing
- Actual model/reasoning effort, routing rationale: **tier 2** (D-0008: presentation on recovered
  layers; the HUD history is presentation-only).
- Provider quota window (D-0004): at claim (5 October 2026 about 14:50 Sydney) weekly 40%,
  five-hour 7%; standing rule: continue until weekly 80%.
- Reviewer: one independent round with a withheld capture.
- Dependencies: SPLIT-PAUSE-MENU (R-0079), TWO-PLAYER-VS (R-0071), LEAGUE (R-0073).
- Base commit: `52f7a79` (main at claim).
- Branch and isolated worktree: `task/split-hud-gaps` in `.worktrees/split-hud-gaps`.
- Owned paths: `src/core/race_hud.*` (the split HUD), `src/core/presentation.*` (object colour
  math), native tests, a research record, this record, `docs/STATE.md`, `tasks/README.md`,
  `tasks/NEXT_SESSION.md`.

## Outcome and boundaries

A two-human split race's picture still differs from the original's in three places R-0079 measured
(a race with no pause shows all three; in `twop-plain` 353 of 530 pictures differ):

1. **Tutorial captions.** The split HUD (`$81:D853`, chosen by `$0DE1` at `$80:8803`) prints each
   rider's caption in its own view, rows 5-6 and 19-20 from column 8; native prints the player's in
   the one-player rows 10-11 (`$81:F311`) and the lower rider's not at all.
2. **The lower view's side arrow** in column 28 of rows 20-21 (and column 5 on ZOOM ZOO: the
   review's `zz2p` 2431, 2495, 2653-2689), which native does not draw.
3. **The lower view's colour math** where its rider covers its BG3 ink: the original darkens the
   rider; native adds the upper view's measured red (R-0042).

Also, found by SPLIT-PAUSE-MENU's review: in 2P the original darkens a finished rider's view while
the race goes on (`drfin` 3967-3989, about 27,000 pixels; native dims only a league pair's), and the
finished split HUD differs in the timer cell and shows TO ROLL where native shows WINNER (`drfin`
3967-4014). And the CONTINUE GAME clear of the pauser's rows in a split race (R-0079 "Not
covered"), which needs (1) first. Recover them from the split HUD routine and the colour math HDMA, and make the
pictures of R-0079's captures and `twop-plain` exact. Race state does not move.

## Inputs and prerequisites

Main `local/evidence/split-pause-menu/` (`twop-plain`, `twop`, `twop2`, `league`, the review's
`zz2p` and `drfin`, `classify.py`),
the static listing for `$81:D853-E8C8`.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Pictures | `classify.py` over R-0079's captures | No caption, arrow or colour-math class left | JSON |
| Nothing else moves | Gates, sweeps, R-0078/R-0079 captures | Unchanged elsewhere | gate logs |
| Review | Tier 2 | Approved with a withheld capture | review on the pull request |
