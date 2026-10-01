# R-0075 - raw title/menu audio observation and hardware isolation

Status: in-progress coupled experiments for AUDIO-TITLE-MENU, 30 September
2026 UTC. Conditional native driver and seedless cold-prefix results are
recorded below; audible product and capability acceptance remain pending.
PAL ROM SHA-256 `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`;
bsnes `7d5aa1e656b9171524d01b1b22917197d8121cb4`, default adapter options,
Strict synchronization, fresh private save directory per process. The new
`tools/locks/audio-observation.json` and `bsnes-audio-exports.patch` are opt-in;
the default lock/patch and all historical manifests retain their identity. R-0074's
old binary and evidence remain unchanged. The task record identifies source
commits and main's `local/evidence/audio-title-menu/` retains private observations.

## Verified observations

The original 700-frame menu baseline was reproduced twice with the old binary.
The updated laboratory core, both without observation and with it, gives the
same full memory/register sample, A/V and final serialized-state hashes. The
Down variation's observed runs also reproduce the preceding original. No original
expectation was regenerated from native output.

Each raw case ran twice in fresh processes:

| Case | Frames | Ordered events | Raw stereo pairs | Repeated event/PCM bytes |
| --- | ---: | ---: | ---: | --- |
| Start 300-305 | 700 | 3,241,178 | 448,326 | Exact |
| Start 300-305, Down 450-455 | 700 | 3,240,836 | 448,325 | Exact |

The event ABI retains global sequence, elapsed SMP divider ticks, completed
DSP clocks, independent CPU `stepOnce` ticks, next PC, address/value and kind.
The fixed 42-byte layout is little-endian `<QQQQIHHBB`. Raw PCM is signed
16-bit interleaved L/R before float conversion or resampling. A PCM event's
DSP clock is the wrapper's batch-delivery position, not the sample's internal
phase. RAM observations include every effective SMP-origin write, including
underlying I/O RAM; hardware-generated echo writes are reproduced by the DSP.
Port observations happen after the existing coroutine synchronization and include
CONTROL's incoming-latch clears. Capture does not add synchronization.

At every drained frame the integer accounting invariant
`dsp.clock == 2 * completed_dsp_clocks - elapsed_smp_ticks` holds. A deliberate
one-entry buffer fails at frame 0 with 2,547 generated events and 2,546 dropped,
and writes a failed metadata record. Loss is not a successful capture.

A standalone laboratory binary includes only the pinned `SPC_DSP.cpp` and its
headers, a configuration value selecting the original Gaussian interpolation,
and a replay harness. It has no CPU, game program or whole-emulator linkage.
Processing runs, DSP writes and effective SMP RAM writes in their original
sequence reproduces **every raw pair and final 64 KiB APU RAM byte** in both
700-frame cases. The two replay frontiers are 14,346,432 and 14,346,400 DSP
clocks. This is hardware isolation evidence, not native sound-driver acceptance:
recorded event replay is explicitly excluded from the product by D-0009.

The private `driver-first-100` run records SPC instruction boundaries through
frame 100. The 97-100 window reaches 813 distinct instruction addresses in the
uploaded driver's `$0400-$15FF` range. Main's ignored
`driver-observed-97-100.lst` is a reading aid, not shipped content or proof of
all reachable driver code. `driver-all-addresses.lst` decodes every possible
start in that range; operand starts and data are present and must not be treated
as verified instructions. Its printed registers are the listing core's state,
not dynamic registers; dynamic YA is in instruction events.

## Static readings and hypotheses

Before capture design, read main `artifacts/static-map/bank-82.lst`, especially
`$82:8000-8160` (R-0074). Source readings from pinned `sfc/smp/timing.cpp`,
`sfc/smp/memory.cpp`, `sfc/smp/io.cpp`, `sfc/dsp/dsp.cpp` and `sfc/cpu/io.cpp`
explain the observation sites above. A bounded fresh Astra/medium consultation
confirmed the DSP lead/idle-debt hazard and proposed the accounting invariant;
its advice is not gameplay evidence.

The ROM record traversal from `$10:8000` reaches record 50 at file offset
`0x9C6B0`, with the first word `0x115D` and destination bytes identifying `$0400`;
records 53 and 57 begin at `0x9EA9A` and `0xA0265`, with destinations `$1600`
and `$1D00`. These are directory readings only. The transport's length versus
six-byte header relationship and the full code/data boundary still require
matching the actual transfer writes; no extraction rule or executable range
is admitted to a content pack.

The observed driver's command dispatcher reads ports at `$0572`, selects a
return address from `$0607/$0614`, and acknowledges at `$0593`. Its timer
updates at `$0448-$0484`, voice update at `$0915`, score dispatch at `$0948-$095B`
and DSP output at `$1312` are readings from the ignored dynamic listing.
Their semantic meanings, exact cycle costs and score controls remain provisional.

## Reproduction and limits

`tools/unirally_lab/reference/worker.py` has an additive experimental
`--audio-out NEW_DIRECTORY`, `--audio-capacity N`, `--audio-instructions`, and
repeatable `--audio-ram-frame N`. It requires a cold uninterrupted run, run-ahead
OFF and the updated observation exports. Existing API-1 reference cores remain
loadable when these options are absent. Full commands and identities are retained
in `capture-commands.json` and `driver-trace-command.json`; build and replay logs
are in main `artifacts/audio-title-menu-integration/`. Outputs stay private.

The replay harness compiles against the pinned files under main
`local/emulators/bsnes/bsnes/sfc/dsp/`; its source is
`tools/unirally_lab/reference/audio_lab/replay.cpp`. `SPC_DSP.cpp` declares
LGPL-2.1-or-later and snes_spc 0.9.0; its notices stay in that checkout. This
laboratory experiment imports no external source into the product and makes no
license compatibility claim. A product dependency still needs the adoption
record, notices, license and build/link/source arrangement D-0009 requires.

Initial observation export compilation failed twice on private/register naming
assumptions, then built successfully after inspecting the actual interfaces.
Seven new loss/order/layout/clock tests and the existing 33 reference-tool tests
pass. Full native regressions, hosted CI, independent tier-1 review, musical-loop
coverage, native driver/sequencer, content extraction, native audio save/restore,
SDL output and audible live checks remain pending. Reference restore audio stays
excluded under D-0001. Echo-shadow and alternative options have no replay claim.

Hosted CI on the first experimental commit failed on the new test's import path
and 56 historical manifest checks after the default patch identity was changed.
The correction isolates audio in its own opt-in lock/checkout/patch and restores
the original default files byte-for-byte. Historical identities and comparison
expectations are not relaxed. The test uses the repository tooling import path.
The new CI result is recorded in the task once available.

## Timed producer observations, 1 October 2026

ABI 2 optionally adds X/SP/P immediately after SPC instruction events. ABI 3
adds up to 64 watched CPU instruction addresses with A/X, Y/SP and DB/E/P,
plus one unforced frame-leave H/V record and CPU/SMP/DSP frontier metadata.
All versions retain the 42-byte event layout and old captures remain supported.
The additive worker option is repeatable `--audio-cpu-watch 0xADDRESS`.
Instruction observations are laboratory reading aids, not executable content.

Canonical `menu-base-cpu` and `menu-base-repeat-cpu` repeat every event/PCM byte:
7,486,497 events and 448,326 raw pairs. Baseline and Down 450 preserve the
earlier complete sample, A/V and final-state digests. Down 449/450/451/452,
Up 450 and Down release/retrigger at 450/470 have fresh timed observations;
commands and identities are in `cpu-capture-commands.json` and
`cpu-timing-audit.json`. These additional cases are diagnostic, not yet frozen
native gates or independent withheld evidence.

The static map reading `$80:B178-18C` queues `$087F` then `$0203`; the same
values are observed in `$82:806D`'s A register and the ordered port 2/3 writes.
For Down 450 those writes occur at CPU ticks 191,399,968/974 and
191,412,094/100. Down on consecutive frames changes the second transfer's
latency: its store-site entry is 2,078, 12,126, 2,630 and 8,422 CPU ticks after
the first store-site entry for frames 449, 450, 451 and 452. Up follows another
menu branch and also changes arrival. A constant frame-relative navigation
offset is rejected by these observations. These are elapsed CPU `stepOnce`
counts, not an asserted common CPU/APU scheduler timestamp.

