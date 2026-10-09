# D-0001 — Reference emulator: bsnes libretro core as the primary adapter

Status: proposed by task M0-03 on 10 September 2026, revised 11 September 2026 after four independent reviews; becomes accepted when M0-03 is accepted.

## Decision

The laboratory's primary reference adapter drives a pinned **bsnes** libretro core (upstream commit `7d5aa1e656b9171524d01b1b22917197d8121cb4`, after v115) through Python `ctypes`, with a small tracked patch that adds laboratory exports. **Mesen Community Edition** (commit `20f497c9620a7f4e55b7752e1d22734f9d7492fa`, 2.2.1+83) is retained as an interactive investigation tool; its core library can also be driven headlessly, and the recipe is recorded, but no adapter is maintained for it. Both are pinned in `tools/locks/emulators.json`.

## Context

M0-03 had to prove, for the identified PAL ROM, headless load and identification, deterministic initial memory and reset, bounded input delivery, capture of memory/registers/trace without a GUI, save/restore, repeatability in fresh processes and useful failure status. Both candidates were built from source on the macOS arm64 development host and driven from Python. Evidence is in [the task record](../../tasks/M0-03.md) and [R-0002](../research/R-0002-reference-adapter-determinism.md).

## Comparison

| Property | bsnes libretro core | Mesen core library |
| --- | --- | --- |
| Build | `make target=libretro`, 8 s wall on 12 cores, no third-party dependency | `make core`, 39 s wall; needs SDL2 dev files; upstream makefile fails on paths containing a space (worked around with a `CXXFLAGS` override) |
| Programmatic interface | Synchronous libretro C API: `retro_run` advances exactly one frame on the caller's thread | C interop meant for the .NET GUI; emulation runs on its own thread, frame stepping goes through the debugger (`Step`) and completion must be polled |
| Determinism controls | Core option `bsnes_entropy=None` gives all-zero power-on RAM; serialized states still carried a clock-derived seed until reseeded (patched) | `SnesConfig.RamPowerOnState=AllZeros`, set by passing a 10032-byte struct whose layout must mirror the C++ header |
| Memory/register access | Not in the libretro target (`retro_get_memory_data` returns null); added by the tracked patch, which also serializes gamepad state (upstream leaves controller ports out of save states) | Available (`GetMemoryState`, `GetCpuState`, trace logger, input overrides) once a debugger is attached |
| Cold-start repeatability (300 frames, 3 fresh processes) | Identical WRAM, registers, video and audio every frame | Identical WRAM and registers with scanline-boundary steps; with PPU-dot-count steps the master-clock reading jittered on 2 of 300 frames |
| Save/restore | With `Strict` synchronization and the patch's gamepad serialization, restore in a fresh process continues with identical WRAM/registers at every one of the 898 save points of the two tracked scripts, including saves taken while a button is held; saving itself alters the continuing run at 59 boot-sequence points (29, 33, 38–39, 41–43, 45–96) under either method, which the tool detects; with bsnes' default `Fast` method restores silently diverged at 2 of 3 boot-script points tried. Video of the first restored frame and the resampled audio stream are not part of the state | Restore in a fresh process continued identically at the one point tried (65680-byte state) |
| Throughput | 300 frames in 0.7 s including full-WRAM hashing every frame | 300 frames in 6.1 s: each debugger step costs about 20 ms of thread hand-off regardless of the maximum-speed flag |
| Failure behaviour | Hung or runaway runs are killed by the bounded runner (exit 4); loading and API mismatches are reported before any run | Exiting without `Stop()` crashes the emulation thread; a step request that lands inside the debugger's break window deadlocks unless the caller also polls `IsPaused()` |

## Consequences

- Reference captures for M0-04 onward use `python3 tools/project.py reference ...` and the script format in `tests/manifests/reference/`. Inputs are delivered per frame through the libretro input callback; sub-frame input timing is not available and must not be claimed.
- Comparisons after a save/restore cover memory and registers only. Video and audio digests are valid within an uninterrupted run.
- A save state is a valid reference origin only when `restore-check` at that save point passes all three required checks, including `save_does_not_perturb`; M0-04 must record that check with every stored state. The adapter uses bsnes' `Strict` synchronization method; `Fast` is available for experiments but its restores can diverge without any sampled frame differing. `restore-check` samples every frame from the save point to the end in all three of its runs, so its verdicts have per-frame resolution regardless of the script's `sample_every`; the worker also always samples the save frame, the following frame and the resume frame in ordinary runs.
- Save states (`retro_serialize`, 290457 bytes for this ROM with the patch's gamepad state) are specific to the pinned core build; the sidecar records the core, ROM, script and method digests and the worker refuses a state from any other combination. A cold-start input script remains the regenerable form.
- Each run must use a private save directory: the core writes cartridge RAM to `<base name>.srm` at unload and reloads it at the next load, which changed the outcome of a later run at frame 228 before the fix.
- The core runs with `bsnes_ppu_fast=ON` and `bsnes_dsp_fast=ON` (upstream defaults). Whether the cycle-accurate PPU changes any gameplay-visible state for this game is unverified; switching would change the reference and must be recorded through the lock and scripts.
- Only the macOS arm64 build of the core is verified. A Linux build of the pinned core is the natural next infrastructure step (ROM-free, so it can run in CI). (Superseded 9 October 2026: see "Core identity across hosts".)

## Core identity across hosts (9 October 2026, PORTABLE-CORE-IDENTITY)

This decision pins the core by its source, the commit and the tracked patch above. The laboratory
had read that as one binary: 16 tools compared the running library's SHA-256, or the one a stored
capture recorded, with the macOS library `e59bf88d...`. A library built from the same source has another hash on another host
(Linux x86_64 and aarch64, GCC 13) and even on this Mac today (Apple clang 17.0.0 under macOS 27
gives `3f092254...`; Homebrew clang 19 gives `ee7bb1b4...`), so every other build was refused
although it behaves identically.

The reading now matches the text: a library is the reference core when `reference build` built it
from the lock's commit and patch, or when it is listed in `tools/locks/verified-cores.json` after
its outputs were checked. The library hash stays in every record as provenance. Equal behaviour is
shown by output pins, not by the hash: the pack rules' SHA-256 of the landing matrices extracted by
running the core, the frozen gates, and `core_check`, which replays a stored capture's inputs and
compares every frame's work RAM, cartridge RAM and video hashes against a capture another verified
library made. The frozen gates' inventories keep the library of their captures, so they read the
stored Mac captures on any host; a capture made again elsewhere needs a refreeze. Save states remain
tied to the exact library that wrote them.

## Alternatives considered

- Mesen as primary: richer debugger and trace logger, but the threaded architecture makes every frame step an inter-thread negotiation, its interop structs have to be mirrored byte-for-byte, and two failure modes (exit crash, break-window deadlock) had to be diagnosed before it produced results. It remains the better tool for interactive investigation of suspected reference errors.
- Unpatched bsnes with state-blob parsing instead of exports: rejected; the serialized layout is version-specific and gives no trace hook.
