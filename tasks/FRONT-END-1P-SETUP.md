# FRONT-END-1P-SETUP - the one-player setup screens

## Assignment

- Status: **in progress**. Claimed 26 September 2026 at 16:25Z by the Claude Code desktop
  session that ran FRONT-END-MAIN-MENU, on base `a76961e`.
- Milestone: M4 (original game coverage: menus)
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`)
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  the claiming session's model at default effort; **tier 2** unless the chosen rider, tour or
  track changes race state in a way the race tasks have not accepted (then tier 1).
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): at claim the 5-hour window was 14% used and the weekly window 28%. The
  user's allowance: continue until the weekly window reaches 50%.
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

- Current base/head commit and uncommitted state: `task/front-end-1p-setup` from `a76961e`; claim
  commit only.
- Findings so far (listing, not yet captured in detail):
  - The 1P handler `$80:BB9C` chains the screens, each with a back path:
    1. `$80:CB04` with the table `$80:BCAF`: PICK YOUR UNI (a result of 0x10 or more means
       exit, to `$80:BC9B`).
    2. It stores the rider in `$017D`/`$00CA`, the opponent 0x10 in `$017F`, clears SRAM
       `$77:1075-10A6`, sets `$77:1073 = 3` and `$77:10AD = 1`, and runs `$80:F4E9`,
       `$80:A858` and `$80:A82B`.
    3. `$80:E550`: PICK TOUR (`$000A = $00D0`); back (`$80:B74A`) returns to step 1.
    4. `$80:E84E`: PICK TRACK; back returns to step 3 (`$83:9EB4`, SRAM `$77:069C` against
       `$77:10D1`, `$83:8957`).
    5. `$80:B18D`: NOW PLAYING; back returns to step 4.
    6. The fade out (`$80:9885`), then `$80:99A4`, the race.
  - Evidence so far: `local/evidence/front-end-1p-setup/` `defaults.json`, with frame images
    600-1399 and per-frame work RAM, and `NOTES.md` (the four screens).
  - The screens show SRAM content (records, medals, "?" badges). Consider splitting this task
    by screen: PICK YOUR UNI first.
- Exact next experiment/command: register-log captures of the loads (`access capture
  --watch-pc 0x82B2DD --watch-pc 0x82B1DB --watch-pc 0x82B183`) and every PPU write (the
  `accesses` list) over frames 600-800; then read `$80:CB04` and its table `$80:BCAF`.
