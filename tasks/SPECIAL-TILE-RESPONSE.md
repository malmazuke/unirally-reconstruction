# SPECIAL-TILE-RESPONSE - the vertical contact response on the tile flags no accepted track reached

## Assignment

- Status: accepted (reviewed and integrated by pull request #16). Claimed 24 September 2026 about 08:25Z, after the weekly reset (prepared
  23 September 2026 by the TRACK-BREADTH session).
- Milestone: M4 breadth (the follow-up TRACK-BREADTH's matrix names first)
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: Claude Opus 5.5 (`claude-opus-5-5`), Claude Code desktop, one
  session as coordinator, primary and integrator
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  the claiming session's model at default effort. Review tier under D-0008: **tier 1**
  (simulation state and arithmetic in `src/core/vertical_contact.cpp`), so the full D-0006
  process: a fresh independent reviewer in an isolated checkout with withheld cases, returned
  rounds until approval, and all eleven differential gates actually run (the change reaches the
  gate binary, so `gate_identity` cannot cite them).
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): at claim (08:25Z) weekly all-models 0% and five-hour 0%; at 08:50Z 1% and
  7%. The user asked for work to continue task after task until 50% weekly or the five-hour
  limit; the 80% review reserve is far off.
- Reviewer (primary automatically spawns fresh model/effort, isolated checkout; no user
  trigger): a fresh Anthropic subagent at the exact candidate. Withheld case: a stop the primary
  did not use as evidence (see the table below), re-compared by the reviewer.
- Dependencies and evidence of acceptance: TRACK-BREADTH (accepted); R-0046 (the matrix and
  observations 1-18); R-0011, R-0024 and R-0025 (the recovered vertical contact); the v10 pack.
- Base commit: the `main` tip at claim.
- Branch and isolated worktree: `task/special-tile-response` in `.worktrees/special-tile-response`.
- Owned paths and shared interfaces: `src/core/vertical_contact.cpp`/`.hpp` and whatever the
  response reaches in `src/core/movement.cpp`; native tests under `tests/native/`; a new
  research record `docs/research/R-0047-special-tiles.md`; this record,
  `docs/STATE.md`, `tasks/README.md`, `tasks/NEXT_SESSION.md`, R-0046's matrix rows. The
  742-byte state is read-only unless a divergence proves a missing field.
- Claim/lease/heartbeat/checkpoint location: this record.
- Session time limit, concurrency allocation and actual spend authorization if relevant: one
  active worker plus the review subagent; no monetary spend.

## Outcome and boundaries

Native reproduces the original's vertical contact response when a rider's contact summary
carries a tile flag outside the recovered set {0, 2, 6, 7, 18, 20}. Today native stops there
with the guard `vertical contact reaches a special response tile`
(`src/core/vertical_contact.cpp:140`). That guard is the most common stop in TRACK-BREADTH's
matrix. It fires on 7 of the 16 compared cold-start races inside their windows, on CROCK and
WARIO PAINT just after, and on 19 of the 45 idle runs. It is also what aborts live play of
those tracks.

Done means every flag value the reachable tracks' riders actually touch is recovered from the
original's code path and matched, update for update, against the original captures. A flag no
capture reaches stays guarded and is listed. Out of scope: HYBRID's `inverted AI marker is
unrecovered` guard, INFINITY's checkpoint guard, PINGPONG's `opponent.response_b`, the locked
tours and the stunt events. Each has its own row in R-0046's "Next experiments".

## Inputs and prerequisites

- ROM (SHA-256 `a1105819d48c04d6...`, path in `local/rom-location.txt`). The pinned bsnes core
  is `local/emulators/bsnes/bsnes/out/bsnes_libretro.dylib`; the v10 pack is
  `local/classic-pal-crawler-tracks-v10.pack`. A new worktree copies both and links
  `local/emulators` to the main checkout's (`bootstrap` otherwise starts a partial core build
  there).
- The part 2 sweep keeps its full captures, with memory, for all 20 cold-start tracks under
  `local/evidence/track-breadth/track-breadth-2/sweep/row<R>-pos<P>/`, together with
  `sweep.json`.
- `python3 -m tools.unirally_lab.native.track_reference recompare --per-track --sweep <that dir>
  --binary build/lab-debug/src/core/zoom_zoo_runner --pack local/classic-pal-crawler-tracks-v10.pack
  --out <json>` re-measures every compared race. `capture ... --hold FRAME BUTTON` adds a held
  input, and `explore` honours it.
- The stops, as rows from the original's boundary with the controller released:

  | Track | Name | Stop (rows exact before it) |
  | ---: | --- | ---: |
  | 3 | SWITCHER | 384 |
  | 4 | MONSTER | 361 |
  | 11 | MEGAJUMP | 531 |
  | 20 | DRAGRACE | 1,353 |
  | 24 | SHORT CUT | 1,483 |
  | 34 | HAIRPIN HILL | 302 |
  | 31 | CROCK | 1,731 (after its exact window; needs a longer capture) |
  | 30 | WARIO PAINT | 1,719 (likewise) |

  The flag values present in these tracks' tile tables, but outside the recovered set, are 1,
  4, 8, 10, 12, 14, 16, 25, 27 and 28 (R-0046 observation 4). Which ones the riders actually
  touch is not yet measured.
- The recovered response code lives around `$81:924E-9358` (see the `$81:` citations in
  `vertical_contact.cpp`, R-0024 and R-0025). Read the listing first:
  `python3 tools/project.py coverage disassemble --out artifacts/static-map --coverage ...`, with
  the four raw captures listed in STATIC-CODE-MAP's handoff.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Flags named | At each stop, the tile flag and the contact summary that reach the guard, from native's state and the original's WRAM | A list of the flag values touched, per track | R-0047 |
| Branch recovered | Listing read, then a watch capture on the original at the stop update | The response for each touched flag, cited by address | R-0047 |
| Match | `track_reference recompare --per-track` on the part 2 sweep; longer held-input captures for CROCK and WARIO PAINT | Each listed track exact past its old stop, to its window's end or to a named next guard | recompare JSON |
| Nothing accepted moves | The eleven differential gates, the v1 contracts, hidden runs, fuzz, ctest, the synthetic suite | All pass with unchanged row digests and restore counts | gate logs |
| Live | Hidden 4,000-update held-input runs on WARIO PAINT and CROCK | No special-tile stop | run logs |
| Review | Tier 1 | Approved, with the withheld stop re-compared by the reviewer | review on the pull request |

## Capability and coverage checkpoint

- Native capability delivered / still missing: the lift (flag pair 24), mud (14), corkscrew (10)
  and jump-driven tile (16), in contact and movement, with every consumer they reach, the
  cross-rider `$0EA3` write, the rotation's surface-mode return and the corkscrew's sprite
  priority. Still missing: pairs 4, 8, 12, 26 and 28 (guarded, never touched by a capture) and
  the sounds (declared omission).
- Frozen exact-match interval, field set and reference/seed identity: the part 2 sweep
  captures; no new freeze unless the task adds one.
- Dynamic captured inputs still consumed (must be zero for autonomy): the landing matrices, as
  before.
- Relevant branches/transitions exercised, including independent variations: every stop of the
  part 2 sweep; CROCK and WARIO PAINT past their stops (3,004 and 3,011 updates); the player on
  mud (SWITCHER), ejected from a corkscrew (SHORT CUT) and through a whole corkscrew
  (MEGAJUMP), each with Right held; 16 pictures through the player's corkscrew.
- First divergence and cheapest next discriminating experiment: see the handoff.
- Trial-wide usage baseline/current, reserve, reset authorization/outcome or none: weekly
  all-models 0% at claim (08:25Z, just after the reset), see the handoff for the close; no reset
  authorization needed.

## Evidence and attempts

| Attempt | Hypothesis | Experiment | Observation | Next decision |
| --- | --- | --- | --- | --- |
| 1 (08:28Z) | The guard names one flag per track | Temporary print in the guard, native on the part 2 captures | All nine stops are the opponent; flags 10, 14, 25 | Read the listing at `$81:9185` |
| 2 (08:30Z) | The contact response branches on the flag | Listing `$81:9185-91D1` | Dispatch on `flag & $FE`: 2, 8/16, 24, 26; 10 and 14 take no branch | Recover pair 24's contact part; narrow the guard |
| 3 (08:33Z) | Narrowing the guard is enough | recompare | Every track diverges one update past its old stop, in `tile_mode`, `launch_override`, `tile_pose` | A second dispatch |
| 4 (08:35Z) | The movement phase has its own dispatch | Listing: table at `$81:82F5`, reset `$81:858E`, scratch-to-persistent copies in `$82` | Handlers for every pair; native handled 2 and 6 and ignored the rest | Implement 24, then 14 and 10 |
| 5 (08:38Z) | Pair 24 lifts the rider | Handler read, checked on HAIRPIN HILL's WRAM, implemented | HAIRPIN HILL exact 302 to 531 (checkpoint guard) | Pairs 10 and 14 need new state |
| 6 (08:44Z) | The consumers of the new words are the reads the listing shows | Every read of `$0F45`, `$0F49`, `$0547`, `$0F4B`, `$0F3B`, `$0F3F`, `$0F2F`, `$0DFB`, `$0E7B` read; state words in URTRnn02; pack v11 for the heights | SWITCHER, DRAGRACE, SHORT CUT exact to their window ends; MONSTER and MEGAJUMP about 50 further, then `player.rolling` | The `$0EA3` write |
| 7 (08:45Z) | `$0EA3` is set on every corkscrew update | Original WRAM, MONSTER 358-414 | Only on steps: the flag ends 0 at step `$30` | Set it only when the step path ran |
| 8 (08:46Z) | - | recompare | MONSTER and MEGAJUMP differ on `opponent.response_b` after the corkscrew, like PINGPONG | Read `$82:A49F` |
| 9 (08:47Z) | Native's rotation clear is out of order | Listing: surface mode and leading support return first | PINGPONG, MEGAJUMP exact to their ends; MONSTER to HYBRID's guard | Longer captures |
| 10 (08:50Z) | - | Captures: CROCK and WARIO PAINT to 4,400; SWITCHER and SHORT CUT held Right | CROCK exact 3,004; SWITCHER and SHORT CUT exact; WARIO PAINT stops at pair 16 at 2,184 | Recover pair 16 |
| 11 (08:53Z) | `$1349` changes pair 16's contact | Listing `$81:9610-9690` and the static map's classes | Read only on a path never entered (inferred, never observed) | Pair 16 in movement only |
| 12 (08:55Z) | - | WARIO PAINT again | Exact 3,011 of 3,011 | Pictures |
| 13 (09:02Z) | Bit 4 of `$1516`/`$151A` is the priority | Native search for a held run through a corkscrew; MEGAJUMP captured with pictures | Exact 2,033 rows; 16 pictures 0 pixels; without the bit 6 of 8 differ by 3-160 | Gates |

## Handoff

- Current base/head commit and uncommitted state: base `4c45eb1` (`main` after #15);
  implementation `f1501d6` and `0cfdf0c`, review fix `76ae927`, records after it, on
  `task/special-tile-response`; merged by #16.
- Verified findings: [R-0047](../docs/research/R-0047-special-tiles.md). The two dispatches
  by flag pair; the lift, mud, corkscrew and jump-driven tile with their consumers; the
  opponent's corkscrew writing the player's rolling flag; the rotation's surface-mode return
  (also PINGPONG's divergence); the corkscrew's sprite priority.
- Current hypothesis and failed approaches: narrowing the contact guard alone (attempt 3) and
  setting `$0EA3` on every corkscrew update (attempt 7) both failed against the captures.
- Commands executed, outcomes and report hashes:
  - `track_reference recompare --per-track` on the part 2 sweep, before and after:
    `local/evidence/special-tile-response/recompare/`.
  - Captures (`captures.sh`, plus `short-cut-held-right-pictures` and `megajump-held-right`)
    and their `explore` reports under `local/evidence/special-tile-response/`.
  - `gates.sh` on `0cfdf0c` (09:08-09:53Z, `gates-0cfdf0c.out`, logs in `gates-0cfdf0c/`):
    ctest 24/24 on lab-debug, lab-release and app-debug; synthetic suite passed; hidden
    4,000-update held-input runs on DRAGSTER, ZOOM ZOO, FLAT FUN, WARIO PAINT, CROCK, MEGAJUMP
    and SHORT CUT, 0 fallback frames; fuzz 40 seeds, 79 races, 0 aborts; **all eleven
    differential gates passed with the same row digests and restore counts as TRACK-BREADTH's
    final set** (`diff` of the two gate listings is empty); `rnc-inventory --expect` passed.
    The v1 winner and loser contracts ran separately (the script's gate directory was outside
    `artifacts/`, which `presentation-check` refuses; fixed in the script) and passed:
    `v1-0cfdf0c/`.
  - Hidden held-input runs on SWITCHER, DRAGRACE and PINGPONG also 0 fallback frames
    (`hidden-special/`); MONSTER and HAIRPIN HILL stop at their out-of-scope guards.
  - `coverage static-map` twice, identical: 709 cited addresses (42 new), unknown share
    unchanged at 40.4%.
  - After the review fix, `gates.sh` on `76ae927` (12:44-13:30Z, `gates-76ae927.out`): the
    same results in full, the v1 contracts included, and the eleven gates again identical to
    TRACK-BREADTH's. `content track-idle-matrix --updates 1200`: 39 of 45 complete (R-0047).
- Unavailable/skipped checks: `lab-sanitize` and `app-sanitize`, unavailable on this host (a
  one-line ASan program still hangs before `main`, rechecked 24 September on macOS 27.0
  26A428); the hosted Linux job covers them.
- Exact next experiment/command: none for this task; [RACE-GUARDS](RACE-GUARDS.md) is next.
- Remaining dependencies: none.
- Runtime needs (network, build time, fixtures, memory): ROM, core, pack v11; the gates take
  about 45 minutes.
- Aggregate parent/child time, provider usage before/after (or unknown), other-account-work
  caveat: primary 08:25-10:00Z and 12:40-13:45Z; reviewer 09:55-10:17Z and 13:30-13:35Z (22 and
  5 minutes by the harness). Weekly all-models 0% at claim, 3% at 13:30Z; final figures in
  the closeout.
- Accepted outcome, review/fix rounds and next routing decision: accepted after one fix
  round; next is [RACE-GUARDS](RACE-GUARDS.md).
- Cleanup by the closing session: the captures, gate logs, recompare reports and pictures
  are already under `local/evidence/special-tile-response/` in the main checkout (the v1 and
  gate directories under the worktree's `artifacts/` are moved there too); copy pack v11 to
  the main checkout's `local/`; delete both worktrees (`special-tile-response`,
  `special-tile-response-review`) with their build output, and the local task branch once
  `main` holds it; the closeout lists what was moved and deleted.

## Review and integration

- Reviewer and independent reproduction/withheld-case results: a fresh Claude Opus 5.5
  subagent in the detached checkout `.worktrees/special-tile-response-review`, tier 1. At
  `0c94e8d` it extracted pack v11 itself, reproduced the recompare on all 20 tracks and native
  on the six captures, and made four withheld captures: WARIO PAINT with Right held and with
  Right and jump held, CROCK and DRAGRACE with Right held. All four were exact to their ends
  (3,011, 3,011, 3,004 and 2,605 rows). It also read every handler against the code, checked
  that `$81:966F` is unreachable, and reran two gates (same digests). **Approved** with one
  should-fix and four advisories
  ([review](https://github.com/malmazuke/unirally-reconstruction/pull/16#pullrequestreview-5303028744)).
  At `76ae927` it checked the serializer (including 66 restored live-tile WARIO PAINT states
  continuing byte-identically in fresh processes), the idle matrix (39 of 45) and one gate:
  **approved**, one records advisory
  ([re-review](https://github.com/malmazuke/unirally-reconstruction/pull/16#pullrequestreview-5305169261)).
- Required changes or acceptance rationale: the should-fix (a live special tile could not be
  saved on DRAGSTER or ZOOM ZOO, whose own table holds the corkscrew) is fixed in `76ae927`
  by the live-only extension, with the full gates rerun. Advisories 2, 3 and 5 are fixed in
  the same commit; 4 (pair 28 aborts) is kept as a named guard, since no capture reaches it.
  The re-review's advisory (three stale layout descriptions) is fixed in the final records
  commit: comments and prose only, so it needs no re-review.
- Exact merge candidate and required-check results: the pull request head; `changes`, `lab
  (ubuntu-24.04)` and `lab (macos-15)` green on it before merging (run IDs in the closeout).
- Integrated commit and evidence location: the merge commit of
  [#16](https://github.com/malmazuke/unirally-reconstruction/pull/16);
  `artifacts/special-tile-response-integration/closeout.json` and
  `local/evidence/special-tile-response/` in the main checkout.
- Remote synchronization: through the pull request; local `main` fast-forwarded and compared
  with `origin/main` after the merge (closeout).
- Scope still unverified: tile pairs 4, 8, 12, 26 and 28; the corkscrew ejection beyond one
  case; the tiles' sounds; new-track finishes and results (outside this task).
