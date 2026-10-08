# ROLLING-CONTACT - a rolling rider's second contact after a long shoulder rotation

## Assignment

- Status: **accepted** 9 October 2026 (tier 1,
  [PR #62](https://github.com/malmazuke/unirally-reconstruction/pull/62)); claimed 9 October 2026
  (8 October 20:30 UTC) at the user's "Next task", on main `5da988e` (equal to `origin/main`).
  Prepared 25 September 2026 by the HUNTER-EFFECTS session, from its review. Chosen over
  PORTABLE-CORE-IDENTITY (tier 2 tooling) because it is a gameplay divergence in shared contact
  code; ATTRACT-DEMO is an OpenAI task (D-0004).
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
  trigger): a fresh Claude Opus 5.5 subagent in `.worktrees/rolling-contact-review` at `cd712f5`,
  with its own search and two withheld captures; review on the pull request.
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

## Gates

`local/evidence/rolling-contact/gates.sh` against main `5da988e`'s binaries (`base-5da988e/`),
pack v35; output `gates-cd712f5.out`, directory `gates-cd712f5/` (main checkout, after closeout).
On `cd712f5`, tree clean at start and end (20:57-22:46 UTC):
- presets and ctest: 41 of 41 on each of lab-debug, lab-release, app-debug and app-release;
  synthetic suite, v1 contracts, hidden runs (8 tracks and the front end): pass;
- the eleven frozen gates: pass with their recorded row digests;
- race sweep: 432 runs, 2,387,105 updates, 0 differences; front-end sweep: 179 of 179 equal;
- every split, race-end, look and restart capture: 0 word differences; cues identical to main;
- track captures: this task's four, HUNTER-EFFECTS' 15 withheld and 47 held captures all exact
  to their end (`explore-summary.txt`; the script's one-line summary had a quoting bug, so the
  JSONs were read directly); the track-breadth-2 and locked-tour sweeps give main's rows;
- tooling 544 tests OK; no function over 80 lines; the address index is current;
- fuzz: 40 aborts, as on main (recorded non-pass).
ASan presets are unavailable on this host (macOS 27); Linux CI covers them. Hosted CI on
`cd712f5`: changes, lab (ubuntu-24.04) and lab (macos-15) pass.

## Review

[Agent review (Claude Opus 5.5, tier 1) of cd712f5](https://github.com/malmazuke/unirally-reconstruction/pull/62#pullrequestreview-5462918673):
**approved**. It rebuilt, reran ctest, reproduced all five captures with the fix and with main's
binary, checked the listing (every reader of `$0FA9` after the stop; `$81:966F` unreachable;
the `$81:97DB` half never meets the stop), and captured two withheld cases from its own search:
`t23-seed1022` (track 23) and `t40-seed2008` (track 40), exact with the fix, main diverging at
2,011 and 847. Three advisories, all handled in the closing commit: an unmirrored stop case in
the unit test (added), the opponent sweep's null result in R-0086 (added), and re-bootstrapping
the main checkout after the review worktree goes (done at closeout).

## Handoff

- Cause and fix: [R-0086](../docs/research/R-0086-rolling-contact.md). Integrated by PR #62;
  the closeout (`artifacts/rolling-contact-integration/closeout.json` in the main checkout) holds
  the merge commit, the check runs and the cleanup list.
- At closeout: the scratch scripts and the review's scripts are copied to
  `local/evidence/rolling-contact/` (`review/` for the reviewer's), the gate directory and its
  output move there, both worktrees and their build output are removed, the branch is deleted
  by GitHub on merge, and `python3 tools/project.py bootstrap` is rerun in the main checkout.
- Open: no capture shows the opponent on this branch; tracks 12 and 42 reach it only in the
  native search.
