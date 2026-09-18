# CLASSIC-PRESENTATION-UNIFICATION - One renderer for both tracks

## Assignment

- Status: in progress (started 18 September 2026 from `main` at `ed504fb`, after the DRAGSTER window-effects review closed and integrated)
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

**A fourth defect: the supported profile is declared twice and never
reconciled.** With a correct 56-entry v8 pack extracted, the launch still
failed:

```
[ passed] atomic_pack_creation (required): 56 entries at local/classic-crawler-two-tracks-v8.pack
[ failed] frontend_launch (required): Unirally launch failed: Classic pack profile is unsupported
```

The pack was right; the binary was stale. `build/app-debug` had been built at
05:46 and the v8 merge landed at 10:40, so the only profile string in it was
`classic.pal.crawler.two-tracks.v7`, while the Python side validated against
the working tree's v8 manifest and passed. The supported profile lives both in
`src/core/content_pack.cpp` and in
`tests/manifests/content/classic-crawler-two-tracks-pack.json`, and nothing
checks the two agree, so a stale build is diagnosed as a bad pack. The
launcher knows the preset and could compare the binary's supported profile
with the manifest's before launching, and say "rebuild" when they differ.

Scope note: this one is about where the identity constant lives rather than
about presentation, so it may belong in a separate tooling task. It is recorded
here because it is the same root - one identity, declared in more than one
place, reconciled nowhere - and because splitting it out before the content
model is decided would fix the symptom in the wrong layer.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| DRAGSTER frozen contracts | `native presentation-check` winner and loser | counts unchanged or lower at every frame | reports |
| ZOOM ZOO visual contract | the M4-16 frozen scenes | unchanged or closer to the original | comparison |
| Rider art | DRAGSTER live and headless runs | zero held-pose fallback frames | run logs |
| Original agreement beyond frozen frames | picture comparison across a race on both tracks | measured, recorded, no regression | comparison report |
| Gates | four presets, historical matrix, DRAGSTER and ZOOM ZOO differential gates | all pass | gate logs |
| Review and CI | fresh reviewer; hosted CI on the exact tip | approve; green | review, closeout |

## Opening measurement - the state is already shared; presentation discards it

Before measuring pictures, the structure. DRAGSTER's live path already holds the
full shared race state:

`LivePresentation::render_dragster_race(const ZoomZooState &race, ...)` in
`src/app/frontend.cpp` receives the same `ZoomZooState` ZOOM ZOO renders from -
the 742-byte shared race state, whose `track` member is serialized as the state
magic (`URDG0001` for DRAGSTER, `URZZ000B` for ZOOM ZOO). Its first statement is
`dragster_presentation_state(race)`, which copies `race.movement`, back-fills
four finish fields from `race.race`, and discards the rest: pause, result,
fade_level, rolls, announcements, surface and reflection transitions. The old
M3 renderer is then called with that narrowed `MovementState`.

So the renderer split is not caused by DRAGSTER lacking state. Presentation
takes a narrower input type than the engine already produces, and throws the
difference away at the call site.

The cost is visible in the next six lines of the same function. Because the
narrowed renderer cannot fade or draw the pause menu, both are re-added outside
it: `race_picture_brightness` scales the converted picture by `(b/15)^1.5`,
carrying the comment "Approximation: the original scales CGRAM words before
output; this scales the converted picture with the same 1.5 output curve", and
`draw_race_pause_menu` is layered on afterwards. M4-16 recovered the correct
form for ZOOM ZOO - scale the CGRAM words, then convert - and it cannot be
reused here precisely because the narrowing removed the state it needs.

