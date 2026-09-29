# ATTRACT-DEMO - the next idle demo cycle

## Assignment

- Status: **ready** after SPLIT-SCREEN-RACE integrates by PR #43.
- Milestone: M4 coverage of the original main menu's idle modes.
- Dependency: [SPLIT-SCREEN-RACE](SPLIT-SCREEN-RACE.md) and its first-cycle evidence
  [R-0069](../docs/research/R-0069-split-screen-race.md).
- Review tier: **1** under D-0008. The next demo cycle chooses a race mode and rider controls;
  its timing and state updates are gameplay claims requiring the full D-0006 review and gates.
- Provider, model, usage baseline, branch and owner: select and record at claim under D-0004.
- Evidence home when claimed: `local/evidence/attract-demo/` in the main checkout.

## Outcome and boundary

After the first idle split ZOOM ZOO race returns to the main menu, reproduce the original's next
idle demo cycle natively from power-on through its own return. The original reaches a one-player
track 3 with SRAM `$77:0750=0xC202` by frame 4399; retained pictures at frames 5500 and 6999
differ from the current native build (R-0069). Treat the track and controls as measured inputs,
not an assumed repeat of the first cycle. Keep 2P and VS menu selection, arbitrary demo cycles
beyond this one, and audio outside this task unless a verified dependency requires more scope.

## First experiment and acceptance

1. Start from the pinned PAL ROM/core and `local/evidence/split-screen-race/demo-primary-full/`
   schedule: Start on frames 300-305, then released through frame 6999. Repeat the original cold
   capture independently, record the second cycle's transition, race pairing, selected track,
   controller writes, return frame and every relevant SRAM/WRAM field. Consult the static code
   map's listing before designing a focused trace and cite it for each routine reading.
2. Freeze the original's frame inputs, full per-frame state and picture samples through the
   second cycle. Compare the native transition, race state and all retained pictures against
   that freeze. Investigate the first divergence, including any result or menu fade.
3. Add independently chosen control or timing variations for reached branches, plus save and
   continuation checks wherever the new path serializes state. Define and record the tested
   domain precisely; refuse unrecovered paths explicitly.
4. Keep the first split demo, one-player menus and race freezes equal. Run the complete task
   gates, hosted macOS/Linux CI, fresh independent tier-1 review and the PR merge workflow.

## Handoff from SPLIT-SCREEN-RACE

- The first demo initializes its two-rider ZOOM ZOO race at frame 1448 and requests exit at
  frame 3348; its menu is stable by frame 3500 (R-0069). The second cycle is a distinct
  one-player track 3 by frame 4399.
- Original whole-WRAM/SRAM/video hashes for frames 1300-6999 are in
  `local/evidence/split-screen-race/demo-primary-full/reference.json`; the original capture is
  repeated by `demo-repeat` and `demo-transitions`. The second cycle needs its own focused
  capture and native comparison before any implementation decision.
- Next command: inspect the frame 3500-4399 transitions in that capture and the mode-5 static
  listing, then capture a dense original window across the second race entry and first updates.