Frame-leave CPU ticks advance by 425,568 in the tested late PAL frames, while
SMP/DSP frontiers vary with the driver's synchronization site and lead. A
frame can leave in the middle of CPU `step<Clocks>` before its pending SMP
clock subtraction; consequently the raw CPU/SMP balance relation has an
in-flight residue. Source reading: pinned `sfc/cpu/timing.cpp`, especially
`step` and `scanline`. No forced synchronization was added to normalize it.

Implementation decision: recover a reduced semantic producer/transport timing
model, including queue readiness, poll phase, raster timing and relevant
interrupt/DMA costs. A common cold-origin DSP horizon can define comparison
packaging before native gates, but exact command arrival and driver consumption
still need prediction from native state. No native timing prediction, complete
score loop, content pack, SDL audio or native restore pass is claimed.

Ten focused observation tests now pass, including ABI compatibility, watch
bounds and unforced frame-frontier retention. Hosted synthetic CI passed on
`91c8f05`; it does not validate these later ABI changes or private raw audio.

## Conditional semantic clock experiments

The filtered ABI-4 SMP watch option `--audio-smp-watch 0xADDRESS` retains
10/11 register pairs only at selected instructions, with all hardware, RAM and
port events unchanged. `filtered-integrity.json` proves exact shared events,
PCM, state/A/V and final-state digests against the earlier baseline and Down.
Four observation binaries and the original/full/selective traces remain private.

`audio_lab/transport_clock.py` reads no ROM or instruction bytes. Its native
semantic bus-cost functions derive from static `bank-82.lst` `$8035-$806D` and
pinned CPU memory/timing code. Given observed function entries and readiness,
it predicts every store boundary in 38 retained transfers across eight traces.
This is conditional timing recovery, not a native cold command producer.

The wider prologue experiment initially misses 85-95 polls per case. A new
trace watches the NMI vector `$00:8587` and hook/return boundaries. All 450 NMI
handlers match source-derived register preservation, settled-logo processing,
palette counter/phase arithmetic, four-colour writes and restoration, including
per-scanline refresh. The first raster/pipeline model leaves six failures:
a JSL's last eight-clock stack write can see the NMI edge after its final-cycle
interrupt test, so the target prologue must still recognize that pending edge.
The corrected model predicts all 2,827,700 receiver-poll access clocks across
eight captures, without taking captured interrupt timestamps as inputs.
`receiver-poll-nmi-conditional-v2.json` retains counts and source hash; earlier
failed reports remain. Original function entry clocks are still inputs.

The source reading is static `bank-80.lst` `$8587-$8598`, `$F622-$F644`,
`$FA60-$FAC9`, `$B0F0-$B0F9` and the frame wait `$FADF-$FAF4`, plus pinned CPU
`irq.cpp` and `timing.cpp`. It remains limited to PAL non-interlace, version-2
CPU, slow ROM, no H/DMA inside these paths, settled downward logo and the tested
menu history. NMI enable and loading/producer timing are still dependencies.

The Down cue reads its final control at score `$17D3` and enters `$126A`,
clearing voice-7 tag `$010F=FF` at SMP tick 18,771,648, then writing `$5C=80`
at tick 18,771,712. This is a bounded terminating effect in the 700-frame domain.
The first handler-table audit erroneously added one to SPC RTS targets;
`score-control-dispatch-corrected.json` retains actual targets. Corrected
score audits distinguish top-level dispatch reads (return `$094B`) from
parameter reads using dynamic stack state. Raw bytes >=128 in parameter reads
are not counted as controls.

The 4,000-frame menu-only exploration moves at 450/1500/2500/3500 before idle
expiry. It contains only initial music start and those four effect starts, with
11 instrument selections and 30 distinct dispatched controls. The unsteered
4,000-frame exploration enters the demo and cannot establish title/menu music
coverage. Neither is a frozen native gate or complete soundtrack assertion.
Eleven observation and eight conditional-clock unit tests pass; full native
audio and independent capability review remain pending.

## Data-only score parser and isolated hardware adapter

`audio_lab/title_menu_score.py` is a laboratory semantic parser. With observed
voice-update modes and command boundaries supplied, it predicts all 905 baseline,
933 Down and 11,490 long-steered score reads exactly, comparing voice, pointer
and byte in order. No captured score read is a parser input. The 30 long-run
controls include calls/returns, counted loops, instrument copies, inline pan/volume
parameters and nine random selections. This is conditional algorithm evidence;
it does not establish cold scheduling, voice arithmetic or ordered DSP output.
`score-parser-conditional.json` retains source/data hashes and counts; two failed
reports retain the initial silent-program data assumption and reversed RNG order.

Source readings are private `driver-observed-down-700.lst` and the bounded
`driver-control-readings.lst`. `$074B-$07FF` selects split music pointers and
copies 252 bytes into instrument records 1-36, seven bytes each; record 0 stays
in place. The silent program's copy root is native voice state `$0101`, including
freshly reset effect tags, rather than another ROM range. `$13DA-$1406` rotates
`$EF,$EE,$ED,$EC` in that order, so the four random-state bytes form a big-endian
integer despite their increasing memory addresses. Feedback and output use EC.
The corrected rule predicts all nine observed random branch choices.

`score-data-boundary.json` identifies nominal data ranges ROM `$09EAA0` (621
bytes, APU `$1600-$186C`) and `$0A026B` (2,200 bytes, APU `$1D00-$2597`). All
1,968 distinct retained long-run score-byte addresses lie inside these nominal
ranges, and neither intersects executed SPC instruction PCs in the full 700-frame
capture. Reading their tables/control structure and this bounded dynamic evidence
supports extracting data only. The six next-record bytes transmitted past each
nominal record are excluded. No whole-game code/data boundary is asserted; pack
revision and native data integration remain pending.

The six pinned DSP-only sources are copied unchanged under `third_party/spc_dsp`,
with per-file hashes, original notices and the GNU LGPL 2.1 text. Its README
records the separate shared-library arrangement and source/build recipe. The
adapter supplies only Gaussian interpolation selection and a C hardware interface;
no CPU or original sound program source is linked. It has shared RAM/echo, exact
DSP phase and native hardware snapshots. This is the D-0009 dependency adoption
experiment, not a legal compatibility or binary-release claim.

The shared-library diagnostic reproduces all 448,326 baseline and 448,325 Down
raw stereo pairs and complete final RAM exactly, using the original unchanged
event streams. Commands are in main `artifacts/audio-title-menu-integration/`
`dsp-shared-replay.json`. Synthetic hardware tests pass clock batching, save
non-perturbation, fresh-instance restore and shared echo-memory continuation,
including nonzero PCM. The first adapter filled its two-sample output buffer,
causing upstream to switch to its extra buffer before `sample_count`; a four-sample
capacity fixes that interface error without editing upstream. The failed report
is retained. This is DSP hardware evidence, not native driver PCM acceptance.

Eleven capture, eight conditional-clock and seven score-boundary unit tests pass.
The native hardware test passes locally; new hosted checks and independent review
are still pending. The product remains silent and the task remains incomplete.

## Native parser and voice calculation experiment

The C++ score parser predicts the same 13,328 ordered original score reads;
`native-score-conditional.json` retains commands, data/source/runner hashes and
exact byte comparisons. Its command boundaries and voice-update modes are
observations. The authored native score check covers calls, counted loops,
unsigned zero duration, object-state continuation and invalid stack rejection.

Readings in `driver-observed-down-700.lst` and the bounded
`driver-voice-readings.lst` identify pan, note alternation, note/pitch slides,
fractional pitch modulation and the seven-byte software envelope. The native
calculation reproduces all 5,139 baseline and 5,155 Down complete arithmetic
state rows, initially using original pre-update states
(`native-voice-conditional.json`). Joining it to the score parser reproduces
all 10,294 rows from native initialized state and identified data, with no
original voice-state seed (`native-sequence-conditional.json`). Supplied
command/update boundaries and update counters still make this conditional.

