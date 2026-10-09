# IDLE-DEMO-ROTATION - every idle demo after the second

## Assignment

- Status: **in progress**, claimed 9 October 2026 (01:30 UTC) on main `c9f0361`. The user asked
  this session to continue to "the two remaining ready tasks", PORTABLE-CORE-IDENTITY and
  ATTRACT-DEMO, then to keep working until weekly usage reaches 50%. ATTRACT-DEMO's outcome (the
  second idle demo) had been accepted by PR #44 on 29 September, with its status left stale; its
  declared exclusion, the later demo cycles (R-0070), is this task.
- Milestone: M4 coverage of the main menu's idle mode (COVERAGE-ROADMAP).
- Coordinator: the claiming session is coordinator, primary and integrator.
- Task provider: Anthropic.
- Worker/session/runtime/model: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`); one
  session as coordinator, primary and integrator.
- Actual model/reasoning effort, routing rationale: **tier 1** (D-0008): the demo's choice of track
  and pairing, its timing and the demo races' state are gameplay claims.
- Provider quota window (D-0004): at claim weekly 3%, five-hour 0% (9 October 01:24 UTC); the user's
  bound for this session is 50% weekly.
- Reviewer: a fresh Anthropic subagent in an isolated checkout, with withheld captures.
- Dependencies: SPLIT-SCREEN-RACE (R-0069), ATTRACT-DEMO (R-0070), TWO-HUMAN-RESTART (R-0084, the
  race counter `$77:10B1`).
- Base commit: main `c9f0361`.
- Branch and isolated worktree: `task/idle-demo-rotation` in `.worktrees/idle-demo-rotation`.
- Owned paths: `src/core/front_end*`, the demo's race setup, native tests, a research record, this
  record, `docs/STATE.md`, `tasks/README.md`, `tasks/NEXT_SESSION.md`.

## Outcome and boundaries

From a cold power-on with Start on frames 300-305 and both pads released, the original plays idle
demos in a fixed rotation (`local/evidence/idle-demo-cycles/cold-30000`, frames to 29,999):

| Cycle | Track written | Track | View | Rider / opponent |
| --- | --- | --- | --- | --- |
| 1 | 1351 | 1 ZOOM ZOO | split | 4 / 14 |
| 2 | 4399 | 3 | one | 6 / 1 |
| 3 | 7474 | 4 | split | 7 / 1 |
| 4 | 10529 | 5 | one | 8 / 1 |
| 5 | 13577 | 6 | split | 9 / 3 |
| 6 | 16623 | 8 | one | 11 / 1 |
| 7 | 19691 | 9 | split | 12 / 6 |
| 8 | 22722 | 10 | one | 13 / 1 |
| 9 | 25811 | 11 | split | 14 / 8 |
| 10 | 28850 | 13 | one | 0 / 1 |

Native plays cycles 1 and 2 and stops at the third (it chooses track 3 again). Recover the
rotation and make native play every cycle from power-on through its menu return: the track, the
view, the pairing, the title and loading timing, and the demo race itself on each track.

Outside: audio; pad presses during cycles after the second (the interrupted returns of R-0070 stay
as measured); 2P and VS menu paths.

## Static reading at claim (to be confirmed dynamically)

- `$80:948D-94B2` (mode 5, unclassified bytes in the static map): rider 3 and opponent 1 into
  `$017D/$017F`, mode 5 into `$77:10AD`, then the track counter `$77:10C8` + 1, wrapped at 40, into
  `$CE`, repeated while the track is a stunt event (race mode 2 from `$83:9983`).
- `$83:9894` copies the direct page `$0000-$019D` to `$77:0E6B`, so `$77:0F34` is `$00C9`, the
  menu's palette-cycle phase.
- `$83:C912-C994` (demo only, option bit 1): `$77:1115` toggles 0/1; 1 is split (option bit 3).
  The rider is `(track + $77:10B1 + $77:0F34) & 15`; a split race's opponent is
  `(track - $77:10B1 - $77:0F34) & 15`, plus 13 if it equals the rider.
- Each cycle after the first: the track is written 453 frames after the idle count reaches zero,
  the race runs about 1,906 frames, and the next idle count starts 598 frames after its exit. The
  loading time between the track and the race start varies by track.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Rotation | A cold capture through a whole lap of tracks | Native's track, view, pairing and timing equal on every cycle | JSON |
| Demo races | Each cycle's race words and sampled pictures against the original | Equal, or each difference named and bounded | JSON |
| Nothing else moves | Gates, sweeps, the first two cycles' captures | Unchanged | gate logs |
| Review | Tier 1 | Approved with withheld captures | review on the pull request |

## Checkpoint - 9 October 2026 02:45 UTC (commit `3dcaac4`)

Evidence (main checkout `local/evidence/idle-demo-cycles/`):
- `cold-30000`, `cold-102000`: `split-screen-race/capture_demo.py` cold captures (Start 300-305,
  pads released), work RAM `$0000-$21FF` and cartridge RAM every frame, pictures every 100 frames
  (1,400-101,900). `cold-102000` covers 34 cycles: a whole lap of 32 races and the wrap.
- `cycles.py CAPTURE` writes `cycles.json` (per cycle: track write, `$11C5` = 270, exit
  `$12B3`, track, view, pairing; `initialization` = the frame `$212C` is set).
- `compare_cycles.py CAPTURE NATIVE_TIMELINE OUT`: native `--race-timeline` rows (bytes 12-564)
  against `zoom_zoo_race_reference.project` of the capture, per cycle.

Verified (dynamic, `cold-102000`):
- The rotation and pairing above hold for all 34 cycles; `$77:0F34` is 3 at every setup.
- The race starts (`$212C`) exactly the track's `race_sound_load_offset` + 7 frames after the
  track write, on all 34 cycles (`$11C5` = 270 comes 5 or 6 frames earlier, by the cue lead).
- Each race runs 1,900 updates to its exit.

Native at `3dcaac4` (with the serialization guards relaxed in a scratch build,
`artifacts/idr/probe-guards.patch`): cycles 1, 2 and 4-12 equal the original on all 1,901 race
frames; cycle 3 (track 4, split) differs at update 823 (`opponent_horizontal` 1 against 0, the
opponent's velocity x); from cycle 13 native starts 1-2 frames late (below).

Found on the way: main's second demo had regressed since 7c301dd (TWO-PLAYER-VS), differing from
R-0070's frozen `second-race-reference.txt` on 1,651 of 1,901 rows (`ai.suppression_counter` 60
against 30): `opponent_tier` gave every opponent below 16 the human tier. Fixed by the scenario's
`two_humans` flag; cycle 2 is exact again.

Open:
1. **State formats.** `race_state_io.cpp` refuses a split state off ZOOM ZOO/local DRAGSTER and a
   one-view demo state off track 3 (pairing {6,1} and tier `{0xF1,0,0x60}` hard-coded in the
   reader). Generalize: split demo on the 15 other split tracks (base 916, new layout letter),
   one-view demo on the 15 other one-view tracks and DRAGSTER (base 742/794, new letters, sizes
   collide with split layouts, so dispatch by letter); tier from `opponent_tier`; the view must
   match the track's place in the cold rotation; second-camera checks per track.
2. **Menu timing jitter.** In the original the race exit to the next track write is 1,050-1,052
   frames: the return to the menu's idle count is 118 or 119 frames (119 after the races on tracks
   20, 24, 36) and the title 453 or 452 frames (452 before tracks 16, 20, 23, 26 and the second
   track 1). The title's extra frame lies around its frame 134, where the blank loading phase
   ends. Hypothesis: the sound program's upload handshake (R-0077 measured such variation for the
   race session). Next: an access capture of the APU ports `$2140-$2143` over a 452 and a 453
   title, then decide between modelling it from native audio and a measured schedule.
3. Cycle 3's divergence at update 823.
4. Pictures (retained every 100 frames), audio is out of scope, the app path (`sdl_main.cpp`).

## Checkpoint - 9 October 2026 04:10 UTC

- **Timing jitter measured** (`timing_capture.py`, `timing-200000.json`: two laps, 66 cycles).
  The return to the menu's idle count holds its frame 100 once after the timer exits of the races
  on tracks 20, 24 and 36, in both laps (native: `demo_return_held`). The title is 452 rather than
  453 frames on 6 cycles of lap 1 and 4 of lap 2, not the same ones: the first wait for the sound
  processor (`$82:8097`, after the driver reset at title frame 58) ends a frame sooner
  (`access-title-c12`/`-c13`). Native keeps 453; `front_end_runner --short-demo-title CYCLE`
  replays a capture's short titles (the frame passes title frame 134).
- **Fixes:** rider 1's hints never run for MIKE in the demo (cycle 15, track 19); a demo rider
  whose marker switches the AI off is left neutral, because the pads are read first (cycles 3, 21
  and 27; tracks 4, 26 and 34).
- **Result** (probe build with the state guards relaxed, `--short-demo-title 13 16 18 21 33`):
  all 34 cycles of `cold-102000` equal the original on every race frame, race words (bytes
  12-564) and the 42-byte demo trailer.
- Next: the state formats (open item 1), then pictures, gates, review.

## Review candidate - 9 October 2026

- Formats (`f796c7c`): split demo J on a track's 916-byte base; one-view 8 on any one-view track,
  K/L on DRAGSTER; the reader dispatches equal sizes by family and letter, takes the trailer's
  pairing and checks the tier and the rotation's view. A one-view demo's parked second camera is
  its track's own place (R-0070's 0x0530/0x0120 is track 3's), so the reader checks only its OAM
  word and speeds. `front_end_runner --restore-check` now covers demos: 9 cycles round-trip and
  continue to the exit.
- Pictures: 906 of 1,006 retained pictures equal; the classes are R-0087's and are queued as
  [DEMO-PICTURES](DEMO-PICTURES.md) (tier 2).
- Records: [R-0087](../docs/research/R-0087-idle-demo-rotation.md).
- Gates: `local/evidence/idle-demo-cycles/gates.sh` against main `c9f0361`'s binaries
  (`base-c9f0361/`), with this task's lap, restores and pictures.
