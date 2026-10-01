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
