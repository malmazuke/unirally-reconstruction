# SPLIT-SCREEN-RACE - native race with two visible riders

## Assignment

- Status: **reviewed for integration**, source candidate `fed95fa`, [pull request #43](https://github.com/malmazuke/unirally-reconstruction/pull/43), 29 September 2026. The final merge and synchronization are recorded in the ignored closeout below.
- Milestone: M4 breadth, toward the native demo, 2P and VS modes.
- Coordinator, primary and integrator: this Codex desktop task.
- Task provider: OpenAI, fixed for the primary and reviewer (D-0004).
- Primary runtime/model: Codex desktop, GPT-6, current session setting.
- Review tier: **1**. Two-rider control, camera and race state change simulation timing and
  arithmetic; use the full independent review process (D-0008).
- Usage at claim: Codex weekly window 2% used, 29 September 2026 at 03:29 UTC; final 20%
  reserved, discretionary implementation stops after a 20 percentage-point increase (D-0004).
- Reviewers: the first fresh subagent was dispatched as `gpt-5.6-sol`/medium and the final
  fresh subagent as `gpt-6-sol`/medium, each without inherited conversation in an isolated
  checkout. Their reports identify the actual runtime as GPT-6 Codex desktop, without a more
  specific backend variant. The tier-1 review remained independent.
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
| First independent review | [PR #43 review](https://github.com/malmazuke/unirally-reconstruction/pull/43#pullrequestreview-5348783819) on `66b1b7c` required controller-word validation, refreshed native-symbol/static-map indexes and core formatting/function-size corrections. Its withheld port-2 Left+Right case matched neutral; build and 29/29 CTests passed. | Commit `178e7b3` bounds all eight controller words and rejects eight all-ones mutations in both F and G layouts; cites R-0069 at `$81:A23C`; regenerates indexes; splits four long functions and formats changed C++. [Agent reply](https://github.com/malmazuke/unirally-reconstruction/pull/43#issuecomment-5885514311) lists the resolution. |
| Corrected candidate checks | `python3 tools/project.py test --suite synthetic --preset app-debug` passed 538/538 on `178e7b3`; `ctest` passed 29/29 for lab-debug, lab-release and app-debug; Linux and macOS [PR checks](https://github.com/malmazuke/unirally-reconstruction/actions/runs/36535421554) passed. Both v1 presentation contracts passed separately. Three save continuations give the same frame-3348 SHA-256 `e6959537dce17dec9cc0004fd02f4b8cc4929e7fa34a249f8b096d9462bc3597`; 82 selected race pictures regenerated byte-identical to the accepted native set, which matches the original. | The full frozen regression run and independent re-review are still in progress. Gate logs belong under `artifacts/split-screen-race-integration/` in the main checkout. |
| Second independent review | [PR #43 re-review](https://github.com/malmazuke/unirally-reconstruction/pull/43#pullrequestreview-5349056043) found that a saved rotation window of 49 was accepted although native writes only 0 or 50; `deserialize_race` had 84 lines; full LLVM 19 formatting still flagged old lines in changed files. The reviewer reproduced the prior F/G mutation fix and passed synthetic, restores and hosted CI. | The next correction requires window 0 or 50, adds F/G mutations of 49, extracts validation from `deserialize_race`, formats the entire changed core files and refreshes the indexes. Three restore checks and 29/29 CTests pass locally on these changes. |
| Final independent review | [PR #43 final review](https://github.com/malmazuke/unirally-reconstruction/pull/43#pullrequestreview-5349147810) accepted `fed95fa` after inspecting the full diff and repeating both F/G mutation families, 29 CTests, 506 tooling tests, synthetic repeatability and an original PAL 53-frame projection. The only remaining full-file LLVM 19 diagnostics are at `front_end.hpp` lines 276, 287 and 288, byte-identical to base `490302a`. | The reviewer found no remaining task defect. Corrections are in `fed95fa`; [Agent reply](https://github.com/malmazuke/unirally-reconstruction/pull/43#issuecomment-5885683824) accounts for the second review. |
| Final original and native comparison | The final source has 357/357 retained first-demo pictures equal across 334 frame labels; two injected controller variations have 31/31 equal pictures each; the 565-byte race projection and extra camera/demo fields remain exact on all 1,901 frames. Original `$137B/$137D` rotation-window values are only 0 or 50 throughout that domain (R-0069). Three split save continuations reproduce frame 3348, and both F/G layouts reject malformed controller words and window 49. | The claim is limited to the first ZOOM ZOO idle demo and injected controls. The second idle cycle and 2P/VS menu paths remain separate work. |
| Final integration gates | On `fed95fa`, all 11 frozen private races passed: five ZOOM ZOO and six DRAGSTER, including restores and repeat reference checks (`artifacts/split-screen-race-integration/private-fed95fa/summary.json`). The full app-debug synthetic run passed on `178e7b3` (538/538); final-head independent review repeated 506 tooling tests and 29 CTests. Local lab-debug, lab-release and app-debug CTests passed 29/29 each; v1 winner 8/8 and loser 2/2, hidden desktop demo through its return, 1P continuation 1,108/1,108 pictures and bounded cold menu comparison passed. [Hosted CI](https://github.com/malmazuke/unirally-reconstruction/actions/runs/36536643833) on `fed95fa` passed macOS and Linux, including the Linux sanitizer step. | A local macOS `app-sanitize` CTest attempt timed out at 600 seconds, and a fresh repeat timed out at 660 seconds while source changed; neither is a pass. Hosted Linux sanitizer and all other final-source checks above passed. The broad historical gate script was stopped because it repeated unrelated routes; the task-specific 11-case matrix completed. |

## Handoff

- Base `490302a`; source commits `66b1b7c`, `178e7b3` and `fed95fa` on
  `task/split-screen-race`, pushed to `origin`; [PR #43](https://github.com/malmazuke/unirally-reconstruction/pull/43).
- Evidence: `local/evidence/split-screen-race/` in the main checkout. The ROM and core identities,
  state projection, picture sets, bounds and injected controls are detailed in R-0069. Final
  ignored gate reports are under `artifacts/split-screen-race-integration/` in the main checkout.
- Review trail: the first two returns and their Agent replies are linked above; final independent
  acceptance is on `fed95fa`. The initial candidate's hosted CI failed three stale-index tests;
  regeneration fixed them. An early local synthetic test failed because its temporary directory
  used the main checkout's `local` symlink; the test now uses its own checkout and passed on rerun.
- Current checks and limitations are in the final integration-gates row. This documentation-only
  successor to `fed95fa` carries the handoff and queues ATTRACT-DEMO. The final-tip hosted CI,
  merge commit, evidence cleanup and source-control synchronization are recorded in
  `artifacts/split-screen-race-integration/closeout.json` after PR merge.
- Next experiment: capture the second idle cycle beginning after the first demo's menu return;
  bound its one-player track-3 state and pictures against the original before implementing its
  transition. If that comparison diverges, reproduce the exact original input/camera frame
  before changing native arithmetic or a frozen expectation.
