# R-0075 - raw title/menu audio observation and hardware isolation

Status: in-progress coupled experiments for AUDIO-TITLE-MENU, 30 September
2026 UTC. No native driver, audible product or capability acceptance is claimed.
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
