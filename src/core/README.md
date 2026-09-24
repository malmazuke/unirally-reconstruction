# Recovered native components

`track_sampling.hpp/.cpp` implements one dependency of riding movement: expand
an indexed collision pose into ten points, then gather the track words under
those points. It does **not** update a rider, accept controller input, advance
an opponent or implement `native compare`. Its captured-argument probe validates
the component only; every call currently receives the original's incoming
position and pose. That research mechanism must not become simulation input.

The original runs the pose expansion at `$81:9E1B–9FAF`, followed by the spatial
gather `$81:8A2A–8B74`. R-0010 gives the source identity, full tested domain,
provenance and arithmetic. Coordinates are unsigned original position units;
point offsets wrap at 8 bits, grid intermediates at 16 bits. The grid has
64-unit coarse cells and 16-unit fine cells. Pose records and templates are
static extracted content. Table meanings beyond this exact lookup are not
assumed. Negative y, the rightmost coarse-cell branch and out-of-range content
are explicitly rejected because their behavior is not recovered here.

`SamplingContent` supplies byte spans, never host-layout structs. Content is
read explicitly little-endian. The component has no global mutable state,
frontend, network, emulator, or asset-extraction dependency.

Build and authored tests:

```sh
python3 tools/project.py build --preset lab-debug
python3 tools/project.py test --suite synthetic
```

ROM-dependent isolated experiment (after regenerating the access capture in
R-0010):

```sh
python3 tools/project.py content decode --manifest tests/manifests/native/movement-sampling.content.json --out artifacts/m2-01/content-expanded
python3 -m tools.unirally_lab.native.probe_sampling --access artifacts/m2-01/sampling-access/access.json --content-manifest tests/manifests/native/movement-sampling.content.json --content artifacts/m2-01/content-expanded --probe build/lab-debug/tests/native/sampling_probe --coarse-width 1024 --report artifacts/m2-01/sampling-probe.json
```

The three C++ tests use authored data for pose reflection/overflow, four grid
quadrants, and unsupported or missing inputs. The Python checks guard the
reference freeze and prevent incomplete captures from passing the probe. No
ROM or extracted table is included in those tests.

`track_progress.hpp/.cpp` observes marker words and advances a transition
counter through the original ordered tables. `ProgressUpdateState` stores both
riders' marker/tag/count/rejection state and the explicit alternating `$0302`
phase. `serialize_progress`/`deserialize_progress` define a 15-byte representation
without padding or host byte-order assumptions. The isolated recurrence matches
both riders through frame 2999 from one frame-1533 seed, but its sampled words
still originate from captured positions/poses. See R-0010 for commands and the
post-gather collision-response dependency that remains before autonomous riding.

## Semantic movement continuation state

`movement.*` defines the first shared autonomous-state boundary. Each rider has
one `ContactMotion`, `RiderContactState`, `SpeedModifiers` and `TrackProgress`
record, plus the named jump, pose, idle-pose, quarter-turn, residue and throttle fields
identified in R-0011-motion. Global state owns the alternating phases, counters,
timer, opponent continuation and reward queue. The canonical 333-byte encoding
is fixed-order little-endian, begins `URMV0001` plus a u32 frame, validates its
binary flags and cursors, and contains no CPU registers or captured calls.

`tools/unirally_lab/native/prepare.py` accepts only the identity-verified
end-of-frame 1533 WRAM/SRAM observation and a complete named 13-file static
inventory. It emits an ignored semantic seed, static content and runtime
metadata. `movement_runner` then advances that state using only controller
inputs and the bound static content. The update order is input/counters and AI,
each rider's active and every-frame motion, timer/reward handling, pose-based
sampling/contact, and alternating marker progress. `$82:A0B7-$82:A236` supplies
the at-rest pose oscillator: nine persistent words per rider, signed wrapping
updates, and the 64-byte signed lookup at ROM file offset `0x1B4`. Its wobble is
truncated toward zero by 16 and applied before contact impulse and reflection at
`$83:F09E-$83:F0F7`. The primary, cadence-17 and release-2347 cases all agree on
all 13 projected fields through frame 2999 in two fresh deterministic processes.

## Full-race finish state

M3-01 extends that semantic update through both frozen CRAWLER / DRAGSTER
full-race paths. `RaceFinishState` names each finish flag, stored decimal time
and centiseconds, animation countdown, the 240-update player delay, outcome,
and result-transition phase/counter. Crossing is observed after ordinary rider
movement; the dispatcher reacts on the next update, forces the derived axis
neutral, freezes the timer, and later stops movement during result loading.
These finish-boundary and low-speed-tail claims are limited to the two
submitted identity-bound paths plus the reviewer-owned crossing release in
R-0013. The tail uses ordered non-crossing ten- and 24-unit adjustments, not a
table of observed speed pairs.

