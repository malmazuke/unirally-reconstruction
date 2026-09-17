# DRAGSTER-PALETTE-CYCLE - Independent review

## Identity

- Verdict: **changes required** (one blocking finding, F1).
- Reviewer: fresh Claude Opus 5 subagent (Claude Code), no prior context, isolated
  worktree `.worktrees/dragster-palette-review` on `review/dragster-palette-cycle`.
- Candidate: `task/dragster-palette-cycle` at exactly
  `a68571246325341f29486dbed40cd2093a3f0d75` (`a685712`), a merge of main
  `1ece1b8` (`6ba26f2`) plus one implementation commit. `origin` resolves the
  branch to the same commit. The worktree stayed clean at `a685712` for every
  gate; all reports record `source.commit=a685712`, `dirty=false`.
- Review started 2026-09-17T10:22:59Z; end time is in the closing section.
- Host: macOS 15.7.2 arm64, Apple clang 17.0.0 (Xcode 26.1.1), isolated CMake
  3.31.10 and Ninja 1.13.2, Python 3.14.3. ROM SHA-256
  `a1105819...0a1fd4e`; audited bsnes core `e59bf88d...9a91b` (both checked by
  the probe before running).
- Binaries from this checkout: `build/lab-debug/src/core/presentation_runner`
  `8a441f12...69b2d`, `build/lab-debug/src/app/live_presentation_runner`
  `445cdf87...120a7`, `build/app-debug/src/core/zoom_zoo_runner`
  `955d8fcf...6c5a3`, `build/lab-debug/src/core/movement_runner`
  `64dc6f06...cd67f`.
- Packs: DRAGSTER v1 `local/classic-crawler-dragster.pack` (`5c1fc5b0...`);
  two-track v7 extracted by the reviewer from the ROM
  (`frontend run --track zoom-zoo ... --updates 20 --hidden`, report
  `artifacts/dragster-palette-review/extract-v7.json`), SHA-256
  `b75539a0adfe13c3...8f4fd442`, the expected identity. All 25 v1 entries are
  present and byte-identical in v7; `presentation.zoom.race-palette-cycle.v1`
  (544 bytes, `a82bb272...`) equals the ROM bytes at file offset `$02AB`.

All evidence below is in the ignored `artifacts/dragster-palette-review/` of the
review worktree (scripts, probe series, renders, reports) unless another path is
named. Scratch harnesses were compiled in the session scratchpad and are
described here in enough detail to rebuild.

## Summary

The recovered CGRAM behaviour is right and independently reproduced: the race
NMI routine, the `(n-1334)&15` index on every racing frame, the loading-start
freeze with black colour 0, and the end of that behaviour at loading update 76
in both outcomes. Accepted gates, the four presets, the M4-16 primary gate and
hosted CI are unchanged and green.

The change has **no visible effect** in native output, and the task's stated
outcome is not delivered. Native DRAGSTER frames rendered with pack v7 are
pixel-identical to pack v1 on all 2,147 frames of a complete race. The native
renderer samples only cycled entries 101-103, which are constant in every
phase. In the original, the cycle is seen as the colour of the large window
shapes (the start arrow, the GO letters and the finish-delay shape), which come
from colour 0. It also shows in a small block beside the start chevrons. Native
draws those windows with fixed RGB colours keyed to pose pairs, which is the same
kind of phase coincidence the task set out to remove. The checkered line does
not animate in the original. R-0037's claim that it does is wrong, and the
task's render-level acceptance criterion was replaced by a CGRAM-only one.

## 1. Original evidence (reviewer-owned)

### Probe method and CGRAM offset validation

`artifacts/dragster-palette-review/cgram_review_probe.py` was written for this
review and does not reuse the implementer's probe. It runs a frozen replay
manifest on the audited core with Strict serialization and reads WRAM before
serializing. It records, per frame, CGRAM colour 0 and colours 96-111, `$0B84`,
`$0B92`, camera and HDMA scroll words, the gameplay projection and a video
digest. It also saves selected original PNGs (`frame_png`) and optional
instruction traces.

The offset was **not assumed**. It was established three independent ways at
frame 1600:

1. **Serializer layout anchor.** bsnes `sfc/ppu-fast/serialization.cpp`
   serializes `uint16 vram[32768]` immediately followed by `uint16 cgram[256]`.
   The DRAGSTER BG2 map and BG2 tiles, which the accepted renderer places at VRAM
   bytes `$E000` and `$2000`, each occur exactly once in the state, and both put
   VRAM start + `$10000` at offset **206063**. This matches the implementer's
   offset.
