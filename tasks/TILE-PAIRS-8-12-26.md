# TILE-PAIRS-8-12-26 - the tile flag pairs that still stop four locked tracks

## Assignment

- Status: claimed 25 September 2026 by the Claude Code desktop session that ran LOCKED-TOURS
  (after a context compaction), base `0de428f`.
- Milestone: M4 breadth
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`)
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  the claiming session's model at default effort; **tier 1** (simulation state in
  `src/core/movement.cpp` and `vertical_contact.cpp`).
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): at claim (25 September 2026) the 5-hour window was 7% used and the weekly
  window 9%; the user's stop point is 50% weekly.
- Reviewer (primary automatically spawns fresh model/effort, isolated checkout; no user
  trigger): a fresh Anthropic subagent with a withheld capture.
- Dependencies and evidence of acceptance: LOCKED-TOURS (accepted); R-0047, R-0050; pack v12.
- Base commit: the `main` tip at claim.
- Branch and isolated worktree: `task/tile-pairs` in `.worktrees/tile-pairs`.
- Owned paths and shared interfaces: the tile dispatch and contact code, native tests, a new
  research record, this record, `docs/STATE.md`, `tasks/README.md`, `tasks/NEXT_SESSION.md`.
- Claim/lease/heartbeat/checkpoint location: this record.
- Session time limit, concurrency allocation and actual spend authorization if relevant: one
  worker plus the review subagent; no monetary spend.

## Outcome and boundaries

Four locked race tracks stop at unrecovered tile flag pairs (R-0050):
- LAST ONE (15) at update 808, contact pair 26;
- DOWN+UP (25) at 519, contact pair 26;
- JUMPOVER (19) at 773, movement pair 12;
- HIGHROAD (26) at 898, contact pair 8 or 26.

Recover each pair the captures reach, in contact (`$81:9185-91D1`: pair 26 clears probe
penetrations when `$0355,y` = 9; pair 8 sets `$1349` and changes the correction) and in movement
(the table at `$81:82F5`: pair 8 at `$81:8554`, 12 at `$81:8950`, 26 at `$81:837E`). Also
the HUNTER tracks' announcement differences (TWO LOOPS at 1,252, event 57 against 31; HUNTER 44
with Right held at 1,620, 62 against 30; R-0050).
Out of scope: stunt events.

## Inputs and prerequisites

The locked-tour captures under `local/evidence/locked-tours/sweep/` (`sweep.json`) and
`recompare-5.json`; the listing under `artifacts/static-map/`; pack v12.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Pairs named | Native's error and the original's WRAM at each stop | Which pair, which path | research record |
| Match | `recompare --per-track` on the locked sweep; longer captures | Each of the four exact past its stop | JSON |
| Nothing accepted moves | Gates, v1 contracts, hidden runs, fuzz, ctest, synthetic | Unchanged digests | gate logs |
| Review | Tier 1 | Approved with a withheld capture | review on the pull request |

## Capability and coverage checkpoint

- Native capability delivered / still missing: the loop (pair 26) in movement and contact, pair
  8 in movement and contact, pairs 12 and 28 in movement; pair 4 and the HUNTER tag effects
  (`$83:CEC9`, now [HUNTER-EFFECTS](HUNTER-EFFECTS.md)) are still missing.
- Frozen exact-match interval, field set and reference/seed identity: the 838-byte projection
  grows to 854 (`URTRnn05`); the locked and cold-start sweeps as captured.
- Dynamic captured inputs still consumed (must be zero for autonomy): none new; the loop's x
  steps are pack content (`zoom.loop-offsets`).
- Relevant branches/transitions exercised, including independent variations: the opponent's
  loops on LAST ONE (twice) and DOWN+UP, the player's loop on DOWN+UP with Right held, pair 8
  once on HIGHROAD, pair 12 for 24 updates on JUMPOVER, pair 28 on DOWN+UP.
- First divergence and cheapest next discriminating experiment: TWO LOOPS at 1,252, the first
  HUNTER tag (HUNTER-EFFECTS' handoff).
- Trial-wide usage baseline/current, reserve, reset authorization/outcome or none: 7% (5-hour)
  and 9% (weekly) at claim.

## Evidence and attempts

| Attempt | Hypothesis | Experiment | Observation | Next decision |
| --- | --- | --- | --- | --- |
| 1 | - | `recompare --per-track` on the locked sweep at `0de428f` (`recompare-0.json`) | Unchanged from R-0050: 15, 25 stop at contact pair 26 (808, 519), 19 at movement pair 12 (773), 26 at contact pair 8 (898) | Read the handlers |
| 2 | Contact pair 26 reads movement pair 26's state | Listing `$81:837E-84AB`, `$81:85AB-8622`; WRAM `$0351-$035B` on LAST ONE (`wram.py`) | Movement pair 26 is a loop: `$0355` counts 1 to `$10`, x steps from the table at `$81:834C`, velocity y `$1CE`, pose `$0610+n`; `$0359` a 3-update cooldown. The opponent loops twice (809-842). Contact clears `$28`/`$2C` at step 9 | Implement with the table in the pack (v13) |
| 3 | Pair 8 and 12 are small | Listing `$81:8554`, `$81:8950`, `$81:92D9`, `$81:96FF`, consumers of `$0F2D`, `$0F3B`, `$0F3D`, `$0FB1` | Pair 8: counter `$0D3D`, tile mode, `$0F2D` (slope nudge, pose target); re-contact goes straight to correction; keeps vy under surface mode. Pair 12: drive step 1, animation delta +-2, mud's brake path | Implement; state words into the special-tile block (`URTRnn05`) |
| 4 | - | Explore the four tracks with the new build | LAST ONE, JUMPOVER, HIGHROAD exact to the end; DOWN+UP stops at movement pair 28 (546) | Recover pair 28 (`$81:8316`) |
| 5 | - | Pair 28 (push `$20`/8 unless the reflection is locked) | DOWN+UP exact to the end | Regression and held captures |
| 6 | - | Recompare both sweeps (`recompare-locked.json`, `recompare-cold.json`) | Locked: 19 of 20 exact over the whole window, 41 TWO LOOPS exact to 1,252; cold start 16 of 16, unchanged | TWO LOOPS |
| 7 | TWO LOOPS is an announcement fault | WRAM `$0CC1-$0CE9` at 1,252; the writers of `$0CE7` | The original pushes event 31 to the front (`$81:C55B`), called from the HUNTER-only routine `$83:CEC9`: eight timed tag effects chosen by the player's x when the riders touch | Out of this task's size: HUNTER-EFFECTS |
| 8 | - | Captures with Right held from 1,500, released at 2,600 (`captures.sh`, `right/`) | All four exact over their whole windows; on DOWN+UP the player rides the loop (steps 1-16) | Records and gates |

## Handoff

- Current base/head commit and uncommitted state: base `0de428f`; on `task/tile-pairs`.
- Verified findings: [R-0051](../docs/research/R-0051-loop-and-tile-pairs.md).
- Current hypothesis and failed approaches: none failed; the announcement difference was not
  a queue fault (attempt 7).
- Commands executed, outcomes and report hashes: `local/evidence/tile-pairs/` holds
  `recompare-0.json`, `explore-*.json`, `recompare-locked.json`, `recompare-cold.json`, the
  Right-held captures and their explores, `wram.py`, `explore.sh`, `captures.sh` and the
  `$83:CEC9` extract.
- Unavailable/skipped checks: the ASan presets (host).
- Exact next experiment/command: none for this task; [HUNTER-EFFECTS](HUNTER-EFFECTS.md) is
  next.
- Remaining dependencies: none outside the project.
- Runtime needs (network, build time, fixtures, memory): captures about 15 s each.
- Aggregate parent/child time, provider usage before/after (or unknown), other-account-work
  caveat: primary from about 21:30Z (24 September UTC).
- Accepted outcome, review/fix rounds and next routing decision: pending review.
