# FRONT-END-MAIN-MENU - the app boots to the title and the main menu

## Assignment

- Status: **accepted** 26 September 2026 (tier 2,
  [#28](https://github.com/malmazuke/unirally-reconstruction/pull/28)). Claimed 25 September 2026 at 13:50Z by the Claude Code desktop
  session that ran COVERAGE-ROADMAP, on base `dd94e64` (queued the same day by that task).
- Milestone: M4 (original game coverage: menus)
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`)
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  the claiming session's model at default effort; **tier 2** (presentation, content and the
  front end's own state; the race state is unchanged).
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): at claim the 5-hour window was 33% used and the weekly window 25%. The
  user's allowance: continue until the weekly window reaches 50%.
- Reviewer (primary automatically spawns fresh model/effort, isolated checkout; no user
  trigger): a fresh Anthropic subagent.
- Dependencies and evidence of acceptance: COVERAGE-ROADMAP (the mode table and its captures);
  R-0053 (the text printer the menus share).
- Base commit: the `main` tip at claim.
- Branch and isolated worktree: `task/front-end-main-menu` in `.worktrees/front-end-main-menu`.
- Owned paths and shared interfaces: new front-end code in `src/core` (its own files), the app's
  start-up in `src/app`, pack rules and content for the title and menu, native tests, a
  research record, this record, `docs/STATE.md`, `tasks/README.md`, `tasks/NEXT_SESSION.md`.
- Claim/lease/heartbeat/checkpoint location: this record.
- Session time limit, concurrency allocation and actual spend authorization if relevant: one
  worker plus the review subagent; no monetary spend.

## Outcome and boundaries

Today the app starts in a race. The original starts at power-on, shows the title, then the main
menu, and only then a mode. Make the native app do the same up to the choice of mode:

- **Boot and title**: what the original shows from power-on to the main menu, with its timing.
  With no input the main menu appears at frame 419 of a cold start; with Start at 300-305 the
  captures show it by frame 450.
- **Main menu** (`$80:ABC8`, COVERAGE-ROADMAP): the logo over the checked background, the five
  entries 1P, 2P, VS, LEAGUE and OPTIONS, and the arrow cursor. Up and Down move the cursor
  (`$9B`, wrapping 0-4), Start chooses. An idle count of 480 frames (`$89`) starts the demo
  (`$9B = 5`).
- **Choices**: 1P enters the existing native race (DRAGSTER, as the app starts now) until
  FRONT-END-1P-SETUP exists. 2P, VS, LEAGUE, OPTIONS and the demo are not native yet: native
  shows a plain placeholder screen that returns to the main menu, and says so.
- **The two controller combinations** the main menu tests (`$72`/`$74` equal to `$02B0` or
  `$8430`, to `$80:A9B4` and `$80:F0D6`): read what they do and record it; implement them only
  if they stay inside the main menu.
- **Structure**: a small front-end state (the screen, the cursor, the counters) with an update
  per frame from the controller, and a picture drawn from pack content, in the style of the race
  code (D-0003). The text printer of R-0053 is the natural shared piece for menu text; draw the
  menu's text through it if the menu uses it.

Out of scope: the 1P setup screens, the other modes and the demo (later tasks), audio.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Pictures | Original frames from power-on through the title and the main menu (idle, cursor moves both ways with the wrap, Start on each entry) against native's | Title and menu frames match to the pixel, or each residue is explained | pictures, research record |
| Timing | The same paths | The main menu appears, the cursor moves and the demo would start on the original's frames | log |
| State | The original's `$9B`, `$89` and cursor words along the paths | Native's front-end state agrees frame by frame | log |
| Playable | The app, with a controller: title, main menu, 1P, a race | Reaches the race; the placeholders return to the menu | report |
| Nothing moves | ctest, the synthetic suite, the v1 contracts, hidden runs (which may now need the front end skipped or driven), the equivalence sweep | Unchanged | logs |

## Result

The native app now starts at power-on, as the original does
([R-0054](../docs/research/R-0054-boot-title-main-menu.md)). It shows the Nintendo screen, the
title and the main menu, frame for frame against the original.

- **A general SNES screen** (`src/core/snes_screen.cpp`) draws a picture from VRAM, CGRAM, OAM and
  the PPU registers, as the reference emulator's fast PPU does: backgrounds in modes 0, 1 and 3,
  objects, colour math and brightness. Every later menu screen is drawn with it.
- **The text printer** (`src/core/text_printer.cpp`) ports `$80:C3BC` for the control codes the
  menus use. The main menu's text is its own stream, `$80:AD1F`, printed as the original prints
  it.
- **The front end** (`src/core/front_end.cpp`):
  - The boot is a script of the frames on which the original loads, waits for a frame, and fades.
  - Two parts run every frame as the original's do: the arrow (its spin, and its quarter-step
    easing with the quirk that leaves it three sixteenths short in x) and the NMI's palette
    cycle.
  - The main menu's loop is the original's: the idle count into the demo, the choice, and the
    moves with their latch and wrap.
  - It keeps a mirror of the OAM buffer so it can be compared byte for byte.
- **The app** (`src/app`): without `--track` it starts at power-on, and 1P starts DRAGSTER. 2P, VS,
  LEAGUE, OPTIONS and the demo show a notice and return to the main menu. `frontend run` passes
  `--track` whenever one is named, so every race run and gate keeps starting in its race.
- **Pack v15** adds the front end's content: seventeen assets and six tables, all raw ROM.
- **Result**: against three captures of the original, every picture matches (0 differing
  pixels), and so do the arrow, the menu's words, the palette cycle and the whole OAM buffer:
  - `cursor`, 1,000 frames from power-on;
  - `cold`, to the demo's start at 900;
  - `buttons`, to the choice at 700.

Decisions and deviations, with reasons:

- **The boot uses measured frames.** How long a load or the sound upload takes is CPU work, not
  game rules. The boot script uses the frames the reference emulator shows for a cold start,
  cited from the frame model in R-0054.
- **The OAM buffer is mirrored, not only drawn.** A menu's objects are presentation state that
  later screens build on (the arrow flies in during the title, invisibly). Keeping the original's
  buffer lets every frame be compared exactly.
- **The codes are recorded, not implemented.** The title's code writes SRAM, Left+A+L+R opens
  the WIPE RAM menu (SRAM), and B+Down+L+R enters code not read yet. They belong with
  persistence and the later modes.
- **Opposing directions read as neither**, as the reference emulator models the pad's rocker
  (the `buttons` capture). The same rule as the race's, R-0041.

## Evidence and attempts

| Attempt | Hypothesis | Experiment | Observation | Next decision |
| --- | --- | --- | --- | --- |
| 1 | - | Every-frame images of `cold` and `cursor` | Three screens with fades; the main menu from 416; Start on the title changes nothing | Find the loads |
| 2 | The screens load through the asset directory | Access captures with register logs at `$82:B2DD`, `$82:B1DB`, `$82:B183` | 17 raw assets; the menu's are the result screen's | Pack v15 |
| 3 | - | Every PPU register write with its frame (`ppu-writes.txt`) | Each screen sets its registers once; fades and the palette cycle are the only changes | Read the arrow |
| 4 | - | Per-frame work RAM 0x0000-0x1FFF | The arrow's words, the cycle counters, the frame waits by frame | The frame model |
| 5 | Native matches | `compare.py` on `cursor` | After one fix (the cycle's first frame), 1,000 of 1,000 pictures and every word | `cold` |
| 6 | - | `compare.py` on `cold` | Equal to the demo's start at 900; the demo frame's `$89`/`$9B` needed the original's store rule | `buttons` |
| 7 | Up and Down together move down | The `buttons` capture | The menu did not move: the emulator models the rocker | `physical_pad` |
| 8 | - | The app, hidden: no track with Start held; idle into the demo notice | 1P after 421 frames, then the race; the notice returns to the menu | Gates |
| 9 | - | Gates at `cb916d6` (`gates-cb916d6.out`; the first run at `5b271ca` was stopped when the review's fixes changed code; `46ccd7c` and `c48ce13` after it change a loop binding, comments and records) | ctest 26/26 on three presets; synthetic and both v1 contracts pass; eight hidden race runs pass with 0 fallback frames; the front end with Start held chooses 1P after 421 frames, and idle shows the demo notice and returns; the eleven differential gates pass on pack v15 with unchanged digests; the equivalence sweep against main's binaries: 351 runs, 1,933,523 updates, 1,047 restarts, 2,052 pictures, 0 differences; recompare per-track results identical on 20 + 25 tracks; `cursor` 1,000/1,000, `cold` 901/901, `buttons` 301/301 pictures equal with no state difference; 0 functions over 80 lines; the index check passes | Review |

## Handoff

- Current base/head commit and uncommitted state: `task/front-end-main-menu` from `dd94e64`;
  the gated candidate `cb916d6`, then `46ccd7c` and `c48ce13` (a loop binding, comments,
  records).
- Verified findings: the Result above and R-0054.
- Commands executed, outcomes and report hashes: `local/evidence/front-end-main-menu/` in the
  main checkout.
  - The manifests `cold`, `cursor`, `buttons`, `cold-470`, `cursor-1000`, and their captures.
  - `compare.py`, `segments.py`, `captures.sh`.
  - `base-dd94e64/`: main's frozen binaries for the sweep.
  - `gates.sh` and `gates-cb916d6.out` (and the stopped `gates-5b271ca.out`), run in a detached
    `.worktrees/front-end-main-menu-gates`.
- Unavailable/skipped checks: the ASan presets (host; the Linux CI job covers them). The demo
  and the other modes are placeholders.
- Exact next experiment/command: FRONT-END-1P-SETUP. Capture 1P's screens (PICK A PLAYER, PICK
  TOUR, PICK TRACK, NOW PLAYING) with every-frame images and per-frame work RAM, as this task
  did, and extend `front_end.cpp` from the main menu's choice of 1P.

## Review and integration

- Tier 2: a fresh Claude Opus 5.5 subagent in `.worktrees/front-end-main-menu-review`.
  - At `5247f01` it **returned** the PR
    ([review](https://github.com/malmazuke/unirally-reconstruction/pull/28#pullrequestreview-5319175965)).
    The reason was M1: the Linux CI build failed on GCC's `-Wsign-conversion` in
    `snes_screen.cpp`.
  - **Its independent checks**, all equal or passing:
    - ctest 26/26 on three presets, the synthetic suite 518/518;
    - `compare.py` on the three captures with its build;
    - two withheld captures of its own. The first ran 2,200 frames: buttons on the boot screens,
      Up held through the load, frame-by-frame alternation, pad 2, opposing directions and the
      idle demo. All 2,182 frames and pictures match. The second held B across the menu's first
      frame;
    - the listing's menu, arrow, cycle and setup code against native's logic;
    - the pack's 23 entries against their ROM ranges;
    - the app with and without `--track`.
  - **Findings, fixed** in `b23b34f`, `cb916d6` and `46ccd7c`:
    - M1: explicit unsigned operands.
    - S1: no halving under clip-to-black-always.
    - S2: HDMA and OAM rotation refused.
    - S3: the v15 pack in the command docs.
    - S4: the main menu's codes are their own outcomes.
    - S5: the cold start's untaken paths recorded.
    - S6: the 14-byte cycle table.
    - S7: named frame waits and layout, four-digit direct-page addresses, `src/app`'s own
      formatting.
    - S8: the app's front-end summary.
  - A second GCC error (`-Wrange-loop-construct`) in the new code loop was fixed in `46ccd7c`.
    CI is green on `46ccd7c`.
  - Its note on equal row hashes (opposing-ride and m4-16-idle both `205d1705...`, and
    opposing-edges and m4-16-primary both `b4a34af7...`): `rows_sha256` is the hash of the
    reference rows a gate must reproduce. The opposing-input captures hold opposing
    directions, which the pad's rocker cancels, so they reproduce the idle and primary
    captures' rows. Every accepted gate run since HUNTER-EFFECTS (for example `gates-661fcd3`)
    shows the same pairs.
- Re-review: a fresh Claude Opus 5.5 subagent.
  - At `46ccd7c` it **approved**
    ([review](https://github.com/malmazuke/unirally-reconstruction/pull/28#pullrequestreview-5319435009)),
    with no must-fix findings.
  - It confirmed the fixes on both heads:
    - clip-to-black: grey before, white now, as bsnes;
    - the codes on either pad, with WIPE RAM first, and near misses still choosing 1P;
    - the 14-byte table;
    - green CI with the Linux sanitizer job.
  - It reran all three captures' pictures, with 0 differing pixels.
  - Its two should-fix items are fixed after it: R-0054 and FRONT-END-1P-SETUP's handoff now
    name the HDMA and OAM-rotation refusal (the 2P screens need HDMA), and the last four
    two-digit addresses in `front_end.hpp` comments have four digits.
