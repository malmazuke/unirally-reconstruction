# AUDIO-TITLE-MENU - audible native title and menu audio

## Assignment

- Status: validated; bounded physical input/listening accepted 2 October 2026
  Sydney. Integration through PR #50 and its ignored closeout.
  Claimed 30 September 2026 UTC after PR #49 verification.
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
- At claim these were not implemented: raw DSP PCM, ordered DSP writes, APU RAM/upload observation,
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

## 02:09 UTC checkpoint and reassessment

Source slice `3d667bc` is pushed after staged-diff inspection and focused native
checks. It includes the semantic voice work helper and the identified 97-value
pitch lookup; neither executes uploaded code. Hosted macOS passes on this head;
Ubuntu is pending. The earlier 807b496 head has both hosted checks passing.
PR #50 remains draft and unreviewed.

The 45-minute reassessment is late by five minutes and is recorded explicitly.
Native score and arithmetic are substantially closed in the conditional domain;
the coupled critical path is driver scheduling, transfer acknowledgments and
cold CPU producer timing. Continue inside this task by identifying reset/transfer
boundaries and closing elapsed score-control work. Do not substitute captured
timestamps, event playback or a lab-only acceptance. Pack extraction, exact cold
PCM, full native restore, live output, regressions and fresh tier-1 review remain.
No worktree cleanup or accepted milestone is appropriate while those are open.

Account sample at 02:07 UTC remains 16% weekly used, startup 12%, boundary 32%,
reset 1791365217. Preserve the final 20% reserve. No reset, spending, provider
change or further frontier consultation. Next durable checkpoint by 02:19 UTC;
next reassessment by 02:54 UTC.

## 02:19 UTC checkpoint

Current pushed `3d667bc` has passing macOS/Ubuntu CI, run 36803954108. Native
score timing is now an uncommitted candidate: all 16,467 baseline and 16,467 Down
completed voice-call durations match exactly (`native-score-work-clock.json`).
The inputs remain observed music/effect/gain commands, update modes and counters.
Initial branch/call accounting failures are retained as `native-score-work-clock-
failure01.json` and `score-clock-difference01.txt`; no original result changed.

A fresh 4,000-frame repeat with poll/return/arithmetic/timer-read markers is
running as `menu-music-steered-clock`. It extends timed-control coverage while
keeping the pad script and original core options fixed. Next close those controls,
then native timer counters and command acknowledgments. Cold scheduling, full PCM,
pack, product output, restore and tier-1 review remain pending. No acceptance
claim or merge. Latest account reading is 16% from a 12% startup, boundary 32%;
no reset/spending/provider change. Next checkpoint by 02:29 UTC, reassess by
02:54 UTC.

## 02:29 UTC checkpoint

Uncommitted native score timing now matches all 106,760 completed voice calls
in the 4,000-frame steered repeat (`native-score-work-clock-extended.json`), plus
32,934 calls in the two 700-frame cases. The repeat preserves state/A/V/final
digests. A fresh 1,800-frame instruction capture closes the later nonzero pitch
convergence timing; `convergence-clock-reading01.txt` retains the decisive
reading. The earlier long timing failures remain as failure01/02 reports.

Native timer hardware and an observed-IO-boundary runner are implemented
candidates; their zero-target, edge, four-bit wrap, CONTROL and object-state
continuation check passes. Original counter comparisons are running. Score
work still takes observed command/update boundaries, and no cold scheduling or
PCM acceptance is claimed. Next connect per-voice output costs, timer-loop work
and acknowledgments causally. Pack/output/full restore/regressions/review remain.
Latest quota is 16% used from 12%, stop 32%; no reset/spending/provider change.
Checkpoint by 02:39 UTC, reassess by 02:54 UTC.

## 02:39 UTC checkpoint

Native timers match all 589,616 completed original counter reads, including
zero-initialized cold hardware phase (`native-timer-conditional.json`). IO write
and read boundaries are observed inputs; the driver loop is not yet native.
Native initialized key masks match 14,073 original writes, all 79,488 per-voice
output offset/return rows match, and all nine observed music/effect setup
durations match (`native-driver-work-conditional.json`). Those comparisons use
the two 700-frame cases and the fresh 1,800-frame full trace.

The source candidate also tracks output-enable state and accumulated key-on/off
masks. Next close the poll handshake and connect these work functions to a native
timer/voice/output loop, then remove the conditional cold CPU/loader boundaries.
No captured timestamp/event playback can enter the product. The app is still
silent; pack, cold PCM, full restore, live evidence and independent review remain.
Current changes are not committed yet; finish focused checks, record the source
and inspect the staged diff before pushing. Latest account sample at 02:36 UTC
is 16% used from 12%, stop 32%, reset 1791365217. No reset/spending/provider change.
Next checkpoint by 02:49 UTC, reassess by 02:54 UTC.

## 02:49 UTC checkpoint

Source slice `d8b15c7` is pushed after staged-diff inspection, focused native
checks and unchanged native-symbol regeneration. It carries native score/setup
work, output-enable/key masks, per-write work offsets, extended convergence
timing and hardware timers. Repeated primary register comparisons still match
all 629,418 values exactly. PR #50 remains draft; new hosted checks are pending.

Next build the native semantic driver loop from its source reading, using the
current components for counters, updates and output. A laboratory comparison
may still supply the loader completion boundary and CPU bus writes as declared
conditions while recovering that loop. They cannot become product acceptance
seeds: cold loader/producer work must subsequently replace them. The product is
still silent; full cold PCM, pack, native audio restore, live input, regressions
and tier-1 review remain required. Latest quota is 16% from 12%, stop 32%, reset
1791365217; no reset/spending/provider change. Checkpoint by 02:59 UTC; 45-minute
reassessment by 02:54 UTC.

## 02:54 UTC 45-minute reassessment

Current pushed d8b15c7 has passing macOS/Ubuntu hosted checks, run 36807332143.
The bounded conditional score, timer, output and key-mask checks close separate
parts of the driver; they still do not establish the cold capability. Continue
by joining them into explicit native poll/update phases and comparing their
computed schedule. The first lab check may condition loader completion and CPU
bus writes, while keeping those inputs outside product acceptance. Then recover
the cold loader and native CPU producer timing in this same task. Full pack,
PCM, restore, live output, regressions and independent review remain required.

Fresh quota at 02:53 UTC is 17% weekly used from 12%, discretionary boundary
32%, reset 1791365217. No reset, spending, provider change or new consultation.
There is no external blocker; the remaining work is coupled native recovery.
Next checkpoint by 03:04 UTC; reassessment by 03:39 UTC.

## 03:04 UTC checkpoint

The assembled native post-upload loop matches every completed DSP write, SMP
port write and port read in the two 700-frame cases and the 1,800-frame steered
case: 1,438,920 rows (`native-driver-loop-conditional.json`). Its inputs are
identified score/pitch data, the original upload-completion tick, prior timer IO
writes and original CPU writes at their observed SMP boundaries. It computes
poll/update/output scheduling itself. This is conditional driver evidence, not
cold initialization, CPU producer or PCM acceptance.

The first expanded comparison included the last observed SMP tick and produced
one additional port read per case. That tick is a pending port-read access at
which the original yielded to the CPU before logging the read. The retained
`native-driver-loop-end-frontier-failure01.json` records it. The comparison now
uses an exclusive closing tick on both sides; every earlier row is unchanged.
The source is uncommitted. Next extend the steered loop and stop fade, then
replace conditional loader and CPU boundaries. Product output, full restore,
pack, regressions and fresh tier-1 review remain. Latest quota is 17% from 12%,
stop 32%; no reset/spending/provider change. Checkpoint by 03:14 UTC; reassess
by 03:39 UTC.

## 03:14 UTC checkpoint

The four-thousand-frame steered post-upload loop also matches all 1,873,047
completed DSP/port rows (`native-driver-loop-extended-conditional.json`).
Native startup/sample-transfer functions now run from the observed final IPL
entry into the driver. The first real bootstrap comparison matched 514,680 of
514,681 rows; its single two-tick port-read offset is retained in
`native-driver-boot-read-offset-failure01.json`. The fix preserves the total
compare work and moves only the access within it. Expanded checks are running;
do not count them as passed before completion. An earlier harness omission of
its boot mode is separately retained as `native-driver-boot-harness-failure01`.

New source remains uncommitted. It constructs the sample directory and receives
BRR/parameter data through its own phase handshake, with no instruction reader.
Original CPU writes and final IPL entry are still conditions. Next validate the
startup slice, stop fade and coupled DSP PCM, then recover those cold CPU/IPL
conditions. Full native save, pack, audible product, regressions and independent
review remain. Quota last 17% from 12%, boundary 32%; no reset/spending/provider
change. Next checkpoint by 03:24 UTC; reassessment by 03:39 UTC.

## 03:24 UTC checkpoint

Source slice 255799a is pushed after staged-diff inspection, focused audio
checks and unchanged native-symbol regeneration. Native initialization from
the observed IPL-to-driver entry matches 2,084,772 completed projected rows
in the three full traces. Coupled native driver/DSP PCM matches all 1,340,625
stereo-pair comparisons at the fixed 14,300,000-clock horizon. These remain
conditional on original CPU write times and the final IPL entry; no cold CPU
producer or audible app acceptance is claimed. PR #50 stays draft/unreviewed,
and current hosted CI is pending.

A native timer-driven stop fade is an uncommitted candidate. A fresh 1,850-frame
HUNTER lifecycle repeat adds driver/stop entry markers and is running. Verify its
non-perturbation and compare the computed fade through the muted IPL boundary.
The subsequent reload, CPU producer, full native save, pack, audible output,
regressions and independent tier-1 review remain open. The readability check
finds one existing candidate function, score update_voice, at 105 lines; split
its controls under the current frozen conditional checks before acceptance.
Latest quota at 03:15 is 17% from 12%, stop 32%, reset 1791365217. No reset,
spending or provider change. Checkpoint by 03:34 UTC; reassess by 03:39 UTC.

## 03:34 UTC checkpoint

Pushed 255799a has passing hosted macOS/Ubuntu checks, run 36810065335.
The uncommitted native stop fade matches all 830,187 completed projected rows
through its original jump to IPL at SMP tick 55,249,134 and all 863,438 raw
stereo pairs through the fixed muted horizon of 27,630,016 DSP clocks.
`native-driver-stop-conditional.json` and `native-driver-stop-pcm-conditional`
record this bounded result. The initial stop comparison also included later
IPL polling, which is not implemented yet; its retained failure records the
extra original rows. The corrected stop domain ends at the original FLG E0
write plus the six-tick jump, not at a native-derived success frontier. The
fresh HUNTER repeat preserves state/A/V/PCM/final-RAM digests.

