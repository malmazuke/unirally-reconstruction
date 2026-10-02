# D-0010 - Frame-anchored sound command timing outside the title/menu CPU model

- Status: adopted by the AUDIO-FIRST-RACE coordinator (Claude Opus 5.5), 1 October 2026 UTC
- Related: [D-0009](D-0009-native-audio.md), [AUDIO-FIRST-RACE](../../tasks/AUDIO-FIRST-RACE.md),
  [R-0075](../research/R-0075-title-menu-audio.md), [R-0076](../research/R-0076-first-race-audio.md)

## Problem

D-0009 asks for exact raw PCM. The sound driver, score, sample data and DSP model now reproduce
the first race exactly when the original's CPU port writes are supplied (R-0076). In a product
those writes come from native producers, and their CPU clock decides which driver poll sees
each command. AUDIO-TITLE-MENU made the title and main menu exact by modelling the CPU work
of every executed instruction path as a cost shadow (`audio_cpu_*.cpp`).

Measured on the cold 1P DRAGSTER path (R-0076):

- The race calls the dispatcher at `$83:CD6E` and `$83:CD9F`, 7,250-42,168 and
  167,452-239,482 master clocks after the frame boundary; the driver polls about every
  1,850 master clocks. The dispatch time depends on the whole race body before it.
- Shifting race command arrivals by up to 50 SMP ticks moves the post-race driver entry by
  10 ticks, and about 160,000 of the 345,000 later raw pairs then differ. Exact audio after
  the race does not reconverge without exact race timing.
- The cost shadow would have to cover the 12,055 distinct instruction sites executed through
  the race load and about 4,700 more in the race (coverage captures `coverage-1330`,
  `coverage-3455`), and then every other track, mode and screen.

## Decision

Outside scenes that already have a cycle-exact CPU work model (title, main menu, HUNTER),
native producers emit the original's commands with exact content, order and frame, and the
transport delivers them at **declared frame-anchored CPU clocks**:

- Each dispatch opportunity has a fixed position in its frame, in master clocks after the
  frame boundary, for its scene and call site. The positions are calibration constants taken
  from the original's observed medians, and the code says so where they are defined.
- Everything after delivery is native and unchanged: the port handshake and its
  acknowledgment, the IPL and sample transfers at their recovered per-byte cost, the driver,
  score, DSP and output.
- Upload sessions start at an anchored clock and then run the recovered transfer work.

## Consequences

- Claims stay separate: the driver, score, samples and DSP are exact for supplied arrivals
  (conditional evidence); producer commands are exact by frame and order (compared with the
  original's enqueue sequence); the product's raw PCM is **not** exact outside the cycle-modelled
  scenes. Reports state the measured arrival error and PCM agreement, never "exact".
- Title/menu exactness is unchanged up to the first anchored scene.
- Every later audio task reuses the same producer and anchor pattern instead of a cost shadow.
- A cycle-exact CPU model of a scene remains possible later work. It is not required for
  audio acceptance, and anchors must be replaced, not mixed, if one is added.

## Alternatives rejected

- **Cost shadow of every scene** (the AUDIO-TITLE-MENU method): exact, but roughly a second
  transcription of the race engine's control flow per instruction, the translation-style code
  the user rejected for native code, and repeated for every mode.
- **Executing original CPU code for timing**: rejected by the user for the product (D-0008).
- **Captured timestamps or events as product inputs**: excluded by D-0009.