The 333-byte `URMV0001` representation remains byte-identical while finish
state is empty. After the first crossing, serialization switches one way to
369-byte `URMV0002`, appending every finish/result field as fixed-width
little-endian values. The reader accepts both revisions and validates enums,
flags and the bounded delay without using native struct layout.

Opponent-first continuation adds the two-byte `$11E7` finish-pose selector as
371-byte `URMV0003`; older V1/V2 states remain readable and V2 offsets stay
unchanged.

## Native continuation boundary

`serialize_movement_state` is also the save/restore boundary; there is no second
snapshot representation. Its333 bytes include the frame, decoded player input,
both complete rider states (motion, contact, speed, progress, jump, pose,
idle-pose, quarter-turn, integration residues and launch state), timer,
opponent-AI continuation, reward queue/learned value, countdown and alternating
update phases/counters. Static decoded content and future controller inputs stay
outside the state and remain identity-bound process inputs.

`native restore-check` runs an uninterrupted baseline, regenerates each prefix
in a separate process, writes only the prefix's final canonical bytes, then uses
those bytes as the seed of another fresh process. It requires the entire resumed
canonical series—not only the13 displayed projections—to equal the baseline.
M2-02 exercises primary boundaries1631/2200 and release boundaries2761/2787;
the latter bracket the recovered idle/contact transition. An authored ROM-free
32-update continuation test pins FNV-1a series hash `7c799b4393d171f2` for
cross-platform CI. These claims apply to the recovered CRAWLER/DRAGSTER domain;
they do not imply that later game modes need no additional state.

`native finish-check` is the additive full-race successor. It requires the V1
to V2 transition, checks the frozen gameplay and finish/result state through
the first stable result frame in two fresh processes, and can repeat
fresh-process save/restore checks at requested finish boundaries.

M3-02 keeps that gameplay transition unchanged while defining an earlier
presentation-only boundary: for the accepted player-win path, the original
result screen is visible when `ResultLoading` reaches update 225 (frame 3678),
one update before the semantic gameplay phase becomes `ResultScreen`. The
headless renderer consumes this counter without mutating or extending the
canonical state. `native presentation-check` binds private states/reference
PNGs by hash and enforces every tracked regional mismatch limit from a
validated Classic pack; no ROM is opened by that command.

## Classic content pack and playable start

M3-02A's schema-1 `URCP0001` pack replaces the thirteen loose runtime filenames
with logical IDs. `ClassicContentPack` validates the binary layout, exact
source-ROM/extraction-rules/profile/start identities, required per-entry
identities and every payload SHA-256 before returning a byte span. The stable
command independently applies the same validation before starting a producer.
ROM offsets exist only in the tracked extractor rules; the simulation sees no
ROM address.

`classic_crawler_dragster_start()` constructs the frozen end-of-frame 1533
`MovementState` using named semantic fields. It has no WRAM/SRAM/ROM input and
serializes to the same 333-byte `URMV0001` identity accepted in M2/M3-01. The
runner accepts either this start ID plus `--content-pack`, or the prior
`--seed`/`--content-dir` research boundary. Pack mode still accepts a canonical
saved state for fresh-process continuation.

## Experimental ZOOM ZOO trial (M4-12)

`zoom_zoo_movement.hpp` declares the task-scoped 395-byte `URZZ0001` continuation.
The implementation at the end of `movement.cpp` reuses the accepted semantic
helpers without changing DRAGSTER dispatch. `update_zoom_zoo` orders controller
and AI production, phase-selected rider functions, reflection/throttle/gravity,
speed limits, position and pose, timer/rewards, then both contact calls. It
admits the frozen 1650–1849 Right/neutral trial only; the app has no new track
selection or presentation support. `vertical_contact.cpp` implements the direct
and mirrored vertical-column reducer and bounded landing response. Unsupported
branches reject transactionally. See R-0030 for source addresses and limits.

`zoom_zoo_runner` opens only canonical seed, static content and controller rows.
The Python `zoom_zoo_trial` laboratory authenticates content and compares
reference output outside that process, including three fresh-process restores.
`zoom_zoo_trial_extract` stops original execution before the race to extract
immutable landing matrices. It is never linked to or called by native gameplay.

M4-13 admits B jump within the experimental ZOOM ZOO interval.
[R-0032](../../docs/research/R-0032-player-landing-recovery.md) explains the
player landing impulse, displacement-quadrant ordering, rolling pose correction
and guarded option boundary. All future state remains in the 395-byte record.

M4-14's `SurfaceTransition` and `URZZ0002` extend the experimental ZOOM ZOO runner
with tile-selected mode/angle, leading support and pose intermediates. Vertical
contact now includes mirrored/inverted vertical and horizontal probes, steep
coefficients and landing branches. [R-0033](../../docs/research/R-0033-sustained-traversal.md)
links source addresses, immutable inputs and the finite differential domain.