The combined native state predicts every six-register voice-output group:
74,088 baseline, 74,250 Down and 481,080 long-steered register values are exact,
629,418 total (`native-register-conditional.json`). The output boundary is also
supplied in that experiment. It neither predicts write clocks nor feeds the
native DSP; raw PCM, command/timer scheduling and full restore remain pending.
The unchanged original register values are compared without tolerances.

Pitch data readings are 85 low bytes at APU `$1407` and 85 high bytes at `$145C`;
the exact ROM source offsets are in `native-voice-conditional.json`. Sample
fraction and transpose bytes come from the already identified sample headers,
not an APU snapshot. The lookup adds `floor(base * fraction / 256)` modulo
65,536. Pan and all envelope parameters retain byte wrap and the original
division quotient, including divisor zero. Negative modulation with amount
zero adds `$FF00`; a signed `-0` replacement would be wrong. Authored integer
boundary checks exercise these cases and the pitch-data bound. No claim is made
for unobserved controls, scripted envelopes or the rest of the soundtrack.

Hosted macOS/Ubuntu checks pass for dependency/parser head `d60761a`, run
36799359085. The C++ voice candidate has not yet received hosted or independent
review. The product remains silent.

## Elapsed work and boundary extension

The semantic voice-work model predicts every elapsed SMP interval from arithmetic
entry `$09B4` to command polling at `$0992`: 5,139 baseline and 5,155 Down rows.
`native-voice-work-clock.json` records the C++ runner/source and exact integer
comparisons. The costs are native semantic work groups under the pinned default
wait states, not instruction fetching or a captured timestamp table. Pre-update
state remains an observation in this experiment. Note-space slides, nonzero pitch
convergence and scripted envelopes remain outside this timed domain, and are
rejected by the timing helper. The main driver schedule is still unrecovered.

Rapid Down at frames 450 and 453 restarts effect 3 before its first termination;
native initialized score/voice state predicts all 74,304 compared per-voice DSP
values. The original main-menu confirm/cancel path (A 450, B 650) uses effects
2/4 and keeps the music running. The initial native comparison rejects a pitch
lookup outside 0-84. `native-register-boundary-failure01.json` retains that failure.

A repeat with pitch-function watches preserves state/A/V digests and identifies
note indices 85, 90, 95 and 96 (`pitch-data-alias-audit.json`). The low-byte lookup
at `$1407+index` overlaps the high-byte table; the high-byte lookup at `$145C+index`
also reads bytes at `$14B3-$14BC` used as code by the original. This is dynamically
observed code-as-data use. The implementation decision is to supply decoded
numeric pitch values only, with a bound of 97 entries. Those bytes are never
fetched, decoded or executed by native code, and no uploaded program is supplied.
Extraction must preserve this explicit alias provenance rather than describe all
source bytes as exclusively data. A pack rule is not yet implemented.

With the identified numeric lookup extension, confirm/cancel predicts all 112,980
per-voice DSP values exactly. `native-register-boundary-conditional.json` holds
both new cases; the earlier three-case report stays unchanged at
`native-register-conditional.json`. The authored bound check now rejects the first
unidentified index 97, preserving its purpose as the identified domain grows.
This does not establish write clocks, original scene stop/restart or raw PCM.

## Native driver work and timers

Sequential pinned readings in `driver-clock-readings.lst` and
`driver-score-clock-readings.lst` are the reading source for driver polling,
score dispatch, voice reset and output. They are private ROM-derived listings.
The native score work candidate reproduces all 16,467 completed voice calls in
each 700-frame original and 106,760 in the 4,000-frame steered original repeat.
`native-score-work-clock.json` and its `-extended.json` report bind the unchanged
expected values, native source and commands. Inputs are still observed consumed
commands, update modes and counters. No whole cold schedule is established.

The later controls exercise random dispatch and nonzero pitch convergence.
The full 1,800-frame prefix, captured cold without a state seed, identifies the
convergence branch's elapsed work (`convergence-clock-reading01.txt`). Earlier
failed timing reports remain retained. Arithmetic from observed pre-update state
also matches all 33,394 complete long-repeat rows
(`native-voice-conditional-extended.json`). Scripted envelopes stay outside the
recovered domain. Neither work function fetches instructions nor uses an observed
timestamp table.

Native timer hardware starts from zero divider/counter state. Its two 128-tick
and one 16-tick divider stages, falling-edge target increments, four-bit read/clear
and CONTROL behavior follow pinned bsnes `sfc/smp/timing.cpp` and `io.cpp`.
Preserve that implementation's inverted timer-2 reset condition explicitly.
`native-timer-conditional.json` matches all 74,965 baseline, 74,517 Down and
440,134 long-repeat reads, 589,616 total. The expected values are inferred from
the original accumulator immediately after timer ADC, its recorded prior RAM
value and preceding CLC. IO-write and read clocks remain supplied boundaries.
This is hardware-phase evidence, not recovered driver-loop scheduling.

Native score state now accumulates key-on/off masks and tracks the original
voice output-enable flag. `native-driver-work-conditional.json` matches all nine
music/effect setup durations, 79,488 output-call rows (six write offsets plus the
next poll, or an inactive return) and 14,073 key-mask values. The domain is the two
700-frame cases and full 1,800-frame prefix. Observed command, update and output
entries remain inputs. Authored timer phase/zero-target/wrap/continuation and
nonzero score/envelope work continuation checks pass; full audio snapshots,
exact cold PCM and product sound remain pending.

## Original soft-reset sound lifecycle

Read R-0064 and bank-80/bank-83's static listings before designing the capture.
The cold HUNTER code route (`hunter-code-audio.script.json`) runs 1,850 frames.
CPU `$80:8858` is reached at master clock 559,931,912. At clock 572,216,704 the
CPU writes FF to port 0, which the driver acknowledges at SMP tick 55,136,364.
The `$0487-$04B7` reading and `hunter-reset-stop-sequence.json` show timer-paced
master-volume subtraction by eight, then key-off FF, CONTROL B0 and DSP FLG E0
at SMP tick 55,249,128 / DSP clock 27,624,576. This is a loader/reset request,
not a score music-stop command.

The game jumps through its reset path without resetting the laboratory DSP
clock. Initialization writes FLG F3 at SMP tick 55,795,198, then FLG 33 at
58,055,056. A fresh title-music command is consumed at 58,071,522
(`hunter-code-audio.consumed-commands.json`). Original music continues during
the ending pages. These facts cover this ROM and pad schedule only; no native
scene lifecycle, loader duration or full audio acceptance is claimed yet.

## Native post-IPL initialization and assembled driver loop

The reading is `driver-clock-readings.lst`, `driver-observed-down-700.lst`
and `instruction-costs-down.json` in the canonical evidence directory.
The native `audio_driver` files express the port handshake, timer counters,
score/update/output order and sample loader as named C++ functions. They fetch
no opcode bytes. The identified 29-pair DSP initialization table at SPC
138F/13AD is expressed as hardware constants; uploaded executable bytes are
not an input. The source remains a conditional laboratory component.

On the same PAL ROM/core/options and pad scripts, the post-upload loop computes
all 1,438,920 completed DSP writes and SMP port reads/writes exactly in the two
700-frame full traces and the 1,800-frame steered trace. The 4,000-frame
steered repeat independently matches 1,873,047 such rows. A port access can
yield to the CPU at its SMP tick before its observer row is emitted, so these
reports exclude the last observed SMP tick on both sides. The retained
`native-driver-loop-end-frontier-failure01.json` shows the extra pending read
when an inclusive frontier was first used. No earlier expected row changed.

`native-driver-boot-conditional.json` extends the computed driver from SPC
0400, at the observed post-IPL tick 1,725,604, through directory construction,
42,498 BRR bytes, fraction/transpose parameters and the ordinary loop.
All 2,084,772 completed DSP/port/selected RAM rows match across those three
full traces. The RAM projection comprises the native clear and sample data,
not all internal score fields. The first harness failure and the subsequent
single two-tick compare-access offset are retained separately. Correcting
that access within the ten-tick compare preserves its total work.

