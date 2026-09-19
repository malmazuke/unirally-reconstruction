# Build, reference execution and validation

This is the M0 implementation specification. Commands marked implemented in the stable-command table exist in `tools/project.py`; the separately listed research modules use `python3 -m`. Other commands remain proposals until a task record shows them running.

## Validation by stage

[D-0006](decisions/D-0006-capability-driven-work.md) changes when checks run, not
what constitutes evidence. Historical frozen expectations and acceptance remain.

| Stage | Required work |
| --- | --- |
| Startup | Verify source/input identities and reproduce the focused accepted foundation. Bootstrap missing local prerequisites; reuse verified downloads/build caches. |
| Experiment/edit | Run affected unit/component tests and native differential comparisons. Use targeted sanitizers for risky native changes. Record first divergence, reached branches and captured inputs removed. No broad matrix per checkpoint. |
| Review candidate | Freeze a coherent commit; run focused checks, declared native primary/variation/restores and affected regressions. A fresh reviewer independently reproduces the behavior and chooses withheld cases targeting relevant branches; whole-document hashes alone do not validate semantics. |
| Final integration candidate | Run complete app-debug and app-sanitize suites, declared private native/replay/content/pack/presentation gates and hosted macOS/Linux CI on the exact source candidate. Required skipped/missing tests are non-passes. |
| Corrections | Repeat affected checks; rerun the full integration matrix if tested code, build/config, inputs or expectations changed. Documentation-only successors may cite the tested code/input identity with an inspected diff; obtain hosted CI for the final pushed tip. |

For M4-13 through M4-16, send the focused passing candidate for initial review before running
the broad matrix. Resolve its findings, then run broad validation on the corrected
candidate alongside focused re-review where useful. Later source changes still
require revalidation. Existing CI triggers are unchanged: pushes (including docs)
run CI automatically; avoid optional preliminary task-branch pushes unless the
task's declared remote review/CI workflow needs them. Final integration CI remains
required. Synthetic Linux CI does not prove private ROM differential execution
on Linux; report these domains separately.

Use [consolidated closeout](AGENT_WORKFLOW.md#consolidated-closeout): prepare the
handoff before the final main push, then record final remote/CI evidence in the
ignored closeout report. One final-tip CI is the target, not permission to skip
checks after a substantive correction. Docs-only pushes still trigger CI.

The primary owns integration checks; do not ask both worker and coordinator to
repeat identical broad suites without a changed candidate or a specific concern.
Independent reference reproduction and withheld cases are not redundant checks.
Retain proof of clean installation at milestone/schema/content-interface changes;
a fresh clean build is not required for every unchanged research checkpoint.

M4-12 now supplies the bounded native trial runner documented below. Earlier
research modules remain evidence tools. M4-13 adds bounded B-jump cases; M4-14
adds the sustained horizon/state/restore laboratory below. Existing M4-12/M4-13 commands do not accept
arbitrary new horizons or scenarios. Freeze exact fields, horizon, source/seed, static inputs and
validation commands before using them as acceptance gates.

## M4-15 audit preflight and validation ledger

Implemented and independently reviewed in M4-15 by `zoom_zoo_race_audit` and
`zoom_zoo_race`; accepted at `8bc2e71` with exact-tip CI and private synchronization.
Before expensive instruction capture, automatically compare the expanded actual
controller timeline (both controllers, pre-seed history and complete horizon)
against the frozen scenario. Fail on differences rather than inheriting legacy
input pulses. Check supplemental capture whole-WRAM hashes against the matching
frozen reference at every sampled frame before using its audit as evidence.
Timeline preflight precedes capture; WRAM authentication follows capture.

Use a small ignored JSON/table ledger recording command, result, source identity,
binary hash, build configuration/toolchain, ROM/core/static/input/contract hashes,
coverage and output report identity. Reuse an unchanged result only when the
relevant identities match and its evidence is available; explain documentation-only
source differences. Invalidate affected entries after changes. No cache service
or general orchestration framework is required. This does not eliminate fresh
independent review/references, withheld replacements, sanitizer runs, required
exact-merge checks or final-tip CI. Never count missing evidence as a reused pass.

## M4-16 product validation requirements

[M4-16](../tasks/M4-16.md) assigns native initialization and live frontend/result
validation. The task branch now has experimental native start/frontend/result
interfaces; main remains the accepted M4-15 product until M4-16 acceptance.
Extend the accepted race/visual tools deliberately. Keep validation source and
binary immutable for an entire run and use the identity ledger to reject mixed
results. Collect initial review findings before broad validation; supersede
results after relevant changes. Verify task-local fixture paths before launching
expensive matrices. Preserve independent, sanitizer and exact-merge evidence.

Use M3-04's real-window/clean-pack acceptance practices, adding ZOOM ZOO native
start, correct winner/loser result and restart. Real input events and visible
state are required; fixed-controller or replay-only modes cannot prove live
controls. Synthetic Linux CI remains distinct from private/live desktop evidence.

## Environment and dependencies

Start on the current macOS machine with Linux as the first additional build/test target. Windows is a later release target, unless the user prioritizes it earlier. Test natively on the relevant architecture; cross-compilation alone is not evidence of runtime determinism.

Pin compiler/tool versions, emulator commit and patches, Python dependencies and frontend dependencies in tracked manifests. Keep machine-local paths in ignored configuration. Bootstrap must be idempotent, bounded by timeouts, detect unsupported prerequisites, and avoid modifying global settings merely to get a green run. Store fetched dependencies in a dedicated cache and record their origin/checksum.

Use a headless build with no window/audio requirement for core checks. Containerized Linux is useful for repeatability, but a container must not be a prerequisite for the native macOS tools. Cache downloads/builds; do not redownload or rebuild everything for every agent. Verify the clean-setup path in an isolated environment before describing it as reproducible.

### Worktree cache and report layout

M4-12's corrected layout keeps `local/` and `artifacts/` as real directories
inside each checkout, not top-level symlinks into another checkout. Create them
with `mkdir -p local artifacts` only after checking existing paths. Do not remove
or replace another agent's directories. Under `local/`, verified cache/toolchain/
emulator subdirectories may be linked to existing caches; copy the small ROM
locator and use explicitly located private fixtures. Treat shared caches as
read-only during concurrent work; write captures/reports in this checkout.
Do not share mutable output directories between primary and reviewer.

Use a fresh `artifacts/<task>/<run-id>/` directory for each capture or native
command requiring fresh output, and place its `--report` inside that same run
directory. Check the command's help: some commands require the directory not to
exist yet, so create only its parent. Resolve paths before running; reports must
not escape the allowed artifacts root or overlap input/capture filenames.
Prefer documented harness commands over proliferating one-off scripts. Check
free disk before large captures; never delete unrelated content to make room.
See [R-0031](research/R-0031-m4-12-retrospective.md) for the observed setup failures.

### Reference emulator selection spike

Evaluate a pinned bsnes core for programmatic execution and Mesen Community Edition for investigation. Do not assume either offers every needed headless API out of the box. Prove:

1. Load the supplied ROM and identify its exact bytes.
2. Set deterministic initial persistent memory and reset conditions.
3. Deliver inputs at a defined point and advance a bounded number of updates.
4. Capture memory, relevant registers and trace data without human clicks.
5. Save/restore and repeat the same experiment in fresh processes.
6. Exit with useful status and artifacts on timeout, crash or mismatch.

Select one primary adapter based on this spike. A second emulator is useful to investigate suspected reference errors, not a mandatory full matrix for every change. Interactive debugging can support exploration; automated acceptance must run without an unattended agent steering a GUI.

## Proposed repository shape

| Path | Contents |
| --- | --- |
| `src/core/` | Native simulation with no UI or network dependency |
| `src/app/` | Desktop input/render/audio integration |
| `tools/` | Bootstrap, ROM inspection, extraction, reference adapter and comparison tools |
| `tests/synthetic/` | Authored fixtures and checks runnable without game data |
| `tests/manifests/` | Replay identities, schema, provenance and regeneration instructions |
| `docs/research/` | Validated findings and linked experiments |
| `docs/decisions/` | Architectural decisions and superseded alternatives |
| `tasks/` | Work orders and durable handoffs |
| `local/` | Ignored ROMs, generated Classic content packs, loose extracted assets, persistent data, private test fixtures and runner state |
| `artifacts/<run-id>/` | Ignored logs, snapshots, traces, reports and visual diffs |

Directories are added when there is an implementation to put in them. Present after M0-02: `tools/`, `tests/tooling/` (Python checks for the tooling itself), `tests/synthetic/`, `tests/manifests/rom/`, `docs/research/`, `src/lab/` (a synthetic determinism probe used to validate the toolchain and reports; it is not game code), `.github/workflows/` (ROM-free CI) and `tools/locks/` (pinned tool artifacts). Added by M0-03: `tools/unirally_lab/reference/` and `tests/manifests/reference/` (reference scripts). Added by M0-04: `tools/unirally_lab/replay/`, `tools/unirally_lab/compare/` and `tests/manifests/replay/` (replay manifests); local-only states and their restore-check reports live under ignored `local/states/`. Added by M1-01: `tools/unirally_lab/coverage/` (trace drain, 65816 opcode table, LoROM mapping, map derivation) and `docs/map/` (tracked observed code maps and summaries per scenario). Added by M1-02: `tools/unirally_lab/access/` (65816 effective-address decoder, streaming access derivation, capture and query commands), `tests/manifests/fields/` (validated field declarations) and `docs/state/` (the player-state schema).

M2-01's accepted component foundation added `src/core/` (collision-point
expansion, spatial sampling and progress recurrence), `tests/native/` (four
ROM-free authored C++ checks plus two ROM-dependent probe executables),
`tools/unirally_lab/native/` (reference-freeze utility and isolated component
probes), and `tests/manifests/native/` (frozen reference projections and static
content provenance). Those initial components did not by themselves implement
a rider update or a complete simulation; the accepted movement and later
milestones build on them. See [R-0010](research/R-0010-native-movement.md) and
the [source guide](../src/core/README.md) for exact units and supported bounds.

