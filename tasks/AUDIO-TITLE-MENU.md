# AUDIO-TITLE-MENU - audible native title and menu audio

## Assignment

- Status: in_progress, claimed 30 September 2026 UTC after PR #49 verification.
- Milestone: M4 (original game coverage).
- Coordinator/primary: `/root`, OpenAI, current Codex session; model identifier
  is not exposed by this runtime. D-0004 Sol/medium project default remains
  recorded; explicit consultation/reviewer settings are named below.
- Tier: 1. Native audio state, clock/order semantics, content extraction and
  new differential evidence require the full D-0006 review process.
- Quota: weekly used 12% at claim; discretionary stop 32% used and final
  20% reserved for review/recovery. Reset epoch 1791365217. Startup telemetry
  is in main `artifacts/audio-title-menu-integration/startup.json`. No reset,
  spending or provider change authorized.
- Reviewer: automatically spawn fresh explicit Sol/medium, no inherited context,
  isolated checkout at the immutable candidate; post report on the PR. Handle
  findings and re-review before integration without a user trigger.
- Base: clean synchronized main `7a958cd6621c213c680d1ffe62fcde964e2b0295`;
  PR #49 is MERGED and its closeout agrees.
- Dependencies: D-0009, R-0074; accepted native power-on/title/main-menu path,
  pack v29 and the pinned PAL ROM/reference core.
- Branch/worktree: `codex/audio-title-menu`, `.worktrees/audio-title-menu`.
- Owned scope: sound observation tools/patch lock, native audio core and tests,
  audio extraction with evidence, SDL output, affected pack/profile interfaces,
  task/research/validation/state records and regenerated static/native maps.
- Main evidence home: `local/evidence/audio-title-menu/`; closeout/logs:
  `artifacts/audio-title-menu-integration/`. No evidence in a worktree or input
  paths into another worker's checkout.
- Checkpoint: this record every ten minutes and before expensive captures;
  45-minute reassessments, primary plus one active child, no spending.

## Outcome and boundaries

From native power-on, play the title/menu music and one dynamically identified
navigation effect, with music/effect overlap, retrigger and scene stop/restart.
Use only locally extracted identified data and native C++ driver/sequencer logic,
feeding a DSP-only hardware model and SDL output as D-0009 specifies. No game
CPU interpreter, original uploaded executable, prerecorded soundtrack or captured
DSP-event playback may satisfy product acceptance.

Capture tooling, code/data separation and driver recovery are coupled experiments
inside this outcome. They are not separate accepted capability substitutes.
The initial diagnostic schedule is R-0074's 700-frame cold Start path, with Down
at 450-455 as a variation. Find one complete recovered loop or a bounded
non-looping cue before freezing the final exact domain. The rest of the
soundtrack, race/ending audio and alternative regions/rates remain later work.

## Inputs and prerequisites

- ROM/core/patch identities and three retained diagnostic schedules: R-0074.
- Existing tools: `reference verify`, `access capture`, `coverage disassemble`,
  `coverage static-map`, the native frontend/runners and pack tooling.
- Not implemented: raw DSP PCM, ordered DSP writes, APU RAM/upload observation,
  native sequencer, hardware-model adapter and audio device output. Implement
  and independently validate these inside this task; do not advertise commands
  before they run.
- Read the static listing before capture design. `$82:8000-8336` covers CPU
  transport/upload readings, not the SPC700 driver. Extend lab-only mapping to
  its code/data boundary with dynamic evidence; keep ROM-derived listings private.
- First DSP candidate: pinned `SPC_DSP`, with source/header/license audit and
  explicit dependency integration before adoption. Preserve its own notices;
  the project MIT license does not relabel external files. No source is imported
  by this ready record.
- Local macOS sanitizer startup is known to stall. Record timeout/unavailability
  accurately and obtain hosted Linux sanitizer evidence for the source candidate.

## Acceptance

Freeze scripts, exact clocks/options, sample/event layouts, horizon and input
hashes before using native results as acceptance. Do not tune tolerances to a
candidate or regenerate expectations to make it pass.

