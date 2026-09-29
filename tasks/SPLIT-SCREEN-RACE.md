# SPLIT-SCREEN-RACE - native race with two visible riders

## Assignment

- Status: **in progress**, review candidate being prepared, 29 September 2026.
- Milestone: M4 breadth, toward the native demo, 2P and VS modes.
- Coordinator, primary and integrator: this Codex desktop task.
- Task provider: OpenAI, fixed for the primary and reviewer (D-0004).
- Primary runtime/model: Codex desktop, GPT-6, current session setting.
- Review tier: **1**. Two-rider control, camera and race state change simulation timing and
  arithmetic; use the full independent review process (D-0008).
- Usage at claim: Codex weekly window 2% used, 29 September 2026 at 03:29 UTC; final 20%
  reserved, discretionary implementation stops after a 20 percentage-point increase (D-0004).
- Reviewer: fresh `gpt-5.6-sol`/medium subagent, no inherited conversation, isolated checkout of
  the exact candidate, once a coherent candidate passes focused checks.
- Base: `490302a` (`main`, `origin/main` at claim).
- Branch and worktree: `task/split-screen-race`, `.worktrees/split-screen-race`.
- Evidence home: `local/evidence/split-screen-race/` in the main checkout; closeout:
  `artifacts/split-screen-race-integration/closeout.json`.
- Owned implementation: `src/core/`, relevant content extraction/manifests, focused tests,
  research record, this task and coordinator records. No other worker is active.

## Outcome and boundary

Reconstruct the original's shared two-rider split-screen race natively, with each rider's
control and viewport, from the idle main menu through the first demo race and its return to the
menu. Use the idle demo first as the original's two-computer-rider input. Leave the next idle demo
cycle and the 2P and VS selection
screens and their records to their queued tasks. The race mechanism must also admit human
controls from both ports without replacing unknown rules with plausible behavior. No original
code executes in the product.

The formal roadmap's RESULT-ICONS entry is already fulfilled for the one-player product path by
FRONT-END-1P-CONTINUATION (R-0057): the row icons, 1P marker, rider and trophies are drawn by
`src/core/race_result.cpp`, with frame-by-frame comparison. The older standalone result renderer
still omits them. This task follows the actual missing game capability and updates the roadmap.

## Acceptance

1. Capture the PAL original from power-on through the demo's split race and return, with the
   exact ROM/core identity, controls, full per-frame work RAM and video for discriminating
   windows. Reproduce the capture independently before using it as a baseline.
2. For the original demo path, native matches the original's measured race state and every
   compared picture, including both viewports and their HUD, through the return.
3. At least two independently chosen controller variations demonstrate both rider inputs and
   the split-specific paths they reach, with saved-state/continuation checks where state is
   serialized. Refuse unrecovered modes explicitly.
4. Existing one-player races and menus remain equal on affected frozen checks. Run the complete
   integration gates, macOS/Linux hosted checks, independent review and pull request workflow.

## Evidence and attempts

| Attempt | Observation | Interpretation and next experiment |
| --- | --- |
| Initial inventory | The idle capture `local/evidence/coverage-roadmap/idle` reaches the demo at frame 900; frames 1600 and 2500 show two stacked viewports, AMY and ALICE. The native front end still exits mode 5 with a notice. `R-0054` says the 2P setup also needs HDMA. | Read the static listing at `$80:93FB` and around the shared race setup, then capture the demo's transition and first active updates with work RAM and selected frame images. |
| Independent replay | The first demo race uses ZOOM ZOO, AMY and ALICE (race SRAM `$77:0748/$0749=4/14`); SRAM `$77:074A/$074B=1`, `$77:0750=0xC20A`. It initializes at frame 1448 and returns to the menu by frame 3500. Three fresh cold replays from frame 1300 through 6999 had equal whole-WRAM, SRAM and video hashes on all 5700 frames. | Constrain acceptance to the first split race; the next demo cycle has a different mode. R-0069 records identities and hashes. |
| First native differential | At initialization the original's first 565 projected bytes equal the accepted native ZOOM ZOO initial state. At update 1 the opponent's screen position and `$1227` differ; at update 2 its physics diverges. Restoring a native seed with just these two bytes changed removes that next physics difference. | Add the second camera and split visibility before drawing; these published positions participate in physics. |
| Full native demo | Native split state matches the original's 565-byte projection and the second camera/demo fields on all 1,901 race frames 1448-3348. All 328 retained RGB pictures from the first demo path are equal; the two pictures from the out-of-scope second cycle differ. | The first demo path and its visible return are implemented; R-0069 states the tested picture set and boundaries. |
| Two withheld inputs | With the original's demo flag `$7E:212C` cleared before frame 1650, independent port 1 Right+B and port 2 Left+A runs both differ from an injected neutral control. Native equals all 31 original RGB frames 1650-1680 for each variation. | Both original port readers reach the native split rider path. This is an injected race-state experiment, not the 2P menu. |
| Save continuation | `split_demo_runner --restore-check` at 1650, 2500 and 3320 round-trips `URZZ000F/G` and equals uninterrupted serialized state through 3348. | The two-view camera and demo controls now survive native save/resume. |

## Handoff

- Current base: `490302a`; candidate commit, pull request and integration evidence will be
  recorded here after review and checks.
- Evidence: `local/evidence/split-screen-race/` in the main checkout. The ROM and core identities,
  state projection, picture sets and injected controls are detailed in R-0069.
- Current checks: `ctest --test-dir build/app-debug --output-on-failure` passes 29/29.
- Remaining: independent review, complete local/hosted gates, pull request integration and
  local evidence closeout. The next demo cycle and 2P/VS setup stay queued separately.