New opt-in observation ABI 5 adds complete CPU boundary/register capture to
recover cold producer and loader work. Its private core SHA-256 is
3483e8bcd27b0709758f186d97474c38fee62d66fb43a5b3d2ca51087f4a4539.
A first-100-frame cold script runs under old ABI 4, ABI 5 tracing off and ABI 5
tracing on: all state/A/V/PCM/final-RAM digests and all 903,135 normalized old
kind-1-through-9 rows are identical (`cold-cpu100-integrity.json`). Full CPU
trace retains 1,585,211 instructions at 636 distinct PCs, beginning at CPU
tick 186. Observer build and 13 focused tooling checks pass. The default
reference lock/patch and frozen expectations remain unchanged.

Next split the 105-line score dispatcher under frozen checks, then recover the
cold CPU/IPL producer and resumable native audio phases. Pack, real output,
full native save, regressions and independent review remain open; the app is
still silent. Latest quota remains 17% from 12%, boundary 32%; no reset,
spending or provider change. Checkpoint by 03:44 UTC; reassess by 03:39 UTC.

## 03:42 UTC 45-minute reassessment

Current pushed 255799a has passing macOS/Ubuntu hosted checks. Native stop and
ABI 5 CPU observation remain uncommitted; the score dispatcher has been split
into note initialization and four identified control groups under a frozen
pre-refactor evidence manifest. The native build and function-size check pass;
exact audio comparisons are being repeated before committing this refactor.
No original expectation or acceptance frontier is regenerated.

Continue cold CPU/IPL timing recovery and explicit resumable semantic phases.
The first CPU trace covers 636 distinct instruction addresses without perturbing
original output. It is research evidence, not a timestamp table for the product.
The existing synchronous driver can overshoot a horizon and has no complete
portable pending-state save, so it is not yet an app component. Cold producer,
pack, full PCM, native restore, audible output, regressions and independent
review remain required. There is no external blocker.

Fresh account sample is 18% used from 12%, discretionary boundary 32%, reset
1791365217. No reset, spending, provider change or new consultation. Next
checkpoint by 03:52 UTC; reassessment by 04:27 UTC.

## 03:46 UTC source checkpoint

The frozen dispatcher refactor passes all 2,084,772 startup rows, 106,760
extended score durations, 1,340,625 primary PCM pair comparisons and the
830,187 stop rows / 863,438 stop PCM pairs. Original expectations and fixed
horizons are unchanged. Four focused native and 13 capture tooling checks
pass. Source remains unaccepted: cold CPU/IPL, resumable audio phases, pack,
full save, audible output, regressions and tier-1 review are open.

Commit this tested stop/observer/refactor slice after staged inspection, then
continue the cold handshake. Next checkpoint by 03:56 UTC; reassess by 04:27.
Quota is 18% from 12%, stop 32%; no reset/spending/provider change.

## 03:56 UTC checkpoint

Tested stop/ABI-5/dispatcher refactor is pushed as cc5250a; separate formatting
commit 1b74042 is also pushed after focused tests and unchanged primary PCM.
The draft PR remains unreviewed and unaccepted; current hosted checks are
being read. Native-symbol regeneration retains 1,004 linked ROM citations.

New resumable IPL hardware-protocol phases start at tick zero and compute the
original driver entry 1,725,604 without supplying it. The first 100-frame
conditional comparison matches all 321,790 projected rows. The additional IPL
watches preserve every original old event and raw PCM; expected process and
observation metadata differ. A synthetic independent sender transfers 258 bytes
across a page/counter wrap and passes continuations from every exercised pending
phase. The original CPU write clocks are still inputs, and the driver itself
remains synchronous. Expanded comparisons and native reset reload are next.

No product/pack/full-save claim. Cold CPU producer, complete PCM, audible live
output, regressions and independent tier-1 review remain open. Fresh quota is
18% from 12%, stop 32%, reset 1791365217; no reset/spending/provider change.
Next checkpoint by 04:06 UTC; reassessment by 04:27 UTC.

## 04:06 UTC checkpoint

Native IPL/restart source is pushed as ba05578 after staged inspection, five
focused native checks and unchanged native-symbol regeneration. Earlier head
1b74042 passes hosted macOS/Ubuntu CI, run 36812608777; ba05578 CI is pending.
The draft PR description now states complete conditional SMP/DSP results and
the remaining CPU/product/save gaps. No review or acceptance claim.

Zero-tick native IPL and driver work match 2,402,280 primary projected rows
and 1,407,361 HUNTER lifecycle rows. Complete conditional raw PCM matches
2,049,776 primary pairs and 1,185,137 HUNTER pairs, including reload/restart.
Original CPU writes at observed SMP times are still inputs. Their executable
upload payloads are consumed only as handshake data and are not retained or
fetched by native logic. The product has no such input path or sound yet.

Next remove cold CPU timing conditions and convert the synchronous driver to
portable pending phases under driver-pending-refactor-frozen.json. Full native
audio serialization, content pack, SDL/live output, regressions and fresh
tier-1 review remain. Latest quota is 18% from 12%, boundary 32%; no reset,
spending, provider change or new consultation. Next checkpoint by 04:16 UTC;
reassess by 04:27 UTC.

## 04:16 UTC checkpoint

Pushed source remains ba05578; new cold CPU work is a private semantic-clock
prototype, not a native product component yet. The static reset/PPU/WRAM/VRAM/
asset-loading listings were read before the experiment. It now matches all
twelve cold milestones from hardware reset tick 186 through the first audio
CPU write at master clock 12,284,978, without an entry timestamp or instruction
reader. `native-cold-cpu-prefix-conditional.json` records the prototype hash,
readings and exact results. It does not recover later producer scheduling.

The first failure omitted the second MMIO byte in the 16-bit VRAM clear; the
second omitted/misclassified loader register-save work. Both failed reports
remain retained. Corrected cost/order matches all prior milestones unchanged.
Next move this semantic clock/work to C++, couple scanline/APU synchronization
and recover native upload/producer phases. The synchronous driver still needs
portable pending state, then full native save, pack, audible SDL/live evidence,
regressions and fresh independent review. No internal checkpoint is a blocker.
Latest quota is 18% from 12%, stop 32%; no reset/spending/provider change.
Next checkpoint by 04:26 UTC; reassess by 04:27 UTC.

## 04:27 UTC 45-minute reassessment

Current pushed ba05578 passes hosted macOS/Ubuntu checks, run 36813317760.
The new C++ cold CPU work is an uncommitted tested slice: all twelve original
clock milestones match without an entry seed. Coupled native CPU/IPL work also
matches every one of 65,500 port rows through the first sound write, including
CPU and SMP clocks and values. There is no original event stream or port timing
input in this bounded check. The retained first coupling failure identifies one
local-call bus-order mistake; the corrected pinned order preserves all expected
rows. New core functions pass the 80-line readability check.

Continue after committing this scoped prefix. Native upload/producer phases and
resumable driver state still need implementation before the complete conditional
SMP/DSP results can become a cold capability. The app remains silent; pack, full
portable audio save, live output, regressions and mandatory independent review
remain. No internal research dependency or reassessment needs user action.

Fresh quota at 04:24 is 19% used from 12%, discretionary boundary 32%, reset
1791365217. No reset, spending, provider change or new consultation. Next
checkpoint by 04:37 UTC; reassessment by 05:12 UTC.

## 04:37 UTC checkpoint

Tested cold-prefix source is pushed as c63bc4f after staged inspection, six
focused native checks and unchanged native-symbol regeneration. The draft PR
description includes the seedless 65,500-row prefix and remaining coupled work.
Current hosted checks are pending.

The driver continuation refactor is now in progress and unverified. It records
authored semantic phases, computed pending port/timer/DSP/RAM operations and
score/timer/counter state, rather than serializing a C++ stack. Original driver
functions remain available for the frozen conditional comparisons until the
resumable path matches. No app, full-save or new pass is claimed.

Complete the phase functions and repeat original event/PCM comparisons under
driver-pending-refactor-frozen.json. Then use the pending port boundary to join
the native CPU upload/producer work. IPL/CPU/DSP coroutine timing, including
scanline synchronization and the SMP forced-sync guard during long work, must
remain explicit rather than replaced with converted timestamps. Product pack,
fresh-process audio save, real audible input, regressions and independent
review remain open. Quota last 19% from 12%, stop 32%; no reset, spending or
provider change. Next checkpoint by 04:47 UTC; reassess by 05:12 UTC.

### 1 October 04:47 UTC checkpoint

The pending-phase refactor builds and preserves all 3,809,641 frozen selected
SMP/DSP/RAM rows across baseline, Down, 1800-frame music and HUNTER restart.
`native-driver-pending-conditional.json` records original hashes and first
differences (none). The three primary PCM captures preserve all 2,049,776
pairs in `native-pending-pcm-conditional.json`; restart PCM is running. Seven
focused native audio checks pass, including an authored stop with suspension
before every port read, reconstruction of a new driver object and restoration
of its pending work. Full native CPU coupling, canonical fresh-process save
and product audio remain open. This is refactor evidence, not acceptance.

CI 36815276400 passed macOS and Ubuntu for c63bc4f. Fresh account telemetry at
04:45 remains 19% weekly used, ordinary usage allowed, reset 1791365217, no
purchase/reset/provider change. The 12% session baseline and 32% discretionary
stop remain in force; preserve the final 20% review/recovery reserve. Continue
with sliced restoration comparisons and cold upload transport. The 05:12 UTC
reassessment is still due.

Restart PCM and sliced object restoration subsequently passed: all 3,234,913
raw stereo pairs remain exact, and 31,324 driver snapshots across the four
event domains preserve all 3,809,641 rows. Reports:
`native-driver-resumed-conditional.json`, `native-resumed-pcm-conditional.json`
and `native-resumed-pcm-restart-conditional.json`. Readability checks pass after
splitting register output into its own small update function; regenerated
native-symbol metadata remains unchanged (1,004 linked citations). This closes
the driver continuation experiment, not canonical full-save acceptance.

Next experiment reads `artifacts/static-map/bank-82.lst` $82:808F-$82:8160
(upload directory, IPL byte transfer and data cursor) and removes observed
CPU writes from the cold transport. The static listing supplies the reading;
the frozen cold CPU instruction/port traces supply dynamic evidence. Preserve
clock coupling order and narrow any first divergence before extending coverage.

### 1 October 05:01 UTC checkpoint

Driver continuation work was committed as 63004f5 and pushed; the separate
formatting-only commit aaa779d is also pushed. Seven focused native audio
checks passed after formatting. The draft PR description now distinguishes
resumable driver state from the still-open canonical full audio save. No
capability review, merge, tag or acceptance was claimed.

Before cold CPU upload implementation, `cold-cpu-upload-frozen.json` fixed
164,801 projected events from power-on through the third final IPL jump
request (C=17,907,842, S=1,725,519). All CPU/SMP port accesses, selected IPL
clear/score RAM, order and both clocks are required. Only the first executable
transfer's 4,445 C1/R1 payload values are opaque; no executable payload is
supplied to native code, and native IPL discards those bytes. Hardware
handshake decisions depend on counter/header latches, independently of those
payloads. This projection was recorded before implementing upload work and
does not relax command/DSP/PCM or product acceptance. Typed resource lengths
and score data with six trailing resource-header bytes remain private inputs.
Static bank-82 $808F-$8160 and bank-80 $A100-$A10E supply the routine reading.
The first coupled comparison is running; CPU resumability, later producer,
full save and product integration remain open. Next checkpoint by 05:11;
45-minute reassessment due 05:12. No new child, credit or provider change.

