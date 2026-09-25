# FRONT-END-1P-SETUP - the one-player setup screens

## Assignment

- Status: ready (queued 26 September 2026 by FRONT-END-MAIN-MENU, second in COVERAGE-ROADMAP's
  queue).
- Milestone: M4 (original game coverage: menus)
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: to be recorded at claim
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  the claiming session's model at default effort; **tier 2** unless the chosen rider, tour or
  track changes race state in a way the race tasks have not accepted (then tier 1).
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): sample at claim.
- Reviewer (primary automatically spawns fresh model/effort, isolated checkout; no user
  trigger): a fresh Anthropic subagent.
- Dependencies and evidence of acceptance: FRONT-END-MAIN-MENU (the front end, the SNES screen,
  the text printer; R-0054); the race scenarios of every track (R-0046, R-0050).
- Base commit: the `main` tip at claim.
- Branch and isolated worktree: `task/front-end-1p-setup` in `.worktrees/front-end-1p-setup`.
- Owned paths and shared interfaces: `src/core/front_end*`, the app's start of a race, pack rules
  and content for the screens, native tests, a research record, this record, `docs/STATE.md`,
  `tasks/README.md`, `tasks/NEXT_SESSION.md`.
- Claim/lease/heartbeat/checkpoint location: this record.
- Session time limit, concurrency allocation and actual spend authorization if relevant: one
  worker plus the review subagent; no monetary spend.

## Outcome and boundaries

After 1P on the main menu (mode 0, `$80:BB9C`, entered once and running the whole one-player
flow), the original shows these screens, then the race:
- PICK A PLAYER: 16 riders, MIKE first;
- PICK TOUR: CRAWLER first; the other tours as SRAM unlocks them;
- PICK TRACK: the tour's five tracks;
- NOW PLAYING: the rider against the opponent, with Race and Exit.

Make native do the same. The rider, tour and track chosen start that race through the existing
race scenarios; a choice the race engine does not cover yet (a stunt event, a rider whose
content is not packed) is refused with a notice rather than approximated. Back and cancel paths
return where the original returns.

Out of scope: what follows a race (FRONT-END-1P-CONTINUATION), persistence beyond reading the
unlocked tours of a cold start, audio.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Pictures and state | Every-frame captures of the original through each screen, with cursor moves both ways, back paths, and choices of several riders, tours and tracks, against native's `front_end_runner` | Pictures match to the pixel and the screens' state words agree frame by frame, or each residue is explained | pictures, logs, research record |
| Race start | For each choice captured, the race native starts | The race state equals the scenario's start that the race gates already accept | log |
| Playable | The app from power-on to a race of a chosen track | Reaches the race | report |
| Nothing moves | ctest, the synthetic suite, the v1 contracts, hidden runs, the differential gates, the equivalence sweep, the main menu's comparisons | Unchanged | logs |

## Handoff

- Exact next experiment/command: capture 1P from power-on with a frame image on every frame and
  per-frame work RAM (FRONT-END-MAIN-MENU's `compare.py` pattern), read `$80:BB9C` and what it
  calls, and list the screens' loads with `access capture` register logs at `$82:B2DD`,
  `$82:B1DB` and `$82:B183`. Check each screen's register writes for HDMA (`$420C`),
  windows and mosaic: `snes_screen` does not model them, so a screen that uses one needs them
  added (or its `unmodelled_features` set, which refuses it). 2P turns HDMA on (R-0054).
