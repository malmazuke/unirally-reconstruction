# D-0009 - native audio driver, extracted score data and a DSP model

- Status: adopted by AUDIO-DECISION, 30 September 2026 UTC.
- Scope: M4 audio; first outcome [AUDIO-TITLE-MENU](../../tasks/AUDIO-TITLE-MENU.md).
- Evidence: [R-0074](../research/R-0074-audio-foundation.md). Architecture choices
  below are implementation decisions, not claims of recovered audio accuracy.

## Reason

The native app remains silent. The next coverage-roadmap outcome is audio:
STUNT-EVENT-RACE, STUNT-RESULT and STUNT-HUD already delivered the preceding
stunt outcome. The user rejects execution of original game code in the product
on every screen (D-0008), including the SPC700 sound program. Audio therefore
needs recovery of the sound driver, not integration of an SPC player.

The current laboratory reproduces uninterrupted callback audio digests, but
does not retain PCM, APU RAM or DSP register-write streams. Its callback audio
has already been resampled and buffered. D-0001 explicitly excludes it from
restored-state exact acceptance. Sound handshakes also affect game timing;
existing native loading intervals cannot simply be discarded when sound is added.

## Decision

Use four explicit components, keeping game decisions independent of the device:

| Component | Responsibility | Evidence boundary |
| --- | --- | --- |
| Native producers and transport | Emit and order recovered commands; retain queue capacity, readiness and gameplay-visible waits | Command/port traces, including same-frame order |
| Native sound driver and sequencer | Decode documented musical events, allocate voices, update pitch/envelope parameters, stop/reset | Command consumption, ordered DSP writes and integer clock positions |
| Locally extracted content | Identified score, instrument and sample data with loops, source ranges and hashes | Source/destination upload audit and bounded extraction |
| DSP hardware model and output | Produce deterministic integer stereo PCM; resample and queue it for the device separately | Raw pre-resampler PCM, DSP phase and independently tested output path |

The driver is ordinary readable C++ with small explicit state updates. It must
not fetch/decode original SPC700 instructions or implement an interpreter for
uploaded game code. A score representation holds identified notes, musical
controls and loops; unsupported bytes require investigation, not execution.
Neither prerecorded PCM nor a captured DSP event stream is a product fallback.

Keep original sample data and identified score/instrument data in a locally
generated pack under D-0005. Do not pack an entire sound upload, APU snapshot,
SPC file or executable range as a shortcut. Prove the code/data boundary before
adding extraction rules. No audio pack revision or schema is frozen by this decision.

Start the hardware experiment with an isolated DSP-only implementation. The
pinned bsnes `SPC_DSP` is the concrete first candidate; no CPU interpreter or
whole-emulator dependency enters the product. Its header declares LGPL-2.1-or-later,
while this repository declares MIT. Before adopting its source, record the exact
files/version, preserve their notices and license, and document the dependency's
build/link/source arrangement. Do not relabel that source as MIT. This task
imports no dependency and makes no legal compatibility claim. If isolation is
impractical, record the evidence and choose an independently implemented hardware
model; do not silently broaden the dependency to an emulator.

Use integer audio-clock positions, documented CPU/APU conversion, stable event
ordering and fractional sample accounting. Device callbacks must not advance
game or musical time. Do not assume a fixed number of samples per PAL frame or
exactly 32,000 Hz. The [pinned DSP wrapper](https://github.com/bsnes-emu/bsnes/blob/7d5aa1e656b9171524d01b1b22917197d8121cb4/bsnes/sfc/dsp/dsp.cpp)
uses the system APU frequency divided by 768 and advances one or 32 DSP clocks
depending on its fast setting; its echo-memory configuration also matters.

Use the existing SDL3 dependency for device output. Its
[audio device stream](https://wiki.libsdl.org/SDL3/SDL_OpenAudioDeviceStream) and
[PCM submission](https://wiki.libsdl.org/SDL3/SDL_PutAudioStreamData) interfaces
are candidates for a bounded output adapter. Verify availability against the
project's pinned SDL build. Device conversion and latency stay outside exact
core PCM comparisons; their live checks establish audible operation.

## First outcome and experiment order

[AUDIO-TITLE-MENU](../../tasks/AUDIO-TITLE-MENU.md) delivers audible native
title/menu music and a dynamically identified navigation effect, including their
overlap, retrigger and stop/restart behavior. Observation-tool extensions and
sound-driver recovery are coupled experiments within this tier-1 capability.

1. Add laboratory-only capture of raw pre-resampler DSP PCM, ordered DSP writes,
   relevant APU RAM mutations and upload segments, using a shared clock/order
   convention. Record options, clocks, PCM format and callback buffers. Prove
   instrumentation does not perturb uninterrupted memory/register/video/audio
   output with it on versus off; do not replace an old reference silently.
2. Capture cold title/menu baseline and controlled input variations twice.
   Find the first command, DSP-write and raw-sample difference. Identify one
   recovered musical loop or bounded non-looping cue before freezing its horizon.
3. Audit an upload from ROM source through APU destination and separate driver
   code from musical/sample data. Recover command dispatch and sequencer timing.
4. As a laboratory diagnostic only, replay ordered DSP writes and relevant RAM
   changes into the isolated hardware model. Exact raw PCM separates hardware
   timing faults from driver recovery faults. Frozen replay is not native acceptance.
5. Replace recorded events with native command/driver output. Compare ordered
   events and raw PCM, then exercise visible audible frontend output and existing
   game regressions. Independent withheld cases must change timing and overlap.

## Acceptance and limits

Require exact counts and signed stereo samples in the frozen uninterrupted raw
PCM domain, alongside ordered command/DSP events and unchanged game timing.
A listening judgment, a waveform resemblance or matching resampled hashes alone
does not establish native audio accuracy. Missing capture interfaces are explicit
prerequisites inside the implementation task, not commands available today.

Native restore tests must include sequencer, voices, pending commands/timers,
DSP phase and decoder/envelope/noise histories, echo memory and fractional output
state. Compare restored native continuations to uninterrupted native runs that
already match the cold original. Reference restore audio needs separate proof,
including saving without perturbation; D-0001's exclusion remains in force.
Device queues can be flushed/reprimed outside the deterministic core, with that
audible boundary stated explicitly.

Whole soundtrack/effect coverage, race/ending audio, streaming behavior not yet
identified, alternative regions/output rates and whole-game accuracy remain
later work. No approximate behavior may be described as recovered. A real
first divergence narrows the experiment; it does not authorize original-code
execution or a headless-only acceptance fallback.

## Consultation and reassessment

The primary obtained one bounded fresh `gpt-6-astra`/medium planning consultation
under D-0004. It recommended the four boundaries above, raw sample/event capture,
instrumentation non-perturbation and a title/menu first outcome. It flagged
loading handshake timing, resampler restore state and upload source ambiguity.
The primary adopted those points and deferred dependency adoption and pack
format until their experiments. This is advisory planning, not independent
review or gameplay evidence. Reassess the hardware candidate after raw-PCM
replay, and the first scope after one upload's code/data audit.