2. **Video consistency.** Each of the 54 distinct output colours at frame 1600
   is a CGRAM word at 206063. Output pixels were converted back to 15-bit words
   by inverting the libretro palette conversion (gamma 1.5, injective per 5-bit
   channel). The same check passes at every sampled racing frame (every 50th
   frame plus all PNG frames), except a few pixels of translucent caption text
   at 1601-1612. Result-screen frames at reduced brightness fail it, as
   expected. A colour-coverage search alone ties across at least five 2-byte
   shifts, so the implementer's "unique phase-10 match" is a weak locator in
   general, though its value is correct.
3. **Causal edit.** The state after 1600 was restored with CGRAM index 119 (a
   word unique in CGRAM) set to a marker, then frame 1601 was run. Exactly the
   pixels of that colour changed to the marker: 23,962 changed pixels, of which
   256 are a one-line restore artefact that also appears with no edit. Editing
   colours 96, 97 and 111, or colour 0, changes nothing beyond that artefact.
   So the offset addresses the PPU's live CGRAM, and the next frame's NMI
   rewrites colours 96-111 and 0 before its active display. The state after
   frame n therefore holds the palette frame n displayed, which the per-frame
   video check also shows.

**Serialization perturbation (informational, F4).** Strict serialization
advances the CPU to a sync point. Serializing every frame from 1200 or 1250
changes the run: from 1200 the video freezes on one image from 1210, and from
1250 the video differs from frame 1345 on. Serializing from
1300, 1334, 1400 or 1600 leaves the video identical to an unserialized run
through 3900. Pre-serialize WRAM still differs on 362 loading frames from 3459,
only in stack bytes `$01DB-$01F0` and direct-page scratch `$0063/$0069/$006F/$0083`.
The implementer's probe serialized from 1600 and read only CGRAM and a video
digest, so its conclusions stand. My probes serialize from 1300.

### Runs

| Probe (manifest) | Frames | Identity checks | Racing rule `(n-1334)&15`, colours 96-111 **and** colour 0 | `$0B84` = `(n-1333)&15` | Loading |
| --- | --- | --- | --- | --- | --- |
| A: `race-crawler-dragster-12000-review-release-3213-fields` (not used by the implementer) | 1300-3900 | video = unserialized run on all frames; WRAM projection = `full-race-review-release-3213.expected.json` on 1,921/1,921 frames | **2,120/2,120** frames 1334-3453 | 1334-3453 | L=3454 (native update 1); frozen index 8 + colour 0 `$0000` on 3454-3528 (updates 1-75); update 76 (3529) loads result palettes |
| B: same manifest, serialize from 1600 | 1600-3900 | video = unserialized run; projection 1,854/1,854 | 1,854/1,854 | 1600-3453 | as A; A and B CGRAM, video and `$0B84` identical on all 2,301 overlapping frames |
| Loser `race-crawler-dragster-12000-release-3000-3299-fields` | 1300-3900 | video = unserialized run; projection = `full-race-release.expected.json` 2,026/2,026 | **2,225/2,225** frames 1334-3558 | 1334-3558 | L=3559 (native update 1); frozen index 1 + black on 3559-3633 (updates 1-75); update 76 (3634) result palettes |
| `race-crawler-dragster-3000-right-released-2000` (3000-frame variant) | 1300-2999 | video = unserialized run | **1,666/1,666** frames 1334-2999 | 1334-2999 | n/a |

Before 1334 (1300-1333) colours 96-111 match no phase and colour 0 is a
different word, consistent with the routine not having run. Native DRAGSTER
starts at 1533, so this domain is not reachable natively.

Native loading numbering was checked, not assumed. `native finish-check` on
`full-race-review-release-3213.case.json` with the v1 pack passes, including
restores at 1603/2703/3302/3500. Its states have `result_loading_updates` 1 at
3454, 75 at 3528, 76 at 3529 and 225 at 3678. The loser case
`full-race-release.case.json` also passes, with 1 at 3559, 75 at 3633, 76 at
3634 and 242 at 3800.

Instruction traces (1<<20 ring per traced frame, no overflow) show exactly one
entry to `$82:D382` per frame, 85 instructions, reached through `$82:D37E`
(`JSR $D382 / RTL`). The routine runs at 1334 and 1335 but not at 1333, runs at
3453 and **at 3454** but not at 3455, 3528-3530 or 3677-3678, and runs at 3558
and **3559** but not at 3560 or 3633-3635 in the loser. So on loading update 1
the routine still runs and writes that frame's phase. Colour 0, which it also
writes, is black in the state after that frame, so a later writer in the same
frame sets it.

### Disassembly `$82:D382-$82:D496`