### 1 October 05:11 UTC checkpoint and reassessment

Cold native CPU/IPL transport matches all 164,801 frozen projected events
through the third final jump request, including both clocks and all handshake
and score data. `native-cold-cpu-upload.json` records C=17,907,842. The first
failed attempt omitted CPU synchronization on SMP port writes; pinned
`sfc/smp/io.cpp` confirms that writes and reads both synchronize. The
comparison's first version also used FFE4 for the declared opaque executable
R1 reads; the observer's advanced PC is FFE0. The unchanged declared domain
was corrected in `cold-cpu-upload-frozen-v2.json` before the next attempt;
version 1 and `native-cold-cpu-upload-failure01.json` remain retained. No
clock, order or non-executable value requirement was changed.

The joint cold CPU/native IPL/native driver extension matches all 169,905
frozen projected events through the first CPU P2=128 ready acknowledgment
(C=18,125,718, S=1,746,507). `native-cold-cpu-driver-ready.json` includes the
initial 29 DSP writes, port clears and selected voice RAM. Driver entry,
physical synchronization and the noncommunicating-SMP lead guard are computed
from zero. The guard preserves suspension before its deferred timer step;
an authored restore check is being added at that boundary. No event/timestamp/
RAM seed or executable payload enters the native run. CPU functions currently
use an uninterrupted caller stack, so canonical CPU continuation remains open.

Fresh telemetry at 05:10 remains 19% weekly used, ordinary usage allowed,
reset 1791365217. Baseline 12%, discretionary stop 32%, final 20% reserve.
CI 36817262129 passes macOS and Ubuntu at aaa779d. At this reassessment,
continue the same assigned capability: next recover the 64-slot sample loader
from static bank-82 $82A5-$8336 and identified resource metadata, then close
producer timing and canonical state. Full command/PCM/product acceptance and
fresh isolated review remain open; no fallback, task boundary, credit or
provider change. Next checkpoint by 05:21 and reassessment by 05:56 UTC.

### 1 October 05:22 UTC checkpoint

Joint cold CPU/native IPL/native driver work now matches all 713,909 frozen
projected events through the 64-slot sample loader's final request, including
21 selected samples and 42,498 BRR bytes. `native-cold-cpu-samples.json`
records C=41,360,956/S=3,985,343 at that request and computed caller return
C=41,361,676, independently matching CPU instruction trace $80:A119. The
first harness run compared 17 valid later return-work events beyond the
already-frozen endpoint; `native-cold-cpu-samples-endpoint-failure01.json`
is retained. The comparator now applies the frozen CPU endpoint to output,
without changing any expected row. No earlier difference occurred. The
62530-pair original PCM prefix through this return is entirely silent; it was
frozen for reference but is not an audible acceptance result.

44fb240 (cold uploads/driver-ready clock coupling) is pushed. Seven focused
audio checks and native-symbol metadata regeneration pass after the sample
work; readability passes. Sample changes remain uncommitted pending their
record/source audit. Next read and capture the remaining cold frontend work
after $80:A119, its cue enqueue and receiver calls, to remove observed producer
entry times. Full canonical CPU continuation/device playback and independent
capability review remain open. No task completion or user prerequisite.
Next checkpoint by 05:32 and reassessment remains 05:56 UTC.

### 1 October 05:36 UTC checkpoint

01c92cc (native cold sample transport) is pushed. New original 350-frame
CPU-full/off captures preserve game state, A/V, raw PCM, final APU RAM and all
1,876,844 shared event rows (`cold-cpu350-integrity.json`): full observation
contains 5,051,777 instructions/957 PCs. A local TSV decoder initially shifted
register fields; its incorrect hash and correction are retained. C/PC columns
and raw captures were unchanged; no native register logic used bad fields.
Correct ABI5 mapping is 12(address=A,value=X), 13(Y,S), 15(B|E<<8,P).

Native ring enqueue and first dispatch now match 713,929 frozen projected
accesses from zero through $80:FAE3 (C=41,365,774). Queue read/write cursors
are 2/4 and expected phase64 after sending music1/parameter2; initial words
use high-byte command/low-byte parameter ($0102, $08FF, $077F). No observed
enqueue/dispatch time enters native work. Changes are uncommitted while the
next frozen extension exercises PAL vertical-blank polling before $80:FAF1
(C=41,587,324). Static readings: bank-82 $8000-$807D, bank-80 $A119-$A169
and $FADF-$FAF1; pinned CPU I/O supplies the hardware blank status reading.

Fresh telemetry at 05:34 is 20% weekly used, ordinary usage allowed, reset
1791365217, from the 12% baseline. Stop discretionary work at 32%; preserve
review/recovery reserve. No credit/provider change. Next checkpoint by 05:46,
reassessment remains 05:56 UTC. CPU continuation, later producer, full save,
device audio and independent capability review remain open.

### 1 October 05:46 UTC checkpoint

Native ring and first PAL vertical-blank wait match all 714,357 frozen events
through $80:FAF1 (C=41,587,324), including initial music and effect-gain commands.
`native-cold-cpu-first-command.json` and `native-cold-cpu-first-vblank.json`
start from zero, use identified data/native handshakes, and retain the earlier
opaque executable-payload projection. The music-gain command remains queued
at this endpoint (read=3/write=4/phase128). The authored ring check passes
full-drop, busy receiver, FIFO, wrap, phase and header-before-parameter cases;
these boundaries have no new original gameplay claim. Readability and native
map regeneration pass. CPU continuation and later frontend work remain open.

CI 36819625073 at 01c92cc passes macOS but fails Ubuntu because the diagnostic
runner placed `return 0` on the same line as a preceding `if` (misleading
indentation under GCC/Werror). The line split is included in this source slice;
no requirement is weakened. Fresh telemetry remains 20% at 05:44 from baseline
12%, stop32%, reset1791365217. No reset/spending/provider change. Next checkpoint
by 05:56 and reassessment remains 05:56 UTC.

### 1 October 05:56 UTC checkpoint and reassessment

cb140ee (native ring/first wait) and f091aa6 (separate runner formatting) are
pushed. Native first-frame scene work matches the original return
C=41,588,122 with phase31 and 714,357 unchanged projected events. Single-channel
OAM544/palette216 DMA extension then matches 715,354 rows and C=42,020,908.
Nintendo graphics load extends the seedless run to 717,169 exact rows and
C=43,737,552 before fade. Reports are `native-cold-cpu-first-frame.json`,
`native-cold-cpu-base-palette.json` and `native-cold-cpu-nintendo-load.json`;
each expected projection/CPU milestone was frozen before its extension.
Identified raw graphics metadata supplies lengths/LoROM cursors, not timings.
Static readings: bank-80 FAF5-FBC4/9318-933B/A8A8-A8D3/B08C-B0CA, bank-83
99F6-9A1D, bank-82 B183-B1AD/B1DB-B20A/B296-B2DC. Pinned DMA/timing code
supplies activation/alignment/split-read work. Source changes are uncommitted.

At the 45-minute reassessment, retain this same capability and primary. Cold
frontend recovery is making exact progress; next finish Nintendo fade/hold,
title load and NMI producer work, then close CPU continuation and full save.
Do not substitute the conditional SMP result for cold/product acceptance.
Two focused CPU checks and readability pass. CI36821857866 at f091aa6 is pending.
Fresh telemetry remains20% at05:55, baseline12/stop32; no credit/provider change.
Next checkpoint by06:06 and reassessment by06:41 UTC.

### 1 October 06:06 UTC checkpoint

b12f8db (native frontend/DMA/load) and f5b35d7 (separate formatting) are pushed.
Nintendo fade/111-frame hold/fade returns at original C=96,917,470 with all
861,545 projected events exact. The same seedless CPU/IPL/driver plus DSP-only
model matches 145,925 complete raw stereo pairs through frozen D=4,669,600
(`native-cold-cpu-nintendo-pcm.json`), including first nonzero pair63620.
The DSP horizon only clips diagnostic output; it does not drive CPU or SMP.
This is the first joined nonzero PCM result without original CPU write inputs.
The app remains silent, CPU continuation/full save and final capability domain open.

The first title-load attempt's PCM matches160,150 pairs but its CPU/event
comparison fails: the implementation assumed the $77:10D0 flag was clear.
The unchanged original instruction trace shows FF and the 16-byte tour-level
restore branch at F56A-F580. Failure is retained in
`native-cold-cpu-title-name-failure01.json`. Correct the provisional domain
wording and model native cold cartridge initialization/flag ownership; no
expected event, clock or sample may change. Weekly telemetry remains20% at06:05,
baseline12/stop32, no reset/provider change. Next checkpoint by06:16,
reassessment remains06:41.

### 1 October 06:14 UTC checkpoint

Corrected title load matches all872,506 frozen projected events, original
C=106,373,342 and all160,150 raw stereo pairs through D=5,124,800. The same
old expected bytes remain unchanged; version2 corrects the mistaken flag
wording only. Pinned Memory::allocate defaults toFF and the initial RAM hash
in cpu350 samples equals8192 FF bytes. Native scene owns title_levels_pending,
executes the sixteen-byte level restore conditionally, then clears the flag.
R-0075 records the failure, corrected reading and exact result.

All eight focused audio checks pass, as do readability and native-map checks
(1009 cited addresses, zero without records). CI36822613819 passes macOS and
Ubuntu including hosted Linux sanitizers at f5b35d7. New Nintendo/title source
and DSP diagnostic extension await this checkpoint's commit. Next recover
NMI's hardware transition/last-cycle pipeline, its native frontend handler
and title fade/input work. The original first interrupt enters copied vector
body at00:8587 after the JSR atF5BD; simply inserting a fixed frame cost is
not justified. Canonical CPU/full save/product remain open. Next checkpoint
by06:24; reassessment06:41. Weekly20%, baseline12/stop32, no reset/spending.

### 1 October 06:32 UTC checkpoint

e3a2d45 (corrected title load and joined PCM) is pushed. The first native
interrupt-body attempt preserves all872,512 frozen events and160,155 PCM
pairs but ends six CPU clocks late. Pinned instructionPushEffectiveAddress
has no idle cycle; remove that extra native work and retain
`native-cold-cpu-first-nmi-clock-failure01.json`. The original first vector
enters bank00's ROM mirror at8587, correcting the preceding checkpoint's
word "copied"; no original expected event/clock/PCM changes. Recurring NMI
last-cycle/raster dispatch remains a separate open experiment.

CI36823945534 fails both hosts on one static-map symbol test: source map now
names audio_cpu_boot.cpp:reset_cpu alongside front_end.cpp:soft_reset at
91D1, but the tracked code map still lists only the latter. Preserve the
log in main artifacts and reconcile regenerated metadata, without weakening
the test. New interrupt source remains uncommitted. Fresh telemetry at06:29
is21% weekly used, ordinary usage allowed, reset1791365217, baseline12/stop32.
No reset/spending/provider change. Next checkpoint by06:42; reassessment06:41.
Canonical CPU/full save/product and fresh capability review remain open.

