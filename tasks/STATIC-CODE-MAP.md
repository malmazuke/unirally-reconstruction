# STATIC-CODE-MAP - a static, annotated code map of the code banks

## Assignment

- Status: reviewed (approve with should-fix items, all applied) and integrated (claimed 22 September 2026 23:41Z on the user's explicit
  override of the reset boundary: "keep going until either the task is complete, or the limit
  is reached"; prepared under [D-0008](../docs/decisions/D-0008-static-map-track-breadth-review-tiers.md))
- Milestone: M1 extension that enables M4 breadth; not an M4 acceptance gate
- Coordinator: the preparing session (Claude Fable 5.1, Claude Code desktop, 22 September
  2026 UTC); the claiming session is coordinator, primary and integrator once it claims this record
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: Claude Opus 5.5 (`claude-opus-5-5`), Claude Code desktop, one
  session as coordinator, primary and integrator; the review subagent is recorded below
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  the claiming session's model at default effort. Review tier under D-0008: **tier 2**
  (tooling). No frontier escalation question is open.
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): at preparation, weekly all-models **84%** at 2026-09-22T11:20Z (five-hour
  38%, per-model Fable 56%; weekly resets 2026-09-24T08:00Z). Above the 80% floor, which is
  why this is prepared and not started. **At claim: weekly all-models 89% (2026-09-22T23:40Z;
  five-hour 0%, Fable 65%), already above the 80% floor; the user lifted the boundary for this
  task until it completes or the weekly limit is reached.** Mid-implementation sample: 89%.
- Reviewer (primary automatically spawns fresh model/effort, isolated checkout; no user
  trigger): one round by a fresh Anthropic subagent in a separate checkout at the candidate,
  against the reviewer checklist; it must regenerate the listing from its own ROM path and
  compare the tracked outputs byte for byte, and walk at least three inferred (not observed)
  routines by decoded length against the ROM as R-0006's review did.
- Dependencies and evidence of acceptance: the four tracked maps under `docs/map/` and
  R-0006 (the opcode length table `tools/unirally_lab/coverage/opcodes.py`, the LoROM rule,
  map schema 1); the cited-address inventory below. Nothing else.
- Base commit: the `main` tip at claim; `ee5c132` or later.
- Branch and isolated worktree: `task/static-code-map` in `.worktrees/static-code-map`.
- Owned paths and shared interfaces: a new `tools/unirally_lab/coverage/static_map.py` (or a
  sibling module), `tools/unirally_lab/coverage/commands.py` for the two subcommands,
  `tests/tooling/test_coverage.py` or a new `test_static_map.py`, `docs/map/static/`,
  `docs/research/R-0045-static-code-map.md`, the command inventory in
  `docs/BUILD_AND_VALIDATION.md`, this record, `docs/STATE.md`, `tasks/README.md`,
  `tasks/NEXT_SESSION.md`. No game code, pack rules or gates change.
- Claim/lease/heartbeat/checkpoint location: this record's Evidence and attempts table;
  local artifacts under `artifacts/static-map/` in the worktree, moved to
  `local/evidence/static-code-map/` at integration.
- Session time limit, concurrency allocation and actual spend authorization if relevant: one
  active worker plus the review subagent; 45-minute reassessment intervals; no monetary spend.

## Outcome and boundaries

Two tracked commands and their outputs:

1. `python3 tools/project.py coverage disassemble --out artifacts/static-map/` reads the
   exact-gated ROM and the union of the tracked coverage maps and writes an **ignored**
   listing of banks `$80`-`$83` (one file per bank, address, bytes, mnemonic, operand,
   mode, class, label, cross-references) plus a `static-map.json` detail file. Seeds: every
   site of every tracked map with its recorded mode; every entry point and vector; every
   static reference into ROM (call/jump targets as code, other absolute and long operands as
   data). Recursive descent from the seeds under the recorded modes; then a gap sweep that
   propagates M/X from REP/SEP along fall-through and branch edges and marks a mode
   ambiguity instead of guessing. Every byte ends in exactly one class: `observed`
   (executed in a tracked map), `inferred` (decoded statically), `data` (referenced by an
   operand or a table walk and never decoded as code), `unknown`.
