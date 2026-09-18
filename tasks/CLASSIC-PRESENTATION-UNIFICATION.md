# CLASSIC-PRESENTATION-UNIFICATION - One renderer for both tracks

## Assignment

- Status: review (implementation candidate on `task/classic-presentation-unification`, head: see `git log`; started 18 September 2026 from `main` at `ed504fb`, after the DRAGSTER window-effects review closed and integrated)
- Milestone: follow-up to M4-16, DRAGSTER-ORDINARY-CONTROLS and DRAGSTER-WINDOW-EFFECTS
- Coordinator: main session
- Task provider: Anthropic (unchanged)
- Worker/session/runtime/model: Claude Code; Claude Opus 5 for measurements one to four, then Claude Fable 5.1 for the fifth measurement, the implementation and everything below (the model changed between sessions on 18 September 2026)
- Base commit: `main` at `ed504fb`
- Branch and isolated worktree: `task/classic-presentation-unification`, `.worktrees/classic-presentation-unification`
- Reviewer: fresh independent subagent in an isolated checkout at the exact candidate

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

## Fifth measurement - the frozen contract race does have inputs

The fourth measurement said the contract "has no menu or replay manifest, only
the seven states and their pictures", and concluded the frozen frames could
not be reached on the shared state without a recapture. That was wrong, and
it was checkable: the winner fixtures' states at 1600, 2000 and 2400 are byte
for byte the release-3213 sweep's states, and the release-3213 case is a
one-frame perturbation of the accepted continuous-Right replay
(`race-crawler-dragster-12000-continuous-right-fields.json`: the menu path,
Right 1500-11999, Up 2200-2259). The contract race is that replay, and the
replay carries every input.

Driving the shared engine from end-1328 with those inputs
(`artifacts/unification-baseline/stage_b.py`) reproduces both riders' pose
indices at 1600, 2000, 2400 and 3213 exactly. At 3453, 3678 and 3679 the
legacy states hold poses 1278/892 where the shared engine holds 2637/2684:
the M3 path never recovered the finish-pose cycle, the shared engine's cycle
is byte-exact against the original on its own captures, and the pictures
decide it below. So the recapture is not needed and the original acceptance
row stands; the "Acceptance, amended" table is superseded by the results.

## Implementation

Four commits on the branch (`git log ed504fb..`):

1. **One renderer.** `render_classic_race` (`src/core/presentation.cpp`)
   replaces `render_zoom_zoo` and DRAGSTER's narrowing call path. It takes the
   742-byte race state and a `ClassicRacePresentationContent` that
   `classic_race_presentation_content(pack, track)` selects from the pack:
   track content by track, the palette cycle, window family and rider object
   tables shared, the scenario and playfield geometry from the engine.
   Inside it, for both tracks: BG scroll from the previous update's camera
   minus the track origin (the original publishes the same relation on
   DRAGSTER: at 3453, camera 25266 against origin 832 gives BG1 24434 and BG2
   12217), the world lookup by the track's column count, rider objects through
   `project_rider_oam` with the track's `$81:A4C1`/`$81:A445` set, the fade
   from the previous update's `$0FF1` instead of a frame formula, the palette
   cycle and the channel-6 window selection from the scenario's setup frame
   (initialization + 6: 1334 and 1382), the authored HUD with the scenario's
   lap count, the pause menu. A track whose pack carries the recovered mode-0
   result screen draws it through `classic_finish_view`; the tour race draws
   the authored lap graph. The app, the fuzz runner and the presentation
   runner (now `classic_race_presentation_runner`, either track, plus a
   `--window-index` mode) draw through it; `dragster_race_picture_runner`,
   `render_dragster_race`, `dragster_presentation_state`,
   `race_picture_brightness` and `presentation_position` are gone. The M3 v1
   renderer and its five-pair atlas stay only behind the frozen v1 contracts.
2. **Neutral names.** `zoom_zoo_content` reads the eight duplicated engine
   tables under `physics.*` for both tracks; `zoom_zoo_runner` binds pack
   content through the accessors instead of a third entry list.
