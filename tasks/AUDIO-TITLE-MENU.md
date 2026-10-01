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

## 23:47 UTC reassessment and next question

The isolated hardware diagnostic passes, so retain SPC_DSP as the first product
candidate. New dynamic evidence changes the next clock question: the two 700-frame
frontiers differ by one raw pair after only Down changes, and native frontend
currently represents frame/update timing rather than CPU instruction cycles.
Request one new bounded fresh Astra/medium consultation on how to recover native
producer command-delivery clocks from original CPU/APU observations without
original-code execution, captured schedule playback, approximate PCM acceptance
or weakening the event/sample contract. This is a different question from the
first capture-ABI consultation. It is read-only in
`.worktrees/audio-native-clock-consult` at `91c8f05`, limited to ten minutes and
one active child. Weekly usage is 13%, startup 12%, boundary 32%.

The three cold IPL transfer groups match full contiguous ROM spans dynamically:
4,445 bytes from `0x9C6B6` to `$0400`, 627 from `0x9EAA0` to `$1600`, and
2,206 from `0xA026B` to `$1D00`. Each advertised record length includes six
bytes beyond its nominal record end when used by this upload path. Preserve
this observation; extracting a whole upload would still admit executable code.
The sample audit finds 21 BRR payloads, 42,498 bytes total, each block-aligned,
matching APU memory and its directory loop pointer. Source table `$03:FCF5`
and the dynamic sample-bank entry at frame 42 identify their record IDs.
`sample-audit.json` and corrected `upload-groups-corrected.json` retain identities.
The incorrect first upload grouping used an acknowledgment-PC site instead of
the store PC and is retained as a failed exploratory artifact, not evidence.

Next detailed trace adds register observations (X/SP/P) at SPC instruction
boundaries; observation ABI 2 remains 42 bytes per event, and ABI 1 captures stay
supported and preserved. Repeat native-independent state/A/V checks before
using those traces to decode score operations. No native capability acceptance.

Hosted macOS/Ubuntu synthetic CI passes on `91c8f05` after isolation correction
(run 36792397858). This proves the existing ROM-free suite, not private Linux
raw capture or native audio. The current ABI-2 trace build passes focused checks
and both 700-frame instruction/register captures keep their earlier sample,
A/V and final-state digests. Canonical `menu-base-registers` and
`menu-down-registers` retain them; initial ABI-1 originals stay intact.

## 1 October 2026 00:03 UTC checkpoint

The second bounded Astra/medium consultation is complete. Adopt its next gate:
predict native producer transfers and driver consumption across varied timing
before substantial sequencer implementation. The same-frame queue polling and
CPU/APU synchronization require a reduced causal timing model; captured command
timestamp lookup, constant frame rounding and executable uploads remain excluded.
A common cold-origin DSP horizon can remove frame packaging ambiguity, but does
not establish a matching native trajectory. Its final horizon is not frozen yet.

Observation ABI 3 adds optional CPU register watches and an unforced frame-leave
raster/CPU/SMP/DSP frontier. Baseline, Down 449/450/451/452, Up 450 and release/
retrigger captures are canonical main evidence with commands in
`cpu-capture-commands.json`; the baseline repeats exact complete event/PCM bytes.
Baseline and Down 450 retain the old sample, A/V and final-state digests. Native
clock prediction and musical-loop coverage remain pending. First-effect queue
latency varies strongly with the input frame, so a fixed command offset fails.
The private `cpu-timing-audit.json` retains exact observations; R-0075 will
distinguish the timing reading from the resulting implementation decision.

The corrected opt-in core build and seven focused audio-capture checks pass.
An initial ABI-3 guard incorrectly suppressed all new CPU events when full SPC
instruction tracing was off; corrected to suppress only kinds 10/11 before
these captures. Two patch-regeneration attempts failed explicitly on lock shape
and the untracked export source; no expectation was changed. The successful
patch build uses the documented full diff path exclusions and intent-to-add.
Hosted CI remains the earlier `91c8f05` result, not this uncommitted ABI-3 slice.
Weekly usage is 13%, startup 12%, boundary 32%; no reset or spending authorized.

## 00:14 UTC checkpoint

Pushed `46d01fc`; hosted macOS/Ubuntu synthetic CI passes at that head
(run 36794372256). Draft PR #50 remains unaccepted with no tier-1 review.
The conditional laboratory transport prototype derives bus costs from the
static map and pinned CPU memory/timing code, including per-scanline DRAM
refresh. It predicts all 38 retained transfer store clocks, but takes observed
function entries and readiness as inputs; it is not cold native acceptance.
Next predict the poll/return paths, producer enqueue phase and native driver
acknowledgments instead of supplying them from a capture.

Filtered SMP register observation (ABI 4) builds and reduces long traces to
selected routine boundaries while retaining all hardware/port/RAM events.
Baseline, Down 450/457, held Down and a 4,000-frame exploration are captured
in main evidence; validate filtered/full shared observations before use.
The long unsteered case enters the already-known idle demo after menu timeout,
so its later audio is outside title/menu coverage. Keep it as exploration,
not as a title/menu loop gate. Use alternating navigation before idle expires
for a music-loop experiment within menu.

The Down effect's last score control reaches `$126A`, clears voice tag
`$010F=FF` at SMP tick 18,771,648, and queues voice-7 KOFF `$5C=80`, written
at tick 18,771,712. This identifies a bounded terminating cue within 700 frames;
its data/control/timing recovery is not yet implemented. The exploratory
handler-table artifact had an erroneous +1 on SPC RTS targets; retain it and
correct against observed targets before using it. Weekly usage is 14%,
startup 12%, boundary 32%. No reset/spending/provider change authorized.

## 00:28 UTC checkpoint

The filtered observation matches all 3,241,178/3,240,836 shared hardware/port/RAM
events, full PCM and sample/A/V/final-state digests in baseline and Down 450.
Selective traces, old full traces and all four core binaries remain canonical
main evidence. The long steered menu run sends only the initial music command
and four navigation effects over 4,000 frames; it stays in menu by moving before
idle timeout. Score controls and instrument selections are recorded in
`music-steered-score-audit.json`; these are recovered observations, not yet a
validated native parser or full musical-loop assertion.

The initial conditional poll model fails 85-95 polls per case due to NMI work.
A source-derived semantic NMI model now matches every observed boundary in all
450 handlers, including palette arithmetic and DRAM refresh. Raster/pipeline
interrupt recognition leaves six poll failures in Down 450 (353,392 of 353,398
exact); keep the failure report and resolve them. No tolerance is introduced.
Next close interrupt recognition, then recover producer/OAM/arrow elapsed work
and native driver acknowledgments. Native driver, pack, output and restore
remain incomplete; PR #50 stays draft and unreviewed.

Focused tests: eleven observation and five conditional-clock checks pass.
Latest quota is still 14% used, startup 12%, boundary 32%. The completed second
consultant's clean isolated checkout can now be removed; it held no evidence.

## 00:34 UTC 45-minute reassessment

The clock prerequisite is advancing: the corrected conditional model matches
all 2,827,700 poll access clocks across eight diagnostic captures, and all 450
NMI handlers' five watched boundaries. It uses native PAL raster/pipeline,
register preservation, palette state and DRAM arithmetic; original function
entries remain supplied observations. This closes the local poll experiment,
not cold native producer/driver timing. The six failures were the caller JSL's
last stack-write edge and are retained in the earlier failed report.

Continue inside AUDIO-TITLE-MENU. Next recover the frame-wait/OAM/arrow/menu
producer phase, then driver acknowledgment and score timing. Freeze the full
cold raw-audio horizon and cases before acceptance comparison. A complete
terminating navigation cue is observed, but the longer title score still needs
a native parser and its control/instrument boundaries. No extra task or user
choice is needed. Reassess again within 45 minutes; checkpoint within ten.

Tier 1 and the automatic fresh Sol/medium review remain reserved for the full
capability candidate. Discretionary boundary remains 32% weekly used from the
12% startup; latest 14%. No reset, credits, paid API or provider switch. The
second consultant's clean checkout was removed; no evidence moved from it.