## Stable command contract

The repository CLI is `python3 tools/project.py`, so each model/runtime can use
the same implemented entry points. The wrapper invokes standard tools instead
of reimplementing a build system; rows still marked proposed are unavailable.

| Subcommand | Status | Contract |
| --- | --- | --- |
| `doctor` | implemented (M0-02) | Report installed/pinned versions, platform and missing capabilities; no game inputs required |
| `bootstrap` | implemented (M0-02) | Prepare isolated dependencies from `tools/locks/toolchain.json`; safe to repeat; emit resolved-version manifest under ignored `local/toolchain/` |
| `rom inspect --path <path>` | implemented (M0-01) | Hash original input, identify header/mapping/region candidates and emit manifest; do not silently normalize bytes; `--expect` rejects another revision |
| `build --preset <name>` | implemented (M0-02) | Configure/build the specified CMake preset with the isolated toolchain and record exact configuration in `build/<preset>/lab-build-info.json` |
| `test --suite synthetic` | implemented (M0-02) | Run ROM-free checks (Python tooling tests, ctest, fresh-process repeatability) and produce machine-readable results |
| `reference build|run|verify|restore-check` | implemented (M0-03) | Build the pinned core from `tools/locks/emulators.json`; run a reference script in a fresh worker process; repeat it in fresh processes; save, restore and compare the continuation (D-0001) |
| `replay validate|run|compare --manifest <manifest>` | implemented (M0-04) | Validate a replay manifest (schema 1, ROM-free); reproduce its reference run in a fresh process; compare fresh-process runs of one manifest (or two manifests) on the declared fields and emit the first divergence with prior sample, inputs, field values, a work RAM localization and trace windows. Supersedes the proposed `reference capture --case` / `compare --case` for the reference side; native/reference comparison is added when native code exists (M2) |
| `coverage capture --manifest <manifest> --out <dir> [--ring N] [--frame-image N ...]` | implemented (M1-01) | Run a replay manifest in a fresh worker process with the core's instruction trace ring drained once per frame (the pinned core and patch are unchanged); write `coverage.json` (per executed site: 24-bit pc, E/M/X mode, data bank, count, first frame; per consecutive site pair: count), the usual samples, optional PNG frame dumps, and check that the run's digests equal the manifest's, that no frame exceeded the ring, that the instruction totals agree and that the first instruction is the emulation reset vector target |
| `coverage map --coverage <file> --out <map> [--summary <md>] [--detail <json>] [--baseline <map>]` | implemented (M1-01) | Derive the tracked code map from a coverage file and the ROM at `local/rom-location.txt`: opcode/operand byte classification under the observed modes, LoROM offsets, executed ranges per bank, vectors with execution counts, entry points with edge kinds, static absolute/long references, sites outside ROM; with `--baseline`, what this scenario executes that another does not. Tracked outputs carry no ROM bytes; `--detail` writes the per-address (opcode-bearing) artifact under ignored `artifacts/` |
| `access capture --manifest <manifest> --out <dir> [--from-frame N] [--to-frame N] [--watch-address A ...] [--watch-pc P ...] [--wram-series-range START LENGTH [--wram-series-every N]] [--frame-image N ...]` | implemented (M1-02) | Run a replay manifest in a fresh worker process and derive, from the unchanged trace ring and the ROM bytes at each pc, every instruction's memory accesses over the frame window (D-0002): effective address, kind (read, write, rmw, push, pull, block_read, block_write), width, stored value; aggregated per (pc, mode, addressing, kind, width, address) for work RAM and registers and per (pc, mode, addressing, kind) with an address range for ROM reads; indirect pointers and work RAM code resolved from recorded stores or the frame's work RAM images and labelled, the residual counted per (pc, addressing); per-frame value logs of watched addresses, register logs at watched pcs, a DMA/HDMA parameter log, an optional binary work RAM series (every frame of the run on the stride); `access.json` (schema 1) and the usual samples; checks that the run's digests equal the manifest's, that no frame exceeded the ring and that the instruction totals agree |
| `access query --access <dir>/access.json (--address A | --pc P) [--kind K] [--json]` | implemented (M1-02) | List the access records covering an address (work RAM mirrors match, multi-byte spans included) and/or made by a pc, with the writer and reader pcs; exit 1 when nothing matches, 2 without the file, 3 for bad arguments |
| `content provenance --access <dir>/access.json --out <dir> [--from-frame N] [--to-frame N]` | implemented (M1-03) | From an access record: every MDMAEN store expanded per enabled channel (frame, sequence, pc, direction, transfer mode, B-bus register, A-bus bank/address and region, size, ROM file offset) paired with the VRAM/CGRAM/OAM address in effect when the record watched `$2115`-`$2117`, `$2121`, `$2102`/`$2103`; every block move through `$00:0199` from its watched register log (grouped by the count register: source/destination offsets, length, destination bank, source banks or the ROM range); the VRAM and CGRAM bytes written through the data ports when `$2118`/`$2119`/`$2122` were watched; `provenance.json` (schema 1); checks that the MDMAEN count equals the record's `dma_triggers` and (optional) that every pairable transfer was paired |
| `content decode --manifest <manifest> --out <dir> [--rom P] [--wram-dump P]` | implemented (M1-03) | Decode every item of a content manifest (schema 1: `raw` pieces by file offset and length, or `rnc` by source bank/address through the port of the ROM's `$81:B8E2` decompressor) from the ROM, write the bytes to the ignored output directory, and check length and SHA-256 against the manifest; with a work RAM dump, compare items that carry `runtime.work_ram_offset` byte for byte, tolerating only the listed `known_runtime_writes`; exit 1 on a mismatch, 2 without ROM/manifest/dump, 3 for an invalid manifest |
| `content compare --manifest <manifest> --access <access.json> [--access <more>] --frame N --frame-image <png> --oam-dump <wram.bin> --scroll-dump <wram.bin> --out <dir> [--rect X Y W H] [--upload-frame N] [--bg1-scroll H V] [--bg2-scroll H V] [--colour-order bgr|rgb] [--max-mismatch-fraction F]` | implemented (M1-03) | Rebuild VRAM, CGRAM and OAM as the original had them at frame N (decoded items at their VRAM/CGRAM positions; ROM-to-VRAM DMAs and the tilemap staging DMAs of the record replayed with the previous frame's work RAM series; CGRAM/OAM port writes from the watch logs; scroll from the HDMA tables in the dump), render BG1, BG2 and sprites with the core's colour conversion, and compare pixel for pixel with the frame image over the rectangle; writes the render, the diff and a side-by-side PNG; reports the mismatch count and the omitted PPU features; fails only when `--max-mismatch-fraction` is exceeded |
| `content pack --rom <path> --out <ignored-pack>` | implemented (M3-02A/M3-02 accepted) | Verify the exact supported PAL ROM before extraction, reproducibly create the current 25-entry Classic pack (13 gameplay and 12 presentation entries), and atomically write a schema-1 local pack with logical IDs, entry hashes and source/extraction-rules/profile/start identities; refuses overwrite and removes an interrupted temporary output |
| `content pack-inspect --pack <path>` | implemented (M3-02 accepted) | Validate pack magic/schema, source/rules/profile/start identities, canonical layout and every required logical entry size/SHA-256 without opening a ROM; corruption, missing/duplicate entries and incompatible identities are invalid input |
| `native compare --manifest <native case> [--from-frame N] [--to-frame N]` | implemented (M2-01 accepted) | Build `movement_runner`, run it twice in fresh processes from the identity-bound semantic seed using only static content and replay controller masks, verify unchanged inputs and canonical-state determinism, and compare all 13 required projections with the frozen reference; primary and two withheld cases pass through frame 2999 |
| `native restore-check --manifest <native case> --save-frame N [--save-frame N ...]` | implemented (M2-02 accepted) | Run one reference-checked uninterrupted native process, then for each interior boundary reproduce its prefix, persist the exact333-byte canonical state and resume its suffix in another fresh process; require every projection and canonical state to equal the uninterrupted series, with all inputs rehashed between processes |
| `native finish-check --manifest <full-race case> [--save-frame N ...]` | implemented (M3-01 accepted) | Run the full-race native producer twice, require the canonical `URMV0001` to `URMV0002` transition and exact frozen gameplay/finish/result state through the first stable result frame, and optionally require restored suffixes to equal uninterrupted execution |
| `native opponent-first-check --manifest <case> --content-pack <pack> [--save-frame N ...]` | implemented (M3-04 accepted) | Run the neutral-after-1533 native producer twice using only the validated pack, compare the identity-bound full opponent x/y/velocity/pose plus player/timer/finish projection through frame 3999, and optionally require restored suffixes to equal uninterrupted execution |
| `native presentation-check --manifest <presentation contract> --fixtures <ignored fixture directory> --content-pack <pack>` | implemented (M3-02 accepted) | Build the pack-only headless renderer, identity-check each private canonical state/reference PNG named by the tracked contract, render each case in a fresh process, and fail/report its exact regional pixel mismatch against the frozen threshold; fixtures must remain below ignored `local/` or `artifacts/` |
| `frontend run [--track dragster\|zoom-zoo] [--pack <ignored pack>] [--rom <supported ROM>] [--replace-pack]` | implemented (M3-03 accepted; pack selection by profile since CLASSIC-PRESENTATION-UNIFICATION) | Launch the SDL3 app on a pack selected by the profile recorded inside it: a typed `--pack` must validate against the supported profile and is refused otherwise (naming both profiles and the remedy); without `--pack` the newest valid pack under `local/` is used; when none exists, exact-gate the explicit ROM, atomically create the pack at the profile's own path and revalidate it, then launch; `--rom --replace-pack` moves an incompatible existing pack aside first. Before launching, the app's `--supported-profiles` are compared with the rules' profile so a stale build is reported with the rebuild command. Existing corruption, cancelled/missing/wrong ROM, absent executable and failed/timed-out app launches are non-success outcomes. Audio is explicitly omitted. |
| `verify --task <id>` | proposed | Run that task's declared checks, validate required artifacts and report eligibility for review |
| `package --preset <name>` | proposed | Later: assemble a runnable build with dependency notices and no unintended local inputs |

All commands must have bounded execution, useful help, noninteractive operation and a `--report <path>` option. Exit codes as implemented: 0 success, 1 check failure, 2 missing prerequisite, 3 invalid input, 4 timeout. Reports include individual passed/failed/skipped checks. A required skipped check prevents task acceptance even if unrelated checks pass. A bare zero exit status must never conceal missing ROM tests.

Use an agreed JSON report schema containing run ID, task ID, source commit, dirty-diff digest if applicable, tool versions, input hashes, command, elapsed time, check outcomes and artifact hashes/locations. A check result applies only to the exact recorded source/input state. The schema is implemented in `tools/unirally_lab/report.py` (schema version 1): each check has an outcome of `passed`, `failed`, `skipped`, `missing` or `timeout` and a `required` flag; a run's `status` is `passed` only when every required check passed.

## Isolated native research modules

These are invoked with `python3 -m tools.unirally_lab.native.<module>`, outside
the stable CLI. They do not provide `tools/project.py native compare`.

`zoom_zoo_contact` is the M4-05 captured-argument research component. Its
`capture` command records the bounded original contact inputs, `extract-content`
derives exact-gated static inputs independently from the supported ROM and
accepted M4-03 contract, and `verify` checks all 102 ordered calls plus the first
non-flat branch neighbourhood against a tracked reference manifest. It does not
implement autonomous movement or production ZOOM ZOO support.

`zoom_zoo_vertical_contact` is the M4-06 consumer of those access documents.
It intentionally reuses `zoom_zoo_contact capture` because that capture's watch
set contains the complete vertical-contact landmarks. Its implemented commands
are `extract-content`, `verify` and `compare-inputs`; it has no separate
`capture` command. It evaluates captured incoming arguments against
independently extracted content and does not provide autonomous movement or
production ZOOM ZOO support.

`zoom_zoo_position` is the M4-08 stateful research component. Its `capture`
command freezes end-of-frame 1649 plus every instruction and publication of
`$82:A627--A6F7` through frame 1700; `extract-routine` authenticates the 209
original bytes; `verify` propagates four signed residues across all 102 ordered
calls while position, velocity and contact state remain captured inputs; and
`compare-inputs` reports the preregistered frame-1662 variation's controller,
component and branch divergence. It has no stable `tools/project.py` interface
and does not provide autonomous movement, a wrap-crossing claim or production
ZOOM ZOO support.

| Module | Implemented contract and limits |
| --- | --- |
| `freeze_reference --manifest <replay> --samples <first> <second> --out <new file>` | Administrative reference-only projection of two matching fresh-process captures; refuses overwriting an existing output. Dedicated freeze commit precedes native computation. This utility emits a projection rather than a standard check report; it does not validate a native result |
| `probe_sampling --access <capture> --content-manifest <manifest> --content <dir> --probe <sampling_probe> --coarse-width 1024 --report <json>` | Identity-checks the primary capture/static content and compares ten sample words per call for both riders on frames 1534–2999. Incoming position/pose arguments come from the capture. Emits report and native stdout artifact; agreement validates only this component |
| `probe_progress --sampling-output <native.txt> --sampling-report <report> --series <wram.bin> --series-access <capture> --content-manifest <manifest> --content <table.bin> --probe <progress_probe> --report <json>` | Validates native sample-output identity and primary series provenance; seeds progress once at 1533 and compares marker/tag/count/rejection for both riders through 2999. Explicit phase and 15-byte state round-trip every frame. Samples still depend on captured positions/poses; this is not autonomous movement or M2-02 acceptance |

The two probes report execution/protocol failure as 1, missing files as 2,
invalid experiment input as 3, and timeout as 4. Their exact regeneration and
validation commands and artifact identities are in R-0010. Extracted tables
and original captures stay ignored; missing content is a prerequisite failure.
The existing `build --preset lab-debug` and `test --suite synthetic` commands
include the authored tests in `tests/native/` and Python tooling checks without
a ROM. `build --preset lab-sanitize` builds the same C++ component tests with
sanitizers. Historical task records preserve the exact clean-source test counts,
CI runs and independent reviews for each component checkpoint. Component output
hashes are not full movement-state hashes; only later accepted whole-simulation
gates make gameplay claims.

## ROM and replay identity

The selected region is PAL Unirally. Choose its exact ROM revision before baseline capture. Record SHA-256 and file size, region, mapping and any header handling; retain original and normalized hashes separately if normalization is necessary. Do not infer region solely from the filename or hard-code NTSC timing. Persistent data, configuration and emulator version are part of the experiment.

Each replay manifest records (implemented as `tests/manifests/replay/*.json`,
schema 1, validated by `tools/unirally_lab/replay/manifest.py`; accepted native
comparison/finish/restore paths use their task-scoped manifests, while broader
replay support remains bounded to the listed implemented commands):

- Scenario ID and tested behavior; ROM/emulator identity and adapter schema.
- Initial reset procedure or snapshot hash, SRAM/configuration hashes and RNG state if known.
- Both controllers' inputs, sequence length, timing units and exact injection/sampling point.
- State-field schema, address mapping and signedness/scales; expected events.
- Canonical reference artifact hashes, regeneration command and storage location.
- Native rules/content versions, expected outcomes and any explicitly justified tolerance.

Save states are emulator-version-specific artifacts. Keep a cold-start input path as well, so a state can be regenerated after a tool migration. Large captures remain local/private and content-addressed. Track a compact manifest that lets another authorized host reproduce or obtain them. A missing fixture is a dependency, not a new expected result invented by the worker.

## Comparison and evidence

Compare normalized gameplay state, not whole-memory byte equality between unrelated implementations. Whole-memory snapshots can aid investigation, but native data layout will differ. M1 defines fields and sampling phase: position, velocity, track progress, rider/trick state, timers and race events as actually discovered.

Use exact comparison for discrete and fixed-point gameplay values. Express any tolerance by field, unit and reason before acceptance; don't enlarge it after a failing run without a reviewed explanation. Identify the first divergent update, prior state, inputs, field values and associated original trace window. Reduce failing recordings where useful.

The reference is also software: verify repeatability and confirm critical discoveries with a second experiment or independent inspection. When native and reference differ, do not assume the native implementation is always at fault.

Visual checks compare native-resolution frames using agreed layer/color conventions and separate display scaling. Audio checks need an explicitly aligned sample/capture method and a defined tolerance for backend differences; early milestones can declare audio out of scope. A screenshot match alone is not gameplay equivalence.

### Test progression

| Stage | Checks |
| --- | --- |
| M0 | Clean setup; repeated reference capture; deliberately changed input; comparator/report failure path; timeout and missing-prerequisite handling |
| M1–M2 | Evidence-linked arithmetic/decoding checks; primary and withheld replays; fresh-process repeatability; save/restore continuation |
| M3 | Whole-track replays; boundary inputs around jump/trick/landing; collision edge cases; controls and render capture; deterministic ROM-to-pack extraction; corrupt/outdated pack rejection; archive-only relaunch with the ROM absent; sustained bounded run |
| M4 | Coverage by track/mode/player count; AI/RNG seeds; progress persistence; audio; cross-platform replay and release smoke checks |
| M5 | Track import/export round-trip; malformed content and resource limits; texture replacement preserves state hashes; editor create-to-play workflow |
| M6 | Two-client state agreement; reorder/loss/jitter; rollback restoration; version/content mismatch; disconnect and recovery |

Reviewers own at least some withheld input cases. Their expected outputs come from the frozen reference, not the candidate implementation. Add meaningful regressions for discovered mechanics and bugs; avoid tests that merely restate the implementation.

## CI and release evidence

Separate public/ROM-free CI from trusted fixture runs. Public CI can compile and test authored fixtures. A trusted local/private runner uses the supplied ROM and reproduces the differential suite. Required private results must attach to the exact commit being accepted. Untrusted pull requests must not run automatically on a host containing private fixtures or credentials.

Under [D-0005](decisions/D-0005-classic-content-distribution.md), public CI also
tests the content-pack reader/writer and mutation failures with authored data.
The trusted runner proves that the supported PAL ROM produces the frozen pack
entry hashes. M3 release evidence includes both a clean installation that
creates a pack from the supplied ROM and a subsequent launch with only that
validated pack available. Neither the ROM nor the generated Classic pack is a
CI artifact exposed to untrusted jobs.

The M3-02A native full-race path accepts `native finish-check --content-pack
<pack>`. In that mode it validates the pack before building or starting a
producer, selects the public semantic start
`classic.crawler.dragster.race-start.v1`, and passes only the pack, controller
stream and later canonical restore states to `movement_runner`. The legacy
directory/seed runtime remains available for research regressions.

The additive M4-09 research command
`python3 -m tools.unirally_lab.native.zoom_zoo_composition` implements
`capture`, `derive`, `verify` and `compare-inputs`. It composes the accepted
position/contact equations over the identity-bound frames 1650--1700, seeds
both riders' eight position/residue words once at end-1649, and derives all
1,020 track sample words and source offsets from recurrent positions plus
authenticated content. It remains a bounded reference-analysis surface;
velocity, pose, reflection and inventoried non-position contact state are
captured external inputs, and it does not add native ZOOM ZOO gameplay.

The additive M4-10 research command
`python3 -m tools.unirally_lab.native.zoom_zoo_response_b` implements
`capture`, `derive`, `verify` and `compare-inputs`. It seeds both riders'
response-B words once at end-1649, executes the instruction-derived active
producer across the identity-bound frames 1650--1700, compares pose/contact
publication and feeds computed response B into the accepted M4-09 composition.
Its exact-bound manifests and ROM-free mutations cover phase/order, seed,
byte/word width and high-byte semantics. It remains a bounded reference-analysis
surface: velocity, pose and other contact fields are captured external inputs,
and it does not add native ZOOM ZOO gameplay.

The additive M4-11 research command
`python3 -m tools.unirally_lab.native.zoom_zoo_vertical_velocity` implements
`capture`, `derive`, `verify` and `compare-inputs`. It seeds both riders'
vertical-velocity words once at end-1649, executes every reached jump, gravity,
vertical-cap and contact publication through frame 1700, and supplies computed
velocity plus computed response B to the accepted position/contact composition.
Its exact-bound manifests and ROM-free mutations cover writer order, signed
wrapping, shifts, widths, short circuits and feedback integrity. It remains a
bounded reference-analysis surface: horizontal velocity, pose/reflection,
jump/control and remaining contact state are external inputs, and it does not
add native ZOOM ZOO gameplay.

If no remote or CI host exists, use the same scripts locally and record their results. Do not describe hosted CI as running until it exists. Integration reruns affected checks on the actual merge candidate; milestones require the broader declared suite. Use sanitizers where supported to expose memory/undefined-behavior defects, alongside replay checks in the release configuration.

Before an unattended run is considered reliable, demonstrate restart after interruption, a failed check reported accurately, and a task resumed from its persisted record. Build success is necessary but cannot substitute for reference comparison.

## Sources and limits

Consulted 10 September 2026; these establish available building blocks, not feasibility of the unimplemented adapter:

- [snesrev/sm](https://github.com/snesrev/sm): its README describes side-by-side execution, frame comparison and mismatch snapshots. This is precedent for differential validation.
- [bsnes](https://github.com/bsnes-emu/bsnes): candidate reference emulator. The project still needs a pinned, tested automation interface.
- [Mesen Community Edition](https://github.com/nesdev-org/MesenCE): multi-system emulator including SNES, with desktop builds. The original [Mesen2 repository](https://github.com/SourMesen/Mesen2) is archived and directs users to this fork; verify APIs against the selected revision.
- [CMake presets](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html): shared configure/build/test configuration and separate local presets.
- [SDL3 documentation](https://wiki.libsdl.org/SDL3/FrontPage): candidate frontend foundation.
- [GGPO](https://github.com/pond3r/ggpo): rollback implementation to study during the networking spike. No SDK choice or integration is committed by this plan.

## M4-12 experimental native ZOOM ZOO laboratory

This task-scoped module is separate from the accepted DRAGSTER CLI and frontend.
It admits Right/neutral controller-0 continuation from the authentic end-1649
seed through 1849. M4-13 subsequently adds B jump for its separately frozen
landing trials; other controls, modes and horizons fail closed. M4-12 acceptance
evidence, including independent cases and integration, is recorded in R-0030.
The original 394-byte frozen projection remains an unchanged prefix of the
395-byte canonical state; the suffix retains original opponent OAM X.

Build with `python3 tools/project.py build --preset app-debug`. Set `CORE` to
the pinned library path in the authenticated reference `samples.json` (a local
path, not a new emulator identity). Use a 180-second subprocess timeout for each
capture/extraction; native children enforce 30 seconds internally.

```sh
python3 -m tools.unirally_lab.native.zoom_zoo_trial_extract --core "$CORE" --out artifacts/m4-12/extracted-content
python3 -m tools.unirally_lab.native.zoom_zoo_trial capture --core "$CORE" --out artifacts/m4-12/trial-primary-a.json
python3 -m tools.unirally_lab.native.zoom_zoo_trial capture --core "$CORE" --out artifacts/m4-12/trial-primary-b.json
python3 -m tools.unirally_lab.native.zoom_zoo_trial compare --reference artifacts/m4-12/trial-primary-a.json --repeat artifacts/m4-12/trial-primary-b.json --binary build/app-debug/src/core/zoom_zoo_runner --content-dir artifacts/m4-12/extracted-content --out artifacts/m4-12/trial-primary-report.json
```

Outputs must be fresh. For independently preregistered cases, add `--case FILE`
to each capture and use separate outputs. Case JSON contains `id` and `changes`;
each change is `{ "from": 1700, "to": 1702, "buttons": [] }`, replacing controller
0 during those inclusive frames and returning to primary Right afterward. The
example is protocol documentation, not a withheld acceptance case. Overlapping
changes and seed/horizon edits are rejected. Comparison includes fresh native
restores at 1700, 1804 and 1823 and fails on any byte/frame mismatch. The native
child's temporary directory contains only canonical seed, controller rows and
authenticated static content; reference state never enters it.

`zoom_zoo_trial_reference.expanded_access_command(out)` reproduces the primary
full access/watch recipe, extending accepted M4-11 capture to 1849 and adding
persistent-state watches. It returns the existing `access capture` command
with a 600-second worker bound. The two original captures' identities are frozen
in `zoom-zoo-trial-primary.reference.json`. The generalization references use
fresh original processes with whole-WRAM hashes, all declared native fields and
excluded-mode guards on every frame. Private captures and extracted bytes must
remain ignored.

## M4-13 player landing laboratory

`tools.unirally_lab.native.zoom_zoo_player_landing` adds `capture`, `freeze` and
`compare` commands for the fixed 1649–1849 continuation with B jump plus
Right/neutral controls. See [R-0032](research/R-0032-player-landing-recovery.md)
for exact commands. `freeze` requires a full player landing and 100 subsequent
updates before native evaluation; `compare` binds every reference row to that
freeze and restores before/after every full player landing. The primary freeze
precedes implementation; its supplemental guard capture preserves every earlier
state/WRAM digest. M4-12 manifests and commands remain unchanged.

## M4-14 sustained traversal laboratory

`tools.unirally_lab.native.zoom_zoo_sustained` adds `extract-content`, `capture`,
`freeze` and `compare` for the 423-byte `URZZ0002` continuation. The frozen primary
is continuous Right through 3299, with recovery at 2185 and 1,114 subsequent
updates. See [R-0033](research/R-0033-sustained-traversal.md) for commands, source
boundaries and the conditional integration record. Variations must repeat/freeze reference before
native evaluation; restore comparisons cover both riders' full landings. Horizon
extensions are laboratory experiments, not general gameplay acceptance.

## M4-15 race-completion laboratory

The task-scoped `zoom_zoo_race_explore` module captures original-only fixed
controller cases, optionally retaining private WRAM/SRAM with `--keep-wram`.
Its marker-guided policy is exploratory only: successful policy output is
converted to a fixed controller timeline and repeated before native evaluation.
`zoom_zoo_race_reference` freezes matching original processes; the final primary
contract is `tests/manifests/native/zoom-zoo-race-primary-v4.freeze.json`.
Older additive freezes remain as inventory-discovery evidence, not interchangeable
current contracts. The race state is 565-byte `URZZ0003`.

```sh
python3 -m tools.unirally_lab.native.zoom_zoo_race_extract --core "$CORE" --out artifacts/m4-15/fresh-content
python3 -m tools.unirally_lab.native.zoom_zoo_race_explore --core "$CORE" --case tests/manifests/native/zoom-zoo-race-primary.case.json --horizon 6724 --keep-wram --out artifacts/m4-15/fresh-a.json
python3 -m tools.unirally_lab.native.zoom_zoo_race_explore --core "$CORE" --case tests/manifests/native/zoom-zoo-race-primary.case.json --horizon 6724 --keep-wram --out artifacts/m4-15/fresh-b.json
python3 -m tools.unirally_lab.native.zoom_zoo_race --reference artifacts/m4-15/fresh-a.json --repeat artifacts/m4-15/fresh-b.json --contract tests/manifests/native/zoom-zoo-race-primary-v4.freeze.json --binary build/app-debug/src/core/zoom_zoo_runner --content-dir artifacts/m4-15/fresh-content --out artifacts/m4-15/fresh-report.json
```

Use a 180-second subprocess bound per capture/extraction and a 900-second bound
for the complete comparison/restore driver (each native child enforces 30 seconds).
Outputs must be fresh. Comparison authenticates every original WRAM hash,
constant mode/camera guards, state row, expanded controller timeline and static
content item before launching native execution. It also checks the empty queue
branch invariant at the first finish-animation call; a new dependency is a
failure requiring recovery. Native children receive only their static directory,
canonical seed/restore state and controller stream. No original process runs
inside the comparison module.

`zoom_zoo_race_audit --manifest REPLAY --reference REFERENCE --out DIR
--from-frame FIRST --to-frame LAST` fails before instruction capture if either
controller's expanded timeline differs, then fails after capture if any whole-WRAM
sample differs. Optional watches/frame images use the existing access engine.
`authentication.json` binds the successful audit. The comparison writes a small
ignored `validation-ledger.jsonl` beside its report, including source/diff,
binary, build/toolchain, ROM/core, static/controller/contract and report identities.
Source or binary changes during validation invalidate the run. Historical ledger
entries are not automatic cached passes; reuse needs matching identities and
available evidence, with source-only documentation differences explained.

The primary covers a complete three-lap race and 240 player post-finish updates
from authentic end-1649. This does not add ZOOM ZOO frontend dispatch, native
race-start initialization, subsequent result-screen loading, rendering or audio.
Reviewers freeze their own successful race timelines before native evaluation
and extend each horizon to include its own post-finish continuation.

## M4-16 experimental task-branch commands

Implemented in `codex/m4-16-playable-zoom-zoo`, not accepted gameplay on main:

```sh
python3 tools/project.py frontend run --track zoom-zoo --pack local/classic-crawler-two-tracks-v8.pack --preset app-debug --report artifacts/m4-16/FRESH-live.json
build/app-debug/src/core/classic_race_presentation_runner local/classic-crawler-two-tracks-v8.pack --timeline <native timeline> <frame> OUT.ppm
python3 -m tools.unirally_lab.native.zoom_zoo_playable --help
python3 -m tools.unirally_lab.native.zoom_zoo_playable_reference --help
# idle variation: case JSON {"idle":{"from":F,"frames":N|null}} releases all buttons, then resumes the primary; horizon up to 40000
# optional "buttons" holds that set instead of releasing; it must be one the port publishes as nothing (opposing pairs only), so the window stays idle (R-0041)
```

The first-launch frontend accepts `--rom` plus a fresh pack destination and
extracts 56 validated static entries (pack v6 added the R-0036 rider object and
look tables; v7 added the race palette cycle tables; v8 adds the R-0040
channel-6 window HDMA family); later pack-only launches do not open ROM. The `--timeline` render
replays consecutive native states from initialization so rider look overlays
are exact; the single-state runner form omits them.
The ordinary `content pack` CLI still targets accepted DRAGSTER rules. V4 pack
profile and URZZ000A serialization are unaccepted experiments. Historical
URZZ0001/2/3 and DRAGSTER v1 remain supported. See [R-0035](research/R-0035-zoom-zoo-playable-recovery.md)
for source/identity/domain limits and the exact original/native compare recipe.
Warm extraction/smoke tests do not prove clean bootstrap, denied access, live
input, representative visuals or full latest regressions. Start, bounce and
other remaining guards preclude playable acceptance.

## DRAGSTER ordinary controls (task branch)

Implemented in `task/dragster-ordinary-controls` for
[DRAGSTER-ORDINARY-CONTROLS](../tasks/DRAGSTER-ORDINARY-CONTROLS.md), pending
independent review. DRAGSTER now plays on the shared race engine from native
initialization ([R-0038](research/R-0038-dragster-ordinary-controls.md)); the
legacy `update_movement` path, its `URMV` formats and every historical DRAGSTER
command above are unchanged.

```sh
# Original capture (private, cold start on the accepted DRAGSTER menu path); --frame-image adds PNGs.
python3 -m tools.unirally_lab.native.dragster_playable_reference --core local/emulators/bsnes/bsnes/out/bsnes_libretro.dylib --case tests/manifests/native/dragster-ordinary-primary.case.json --horizon 3900 --out artifacts/FRESH-a
# Freeze two identical captures, before evaluating native.
python3 -m tools.unirally_lab.native.dragster_playable freeze --reference artifacts/FRESH-a --repeat artifacts/FRESH-b --out artifacts/FRESH.freeze.json
# Native gate: 742-byte URDG0001 rows, second run, restart, fresh-process restores.
python3 -m tools.unirally_lab.native.dragster_playable compare --reference artifacts/FRESH-a --repeat artifacts/FRESH-b --contract tests/manifests/native/dragster-ordinary-primary.freeze.json --binary build/app-debug/src/core/zoom_zoo_runner --pack local/classic-crawler-two-tracks-v8.pack --out artifacts/FRESH-compare.json
# First divergence only, for exploration (no freeze needed).
python3 -m tools.unirally_lab.native.dragster_playable explore --reference artifacts/FRESH-a --binary build/app-debug/src/core/zoom_zoo_runner --pack local/classic-crawler-two-tracks-v8.pack
# Abort fuzz over complete races with the app's update, restart and render calls; failing races become capture cases.
build/lab-release/src/app/dragster_fuzz_runner --content-pack local/classic-crawler-two-tracks-v8.pack --first-seed 1 --seeds 3000 --races 3 --max-updates 40000 --failure-cases artifacts/FRESH-failures
# One race state of either track drawn as the app draws it (CLASSIC-PRESENTATION-UNIFICATION: one renderer);
# --timeline replays a native timeline so the rider look overlays and the opponent's finish frame are exact,
# and --window-index prints the channel-6 window member per row for index-level checks against the original.
build/app-debug/src/core/classic_race_presentation_runner local/classic-crawler-two-tracks-v8.pack STATE.bin OUT.ppm [PREVIOUS_STATE.bin]
build/app-debug/src/core/classic_race_presentation_runner local/classic-crawler-two-tracks-v8.pack --timeline NATIVE_TIMELINE FRAME OUT.ppm
build/app-debug/src/core/classic_race_presentation_runner local/classic-crawler-two-tracks-v8.pack --window-index NATIVE_TIMELINE
# Extract the two-track pack (profile classic.pal.crawler.two-tracks.v8, 56 entries).
python3 tools/project.py content pack --rules tests/manifests/content/classic-crawler-two-tracks-pack.json --out local/classic-crawler-two-tracks-v8.pack
# Live play. Without --pack the newest pack under local/ carrying the supported profile is used; a typed --pack
# must carry it (a DRAGSTER v1 pack is refused, not substituted); --rom extracts when no such pack exists, and
# --rom with --replace-pack moves an incompatible pack aside first. A stale build is reported before launch.
python3 tools/project.py frontend run --track dragster --preset app-debug
```

`zoom_zoo_runner --start classic.crawler.dragster` requires `--content-pack`
with the two-track pack. Frozen cases are
`tests/manifests/native/dragster-ordinary-*.case.json` with their `.freeze.json`
contracts; DRAGSTER guard overrides are in
`tests/manifests/native/dragster-race-guards.reference.json`. The fuzz reports
aborts only; divergences need an original capture of the same timeline.

## Opposing directions (ZOOM-ZOO-OPPOSING-INPUT)

Added in `task/zoom-zoo-opposing-input` for
[ZOOM-ZOO-OPPOSING-INPUT](../tasks/ZOOM-ZOO-OPPOSING-INPUT.md); see
[R-0041](research/R-0041-opposing-directions.md). No command is new: the three
cases use the M4-16 capture, freeze and compare commands above, with the `idle`
variation's optional held `buttons`.

```sh
CORE=local/emulators/bsnes/bsnes/out/bsnes_libretro.dylib
# Left+Right over a 1,000-update riding window (horizon 8100); axes and edges use 7600.
python3 -m tools.unirally_lab.native.zoom_zoo_playable_reference --core "$CORE" --case tests/manifests/native/zoom-zoo-playable-opposing-ride.case.json --horizon 8100 --out artifacts/FRESH-ride-a
python3 -m tools.unirally_lab.native.zoom_zoo_playable freeze --reference artifacts/FRESH-ride-a --repeat artifacts/FRESH-ride-b --out artifacts/FRESH-ride.freeze.json
python3 -m tools.unirally_lab.native.zoom_zoo_playable compare --reference artifacts/FRESH-ride-a --repeat artifacts/FRESH-ride-b --contract tests/manifests/native/zoom-zoo-playable-opposing-ride-v11.freeze.json --binary build/app-debug/src/core/zoom_zoo_runner --pack local/classic-crawler-two-tracks-v8.pack --out artifacts/FRESH-ride-compare.json
# Hold an opposing pair through a hidden run: Left+Right is mask 192, Up+Down is 48.
python3 tools/project.py frontend run --track zoom-zoo --pack local/classic-crawler-two-tracks-v8.pack --preset app-debug --updates 4000 --hidden --fixed-controller-mask 192 --report artifacts/FRESH-hidden.json
```

The three frozen cases are
`tests/manifests/native/zoom-zoo-playable-opposing-{ride,axes,edges}.case.json`
with their `-v11.freeze.json` contracts. Two of them must reproduce an accepted
contract's rows exactly, which is the equivalence itself: `opposing-ride` must
equal `zoom-zoo-playable-idle-late-start-v11.freeze.json` (`205d1705...`) and
`opposing-edges` must equal `zoom-zoo-playable-primary-v11.freeze.json`
(`b4a34af7...`), each with a different `timeline_sha256`. That expectation lives
here and in R-0041 rather than inside the case files: a capture stores the
parsed variation and the freeze hashes it into `original_sha256`, so adding an
annotation key to a case would stop a fresh capture matching its own contract. `ride` holds Left+Right for updates
1650-2649, `axes` holds both axes for 1650-2049, and `edges` holds Left+Right
over the countdown 1377-1649 and Left+Right then Up+Down over the whole result
screen 6725-7600. An opposing window spliced into the marker-guided riding
script instead of the `idle` window desynchronizes that steering and never
finishes, which is why the accepted `constant-left`/`constant-right` cases are
declared incomplete inventories.

The `native presentation-check` v1 contracts need their fixtures below `local/`
or `artifacts/` in the checkout that runs them: copy
`local/evidence/classic-presentation-unification/unification-baseline/{winner,loser}-fixtures`
to `local/v1-fixtures/` first. An absolute path into another checkout is
refused as an unauthorized fixture directory.

## Local evidence layout

Since REPO-LOCAL-STATE-CLEANUP (18 September 2026) the ignored evidence has one
home per kind in the main checkout, and a task worktree holds only its
checkout, `build/` and a copy of the private inputs it needs under `local/`:

| Kind | Location | Examples |
| --- | --- | --- |
| original captures and other evidence a record cites | `local/evidence/<task-worktree>/` (the former `artifacts/` of that worktree, intact) | `local/evidence/dragster-ordinary-controls/dragster-ordinary-controls/originals/primary-a`, `local/evidence/m4-16-playable-zoom-zoo/m4-16/boundary-a` |
| closeouts and the gate logs behind them | `artifacts/<task>-integration/closeout.json` | `artifacts/m4-16-integration/closeout.json`, `artifacts/window-pause-integration/closeout.json` |
| recorded gate scripts | beside the closeout or under the task's evidence directory, with their input paths rewritten to `local/evidence/...`; their `cd .worktrees/<name>` lines name checkouts that no longer exist, so recreate one at the recorded commit with `git worktree add` before rerunning a script verbatim | `artifacts/dragster-ordinary-integration/gates-b452170/gates-frozen.sh` |
| private inputs | `local/` (ROM locator, packs, toolchain, `native/dragster-idle`, the bsnes lab core) | `local/classic-crawler-two-tracks-v8.pack` |

Point a gate at the evidence directly, from any checkout:

```sh
O="$(git rev-parse --show-toplevel)/local/evidence/dragster-ordinary-controls/dragster-ordinary-controls/originals"   # from the main checkout; from a worktree use the main checkout's absolute path
python3 -m tools.unirally_lab.native.dragster_playable compare --reference "$O/primary-a" --repeat "$O/primary-b" --contract tests/manifests/native/dragster-ordinary-primary.freeze.json --binary build/app-debug/src/core/zoom_zoo_runner --pack local/classic-crawler-two-tracks-v8.pack --out artifacts/FRESH-compare.json
```

The retention rule a closing task follows is in `AGENTS.md`; the audit tables,
moves and deletions of the cleanup itself are in
[REPO-LOCAL-STATE-CLEANUP](../tasks/REPO-LOCAL-STATE-CLEANUP.md).

## DRAGSTER 10:00 clock limit (task branch)

Added in `task/dragster-clock-limit` for
[DRAGSTER-CLOCK-LIMIT](../tasks/DRAGSTER-CLOCK-LIMIT.md), pending independent
review; see [R-0039](research/R-0039-dragster-clock-limit.md). Two commands are
new; every command above is unchanged.

```sh
# Declared incomplete original inventory, for a case freeze deliberately refuses.
python3 -m tools.unirally_lab.native.dragster_playable inventory --reference artifacts/FRESH-idle-a --repeat artifacts/FRESH-idle-b --out artifacts/FRESH-idle.inventory.json
# Native gate over the inventory's exact race and loading prefix, with a bounded restore set.
python3 -m tools.unirally_lab.native.dragster_playable compare --prefix --reference artifacts/FRESH-idle-a --repeat artifacts/FRESH-idle-b --contract tests/manifests/native/dragster-clock-limit-idle-incomplete.inventory.json --binary build/app-debug/src/core/zoom_zoo_runner --pack local/classic-crawler-two-tracks-v8.pack --out artifacts/FRESH-idle-gate.json
```

The frozen case is `tests/manifests/native/dragster-clock-limit-idle.case.json`
at horizon 32200, about ten minutes and 4 GB of raw memory per capture. `freeze`
refuses it because its result loading waits two extra updates in the SPC700
reset handshake; `inventory` records the same evidence with
`acceptance: false`, the whole-capture rows hash and the exact prefix hash, and
`compare --prefix` gates only that prefix. Ordinary DRAGSTER cases keep using
`freeze` and `compare`.

## ZOOM ZOO window effects (task branch)

[ZOOM-ZOO-WINDOW-EFFECTS](../tasks/ZOOM-ZOO-WINDOW-EFFECTS.md) binds the
channel-6 window family for ZOOM ZOO in `classic_race_presentation_content`,
so both tracks' countdown digits, GO and winner banner draw from the
original's per-frame selection; the only track difference is the countdown
transition member (`classic_window_transition_member`, from the player's
start reflection in the track header). No new command: the runner's
`--window-index` mode and `--timeline` mode above serve ZOOM ZOO timelines
too. The original's pointer is read from any `zoom_zoo_playable_reference`
capture's `memory.wram` (one 131072-byte image per frame from
`reference.json["frames"][0]`; frame n shows the `$11FD` word at the end of
frame n-1, member k at `$8000 + 899k`, `$DB4E` no window). The task's probe
and picture scripts are kept with its evidence under
`local/evidence/zoom-zoo-window-effects/`.