3. **The opponent-won banner.** `ClassicRaceHistoryTracker` keeps the frame on
   which the opponent finished, and `classic_opponent_finish_frame` recovers
   it for a restored state from the two finish times (two centiseconds per
   frame plus the frame parity; the relation holds on all four original races
   with both finishes). R-0040 records the closure.
4. **The launcher.** `frontend run` selects by profile: a typed `--pack` must
   carry the supported profile and is refused otherwise, naming both profiles
   and the remedy; without `--pack` the newest valid pack under `local/` is
   used; a first extraction goes to a path named after the profile; `--rom`
   over an incompatible pack replaces nothing unless `--replace-pack` moves it
   aside; and the app's `--supported-profiles` (declared once, in the pack
   reader) is compared with the rules before launch, so a stale build is
   reported with the rebuild command. The hardcoded substitute list, the
   v1-to-two-track upgrade and the per-track pack rewrite are deleted. All
   four playtest defects above are closed; the fourth did not need a separate
   task.

## Measurements on the candidate

All scripts and outputs are under ignored `artifacts/unification-baseline/`
(`stage_c.py`, `stage_c/report.json`, `verify_banner.py`,
`banner/banner-agreement.json`, the gate logs).

**DRAGSTER frozen contract frames** (`stage_c.py` part A): the shared engine
driven by the contract race's inputs, drawn by the unified renderer, against
the fixture pictures on each case's rectangle, beside the accepted v1 renderer
on the same frames:

| frame | rect pixels | v1 renderer | unified | threshold |
| --- | --- | --- | --- | --- |
| 1600 | 26,656 | 36 | 36 | 533 |
| 2000 | 50,176 | 697 | 358 | 1,003 |
| 2400 | 50,176 | 279 | 279 | 1,003 |
| 3213 | 50,176 | 445 | 322 | 1,505 |
| 3453 | 50,176 | 653 | 777 | 1,505 |
| 3678 | 57,344 | 962 | 962 | 8,601 |
| 3679 | 57,344 | 961 | 961 | 8,601 |

Every frame is inside its threshold; four are equal or better, 3453 is 124
pixels worse (the shared engine's finish pose against the atlas captured at
that frame) and still under half its threshold. Outside the rectangles the
authored HUD band differs from the original HUD, as declared.

**Release-3213 race against the original pictures** (part B): the 132 frames
the window-effects measurement scored, rectangle (0, 28, 256, 196), previous
renderer's v8 renders against the unified renderer, both against the same
original captures:

| segment | frames | previous mismatch | unified mismatch |
| --- | --- | --- | --- |
| countdown 1510-1534 | 2 | 4,433 | 408 |
| GO 1535-1603 | 44 | 39,623 | 7,802 |
| racing 1604-3213 | 24 | 27,687 | 10,538 |
| winner banner 3214-3453 | 46 | 60,857 | 37,379 |
| result loading 3454-3677 | 14 | 622,751 | 622,757 |
| result 3678-3679 | 2 | 1,923 | 1,923 |
| all 132 | 132 | 757,274 | 680,807 |

114 frames improve, 14 are unchanged, 4 are worse: 3452 and 3453 by 124 (as
above) and 3600 and 3677 by 3, inside the declared "native draws the race
while the original fades in the result screen" difference. A further 51
frames with originals but no previous render (1330-1339 initialization,
1420-1436 and 1510-1532 countdown) score 0 to 490 each.

**ZOOM ZOO scenes** (part C): the ten M4-16 scenes rendered by the new runner
are pixel-identical to the before-change runner on every scene.

**Rider art:** hidden app runs of 4,000 updates with Right held, both tracks:
`rider-pose fallback frames: 0`; DRAGSTER reaches its stable result
(phase 3, outcome 1, 3357/3358). The user's playtest run had 21 of 21.

**Opponent-won banner** (`verify_banner.py`, the `lose-a` capture): with the
tracked history the renderer's member equals the original's `$80:868E` read on
2,226 of 2,226 frames from 1334 to 3559; without history, the derivation
agrees on every frame from the player's finish (3318) through loading and
selects nothing on the 102 frames before it.

