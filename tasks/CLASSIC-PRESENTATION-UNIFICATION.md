# CLASSIC-PRESENTATION-UNIFICATION - One renderer for both tracks

## Assignment

- Status: planned (queued 18 September 2026 at the user's request; start after the DRAGSTER window-effects review closes)
- Milestone: follow-up to M4-16, DRAGSTER-ORDINARY-CONTROLS and DRAGSTER-WINDOW-EFFECTS
- Coordinator: Claude Opus 5 primary session
- Base commit: current `main`
- Branch and isolated worktree: `task/classic-presentation-unification`, `.worktrees/classic-presentation-unification`
- Reviewer: fresh Claude Opus 5 subagent at the exact candidate

## Why

The simulation scaled and the presentation did not. DRAGSTER now runs the
shared recovered race engine with track content as data, and inherited the
10:00 clock limit with no new code. Presentation still has two renderers:
`render_dragster` (M3: tile inventories captured at five frames, the last pose
held, approximations keyed to rider poses) and `render_zoom_zoo` (M4-16: rider
objects composed from the original's pose tables, the look animation, recovered
HUD, fade and palette cycle).

That split is history, not design: DRAGSTER's renderer was accepted when the
bar was "a few frozen frames within a threshold", which admits approximations;
M4-16's bar was "playable", which forced recovery. Sharing has since happened
one routine at a time and reactively - the palette cycle, its colour inside the
window shapes, then the window table family - each time because the evidence
showed the original's routine is track-independent.

The user asked directly whether the codebase is being architected so that this
does not scale. On presentation, yes, and this task is the answer.

## Outcome and boundaries

One renderer driven by track content, with DRAGSTER's own frozen frames as the
check that it still matches the original. Expected to clear, together: held
rider art, the approximate fade, the authored pause overlay, the static result
background, and the opponent-banner gap left by DRAGSTER-WINDOW-EFFECTS.

Fold in the naming and content-model cleanups this exposed:
- `presentation.zoom.race-palette-cycle.v1` is read by DRAGSTER; the shared
  entries should not be track-named.
- Content is siloed per track, which is why a DRAGSTER-only pack can no longer
  play DRAGSTER. Engine content and track content should be separable.
- `PresentationContent` is an aggregate whose every new member edits four call
  sites; a pack-backed accessor would not.
- Remove the pose-keyed flags that exist only to disable one track's guess.

Out of scope: audio, menus, other tracks and modes.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| DRAGSTER frozen contracts | `native presentation-check` winner and loser | counts unchanged or lower at every frame | reports |
| ZOOM ZOO visual contract | the M4-16 frozen scenes | unchanged or closer to the original | comparison |
| Rider art | DRAGSTER live and headless runs | zero held-pose fallback frames | run logs |
| Original agreement beyond frozen frames | picture comparison across a race on both tracks | measured, recorded, no regression | comparison report |
| Gates | four presets, historical matrix, DRAGSTER and ZOOM ZOO differential gates | all pass | gate logs |
| Review and CI | fresh reviewer; hosted CI on the exact tip | approve; green | review, closeout |

## Handoff

- Not started. Begin by measuring what `render_zoom_zoo` already reproduces for
  DRAGSTER's frozen frames when given DRAGSTER content, and record the first
  divergence, as the controls task did for the engine.