A linear disassembly from the ROM with the traced M/X modes
(`disasm_d382.py`, output `disasm-82D370-82D4A0.txt`, ignored) reads as follows.
`SEP #$30`, then return unless `$0B92` is nonzero. Set CGADD to `$60`, set
`X = $0B84 * 2`, and write sixteen 16-bit words to CGDATA from the tables at
`$80:82AB + 32*t` (t = 0..15), indexed by X, into colours 96-111. Set CGADD to
0 and write the word at `$80:84AB,X` to colour 0. Finally
`$0B84 = ($0B84 + 1) & $0F`. This is exactly the candidate's model; the
`LDY #$10` at `$82:D398` is unused.

## 2. Accepted gates

| Command | Result |
| --- | --- |
| `python3 tools/project.py native presentation-check --manifest tests/manifests/presentation/classic-crawler-dragster-v1.json --fixtures artifacts/dragster-palette-review/fixtures/m3-02-integration-fixtures --content-pack local/classic-crawler-dragster.pack --preset lab-debug --artifacts artifacts/dragster-palette-review/pres-winner-v1 ...` | exit 0: **36/697/279/445/653/962/961** |
| same, `classic-crawler-dragster-loser-v1.json`, `m4-01-loser-fixtures` | exit 0: **1,073** |
| both again with `--preset lab-sanitize` | exit 0, identical counts, no diagnostics |
| `render_compare.py` renders every frozen case with the checker's runner and scores it with `presentation.compare_rgb` | v1 and v7 give identical counts, and v1 and v7 PPMs are byte-identical in all eight cases |

Fixture copies under `artifacts/dragster-palette-review/fixtures/` match every
contract SHA-256.

## 3. Visible effect

Native states come from the finish-check run above. Original camera and scroll
come from WRAM `$2026`, `$2047/$2049` and `$2059/$205B` after the frame. That
rule reproduces the contract's values exactly at 1600, 2000 and 2400. The
product renderer `live_presentation_runner` was used for all frames, because the
headless runner fails closed on unrecovered pose pairs.

- **Full-race sweep** (`sweep_v1_v7.py original`, `sweep-original.json`): all
  **2,147** native frames 1533-3679 with v1 and with v7, and **0 frames differ
  by any pixel**. This covers racing, finish delay, loading and result.
- **Marker harness** (scratch `marker.cpp`, linked against this build's
  `libunirally_frontend/presentation/movement` libraries). It renders every
  frame with v7's real tables and with a table set in which each of the 17
  tables holds one unique marker colour in all phases. Native pixels sample only
  tables 5, 6 and 7 (colours 101-103: 915,159, 305,771 and 612,229 pixel
  samples over 598 frames). Their words, like colour 96's, are identical in all
  16 phases. Colours 97-100, 104-111 and colour 0 are never sampled: no native
  pixel falls through to the backdrop, and the XOR windows use fixed RGB.
- **Marker ROM run** (scratch `marker_original.py`). A patched copy of the ROM
  with the same marker tables was written to the session scratchpad, outside
  the repository, and deleted after the run. It ran on the same inputs, and the
  gameplay projection matched 1,921/1,921 frames. In the original, the phase-varying entries are displayed on 484
  frames (1359-1603 and 3215-3453). Colour 0 colours the large window shapes:
  the start arrow, the "G"/"O" letters and the finish-delay/winner shape, for
  example 9,673 pixels at 1534 and 5,001 at 3452. Colours 97 and 98 colour a
  small block among the start chevrons (x 0-39 at 1534). The checkered line
  uses constant colours 101-103.
- **Concrete frame comparisons** (`window-colour-check.json`,
  `renders/side-03452-orig-v1-v7.png`):

| Frame | Phase | Original winner-shape colour (colour 0) | Original pixels in that colour | Native v1 and v7 (identical) |
| --- | --- | --- | --- | --- |
| 3322 | 4 | `$7CE7` (38,38,255) | 2,810 | 5,001 px in (98,98,255), 0 in the original colour |
| 3452 | 6 | `$7CEF` (121,38,255) | 5,001 | 5,001 px in (98,98,255), 0 in the original colour |
| 3453 (frozen) | 7 | `$7DAD` (98,98,255) | 5,001 | 5,001 px in (98,98,255), matches |

  `render_window_xor(f, content.winner_window, {98, 98, 255})` and the GO
  window's fixed `{255, 255, 255}` are phase 7's and phase 10's colour 0. They
  coincide with the frozen frames 3453 and 1600, just as the pose-keyed arrays
  did. In this case native draws the winner window on 3322 and 3452-3677 and the
  GO window only on 1600.