`native-driver-pcm-conditional.json` couples this native initialization and
loop to the DSP-only adapter, starting that hardware and its RAM from zero.
At the predeclared 14,300,000 DSP-clock horizon, all 446,875 raw stereo pairs
match in each full trace (1,340,625 comparisons, with the longer case's early
prefix equal to Down). The baseline prefix SHA-256 is
`34cf53c293f10841b4b1607f328f53e19bce71b07409e3c23e1667353b135141`
and Down's is `d557f0570606362cc1638b8cba9326c423a5fb64cce7a8ec01f2d26fcd940bed`.
Native sample writes and DSP register writes are computed; no captured DSP
events, DSP snapshot or original post-entry RAM writes enter this check.

The final IPL entry tick and CPU writes at their observed SMP boundaries
remain explicit conditions. These checks therefore establish conditional
native driver work and PCM, not a cold CPU producer, product audio or task
acceptance. Calls can run past the diagnostic closing horizon; resumable
pending phases and full native audio serialization remain open. The app
still has no audio output, pack v29 is unchanged, and independent capability
review/regressions remain required.

## Stop fade, cold CPU observer and frozen dispatcher refactor

The native stop path now handles the observed bulk FF request, acknowledges
it, waits for timer-2 pulses and subtracts eight from both master volumes.
It emits key-off FF, CONTROL B0 and FLG E0, then returns to a declared IPL
boundary. `native-driver-stop-conditional.json` matches all 830,187 completed
projected rows through the original six-tick jump at SMP 55,249,134.
`native-driver-stop-pcm-conditional.json` matches all 863,438 raw stereo pairs
through the fixed DSP horizon 27,630,016 in the muted interval, before the
first later initialization write. Its prefix SHA-256 is
`87f9e8b5bf53f11ae32466195679df82edd5bb0c7b4488f39957f7aaa5005582`.
The first stop comparison included later IPL polls that are still unimplemented;
`native-driver-stop-includes-ipl-failure01.json` retains those unmatched rows.
The bounded result is not a complete native reload/restart pass.

The fresh 1,850-frame HUNTER repeat, `hunter-code-audio-clock`, preserves every
original state/A/V/PCM/final-RAM digest (`hunter-clock-repeat-integrity.json`).
It adds watches at driver entry, ordinary loop and stop entry. Original CPU
write boundaries and final IPL driver entry remain conditions in native tests.

Optional observation ABI 5 adds complete CPU instruction/register boundaries.
Its core SHA-256 is
`3483e8bcd27b0709758f186d97474c38fee62d66fb43a5b3d2ca51087f4a4539`.
`cold-cpu100-integrity.json` compares a fixed 100-frame cold script under ABI 4,
ABI 5 tracing off and ABI 5 tracing on. State, video, callback audio, raw PCM,
final WRAM/SRAM/APU RAM and serialization metadata agree. All 903,135
normalized old kind-1-through-9 events also agree exactly. The full CPU capture
contains 1,585,211 instruction boundaries at 636 distinct PCs. These are
original research observations, not native product timestamps or acceptance.
The static bank-80 reset/loading and bank-82 sound listings were read before
the capture; the default reference patch/lock remains unchanged.

Under D-0003, `score-dispatch-refactor-frozen.json` retains the pre-refactor
source hashes and reports. The 105-line voice dispatcher is split into note
initialization and sequence, pitch, instrument and mix controls. The renamed
`per_note_volume` flag reads the inline volume byte already identified in the
score format. Repeated native startup checks still match all 2,084,772 rows,
the extended score check all 106,760 voice-call durations, and the three
14,300,000-clock PCM prefixes all 1,340,625 pairs. The stop row/PCM checks
also remain exact. Four focused native tests and 13 capture-tool tests pass;
the two dispatcher source files pass the 80-line function-size check. Full
cold timing, resumable phases, pack, native save, audible app and independent
review remain pending.

## Native IPL protocol and conditional complete PCM

`ipl-protocol-reading.lst` records the pinned hardware IPL reading from
bsnes target-libretro/resources.hpp and processor/spc700 timing primitives.
The native implementation uses named pending handshake phases with byte
subtraction, page/counter wrap, duplicate end comparison and port dummy reads.
It fetches no instructions. Uploaded executable bytes are ignored; only
identified score data is retained from those transfers in DSP RAM. Native
score logic uses the separately bounded data inputs already described above.

A fixed 100-frame original repeat adds watches for those IPL phases.
`cold-ipl-observation-integrity.json` verifies all 903,135 normalized old events,
all non-process/non-observation sample fields, raw PCM and final APU RAM
remain exact. Only process metadata, observation row count and output path are
excluded from the sample-record equality. The core/ROM/script/options agree.
The first 100-frame native comparison matches all 321,790 projected rows.

`native-cold-ipl-primary-conditional.json` starts native SMP work at tick zero
and computes driver entry 1,725,604. All 620,517 baseline, 620,012 Down and
1,161,751 long-prefix rows agree, 2,402,280 total. The projection includes every
completed port read/write and DSP write, IPL direct-page clears, identified
score uploads and the existing driver clear/sample writes. Other internal RAM
and executable uploads are excluded. Original CPU port writes at their observed
SMP boundaries remain conditions; there is no cold native CPU producer claim.

`native-cold-ipl-restart-conditional.json` covers the HUNTER lifecycle: all
1,407,361 projected rows agree, including native fade, IPL transfer, retained
timer/DSP clocks and repeated driver initialization. The restart entry is
computed as SMP tick 55,795,112. The expected original closing tick remains
exclusive, preserving the pending-port-access boundary described earlier.

`native-cold-pcm-conditional.json` compares each complete original raw capture,
with the horizon fixed from that capture's sample count before the native run.
All 448,326 baseline, 448,325 Down and 1,153,125 long-prefix stereo pairs match,
2,049,776 total. `native-cold-pcm-restart-conditional.json` also matches all
1,185,137 HUNTER pairs, including the fade, muted reload and restarted title.
It retains the original SHA-256
`1a5dc984ef02f66c3897cce84348a6ea301b6a34f98082af9b6a98f5e1a7ce87`.
No original DSP event/snapshot or post-entry RAM write is an input. CPU writes
and their timing still are, so this remains conditional SMP/DSP evidence.

The authored sender transfers 258 bytes across a page/counter wrap and checks
identical continuation events/entry clock from every exercised pending IPL
phase. Five focused native checks pass. This object-level continuation is not
a complete fresh-process audio save pass. The native driver still runs whole
iterations and boot calls; `driver-pending-refactor-frozen.json` records the
source and complete conditional reports before converting those pending phases.
Cold CPU work, pack, full native save, audible app and independent review remain.

## Cold native CPU prefix and coupled first write

The reading sources are main's static bank-80 listing 91D1-9311, B612-B625
and A09A-A0FC, and bank-82 B183-B1AD, B1DB-B20A, B296-B29E, B2B0-B2DC
and 807E-808F. Pinned CPU memory/timing and WDC65816 call/return primitives
provide bus-step order. The complete CPU observer has 431,760 instructions at
266 distinct PCs before the first sound write. Those are research observations;
no opcode reader or CPU interpreter enters the native timing model.

The native C++ work clock counts ROM/WRAM/MMIO bus accesses, width/register
work and calls, with fast-ROM selection, split reads and version-2 PAL refresh.
Named cold work functions account for reset, PPU-control work, clearing WRAM
and 32,768 VRAM words, frontend storage, and the two initial asset uploads.
They model work elapsed for the separately native frontend; they do not write
game CPU memory or execute those instructions. The supported asset-clock domain
is the identified 32-byte palette and 8,192-byte tile upload without a source
bank wrap. Other sizes fail explicitly. Later producers and interrupts remain
unrecovered; this is not a general CPU model.

`audio_cpu_boot_tests` compares twelve fixed original milestones, from reset
entry 186 through first audio write 12,284,978, without a supplied entry clock.
All match. The private prototype's first two failures are retained: the first
omitted the second byte of the word VRAM write; the second misclassified the
loader's saved word registers/local call. Corrected work preserves all earlier
matching milestones. `native-cold-cpu-prefix-conditional.json` records the
prototype source and results; the C++ check additionally binds the compiled
source via the coupled report below.

