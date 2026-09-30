# R-0074 - cold audio observations and the native recovery boundary

Status: laboratory observations for AUDIO-DECISION, 30 September 2026 UTC,
on source `16675d0bd444197913f3460efcf4ae76a6a8c812`. No native audio is implemented.
PAL ROM SHA-256
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`;
bsnes commit `7d5aa1e656b9171524d01b1b22917197d8121cb4`, audited core library
`e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b`,
patch `a719f5ffe2222dad4c1ab04336633319ad85004f74e32fc14893a058be333885`.
Default adapter options, PAL, Strict synchronization, private fresh save directory
per process. D-0001's reference restore-audio exclusion remains unchanged.

## Verified observations

Main `local/evidence/audio-decision/` retains scripts, samples, access records,
queue WRAM series and `callback-audit.py`. Main
`artifacts/audio-decision-integration/` retains doctor and command reports.
`reference verify` ran each schedule twice in separate fresh processes.

| Schedule | Controller 1 | Compared audio frame rows | Stereo sample pairs delivered per run | Audio row digest prefix |
| --- | --- | ---: | ---: | --- |
| boot-start-600 | Start 300-305, frames 0-599 | 600 | 575,040 | `84c8e32ce1f405fa` |
| menu-base | Start 300-305, frames 0-699 | 700 | 671,040 | `04c3f02ef16e6e6d` |
| menu-down | Start 300-305, Down 450-455, frames 0-699 | 700 | 671,040 | `7ee5c7cbfb311219` |

Controller 2 is released throughout. Every compared callback count and digest
is identical between the two runs of each case: 2,000 frame rows in total.
Counts per frame are 0 or 960 stereo pairs, reflecting this core's buffering.
The audit compares each hash to an all-zero stream of the same length: 500,
600 and 600 frame rows respectively contain nonzero callback samples. PCM bytes
were not retained, so these are hash/count observations, not a waveform contract.

The baseline and Down schedule first differ in callback audio and WRAM at
frame 450. Their audio differs on 249 frame rows through 699. This associates
the intervention with changed sound; it does not identify a command's meaning
or establish a complete effect or musical-loop duration.

Both 700-frame schedules also ran with the existing CPU access instrumentation.
Each instrumented run has exactly the matching uninstrumented memory/register
sample digest, A/V digest and per-frame audio counts/digests. Totals: 9,883,017
instructions on the baseline, 9,884,227 with Down. Neither ring overflows
(maximum frame 18,346 instructions, capacity 524,288), and watched-PC logs are
not truncated. Each access record declares 12,179 unresolved accesses, zero
unresolved stores and two WRAM program counters; this is diagnostic CPU access
evidence, not complete APU instruction or cycle evidence.

Watched entries in the two captures:

| Point | Baseline | With Down |
| --- | --- | --- |
| Sound upload entry, X at `$82:807E` | frame 29: 50; 37: 53; 38: 57 | Same |
| Queue entry, A at `$82:8000` | frame 97: `0102`, `08FF`, `077F` | Same, then frame 450: `087F`, `0203` |
| Paired port store, A at `$82:806D` | frame 97: `0241`, `FF88`; frame 98: `7F47` | Same, then frame 450: `7F88`, `0342` |

At frame 450 the two 16-bit stores are to `$00:2142-2143`, at CPU instruction
sequence positions 300 and 693 in that frame. Sequence positions are not master
clock timestamps. Port reads have address/register derivations; their exact
APU-side latch values and acknowledgment times are not independently exported.
The next task needs that instrumentation before claiming transport fidelity.

## Static reading and implementation facts

The source of these readings is main `artifacts/static-map/bank-82.lst`, read
before the captures above:

- `$82:8000-8034`: enqueue the two bytes into masked 16-entry arrays at
  `$7E:2006` and `$7E:2016`, with producer/consumer positions at `$7E:2002/2000`
  and a full-queue branch. The capture reaches enqueue/dequeue; queue-full
  behavior is a reading only.
- `$82:8035-807D`: compare the port with the acknowledgment byte `$7E:2004`,
  combine the queued bytes with the toggled bits, advance the consumer and write
  the paired ports. Dynamic command/store observations are the table above;
  precise handshake timing remains unrecovered.
- `$82:807E-8129`: upload entry and port protocol. `$82:812A-8160` starts its
  source pointer at bank `$10`, offset `$8000`, and traverses length-prefixed
  records with bank-wrap handling. This is not a proven code/data directory.
  The earlier consultation prompt mistakenly named `$84:8000`; the consultant
  corrected it from the listing. No ROM offset or asset identity is assigned here.

`tools/unirally_lab/reference/bsnes.py` hashes signed 16-bit little-endian stereo
callback samples but retains no PCM buffer. Its memory exports cover WRAM and
cartridge RAM; there is no existing APU RAM, DSP-write or raw-DSP-PCM capture
interface. These proposed interfaces remain work to implement.

The [pinned libretro source](https://github.com/bsnes-emu/bsnes/blob/7d5aa1e656b9171524d01b1b22917197d8121cb4/bsnes/target-libretro/libretro.cpp)
configures 48 kHz output and a PAL 960-pair batch buffer. The
[DSP wrapper](https://github.com/bsnes-emu/bsnes/blob/7d5aa1e656b9171524d01b1b22917197d8121cb4/bsnes/sfc/dsp/dsp.cpp)
passes integer samples into an audio stream after conversion to float, before
device-rate conversion. Hence callback hashes cannot substitute for raw DSP PCM.
`src/app/sdl_main.cpp` initializes video/gamepad only; no native audio device or
driver is present. Existing loading handshakes already affect the race result's
publication timing (D-0001, R-0039 and `native/dragster_playable.py`).

## Reproduction

From a fresh task checkout with the pinned core and private ROM locator:

```sh
E='/Users/markfeaver/Projects/Unirally Decompilation/local/evidence/audio-decision'
python3 tools/project.py reference verify --script tests/manifests/reference/boot-start-600.json --runs 2 --timeout 30 --artifacts "$E/FRESH-boot" --report "$E/FRESH-boot/report.json"
python3 tools/project.py reference verify --script "$E/menu-base.script.json" --runs 2 --timeout 30 --artifacts "$E/FRESH-base" --report "$E/FRESH-base/report.json"
python3 tools/project.py reference verify --script "$E/menu-down.script.json" --runs 2 --timeout 30 --artifacts "$E/FRESH-down" --report "$E/FRESH-down/report.json"
python3 tools/project.py access capture --manifest "$E/menu-down.replay.json" --out "$E/FRESH-down-access" --from-frame 0 --to-frame 699 --watch-pc 0x828000 --watch-pc 0x82806D --watch-pc 0x82807E --watch-pc 0x8282A5 --watch-address 0x2142 --watch-address 0x2143 --wram-series-range 0x2000 0x26 --ring 524288 --timeout 120
python3 "$E/callback-audit.py"
```

The baseline access command substitutes `menu-base.replay.json` and a fresh
output directory. The final audit reads the retained original directories,
leaving expectations unchanged; compare a fresh run's samples separately.
`callback-audit.json` and closeout retain full hashes. Initial access capture
rejected an invalid replay event containing a redundant `port` field; the
invalid manifest and failed report are retained. Removing that field follows
the existing manifest schema and changes no controller timeline.

## Decision, hypotheses and limits

[D-0009](../decisions/D-0009-native-audio.md) chooses native command producers,
a recovered sound driver/sequencer, identified local content and an isolated
DSP hardware model. A bounded Astra/medium consultation supported that design;
it is advice, not audio accuracy evidence. The upload source's code/data layout,
music event format, effect identities, voice allocation, loop lengths and exact
CPU/APU timing remain hypotheses for AUDIO-TITLE-MENU to investigate.

No native PCM, live audible product, audio save/restore, full soundtrack,
alternative region, race/ending sound or complete stunt edge-case coverage is
claimed. The accepted stunt records retain their original limits. Original-code
execution inside the product remains rejected; laboratory execution is evidence
collection only.
