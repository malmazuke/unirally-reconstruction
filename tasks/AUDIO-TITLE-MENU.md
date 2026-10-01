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

## 00:45 UTC checkpoint

Hosted macOS/Ubuntu CI passes for pushed `d0675dc` (run 36796940580).
The PR remains draft and lacks tier-1 review because cold native audio is still
incomplete. Conditional poll/NMI recovery remains exact in its supplied-entry
domain. The producer phase and driver acknowledgment are coupled: polling can
consume another queued command during a voice update, so continue driver
recovery alongside the native menu bus model instead of treating it as a blocker.

Reading `driver-observed-down-700.lst` identifies separate music/effect entry
tables, seven-byte instruments and per-voice control/call/loop state. Audit their
bounded data ranges and the runtime instrument-table copy before extraction;
full executable upload and captured state remain excluded from the product.
The two nominal data uploads include six next-record bytes in the observed
transfer, so payload length alone is not a proven score boundary.

Weekly usage at 00:43 UTC is 14%, startup 12%, discretionary boundary 32%;
no reset/spending/provider change. Next checkpoint within ten minutes and
45-minute reassessment by 01:19 UTC. Main is clean and synchronized at the
original task base; the preexisting playtest checkout is untouched.

## 00:54 UTC checkpoint

The data-only laboratory score parser predicts all 905 baseline, 933 navigation
and 11,490 long-steered score reads (voice, pointer, byte) exactly. It consumes
observed voice-update modes and command boundaries, so this is a conditional
parser result, not cold native audio. All 30 long-run controls are covered,
including nine random branches, calls/returns, counted loops and inline volume.
Failed reports retain the first missing silent-program native-state copy and
the initial reversed RNG byte order; expected reads were unchanged.

Next implement voice arithmetic and command consumption against these controls,
and adopt the already exact DSP-only diagnostic as a separately linked hardware
dependency. Preserve the six pinned source files byte-for-byte, their notices
and LGPL text; record hashes and the shared-library source/build arrangement.
No CPU source or executable upload will enter that dependency or content.
The current pack/profile is unchanged and the product is still silent.

Latest quota remains 14% weekly used from 12%; boundary 32%, no reset/spending.
Next checkpoint by 01:04 UTC, reassess by 01:19 UTC. Draft PR #50 remains
unaccepted; tier-1 review is reserved for the full capability candidate.

## 01:04 UTC checkpoint

The DSP hardware is now a separate shared-library candidate using six unchanged
pinned files, preserved notices/license and a documented build/source arrangement.
Its shared-library replay matches every raw pair and final RAM byte in baseline
(448,326 pairs) and Down (448,325), against unchanged evidence. Authored nonzero
BRR/echo tests pass batching, save non-perturbation and fresh-instance continuation.
The first two-sample adapter buffer failed because upstream switches buffers at
capacity; four samples fix the adapter. The first replay build failed a strict
stream-size conversion, then builds with an explicit bounded conversion. Earlier
failed reports remain. No native sequencer/PCM or audible product pass is claimed.

The conditional data parser and seven authored boundary tests pass, including
zero duration wrap, calls/loops, separate modes, restore, unsupported controls and
executable-pointer rejection. The two data ranges total 2,821 nominal bytes,
with all 1,968 retained score addresses inside them; six upload-overrun bytes per
record are excluded. No pack revision yet. Next recover voice arithmetic and
readable C++ driver state, including mid-update command polls, then close causal
timing and frozen cold raw acceptance. Tier-1 review and live output remain.

At 01:02 UTC weekly usage is 15%, startup 12%, discretionary boundary 32%.
No reset/spending/provider change. Reassessment remains due by 01:19 UTC.

## 01:14 UTC checkpoint

Hosted macOS/Ubuntu checks pass for pushed `d60761a` (run 36799359085).
The recovered conditional parser now has a C++ core candidate; its authored
calls/loops, unsigned duration and object-state continuation check passes.
Comparison to original score reads is running against the unchanged three
retained schedules. Observed commands/update modes still supply its inputs;
no cold native audio, full audio restore or audible product pass is claimed.
Next recover voice arithmetic and the coupled causal command/timer scheduler.
Reassessment remains due by 01:19 UTC. Latest usage is 15%, boundary 32%,
with review/recovery reserve and no reset/spending/provider change.

## 01:19 UTC 45-minute reassessment

Continue the same capability. The C++ conditional parser matches all 13,328
original reads in baseline, Down and the 4,000-frame steered menu schedule;
`native-score-conditional.json` retains data/source/runner/input hashes and
byte comparisons. Score controls now have a readable native state candidate.
The isolated shared DSP replay and conditional poll model remain exact in
their recorded diagnostic domains. None substitutes for cold native audio.

Next close per-voice arithmetic against original state at each update, then
connect command consumption and timer/producer timing causally. This reduces
the unknown output values before debugging clock positions. Product audio,
pack extraction, full audio restore, live evidence, regressions and tier-1
review remain required. Do not freeze an abbreviated headless acceptance.
Latest account sample at 01:16 UTC is 15% weekly used, reset 1791365217,
startup 12% and discretionary boundary 32%. Preserve the review reserve;
no reset, spending, provider switch or new frontier consultation.
Next durable checkpoint by 01:29 UTC and reassessment by 02:04 UTC.

## 01:29 UTC checkpoint

The C++ voice arithmetic matches 5,139 baseline and 5,155 Down updates exactly
from observed pre-update state (`native-voice-conditional.json`). Joining it to
the data-only score parser also matches all 10,294 complete voice state rows
from native initialization, with no observed voice-state seed
(`native-sequence-conditional.json`). Command boundaries, update mode and the
update counter remain supplied observations. The authored score check passes.

Next compare the native values of all six per-voice DSP writes, including the
long-steered schedule, then close causal scheduling. Cold original PCM, full
restore and live output are still unimplemented. No acceptance gate has been
weakened or frozen expectation changed. Tier-1 review remains pending.
Latest usage is 15%, startup 12%, boundary 32%, no reset/spending/provider change.
Checkpoint by 01:39 UTC; reassessment by 02:04 UTC.

## 01:39 UTC checkpoint

Native score/voice source slice `466f2bb` is pushed. Its conditional six-register
comparison is exact for 629,418 values across baseline, Down and long-steered
menu evidence. All 10,294 complete voice rows also match from native initialized
state. Authored integer-boundary and nonzero score/envelope continuation checks
pass locally. Native-symbol regeneration passes without changing its map.
Hosted macOS passes, Ubuntu fails in run 36801725844; inspect and fix the exact
failure before treating CI as passed. No private expectation is rewritten.

Next measure semantic driver work against original elapsed SMP ticks, then
recover its timers/acknowledgments and cold producer phase. Read bank-80's
static listing before designing the upcoming main-menu lifecycle capture.
PR #50 remains draft and unreviewed; cold PCM, pack, output and full restore
are still pending. Account usage at 01:36 UTC remains 15%, startup 12%,
boundary 32%, no reset/spending/provider change. Next checkpoint by 01:49 UTC,
reassessment by 02:04 UTC. Main remains at the task base.

## 01:49 UTC checkpoint

Pushed `807b496` makes two promotions explicitly unsigned after GCC rejected
them; local focused checks and hosted macOS/Ubuntu run 36802193352 pass. The
failed 466f2bb log remains in the canonical integration artifacts. The semantic
arithmetic work model matches all 10,294 original elapsed SMP intervals
(`voice-work-clock.json`), with no timestamp lookup. It is still conditional on
pre-update state and does not establish the whole driver schedule.

Fresh original captures retain rapid Down at 450/453 and main-menu A at 450,
B at 650 (`lifecycle-capture-commands.json`). The retrigger predicts all 74,304
per-voice DSP values exactly (`native-register-boundary-conditional.json`).
Confirm/cancel sends effects 2/4 and does not stop music. Its cancel sound reaches
the current pitch lookup bound and the native comparison fails explicitly;
retain the failure rather than treating it as a covered lifecycle case.
The earlier three-case report is retained unchanged at
`native-register-conditional.json` (also copied as the baseline report).

Next port elapsed-work accounting into native functions, recover score-control
work and driver timers, and capture a real scene transition that stops/restarts
title music. The data bound failure is internal recovery, not a user blocker.
Full cold PCM, live output, native audio restore and tier-1 review remain pending.
Latest usage remains 15%, startup 12%, boundary 32%, no reset/spending/provider
change. Next checkpoint by 01:59 UTC and reassessment by 02:04 UTC.

## 01:59 UTC checkpoint

The C++ work helper predicts all 10,294 original arithmetic durations exactly
(`native-voice-work-clock.json`), rejecting unclosed timed branches explicitly.
The confirm/cancel pitch repeat preserves state/A/V digests and identifies
indices 85, 90, 95 and 96. They alias the adjacent high-byte table and original
code bytes as numeric lookup data (`pitch-data-alias-audit.json`). Supply only
decoded pitch values, never executable behavior; preserve that provenance in
the future extraction rule. Extending the identified bound to 97 makes all
112,980 confirm/cancel DSP values exact. The initial bound failure remains;
the synthetic unidentified-index check moves to 97 for this recorded reason.

After reading R-0064 and the bank-80/bank-83 static listings, a fresh 1,850-frame
HUNTER code/soft-reset capture is canonical as `hunter-code-audio`, with its
command and consumed-command reports. It uses the accepted original code-route
pad schedule, not a state seed. Music persists during the pages; a new title
music command is consumed after the soft reset at SMP tick 58,071,522. Next
audit the bulk-transfer/driver-reset path's actual stop and restart boundaries.
This is lifecycle research; no new native lifecycle or PCM pass is claimed.

Account usage at 01:55 UTC is 16%, startup 12%, boundary 32%, reset 1791365217.
No reset/spending/provider change. Clock-work source and lookup extension are
not pushed yet; commit after focused validation and staged-diff inspection.
PR #50 remains draft; current pushed 807b496 has green hosted checks and lacks
independent capability review. Checkpoint by 02:09 UTC, reassess by 02:04 UTC.