- Start-line frames 1533-1570 were also compared. There, the original shows
  the colour-0 arrow and the chevron block, but native draws neither the window
  (wrong pose pair) nor the BG1 columns left of about x=104 at the stationary
  start camera. This is a pre-existing limit, so the cycle cannot appear there
  natively either.

## 4. Code review

- **v1 compatibility** is explicit: `content.race_palette_cycle.empty()` keeps
  `build_race_cgram`'s pose-keyed arrays and skips the cycle, and every direct
  caller passes `{}` or `optional_entry`. It is exercised by the accepted v1
  presentation gates and the `frontend_contract_tests` `{}` path. It is not
  pinned by a render-level test for the tables-present branch: deleting the
  `apply_dragster_palette_cycle` call from `render_race_background` still
  passes `presentation_tests` (scratch mutation). This follows from F1.
- **Unit test strength**: four rule mutations (start 1333, start 1335, no black
  colour 0 in loading, no loading freeze) each abort `presentation_tests`
  (exit 134). The unmutated copy passes.
- **Loading arithmetic** (F2): no undefined behaviour and no sanitizer
  diagnostic in the preset builds. However, `state.frame - (result_loading_updates - 1U)`
  wraps when the counter exceeds the frame, and the `phase_frame < 1334` guard
  does not catch the wrap. Reproduction: a harness compiling
  `src/core/presentation.cpp` with
  `-fsanitize=address,undefined,unsigned-integer-overflow` reports
  `presentation.cpp:1014:47: unsigned integer overflow: 10 - 19`. It draws
  phase 1 with black colour 0 for `frame=10, ResultLoading, updates=20`, and
  phase 12 for `frame=0, updates=65535`. Frame 1334 with 2 updates is correctly
  left untouched. Such states are unreachable from native play, since loading
  starts at or after 3454.
- **ZOOM ZOO path**: `apply_zoom_zoo_palette_cycle` keeps its size check and
  index function and calls `load_race_palette_phase(..., true)`. That runs the
  same 17-table loop with the same source/destination arithmetic, a pure
  extraction. A scratch harness (`zoomequiv.cpp`) compiled the `1ece1b8`
  function body verbatim next to the candidate library. With v7's real tables
  it compared both on every frame 0-200,000 from a non-trivial initial CGRAM:
  0 frames differ, and a 543-byte table is still rejected. The M4-16 primary
  gate, a state gate, is below.
- **GCC safety and readability**: the new code mixes `size_t` and `unsigned`
  only in unsigned comparisons and conditionals, and has no signed/unsigned
  pitfalls. Hosted Ubuntu 24.04 (GCC) built and passed all presets. Names and
  evidence links follow D-0003. One comment is inaccurate (F3), and the size
  check is repeated in the wrapper and the helper (harmless).
- **No extracted content**: the tracked diff adds synthetic test tables only,
  plus a few colour words cited as evidence in R-0037. No binaries or table
  dumps are included.

## 5. Regressions and CI

| Command | Result |
| --- | --- |
| `python3 tools/project.py build --preset P` then `test --suite synthetic --preset P`, for P in app-debug, lab-debug, lab-sanitize, app-sanitize (10:25-10:29Z) | 4x exit 0: **406/406 passed each, 22 ctest, 0 skipped**, source `a685712` clean |
| `native finish-check` review-release-3213 (v1 pack, 4 restore boundaries) and release (loser) | passed |
| `python3 -m tools.unirally_lab.native.zoom_zoo_playable compare --reference .../m4-16-playable-zoom-zoo/artifacts/m4-16/boundary-a --repeat .../boundary-b --contract tests/manifests/native/zoom-zoo-playable-primary-v11.freeze.json --binary build/app-debug/src/core/zoom_zoo_runner --pack local/classic-crawler-two-tracks-v7.pack --out artifacts/dragster-palette-review/m4-16-primary` | exit 0, status passed, source `a685712` clean, binary `955d8fcf...`, pack `b75539a0...`: 1376-7600, 742 bytes, **rows `b4a34af722b26693...` (same as the M4-16 review)**, **757 fresh restores**, full restart; finishes 6484/6488, loading 6725, visible 6833, stable 6839, player_won; 10:47:25-10:52:28Z |
| `gh run view 35210129769` | workflow `synthetic`, push on `task/dragster-palette-cycle`, head `a685712`: **success**; `lab (macos-15)` success 10:22:28-10:25:29Z, `lab (ubuntu-24.04)` success 10:22:25-10:26:36Z |

## Findings

### F1 - High (blocking): the recovered cycle is never visible natively, and the task outcome and research claim are wrong

