# DSP-only dependency

This directory contains the six DSP files from
[bsnes `7d5aa1e`](https://github.com/bsnes-emu/bsnes/tree/7d5aa1e656b9171524d01b1b22917197d8121cb4/bsnes/sfc/dsp),
the embedded snes_spc 0.9.0 DSP implementation. `upstream.json` records the
full revision and each unchanged source file's SHA-256. There are no CPU,
cartridge, uploaded sound program or original instruction interpreter sources.

`SPC_DSP.cpp` preserves Shay Green's 2007 copyright notice and declares
LGPL-2.1-or-later. `COPYING.LESSER` is the
[GNU LGPL version 2.1 text](https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt).
This dependency and `adapter.cpp` retain that license; the project's MIT license
does not relabel them. This records the dependency arrangement and is not a
legal compatibility judgment or authority to publish a binary release.

CMake builds `unirally_dsp` as a separate shared library (`.dylib`, `.so` or
`.dll`). Native callers use the small C interface in `adapter.h`; no LGPL class
or internal state layout enters the native driver's interface. Source is
present alongside the build recipe, with no executable game data. Rebuild the
library with the normal CMake preset and the `unirally_dsp` target; applications
and tests link dynamically, so the hardware library can be replaced/relinked.
There is no static-link fallback. A future binary distribution must preserve
this source/license arrangement and review its distribution obligations.

The adapter supplies the upstream source's sole external configuration reference:
Gaussian interpolation (`cubic=false`). It shares the 64K RAM and echo-memory
buffer, matching R-0075's pinned `bsnes_dsp_echo_shadow=OFF`. It exposes clocks,
registers, memory and native snapshots, and has no host audio callbacks or game
time. At most 32 DSP clocks are submitted per call; generated signed 16-bit
stereo samples are drained immediately. Version-1 snapshots retain shared RAM,
DSP phase, BRR decoding, envelopes, noise and echo histories. They are adapter
snapshots, not an SPC file format or captured emulator save state.

The six imported files remain byte-for-byte unchanged. Project warnings are
not imposed on the external source; configured address/undefined sanitizers
still instrument the shared library. Required platform checks must establish
actual behavior before adoption is accepted.
