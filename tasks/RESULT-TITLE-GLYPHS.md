# RESULT-TITLE-GLYPHS - the result title's `+` and `!`

## Assignment

- Status: **accepted** 25 September 2026 (tier 2,
  [#26](https://github.com/malmazuke/unirally-reconstruction/pull/26)). Claimed 25 September
  2026 at 12:40Z by the Claude Code desktop session that ran NATIVE-READABILITY, on base
  `47c7644` (prepared the same day by the TILE-PAIRS-8-12-26 session). The user asked for tasks to continue one after another toward
  full native coverage.
- Milestone: M4 breadth
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`)
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  the claiming session's model at default effort; **tier 2** (presentation only:
  `src/core/presentation.cpp`; no simulation state).
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): at claim the 5-hour window was 23% used and the weekly window 24%. The
  user's allowance: continue until the weekly window reaches 50%.
- Reviewer (primary automatically spawns fresh model/effort, isolated checkout; no user
  trigger): a fresh Anthropic subagent.
- Dependencies and evidence of acceptance: R-0051 (the defect); pack v13.
- Base commit: the `main` tip at claim.
- Branch and isolated worktree: `task/result-title-glyphs` in `.worktrees/result-title-glyphs`.
- Owned paths and shared interfaces: `result_title_tile` and the title check (in
  `src/core/result_screen.cpp` since NATIVE-READABILITY split `presentation.cpp`), native
  tests, a research note, this record.
- Claim/lease/heartbeat/checkpoint location: this record.
- Session time limit, concurrency allocation and actual spend authorization if relevant: one
  worker plus the review subagent; no monetary spend.

## Outcome and boundaries

The one-run result screen titles the race with the track's name. Native's font mapping
(`result_title_tile`) covers 0-9 and a-z; `_` is a one-tile space. Two one-run tracks' names
use other characters: `down+up` (25) and `boo!` (28). Native checks the name when it loads the
track's content, so the app refuses both tracks at start, though their simulation is exact
(R-0051). `to_and_fro'` (44) is a lap race and draws no title.

Find what the original draws for `+` and `!`, either the tile or whether it skips them, and
draw that. The font's slots after `z` (`$46-$4E`) hold a blob, two arrows and two unclear
shapes (`local/evidence/tile-pairs/glyphs.py` prints them). The routine that maps name bytes to
tiles is not yet found; the name table's printer (`$80:9B55`) copies through the WRAM trampoline
at `$00:0199`.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Glyphs named | Original result screen of DOWN+UP and BOO! (a capture where the player finishes, or a lab probe of VRAM) | The tile or rule for `+` and `!` | research note |
| Match | Native result pictures against the original's | 0 differing pixels in the title | pictures |
| Playable | `frontend run --track 25` and `--track 28 --hidden` | Both start and run 4,000 updates | reports |
| Nothing accepted moves | Presentation contracts, hidden runs, ctest | Unchanged | logs |

## Result

The original prints the result title with its general text printer, and the printer's own table
decides every glyph ([R-0053](../docs/research/R-0053-result-title-printer.md)).

- The result stream `$80:D187` prints the name with `F7`, centred with `FC 02`. Its `EE`
  (`$80:F8B9`) first makes the letters uppercase and the digits the big font's.
- The table `$80:C709` then gives letters and digits a big 2 x 2 glyph: native's fitted layout,
  confirmed for all 36.
- Any other byte gets a small glyph, one tile wide: `+` is tile `$A5` over `$E1`, `!` is `$A0`
  over `$DC`, and `_` is the small font's space, `$CE` over `$10A`.
- **Native** (`src/core/result_screen.cpp`): `result_title_glyph` gives each title byte its
  glyph and size, and `result_title_width` centres the title. The underscore now draws the small
  space, as the original does; FLAT FUN's pictures are identical either way.
- **A composition the capture added**: on DOWN+UP the winner's result loads while the opponent
  still rides. Native's guard required both finishes. A winner's result now needs only the
  player's, and a loser's still needs the opponent's.
- **Lab tooling**: `track_reference`'s original rows took the outcome from both finishes and
  failed on that capture. A rider still riding at the load now counts as finishing last.
- **Result**: DOWN+UP and BOO! start in the app and run. Their result titles match the
  original's shapes on every captured frame, and match to the pixel on the frames in native's
  palette phase.

Decisions and deviations, with reasons:

- **The schedules were found natively, then captured.** No known schedule finished the player
  on either track: with Right held, only the opponent finishes. A beam search on the native
  runner, which is exact on both tracks, found schedules on which the player wins. The original
  then played them exactly (`explore`), which is what makes the native search valid.
- **The rest of the frame is not this task's.** On the phase-matched frames about 1,000 pixels
  differ, all in the icons beside the rows (tile columns 2-6, 15-16 and 25-29): the `1P` arrow,
  the unicycles and the award icons. Native's result screen does not draw them, a declared
  omission since R-0038. The other frames also differ in the result palette's phase, about
  26,000 pixels, the title's colours among them; their shapes match.

## Evidence and attempts

| Attempt | Hypothesis | Experiment | Observation | Next decision |
| --- | --- | --- | --- | --- |
| 1 | Right held finishes the player | Native, Right, Right with jumps and random schedules for 5,000 updates on 25 and 28 | Only the opponent finishes; the player stalls at the wrap of DOWN+UP's playfield | Search |
| 2 | A search finds a winning schedule | `beam.py`: 40-update held moves, beam 4, scored along the opponent's route | The player finishes first on both (updates 2,800 and 2,840) | Capture |
| 3 | - | `captures.sh`: the original on both schedules, frame images every 50 frames across the result; `explore` | Exact over 3,901 and 3,941 rows, through the finish and the result load; the DOWN+UP winner's result loads with the opponent riding (explore needed the tooling fix) | Find the glyphs |
| 4 | The title uses a character table | The static listing: `$80:C3BC`, its table `$80:C709`, the name handler `$80:C628`, `EE` at `$80:F8B9`, the stream `$80:D187` | Letters and digits big; `+`, `!`, `_`, `'` small, one tile | Implement |
| 5 | Native now matches | `render.py`, `shape.py` on 26 original frames | Title rows 0 px on 3 phase-matched frames per track; shapes identical on all 26 | Controls |
| 6 | The pictures would catch a wrong tile | `+` and `!` as the small space, then as the neighbouring tiles | 24 / 28 and 44 / 58 title pixels differ | Underscore |
| 7 | The underscore's change is invisible | `flatfun.py`: TRACK-BREADTH's FLAT FUN capture, base `a2169c3` against the candidate | Pictures identical on all 9 frames | Gates |
| 8 | - | The app, hidden, 4,000 updates with Right held, on 25 and 28 | Both start and run; 0 rider-pose fallback frames | Gates |
| 9 | - | Gates at `309890e` (below) | GATES_PENDING | Review |

## Handoff

- Current base/head commit and uncommitted state: `task/result-title-glyphs` from `47c7644`;
  the candidate is `309890e`, plus these records.
- Verified findings: the Result above and R-0053.
- Commands executed, outcomes and report hashes: `local/evidence/result-title-glyphs/` in the
  main checkout.
  - `beam.py` and `holds.py`: the search, and its schedules as capture holds
    (`beam-25.json`, `beam-28.json`).
  - `captures.sh`: the captures `down-up/` and `boo/`, with their logs, and `explore-*.json`.
  - `render.py`, `shape.py`: native's result frames (`native-*.ppm`) and the comparisons.
  - `flatfun.py`: base against candidate on FLAT FUN.
  - `hidden-25.json`, `hidden-28.json`: the app runs.
  - `gates.sh` and `gates-309890e.out`: the gates, run in a detached
    `.worktrees/result-title-glyphs-gates`.
- Unavailable/skipped checks: the ASan presets (host; the Linux CI job covers them). The
  eleven differential gates were not rerun: their binary, `zoom_zoo_runner`, links only
  `unirally_movement`, which this task does not touch. The app runs hold Right, so they do
  not reach a result; the result pictures come from `classic_race_presentation_runner`, which
  uses the app's code.
- Exact next experiment/command: none for this task. Next is a coverage roadmap toward the
  whole game natively (the user's stated goal, 25 September 2026), then the tasks it queues.

## Review and integration

- Tier 2: pending.