**Evidence.** Section 3: 2,147/2,147 native frames are identical with v1 and
v7. Native samples only the phase-invariant entries 101-103. The original shows
the cycle as colour 0 on the start and finish window shapes, plus colours 97 and
98 beside the chevrons. At frames 3322 and 3452, native draws 5,001 winner-window
pixels in (98,98,255), while the original draws its winner shape in (38,38,255)
(2,810 pixels) and (121,38,255) (5,001 pixels).

**Consequences.**

- The task outcome ("so DRAGSTER's checkered start/finish line animates as in
  the original on every race frame") is not achieved.
- The premise is false: the checkered line uses constant entries.
- R-0037's "on the other six in eight the checkered line is drawn with the wrong
  phase" is incorrect.
- The acceptance row "Cycle is right beyond frozen frames" was changed from
  "start-line colours equal the original at every sampled frame" (a pixel
  comparison) to a CGRAM-only criterion. That is a weakened comparison under
  the reviewer checklist, and the pixel sweep is listed as "not done".
- The actual pose-keyed coincidence left in the product is the fixed window RGB
  in `render_dragster`.

**Required correction.** Choose one:

1. Draw the DRAGSTER window shapes in CGRAM colour 0 from the cycle when the
   tables are present, keeping v1's fixed colours. Add a render-level test that
   fails if the cycle is not applied (for example a phase-4 or phase-6
   winner-window frame). Compare at least frames 3322 and 3452, or equivalent,
   against original pictures. Also decide and record what native should show
   during loading, where the original screen is black from update 1 and the
   result palette loads from update 76.
2. Or re-scope honestly: state in the task and R-0037 that the change is a
   CGRAM-model refactor with no visible effect in the current renderer, correct
   the checkered-line claims, restore or explicitly retire the pixel-comparison
   criterion with a recorded decision, and name the window colour as the
   remaining visible discrepancy.

### F2 - Low: unsigned wrap for an inconsistent loading counter

`apply_dragster_palette_cycle` computes `frame - (updates - 1)` without
checking `updates - 1 <= frame`. The behaviour is defined, but for impossible
states it produces an arbitrary phase instead of the "before the routine"
branch. Reproduction is in section 4. A one-line guard would make the domain
explicit.

### F3 - Low (documentation)

- `presentation.hpp` says "From loading update 1 the routine has stopped". The
  trace shows the routine runs on update 1 (3454, 3559) and stops from update 2.
  Colour 0 is blacked later in update 1.
- R-0037 and the task record say the loading rule "matches through loser update
  75" as if winner behaviour were unknown past 3470. The winner-type case also
  holds through update 75 and switches at update 76 (3529).
- The loading rule is invisible in the original, whose screen is black from
  update 1. Native keeps drawing the race until update 225 (winner) or the
  result (loser); this is pre-existing.

### F4 - Informational (method)

For future probes: the colour-uniqueness offset locator can tie across shifts,
so the serializer layout anchor (VRAM then CGRAM) is a sounder validation.
Strict serialization every frame perturbs stack and scratch WRAM from 3459, and
derails the run if started before about 1300. The implementer's numbers are
unaffected.

## Verified claims

- DRAGSTER runs `$82:D382-D496`; racing frame n draws colours 96-111 and colour
  0 from `$80:82AB` at `(n-1334)&15`: **confirmed** on three scenarios,
  including two the implementer did not probe, from 1334.
- From loading update 1, colours 96-111 hold that frame's phase and colour 0 is
  black: **confirmed** through update 75 in both outcomes; stops at update 76.
- `racing_cycle` and `finish_cycle` are phases 10 and 7: **confirmed**.
- v1 packs keep the accepted palette; v7 carries the tables; no new pack
  version: **confirmed** as implemented (the decision is recorded).
- Accepted counts unchanged with v1 and identical with v7: **confirmed**
  (debug and sanitizer).
- "Native draws the cycle": true for CGRAM, **false for pixels** (F1).

## Not assessed

- Live SDL play with the two-track pack on DRAGSTER. Headless live-runner
  renders were used instead, which run the same `LivePresentation::render` code.
- Product camera (`presentation_position`) instead of original camera: not
  run. With the original camera no native pixel samples a phase-varying entry.
  The elements that carry the cycle in the original, the colour-0 window shapes
  and the start chevron block, are not drawn from CGRAM natively, so another
  camera is not expected to change F1.
- Linux private differential execution (not available; hosted Linux is
  synthetic only).

## Closing

- Review ended 2026-09-17T10:54:12Z; about 31 minutes wall clock.
- Report commit: on `review/dragster-palette-cycle`, pushed to `origin`; the
  remote ref is verified in the reviewer's handoff.