M4-15's `ZoomZooRaceState` extends the research continuation with
ordered checkpoints/laps, stored race times, camera visibility feedback and
finish collision-pose selectors. `update_zoom_checkpoint`, `update_zoom_camera`,
`update_zoom_visibility` and `update_zoom_finish` in `movement.cpp` preserve the
source order documented by [R-0034](../../docs/research/R-0034-zoom-zoo-race-completion.md).
This is a seed-based simulation laboratory, not a ZOOM ZOO frontend or native
race initializer. Existing `URZZ0001` and `URZZ0002` contracts remain supported.

## ZOOM ZOO rider objects (R-0036)

`rider_object.hpp/.cpp` composes one 64x64 rider OBJ as `$83:F0FF` builds it:
a pose index selects a frame of five six-bit row masks and tile references in
the packed `$20:8000` pointers, `$23-$26` frames and `$27-$3F` tiles; an
optional overlay frame replaces covered slots; a row clip drops a wrapping
row. `project_rider_oam` reproduces `$82:ACAC` for the one-player race
(reflection is the horizontal flip) and `draw_rider_object` draws with the
PPU's vertical wrap. Unknown poses and references throw.

`rider_look.hpp/.cpp` reproduces the presentation-only look animation
(`$82:836D-$82:8926`) and overlay choice (`$83:EC8E`). The serialized race
does not carry its state, so `ClassicRaceHistoryTracker` in `presentation.hpp`
follows consecutive updates for the live frontend and the runner's
`--timeline` mode; it also keeps the channel-6 window pointer as the
original's drivers choose it and the vblank publishes it (`ClassicWindowPointer`:
the countdown word before each update, the update parity, one banner driver
per rider with a 360-update life; a paused update disables it), and the frame on which the
opponent finished for the frame-based fallback a single restored state uses
(R-0040, DRAGSTER-WINDOW-PAUSE).
`rider_presentation_tests` pins the mapping with synthetic
tables; private validation against eight original WRAM series is in R-0036.

## One renderer for both tracks

`render_classic_race` in `presentation.cpp` draws either track from the
742-byte shared race state and a `ClassicRacePresentationContent`, which
`classic_race_presentation_content(pack, track)` selects from the pack by
track: the track data, BG tiles, maps and palette are the track's own entries;
the race palette cycle, the channel-6 window family and the rider object
tables are shared ROM content; the scenario (initialization frame, laps) and
the playfield geometry come from the engine. The same code composes the BG
scroll from the previous update's camera, the rider objects, the fade from
`$0FF1`, the palette cycle and the window selection for both tracks; a track
whose pack carries the recovered mode-0 result screen draws it, the tour race
draws the authored lap graph. `classic_finish_view` derives the legacy finish
phases the recovered result screen reads, for presentation only.
`draw_bg3_text` draws the one BG3 text layer both the caption (R-0042) and the
HUD (R-0043) live on: a character is a tile and the tile `$10` above it, a cell
is eight by sixteen, and tilemap row `r` shows at `y = 8r - 1`. The caption sits
at row 10 column 8; `classic_race_hud_text` gives the HUD's four fields - the
left field (the lap count, `race` or `finish`) and the clock on row 2, and the
two centred fields on rows 5 and 20, each rider's crossing time after a lap or
the finish and its signed split at a checkpoint the other rider has passed
(R-0044) - from what `ClassicRaceHudClock` has followed the original's redraw
queue writing, including the clock the first rider through each checkpoint
stored, which the serialized race does not carry. Both draw into one ink mask before the riders,
so the measured red add where a sprite covers a glyph and the channel-6 members
that cover everything apply to both.
`classic_race_presentation_runner` renders one state, a timeline frame, or
the window member per row. The accepted M3 `render_dragster_headless` and its
five-pair rider atlas remain only behind the frozen DRAGSTER v1 contracts; the
app does not draw through them. The pack's eight track-independent engine
tables are read under their neutral `physics.*` names for both tracks; their
`zoom.*` aliases in the two-track pack are unread.


## Shared race engine for DRAGSTER

`update_zoom_zoo` was recovered on ZOOM ZOO but is the original's race engine
for both tracks (R-0038). `ClassicRaceScenario` names what differs between the
supported one-player races: the original frame label at initialization, laps,
the result-screen stable counts and race mode `$77:074B` (which also selects the
speed-limiter bound, the final-lap announcement and the lap-graph result).
`track_geometry` derives the playfield width, x wrap and camera/visibility scale
from decoded track byte 13. `classic_crawler_dragster_race_start` and
`dragster_race_content` (two-track pack) start DRAGSTER on this path; its state
serializes as `URDG0001` in the 742-byte `URZZ000B` layout. Every other race track's
state is `URTRnn02`: that layout followed by the 34 special-tile bytes of R-0047, which
DRAGSTER and ZOOM ZOO also append, as `URDG0002` and `URZZ000C`, only while one of those
words is live. The legacy DRAGSTER
`update_movement` path and its `URMV` formats remain for the accepted gates.