| Criterion | Experiment/check | Expected result | Artifact |
| --- | --- | --- | --- |
| Observation integrity | Fresh original repetitions; instrumentation on/off | Exact uninterrupted state and output; complete ordered raw capture, no loss | identity/non-perturbation ledger |
| Content boundary | ROM upload audit, bounded extraction and pack checks | Identified score/instrument/sample ranges; no executable dependency | extraction record and rules |
| Hardware isolation | Laboratory replay of timed DSP/RAM changes | Exact raw stereo PCM and counts, independent of the sequencer | diagnostic report; not product acceptance |
| Native audio | Native commands/sequencer on cold primary and variations | Exact ordered command/DSP events and raw PCM throughout frozen domain | frozen original/native differential reports |
| Boundary behavior | Overlap, retrigger, stop/restart and withheld timing | Exact declared state/event/sample differences; stable integer ordering | cases and independent review |
| Native save | Fresh-process restore/continue against matching uninterrupted native runs | Exact core events and PCM including internal DSP/history/fraction state | restore reports; reference-audio exclusion preserved |
| Product | Visible frontend and real navigation input with audible output | Native music/effect overlap and scene lifecycle, no emulator fallback | live report and recording |
| Regressions | Required suites and frozen game/presentation gates | Accepted gameplay timing/state/pictures unchanged | checks with source/input identities |
| Review/integration | Fresh isolated tier-1 review, final-tip required CI, merge PR | Findings resolved; synchronized main and cleanup | PR comment reviews and closeout |

## First experiment and handoff

1. Verify AUDIO-DECISION PR/closeout and synchronized clean main; sample quota.
2. Reproduce R-0074's cold `reference verify` and callback audit. Read pinned
   DSP output/serialization and CPU sound listing, then define a common integer
   clock and event-order schema for new laboratory instrumentation.
3. Add lab-only pre-resampler PCM/DSP-write/APU-memory capture and prove it
   non-perturbing before decoding one sound upload. Keep old references intact.
4. Compare baseline with the one-input variation to locate the first driver
   branch and separate executable from content ranges. Update this record with
   exact frozen scope and evidence before native implementation grows.

No implementation, capture interface, native sound or future pass is claimed by
this preparation. Internal research checkpoints do not require the user to
choose or resume the next experiment. Continue the capability through review
and integration within the actual D-0004 resource boundary.

## Claim experiment

Read R-0074 and main `artifacts/static-map/bank-82.lst` before new capture
design. Preserve the old core and evidence. Build an isolated updated laboratory
core so instrumentation on/off and the previous binary can be compared. One
bounded fresh Astra/medium consultation in `.worktrees/audio-clock-consult`
examines the integer clock/event schema before it is implemented; it has no
write ownership or acceptance authority. Tier 1 remains required for the full
capability, including a fresh Sol/medium reviewer and withheld cases.

## 30 September 2026 23:33 UTC checkpoint

- Foundation reproduced: two old-core cold 700-frame runs, exact R-0074 callback
  audit and passing doctor. Old/new-disabled/new-observed baseline sample, A/V
  and final-state digests agree. Both observed schedules repeat full event/PCM
  bytes; details and remaining limits are in R-0075.
- Laboratory capture and standalone DSP replay are implemented experiments.
  Replay matches 448,326/448,325 raw pairs and both final 64 KiB APU memories.
  One-entry overflow fails explicitly. Seven new tooling checks and the existing
  33 reference-tool checks pass; full native/hosted checks have not run.
- Bounded fresh `gpt-6-astra`/medium consultation completed. Adopt separate
  executed DSP clocks, SMP/CPU tick domains and sequence order; retain existing
  synchronization, effective RAM writes and CONTROL latch clears. Do not use a
  replacing PCM hook or infer per-cycle sample phase from a batch's final phase.
- Captures, core copies, private listings and failed overflow are in main
  `local/evidence/audio-title-menu/`. Build failures/success and replay report
  are in main `artifacts/audio-title-menu-integration/`. No product driver or
  audio pack is implemented; no task acceptance, PR merge or tag is claimed.
- Next experiment: audit the three cold upload records against effective IPL
  RAM writes, extend score-pointer/timer observation to a complete music loop,
  identify score controls/instruments/samples, then freeze native-domain gates
  before implementing recovered native functions. Check the first navigation
  DSP/PCM difference against the baseline and keep its timing explicit.
- Weekly usage remains 12% at 23:31 UTC, startup 12%, boundary 32%. No reset,
  spending, provider change or percentage-boundary override authorized.

## 23:43 UTC isolation correction

Source experiment `e9c64e6` is pushed; draft [PR #50](https://github.com/malmazuke/unirally-reconstruction/pull/50)
is attached and not ready for merge. First hosted checks failed: one new test
import and 56 frozen manifest identity checks. Restore the default lock/patch
unchanged and use new opt-in `tools/locks/audio-observation.json` with
`bsnes-audio-exports.patch` in `local/emulators/bsnes-audio`; do not rewrite any
frozen manifest or loosen identity matching. All captures remain canonical main
evidence. The consultant's read-only checkout is removed; no capture was there.
