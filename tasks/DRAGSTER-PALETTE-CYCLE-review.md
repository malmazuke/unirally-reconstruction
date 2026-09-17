# DRAGSTER-PALETTE-CYCLE - Independent review

## Identity

- Current verdict: **approve** candidate `a2aac15` (round 3, launcher
  fallback, at the end of this report), with one medium and two low
  non-blocking residuals.
- Round 2 verdict at `2c9dea3`: **approve** (re-review, round 2, below).
- Round 1 verdict at `a685712`: **changes required** (one blocking finding,
  F1). Sections 1-5 and the findings below are round 1 as reported.
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

## Re-review, round 2: candidate `2c9dea3`

### Identity

- Candidate: `task/dragster-palette-cycle` at
  `2c9dea3e8159eacfe3e9f6b1c845c756630ee20c` ("Fill DRAGSTER's windows with
  the cycled colour 0"), one commit on `a685712`. `origin` resolves the branch
  to that commit.
- Checkout: `origin/task/dragster-palette-cycle` merged into
  `review/dragster-palette-cycle` as `4dc0dac`. `git diff --stat 2c9dea3 4dc0dac`
  shows only this report file, so the source tree equals the candidate's. Every
  round-2 report records `source.commit=4dc0dac`, `dirty=false`. Objects in all
  four build directories were rebuilt after the merge (11:00-11:02Z).
- Re-review started 2026-09-17T10:59:31Z. The end time is in the closing
  section.
- Binaries: `build/lab-debug/src/app/live_presentation_runner` `9ce3d161...`,
  `build/lab-debug/src/core/presentation_runner` `b5fe6839...`, and
  `build/app-debug/src/core/zoom_zoo_runner` `955d8fcf...`, byte-identical to
  round 1.
- Evidence: ignored `artifacts/dragster-palette-review/round2/`. Original
  pictures and the original's colour-0 pixel masks were recaptured in the
  session scratchpad with the round-1 scripts (`pngs.py`, and
  `marker_original.py` extended to save the colour-0 masks).

### What changed

- `render_dragster` fills the GO and winner windows with CGRAM colour 0 from
  `build_race_cgram(..., false)` plus `apply_dragster_palette_cycle` when the
  pack has the tables. Otherwise it keeps white and (98,98,255).
- `apply_dragster_palette_cycle` returns early when
  `result_loading_updates - 1 > frame`.
- `presentation_tests` adds a window test.
- The `.cpp` comment, R-0037 and the task record are updated, including a
  round-1 disposition table and a picture-level acceptance row.

The diff touches no ZOOM ZOO function.

### 1. Picture level (independent)

`round2/window_compare.py` renders frames with the live runner and both packs.
Native states come from a fresh `native finish-check` on
`full-race-review-release-3213.case.json`. Its per-frame output is
byte-identical to round 1, so gameplay is unchanged. Camera and scroll are the
original's (`$2026`, `$2047/$2049`, `$2059/$205B`), as in round 1.

It compares three things per frame:
- the native window mask, read independently from the pack's HDMA window
  tables (XOR of two inclusive windows per line);
- the original's colour-0 pixels, from a marker ROM run on the same inputs;
- the original picture.

Results (`round2/window-compare.json`):

| Frames | Native window px | Original colour-0 px | Overlap | v7 = original on overlap | v1 = original on overlap |
| --- | --- | --- | --- | --- | --- |
| 3322 winner, phase 4 | 5,001 | 2,810 | 823 | **823** | 0 |
| 3452 winner, phase 6 | 5,001 | 5,001 | 5,001 | **5,001** | 0 |
| 3453 winner, phase 7 (frozen) | 5,001 | 5,001 | 5,001 | 5,001 | 5,001 |
| 1600 GO, phase 10 (frozen) | 9,895 | 9,895 | 9,895 | 9,830 | 9,830 |
| GO, synthetic 1596 / 1598 / 1602 (phases 6, 8, 12) | 9,895 each | 9,895 each | 9,895 each | **9,868 / 9,479 / 9,839** | 0 / 0 / 0 |
| GO, synthetic 1597 / 1599 / 1601 / 1603 | 9,895 each | 9,260 each | 0 | n/a (other shape) | n/a |

- On every compared racing and finish-delay frame (the rows above), the
  original's colour-0 pixels equal that frame's cycle phase colour.
- The v7 window colour always equals the expected phase colour: (38,38,255) at
  3322, (121,38,255) at 3452, (156,156,255) at 1598 and so on.
- Outside the window, v1 and v7 are identical on every compared frame.
- Frozen 1600 mismatches 65 pixels in both packs. These are pixels drawn over
  the window.

**GO frames.** In all six accepted native DRAGSTER series (the three full-race
cases, m3-04 opponent-first, primary, withheld-cadence-17 and
withheld-release-2347), the native GO pose pair `0x04F9/0x0263` occurs only at
frame 1600, which is phase 10. No native state draws GO at another phase. The
"synthetic" rows therefore take the frame-1600 native state, relabel its frame
field, and use that frame's original camera and scroll. They test colour only;
the remaining overlap mismatches are riders at their 1600 positions.

On odd frames the original's GO shape is a different 9,260-pixel shape with no
overlap with native's single table. This is shape and timing, outside the
task.

**Loading.** Native v7 draws the winner window black on loading updates 1-224
(3454-3677).
- The original screen is entirely black on 3454-3561 (updates 1-108). There, v7
  matches **540,108 of 540,108** window pixels and v1 matches 0.
- From 3562 (update 109) the original fades in the result screen (55,121
  non-black pixels at 3562). There, v7 matches 23,510 of 580,116 and v1 0.
- Native shows the race until update 225. This is a pre-existing
  result-timing limit.
- The loser original is black on 3559-3666 and fades in from 3667, also update
  109.

**Full-race sweep** (`round2/sweep-original.json`): exactly **226** of 2,147
native frames now differ between v1 and v7: 3322, 3452 and 3454-3677, each by
5,001 pixels (the winner window). Round 1 had 0.

**Non-frozen frame score:** at 3452, rendered on the finish-delay rectangle and
scored like the contract, v1 gives 5,654 mismatches and v7 gives **653**
(`round2/render-compare-frozen.json`).

### 2. v1 accepted checks

| Command (lab-debug, v1 pack, `--artifacts artifacts/dragster-palette-review/round2/pres-*-v1`) | Result |
| --- | --- |
| `native presentation-check` winner contract | exit 0: **36/697/279/445/653/962/961** |
| `native presentation-check` loser contract | exit 0: **1,073** |
| `render_compare.py`: frozen cases with v1 and v7 | identical counts, and v1 and v7 PPMs byte-identical in all 8 cases (phases 10, 10, 10, 7, 7, result, result, result) |

### 3. Test strength

Each mutant was compiled from a scratch copy of `presentation.cpp` with the
candidate's `presentation_tests.cpp` against this build's libraries:

| Mutation | `presentation_tests` |
| --- | --- |
| none | exit 0 |
| windows ignore the tables (always the accepted colour) | **exit 134** |
| only GO ignores the tables | **exit 134** |
| only winner ignores the tables | **exit 134** |
| v1 packs (no tables) get black windows | **exit 134** |
| window uses colour 96 instead of colour 0 | **exit 134** |
| F2 guard removed | exit 0 (not covered; residual R2-3) |

### 4. ZOOM ZOO unchanged

- No ZOOM ZOO function is in the diff.
- The `zoomequiv` harness (the verbatim `1ece1b8` body against the rebuilt
  library, real v7 tables, frames 0-200,000): **0 frames differ**, and a short
  table is still rejected.
- `python3 -m tools.unirally_lab.native.zoom_zoo_playable compare ...
  zoom-zoo-playable-primary-v11.freeze.json --binary build/app-debug/src/core/zoom_zoo_runner
  --pack local/classic-crawler-two-tracks-v7.pack --out artifacts/dragster-palette-review/round2/m4-16-primary`
  (11:02:48-11:08Z): exit 0, passed.
  - Rows `b4a34af722b26693...`, unchanged.
  - 757 fresh restores, 1376-7600, 742 bytes.
  - Finishes 6484/6488, loading 6725, visible 6833, stable 6839.
  - Binary `955d8fcf...`, identical to round 1.

### 5. Regressions and CI

| Command | Result |
| --- | --- |
| `build` + `test --suite synthetic` for app-debug, lab-debug, lab-sanitize, app-sanitize (11:00:13-11:03:02Z) | 4x exit 0: **406/406 each, 22 ctest, 0 skipped** |
| `native finish-check` review-release-3213 (v1 pack) | passed; output identical to round 1 |
| `native opponent-first-check` m3-04, `native compare` primary / withheld-cadence-17 / withheld-release-2347 | passed (run for GO-pair states) |
| F2 harness with `-fsanitize=address,undefined,unsigned-integer-overflow` on the candidate source | no diagnostic. `frame=10, updates=20` and `frame=0, updates=65535` now leave CGRAM untouched. Frame 1334 with 2 updates is untouched, and 1335 with 2 updates draws phase 0 black, both correct. |
| `gh run view 35213305070` | workflow `synthetic`, push, head `2c9dea3`: **success**; `lab (macos-15)` 10:59:21-11:02:40Z, `lab (ubuntu-24.04)` 10:59:17-11:03:46Z |

### Round-1 findings

| Finding | Status |
| --- | --- |
| F1 | **Closed.** The cycle is visible where the original shows it: the window colour matches the original picture wherever the window shapes overlap, at every tested phase. Pixel-level acceptance is restored, and a render-level test fails when the windows ignore the tables. |
| F2 | **Closed** (behaviour); no regression test (R2-3). |
| F3 | **Partly closed.** The `.cpp` comment and R-0037's update-1 and update-75 wording are fixed. The header comment remains (R2-2). |
| F4 | Recorded in the task record. |

**Scoping judgement.** Keeping window timing and shape pose-gated, with one
frozen table each, is acceptable for this task. The task's corrected outcome is
the colour, and that is now recovered and verified. Window timing and shape
predate this task (M3-02/M4-01). They need a different recovery: in the
original, the GO shapes alternate each frame over roughly 1359-1603, and the
finish shapes change over 3215-3453. Both R-0037 ("Not established") and the
task handoff ("Not done") record this. I recommend the coordinator register it
as a named follow-up next to the DRAGSTER 10:00-limit follow-up, so it is not
lost.

### Residuals (non-blocking, for closeout)

- **R2-1 (docs, R-0037).** Two statements are false:
  - "The original screen is black throughout loading".
  - "at 3600 and 3677 the original screen is black during loading".

  Measured: the original is black only on loading updates 1-108 (winner
  3454-3561, loser 3559-3666). From update 109 it fades in the result screen:
  3562 has 55,121 non-black pixels, and 3600 and 3677 show the result screen.
  The 3600 and 3677 mismatches come from native still drawing the race until
  update 225, not from a black original. Also, "this rule has no visible
  effect" is no longer true: native v7 now draws a black winner window through
  loading, which matches the original's black screen on updates 1-108.
- **R2-2 (docs).** `src/core/presentation.hpp` still says "From loading update
  1 the routine has stopped". The task record's F3 disposition says the code
  comment is corrected, but only the `.cpp` comment was.
- **R2-3 (test).** Nothing tests the new F2 guard: removing it keeps
  `presentation_tests` green. One `require` with frame 10 and 20 loading
  updates would pin it.

### Verdict (round 2)

**Approve** `2c9dea3`. The residuals above do not affect behaviour or accepted
gates, and can be fixed in the integration commit.

### Not assessed (round 2)

- A GO window drawn by native play at a phase other than 10. No accepted
  native series produces one, so synthetic relabelled states were used.
- Live SDL play with pack v7.
- Private Linux differential execution.

### Closing (round 2)

- Re-review ended 2026-09-17T11:09:23Z; about 10 minutes wall clock.
- Commit on `review/dragster-palette-cycle`, pushed to `origin`; the remote ref
  is verified in the reviewer's handoff.

## Re-review, round 3: launcher fallback, candidate `a2aac15`

### Identity

- Candidate: `task/dragster-palette-cycle` at
  `a2aac15de7dbe51673f26c26aa56bbe8d1b31786`, which `origin` resolves to that
  commit. Since the approved `2c9dea3` it contains:
  - `a4fd477`: the round-2 integration fixes plus the coordinator's
    window-evidence wording correction;
  - `829a7c9`: a merge of this review branch at `223cd0d`, where `main`
    (`1ece1b8`) is already an ancestor;
  - `a2aac15`: the launcher fallback in `tools/unirally_lab/frontend/commands.py`.
- Checkout: merging `origin/task/dragster-palette-cycle` fast-forwarded
  `review/dragster-palette-cycle` to `a2aac15`. All round-3 reports record
  `source.commit=a2aac15`, `dirty=false`.
- Round 3 started 2026-09-17T11:21:48Z. The end time is in the closing section.
- Binaries rebuilt at `a2aac15`:
  - `build/lab-debug/src/app/live_presentation_runner` `6001fb2a...` and
    `build/lab-debug/src/core/presentation_runner` `f7f85a4e...`. They differ
    from round 2 only through the header comment change;
    `src/core/presentation.cpp` has no diff since `2c9dea3`.
  - `build/app-debug/src/core/zoom_zoo_runner` `955d8fcf...`, identical to
    rounds 1 and 2.
- Evidence: ignored `artifacts/dragster-palette-review/round3/`, including
  `launcher_cases.sh`, `launcher-cases.log`, `launcher/*.json|log` and the
  preset reports.

**Process note (evidence handling).** At 11:23Z I accidentally ran my
four-preset script, which still wrote into `round2/`. Before I stopped it, it
overwrote the round-2 JSON and log reports for app-debug, lab-debug and
lab-sanitize with complete, valid runs of `a2aac15`; those were moved to
`round3/`. The round-2 section's figures (406/406 at `4dc0dac`) stand as
recorded; only the app-sanitize round-2 JSON survives. The stopped app-sanitize
run was repeated cleanly for round 3 below. I also stopped the orphaned tooling
test processes it had left behind.

### 1. Code review of the launcher diff

The fallback runs only when all of the following hold:
- an existing pack fails validation against the loaded rules;
- `--track` is `dragster`;
- the resolved `--rules` path equals the resolved default DRAGSTER rules path.

It then validates the same file against the repository's two-track rules and
accepts it only if that validation fully passes. That validation checks magic,
schema, source ROM identity, rules identity, profile, start state, inventory,
layout and every entry hash. If it also fails, the v1 diagnosis is kept when the
two-track failure is the rules identity; otherwise the two-track diagnosis is
reported.

Can it accept a pack it should not?
- **Only a fully valid two-track pack can pass.** Such a pack carries the
  two-track rules identity, which the v1 validation rejects at or before its
  identity check.
- **The launched pack keeps every DRAGSTER entry.** All 25 v1 entries are
  byte-identical in v7 (round 1), and the app validates the pack again itself.
- **Explicit `--rules` to any other file disables the fallback**, including a
  byte-identical copy of the v1 rules (L05). Naming the default file or a
  symlink to it enables it, which is the same rules file (L06, L07).
- **`--track zoom-zoo` is excluded.** Its default rules are already the
  two-track rules, and a v1 pack is still refused (L04).
- **Missing pack with `--rom`** takes the extraction branch, which is unchanged
  and uses the rules actually selected. DRAGSTER still extracts a
  byte-identical v1 pack (L18).
- **Non-pack, truncated, corrupt and wrong-source files** are refused with exit
  3 and a specific diagnosis (L10-L16).

Python: rebinding `exc` inside the nested `except` is valid, and `exc` is only
used inside the outer handler. The report records
`two_track_extraction_rules` as an input only when the fallback succeeds.

### 2. Launcher cases (reviewer-owned)

All cases ran `python3 tools/project.py frontend run --preset app-debug --hidden
--timeout 300 --report ...` with `SDL_VIDEODRIVER=dummy`, via
`round3/launcher_cases.sh`, 11:29Z.

| Case | Arguments | Exit | Diagnosis / inputs |
| --- | --- | --- | --- |
| L01 | dragster, v1 pack | 0 | `extraction_rules` |
| L02 | dragster, v7 pack | **0** | `+ two_track_extraction_rules`; app "Classic pack validated", DRAGSTER race |
| L03 | zoom-zoo, v7 | 0 | |
| L04 | zoom-zoo, v1 pack | 3 | extraction-rules identity is incompatible |
| L05 | dragster, v7, `--rules` = a copy of the v1 rules elsewhere | **3** | identity incompatible (no fallback) |
| L06 | dragster, v7, `--rules` = the default path spelled explicitly | 0 | fallback |
| L07 | dragster, v7, `--rules` = a symlink to the default rules | 0 | fallback (resolved path) |
| L08 | dragster, v7, `--rules` = two-track rules | 0 | direct (as before the change) |
| L09 | dragster, v1 pack, `--rules` = two-track rules | 3 | identity incompatible (no reverse fallback) |
| L10 | dragster, v7 with one payload byte flipped | 3 | entry payload hash differs (two-track diagnosis) |
| L11 | dragster, v1 with one payload byte flipped | 3 | entry payload hash differs (v1 diagnosis kept) |
| L12 | dragster, v7 with a flipped rules-identity byte | 3 | extraction-rules identity is incompatible |
| L13 | dragster, v7 with a flipped source-identity byte | 3 | source ROM identity is incompatible |
| L14 | dragster, first 200 bytes of v7 | 3 | pack is truncated |
| L15 | dragster, a non-pack text file | 3 | header magic is invalid |
| L16 | zoom-zoo, corrupt v7 | 3 | entry payload hash differs |
| L17 | dragster, v7 plus `--rom` | 0 | fallback; ROM not opened |
| L18 | dragster, missing pack plus `--rom` | 0 | extracted profile `classic.pal.crawler.dragster.v1`, byte-identical to `local/classic-crawler-dragster.pack` |
| L19 | dragster, missing pack, no `--rom` | 2 | supported ROM missing |

### 3. Observable effect through the launcher

The hidden app prints redraw counters, which depend on pixels. I built a
prediction harness (scratch `applike.cpp`) linked against this build's
libraries. It replays `sdl_main.cpp`'s DRAGSTER path: classic start, fixed
controller mask, `presentation_position` while racing, a persistent
`LivePresentation`, an initial render plus one render per update, and the same
identical-redraw counting.

For 2,146 updates with Right held (`--fixed-controller-mask 128`), it predicts:
- v1: 2,147 frames, 1,421 fallback frames, **382** identical redraws, longest
  run **225**;
- v7: **380** and **223**.

The two differences are at 3453 and 3454. With v1, 3452 to 3453 and 3453 to 3454
are identical. With v7, 3452 (phase 6, window (121,38,255)) differs from 3453
(phase 7), and 3453 differs from 3454 (loading, black window).

Launcher runs, 11:29:24-11:30:50Z:

| Command | Exit | App output |
| --- | --- | --- |
| `SDL_VIDEODRIVER=dummy python3 tools/project.py frontend run --track dragster --pack local/classic-crawler-dragster.pack --preset app-debug --hidden --updates 2146 --fixed-controller-mask 128 --timeout 300` | 0 | Presentation frames 2147; fallback 1421; identical redraws **382**; longest run **225**; frame 3679; phase 3, outcome 1 |
| same with `--pack local/classic-crawler-two-tracks-v7.pack` | 0 | Presentation frames 2147; fallback 1421; identical redraws **380**; longest run **223**; frame 3679; phase 3, outcome 1 |

Both runs match the prediction exactly. So the documented launcher now reaches
the app path that draws the cycled window colour at a phase other than 10 or 7.
This is an indirect observable: no pixels leave a hidden run.

### 4. Other checks at `a2aac15`

| Check | Result |
| --- | --- |
| `build` + `test --suite synthetic`: app-debug, lab-debug, lab-sanitize (11:23-11:25Z), app-sanitize (11:32:12-11:32:54Z) | 4x passed: **406/406, 22 ctest, 0 skipped**, `a2aac15` clean, no source change during the run |
| `native presentation-check` v1 winner and loser | **36/697/279/445/653/962/961** and **1,073** |
| `render_compare.py`: frozen cases with v1 and v7 | identical counts, and v1 and v7 byte-identical in all 8 cases; frame 3452 scores v1 5,654 vs v7 653, as in round 2 |
| Round-2 residual R2-3: F2 guard mutant against the new `presentation_tests.cpp` | **exit 134** (now caught); the window mutant is also still caught |
| Round-2 residual R2-2: `presentation.hpp` comment | fixed |
| Round-2 residual R2-1: R-0037 loading wording | fixed: black on updates 1-108 (winner 3454-3561, loser 3559-3666), result fading in from update 109, 3600/3677 explained by native drawing the race until update 225 |
| ZOOM ZOO | no source change since `2c9dea3` apart from the header comment; `zoom_zoo_runner` byte-identical to the binary that passed the M4-16 primary gate in rounds 1 and 2, so that gate was not rerun. L03, L04 and L16 cover the ZOOM ZOO launcher path. |
| `gh run view 35215252623` | workflow `synthetic`, push, head `a2aac15`: **success**; `lab (macos-15)` 11:21:38-11:24:01Z, `lab (ubuntu-24.04)` 11:21:33-11:24:52Z. Integration candidate `829a7c9` run 35214438206 also succeeded. |

### 5. Window-evidence wording (coordinator's correction in `a4fd477`)

The corrected R-0037 verification bullets and task-record attempt 5 match my
round-2 measurements:
- winner window 5,001/5,001 at 3452, and 823/823 on the overlap at 3322;
- GO checked only through states relabelled to 1596, 1598 and 1602 (9,868,
  9,479 and 9,839 with v7, 0 with v1), because native draws GO only at 1600;
- the 3454-3561 matches are black loading frames on both sides (my
  540,108/540,108).

I did not recompute the coordinator's own 15-frame totals (61,239 of 75,015),
but their described composition is consistent with my figures. The corrected
text no longer overstates the racing-phase evidence.

### Residuals (non-blocking)

- **R3-1, medium: no automated test exercises the fallback.**
  - `tests/tooling/test_frontend.py` uses a temporary rules path, so the new
    branch never runs.
  - Mutation: restoring the `2c9dea3` `commands.py` in a scratch copy of the
    repository still passes all 10 frontend tests. The candidate's copy also
    passes.
  - The launcher gap was found only by an integration gate, and the checks live
    in ignored artifacts. A ROM-free test (patching `commands.ROOT` or the rules
    constants with two authored rule sets) should cover:
    - a two-track pack launching under `--track dragster` with default rules;
    - refusal with an explicit other rules file;
    - refusal for `--track zoom-zoo` with a DRAGSTER pack;
    - the payload-hash diagnosis for a corrupt two-track pack.
  - A test-only addition that keeps the four presets and CI green would not
    change this verdict.
- **R3-2, low (docs precision): the trigger is broader than recorded.**
  R-0037 and the task record say the fallback applies to a pack that "fails the
  v1 rules identity". The code tries the two-track rules after *any* v1
  validation failure under the default rules. The outcome is the same (see
  section 1), but the wording should match the code.
- **R3-3, low (docs): `docs/STATE.md` overstates the windows.** It says the
  windows "now animate" with v7. In native play the winner window changes
  colour only on 3322 and 3452-3453 before turning black for loading, and GO is
  drawn only at 1600 (phase 10, unchanged white). "Take the cycled colour 0"
  would be accurate until window timing is recovered.

**Pre-existing (not a finding):** `frontend run --track dragster --pack
<absent path> --rom` always extracts a DRAGSTER v1 pack, even at a path named
`...two-tracks...`.

### Verdict (round 3)

**Approve** `a2aac15`. The fallback is correct and conservative: in 19 reviewer
cases it accepted exactly the valid two-track pack under the default DRAGSTER
rules and nothing else. The documented DRAGSTER v7 launch reaches the recovered
palette path, which the full-run redraw counters confirm. Presets, accepted
counts and CI are green.

### Not assessed (round 3)

- Visible (non-hidden) play.
- Linux launcher behaviour beyond hosted CI.
- A rerun of the M4-16 primary gate, since the binary is identical.

### Closing (round 3)

- Round 3 ended 2026-09-17T11:34:53Z; about 13 minutes wall clock.
- Commit on `review/dragster-palette-cycle`, pushed to `origin`; the remote ref
  is verified in the reviewer's handoff.