2. `python3 tools/project.py coverage static-map --out docs/map/static/code-banks.map.json --summary docs/map/static/code-banks.md`
   derives the **tracked** map: per-bank class totals; a routine table (start, end, entry
   kinds and counts, callers, observed or inferred, mode at entry, label, cited-by records);
   and `docs/map/static/labels.json` (address, label, comment, source record), seeded from
   the addresses the research and task records already cite. Tracked outputs carry **no ROM
   bytes, opcode bytes or mnemonics**, as map schema 1 already requires; the existing test
   that enforces this is extended to the new files.

Boundaries: no semantic recovery of any routine beyond its label; no data decoding beyond
classification; no change to game code, packs, gates or the coverage capture. Data banks
`$84`-`$BF` are out of scope except as reference targets. The listing is a reading aid and a
hypothesis generator (D-0008 rule change); it is not gameplay evidence.

## Inputs and prerequisites

- ROM SHA-256 `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e` at the path
  in `local/rom-location.txt`; the exact-identity gate from `project.py rom inspect`.
- The four tracked maps (`docs/map/*.map.json`); their union is 41,778 executed bytes in
  587 maximal runs, 2,385 entry points in the largest capture, and 34 bytes in bank `$00`
  (the vectors' trampolines) plus `$80` 8,360, `$81` 13,202, `$82` 13,109, `$83` 7,073.
- Cited-address inventory, regenerated at claim: the regular expression
  `\$(8[0-3]|0[0-3]):[0-9A-Fa-f]{4}` over `docs/research/*.md`, `tasks/*.md`,
  `docs/inventory/*.md` and `docs/content/*.md` gave 644 distinct addresses on 22 September
  2026, 545 of them inside observed code, and explicit ranges (`$82:A627--A6F7` style)
  covering 30,160 bytes. These seed `labels.json` with the record that cites them.
- No emulator run is needed. A fresh host needs the ROM and Python only.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Agrees with every observation | `coverage disassemble` then a check that every site of every tracked map decodes at the same address with the same length under the recorded mode | 0 disagreements; report lists count checked | `artifacts/static-map/agreement.json` |
| Partition | Class counts per bank | Sum is 131,072; no byte both opcode and operand; no code run crosses into a data-classified byte without a recorded boundary | `docs/map/static/code-banks.md` totals |
| Cited addresses land | Every address in the cited inventory | Each is an instruction start, inside an instruction (listed as such), or data; any in `unknown` are listed by record | `docs/map/static/labels.json`, summary section |
| Deterministic | Two fresh regenerations of the listing and the tracked map | Byte-identical files; digests recorded | summary, task handoff |
| ROM-free tests | `python3 tools/project.py test --suite synthetic` | New checks on a synthetic image: descent from seeds, REP/SEP mode propagation, ambiguity marking, gap sweep, data classification from operands, the no-bytes/no-mnemonics rule on the tracked outputs; suite count grows and is recorded | test report |
| Tracked outputs are clean | Existing "no opcode or mnemonic keys" test extended to `docs/map/static/` | Pass | test report |
| Measured breadth | The summary's class totals | Observed, inferred, data and unknown bytes per bank; the unknown share is reported, not hidden, and copied to `docs/STATE.md` | summary, STATE |
| Review | One independent round (tier 2) | Regenerated on the reviewer's ROM path; three inferred routines walked by decoded length; approve or a specific reproducible failure | review record |

The D-0008 revisit trigger applies: if more than a quarter of the code banks stay `unknown`
after the gap sweep, record it and stop extending heuristics; the unknown regions become
targets for dynamic capture.

## Capability and coverage checkpoint

- Native capability delivered / still missing: none delivered; this is tooling. Missing: unchanged.
- Frozen exact-match interval, field set and reference/seed identity: not applicable.
- Dynamic captured inputs still consumed (must be zero for autonomy): not applicable.
- Relevant branches/transitions exercised, including independent variations: not applicable.
- First divergence and cheapest next discriminating experiment: not applicable.
- Trial-wide usage baseline/current, reserve, reset authorization/outcome or none: 84% at
  preparation; to be sampled at claim; no reset or purchase authorized.

## Evidence and attempts

| Attempt | Hypothesis | Experiment | Observation | Next decision |
| --- | --- | --- | --- | --- |
| 0 (preparation, 22 September 2026) | The code lives in the first four banks | Per-bank entropy and JSR/RTS/JSL/RTL byte density over the ROM | `$80`-`$83` profile as code (26-80 such bytes per KiB); every other bank profiles as data, including `$97`, whose apparent density is the space character in text tables | Scope the map to `$80`-`$83` |
| 1 | Tracked ranges plus REP/SEP propagation fix the observed boundaries | Tile each range with its instruction count; check against the four raw captures (`check_sites`) | 50 of 2,020 ranges ambiguous or untileable; about 800 sites per race capture undecoded; one wrong length at `$81:9FCA` from a unique but propagated tiling | Tile without propagation, keep only shared instructions (R-0045 obs. 3) |
| 2 | Shared tilings suffice | Same check | 0 wrong lengths, but 462 ranges have several tilings and 6,179 instructions stay unresolved (2,051 undecoded sites on the ZOOM ZOO capture) | Take observed boundaries from the raw sites behind the maps (digest-matched); keep the tiling as the cross-check (R-0045 obs. 2) |
| 3 | Descent plus a plausibility gap sweep | Listing inspection | 0 disagreements over 53,062 sites; but the gap sweep and descent from it decode tables as code (`$80:8000` bit table, `$83:8535` pointer table) | Transactional descent that rejects a whole routine on an implausible instruction; gap-sweep results become unclassified candidates (R-0045 obs. 4) |
| 4 | Final | `coverage disassemble` and `coverage static-map`, twice each | observed 41,778, inferred 35,134, data 61, unknown 54,099 (41.3%); 0 disagreements; byte-identical regenerations | D-0008 trigger met (unknown above a quarter): recorded, no more heuristics; review |
| 5 (review S1) | A `$0000` first entry is a table placeholder | Walk `$81:82F5` past it | 15 entries walked; inferred 36,321, data 92, unknown 52,881 (**40.4%**) | Apply; the conclusion is unchanged |

## Handoff

- Current base/head commit and uncommitted state: base `bdfd82e`; implementation on
  `task/static-code-map` (see Review and integration for the candidate).
- Verified findings: [R-0045](../docs/research/R-0045-static-code-map.md). Every one of the
  53,062 recorded sites decodes at its recorded length (0 disagreements); the 42,700
  instructions shared by every tiling of the 2,020 ranges all match sites. Classes: observed
  41,778, inferred 36,321, data 92, unknown 52,881 (**40.4%**, above D-0008's trigger; after
  review S1).
  Of 649 cited addresses (at integration): 112 routine starts, 483 instruction starts, 17
  inside an instruction, 9 data, 28 unknown (listed in the summary).
- Deviation from this record, with reason: observed boundaries come from the raw coverage
  files behind the tracked maps (`--coverage`, matched by digest), because the tracked maps
  alone leave 6,179 instructions unresolved; without them the tools fall back to shared
  tilings (48.4% unknown) and report the per-site check as skipped. Gap-sweep decodings are
  candidates that stay `unknown`, because the sweep accepted tables.
- Failed approaches: REP/SEP propagation inside observed ranges (excluded the true tiling at
  `$81:9FCA`); a classifying gap sweep (decoded pointer and bit tables as code).
- Commands executed (raw coverage: `local/evidence/m1-01/m1-01/cap1`, `.../race-cap1`,
  `local/evidence/m3-00-finish-evidence/m3-00-coverage/capture`,
  `local/evidence/m4-02-second-track/m4-02/coverage/capture-1`, each `coverage.json`):
  `coverage disassemble --out artifacts/static-map --rom <ROM> --coverage ...` twice (second
  into a scratch directory, `cmp` identical, then deleted), and `coverage static-map` twice,
  all `status=passed`, about 17 s each. Final tracked digests (two regenerations identical,
  after the records were final, since `labels.json` reads the records' citations):
  `code-banks.map.json` `bb35bb0b9a68f1b1...`, `code-banks.md` `91b237f1dc5e737a...`,
  `labels.json` `55312c4326e596af...`. Listing digests in
  `artifacts/static-map/listing-digests.txt`.
  `python3 tools/project.py test --suite synthetic` (after `bootstrap` and
  `build --preset lab-debug` in the new worktree): `status=passed`, 444 Python tooling tests
  (433 before plus the 11 in `test_static_map.py`), 0 failed, every ctest passed.
- Unavailable/skipped checks: none of the acceptance checks. Sanitizer presets are not part of
  this tooling-only change.
- Exact next experiment/command: the unknown regions are dynamic-capture targets; the
  cheapest resolution of the largest block would be the data bank at each indexed or
  absolute read, which the access captures already record (not used here).
- Remaining dependencies: none.
- Runtime needs (network, build time, fixtures, memory): the ROM and Python; no build.
- Aggregate parent/child time, provider usage before/after (or unknown), other-account-work caveat: weekly all-models 89% at claim and 89% after the review and fixes (2026-09-23T00:12Z,
  five-hour 5%); the task fit inside the one weekly point the displayed figure resolves. Wall
  clock from claim (23:41Z) to integration about an hour and a half, the review subagent about 12
  minutes of it.
- Accepted outcome, review/fix rounds and next routing decision: to be recorded.

## Review and integration

- Reviewer and independent reproduction/withheld-case results: one fresh Claude Opus 5.5
  subagent (D-0008 tier 2, one round) in `.worktrees/review-static-code-map` at the exact
  candidate `d287845`, about 12 minutes: **approve with should-fix items** at `a17e77a`
  (report [STATIC-CODE-MAP-review](STATIC-CODE-MAP-review.md) on `review/static-code-map`,
  pushed to `origin`). It checked that the four raw coverage files' digests equal the maps',
  regenerated the tracked files on its own ROM path (byte-identical: `bb35bb0b...`,
  `91b237f1...`, `55312c43...`), ran `disassemble`, and walked four inferred routines against
  the ROM with its own decoder (`$80:98B3`, `$81:D936`, `$82:AEF5`, `$83:D581`: boundaries and
  REP/SEP modes match, all read as code). Its own script over all four raw files gave 53,062
  sites, 17,569 distinct, none overlapping and none with two lengths, equal to the observed
  instructions and 41,778 bytes.
- Required changes or acceptance rationale: no blocking finding. S1, a correctness bug: a
  `$0000` first entry ended the jump-table walk, which left the `$81:82F5` table unwalked. Fixed
  (a placeholder at index 0 only, since a rule for any zero entry ran on through zero filler in
  the new test) and tested (attempt 5). S2 (the per-site check largely re-reads its own input),
  S4 (a fresh host needs the raw captures), S5 (80 of 682 observed calls return in another mode),
  S6 (first-path modes), S7 (a stale docstring), S9 (paths leaving ROM end silently) and S10
  (ranges label only their start): the records and the docstring now say so. S3 (`labels.json`
  follows the records): recorded in R-0045, and the tracked files are regenerated last. S8:
  tests added for the jump-table walk and the cited-address scan. The remaining nits about test
  breadth are left as they are.
- Exact merge candidate and required-check results: the tip of `task/static-code-map` after
  the should-fix commit, containing the reviewed `d287845`. On it: `test --suite synthetic`
  `status=passed` (446 Python tooling tests, 0 failed; every ctest passed), and two identical
  regenerations of the tracked files (`code-banks.map.json` `e7576089356dbba5...`,
  `code-banks.md` `46469f1ff1403e4b...`, `labels.json` `347508273ffb19e2...`). Hosted CI on `d287845`: run
  35799810644, success.
- Integrated commit and evidence location: by fast-forward of `main` from `bdfd82e`; the ignored
  listing, agreement, reports and logs at `local/evidence/static-code-map/static-map/`, the closeout
  at `artifacts/static-code-map-integration/closeout.json` in the main checkout.
- Remote synchronization: pushed ref(s), verified local/remote commit IDs, or exact push failure:
  `refs/heads/main` and `refs/heads/task/static-code-map` at `1af38f05e471a4495b575de2554c9e2edda17855`,
  `refs/heads/review/static-code-map` at `a17e77a`, each verified equal to local before the local
  branch was deleted. Final-tip CI on `main` at `1af38f0`: run 35801187607, success (candidate
  run 35799810644 on `d287845`, success).
- Cleanup: moved the worktree's `artifacts/static-map/` (listing, agreement, run reports, suite and
  build logs, digests) to `local/evidence/static-code-map/static-map/`; deleted
  `.worktrees/static-code-map` with its `build/` (54 MB), its bootstrapped `local/` toolchain
  (167 MB) and the synthetic suite's unreferenced scratch directories, deleted
  `.worktrees/review-static-code-map`, and deleted both local branches.
- Scope still unverified: every `inferred` routine is a static reading that no capture has
  executed. The unknown 40.4% is not resolved. The data bank at indexed and absolute reads,
  which the access captures record, is the next discriminating input.
