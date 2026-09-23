# SPECIAL-TILE-RESPONSE - the vertical contact response on the tile flags no accepted track reached

## Assignment

- Status: ready (prepared 23 September 2026 by the TRACK-BREADTH session). Claim after the
  weekly reset on 2026-09-24T08:00Z, or on an explicit user override; weekly all-models usage
  was 96% when this record was written.
- Milestone: M4 breadth (the follow-up TRACK-BREADTH's matrix names first)
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: to be recorded at claim
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  the claiming session's model at default effort. Review tier under D-0008: **tier 1**
  (simulation state and arithmetic in `src/core/vertical_contact.cpp`), so the full D-0006
  process: a fresh independent reviewer in an isolated checkout with withheld cases, returned
  rounds until approval, and all eleven differential gates actually run (the change reaches the
  gate binary, so `gate_identity` cannot cite them).
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): sample at claim and record here.
- Reviewer (primary automatically spawns fresh model/effort, isolated checkout; no user
  trigger): a fresh Anthropic subagent at the exact candidate. Withheld case: a stop the primary
  did not use as evidence (see the table below), re-compared by the reviewer.
- Dependencies and evidence of acceptance: TRACK-BREADTH (accepted); R-0046 (the matrix and
  observations 1-18); R-0011, R-0024 and R-0025 (the recovered vertical contact); the v10 pack.
- Base commit: the `main` tip at claim.
- Branch and isolated worktree: `task/special-tile-response` in `.worktrees/special-tile-response`.
- Owned paths and shared interfaces: `src/core/vertical_contact.cpp`/`.hpp` and whatever the
  response reaches in `src/core/movement.cpp`; native tests under `tests/native/`; a new
  research record `docs/research/R-0047-special-tile-response.md`; this record,
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

- Native capability delivered / still missing: to be filled.
- Frozen exact-match interval, field set and reference/seed identity: the part 2 sweep
  captures; no new freeze unless the task adds one.
- Dynamic captured inputs still consumed (must be zero for autonomy): the landing matrices, as
  before.
- Relevant branches/transitions exercised, including independent variations: to be filled.
- First divergence and cheapest next discriminating experiment: see the handoff.
- Trial-wide usage baseline/current, reserve, reset authorization/outcome or none: to be
  sampled at claim.

## Evidence and attempts

| Attempt | Hypothesis | Experiment | Observation | Next decision |
| --- | --- | --- | --- | --- |

## Handoff

- Current base/head commit and uncommitted state: not claimed; prepared on `main` after PR #14.
- Verified findings: the stops in the table above (R-0046 observation 9, part 3 recompare).
- Current hypothesis and failed approaches: none yet.
- Commands executed, outcomes and report hashes: none yet.
- Unavailable/skipped checks: `lab-sanitize` and `app-sanitize` are unavailable on this host
  (the ASan runtime hangs before `main` since the macOS 27 update); the hosted Linux job covers
  them.
- Exact next experiment/command: native first, since it is cheap. Run
  `build/lab-debug/src/core/zoom_zoo_runner --start classic.track.03 --content-pack
  local/classic-pal-crawler-tracks-v10.pack --inputs <released controller from frame 1419>`
  for 400 updates, and read the contact summary (`summary.tile_flags` and the probe words) on
  the update that throws. A temporary `std::cerr` of the flag in the guard is enough; do not
  commit it. Then read the original's WRAM at the same row of
  `local/evidence/track-breadth/track-breadth-2/sweep/row0-pos3`, and the listing from
  `$81:924E`, to see what the original does with that flag.
- Remaining dependencies: none outside the project.
- Runtime needs (network, build time, fixtures, memory): ROM, core, v10 pack, a lab-debug build
  (seconds), about 40 minutes for the full gate script (the part 3 script is at
  `local/evidence/track-breadth/track-breadth-3/gates.sh`; update its worktree path).
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
