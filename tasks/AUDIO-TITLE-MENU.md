# AUDIO-TITLE-MENU - audible native title and menu audio

## Assignment

- Status: ready after AUDIO-DECISION integration; not claimed by preparation.
- Milestone: M4 (original game coverage).
- Coordinator/primary/provider/model: claiming session records these at claim.
  New OpenAI work defaults to Sol/medium under D-0004; retain its starting provider.
- Tier: 1. Native audio state, clock/order semantics, content extraction and
  new differential evidence require the full D-0006 review process.
- Quota: sample at claim; record baseline, 20-point discretionary boundary,
  final 20% review/recovery reserve, and UTC timestamps. Preparation grants no
  reset redemption, spending or provider change.
- Reviewer: automatically spawn fresh explicit Sol/medium, no inherited context,
  isolated checkout at the immutable candidate; post report on the PR. Handle
  findings and re-review before integration without a user trigger.
- Base: synchronized main after AUDIO-DECISION; record exact hash at claim.
- Dependencies: D-0009, R-0074; accepted native power-on/title/main-menu path,
  pack v29 and the pinned PAL ROM/reference core.
- Branch/worktree: create `codex/audio-title-menu` in its isolated checkout.
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