### 1 October 06:41 UTC checkpoint and reassessment

First NMI correction and the recurring semantic raster/last-cycle pipeline
match all872,512/880,206 frozen events and160,155/164,501 PCM pairs through
first handler return/title fade return respectively. Computed CPU endpoints
106,375,654/109,262,190 match the original. Title fade dispatches eight native
NMI bodies. Reports and the retained PEA failure are named in R-0075. The
CPU observer supplies a native callback, not an opcode stream; CPU cost helpers
now preserve each last-cycle point and DMA/MMIO lock delay. IRQ/HDMA/interlace
remain outside the tested domain. Readability passes.

Tracked static map regeneration passes with unchanged four coverage inputs:
53,062 sites,42,700 shared instructions,zero decoding disagreements. The
previous CI failure is stale native map metadata, now regenerated; source
checks are running before commit. One new A09F citation lacked a linked record
and is explicitly documented before final map regeneration. No expected
event/sample or test is weakened. Source changes remain uncommitted.

At reassessment, retain the same primary/capability: exact cold progress now
reaches recurring title interrupts. Next recover auto controller polling and
the remaining title/main-menu CPU producers, then canonical continuation,
pack and audible device integration. Existing CPU350 observations cover title
input but not the main-menu load; read static D20E-D3xx before a bounded longer
capture. Fresh telemetry at06:39 remains21% weekly used, ordinary usage allowed,
reset1791365217,baseline12/stop32. No credit/provider change. Next checkpoint
by06:51 and reassessment by07:26 UTC. Review/acceptance remain open.

### 1 October 06:52 UTC checkpoint

dd582e7 (native NMI pipeline/title fade and static metadata correction) is
pushed. Eight focused audio checks and 24 native-symbol tooling checks pass.
The draft PR description now reports the joined cold result and remaining
product/save/producer gaps; no acceptance, review, merge or tag is claimed.

Fresh CPU500 full/off observations cover title hold and main-menu load with
identical state, A/V, PCM, final APU RAM and all 2,451,192 shared kind1-9 rows
(`cold-cpu500-integrity.json`); full observation has 7,146,239 CPU instructions.
Static bank-80 D1EC-D1F9/D20E-D338 and B74A/BB9C-BCBF were read before capture.
The original six Start words are read on physical frames299-304, confirming
that auto polling after a frame leave latches the next worker input frame.
Native clock now owns serial auto-poll work; the runner receives a declared
controller schedule rather than observed port values.

Before native comparison, `cold-cpu-title-hold-frozen.json` fixes 1,007,259
events,235,622 raw pairs,C=156,499,030 and111 controller/end-clock projections.
New hold/input source is uncommitted and unverified. Continue from its first
difference before main-menu work. Fresh usage remains21% at06:52,baseline12/
stop32,ordinary usage allowed,reset1791365217. No credit/provider change.
Next checkpoint by07:02; reassessment07:26. Full save/product/review remain open.

### 1 October 07:00 UTC checkpoint

Native automatic serial polling and title hold match all 1,007,259 frozen
events,235,622 raw pairs,C=156,499,030 and every111 controller/end-clock
projection. Title fade-out extends to 1,015,110 events,240,101 raw pairs and
C=159,477,794 at886A. Reports are `native-cold-cpu-title-hold-pcm.json` and
`native-cold-cpu-title-return-pcm.json`; no expected bytes or clocks changed.
The Start-only domain does not exercise the newly implemented positive title
code branch, so it retains no dynamic claim until its own frozen variation.
All source functions pass readability. Both affected CPU checks and all24
native-symbol tooling checks pass; regenerated metadata has1016 linked ROM
citations and zero without records. New source/maps await commit.

CI36826309740 passes macOS/Ubuntu including Linux sanitizers at dd582e7.
The draft PR remains unreviewed and unaccepted. Next recover main-menu load
from static D20E-D36F and native resource metadata; NMI stays active during
loads, so raw graphics helpers need explicit instruction boundaries before
claiming that domain. Canonical CPU/full save, product playback and capability
review remain open. Last fresh usage21% at06:52,baseline12/stop32,no reset/
spending/provider change. Next checkpoint by07:10; reassessment07:26 UTC.

### 1 October 07:11 UTC checkpoint

5170576 (native automatic controller polling/title hold) is pushed. The next
menu-first-palette comparison passes all 1,017,400 events,241,424 raw pairs
and C=160,342,178 atD263. Before extending it, `cold-cpu-menu-graphics-frozen.json`
fixes the all-graphics endpoint D2AD,C=171,198,560,D=8,247,936,1,030,090
events and257,748 raw pairs. First all-graphics native comparison is running.
Original9202/9205 registers show FFFF/FF at the cold mode read; typed cold
menu_mode255 selects the default asset2 palette. Mode1's rider selection
remains outside this cold helper, explicitly rejected until recovered.

The raw loader now marks directory/asset read instruction boundaries, and
A16A clears native palette counters at its actual final two writes. First
Nintendo palette work is factored into a shared A8A8 callee; no original time
constant enters these helpers. Main graphics metadata is versioned separately
so the first-palette manifest/input identities remain unchanged. New source
is uncommitted; native-map/readability/affected regressions remain before
its commit. A runner variable-shadow compile error was corrected; the
preceding old executable rejected the new mode and produced no result.

Next finish menu OAM/text/intro/input producers, then canonical continuation
and product audio. The positive title code branch still needs its frozen
variation. Fresh weekly usage21% at07:06,baseline12/stop32,ordinary usage allowed,
reset1791365217,no reset/spending/provider change. Next checkpoint by07:21;
reassessment07:26 UTC. Capability review/acceptance remain open.

The all-graphics comparison subsequently passes: all 1,030,090 events,257,748
raw pairs,111 prior controller projections and C=171,198,560 match. Native
interrupt count153 and palette phase3/index0 are computed. No original
asset duration or producer clock is an input; typed raw lengths and LoROM
cursors include the bank wrap. Continue menu OAM layout/intro from D2AD.

### 1 October 07:21 UTC checkpoint

CI36827852572 passes both hosts at5170576. The new menu graphics and OAM
layout match all 1,030,159 frozen events,257,813 raw pairs and computed
C=171,240,248 beforeD36F. Native interrupt count153 and all111 prior title
controller frames match. R-0075 names the preimplementation frozen inputs
and reports. Both affected CPU checks and readability pass; regenerated
map/tooling checks remain before this source commit. Next native D36F wait,
menu cartridge validation and text work; static8871/8C4E/ACD5 was read.

Fresh telemetry22% at07:19,baseline12/stop32,ordinary usage allowed,
reset1791365217. No reset/spending/provider change. Next checkpoint07:31;
reassessment07:26. Full product/save/capability review remain open.

Menu checkpoint follow-up: the frozen Nintendo PCM regression also passes
after factoring the shared arrow callee. Static regeneration passes with
53,062 sites,42,700 shared instructions and zero disagreements; all24 native
symbol tooling checks pass after regeneration. A premature parallel tooling
run saw the old map during regeneration and failed its symbol-name check;
the sequential rerun is the reported pass. No expected result changed.

### 1 October 07:26 UTC reassessment

1355e17 is pushed: menu graphics/OAM and explicit NMI boundaries on raw
assets, with all frozen menu/Nintendo checks named above. Keep the current
primary/capability. Native exact progress now reaches the menu layout;
continue the coupled cartridge check/defaults and text work rather than
accepting a partial producer. Read static bank-83 8AF7-8B90/FB41-FB55 and
bank-80 8C4E-8C80 before the next candidate. The new frozen menu-clear
endpoint uses original C=171,925,134,D=8,283,232, through the8KiB clear.
Its native candidate is not yet implemented or verified. Cold SRAM and the
post-restart valid-signature branch must remain distinct. Last telemetry22%
at07:19,baseline12/stop32,no reset/spending/provider change. Next checkpoint
07:36,reassessment08:11. Canonical continuation/product/review remain open.

### 1 October 07:36 UTC checkpoint

1355e17 hosted CI36830051179 passes macOS/Ubuntu. Native cartridge clear
and first default tables pass their frozen comparisons:1,031,200/1,031,290
events,258,851/259,029 raw pairs andC=171,925,134/172,050,810 respectively.
No expected event, sample or endpoint changed. Native SRAM is an owned8KiB
array with explicit mirror and header decisions; the following initializers
consume identified local tables. New source is uncommitted. R-0075 records
this partial domain, not product/save or all records acceptance.

Read static83:93F5/963C/96BE and80:AD43/EFD4 throughF02A before league
default recovery. Cold member masks are six zero words; retain conditional
bit selection and unsigned tie comparison in the native helper. Freeze its
original endpoint before implementation. Usage remains22% at07:31,
baseline12/stop32,ordinary usage allowed,reset1791365217. No reset/spending/
provider change. Next checkpoint07:46,reassessment08:11. Product/save/review open.

### 1 October 07:46 UTC checkpoint

The native league slice matches1,031,547 events,259,375 raw pairs and
C=172,266,996. Full record defaults/checksums/reset initially match
1,031,945 events,259,822 pairs andC=172,562,846, but source inspection finds
an extracted-table bounds error: ten words at83:8472 include8484/8485,
beyond the1156-byte input. That initial match is retained as
`native-cold-cpu-menu-records-bounds-gap01.json` and is incomplete evidence,
not an accepted pass. Preserve the v1 input and all original events/PCM;
extract1158 bytes into a separate v2 file, freeze its hash before the
corrected candidate, and rerun with a focused terminal-word check.

Both affected CPU checks and readability pass before this correction. New
source remains uncommitted and maps are stale until regenerated. No product/
save acceptance or capability review yet. Next text work follows a bounded
corrected record slice. Fresh usage22% at07:46,baseline12/stop32,ordinary usage
allowed,reset1791365217,no reset/spending/provider change. Next checkpoint
07:56;reassessment08:11.

Bounds correction follow-up: the1158-byte v2 input has an identical1156-byte
prefix; all original events,PCM and endpoint stay fixed. Corrected full
records comparison passes1,031,945 events,259,822 pairs and111 controller
frames. All three affected CPU checks pass, including the terminal settings
word sentinel. Native files pass the80-line readability limit. Next regenerate
static/native metadata and commit this corrected cold record slice, then
recover menu text from8878. No acceptance, review or merge is claimed.

### 1 October 07:56 UTC checkpoint

Corrected cold menu record candidate and maps await this checkpoint commit.
The full prefix matches 1,031,945 events, 259,822 raw pairs, C=172,562,846
and all 111 prior title controller frames. The v2 table extends the original
identical prefix; its earlier bounds-gap result remains retained and incomplete.
Three affected CPU checks, readability and all 24 native-symbol tooling checks
pass. Static regeneration checks 53,062 sites and 42,700 shared instructions
with zero disagreements; all 1036 native citations now have records.
A missing explicit 8C80 record citation was added before final regeneration.

