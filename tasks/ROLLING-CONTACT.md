# ROLLING-CONTACT - a rolling rider's second contact after a long shoulder rotation

## Assignment

- Status: **in progress**, claimed 9 October 2026 (8 October 20:30 UTC) at the user's "Next task", on
  main `5da988e` (equal to `origin/main`). Prepared 25 September 2026 by the HUNTER-EFFECTS
  session, from its review. Chosen over PORTABLE-CORE-IDENTITY (tier 2 tooling) because it is a
  gameplay divergence in shared contact code; ATTRACT-DEMO is an OpenAI task (D-0004).
- Milestone: M4 breadth
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`); one
  session as coordinator, primary and integrator
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  the claiming session's model at default effort; **tier 1** (`src/core/vertical_contact.cpp`,
  movement).
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): at claim (8 October 2026 20:27 UTC) weekly 0%, five-hour 0%. No standing
  rule to chain tasks: this session stops after this task.
- Reviewer (primary automatically spawns fresh model/effort, isolated checkout; no user
  trigger): a fresh Anthropic subagent with a withheld capture.
- Dependencies and evidence of acceptance: HUNTER-EFFECTS (R-0052).
- Base commit: main `5da988e`.
- Branch and isolated worktree: `task/rolling-contact` in `.worktrees/rolling-contact`.
- Owned paths and shared interfaces: the contact and pose code, native tests, a research
  record, this record, `docs/STATE.md`, `tasks/README.md`, `tasks/NEXT_SESSION.md`.
- Claim/lease/heartbeat/checkpoint location: this record.
- Session time limit, concurrency allocation and actual spend authorization if relevant: one
  worker plus the review subagent; no monetary spend.

## Outcome and boundaries

HUNTER-EFFECTS' review capture `w-rev-buttons` (TWO LOOPS, Right from 1,585 with Y, A, L, R, B,
X and Left under effect 7) matches to update 1,737 (frame 3,128). There the player's velocity y
is 72 natively and 0 in the original. The published inputs are equal through the divergence.

It is the second contact of a rolling rider (surface mode 1, orientation 37 to 32) after a
32-update airborne rotation on the shoulder buttons. No HUNTER word is read on that path, so the
cause is in the shared contact code.

Find the branch native misses, and make the capture exact past 1,737.

## Inputs and prerequisites

`local/evidence/hunter-effects/review-withheld/w-rev-buttons` (in the main checkout after
HUNTER-EFFECTS' closeout), pack v14, the listings under `artifacts/static-map/`.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Cause named | WRAM around frame 3,128 and the contact listing | The branch and its words | research record |
| Match | `track_reference explore` on the capture; a withheld rolling capture | Exact past 1,737 | JSON |
| Nothing accepted moves | Gates, v1 contracts, hidden runs, fuzz, ctest, synthetic | Unchanged digests | gate logs |
| Review | Tier 1 | Approved with a withheld capture | review on the pull request |

## Baseline at claim

On main `5da988e`, lab-debug, pack v35: `track_reference explore` on `w-rev-buttons` is exact for
1,737 updates; the first divergence is update 1,737 (frame 3,128), `player.velocity_y` 72 native
against 0 original, as HUNTER-EFFECTS recorded with pack v14/v15. The disk had 9.6 GB free at
claim, so captures here stay small and are stripped after use.

## Evidence and attempts

Captures, the access capture, the search tools and main's binaries (`base-5da988e/`) are in main
`local/evidence/rolling-contact/`; the scratch scripts (`side.py`, `trace.py`, `search.py`,
`capture_seed.py`, `hit_tiles.py`, `debug.patch`) are copied there at closeout.

| Attempt | Hypothesis | Experiment | Observation | Next decision |
| --- | --- | --- | --- | --- |
| 1 | The divergence is in the contact step of update 1,737 | A temporary native trace of the player's contact (`debug.patch`) against the capture's WRAM | Native: supported, not leading, angle 24, high tile `$C50E`, incoming velocity (-36, -112), unsupported 2; the inverted-face test zeroes velocity x, then the slope sets velocity y 72 from the incoming -36. Original: (0, 0) | Read the listing |
| 2 | The slope reads the scratch velocity x after the stop | `$81:924E-9275`, `$81:9610-97E4` | `$81:96B6 LDA $0FA9` reads the word `$81:9265-9275` cleared | Fix `follow_slope`; confirm dynamically |
| 3 | The original executes that path at 3,128 | `access capture` of the same inputs (`w-rev-buttons.json`), PCs `$81:9265`, `$81:96B6`, `$81:96CD`, `$81:970B`, `$81:97E4`; `$0200-$1FFF` equal to the reference on frames 1,391-3,135 | `$81:9265` runs once, at 3,128, A = `$FFDC`; the shifted value at `$81:96CD` is 0; `$81:970B` stores 0 | Native reads `moved.velocity_x` |
| 4 | Native is exact past 1,737 | `track_reference explore` | 2,210 of 2,210 | Find other reaches of the branch |
| 5 | Other schedules reach it | `search.py`: main against the fix on random schedules, tracks 2-44 | The race sweep never reaches it; 10 of 2,064 search schedules do (tracks 12, 14, 21, 40, 41, 42) | Capture the original on four |
| 6 | The fix holds on other tracks and on the unmirrored face | `seed7-a`/`-b` (41), `t14-seed23`, `t21-seed20`, `t14-seed28` | Main diverges on each at the searched update; the fix is exact to each end (910, 925, 2,334, 1,675). The unmirrored face gives velocity y -1, as predicted | Gates, review |

## Handoff

- Cause and fix: [R-0086](../docs/research/R-0086-rolling-contact.md). Next: the gates
  (`local/evidence/rolling-contact/gates.sh` with `EXPLORE_CAPTURES` set to the five captures and
  HUNTER-EFFECTS' other withheld captures), then the tier 1 review with a withheld capture.