**Launcher, real runs** (`launch-*.json`): a typed DRAGSTER v1 pack is refused
naming `classic.pal.crawler.dragster.v1`, `classic.pal.crawler.two-tracks.v8`
and the remedy; no `--pack` selects the v8 pack by profile and the app reports
the supported profile; a v7 pack under `--rom` without `--replace-pack` is
refused with the remedy.

## Gates on the candidate

| Gate | Result |
| --- | --- |
| lab-debug ctest | 23/23 (new checks: shared-state palette phase, window index and finish view agree with the legacy functions; the finish-frame derivation on the reversal, random-1 and lose-a values; DRAGSTER OAM limits) |
| tooling `tests.tooling.test_frontend` | 14/14 (profile refusal, selection under `local/`, `--replace-pack`, stale build) |
| M4-16 ZOOM ZOO primary (`zoom_zoo_playable compare`, v8 pack) | passed at `cfb539d` |
| DRAGSTER frozen originals primary, random-1, reversal | passed at `cfb539d` |
| historical matrix (`hist.sh`, `hist2.sh`: 20 commands) | 20/20 at `cfb539d`; the v1 contracts report winner 36/697/279/445/653/962/961 and loser 1,073, identical to the accepted figures |
| five presets at the candidate `f734b4e` (`final-gates.sh`) | lab-debug, lab-release, lab-sanitize, app-debug, app-sanitize: 23/23 each |
| `test --suite synthetic` (lab-debug) at `f734b4e` | passed, 411 checks, 3 fresh-process repeatability runs |
| M4-16 ZOOM ZOO primary and DRAGSTER primary, random-1, reversal at `f734b4e` | all passed (app-debug `zoom_zoo_runner`, v8 pack) |
| `dragster_fuzz_runner` (lab-release) at `f734b4e` | 60 seeds, 549,051 updates, 119 completed races, 1,878 pause restarts, 11,170 renders, 0 aborts |
| hidden app runs at `f734b4e`, both tracks, 4,000 updates | 0 rider-pose fallback frames each |
| hosted CI on the pushed tip `f734b4e` | run 35309658396: success on ubuntu-24.04 and macos-15 |

## Mistakes

- The fourth measurement's claim that the frozen contract has no inputs was
  recorded without checking the fixtures against the sweep states beside
  them. Ten minutes of comparison would have found the replay manifest; the
  claim instead sent the plan toward a recapture the task did not need.
- The banner check first used the wrong alignment (the setup's read on frame
  n taken as the table for n+1) and reported 248 disagreements; the script
  had tried both shifts, and reading its own output found 0 at shift 0.
- The finish-frame derivation was one frame late during result loading until
  the same check showed 16 against 17 at 3559; the delay stops counting one
  update before loading starts.
- Ten historical-matrix commands failed twice for reasons that were not
  regressions: the private `local/native` inputs are per worktree, and
  `native compare` refuses a reused artifact directory. Both are recorded in
  the gate logs.

## Handoff

- Branch `task/classic-presentation-unification`; base `ed504fb`; head: see
  `git log`. Measurements one to four (Opus 5) and five onward (Fable 5.1)
  are above; the code is the four commits listed under Implementation plus
  this record.
- Not done, by decision: ZOOM ZOO's own window content stays omitted (no
  family bound for it) until its members are captured; the authored HUD and
  result styles stay authored; the eight `zoom.*` aliases stay in the v8 pack
  until a later profile drops them; renaming other accepted entries is not
  permitted.
- Next experiment if picked up cold: bind the window family for ZOOM ZOO in
  `classic_race_presentation_content`, render scene 1450 and compare with the
  original; if the countdown digits match, the same selection serves both
  tracks and R-0040's ZOOM ZOO bullet closes.
- Review: fresh independent reviewer in an isolated checkout at the
  candidate, then integration, final-tip CI and the closeout under ignored
  `artifacts/unification-integration/closeout.json`.
