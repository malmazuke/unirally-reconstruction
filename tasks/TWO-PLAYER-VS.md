# TWO-PLAYER-VS - local two-rider modes

## Assignment

- Status: **in progress**, claimed 29 September 2026 11:31 UTC after ATTRACT-DEMO merged in [PR #44](https://github.com/malmazuke/unirally-reconstruction/pull/44).
- Milestone: M4 original-game coverage; next outcome in [COVERAGE-ROADMAP](COVERAGE-ROADMAP.md).
- Base: `5df56c6d6b80433aafd0d08ee0df8f572893c281` (`main` and `origin/main` at claim).
- Branch and isolated worktree: `codex/two-player-vs`, `.worktrees/two-player-vs`.
- Provider and owner: OpenAI, this Codex desktop task. The runtime reports GPT-6; a more specific model slug and effort are not exposed, so neither is inferred. A fresh `gpt-5.6-sol`/medium reviewer will inspect the exact candidate in a separate checkout.
- Review tier: **1** under D-0008. Two human controller paths, race setup, result state and timing change simulation state. Use the full independent review process.
- Usage at claim: Codex weekly window 40% used, 60% remaining, sampled 29 September 2026 11:31 UTC. The D-0004 discretionary implementation boundary is 60% used for this task and the final 20% weekly allowance is reserved for review and recovery. One reset credit exists but is not authorized to redeem. Reassess at 45 minutes and checkpoint at least every 10 minutes.
- Evidence home: `local/evidence/two-player-vs/` in the main checkout; gate and closeout home: `artifacts/two-player-vs-integration/` there.
- Owned source: relevant native frontend, race, presentation, content and tests; this task, its research record and coordinator records. No other task checkout is owned.

## Outcome and boundary

From the original main menu, make 2P and VS playable natively through their player-selection screens, race, result and return/restart paths. Both controller ports must select and control their respective riders where the original does. Reuse accepted split-race and one-player mechanisms only where the measured original state agrees. Keep LEAGUE, OPTIONS, further idle cycles and audio outside this task. No original CPU code runs in the product.

The prior roadmap's first-screen captures are reconnaissance only: they ran to frame 3000 with repeated Start and did not establish either mode's complete flow. Freeze new original PAL paths and independent variations before claiming native equality. If a distinct mode warrants a separate acceptance task because of ownership, outcome or review size, record the boundary and rationale without resetting this task's quota baseline.

## First experiment and acceptance

1. Verify the pinned PAL ROM and bsnes core, the accepted one-player and split-demo foundation, and the existing 2P/VS reconnaissance. The static listing at `artifacts/static-map/bank-80.lst` marks `$80:BCBF` and `$80:BF49` as unknown bytes, so capture their executed instruction paths before interpreting them. Cite the listing as the static source and a dynamic trace for gameplay claims.
2. Capture each mode from power-on through its player choices and first race update, with frame inputs, full relevant WRAM/SRAM and picture samples. Repeat the reference independently and establish the original screen and race timelines before implementation.
3. Implement the measured setup, controls, two-view race, result and continuation. Compare native state and pictures throughout a complete default path for each mode. Exercise independently chosen rider/port inputs, back paths and save/restore boundaries; name the exact tested domain and refuse unrecovered choices.
4. Preserve the frozen one-player, split-demo and attract-demo paths. Run affected and final gates under BUILD_AND_VALIDATION, obtain exact-candidate independent tier-1 review, and integrate through a PR with a merge commit and closeout.

## Claim observation and next experiment

- `main` and `origin/main` agree at the ATTRACT-DEMO merge. Its ignored closeout at `artifacts/attract-demo-integration/closeout.json` records the final-tip CI and cleanup. `doctor` passes all required checks; system Ninja is optional and the isolated pinned toolchain is present.
- COVERAGE-ROADMAP's two captures use Start at frames 620, 900 and every 150 frames, with one or two Down presses on the main menu. Their coverage runs reach only the first screens. Their source is `local/evidence/coverage-roadmap/mode-1.json` and `mode-2.json`.
- The static listing's mode entries `$80:BCBF` and `$80:BF49` are currently unknown/data, despite the high-level dispatch being observed. A new dynamic capture is required before a routine reading or native behavior claim.
- Next: set up this isolated checkout's local pinned toolchain and ROM locator, reproduce the mode-1 and mode-2 cold paths with a short capture through their first screens, and inspect their frame state and rendered pictures. Then choose controlled inputs that reach the next screen and race entry.

## Checkpoint - 29 September 2026 11:58 UTC

- The isolated checkout has real ignored `local/` and `artifacts/` directories, a copied ROM locator and links to the pinned local toolchain, emulator, cache and v26 pack. `rom inspect --expect` passed 10/10 identity fields. Two fresh 3,000-frame PAL captures reproduce the old mode-1 and mode-2 whole-WRAM and video hashes on every frame, zero differences.
- The original's second rider selection takes port 2. With MIKE chosen first at frame 900, repeated Start or port 1 Down does not progress. Port 2 Down at 1120 and Start at 1200 selects MARTIN (`$017F=2`) and shows PICK TOUR by 1250 in both modes. [R-0071](../docs/research/R-0071-two-player-versus.md) separates those observations from the routine reading and lists the exact captures.
- The tracked static listing had both handlers as unknown. Temporary maps from the new dynamic captures decode `$80:BCBF` and `$80:BF49` as observed, with 8,254 sites agreeing on address/length. The handlers compare the second choice with `$017D`; a match loops at the second prompt. Their subsequent tour and race code is not yet dynamically covered.
- Codex weekly usage is 43% at 11:58 UTC, up 3 points from claim and below the 60% discretionary boundary. No reset was used. Next: extend each successful capture with timed tour/track/now-playing presses through the first race update and obtain per-frame WRAM/SRAM and picture windows before implementation.

## Checkpoint - 29 September 2026 12:08 UTC

- Both extended original paths reach PICK TRACK by 1600, NOW PLAYING by 1750 and split DRAGSTER pictures from 1995. Their race video hashes agree on all 1,205 frames 1995-3199 with the exploratory Start pulses still in the race schedule. NOW PLAYING differs on 118 video hashes from 1739-1856; pixel/field cause is still unexamined. Do not call those screens equal.
- Original access captures authenticate the continuation sample digests and retain `$0000-$21FF` every frame. `$0DE1` becomes 1 at 1899 and `$212C` stays 0. Mode 1 enters with SRAM option word `$0750=0xC208` and flags `$0742=0x0A`; mode 2 has `0xC20C` and `0x0E`. Both pair MIKE/MARTIN (`$0748/$0749=0/2`) at 1857. At 2500 only six captured work-RAM bytes differ, but their meaning and later mode consequences remain open. See R-0071 for the exact domain.
- A bounded Astra/medium D-0004 architecture consultation was read-only. Its most consequential source risks are the menu-race bridge dropping port 2, the one-player scenario factory rejecting human opponent IDs, result logic assuming rider 1 and serialization refusing split DRAGSTER. The primary accepts a small internal slice through 300 measured two-pad updates but keeps natural finish, result and return in both modes as task acceptance. No implementation change yet.
- Weekly used is 45% at 12:08 UTC. Next: remove race-time Start pulses from the PAL schedule, capture asymmetric two-pad controls with work RAM and pictures, then align a native initializer with the exact original state. Do not generalize demo options to 2P or VS without their measured fields.

## Reassessment - 29 September 2026 12:22 UTC

- The isolated `app-debug` build passed with the pinned toolchain on the unmodified source. The core already has a human port-2 branch (`race_update.cpp`), but its menu-race bridge and app path forward port 2 only for a demo. The one-player scenario factory rejects a human second rider, and split serialization is gated to ZOOM ZOO. These are implementation facts; the reference captures above determine which paths to extend.
- Measured first split race: both modes are black at 1857-1994 and have byte-identical video hashes on every frame 1995-3199 under the exploratory pad schedule. At 2500 their captured `$0000-$21FF` work RAM differs on only six bytes in the menu/saved-menu range. The different `0xC208`/`0xC20C` SRAM options and later results still require separate testing. No native 2P/VS code has been claimed accurate.
- The internal checkpoint remains both native selections through MIKE/MARTIN split DRAGSTER initialization plus 300 independently controlled updates. This does not narrow task acceptance: both modes must continue through measured results and return/restart. The next cheapest experiment is a neutral race capture and each-rider-finish variations, then implementation of the observed first path and its differential check. This reassessment continues the task under D-0006; it is not a session stop.
- Codex weekly usage 47% at 12:22 UTC, seven points above the 40% claim baseline; the D-0004 discretionary boundary remains 60% used and the review/recovery reserve begins at 80%. No reset or purchase. Next checkpoint within ten minutes.

## Checkpoint - 29 September 2026 12:32 UTC

- Four fresh 5,500-frame PAL finish captures remove post-entry Start. Both ports holding Right produce a natural MIKE/MARTIN result by frame 4400 in 2P and VS: MARTIN 0:34.37, MIKE 0:34.41. Driving only one port leaves the other rider active and no result screen appears through 5499. This is an observed domain, not a general result rule. R-0071 names the captures.
- With Start at frame 5200 after the shared result, 2P reaches a five-choice NEXT TRACK/SAME TRACK/SELECT TRACK/SELECT TOUR/QUIT screen by 5300. VS reaches a VS CHAMPIONS table instead. Both remain there through 6499 without further input. These distinct continuation outcomes justify separate implementation modules and separate evidence cases within this task; the current task still owns both modes.
- Source is still unchanged from base; `app-debug` built successfully. The next native work is to preserve selected mode and human pairing through the shared setup screens, extract the ROM's mode-specific titles, and forward port 2 into the race. The finish and continuation are not yet implemented or reviewed. Codex weekly usage was 48% at 12:32 UTC, eight points from claim and below the 60% discretionary boundary. No reset.