`native-cold-cpu-ipl-prefix.json` couples this native work to the zero-tick IPL
phases through the first CPU sound write. CPU scanline synchronization occurs
on its exact two-clock tick before the enclosing bus step updates the CPU/SMP
balance. SMP resumes until a port read yields to the CPU; the read stays pending
until CPU work catches up. All 65,500 projected port read/write rows, including
CPU and SMP clocks and values, match exactly. First CPU FF is at master clock
12,284,978 / SMP tick 1,183,725. No original timestamp, executable, port input
or event stream is supplied. The first coupled failure differed in one row's
CPU stamp; `native-cold-cpu-ipl-prefix-failure01.json` retains it. The native
local-call work now reads both target bytes before idle/stack stores, matching
the pinned primitive. No expected row changed.

This closes only the cold prefix through the first write. Complete upload,
command producers, resumable driver/CPU work, full audio save, pack, audible
app, regressions and independent review still remain. Earlier complete PCM
results continue to depend on original CPU writes and clocks.

## Resumable native driver work, 1 October

Implementation decision: the semantic driver also supports explicit,
pointer-free continuation phases. Each phase computes a bounded hardware-work
plan; pending port reads are retained when CPU synchronization interrupts an
access. Sample transfer, stable command polling, timer reads, voice passes and
stop no longer require a suspended C++ caller stack in this mode. Plans contain
natively computed work and values, not captured events or original opcodes.
The synchronous laboratory form remains available for frozen comparisons.

Verified in the same PAL ROM/core/options and four pad domains above:
`native-driver-pending-conditional.json` and
`native-driver-resumed-conditional.json` match all 3,809,641 selected original
SMP/DSP/RAM rows. The resumed run constructs a new driver every 6,371 SMP ticks,
restoring 31,324 snapshots across the four domains, including both sides of the
HUNTER restart. `native-pending-pcm-conditional.json` and
`native-pending-pcm-restart-conditional.json` preserve all 3,234,913 complete
raw stereo pairs. The corresponding `native-resumed-pcm-*` reports preserve
the same pairs with repeated object reconstruction. Evidence is under main's
`local/evidence/audio-title-menu/`; expectations were frozen in
`driver-pending-refactor-frozen.json` before editing.

Seven focused native audio checks pass. The new authored driver test interrupts
every port read once, restores a new driver, and checks the complete IO stream,
timer phase, score state and stop. No full audio save is established: the CPU
transport, DSP history and output queue still need one canonical owner and a
fresh-process comparison. Original CPU writes remain conditional inputs here;
cold upload/producer recovery and audible product integration are still open.

## Seedless native cold uploads and driver ready, 1 October

Verified: `native-cold-cpu-upload.json` matches 164,801 projected accesses
from power-on through all three IPL transfers' final jump request. Both clocks
and all non-executable values/order are exact, ending C=17,907,842.
`native-cold-cpu-driver-ready.json` extends to 169,905 accesses including the
29 initial DSP writes, incoming-port clears and selected voice RAM, ending
at CPU P2=128 (C=18,125,718, S=1,746,507). Native CPU work, native IPL and
resumable native driver start at zero; no observed clocks/events, snapshots,
RAM seed or executable payload are inputs. The data inputs are identified
resource lengths and the two score transfers, including their six trailing
resource-header bytes. CPU work reads no original opcode or ROM bytes.

Implementation decision: the executable's 4,445 transfer bytes are discarded
by native IPL and supplied as zero in the CPU work experiment. Their C1/R1
values are declared opaque in the frozen projection, while every access,
clock and handshake value remains required. Version 1 accidentally used the
RAM-store PC FFE4 for opaque R1 reads; the hook's advanced read PC is FFE0.
The original projection/failure remain retained, and version 2 corrects only
that marker under the already-declared domain. The first native attempt also
omitted SMP port-write synchronization. Pinned `sfc/smp/io.cpp` requires it
for writes and reads; adding it makes the two-clock upload comparison exact.

The driver-ready comparison also exercises the pinned noncommunicating-SMP
lead guard (`768 * 24 * 24,000,000` balance units), yielding at S=1,746,422
before the timer step and resuming without losing its pending work. The
authored restore test preserves that deferred timer step. Seven focused audio
checks pass; sliced-driver events/PCM remain exact after the clock extension.
Evidence and frozen projections live in main `local/evidence/audio-title-menu/`.
Source readings: static bank-82 $808F-$8160, bank-80 $A100-$A10E and pinned
CPU/SMP memory, I/O and timing code. CPU functions still have an uninterrupted
caller stack. Full sample upload, cold music/command producer, complete native
fresh-process save, pack/device audio and capability review remain open.

## Seedless 64-slot sample transport, 1 October

Verified: `native-cold-cpu-samples.json` matches all 713,909 projected events
from power-on through the CPU sample-loader's final P2=128 request
(C=41,360,956/S=3,985,343). Native CPU selects 21 nonempty slots from the
identified 64-byte table at ROM offset 0x1FCF5, traverses typed resource lengths,
sends metadata and 42,498 BRR bytes, and waits on native SMP acknowledgments.
SMP directory/fraction/transpose and BRR writes, all port accesses and both
clocks/order/values match. The computed caller return C=41,361,676 independently
matches the original CPU trace at $80:A119. No observed clock/event/RAM seed or
executable payload is used. The same opaque executable-payload projection is
retained; score, sample and handshake values remain exact requirements.

The initial comparator reported 17 additional events from the callee's return
work beyond the previously frozen request endpoint. Its failed report is
retained; comparison now applies that frozen CPU endpoint to emitted output,
without changing any expected row. `cold-cpu-samples-frozen.json` predates
native sample implementation. The original 62,530-pair prefix through caller
return is all zero and is not claimed as audible evidence. Static reading:
bank-82 $82A5-$8336 and bank-80 $A112-$A115. Seven focused audio checks and
readability pass; native-symbol metadata remains unchanged. Remaining cold
frontend enqueue/dispatch work, CPU continuation, complete save/device audio
and independent capability review remain open.

## Seedless command queue and first vertical-blank wait, 1 October

Verified: `native-cold-cpu-first-command.json` matches all 713,929 projected
events from zero through $80:FAE3 (C=41,365,774); the initial queue words
$0102/$08FF/$077F are music1/parameter2, effect-gain255 and music-gain127.
`native-cold-cpu-first-vblank.json` extends to 714,357 exact events through
$80:FAF1 (C=41,587,324), including the second dispatch. CPU polling uses
native queue/acknowledgment state and PAL raster timing, without observed
producer entry times. Music gain remains queued at the endpoint. Frozen
original projections predate each extension. The same opaque executable
payload domain is retained; all clock/order/non-executable values remain exact.

Static routine readings: bank-82 $8000-$807D, bank-80 $A119-$A169 and
$FADF-$FAF1. Pinned `sfc/cpu/io.cpp` supplies HVBJOY: vertical blank is
`vcounter >= ppu.vdisp()`, with no NMI edge delay. This domain has no enabled
NMI/DMA. An authored ring test covers full-drop, busy acknowledgment, FIFO,
wrap and alternating phase/header-before-parameter ordering; it is not dynamic
original boundary evidence. Readability and native map checks pass. Later
frontend work, CPU continuation, complete save and device audio remain open.

## Seedless frontend work and graphics DMA, 1 October

Verified: the first frame's native scene work returns at C=41,588,122, with
phase31 and equal zero targets, matching the independent CPU observation.
`native-cold-cpu-first-frame.json` preserves all 714,357 preceding projected
events. `native-cold-cpu-base-palette.json` extends to 715,354 exact events
and C=42,020,908, with all three initial audio commands delivered. Native
CPU models OAM544/palette216 single-channel DMA's one-cycle activation, bus
alignment and split byte reads. `native-cold-cpu-nintendo-load.json` extends
to 717,169 exact events and C=43,737,552 before fade, using identified raw
graphics lengths/cursors for assets31/80/74. Original projections and CPU
endpoints were frozen before each extension; no observed producer clocks
enter native execution. No NMI/HDMA domain is yet claimed.

