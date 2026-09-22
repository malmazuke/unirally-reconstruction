# STATIC-CODE-MAP - a static, annotated code map of the code banks

## Assignment

- Status: ready (prepared 22 September 2026 UTC under [D-0008](../docs/decisions/D-0008-static-map-track-breadth-review-tiers.md);
  claim after the weekly reset on 2026-09-24T08:00Z or on an explicit user override)
- Milestone: M1 extension that enables M4 breadth; not an M4 acceptance gate
- Coordinator: the preparing session (Claude Fable 5.1, Claude Code desktop, 22 September
  2026 UTC); the claiming session is coordinator, primary and integrator once it claims this record
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: to be recorded at claim
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  the claiming session's model at default effort. Review tier under D-0008: **tier 2**
  (tooling). No frontier escalation question is open.
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): at preparation, weekly all-models **84%** at 2026-09-22T11:20Z (five-hour
  38%, per-model Fable 56%; weekly resets 2026-09-24T08:00Z). Above the 80% floor, which is
  why this is prepared and not started. Sample fresh usage at claim and record it here; the
  discretionary allowance is 20 points from that baseline and the floor is 80%.
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

## Handoff

- Current base/head commit and uncommitted state: not claimed.
- Verified findings: the preparation measurements in D-0008.
- Current hypothesis and failed approaches: none yet.
- Commands executed, outcomes and report hashes: none yet.
- Unavailable/skipped checks: none yet.
- Exact next experiment/command: at claim, sample usage, then implement the seed loader
  and run descent from the observed sites alone; the first report is the agreement check,
  which must be 0 disagreements before any inference is added.
- Remaining dependencies: none.
- Runtime needs (network, build time, fixtures, memory): the ROM and Python; no build.
- Aggregate parent/child time, provider usage before/after (or unknown), other-account-work caveat: to be recorded.
- Accepted outcome, review/fix rounds and next routing decision: to be recorded.

## Review and integration

- Reviewer and independent reproduction/withheld-case results:
- Required changes or acceptance rationale:
- Exact merge candidate and required-check results:
- Integrated commit and evidence location:
- Remote synchronization: pushed ref(s), verified local/remote commit IDs, or exact push failure:
- Scope still unverified:
