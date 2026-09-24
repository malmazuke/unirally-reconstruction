# LOCKED-TOURS - the five tours a cold start does not offer, and their 25 tracks

## Assignment

- Status: in progress. Claimed 24 September 2026 about 16:10Z by the session that closed RACE-FINISH-BREADTH.
- Milestone: M4 breadth (R-0046 next experiment 5)
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: Claude Opus 5.5 (`claude-opus-5-5`), Claude Code desktop, coordinator, primary and integrator
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  the claiming session's model at default effort. Review tier under D-0008: tier 1 if race
  state or the pack format changes (new scenarios and pack entries are expected), else tier 2.
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): weekly all-models 6% and five-hour 20% at claim (16:10Z).
- Reviewer (primary automatically spawns fresh model/effort, isolated checkout; no user
  trigger): a fresh Anthropic subagent at the exact candidate, with a withheld track.
- Dependencies and evidence of acceptance: RACE-FINISH-BREADTH (accepted); R-0046 to R-0049.
- Base commit: the `main` tip at claim.
- Branch and isolated worktree: `task/locked-tours` in `.worktrees/locked-tours`.
- Owned paths and shared interfaces: the scenario table in `src/core/movement.cpp`, pack rules
  and `tools/unirally_lab/content/tracks.py` (a new profile if entries are added),
  `track_reference` (capture through an unlocked menu), native tests, a new research record,
  this record, `docs/STATE.md`, `tasks/README.md`, `tasks/NEXT_SESSION.md`, R-0046's matrix.
- Claim/lease/heartbeat/checkpoint location: this record.
- Session time limit, concurrency allocation and actual spend authorization if relevant: one
  active worker plus the review subagent; no monetary spend.

## Outcome and boundaries

The cold-start PICK TOUR screen offers CRAWLER, SHUFFLER, WALKER and HOPPER. JUMPER, BOUNDER,
RUNNER, SPRINTER and HUNTER hold the other 25 tracks. They are located and unpacked, but they
have no captured scenario (race mode, laps, initialization frame) and no comparison with the
original (R-0046, "Tours" hypothesis).

Done means:
- the unlock condition is recovered from the original (the menu code and the SRAM progression
  it reads), and a documented capture method reaches every locked tour;
- each locked race track has a native scenario from captures, pack entries for its content,
  and a released-controller comparison with the original, with any new guard named;
- the stunt events among them are listed but not compared (no native stunt event).

Recording an unlocked SRAM as a capture input is a laboratory method on the original side
only, so decide it explicitly and record it. The product never executes original code.

## Inputs and prerequisites

- ROM, pinned core, pack v11. The name table (`$83:9FFA`) and the tour-to-track rule (index =
  5 x tour + position) are in R-0046 observations 5-6. The menu inputs are in
  `track_reference.menu_events`.
- Tracks 25-28 and 35-44 are in the idle matrix; LAST ONE and DOWN+UP stop at flag pair 26 and
  LITTLE DIPPER on its shape `$04` (R-0048).

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Unlock | Listing of the PICK TOUR code and the SRAM it reads; a capture that shows a locked tour | The condition and the capture method, cited | research record |
| Scenarios | Captures of every locked race track through the menu | Race mode, laps and boundary per track | research record, scenario table |
| Match | `track_reference` on each | Exact over the window or a named guard | JSON |
| Nothing accepted moves | Gates, v1 contracts, hidden runs, fuzz, ctest, synthetic | All pass, unchanged digests | gate logs |
| Review | As the tier | Approved, with a withheld track | review on the pull request |

## Capability and coverage checkpoint

- Native capability delivered / still missing: to be filled.
- Frozen exact-match interval, field set and reference/seed identity: new captures.
- Dynamic captured inputs still consumed (must be zero for autonomy): the landing matrices.
- Relevant branches/transitions exercised, including independent variations: to be filled.
- First divergence and cheapest next discriminating experiment: see the handoff.
- Trial-wide usage baseline/current, reserve, reset authorization/outcome or none: at claim.

## Evidence and attempts

| Attempt | Hypothesis | Experiment | Observation | Next decision |
| --- | --- | --- | --- | --- |
| 1 (16:12Z) | The names are printed through a table | Listing | `$80:9B55` prints name *n* through the pointer table `$83:9F96` (entries for `$83:9FFA` on) | Find the PICK TOUR caller later; try SRAM first |
| 2 (16:14Z) | Winning a race records progression in SRAM | SRAM diff over `race-finish-breadth/east-right` (EAST won) | After the race: `$0825` 0 to 4, `$07D5` 0 to 1, `$07D7` 0 to 4, `$1118` 0 to 2, plus times at `$0618`, `$073C`, `$0755`-`$07D4`, `$086B`, `$0E69`. The AI reads `$77:0825` (`$83:E140`) | Find the reader of these near the PICK TOUR builder |
| 3 (16:20Z) | `$1118` is read by the menu | ROM scan for long accesses | Only written (ORA/STA at `$83:FAAE`-`FAD4`); the menu code near `$80:C950` is unclassified | Empirical: preload SRAM |
| 4 (16:25Z) | Some SRAM byte unlocks tours | Lab probe (`local/evidence/locked-tours/probe.py`): power on with patched cartridge RAM, walk to PICK TOUR, picture | `$1000-$1FFF` = `$FF` lists all nine tours; bisection: `$77:1000` alone; any nonzero value (1, 2, 3, 4, 5, 8, 9, `$10`) unlocks all five | Capture method: preload `$77:1000` = 1; find the cursor order |

## Handoff

- Current base/head commit and uncommitted state: not claimed; prepared on
  `task/race-finish-breadth`.
- Verified findings: none yet.
- Current hypothesis and failed approaches: tours unlock by progression that SRAM records.
- Commands executed, outcomes and report hashes: none yet.
- Unavailable/skipped checks: the ASan presets are unavailable on this host.
- Exact next experiment/command: find the PICK TOUR screen's list builder in the static map
  (`docs/map/static/code-banks.md`; the listing is `artifacts/static-map/` in the main
  checkout). Look for reads of the tour names in `$83:9FFA` and of SRAM `$77:07xx` near the
  menu, and diff SRAM between a cold start and a capture after a won CRAWLER race.
- Remaining dependencies: none outside the project.
- Runtime needs (network, build time, fixtures, memory): as RACE-FINISH-BREADTH.
- Aggregate parent/child time, provider usage before/after (or unknown), other-account-work
  caveat: to be recorded.
- Accepted outcome, review/fix rounds and next routing decision: to be recorded.

## Review and integration

- Reviewer and independent reproduction/withheld-case results:
- Required changes or acceptance rationale:
- Exact merge candidate and required-check results:
- Integrated commit and evidence location:
- Remote synchronization: pushed ref(s), verified local/remote commit IDs, or exact push failure:
- Scope still unverified:
