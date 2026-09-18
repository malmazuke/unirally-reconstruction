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

## Finding from the 18 September 2026 playtest - the launcher substitutes packs

The user ran DRAGSTER with the superseded v1 pack, expecting a refusal:

```
python3 tools/project.py frontend run --track dragster \
  --pack local/classic-crawler-dragster.pack --preset app-debug --updates 20 --hidden
```

It did not refuse. It validated the named pack, decided that pack can no longer
play DRAGSTER, searched `local/` for a substitute and ran
`classic-crawler-two-tracks-v7.pack` instead, reporting the substitution as a
passing check:

```
[ passed] classic_pack (required): validated existing pack .../classic-crawler-dragster.pack
[ passed] dragster_two_track_pack (required): the DRAGSTER-only pack lacks the shared
          race tables; using .../classic-crawler-two-tracks-v7.pack
[ passed] frontend_launch (required): Classic pack validated: ".../classic-crawler-two-tracks-v7.pack"
```

The race that ran was not the pack that was asked for. Two separable defects,
both in `tools/unirally_lab/frontend/commands.py`:

**An explicit `--pack` cannot be distinguished from the default.** `--pack`
defaults to `local/classic-crawler-dragster.pack`, the same path the user
typed, so `_dragster_two_track_pack` cannot tell an upgrade of an unstated
default from an override of a stated flag. Upgrading the default is the right
behaviour and should stay; overriding a flag the user typed should be an error
that names the profile the track needs. `default=None` separates the two cases.

**Substitutes are found by filename, not by profile.** `TWO_TRACK_PACK_NAMES`
is a hardcoded tuple of pack filenames, so every profile bump edits it - v8 was
added to it in DRAGSTER-WINDOW-EFFECTS - and a name that no longer exists
(`classic-crawler-two-tracks.pack`) sits in it unnoticed, as does the same
stale name in the `zoom-zoo` rewrite at the top of the module. The profile is
recorded inside each pack and is what `validate_pack` already checks; selecting
by profile removes the list, and with it the per-bump edit and the staleness.

This is the content-model defect this task already lists ("Content is siloed
per track"), reached from the launcher rather than from the renderer, and it is
in scope for the same reason: the track should select content, not the filename.

The same run also showed `rider-pose fallback frames: 21` of 21 presentation
frames - every frame held the last recovered rider art - which is the renderer
gap this task exists to close, measured on a live DRAGSTER launch rather than
on the frozen contract frames.

**A third defect, from the same session: `--rom` cannot replace an
incompatible pack.** Told to re-extract after the v8 bump, the user ran the
documented recipe with `--rom` over a pack file holding v7 content and got:

```
[ failed] classic_pack (required): existing pack is invalid and was not replaced:
          Classic pack extraction-rules identity is incompatible
```

Extraction is gated on `pack_path.exists()` being false, so an existing pack
that fails validation is never rebuilt and `--rom` is ignored on that path. Not
overwriting a pack without being asked is the right default and should stay.
The defect is that the failure names no remedy, while the sibling check in
`_dragster_two_track_pack` does ("pass `--rom PATH` once to create ..."), so
the user is stopped with nothing to act on; the remedy is to move the file
aside, which no message says. Either name it, or accept an explicit opt-in such
as `--replace-pack` and say so in the message.

All three defects share one cause worth stating plainly, because it is the
same cause as the renderer split: pack identity is carried by filename and by
argv rather than by the profile recorded inside the pack. The launcher asks
"which file?" everywhere it should ask "which profile?", so it cannot tell a
typed flag from a default, cannot find a substitute without a hardcoded list,
and cannot tell a stale pack from a foreign one. Fixing the content model fixes
all three; fixing them one message at a time does not.

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
