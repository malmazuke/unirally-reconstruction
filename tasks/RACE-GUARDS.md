# RACE-GUARDS - the last two native guards in the cold-start races

## Assignment

- Status: review. Claimed 24 September 2026 about 13:40Z by the session that closed SPECIAL-TILE-RESPONSE.
- Milestone: M4 breadth (R-0046 next experiments 1 and 3)
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: Claude Opus 5.5 (`claude-opus-5-5`), Claude Code desktop, one session as coordinator, primary and integrator
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  the claiming session's model at default effort. Review tier under D-0008: **tier 1** (race
  state and opponent steering in `src/core/movement.cpp`): a fresh independent reviewer in an
  isolated checkout with a withheld case, and all eleven differential gates actually run.
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): weekly all-models 4% and five-hour 3% at claim (13:41Z); the user's stop
  is 50% weekly or the five-hour limit.
- Reviewer (primary automatically spawns fresh model/effort, isolated checkout; no user
  trigger): a fresh Anthropic subagent at the exact candidate. Withheld case: a held-input
  capture of one of the four tracks that the primary did not compare.
- Dependencies and evidence of acceptance: SPECIAL-TILE-RESPONSE (accepted); R-0046, R-0047;
  pack v11.
- Base commit: the `main` tip at claim.
- Branch and isolated worktree: `task/race-guards` in `.worktrees/race-guards`.
- Owned paths and shared interfaces: `src/core/movement.cpp` (`update_zoom_checkpoint`,
  `update_zoom_ai`) and whatever the two branches reach; native tests; a new research record
  `docs/research/R-0048-race-guards.md`; this record, `docs/STATE.md`, `tasks/README.md`,
  `tasks/NEXT_SESSION.md`, R-0046's matrix rows. The 742-byte ZOOM ZOO and DRAGSTER layouts
  are read-only; the other tracks' `URTRnn02` layout may grow if a divergence proves a
  missing field (bump its version and the projection in `track_reference`, as R-0047 did).
- Claim/lease/heartbeat/checkpoint location: this record.
- Session time limit, concurrency allocation and actual spend authorization if relevant: one
  active worker plus the review subagent; no monetary spend.

## Outcome and boundaries

After SPECIAL-TILE-RESPONSE, native stops in two places on the 16 cold-start race tracks:

1. **`race checkpoint index invalid`** (`update_zoom_checkpoint`): INFINITY (7 laps) at update
   379 and HAIRPIN HILL (5 laps) at 531. The checkpoint-seen flags are 20 bytes at `$114D`,
   indexed `laps_remaining * 4 + checkpoint`; with 5 or 7 laps that index passes 20, so the
   original writes past `$1160`. Recover what those bytes are and whether anything reads them.
2. **`ZOOM ZOO inverted AI marker is unrecovered`** (`update_zoom_ai`): HYBRID at update 2,053
   and MONSTER at 809. The opponent's marker word has bit 15 set; recover the steering the
   original takes then.

Done means INFINITY, HAIRPIN HILL, HYBRID and MONSTER run past these points and match the
original update for update to the end of their captures or to a newly named guard. Out of
scope: tile flag pairs 4, 8, 12, 26 and 28 (still guarded, R-0047), the stunt events, the five
locked tours, new-track finishes and result screens.

## Inputs and prerequisites

- ROM (SHA-256 `a1105819d48c04d6...`, path in `local/rom-location.txt`), the pinned bsnes
  core, pack v11 (`local/classic-pal-crawler-tracks-v11.pack`). A new worktree copies the
  pack, `rom-location.txt`, `local/native` and `classic-crawler-dragster.pack`, and links
  `local/emulators` and `local/toolchain` to the main checkout's.
- The part 2 sweep under `local/evidence/track-breadth/track-breadth-2/sweep/` (horizon frame
  2,900). HYBRID's guard lies past it: capture HYBRID (tour row 2, position 3) to frame 4,400.
