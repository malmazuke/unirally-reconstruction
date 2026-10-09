# PORTABLE-CORE-IDENTITY - the laboratory and the pack build work on any host that builds the pinned core

## Assignment

- Status: **in progress**, claimed 9 October 2026 (8 October 23:40 UTC) on main `56424c6`, after
  ROLLING-CONTACT, at the user's request to chain the remaining ready tasks. Queued 27 September
  2026 (UTC) from a cloud session that recorded gameplay video on Linux (the user's request).
- Milestone: tooling (no milestone; it serves every later task that runs the reference core)
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`); one
  session as coordinator, primary and integrator
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  **tier 2** (tooling and manifests; no gameplay arithmetic changes). A reviewer escalates if a
  frozen reference result changes.
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): at claim weekly 1%, five-hour 8% (8 October 22:47 UTC). The user asked to
  chain this task and ATTRACT-DEMO.
- Reviewer (primary automatically spawns fresh model/effort, isolated checkout; no user
  trigger): a fresh Anthropic subagent.
- Dependencies and evidence of acceptance: M0-03 (the pinned bsnes lock), M3-02A (the content
  pack and its extraction).
- Base commit: main `56424c6`.
- Branch and isolated worktree: `task/portable-core-identity` in
  `.worktrees/portable-core-identity`.
- Owned paths and shared interfaces: `tools/unirally_lab/reference/`,
  `tools/unirally_lab/content/landing_matrix.py`, the `CORE_SHA` users under
  `tools/unirally_lab/native/`, the `core_sha256`/`library_sha256` fields of
  `tests/manifests/`, `tests/tooling/`, `docs/BUILD_AND_VALIDATION.md`, this record,
  `tasks/README.md`, `tasks/NEXT_SESSION.md`.
- Claim/lease/heartbeat/checkpoint location: this record.
- Session time limit, concurrency allocation and actual spend authorization if relevant: one
  worker plus the review subagent; no monetary spend.

## Outcome and boundaries

The reference core's identity is pinned as the SHA-256 of one built library binary,
`e59bf88d...9a91b`, the macOS `bsnes_libretro.dylib`. The lock
(`tools/locks/emulators.json`) pins the source (commit `7d5aa1e6`, tree, patch SHA-256), and
`reference build` records the library it produced in `lab-core.json`. But the checks compare the
binary's hash with the one constant. A library built from the same commit and patch on another
OS, compiler or linker has a different hash, so every check refuses it. Observed on Linux
(below, at `e01aaaa` with pack v23): `frontend run --rom` cannot create the pack, so no host but
the original Mac can build a playable pack from a ROM. The extraction step is unchanged on `main`
at `0173cbb` (pack v25).

The same constant is compared in `landing_matrix.py`, `zoom_zoo_trial_reference.py` and 14
other modules under `tools/unirally_lab/native/`, and recorded in 73 manifests under
`tests/manifests/`. The differential and freeze gates that run the core are likely blocked the
same way on another host; confirm which ones at claim.

Deliver:
- **Identity by source, checked by output.** A core is acceptable when `lab-core.json` shows it
  was built from the lock's commit and patch, and its library hash equals the one recorded at
  build time (as `reference` already checks, `reference/commands.py:304`). Where a result's
  bytes are pinned (the pack rules' `sha256` for `zoom.landing-response-matrices`, frozen
  digests, replay digests), that pin is the evidence of equal behaviour, not the binary's hash.
- **Manifests keep provenance.** Records keep the library hash they were made with as
  provenance. Gates stop requiring it to equal the running core's hash, or accept a recorded set
  of verified library hashes. Choose one at claim and record the reason in a decision if it
  changes D-0001's reading.
- **A portability check.** CI's Linux job (or a documented local run) builds the core and
  proves that the landing-matrix extraction reproduces `229eda89...ef69782b` there. This needs
  a ROM, so it cannot be a hosted CI check; record it as a local gate.
- **Linux build prerequisites documented.** `docs/BUILD_AND_VALIDATION.md` lists the X11
  development packages the pinned SDL3 needs on Linux (CI's workflow already installs them:
  `libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxfixes-dev libxi-dev libxss-dev
  libxtst-dev libxkbcommon-dev`, and `pkg-config`).

Outside: changing the pinned emulator commit or patch; refreezing any reference result;
Windows.

## Inputs and prerequisites

- The supported ROM, SHA-256 `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`
  (user-supplied; never tracked).
- The pinned bsnes lock in `tools/locks/emulators.json`; `reference build` fetches and builds it.
- A Linux host (Ubuntu 24.04, x86_64) and the macOS host, to show both accepted. At claim the
  local Linux host is Docker Desktop's `ubuntu:24.04` on aarch64 (this Mac); the x86_64 result is
  the cloud session's attempts 1-2.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Pack from a ROM on Linux | `frontend run --track dragster --pack local/<current profile>.pack --rom PATH` on Ubuntu 24.04 after `reference build` | The pack is created and validates; `zoom.landing-response-matrices` is `229eda89...` | report |
| Pack on macOS unchanged | The same on the Mac | The same pack bytes as before the change | report, pack SHA-256 |
| A wrong core is refused | A core built from another commit or with the patch changed | Refused, naming the lock mismatch | test |
| A wrong result is refused | A core whose extraction yields other bytes | Refused by the output pin | test |
| Gates | The differential and freeze gates that run the core, on the Mac and on Linux | Mac unchanged; Linux passes or each remaining failure is listed with its cause | logs |
| Docs | `docs/BUILD_AND_VALIDATION.md` | Linux SDL3 prerequisites listed | diff |

## Evidence and attempts

| Attempt | Hypothesis | Experiment | Observation | Next decision |
| --- | --- | --- | --- | --- |
| 1 | The v23 pack can be built from the ROM in a Linux cloud container | `frontend run --track dragster --pack local/classic-pal-crawler-tracks-v23.pack --rom ...` at `e01aaaa`, Ubuntu 24.04 x86_64, GCC 13.3.0 | `supported_rom` failed: `landing_matrix` raised "pre-race extractor core identity differs". `reference build` produced `bsnes_libretro.so` `668e3219...1c650489ad6959ddd7dce8812ad9c3e315` from commit `7d5aa1e6` with the lock's patch | Try the extraction with this core, checked by the output pin |
| 2 | The Linux core, built from the same source, extracts the same bytes | A scratch copy of `landing_matrix` accepting the Linux library hash (user-approved, not committed), then the pack built with `pack.build_pack` and `validate_pack` | The extracted 1,512 bytes hash to `229eda89d8f29fd9daf2b2f9247e98e244a7683511d82bfa6abdc6d3ef69782b`, the rules' pin. The pack validated: profile `classic.pal.crawler.tracks.v23`, 371 entries, 4,448,960 bytes, SHA-256 `9054090d4a8b2144bfab6034f0aa6523eb57d1bba3f0fdaa1bb7e649771f6088`. The SDL3 app played the menus and two DRAGSTER races on it | The binary hash is not needed for correctness here; this task |

| 3 | The Mac's v23 pack equals the Linux one | `shasum -a 256 local/classic-pal-crawler-tracks-v23.pack` (9 October) | `9054090d...71f6088`: the same bytes | Sort the hash checks |
| 4 | The Mac's own library hash is stable | The lock's source rebuilt in the scratchpad today (macOS 27): Homebrew clang 19 and Apple clang 17.0.0 (the compiler `lab-core.json` records) | `ee7bb1b4...` and `3f092254...`, neither `e59bf88d`: every check would refuse a rebuild on this Mac too | Identity by source; check behaviour by output |
| 5 | Two kinds of check | `git grep` of the hash in tools | 16 tools: running checks (the library before it runs) and stored checks (a capture's recorded `core_sha256`). Stored checks pass anywhere for Mac captures; they block only fresh captures on another host | `core_identity` for both; `core_check` for behaviour |
| 6 | A rebuilt library behaves identically | `core_check` (new): replay a stored capture's inputs, compare every frame's WRAM, SRAM and video hash | Installed `e59bf88d` and rebuilt `3f092254` both equal on all 1,201 frames of `seed7-a`; `3f092254` also extracts landing matrices `229eda89` | Linux |
| 7 | Linux builds and runs everything from the ROM | Docker `ubuntu:24.04` aarch64, GCC 13.3 (`artifacts/pci/linux/run.sh`): bootstrap, `reference build`, `frontend run --rom`, `core_check`, app-debug, the eleven gates against the Mac's captures | Library `f0bfd766...`; the v35 pack (562 entries) is `a12a41ec...`, byte-identical to the Mac's; `core_check` equal on four captures (1,201-2,601 frames); the app runs 600 hidden updates (`SDL_VIDEO_DRIVER=offscreen`); gates below | x86_64 |
| 8 | x86_64 too | The same in `--platform linux/amd64` (emulated) | Library `b3fb5ff5...` (the cloud session's x86_64 build of the same source was `668e3219...`); the same pack `a12a41ec...`; `core_check` equal on two captures; the app reaches the same state at update 600 | Verified list |

Attempts 1-2 ran in a cloud container (its scripts and pack stayed there); 3-8 on this Mac and its
Docker Desktop. Decision (recorded in D-0001, "Core identity across hosts"): identity by source with
a recorded set of verified libraries, because stored captures name the library that made them and
a reader on another host can check a foreign hash only against a list. Logs are in main
`local/evidence/portable-core-identity/` after closeout.

## Handoff

- Current base/head commit and uncommitted state: observed at `e01aaaa`, queued on `0173cbb`;
  nothing implemented.
- Verified findings: the two attempts above.
- Current hypothesis and failed approaches: every host that builds the lock's commit and patch
  runs the same emulation; the binary hash only tells the compiler apart.
- Exact next experiment/command: on the Mac, `shasum -a 256 local/classic-pal-crawler-tracks-v23.pack`
  (if still present), compared with `9054090d...`; then list every check that compares a core hash (`rg -n
  "CORE_SHA|core_sha256|library_sha256" tools tests`) and sort them into identity checks and
  provenance records.
- Remaining dependencies: a Linux host with the ROM for the acceptance run (a cloud session with
  the ROM uploaded works).
- Runtime needs (network, build time, fixtures, memory): GitHub for the core's and SDL3's
  sources; the core builds in about 30 s on 4 cores.

## Review and integration

- Reviewer and independent reproduction/withheld-case results: at claim.
- Scope still unverified: whether the macOS hash is stable across Xcode versions (if not, the
  Mac is exposed to the same failure after a toolchain update).