Source readings: static bank-80 FAF5-FBC4/9318-933B/A8A8-A8D3/B08C-B0CA,
bank-83 99F6-9A1D, bank-82 B183-B1AD/B1DB-B20A/B296-B2DC and pinned
CPU DMA/timing primitives. R-0054 identifies graphics pieces and OAM/base
palette sizes. Raw graphics metadata is retained privately in
`cpu-graphics-assets.json`. No image/executable bytes are needed for this
work-clock diagnostic. Both focused CPU checks and readability pass.
Remaining cold scene work, canonical CPU continuation/full save, audio pack
and device playback still block capability acceptance.

## Joined seedless PCM through Nintendo/title load, 1 October

Verified: `native-cold-cpu-nintendo-pcm.json` matches all861,545 projected
events, C=96,917,470 and every145,925 raw stereo pair through D=4,669,600.
This includes first nonzero pair63620 and uses native CPU/IPL/driver work
from zero, not original CPU write inputs. Nintendo fade/hold/fade work comes
from static $80:9869-$80:98A3/$80:B0CC-$80:B0DB. The DSP-only diagnostic horizon
clips output packaging and does not advance or seed CPU/SMP work.

`native-cold-cpu-title-pcm.json` extends to all872,506 projected events,
C=106,373,342 and every160,150 raw pair through D=5,124,800, ending before
NMI enable. Native A16A initialization and identified graphics assets27/78/72
include asset72's LoROM cursor wrap. Static readings: bank-80 A16A-A1F1
and F55F-F5B8; bank-82 B296-B2AB supplies the wrap work.

The first attempt wrongly assumed $77:10D0 was clear. Existing original
CPU observations show FF at F568 and the sixteen-byte level copy branch.
Pinned Memory::allocate defaults to FF; fresh capture initial cartridge RAM
hash equals8192 FF bytes (7d2c7ac4888bfd75cd5f56e8d61f69595121183afc81556c876732fd3782c62f).
Native cold state now owns that flag, conditionally copies the level work and
clears it. `native-cold-cpu-title-name-failure01.json` and version1 manifest
are retained. Version2 corrects only the provisional domain description;
all expected events, clocks and PCM bytes remain unchanged.

The app is still silent. No native CPU continuation, full audio save, menu
producer/NMI lifecycle or device acceptance follows from this cold prefix.

The cold flag setup at $80:A09F-$80:A0A5 clears bit1 of erased FF flags,
leaving FD for the native title callback. Static bank-80 supplies this reading;
the cold CPU/register trace supplies its executed branch.

## Native NMI entry and recurring title fade, 1 October

Verified: `native-cold-cpu-first-nmi-pcm.json` extends the cold prefix through
first title interrupt return at $80:9869, C=106,375,654, matching all872,512
frozen projected events and160,155 complete raw stereo pairs. Static bank-80
$80:8587-$80:8598/$80:F622-$80:F644/$80:FA60-$80:FAC9/$80:B0F0-$80:B0F9
supplies the mirrored bank00 vector wrapper, title scroll, rotating palette
and return reading. The first attempt was six clocks late because PEA was
given an idle cycle; pinned instructionPushEffectiveAddress has none. Its
failed report is `native-cold-cpu-first-nmi-clock-failure01.json`. No expected
event, CPU clock or sample was changed.

`native-cold-cpu-title-fade-pcm.json` then matches all880,206 events,
C=109,262,190 and164,501 raw pairs through seven-wait fade return $80:F5C0.
The native run dispatches eight NMI bodies, including enable during blank.
`cold-cpu-title-fade-frozen.json` pins this extension before comparison.
Pinned CPU::nmiPoll/lastCycle/nmitimenUpdate, CPU bus/DMA and PPUcounter supply
the four-clock polling, two-clock raster lookback, one-bus-cycle-early test,
IRQ lock and idleIRQ bus-read replacement. Semantic CPU functions call those
boundaries; they fetch no original opcodes and use no observed producer times.
PAL noninterlace, slow ROM, one-channel DMA and the native title callback are
the tested domain. IRQ/HDMA/interlace and other handlers remain outside it.

The same opaque executable-payload projection and identified data inputs are
retained; all ordered command/DSP/CONTROL accesses, both clocks and nonopaque
values remain required. The DSP horizon clips diagnostic packaging only.
The app remains silent; later title/menu producer work, canonical CPU/full
save, product output and independent capability review remain open.

## Native automatic controller polling and title hold, 1 October

Verified: fresh CPU500 instruction-full/off captures agree on state, A/V, raw
PCM, final APU RAM and all 2,451,192 shared kind1-9 rows. Full observation has
7,146,239 CPU instructions. `cold-cpu500-integrity.json` and retained command
records bind this to the same PAL ROM/core/ABI5 identity. Static bank-80
$80:D1EC-$80:D1F9 and $80:D20E-$80:D338 were read before these captures.

`native-cold-cpu-title-hold-pcm.json` matches all 1,007,259 projected events,
235,622 raw stereo pairs and original C=156,499,030 at $80:F614. All 111
retained controller-word/frame-end-clock projections also match, including
the six Start words on physical frames299-304 from script frames300-305.
Pinned CPU::joypadEdge supplies the 128-clock serial polling, 256-clock start
window and separately sampled result bytes. The worker leaves at blank and
installs the next frame's input before polling; native input latches use this
computed raster/frame relation. Native CPU starts from zero and receives only
the declared controller schedule, identified data and native handshakes.

`native-cold-cpu-title-return-pcm.json` extends to all 1,015,110 events and
240,101 raw pairs at C=159,477,794/$80:886A, after the seven-frame fade-out.
Both extension manifests and the 111 controller projections were frozen
before their native comparisons. Expected event/sample/controller bytes are
unchanged. Static $80:F5C0-$80:F617 supplies hold/input/fade work. New hold
state owns its wrapping counter, code index and sampled controller words.
The $80:F5E3/$80:B124 code-acknowledgment branch is implemented from that
reading but has no new dynamic claim in the Start-only domain; freeze a
positive code schedule before testing it.

Main-menu resource work, canonical complete CPU/audio continuation, pack and
real device output remain open. These joined cold checks do not accept the
capability or change the app's silent status.

## Native menu palette and raw graphics loading, 1 October

Verified: `native-cold-cpu-menu-first-palette-pcm.json` matches all 1,017,400
events,241,424 raw pairs and C=160,342,178 before $80:D263.
`native-cold-cpu-menu-graphics-pcm.json` extends to all 1,030,090 events and
257,748 pairs at C=171,198,560/$80:D2AD. Earlier111 controller/end-clock
projections remain exact. Frozen manifests predate both native comparisons.
NMI stays enabled during base-palette DMA, initialization and all transfers.
Native A16A clears palette counters on the final two work writes; the raw
loader preserves directory/asset read instruction boundaries and LoROM wrap.

Static bank-80 $80:D20E-$80:D2AD/$80:B091, bank-82 $82:B183-$82:B2DC and
bank-83 $83:91F7-$83:9228 supply the reading. CPU-full observation at9202/9205
shows FFFF/FF for the cold erased mode word, so the native mode byte255 takes
the non-1 asset2 palette path. Identified metadata is versioned privately: the
original eight-asset file is retained; version2 adds the separately identified
default rider palette. Native mode1 selection has no claim in this cold helper
and is explicitly rejected pending its typed palette path.

No expected event, clock, sample or input hash was changed. Product audio,
full canonical CPU/audio continuation, menu OAM/text/input production and
independent capability acceptance remain open.

The next cold menu slice reads static bank-80 D2AD-D36F, including sprite
reset D2C1 and choice work D30B, and factors the existing $83:99F6 arrow
parking callee without changing its bus order. Frozen before implementation,
`cold-cpu-menu-oam-frozen.json` ends before D36F at C=171,240,248 and
D=8,250,016. `native-cold-cpu-menu-oam-pcm.json` matches all 1,030,159
projected events,257,813 raw stereo pairs and the prior111 title controller
frames; no original duration, opcode or producer time is a native input.
The current helper owns only work used by the native scene/audio clock,
not a separately accepted menu picture. Main-menu return/text/input, product
playback and canonical CPU continuation remain incomplete.