- `track_reference recompare --per-track`, `capture`, `explore`, as in R-0047's reproduction.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Checkpoint overflow named | WRAM `$1161` onward on INFINITY and HAIRPIN HILL around the stop; listing of the checkpoint writer (`$81:8050-82B6`) and of every reader of the bytes it reaches | What the bytes are and what reads them | R-0048 |
| AI marker branch recovered | Listing of the opponent steering (`$83:E0xx`, see `update_zoom_ai`) and the original's WRAM at HYBRID's update 2,053 | The steering for an inverted marker, cited by address | R-0048 |
| Match | recompare on the sweep; explore on the longer HYBRID capture and a held-input capture of each track | Each of the four exact past its old stop | explore JSON |
| Nothing accepted moves | The eleven differential gates, v1 contracts, hidden runs, fuzz, ctest, the synthetic suite | All pass with unchanged row digests | gate logs |
| Live | Hidden 4,000-update held-input runs on the four tracks | No guard | run logs |
| Review | Tier 1 | Approved, with the withheld case re-compared | review on the pull request |

## Capability and coverage checkpoint

- Native capability delivered / still missing: all 80 checkpoint-seen flags and the inverted-marker return; nothing left at a guard in the 16 cold-start races.
- Frozen exact-match interval, field set and reference/seed identity: the part 2 sweep and new
  captures; no new freeze.
- Dynamic captured inputs still consumed (must be zero for autonomy): the landing matrices.
- Relevant branches/transitions exercised, including independent variations: flags 21-31 (INFINITY) and 22 (HAIRPIN HILL); 14 inverted-marker updates on three tracks; each track released and with Right held.
- First divergence and cheapest next discriminating experiment: see the handoff.
- Trial-wide usage baseline/current, reserve, reset authorization/outcome or none: 4% weekly at
  claim; no reset authorization needed.

## Evidence and attempts

| Attempt | Hypothesis | Experiment | Observation | Next decision |
| --- | --- | --- | --- | --- |
| 1 (13:42Z) | The checkpoint array is longer than 20 bytes | INFINITY's WRAM `$114D-$1190` per update; boundary fill; listing `$81:CD25` | Flags past `$1160` clear like the first 20; setup fills 80 | Carry 80 flags in URTRnn03 |
| 2 (13:44Z) | The inverted marker just returns | Listing `$83:E0A7`; implemented as a return keeping the direction | INFINITY and HAIRPIN HILL exact; MONSTER differs at 809 in `opponent_horizontal` (original 1) | Find the other writer of `$031B` |
| 3 (13:46Z) | The port-2 reader resets AI inputs first | Listing `$82:AB6F-AB8B` | All inputs released, direction 1, X released, before the AI runs | Neutral direction, A and X masked |
| 4 (13:47Z) | - | recompare; eight new captures to frame 4,400 | All 16 exact to their windows; all eight captures exact to their ends or a finish | Records, gates |

## Handoff

- Current base/head commit and uncommitted state: base `b287670` (`main` after #16);
  implementation `7ea1eee` on `task/race-guards`, records after it.
- Verified findings: [R-0048](../docs/research/R-0048-race-guards.md).
- Current hypothesis and failed approaches: keeping the opponent's direction on an inverted
  marker (attempt 2) failed at MONSTER 809.
- Commands executed, outcomes and report hashes: recompare (all 16 exact); `captures.sh` and
  eight `explore` reports under `local/evidence/race-guards/`; hidden 4,000-update held runs on
  MONSTER, INFINITY, HYBRID and HAIRPIN HILL (0 fallback frames); idle matrix 41 of 45; ctest
  24/24 on lab-debug. Gates: see the closing section.
- Unavailable/skipped checks: `lab-sanitize` and `app-sanitize` (ASan hangs on this host); the
  hosted Linux job covers them.
- Exact next experiment/command: none; [RACE-FINISH-BREADTH](RACE-FINISH-BREADTH.md) is next.
- Remaining dependencies: none.
- Runtime needs (network, build time, fixtures, memory): as SPECIAL-TILE-RESPONSE.
- Aggregate parent/child time, provider usage before/after (or unknown), other-account-work
  caveat: primary from 13:40Z; figures in the closeout.
- Accepted outcome, review/fix rounds and next routing decision: see Review and integration.

## Review and integration

- Reviewer and independent reproduction/withheld-case results:
- Required changes or acceptance rationale:
- Exact merge candidate and required-check results:
- Integrated commit and evidence location:
- Remote synchronization: pushed ref(s), verified local/remote commit IDs, or exact push failure:
- Scope still unverified:
