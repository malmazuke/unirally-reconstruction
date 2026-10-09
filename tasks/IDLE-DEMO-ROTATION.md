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