The cold menu cartridge slice reads static $83:8AF7-$83:8B90 and $83:FB41
through FB55, $80:D36F/$80:8C4E through 8C80. Native 8 KiB state verifies the
wrap at 1FFF/2000, restores byte 0, compares the identified 12-byte signature
and clears all 4096 words on mismatch. `cold-cpu-menu-clear-frozen.json`
predates its candidate; `native-cold-cpu-menu-clear-pcm.json` matches
1,031,200 events, 258,851 raw pairs and C=171,925,134. A matching-header branch
is implemented but untested in this cold domain; its later continuation
still needs recovery before restart acceptance.

The first default slice reads static $83:9983, $80:96FD, $83:92CC/$83:92FA/
$83:9328 and $83:92E3/$83:9311/$83:936E/$83:93D8. Its locally extracted
`cpu-cartridge-defaults.bin` covers only the identified default-data table
83:8000-8483 (physical 18000-18483); `cpu-track-types.bin` covers the 50
bytes at 83:A254, with numeric type 0 at index 0. Consumers read only names,
league masks and times from bounded offsets; no code or producer clocks.
`cold-cpu-menu-records-first-frozen.json` fixes the data hashes before its
candidate. `native-cold-cpu-menu-records-first-pcm.json` matches 1,031,290
events, 259,029 raw pairs and C=172,050,810 before 8CA9. This is audio event/PCM
evidence during record work, not a newly accepted record-state contract.

League default recovery reads static $83:93F5/$83:963C/$83:969F/$83:96BE
and $80:AD43/$80:AD79/$80:AD94/$80:EFD4 through F02A. Native membership
expansion, eight-row unsigned selection sort (replace on equal), RAM block
copy work and six league loops preserve the original ordering. All cold
masks are zero; positive membership branches have no dynamic claim here.
`cold-cpu-menu-league-frozen.json` and `native-cold-cpu-menu-league-pcm.json`
match 1,031,547 events, 259,375 raw pairs and C=172,266,996 before 8CB6.

The remaining static defaults at $83:9486/$83:9340/$83:94A5/$83:90F4,
followed by $83:8B23 from $80:8CB6, finish before the menu text call 8878.
Source inspection found that ten settings words at 8472 extend through 8485.
The preceding 1156-byte candidate's matching audio is retained as incomplete
evidence in `native-cold-cpu-menu-records-bounds-gap01.json`; it does not
validate the out-of-bounds settings read. The v1 input remains unchanged.
`cpu-cartridge-defaults-v2.bin` extends its identical prefix by two bytes to
1158, and `cold-cpu-menu-records-frozen-v2.json` freezes the new hash before
the corrected candidate. Original event/PCM bytes and endpoint are unchanged.
`native-cold-cpu-menu-records-pcm.json` then matches 1,031,945 events, 259,822
raw pairs, C=172,562,846 and 111 controller frames with the corrected bounded
table. The synthetic terminal-word sentinel check passes, as do both affected
CPU checks. Still no new record-state, audible product or canonical-save claim.

$80:8C80 is the native first-default-table entry named in the cold slice above.

The next source reading uses static $80:ACD5, $80:D1FA, $80:C3BC through
C5C8, $80:8C41 and the already recovered $83:8B51 text-work reset. Native
$80:C4B8 centering retains the byte budget33 and its wrapping subtraction
before halving; $80:C41E big glyphs occupy two columns, $80:C56C small
glyphs one. $80:C3FE normalizes printed characters and $80:C557 applies
the word attribute. Identified inputs `cpu-menu-text.bin` (80:AD1F-AD42)
and `cpu-character-table.bin` (80:C709,256 bytes) use the same data ranges
as existing frontend extraction; no opcode/producer times are inputs.
`cold-cpu-menu-text-frozen.json` fixes both hashes and the original projection
before candidate implementation. `native-cold-cpu-menu-text-pcm.json`
matches1,032,122 events,260,044 raw pairs,C=172,720,798 and all111 title
controller frames. Its cursor727/attribute7168 are owned diagnostics only.
The initial menu's FF/FB/FC/F9 controls define this helper's tested domain;
other controls are rejected. Special small-glyph and backquote branches
have no new dynamic claim in this slice. Product/canonical save remain open.

The reveal slice reads static $80:937B, $80:A877, $80:F52B, $80:AD0F,
$80:ACF7 and $80:887B through $80:889A. It uploads 2048 text bytes, performs
the common palette/OAM/font work, fades in and repeats the already drawn menu
uploads. Its cold intro bit is clear; the positive intro branch is rejected
pending recovery. `cold-cpu-menu-reveal-frozen.json` precedes implementation.
`native-cold-cpu-menu-reveal-pcm.json` matches all 1,046,913 events, 268,323
raw stereo pairs, C=178,217,620, D=8,586,336 and 111 prior title controller
frames, ending before the first interactive menu call. No original producer
time advances native CPU or SMP; the closing DSP limit clips PCM packaging.
Product playback, full canonical continuation and interactive/restart
producers remain open.

## Native interactive menu input, 1 October

Static $80:ABC8-$80:ACD5, $83:9558, $80:B71D/$80:B76F/$80:B794 and
$80:B178 supply the menu input reading. Native pads are automatic poll words;
D1EC includes OAM DMA before the two reads. The idle counter wraps as a word,
the directional release latch suppresses repeat cues, and five arrow targets
use the identified bytes at $80:88BE (physical 08BE-08C2). Navigation queues
{8,127} then {2,3}; no original enqueue time enters the producer.

`cold-cpu-menu-input-begin-frozen.json` and `cold-cpu-menu-idle80-frozen.json`
predate the candidate. Native comparisons pass 1,046,913 and 1,138,321 events,
268,323 and 319,554 raw pairs respectively. The latter includes 80 menu
controller/end-clock projections at C=212,250,252. A fresh 500-frame Down
variation freezes Start300-305/Down450-455 in `cold-cpu500-down.script.json`.
Instruction observation on/off preserves exact state/A/V/PCM/APU digests
and all normalized kind1-9 events (`cold-cpu500-down-integrity.json`).
`cold-cpu-menu-down80-frozen.json` precedes navigation implementation;
`native-cold-cpu-menu-down80-pcm.json` matches 1,137,795 events, 319,556 pairs,
80 menu controller/end-clock projections and C=212,250,772. All 111 prior
title frames also match in both comparisons. These cover first effect/music
overlap and hold suppression, not retrigger, Up wrap or full lifecycle.
Product playback, complete canonical save and independent review remain open.

Further withheld inputs freeze before their comparisons, leaving the same core
candidate unchanged. `cold-menu-variations-integrity.json` verifies on/off
state/A/V/PCM/APU and normalized kind1-9 equality for Up450-455, Down450 then
453-455, and the 1800-frame steered menu (Down450-455, Up1500-1505). Their
`cold-cpu-menu-*-frozen.json` manifests retain exact input, event, PCM and end
clock hashes. Native Up wrap/retrigger comparisons match 1,137,832/1,137,577
events, 319,556/319,555 raw pairs and 80 menu end clocks each. The steered
comparison matches 2,612,233 events, 1,152,477 pairs and 1380 menu end clocks
at C=765,490,044, D=36,879,264; all111 prior title frames remain exact.
This establishes native cold producer prediction in these bounded domains,
including overlap, hold suppression, Up wrap and retrigger. It does not accept
product audio, complete save or scene restart.

`audio-system-refactor-frozen.json` freezes these unchanged reports/inputs
before factoring CPU/SMP/DSP ownership and adding canonical continuation.

## Native clock ownership and canonical continuation, 1 October

`audio-system-refactor-frozen.json` retains pre-refactor reports. The shared
native engine replaces the runner-only CPU/SMP bus, owns its native clock,
IPL/driver, timers/voices, DSP RAM/history and pending stereo PCM, and
preserves all six frozen event/PCM domains above. Static content and future
controller words remain external immutable/input owners; no original clock,
opcode, CPU write or state is supplied. Native title/menu phases compose the
same recovered cold calls at semantic boundaries.

