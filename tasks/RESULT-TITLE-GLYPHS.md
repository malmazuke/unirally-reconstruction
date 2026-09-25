# RESULT-TITLE-GLYPHS - the result title's `+` and `!`

## Assignment

- Status: **in progress**. Claimed 25 September 2026 at 12:40Z by the Claude Code desktop
  session that ran NATIVE-READABILITY, on base `47c7644` (prepared the same day by the
  TILE-PAIRS-8-12-26 session). The user asked for tasks to continue one after another toward
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

## Handoff

- Exact next experiment/command: find a controller schedule on which the player finishes
  DOWN+UP (with Right held from frame 1,450 only the opponent finishes, at 4,427), capture
  frame images at the stable result, and compare with native's candidates.