Next recover the menu text/intro work from 8878, reading static C3BC-C5C8,
8C41, D1FA,937B,F52B,A877 as the source of the routines. Existing CPU500
observations cover the first interactive menu entry, so no new capture is
needed for this prefix. Positive title-code/member branches and valid-header
restart still lack dynamic claims. Product playback, full canonical save,
final native primary/variations and capability review remain open. Last fresh
telemetry 22% at07:46, baseline12/stop32, no reset/spending/provider change.
Next checkpoint08:06; reassessment08:11.

### 1 October 08:08 UTC checkpoint

4d1439c is pushed. CI36833341266 passes macOS/Ubuntu; the draft PR body
reports the corrected record prefix and remaining acceptance gaps. PR issue
and line comments were read at07:52 and are empty; capability review has
not started. New native text code matches all 1,032,122 frozen events,
260,044 raw pairs and C=172,720,798 beforeACF7. Its owned cursor727 and
attribute7168 are diagnostics, not separately compared picture claims.
All111 prior controller projections match; readability passes.

The first edit script stopped at a wrapped runner anchor after writing core
files. The runner edit was completed and built before comparison; no stale
runner result is reported. No original expected bytes/clock changed. New
text source is uncommitted; maps/affected checks remain before its commit.
Next text DMA, common menu upload work and reveal to first interactive entry.
Fresh weekly usage22% at08:08,baseline12/stop32,ordinary usage allowed,
reset1791365217,no reset/spending/provider change. Next checkpoint08:18;
45-minute reassessment08:11. Full product/save/variations/review remain open.

### 1 October 08:33 UTC overdue checkpoint and reassessment

The 08:18 checkpoint and08:11 reassessment were missed during design work;
record the actual time rather than claiming those updates happened. Native
text remains the latest verified slice; the next frozen reveal target fixes
1,046,913 events,268,323 pairs,C=178,217,620 andD=8,586,336 before889A.
The native reveal candidate is now being implemented, not yet compared.
Keep the same primary/capability and continue interactive menu producers,
then full save/product/withheld variations. Product phase scheduling and
serialization ideas are hypotheses only, not implemented capabilities.
Last actual quota22% at08:08,baseline12/stop32,no reset/spending/provider
change. Next checkpoint08:43,reassessment09:18. Review reserve remains intact.

### 1 October 08:50 UTC overdue checkpoint

The 08:43 checkpoint is recorded at its actual late time. Native menu reveal
now matches all 1,046,913 frozen events, 268,323 raw pairs, C=178,217,620
and all 111 prior controller frames before 889A. Three affected CPU checks
and text/reveal readability pass. New source is uncommitted; regenerate
static/native maps and inspect the staged diff before committing this slice.
Read static ABC8-ACD5, B71D/B76F/B794/B178 and 83:9558 before the interactive
menu producer. The existing CPU500 baseline covers its first idle loops.

Last actual weekly usage22% at08:36, baseline12/stop32, ordinary usage allowed.
No reset/spending/provider change. Product audio, complete canonical save,
full primary/variations and independent capability review remain open.
Next checkpoint09:00; reassessment09:18.

Reveal checkpoint follow-up: static regeneration passes 53,062 sites and
42,700 shared instructions with zero disagreements; all 24 native-symbol
checks pass and all 1041 citations have records. Polling D1EC includes its
OAM DMA callee before automatic pad reads, as the listing states. No expected
projection changed. This slice is ready for a task-scoped commit.

### 1 October 09:00 UTC checkpoint

98fc2bd is pushed; hosted CI36838881558 passes both hosts. Native interactive
menu initialization and80 idle loops match the frozen prefix. Fresh Down450-455
observation on/off is non-perturbing, and the native variation matches all
1,137,795 events,319,556 raw pairs,80 menu controller/end clocks and the111
prior title frames. C=212,250,772. Its command ring produces{8,127}/{2,3}
from sampled input without original event times. Readability passes. New
menu input source is uncommitted; affected checks/maps remain before commit.

Fresh quota23% at08:53,baseline12/stop32,ordinary usage allowed,reset1791365217.
No reset/spending/provider change. Next freeze Up wrap/retrigger and long
steered-menu variations, then recover full canonical continuation and product
output. Lifecycle restart stays inside this task. Positive special-code,
attract and confirm transitions are semantic boundaries pending their joined
audio continuation, not newly accepted gameplay. Next checkpoint09:10,
reassessment09:18. No acceptance or capability review claimed.

Menu-input follow-up: the three affected CPU checks pass; regenerated static
metadata checks53,062 sites/42,700 shared instructions with zero disagreements.
All24 native-symbol checks pass and all1049 citations have records. Inspect
and commit this bounded slice before the longer withheld comparisons.

### 1 October 09:06 UTC continuation checkpoint

bfa377b is pushed: first native menu input producer. Up wrap, released-input
retrigger and 1380 steered menu loops now pass their withheld exact event/PCM
comparisons with the same core code. R-0075 names counts, horizons, on/off
integrity and precomparison hashes. The longest run is2,612,233 events and
1,152,477 raw pairs; full title/menu controller/end clocks match.

Before native system/serialization refactoring, freeze these reports in
`audio-system-refactor-frozen.json` under main evidence. Continue with one
owner for CPU clock/phase, native IPL/driver/timers, DSP/RAM/history and PCM
queue, then fresh-process continuation and product output. Scene stop/restart
remains coupled recovery; no audio device or capability acceptance yet. Last
fresh quota23% at08:53,baseline12/stop32,no reset/spending/provider change.
Next checkpoint09:16,reassessment09:18.

### 1 October 09:16 UTC checkpoint

ecbc52e is pushed with the unchanged-core withheld menu evidence. The new
shared native engine owns CPU/IPL/driver/timers/DSP/history and pending PCM.
Six frozen comparisons, including1380 steered loops, remain exact after
replacing the runner-only bus with this engine. Readability passes for the
engine, DSP wrapper and clock-state accessors. New ownership/session source
is uncommitted; canonical file serialization and fresh-process tests remain.

A checker source-list edit lost a quote and stopped before comparison; its
old JSON was not a new pass. The corrected scripts completed all six fresh
comparisons and bind the new source files. The pre-refactor reports remain
in `audio-system-refactor-frozen.json`; original expectations are unchanged.

Fresh quota23% at09:15,baseline12/stop32,ordinary usage allowed,reset1791365217.
No reset/spending/provider change. Next canonical state/file continuation,
then pack/device and joined scene restart. Product remains silent; full
capability review/acceptance open. Next checkpoint09:26,reassessment09:18.

### 1 October 09:18 UTC reassessment

Keep the current primary and capability. Seedless producer prediction now
covers the declared menu variations; coupled native ownership and canonical
serialization compile, with six pre-refactor event/PCM checks unchanged.
Continue fresh-process state/PCM verification, product packaging/output and
scene restart recovery. Do not substitute diagnostic or partial producer
acceptance for the requested audible capability. Last fresh usage23% at09:15,
baseline12/stop32, review reserve intact, no reset/spending/provider change.
Next checkpoint09:26; reassessment10:03.

### 1 October 09:26 UTC checkpoint

Native canonical URAU0001 source and the new phase owner compile. The format
carries CPU raster/NMI/autopoll/DMA state, IPL/driver/timers/voices, DSP RAM
and history, queue/scene/menu/title/cartridge work and pending PCM. Four
affected checks pass, including strict file/flag/truncation and transactional
rejection checks. The new runner's uninterrupted PCM/end CPU match the frozen
steered1800 run; its first event checker wrongly excluded every N RAM row,
including the frozen cold-upload prefix. Correct the projection to preserve
that prefix; no expected event or native code changed for this correction.
Fresh-process splits are now running and remain unverified at this checkpoint.
New ownership/serialization source is uncommitted; metadata regeneration and
staged inspection remain. Last fresh usage23% at09:15,baseline12/stop32,
no reset/spending/provider change. Next checkpoint09:36,reassessment10:03.
Product/lifecycle/review remain open.

Canonical follow-up: all eight fresh-process splits pass against the native
steered1800 run, whose original event/PCM/end-clock comparison also passes.
Each save parses/re-encodes/restores identically; concatenated events, pending
plus new PCM and final canonical state are exact. Four affected public checks
and new-file readability pass. R-0075 records this call-boundary domain and
remaining device/pack/lifecycle limitations. New source remains uncommitted
until map regeneration and staged inspection.

Ownership checkpoint follow-up: all10 focused native audio checks pass, along
with24 native-symbol tooling checks and the80-line readability checks. Static
regeneration verifies53,062 sites/42,700 shared instructions with zero
disagreements; all1049 citations have records. The engine/session/canonical
slice is ready for staged inspection and a task commit. Hosted CI at ecbc52e
is green; final candidate CI remains required after new source is pushed.

### 1 October 09:36 UTC checkpoint

451629f is pushed with shared native ownership/canonical continuation. Eight
fresh-process splits and all10 native audio checks pass; static/tooling and
readability checks pass as recorded. Product/device fractions and pack binding
remain open. Next add a bounded audio pack extraction: identified tables,
decoded97 numeric pitch entries (including the documented code-as-data alias),
sample headers/BRR and typed clock-work metadata, excluding uploaded executable
bytes and original event/time input. Preserve all existing payload identities.
Then connect native phase work to the visible frontend and SDL output, and
continue joined scene restart recovery. Fresh quota23% at09:35,baseline12/stop32,
ordinary usage allowed,reset1791365217,no reset/spending/provider change.
Next checkpoint09:46,reassessment10:03. Capability review/acceptance open.

### 1 October 09:46 UTC checkpoint

The v30 audio extraction candidate reproduces all 33 private fixture payloads,
including 97 numeric pitch entries,21 sample resources and typed clock-work
data. Its existing475 v29 entries are unchanged; the frozen compact sorted
entry-list SHA-256 is 6df6cfce7f83ef8ac93a4e6143f1987244dbb1901dc6a7b5e1fe61618cd487f7.
The program resource is excluded. New rules/extractor/compiled inventory are
uncommitted and have not yet passed the tooling/build checks. Next finish the
profile compatibility tests, typed content factory and canonical pack binding.
Last fresh quota 23% at 09:35,baseline 12/stop 32,no reset/spending/provider change.
Product/device/lifecycle and independent capability review remain open.
Next checkpoint 09:56,reassessment 10:03.

Pack follow-up: v30 extraction (508 entries), pack-only 1380-loop event/PCM/state
comparison, eight URAU0002 fresh-process saves, all 10 focused audio checks, 27
track inventory and 15 frontend tooling tests pass. R-0075 records preserved
v29/v1 data and the unaltered original expectations. Wrong initial unittest
module environment and an empty ctest selection are recorded as such, not
passes. Source is uncommitted pending maps, tooling and staged inspection.
Hosted CI passed for 451629f (run 36843422524); this new candidate still needs CI.

### 1 October 09:56 UTC checkpoint

