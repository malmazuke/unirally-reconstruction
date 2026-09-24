# LOCKED-TOURS - the five tours a cold start does not offer, and their 25 tracks

## Assignment

- Status: review. Claimed 24 September 2026 about 16:10Z by the session that closed RACE-FINISH-BREADTH.
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
| 5 (16:30Z) | Down and Right walk PICK TOUR | Probe pictures after k Downs / Right | Down walks CRAWLER, SHUFFLER, WALKER, HOPPER, HUNTER; Right takes JUMPER, BOUNDER, RUNNER, SPRINTER | `menu_events(position, row, column)` |
| 6 (16:40Z) | A preload of `$1000` = 1 survives to the race | Captures; SRAM traces | Fresh RAM is `$FF` and the menu formats it at frames 403-405; the core loads RAM from `<save dir>/<rom>.srm` at power-on (a memory write before Strict's reset is lost); with `$1000` alone the menu lists the tours but still loads CRAWLER's track | Find what selection needs |
| 7 (16:50Z) | Selection needs more bytes | Bisection with `$1000` = `$FF` | Bytes in both `$10C0-$10DF` and `$10E0-$10FF` are needed, no single pair found; `$1000-$1FFF` = `$FF` on the formatted cold image works | Method: preload that image (`track_reference capture --unlock-tours`) |
| 8 (16:58Z) | - | 25 captures (`captures.sh`), released, horizon 2,900 | Tracks 5-9, 15-19, 25-29, 35-39, 40-44 = 5 x tour + position; stunt events at position 2 (7, 17, 27, 37, 42); laps 2, 3 or 5 | Scenarios and pack entries for the 20 race tracks |
| 9 (17:05Z) | - | Pack v12 (`tracks.v12_new_entries`: 20 tracks and sceneries 1, 8, 12); 20 scenarios; recompare on the `$1000-$1FFF` preload | All differ at update 0 in the SRAM graph extrema (`$106F`) and `hints_active`: the fill reached them | Preload only `$1000` and `$10C0-$10FF` |
| 10 (17:10Z) | - | Recapture (same tracks, modes, laps) and recompare (`recompare-2.json`) | **12 of 20 exact over their windows** (5, 6, 8, 9, 16, 18, 28, 29, 35, 36, 38, 39); LAST ONE and DOWN+UP stop at pair 26; JUMPOVER (551, opponent) and HIGHROAD (741, opponent jump) diverge; HUNTER 40, 43, 44 differ in `ai.suppression_counter` (30 vs 60) and 41 in `progress_adjustment` | `$1275`/`$1283` per track |
| 11 (17:12Z) | HUNTER is a higher tier | `$1275`, `$1283`, `$1281` at every boundary | 1, 0 on every track but HUNTER's 40, 41, 43, 44: 3, 64; `$1281` 96 even on HUNTER's lap races (72 elsewhere) | Recover `$83:E16B`'s other arms and `$83:CC59`'s bound; add the tier to the scenario |
| 12 (17:25Z) | Level 3 launches always and suppresses for 60 | `ClassicRaceScenario::ai_level` / `adjustment_limit` (3 and 96 for tracks 40-44); recompare-3 | **13 of 20 exact** (40 joins); 41, 43, 44 now differ in the opponent's jump input, as HIGHROAD (26) does at level 1, and JUMPOVER (19) in its direction | The AI turnaround `$83:E0C5-E111` (`$0C73`), unmodelled |
| 13 (17:40Z) | The turnaround explains HIGHROAD and JUMPOVER | `opponent_turnaround` (`$0C73`) in `update_zoom_ai`; extension grows to 36 bytes (URTRnn04 838, URZZ000D/URDG0003 778 while live); projection appends `$0C73`; recompare-4 | JUMPOVER now runs to update 773 (movement pair 12, unrecovered) and HIGHROAD to 898 (contact pair 8 or 26); still 13 exact; HUNTER 41, 43, 44 differ in the opponent's jump (native 0, original 1) at level 3 | Level 3's jump; update the unit tests to the new sizes |
| 14 (17:50Z) | Level 3 skips the jump override | `$83:E135`: a level other than 1 takes `$83:E222` at once; native's count-below-4 branch now does too; recompare-5 | **15 of 20 exact** (43, 44 join); TWO LOOPS (41) exact to 1,252, then `player_queue.entry14` (57 vs 31); LAST ONE, DOWN+UP, JUMPOVER, HIGHROAD stop at pairs 26, 26, 12 and 8/26 | TWO LOOPS' announcement, then records |

## Handoff

- Current base/head commit and uncommitted state: base `07fdb87`; on `task/locked-tours`,
  commits through the scenario table and pack v12 (see `git log`).
- Verified findings: [R-0050](../docs/research/R-0050-locked-tours.md). The unlock preload and
  menu path (attempts 4-8); 12 of 20 exact with content and scenarios (attempt 10), 15 with
  the AI level, the jump and the turnaround (attempts 12-14, `recompare-5.json`, unchanged by
  the review fix in `recompare-6.json`).
- Current hypothesis and failed approaches: a `$1000`-only preload (attempt 6) and a whole
  `$1000-$1FFF` fill (attempt 9) both fail, for different reasons.
- Commands executed, outcomes and report hashes: `local/evidence/locked-tours/captures.sh`,
  `sweep/sweep.json`, `recompare-1.json` (wide fill), `recompare-2.json` (narrow preload) to
  `recompare-6.json` (after the review fix).
- Unavailable/skipped checks: the ASan presets (host).
- Exact next experiment/command: none for this task; [TILE-PAIRS-8-12-26](TILE-PAIRS-8-12-26.md)
  is next (the four tile stops and TWO LOOPS' announcement).
- Remaining dependencies: none outside the project.
- Runtime needs (network, build time, fixtures, memory): captures about 12 s each.
- Aggregate parent/child time, provider usage before/after (or unknown), other-account-work
  caveat: primary from 16:10Z.
- Accepted outcome, review/fix rounds and next routing decision: to be recorded.

## Review and integration

- Reviewer and independent reproduction/withheld-case results:
- Required changes or acceptance rationale:
- Exact merge candidate and required-check results:
- Integrated commit and evidence location:
- Remote synchronization: pushed ref(s), verified local/remote commit IDs, or exact push failure:
- Scope still unverified:
