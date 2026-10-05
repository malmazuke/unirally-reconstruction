# SPLIT-HUD-GAPS - the split race's captions, lower arrow and lower colour math

## Assignment

- Status: ready (prepared 5 October 2026 by the SPLIT-PAUSE-MENU session, from R-0079's split
  picture classes and R-0071's residual).
- Milestone: M4 breadth
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider (fixed for all children; record any user-initiated platform change): to be
  recorded at claim
- Worker/session/runtime/model: to be recorded at claim
- Actual model/reasoning effort, routing rationale: **tier 2** (D-0008: presentation on recovered
  layers; the HUD history is presentation-only).
- Provider quota window (D-0004): sample at claim.
- Reviewer: one independent round with a withheld capture.
- Dependencies: SPLIT-PAUSE-MENU (R-0079), TWO-PLAYER-VS (R-0071), LEAGUE (R-0073).
- Base commit: the `main` tip at claim.
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
2. **The lower view's side arrow** in column 28 of rows 20-21, which native does not draw.
3. **The lower view's colour math** where its rider covers its BG3 ink: the original darkens the
   rider; native adds the upper view's measured red (R-0042).

Also the CONTINUE GAME clear of the pauser's rows in a split race (R-0079 "Not covered"), which
needs (1) first. Recover them from the split HUD routine and the colour math HDMA, and make the
pictures of R-0079's captures and `twop-plain` exact. Race state does not move.

## Inputs and prerequisites

Main `local/evidence/split-pause-menu/` (`twop-plain`, `twop`, `twop2`, `league`, `classify.py`),
the static listing for `$81:D853-E8C8`.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Pictures | `classify.py` over R-0079's captures | No caption, arrow or colour-math class left | JSON |
| Nothing else moves | Gates, sweeps, R-0078/R-0079 captures | Unchanged elsewhere | gate logs |
| Review | Tier 2 | Approved with a withheld capture | review on the pull request |