Native pack binding/canonical v2 and all focused comparisons pass. Full lab and
Python tooling suites are running; new source remains uncommitted until their
results and staged inspection. Static regeneration again validates 53,062 sites
and 42,700 shared instructions with zero disagreements. The first generation
ran while this record was edited and reports source_changed_during_run=true;
`static-map-0954-stable.json` is the stable rerun. No expectation changed.

Fresh quota 24% at 09:56,baseline 12/stop 32,ordinary usage allowed,reset 1791365217;
no reset/spending/provider change. Next commit/push the content slice, then
stream native work against observed frontend input and add output queue/device
continuation. Monolithic cold work must stream its PCM without advancing using
unknown future controller words; its scheduling is an implementation dependency.
Scene stop/restart, live audible evidence and independent review remain open.
Next checkpoint 10:06,reassessment 10:03.

Content validation follow-up: full 539-test Python tooling suite and all 40 lab
tests pass (pack-tooling.log and pack-lab-tests.log). The v30/identity slice is
ready for a task-scoped commit after staged inspection. Product remains silent;
these checks do not close playback, lifecycle or capability review.

### 1 October 10:03 UTC reassessment

169a825 is pushed with the validated content pack and bound canonical v2.
The direct test-file invocation lacked the repository root on PYTHONPATH and
failed in existing imports; PYTHONPATH=.:tools ran all27 successfully. The
preceding full539-test tooling and40-test lab suites passed. Next retain the
exact native waveform while delivering PCM incrementally. Freeze the pack/v2
reports before this change (incremental-pcm-before.json); the candidate now
advances DSP from native SMP steps rather than only register/RAM accesses.
Its original event/PCM and eight fresh-process checks are running, not yet
claimed. The proposed live producer waits for actual sampled controller frames
and streams output; no guessed future words or observed duration input.

Keep this capability and primary. Pack/device scheduling and joined lifecycle
remain internal work; there is no audible acceptance or independent review.
Fresh usage was24% at09:56,baseline12/stop32,review reserve intact,no reset or
spending/provider change. Next checkpoint10:06,reassessment10:48.

### 1 October 10:06 UTC checkpoint

Incremental DSP advancement and a PCM output sink preserve the unchanged
original1380-loop event/PCM projection. All eight fresh-process native saves
pass (`native-incremental-pcm-save.json`). The pack-only streamed runner also
matches buffered events, PCM and final state (`native-stream-pcm.json`).
A modern output converter/queue and URAO0001 continuation are uncommitted;
the first build rejects three iterator signedness conversions under -Werror.
Fix explicit iterator offsets, then verify chunk boundaries, signed values,
fractional continuation and partial device drains before product wiring.
No original waveform expectation changed. Last fresh quota24% at09:56,
baseline12/stop32, no reset/spending/provider change. Live playback, joined
scene lifecycle and independent capability review remain open.
Next checkpoint10:16,reassessment10:48.

### 1 October 10:16 UTC checkpoint

169a825 passes hosted macOS/Ubuntu CI (run36846294670). The uncommitted output
converter/URAO0001 queue passes known signed interpolation, five output rates,
chunking, saved fraction, partial drains and malformed-file rejection. The
stream prototype blocks native automatic polling until the main loop supplies
its sampled input frame; SDL drains output only. A diagnostic opt-in flag wires
it into the app's cold power-on path, with joined post-menu producers still
outside this prototype. No full product/lifecycle/stream-restore pass is claimed.
The first app configure lacked Ninja in the shell path and failed; retry with
the repository's pinned Ninja and pinned SDL FetchContent source is running.
Last fresh usage24% at09:56,baseline12/stop32,no reset/spending/provider change.
Next compile and exercise visible audible navigation, then native lifecycle and
combined stream continuation. Capability review remains open.
Next checkpoint10:26,reassessment10:48.

### 1 October 10:26 UTC checkpoint

The app builds with pinned SDL/CoreAudio and an opt-in native title/menu stream.
The visible800-update run delivered753,664 stereo pairs,657,275 nonzero, with
zero reported underruns. A second1200-update real-key attempt observed Down
press/release events but zero nonzero sampled updates: both events arrived
between samples. It reached the idle-demo boundary after the prototype's menu
producer ended and reported274,032 underrun pairs. This is a failed navigation
check and an uncovered lifecycle, not a product pass. The UI screenshot showed
the main menu and later the demo title; no original code executed in the app.
The UI tool's first ambiguous bundle selection and expired-run timeout are
retained as such; the successful selection used this task's full app path.
Logs/commands: audio-live-prototype and audio-live-navigation under main artifacts.

The new lifecycle static listings are in main evidence native-lifecycle-static.
Read bank80 F0D6/F51B/98A4 and bank83 AB9A/ABDA-AC32 before the next capture.
The HUNTER route is a coupled stop/restart experiment, not an ending-audio
accuracy claim. Next freeze its native menu exit/31-frame transition and extend
through page work/soft reset; keep combined stream save and live navigation open.
Fresh usage24% at10:23,baseline12/stop32,ordinary usage allowed,reset1791365217;
no reset/spending/provider change. New output/stream source is uncommitted.
Next checkpoint10:36,reassessment10:48. Capability review/acceptance open.

### 1 October 10:42 UTC checkpoint

Fresh HUNTER lifecycle captures with observation disabled/enabled preserve
state, A/V, raw PCM and normalized APU events exactly. Three original endpoints
are frozen before the new transition code: menu exit,31 waited frames and first
fade. `hunter-native-lifecycle-integrity.json` and `cold-cpu-hunter-*-frozen.json`
in main evidence hold the identities. Static bank80 AC0A/F0D6 identifies8430 as
HUNTER and02B0 as RAM wipe; their provisional enum names were swapped and are
corrected without changing timing. New native31-frame entry and canonical v3
fields are uncommitted and unverified. Next build and compare exact endpoints,
then fresh-process continuation; HDMA/pages/warm soft reset remain unrecovered
for audio timing. Incremental PCM, output converter and SDL prototype changes
also remain uncommitted; their earlier focused passes remain bounded as above.

Fresh weekly usage25% at10:42,baseline12/stop32,ordinary usage allowed,
reset1791365217,no reset/spending/provider change. Capability review/acceptance
remain open. Next checkpoint10:52,reassessment10:48.

### 1 October 10:48 UTC reassessment

The menu code exit and31-frame HUNTER transition now match all frozen events,
raw PCM and per-iteration/end CPU clocks. The failed first transition report
(`native-hunter-entry-missing-interrupt-flag.json`) identifies missing shared
cartridge flag propagation to the native title NMI; connecting the flag after
its timed store fixes it without changing expectations. Canonical v3 public
file/transactional checks pass; a mistaken ctest regex selected no tests and
is not a pass. Fresh-process v3 splits remain next.

Keep the primary and task. Extend the frozen first fade before pages/HDMA and
soft reset; those dependencies belong here. HUNTER remains the chosen coupled
lifecycle because its original reset is already captured and source mapped.
Output queue/device continuation and successful sampled live navigation remain
open. Last fresh usage25% at10:42,baseline12/stop32,no reset/spending/provider
change. Next checkpoint10:52,reassessment11:33; no capability acceptance.

### 1 October 10:53 UTC checkpoint

All three frozen HUNTER endpoints match:1,082,257/1,117,696/1,136,291 ordered
events and288,176/308,018/318,288 raw stereo pairs, with exact CPU/frame clocks.
`native-hunter-entry.json` binds source/content and the unchanged originals.
Canonical v3 steered-menu fresh-process cases are running and remain pending.
The app rebuild initially named a nonexistent target (`unirally_app`); the
implemented `unirally` target builds successfully. Retain this failure as such.
Next add measured native navigation counts to the prototype's live report,
retry sampled keyboard input and continue owned output/lifecycle state.

Fresh usage25% at10:53,baseline12/stop32,ordinary usage allowed,reset1791365217,
no reset/spending/provider change. About11GiB free remains; keep captures bounded.
New streaming/output/HUNTER source is uncommitted. Product lifecycle and
independent capability review remain open. Next checkpoint11:03,reassessment11:33.

### 1 October 11:04 UTC checkpoint

All eight canonical v3 menu splits and five HUNTER splits pass fresh-process
event/PCM/final-state continuation. The40-key visible retry again observes
40/40 key edges and zero sampled input/native navigation changes; retain
`audio-live-repeated-navigation.log` as a failed live check. It crosses the
uncovered idle boundary and reports273,008 underrun pairs. No input policy or
frozen expectation is changed to force this check.

The new combined playback owner/URAP0001 passes owned PCM/count/fraction and
transactional malformed-state checks. Output restore now rejects inconsistent
counter/fraction histories; input chunks and queued output have explicit bounds.
Native engine moves are disabled because retained observers reference its
address. Next prove fresh-process combined continuation with partial drains,
then stable maps/source commit before further coupled lifecycle recovery.
Last fresh usage25% at10:53,baseline12/stop32,no reset/spending/provider change.
Capability acceptance/review and pages/reset remain open.
Next checkpoint11:14,reassessment11:33.

### 1 October 11:14 UTC checkpoint

The combined URAP0001 owner passes all eight fresh-process saves with257-pair
partial drains. Events, converted PCM and final native/output state are exact;
an independent integer conversion of frozen original raw PCM matches all
1,726,556 output pairs. Noncold saves retain nonzero fractions and pending queues
(`native-playback-fresh-process-save.json`). Both owners validate before restore
mutation; native DSP pair count is bound to output source count. The producer
prototype now uses this same owner, with mutex-protected callback drains.
Asynchronous thread-stack/OS mixer save and joined lifecycle remain unaccepted.

The latest source builds in lab/app and all41 lab tests pass. Native symbol/static
regeneration passes1049 supported citations,53,062 sites and42,700 shared
instructions with zero disagreements. The initial readability command failed
shell glob expansion; the structured subprocess retry is running. Full tooling
checks are next before staged inspection and a task-scoped draft source commit.
Fresh usage25% at11:12,baseline12/stop32,ordinary usage allowed,reset1791365217;
no reset/spending/provider change. No review or acceptance is preclaimed.
Next checkpoint11:24,reassessment11:33. Then resume HUNTER pages/HDMA/reset.

Source-slice validation follow-up: all539 tooling tests pass in83.1 seconds
(`audio-stream-tooling.json/log`), all41 lab tests pass, and six changed core
files pass the80-line readability check (`audio-stream-readability.log`; only
external-header warnings suppressed). Stable static regeneration passes and
records source_changed_during_run=false. The full capability/private regression
matrix, local sanitizer result, final-tip CI and independent review remain
required for acceptance. This slice is ready for staged privacy inspection and
a task commit; no original expectation changed.

### 1 October 11:26 UTC checkpoint

61aa53d0b9d5c8f2a27e2e8d8ce31873ec9c5431 is committed and pushed after staged
privacy/diff inspection. Source/content expectations are unchanged; only source,
tests, maps and records are tracked. PR50 remains draft with capability review
and lifecycle acceptance pending. The README/validation record distinguishes
the implemented opt-in prototype and call-boundary continuation from full
product/device/thread acceptance. No merge or milestone tag.