That is the whole scaling failure, and it is one function wide: every capability
that depends on the wider state must be re-implemented outside the renderer,
approximately, for as long as the narrowing stands. It also explains the live
symptom - `is_recovered_pose_pair` whitelists five pose pairs, so a DRAGSTER
launch that matches none of them falls back on every frame (21 of 21 in the
user's run).

This makes the task a merge rather than a rewrite: the state to drive
`render_zoom_zoo` for DRAGSTER is already at the call site, unused.

## Second measurement - the pack carries eight engine tables twice

Grouping the 56 v8 entries by content hash: 48 distinct hashes, 8 shared by two
entries each, and every pair has both the same `sha256` and the same raw source
offset and length, so these are provably the same ROM bytes and not merely
same-sized ones.

| bytes | neutral name | track-named duplicate |
| --- | --- | --- |
| 32768 | `physics.rider.collision-poses` | `zoom.collision-poses` |
| 17249 | `physics.rider.collision-templates` | `zoom.collision-templates` |
| 512 | `physics.rider.displacement-table` | `zoom.displacement-table` |
| 128 | `physics.rider.pose-slopes` | `zoom.pose-slopes` |
| 80 | `physics.track.progress-transitions` | `zoom.progress-transitions` |
| 64 | `physics.rider.idle-pose-table` | `zoom.idle-pose-table` |
| 18 | `physics.speed.decrements` | `zoom.speed-decrements` |
| 9 | `physics.speed.masks` | `zoom.speed-masks` |

50,828 bytes, 4.0% of the entry bytes, and the container stores each entry
separately rather than deduplicating by hash: the entry sizes sum to 1,281,915
against a 1,286,427-byte pack file. Every duplicated table is rider or physics
content that is track-independent; none is ZOOM ZOO track content.

The history is legible from the names. Engine tables were first captured while
ZOOM ZOO was the only recovered track, so they took its name. When DRAGSTER
moved onto the shared engine they were captured again under neutral
`physics.*` names, and nothing removed the originals, because the accepted
contracts and the v1 pack still reference them and entries are immutable once
accepted. This is the same failure as the renderer split, in the content model:
sharing was done by adding a second copy rather than by renaming one, so the
cost of each new track is paid in duplication instead of reuse.

That also bounds the naming cleanup the task lists. It is not only
`presentation.zoom.race-palette-cycle.v1` being read by DRAGSTER: 26 of 56
entries carry a `zoom.` prefix and only 6 of those are ZOOM ZOO track content
(`zoom.track-data`, `zoom.bg1-tiles`, `zoom.bg2-tiles`, `zoom.bg2-map`,
`zoom.palette`, and the tile tables). The rest name the engine after a track.

Renaming accepted entries is not permitted, so the cleanup has to be additive
in the other direction: the shared entries already exist under neutral names,
so the work is to stop reading the `zoom.*` aliases, not to rename them, and to
let a later profile drop the aliases once nothing reads them.

## Third measurement - the recovered renderer hardcodes its track

`render_zoom_zoo` reads six pack entries by literal name, five of which are
ZOOM ZOO track content: `zoom.track-data`, `zoom.bg1-tiles`, `zoom.bg2-tiles`,
`zoom.bg2-map`, `zoom.palette`, plus the already-shared
`presentation.zoom.race-palette-cycle.v1` that DRAGSTER reads despite its name.
The result screen also embeds track and rider strings as literals
("LAPS ON ZOOM ZOO", "MIKE", "BRONSEN").

So the renderer cannot draw DRAGSTER today for a reason unrelated to state: it
names its track content inline. The DRAGSTER equivalents are present in the
same pack under `presentation.track.dragster.*` and
`physics.track.dragster.*`, so this is a lookup-by-track change, not a
recovery. Together with the first measurement it sets the shape of the work:
widen the input type from `MovementState` to the shared race state, and select
content by track instead of by literal.

## Handoff

- Structural measurement above is done. Next: call `render_zoom_zoo` with the
  DRAGSTER state and two-track content, measure against DRAGSTER's frozen
  contract frames, and record the first divergence, as the controls task did for
  the engine. The frozen M3 contracts are `PresentationSample` captures, so they
  need a projection from the shared state; the live path does not.
