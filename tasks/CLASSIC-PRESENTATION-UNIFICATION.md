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

**Correction to the first version of this section.** It said engine tables were
first captured under `zoom.*` and captured again under neutral `physics.*`
names when DRAGSTER moved onto the shared engine. That history is backwards,
and I inferred it from the names instead of checking. The evidence:

- The DRAGSTER v1 pack, `classic.pal.crawler.dragster.v1`, has 25 entries and
  not one `zoom.*` among them. Its vocabulary is entirely neutral
  (`physics.rider.collision-poses`, `physics.speed.masks`, ...), and it predates
  ZOOM ZOO's recovery.
- The neutral duplicates are read only by `src/core/movement_runner.cpp`, the
  M3 DRAGSTER movement path, and by the pack registry.
- The shared engine reads the track-named ones. `dragster_race_content` in
  `src/core/zoom_zoo_pack.hpp` is `zoom_zoo_content(pack)` with three overrides
  for DRAGSTER's own track, tile columns and tile flags; every other table it
  takes under its `zoom.*` name, for DRAGSTER races as much as ZOOM ZOO ones.

So the neutral names are the older ones and the track-named ones are newer. The
project already had a correct vocabulary for track-independent tables, and the
ZOOM ZOO recovery introduced a second, track-named one beside it rather than
reusing it. The duplication is two vocabularies coexisting in one pack, because
the pack must satisfy both the v1 DRAGSTER consumers and the shared engine.

That makes the criticism sharper, not milder: this was a regression in naming
introduced by the newer work, and the shared engine now reads a track's name
for tables that have nothing to do with that track. It is the same failure as
the renderer split - the newer recovery grew its own vocabulary instead of
adopting the one already there.

One design credit where it is due, since the first version implied otherwise:
`dragster_race_content` is not a second content builder. It is the shared one
plus three overrides, which is the right shape. The defect is the names it
reads, not its structure.

That also bounds the naming cleanup the task lists. It is not only
`presentation.zoom.race-palette-cycle.v1` being read by DRAGSTER: 26 of 56
entries carry a `zoom.` prefix and only 6 of those are ZOOM ZOO track content
(`zoom.track-data`, `zoom.bg1-tiles`, `zoom.bg2-tiles`, `zoom.bg2-map`,
`zoom.palette`, and the tile tables). The rest name the engine after a track.

Renaming accepted entries is not permitted, so the cleanup has to be additive
in the other direction, and the correction above tells us which direction that
is: the neutral names already exist and already carry these bytes, so the work
is to point `zoom_zoo_content` at them and stop reading the `zoom.*` aliases
for track-independent tables. A later profile can then drop the aliases once
nothing reads them, which removes the 50,828 duplicated bytes without renaming
anything accepted.

Six `zoom.*` entries are genuine ZOOM ZOO track content and stay: `track-data`,
`bg1-tiles`, `bg2-tiles`, `bg2-map`, `palette` and the tile tables. Those want
a track-selected lookup, not a neutral name.

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

## Fourth measurement - the frozen contracts cannot drive a state-driven renderer

The plan was to run the shared engine to each frozen reference frame, check the
movement it produced against the frozen capture, and then render both ways. The
bridge does not exist, and the reason is worth recording before any code moves.

The frozen contract states are 333 and 369 bytes; the shared race state is 742.
The contracts captured the narrowed state, so the fields the recovered renderer
reads were never recorded in them. The obvious repair - replay the race and take
the wide state at those frames - needs the contract's own controller inputs, and
the contract does not record any: it has no menu or replay manifest, only the
seven states and their pictures.

The nearest capture that does carry a timeline is `primary-a` from
DRAGSTER-ORDINARY-CONTROLS. Running the shared engine over it and comparing at
the seven contract frames (`artifacts/unification-baseline/stage_a.py`):

| frame | contract player | native player | contract opp | native opp | camera delta |
| --- | --- | --- | --- | --- | --- |
| 1600 | 1273 | 2197 | 611 | 611 | +871 |
| 2000 | 2133 | 313 | 2197 | 2197 | +757 |
| 2400 | 2197 | 185 | 2197 | 2197 | +920 |
| 3213 | 2133 | 2629 | 2261 | 2261 | +857 |
| 3453 | 1278 | 2635 | 892 | 2684 | +833 |

0 of 7 frames agree, but the disagreement is structured. The opponent's pose is
identical at every racing frame and the player's never is, with the camera
consistently 757-920 further along. The opponent is AI-driven from the same
scenario start, so it replays identically until the player's finish perturbs the
shared state, which is why the opponent also diverges from 3453 onward. Same
track and scenario, a different driver. `primary-a` cannot stand in for the
contract race, and no other capture in the tree can either.

So the acceptance criterion "DRAGSTER frozen contracts: counts unchanged or
lower at every frame" is not satisfiable by a renderer driven by the shared
state, for a reason that has nothing to do with whether that renderer is
correct. Three ways out, and only one is honest:

- Fabricate the missing fields to widen the 333-byte states. Rejected: the
  fade level, pause, result and lap state would be invented, and the contract
  would then be checking numbers this task made up.
- Keep the old renderer for the frozen contracts and the recovered one for live
  play. Rejected: that is the split this task exists to remove, preserved under
  a different name.
- Recapture the seven frames under the shared state schema, additively, as a new
  contract profile beside the accepted v1 one. The project has the capture
  tooling and the ROM, the accepted contracts stay untouched and still checked,
  and the new profile can be driven by the same state the live path uses.

Recapture is the path. Until it lands, the unified renderer is measured against
the original captures that do carry timelines - `primary-a` and `reversal` for
DRAGSTER, the M4-16 scenes for ZOOM ZOO - which is the evidence the window
effects and clock limit were accepted on.

## Acceptance, amended

The criteria below replace the DRAGSTER frozen-contract row of the table above,
which the fourth measurement showed to be unsatisfiable as written. The v1
contracts remain accepted and unchanged; what changes is what is asked of the
unified renderer.

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| DRAGSTER original agreement | shared engine over `primary-a` and `reversal`, rendered and compared to the original capture | no regression against the window-effects measurement | comparison report |
| Recaptured frozen frames | new contract profile at the seven frames, shared state schema | native matches the original picture | manifest and capture |
| Accepted v1 contracts | `native presentation-check` winner and loser, v1 pack | unchanged; the v1 path is untouched by this task | reports |

## Handoff

- Measurements one to four are done and recorded. The state is already shared
  and discarded at the call site; the pack carries eight engine tables twice
  under two vocabularies, the older of which is the neutral one; the recovered
  renderer names its track inline; and the frozen contracts cannot drive it.
- Next: recapture the seven DRAGSTER contract frames under the shared state
  schema, then widen `LivePresentation::render_dragster_race` to pass the race
  state through and select content by track rather than by literal name.