URAU0001 serializes explicit little-endian fields and strict binary flags,
including CPU raster/refresh/NMI/autopoll/DMA fields, retained driver IO plans,
queue/scene/title/menu/text/cartridge work, DSP continuation and unconsumed
PCM. State restore validates separate owners before changing the engine.
`audio_canonical_file_and_transactional_restore` passes version/truncation/
trailing-byte/binary-flag checks, signed PCM extremes and rejection of bad CPU
or DSP state without mutating the running session. No untrusted SPC parser
or original save-state import is added.

`native-save-domain-frozen.json` freezes the same steered1800 input/horizon
and eight split choices before comparison. The uninterrupted phase owner
matches all2,612,233 original projected events,1,152,477 raw pairs and
C=765,490,044. `native-fresh-process-save.json` passes all eight separate
processes: cold, title20/title111, menu1/menu30/menu32/menu60/menu1082. Each
saved file re-encodes identically after parsing and restoration; concatenated
events, retained-plus-new PCM and final canonical state equal the uninterrupted
native run. Saved sizes range75,736 to3,922,016 bytes, including pending PCM.
The first checker wrongly removed all N RAM rows, including the frozen upload
prefix, and failed before restore cases. Its corrected projection preserves
that prefix; neither expected data nor native behavior changed.

These verify call-boundary native continuation in the declared title/menu
domain. Device/resampler fractions, pack binding, live playback and joined
scene-stop/restart remain open; no complete product-save claim is made.

### Native content pack and bound continuation - 1 October 09:54 UTC

The v30 rules append33 identified audio entries to the unchanged 475-entry v29
inventory. Main evidence `audio-pack-extraction-proof.json` compares every new
payload to the identified loose recovery inputs. The frozen sorted compact v29
entry-list digest is 6df6cfce7f83ef8ac93a4e6143f1987244dbb1901dc6a7b5e1fe61618cd487f7;
its rules file is retained as `classic-crawler-tracks-v29-rules.json`.
New rules SHA-256 is 293039530c6ad250aa072415ea0f9af096532754648029d9ab69e734e0c81cf7.
The uploaded executable is excluded. Numeric pitch indices 85-96 retain the
documented data alias; they are interleaved low/high values, never executed.
Sample resources exclude their length word and retain loop/parameter/BRR data.
The transfer payloads preserve the original sender's six-byte trailing header
behavior. Graphics directory entries supply bounded native work metadata.

`title_menu_audio_content` loads only validated logical pack entries. The
URAU0002 canonical state carries the pack's validated extraction identity. A
mismatched identity is rejected before engine mutation. URAU0001 files and their
previous reports remain retained; there is no implicit conversion.
`native-save-domain-v2-frozen.json` preserves the previous eight split choices
and original event/PCM identities before v2 continuation comparisons.
`native-fresh-process-save-v2.json` passes all eight fresh-process cases, with
identical concatenated events, pending plus new PCM and final canonical state.
`native-pack-v30.json` additionally passes a pack-only 1380-loop steered run: its
2,612,233 events, 1,152,477 raw pairs and final state equal the loose-data native
run, which matches the unchanged original projection. The native command opens
only the pack and future controller schedule; no ROM, original state/event or
timestamp is an input. The extraction report is main
`artifacts/audio-title-menu-integration/pack-v30.json` (508 entries).

Ten focused native audio checks pass, including transactional identity/CPU/DSP
rejection. The 27 track inventory tests and 15 frontend tooling tests pass. The
first frontend unittest command lacked PYTHONPATH and failed to import the
module; rerunning with PYTHONPATH=tools passed. A ctest expression `audio_state`
matched no tests and is not a pass; the explicit `^audio_` expression ran all 10.
The new factory/session/archive pass the 80-line readability check. This adds
content packaging and call-boundary continuation evidence only. SDL device
output, resampler fractions and joined scene stop/restart remain incomplete.

## Incremental output and bounded native lifecycle - 1 October 11:10 UTC

The pre-change `incremental-pcm-before.json` preserves the v2 reports. Advancing
DSP at native SMP step boundaries and delivering generated PCM incrementally
preserve the steered1800 original event/PCM projection and all eight separate
process saves (`native-incremental-pcm-save.json`). The pack-only streaming and
buffered runners also agree (`native-stream-pcm.json`). No original clock/event
input or waveform expectation is added or changed.

Native modern output uses signed integer linear interpolation from32040 stereo
pairs/second, with an owned fraction, source history, pending converted pairs
and source/generated/delivered counters. This is a host output policy, not the
reference resampler. URAO0001 stores this continuation; URAP0001 combines it
with URAU0003 native state and requires source pairs to equal generated DSP
pairs. The engine's retained observers prohibit moving its object. Output
drains are mutex protected; native work/snapshot/restore require a quiescent
producer, and restore requires the device stopped as well. The OS mixer is
outside serialization. Bounds and malformed counter/fraction/CPU/DSP/content
rejection are explicit and transactional.

`native-playback-save-frozen.json` declares48,000 output pairs/second,257-pair
drains after each title/menu call and the previous eight split choices.
`native-playback-fresh-process-save.json` passes all eight fresh-process
continuations: concatenated events, partially drained plus remaining converted
PCM and final owned state match uninterrupted native playback. The independent
integer conversion of the unchanged original raw PCM equals all1,726,556
converted pairs. Seven noncold saves retain nonzero fractions and queues,
including1,133,918 pending pairs at menu1082. The initial copied runner omitted
the playback include and failed to compile; the explicit include fixes that
build prerequisite. Public canonical/output checks pass. This is call-boundary
playback continuation, not asynchronous thread-stack or OS device restore.

Static listings in main evidence `native-lifecycle-static/bank-80.lst` and
`bank-83.lst` supplied the readings F0D6/F51B/98A4,AB9A/A4E9/A923 and E2CF.
Fresh HUNTER on/off observations preserve all state/A/V, normalized APU events
and PCM (`hunter-native-lifecycle-integrity.json`). The unchanged1850-frame
script enters B+Down+L+R on pad1 at450-453, presses pad2 Start at700-703 and
1000-1003 and pad1 Start at1300-1303. ROM/core/options are the identities above.
The three original endpoints are frozen before native implementation in
`cold-cpu-hunter-{exit,ready,first-fade}-frozen.json`.

`native-hunter-entry.json` predicts each endpoint from pack/future input only:
1,082,257/1,117,696/1,136,291 events,288,176/308,018/318,288 raw pairs and CPU
clocks191,395,368/204,587,516/211,391,414, with all31 waited iterations exact.
The first failed transition retained in
`native-hunter-entry-missing-interrupt-flag.json` missed the F51B cartridge flag
update used by title NMI. Propagating the flag after its timed store corrects
the reading without changing expected clocks. Static AC0A identifies8430 as
HUNTER and ABF9 identifies02B0 as RAM wipe; the provisional enum names were
swapped and are corrected. URAU0003 owns the pending action and HUNTER count.
Eight menu and five HUNTER fresh-process saves pass event/PCM/final-state
continuation (`native-fresh-process-save-v3.json` and
`native-hunter-fresh-process-save-v3.json`). Earlier format reports/files remain
retained and are rejected without an implicit conversion. Ending pages/HDMA,
credits, warm reset and any ending audio accuracy claim remain outside this
verified prefix; their recovery stays inside the capability task.

The visible SDL prototype waits at automatic controller polling for actual
frontend samples and feeds a converted queue to CoreAudio. The800-update run
in main artifacts `audio-live-prototype.log` delivers753,664 pairs,657,275
nonzero, with zero reported underruns. Two live key checks fail: single Down
and40 repeated Down presses produce mapped down/up events but zero sampled
input or native navigation changes. `audio-live-navigation.log` and
`audio-live-repeated-navigation.log` retain these failures; both eventually
cross the unrecovered idle boundary and report underruns. UI selection used
this task's exact app path because multiple installed copies share its ID.
The CUA documented key API supplies a press/release pair, not a held key.
No keyboard policy or expectation changes to force the check. These counters
establish device delivery, not human listening or completed live acceptance.
The ordinary app remains silent; the prototype, scene lifecycle and full
capability review are unaccepted.