Read the native lifecycle listings' E2CF reveal, AB9A-AC32 page transitions and
the pinned bsnes CPU timing.cpp/dma.cpp before designing HDMA work. The current
work clock has no HDMA support. Next freeze existing original page/reset
endpoints and derive direct channels5/6 setup/run work, then load the identified
page/credit work metadata. Do not supply original scene clocks as native inputs.
Last fresh usage25% at11:12,baseline12/stop32,no reset/spending/provider change.
Next checkpoint11:36,reassessment11:33. Exact-head CI is running, not preclaimed.

### 1 October 11:33 UTC reassessment

PR50 describes the pushed61aa53d slice and its remaining capability gaps.
Ubuntu/changes checks pass for run36854994081; macOS was pending at11:30.
The static E2D3/B139 reading finds a page-reveal cue (gain79, effect2) in addition
to HDMA work. Native reading of25 required graphics entries shows all selected
page/credit palette/map/tiles are uncompressed; their metadata is in main
evidence `hunter-graphics-work-read.json`. The first attempted helper import
named a nonexistent symbol and failed; reading the documented directory bus
with the existing provenance converter succeeded. No source/data expectation
change follows that failed command.

The ending's copied110 bytes are followed by repeated HDMA reads of nearby
RAM. A bounded512-byte WRAM series on the unchanged1850-frame script is running
before choosing a native table-memory domain. Its capture/integrity and new
page/reset endpoint freezes precede implementation. Keep this primary/task;
these are coupled dependencies, not new acceptance tasks or a user blocker.
Last fresh usage25% at11:12,baseline12/stop32,no reset/spending/provider change.
Next checkpoint11:43,reassessment12:18. New hardware timing remains unverified.

### 1 October 11:43 UTC checkpoint

The unchanged1850-frame HUNTER capture with a512-byte WRAM series preserves
state/A/V/final digests, normalized original audio events, PCM and final APU
(`hunter-hdma-workram-integrity.json`). The adjacent cleared RAM is zero in
that tested domain. Five page/credits/reset endpoints are frozen before native
implementation (`cold-cpu-hunter-*-frozen.json`); endpoints are output checks,
not native timing inputs. The reveal queues gain79/effect2 over the music.

Native direct channel5/6 setup/run work is now being implemented with mutable
tables, byte line counters, cursors, scanline triggers and bus-only alignment.
The expanded continuation is explicitly URAU0004; earlier state formats remain
retained evidence and are rejected, not silently converted. This source is
unbuilt/unverified at this checkpoint. Joined lifecycle/product/review remain
open. Fresh usage26% at11:41,baseline12/stop32,ordinary usage allowed;
no reset/spending/provider change. Next checkpoint11:53,reassessment12:18.

### 1 October 11:53 UTC checkpoint

The new509-entry v31 pack passes extraction/identity/inventory checks; its only
addition is95 bytes of identified HUNTER graphics work metadata. All508 v30
entry descriptions/payload identities remain unchanged (inventory SHA256
945b74512a544739f8ca8d2ab35ea9aa537e73f528b73025acd3b0e8eb8905b0).
The native direct-HDMa reveal matches the first-page frozen original exactly:
1,225,156 events,372,735 raw stereo pairs and CPU247564544
(`native-hunter-page.json`), including gain79/effect2 over continuing music.
No original event/clock input is used. The typed tables/cursors are URAU0004.

The first focused tooling run had one failed source-table search: the old
substring selected hunter_audio_required rather than audio_required. A precise
33-entry declaration search fixes the reader; all3 audio inventory tests pass.
No frozen payload expectation changed. The second page's two-press wait,
decoration-counter work and timed wait are implemented and building; original
press/timed/credits-ready comparisons are next, not preclaimed. Full credits,
soft reset, canonical v4 splits, visible product and review remain open.
Last fresh usage26% at11:41,baseline12/stop32,no reset/spending/provider change.
Next checkpoint12:03,reassessment12:18.

### 1 October 12:01 UTC checkpoint

Three additional original endpoints are exact (`native-hunter-page-waits.json`):
two-press return CPU298213036/1,360,221 events/448,975 pairs; second-page reveal
CPU341615464/1,467,760 events/514,317 pairs; timed wait to credits entry
CPU425456504/1,692,487 events/640,565 pairs. The native decoration counters,
whole-pad two-consecutive-press rule, original enabled-pad test and byte/word
ordering produce these results. URAU0004 canonical malformed-state checks pass.
Credit pose work itself is not yet implemented. A fresh watch at838E3A/8484,
RAM199/19C and credits/pose boundaries completed in7.44 seconds after reading
those static listings and bank80 F814-F88C. This watch is original output only;
its integrity and endpoint freeze precede credits implementation. Native poses
will use already identified pack headers/reference words and explicit copy/DMA
work, never the original RAM instructions or a CPU interpreter. Full live/reset,
v4 fresh-process splits and independent review remain open. Fresh usage26%
at11:55,baseline12/stop32,no reset/spending/provider change.
Next checkpoint12:11,reassessment12:18.

### 1 October 12:11 UTC checkpoint

The new credits watch's normalized audio events, raw PCM, final APU and state/A/V
hashes match the unchanged1850-frame original (`hunter-credits-work-watch-integrity.json`).
Its endpoint artifact was written successfully before a diagnostic print used
an incorrectly padded hex key and raised KeyError; the retained capture itself
is complete. Frozen credits setup ends at CPU444899420, before pose work.

Native credits graphics matches AC6D432377202 and AD25444842364 exactly. The
first setup comparison fails at the final CPU endpoint by194 clocks, despite
matching events/PCM; retained `native-hunter-credits-setup-missing-work.json` is
a failure, not acceptance. Static glyph reading found an omitted large-glyph
shift and small-glyph transfer; the corrected comparison is running. No expected
endpoint is adjusted. Pose-copy implementation has not started: the attempted
atomic patch failed on a changed declaration and applied no files. Next finish
this frozen setup, then native pose copy/upload and remaining lifecycle work.
Fresh usage26% at12:05,baseline12/stop32,no reset/spending/provider change.
Next checkpoint12:21,reassessment12:18. Capability/product/review stay open.

### 1 October 12:18 UTC reassessment

The corrected credits setup is exact at all four native work marks:
AC6D432377202, AD25444842364, AD33444876556, AD67444899420
(`native-hunter-credits-setup.json`). The two omitted glyph operations are
restored from the static reading; original endpoints/payloads remain unchanged.
The first and second native pose builds also match original endpoints exactly,
CPU445008034 and445127992 respectively (`native-hunter-first-pose.json`,
`native-hunter-second-pose.json`). This includes native33-byte RAM copy work,
five192-byte DMA row uploads and the intervening NMI. The product loads no
original RAM routine, CPU opcodes or whole-emulator state; pose headers remain
identified static content from existing pack entries.

Continue the same coupled lifecycle through credits animation and warm reset;
then freeze fresh-process v4 splits and visible device/controller checks. The
currently pushed61aa53d slice remains the last durable commit, with new source
uncommitted until this next bounded slice is checked and maps regenerated.
No partial capability acceptance/review or merge is claimed. Last fresh usage
26% at12:14,baseline12/stop32,no reset/spending/provider change.
Next checkpoint12:28,reassessment13:03. Existing unaccepted scopes remain open.

### 1 October 12:28 UTC checkpoint

The native credits loop entry matches CPU451411026,1,742,677 events and679,626
raw stereo pairs. The loop-entry original was frozen before implementation
(`cold-cpu-hunter-credits-loop-frozen.json`); loop exit/reset-ready comparisons
are running. New native continuation owns the96-frame animation count and
uses the existing identified48-word credits pose table, with no clock/event
input. Latest fresh usage26% at12:24,baseline12/stop32,ordinary usage allowed;
no reset/spending/provider change. Capability/product/review remain open.
Next checkpoint12:38,reassessment13:03.

### 1 October 12:39 UTC checkpoint

Native credits exit/reset entry also pass CPU553148856/559931912, with1,927,492/
1,945,832 events and832,790/843,001 raw pairs (`native-hunter-credits-loop.json`).
The joined warm menu reaches CPU738265952 and all1,111,493 raw pairs exactly.
Its initial literal transport comparison fails because the native resource-50
uploader sends zero for executable bytes, as in cold boot. Retain the failure
(`native-hunter-warm-menu-opaque-code-mismatch.json`). A separately named v2
projection applies the existing cold policy to exactly4445 warm CPU writes at
82:80D6 and their4445 IPL port-1 reads: clocks/order/address stay asserted, code
values are opaque. All2,933,842 projected rows then match; no original raw
capture, PCM or literal projection is replaced. This explicit projection-policy
correction needs independent review and is not yet accepted capability evidence.

The full1850-frame tail diagnostic disproves a broad cleared-RAM claim: relative
bytes110..239 first change at frame1045 in credits. They are zero on every
frame0..803, covering both reveals (`hunter-hdma-tail-domain.json`). Earlier
comments/records must be read in that reveal domain, not the entire capture.
Fresh-process v4 and new transactional state checks are next. Last fresh usage
27% at12:37,baseline12/stop32; no reset/spending/provider change. All three hosted
checks pass for pushed61aa53d. No final-tip review/integration/acceptance. The
first validation patch failed on a formatted context and applied no files; the
precise retry is building. Next checkpoint12:49,reassessment13:03.

### 1 October 12:51 UTC checkpoint

All eight v4 native menu saves, eight URAP0001/URAU0004 partial-drain saves and
ten coupled HUNTER saves pass event/PCM/final-state continuation. HUNTER cases
include retained first_press=1, timed_remaining1100, credits wrap96->0 and
97->1, reset and warm-title entry. Reports: `native-fresh-process-save-v4.json`,
`native-playback-fresh-process-save-v4.json`,
`native-hunter-fresh-process-save-v4.json`. New native event text is verified
byte-for-byte in gzip before removing plain copies; original captures remain.
All11 native audio tests pass in2.75s, including new transactional bad-state checks.

A credits press at1297..1300 (three frames earlier) is frozen before comparison;
on/off instrumentation preserves original state/A/V/PCM/APU/events. Native warm
return matches CPU736989304,2,931,808 projected rows and1,109,573 raw pairs
(`native-hunter-warm-variation-v2.json`). The established opaque code-transport
policy is declared before this comparison. Native source stayed unchanged.
Next add active reveal call-boundary saves and clear represented reveal RAM at
the already timed warm WRAM clear, then stable source/maps/checks/commit.
Native-symbol generation writes1057 citations but reports two unsupported
record addresses83:A9FB/AAC5; add their identified glyph readings before the
required tooling check. Last fresh usage27% at12:50,baseline12/stop32; no reset/
spending/provider change. No capability/live/review acceptance.
Next checkpoint13:01,reassessment13:03.

### 1 October 13:03 UTC reassessment

Nine fresh-process reveal/lifecycle saves now pass, including active first-page
frames1/30/74 and second-page1/40, retained press history, animation wrap, reset
and warm title (`native-hunter-reveal-fresh-process-save-v4.json`). Represented
reveal RAM clears at the already modeled boot WRAM clear; native event/PCM
expectations stay unchanged. All41 lab tests pass in7.31s; all540 tooling tests
pass in96.78s;19 changed core files pass the80-line readability check. Maps pass
1057 supported citations,53,062 sites,42,700 shared instructions and zero decoding
disagreements (`static-map-hunter.json`). Private11-race gates are running.

The opt-in SDL producer now uses the same HUNTER page/credits/reset owner, then
restarts the warm title/menu. The visible1850-update scripted CoreAudio run
returns successfully and reports1,185,137 source pairs,1,761,280 delivered,
1,594,126 nonzero,0 underrun,1 native restart (`hunter-live-scripted-result.json`).
This uses the existing future-controller smoke-test path. It is not real-input
or listening acceptance. AX bound the visible app; a later screenshot call
timed out after this bounded run had closed. No screenshot is claimed.

Keep this primary/task. An asynchronous availability question requests the
physical held-key/listening input that the documented UI API cannot supply;
continue independent validation and durable source work while waiting. Internal
research is not a user blocker. Latest fresh usage27% at13:00,baseline12/stop32;
no reset/spending/provider change. Review and capability acceptance remain open.
Next checkpoint13:13,reassessment13:48.


### 1 October 13:15 UTC checkpoint

The eleven-race wrapper reported a120-second timeout on ZOOM ZOO primary and
left nine cases without completed reports; two short DRAGSTER regressions
completed with exact passing reports.
Each full comparison includes many fresh-process restores, so the wrapper's
timeout is distinct from its existing30/60-second per-native-run limits. No
behavioral mismatch was reported, and the nine incomplete cases are not passes.
Retain these results in `race-hunter/`; retry incomplete comparisons in a fresh
output directory with a900-second wrapper limit and unchanged native run limits.

Prepare a coherent immutable source candidate for automatic independent review
while completing full app/presentation/regression checks. The held physical-key
and listening question remains pending. No product acceptance or merge is
claimed. Latest weekly telemetry27% at13:12,baseline12/stop32; no reset,
spending or provider change. Next checkpoint13:25,reassessment13:48.


### 1 October 13:34 UTC review and portability correction

Immutable source `c520df4fb0d37433d393481cb0eb61c936b3ed0b` is pushed to PR #50.
Fresh isolated `gpt-5.6-sol`/medium [tier-1 review](https://github.com/malmazuke/unirally-reconstruction/pull/50#pullrequestreview-5379898024)
returns CHANGES REQUIRED: GCC signed conversion in pose-mask counting and the
SDL option's advertised v30 path failing when HUNTER needs v31 metadata. The
correction casts each byte to unsigned before shifting and rejects missing
HUNTER metadata before opening the audio device; help now names v31. Bounded
v30 laboratory support stays explicit. Source re-review and successor CI remain
required. Physical-input/listening acceptance is still absent.

The reviewer independently delayed the first-page press by seven frames, froze
original expectations before native comparison, and matched CPU 344,594,398,
1,475,651 projected rows and 518,802 raw pairs. Page-wait 122 fresh-process
restore matched events, PCM and final state. Its on/off instrumentation matches
all state/A/V/PCM/APU/event digests. It independently confirmed that the warm
v2 correction changes only 4,445 CPU port-1 writes and 4,445 matching IPL reads to
opaque, preserving clocks/order/address; all 2,933,842 rows and 1,111,493 pairs
match. Review evidence lives in main `local/evidence/audio-title-menu/reviewer-c520df4/`.

Exact-c520 checks: eight canonical native saves and eight combined playback
saves pass, source_changed=false; output independently matches 1,726,556 pairs
(`native-fresh-process-save-c520df4.json`,
`native-playback-fresh-process-save-c520df4.json`). App 41 CTest plus fresh
repeatability and both v1 presentation contracts/eight cases pass. Hosted
macOS/changes pass; Ubuntu fails on the compiler warning. Local app-sanitize
build passes, but CTest 45 s and repeatability time out; neither is a pass.

Four full frozen races pass at c520: ZOOM ZOO primary/idle/opposing-ride/opposing-axes.
After the review rejected this head, the remaining seven were deliberately
interrupted before source edits (`race-c520df4-interrupted.json`), not counted
as passes. Retry the unchanged full eleven modules/restore domains on an
optimized exact successor instead of spending the remaining matrix on a
rejected head. Original contracts/tolerances stay frozen. Draft coordinator
handoffs are held in main artifacts until the source checks are no longer
running. Last fresh quota 28% at 13:20, baseline 12%/stop 32%; no reset/spending/provider
change. Next checkpoint 13:44, reassessment 13:48.


### 1 October 13:51 UTC handoff - physical product check remains

Source `42d2cc8d8377a303b26e737afebea0adebd895e5` has accepted
[Sol/medium source re-review](https://github.com/malmazuke/unirally-reconstruction/pull/50#pullrequestreview-5380064584).
Both initial findings have a named-fix reply and are independently resolved.
The reviewer re-ran its delayed press and fresh-process restore, and the warm
return, against preserved original expectations with exact events/PCM/state.
All PR conversation/review bodies and line comments were read at 13:46; all
comments are marked Agent, no user instruction or unanswered finding remains.
Task acceptance is still open.

- Required CI run 36870139058 passes changes, macOS and Ubuntu on 42d2cc8.
  Ubuntu's actual Linux sanitizer step succeeds; local macOS CTest/repeatability
  timeouts remain explicit non-passes. Jobs/steps are retained in main
  `hosted-42d2cc8-jobs.json`, not inferred from the job name.
- The full eleven frozen race gates pass with 6,023 fresh-process restore
  boundaries on the optimized source-bound binary (`race-42d2cc8-release/summary.json`).
  Originals, contracts, field sets and per-native-process limits are unchanged.
- The source-head app 41 CTest plus fresh repeatability passes in 4.19 s
  (`app-42d2cc8.json`); the reviewer independently passes 41/41 too. Both v1
  presentation contracts/eight cases pass (`v1-42d2cc8/report.json`).
- v30 product input is rejected before opening the audio device or producing
  a frame (`product-v30-rejection-42d2cc8.json`). A visible v31 scripted run
  completes 1,000 updates, 640,541 source pairs, 944,128 delivered, 845,900 nonzero,
  zero underruns and two navigation events. CUA captured the visible main menu
  in the chat. The report records mapped live keys 0/0: this proves scripted
  device delivery, not physical input/listening (`product-v31-navigation-42d2cc8.json`).

Only the product criterion's held physical input/listening result remains.
The asynchronous readiness question is unanswered. The documented CUA API has
press/release only; 40 earlier taps produced zero sampled holds. Do not change
the game's sampled-input policy or describe scripted/PCM checks as physical
acceptance. When the user is ready, announce a fresh unscripted run, launch the
existing app with v31, `--native-title-menu-audio --updates 1400`, and have them
hold Down and then Up for about one second each while the main menu is visible.
Capture the visible state while the run is alive, retain its command/binary/input
telemetry and ask for the actual music/navigation sound observation. If this
reveals a failure, fix and re-review it inside this same task. Otherwise finish
the product report, current-tip records/CI/comments, merge PR #50 with a merge
commit, fast-forward main and perform the required task-owned cleanup.

The primary worktree, app/lab builds and all main evidence remain available.
The independent review checkout is archived through Codex after its clean
42d2cc8 review; all reviewer captures remain in main evidence. No captured
evidence was deleted. This record-only successor leaves reviewed source
unchanged. Main `artifacts/audio-title-menu-integration/handoff.json` holds its
actual Git head, commands, source checks and live command. The PR is draft;
no acceptance, merge or tag. Fresh usage was 29% at 13:41 from 12%, stop 32%;
no reset, spending or provider change.


### 2 October 2026 Sydney - live product acceptance and integration handoff

At 07:37 Sydney, a fresh unscripted 1,400-update app run used source
`42d2cc8`, record head `c10b333` and the same SHA-256-bound v31 pack/binary.
No input file or fixed controller mask was supplied. CUA raised the actual
window and recorded the main menu with its arrow at OPTIONS in the chat.
This is a retained screenshot observation and console recording, not a video.
The connected Xbox Series X Controller generated 40/40 button down/up events
and 377 gamepad-only nonzero sampled updates. The keyboard count is 0/0;
this run verifies physical controller input, not physical keyboard holds.

The user replied: "Yep I could hear the menu music, plus navigation sounds".
The process exited 0 without stderr in 28.77 seconds. It reported 32 navigation
changes, 709,756 native source pairs, 1,063,304 delivered pairs and 964,841
nonzero pairs at 48,000 Hz. It also reported 265,848 underrun pairs. Preserve
that shortfall as an observed limitation; this is not a zero-underrun or
all-menu-exits continuity pass. The bounded producer returns on selected/idle
menu exits other than HUNTER. The physical input also includes A/B/X, and its
aggregate counters do not timestamp an exit or prove the shortfall's cause.
The listening observation closes only the music/navigation product criterion;
scene lifecycle remains supported by the separate exact HUNTER evidence and
scripted zero-underrun device run, not by this user's listening report.

Main `artifacts/audio-title-menu-integration/product-physical-42d2cc8.json`
retains the exact command, binary/source/input telemetry, visual observation,
user reply and limits. The raw command, result and transcript are
`20261002-0736-physical-command.json`, `20261002-0736-physical-result.json` and
`20261002-0736-physical.log`. Together with the reviewed original/native
comparisons, this satisfies the declared bounded audible title/menu outcome.
The default app remains silent; the opt-in flag requires v31. No full soundtrack,
race audio, arbitrary menu exit audio or live device/thread save is accepted.

The final successor changes records only (tier 3 under D-0008), including the
next ready task. Source, tests, extraction, build rules and frozen expectations
remain identical to independently reviewed `42d2cc8`. Both review findings have
named fixes and accepted re-review. Final-tip hosted checks and all PR bodies/
line comments must be read before `gh pr merge 50 --merge`; no direct main push.
The main ignored closeout records the actual final commit, checks, merge and
main equality after integration, plus task-owned cleanup. All captures and
reviewer evidence already live under main `local/evidence/audio-title-menu/`;
logs live under main `artifacts/audio-title-menu-integration/`. Delete this
task's build output, owned emulator/toolchain copies and primary worktree only
after verifying the pushed branch is integrated. The reviewer checkout is
already archived. Preserve every cited capture and the user playtest checkout.
No capture is moved or deleted merely because the task closes; actual cleanup
paths and byte counts belong in the closeout.

Next outcome: [AUDIO-FIRST-RACE](AUDIO-FIRST-RACE.md), ordinary 1P setup,
DRAGSTER race/result and return with continuous native audio. This resolves the
ordinary exit gap before extending the soundtrack by track. It is preparation,
not a claim or implementation. Fresh weekly usage at this continuation is 29%,
from 12% at claim, stop 32%; the review reserve remains intact. No reset,
spending, provider change, release or M4 milestone tag.
