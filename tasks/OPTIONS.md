# OPTIONS - native options and records menus

## Assignment

- Status: claimed 29 September 2026 20:48 UTC; implementation in progress.
- Milestone: M4 original-game coverage, after TWO-PLAYER-VS and before LEAGUE in [COVERAGE-ROADMAP](COVERAGE-ROADMAP.md).
- Base: `f2da955d08e754a6af82af33a4411e1dc9a6198b`, synchronized local and remote `main` after [PR #45](https://github.com/malmazuke/unirally-reconstruction/pull/45).
- Branch and isolated checkout: `codex/options`, `.worktrees/options`.
- Provider and owner: OpenAI, current Codex desktop task. Runtime model is GPT-6; a specific slug and effort are not exposed. Independent review will use a fresh `gpt-5.6-sol`/medium child in a separate exact-candidate checkout, with no inherited conversation.
- Review tier: **1** under D-0008. The records, player names and league setup persist in cartridge RAM, so this outcome changes state and timing. Full D-0006 differential and independent review apply.
- Usage at claim: Codex weekly window 60% used, 40% remaining. The user's 29 September "Next task" instruction begins a new task after the previous task reached its 60% discretionary limit. This task's D-0004 discretionary boundary is 80% used, preserving the final 20% for review and recovery. Two reset credits were shown but none is authorized for redemption. Reassess at 45 minutes and checkpoint at least every 10 minutes.
- Evidence home: `local/evidence/options/` in the main checkout; gate and closeout home: `artifacts/options-integration/` there. Neither location is tracked. No capture or gate input points into another worktree.
- Owned paths: native front-end, presentation and pack code relevant to OPTIONS, focused tests, this task, its research record and coordinator state. LEAGUE's tournament flow and audio remain separate.

## Outcome and acceptance

From native power-on, OPTIONS must show and navigate its five choices, enter and leave the original records views, define and rename players, define league settings, and return to the main menu. Compare original PAL state, cartridge RAM and pictures across default and independently varied inputs, including persistence across a reset. Reuse existing front-end and save rules only where the original agrees. No original CPU code runs in the product.

Freeze the original paths before changing native behavior. Run focused and frozen regressions, the final gates in [BUILD_AND_VALIDATION](../docs/BUILD_AND_VALIDATION.md), an independent exact-candidate tier-1 review, and hosted checks on an up-to-date pull request. Integrate with a merge commit and write the ignored closeout.

## Claim observations and next experiment

- PR #45 is merged at `f2da955`; local `main` equals `origin/main`. `artifacts/two-player-vs-integration/closeout.json` records its final-tip checks and cleanup. The OPTIONS checkout's PAL ROM passed 10/10 identity fields. Its doctor check passes every required prerequisite; system Ninja and the optional `judge` key are missing, with the pinned isolated toolchain available.
- [The static listing](../docs/map/static/code-banks.md) was read at `local/evidence/static-code-map/static-map/bank-80.lst`, `$80:B626-$80:B66D`, before capture design. It classifies the OPTIONS dispatcher as unknown/gap candidate and contains a five-entry pointer table at `$80:B66E`. This is a static reading, not proof that a path executes.
- COVERAGE-ROADMAP's preliminary `mode-4` PAL capture enters OPTIONS at frame 620. Its picture at frame 700 shows RECORDS, DEFINE PLAYER, RENAME PLAYER, DEFINE LEAGUE and MAIN MENU. A picture at frame 1000 shows the records category screen (TRACK RECORDS, HIGH SCORES, PLAYER SCORES, GROUP TABLES, MAIN MENU). The repeated Start schedule has not established navigation or persistence.
- Next experiment: reproduce a short cold OPTIONS path with per-frame relevant work RAM and cartridge RAM, observe the records category and a return to the main menu, then design separate bounded schedules for each remaining option. Update [R-0072](../docs/research/R-0072-options.md) with observations before implementing.

## Handoff

Current branch is uncommitted at claim. The main checkout owns ignored evidence and closeout; the task checkout owns only tracked changes and build output. Next command is a focused PAL capture manifest derived from `local/evidence/coverage-roadmap/mode-4.json`, with new output under `local/evidence/options/`. Read the implemented command help in `docs/BUILD_AND_VALIDATION.md` and `tools/project.py` before running it. Keep observations, interpretations and implementation choices separate.

## Checkpoint - 29 September 2026 20:59 UTC

- The OPTIONS checkout's `app-debug` baseline build passes. The reference's first 900 frames were authenticated against COVERAGE-ROADMAP's older mode-4 capture: every video output, whole-WRAM digest and first 512 work-RAM bytes agree before the new input branch. This also rules out interference from the user's simultaneous controller use. One initial 96-entry trace-window gate failed; the 64-entry rerun passed, and only the latter is cited as a passing capture.
- [R-0072](../docs/research/R-0072-options.md) records the cold paths through every top-level choice, the four RECORDS categories, and confirmation versus Y-only controls for the two destructive warnings. SELECT+Y+A removes MIKE's name at frame 1600, or the first league label at frame 1651, before their next picker appears. The Y-only controls change no SRAM byte. All ROM-derived output is ignored under `local/evidence/options/` in the main checkout.
- A bounded read-only Astra/medium D-0004 architecture consultation found that immutable pack names feed rider selection, NOW PLAYING, local modes, results and race HUD. It recommended an owned mutable-name contract, complete SRAM projection, populated-data controls and exact mutation-frame gates. The primary accepts these gates. Its findings do not by themselves establish original behavior.
- Weekly used was 61% at 20:51 UTC, 1 point above claim and below this task's 80% discretionary boundary. No reset or purchase. Next: complete a non-default keyboard name, return and soft reset; confirm a populated removal; define league slots and browse records with non-cold data. Then implement the evidenced menus in small slices, compare against the frozen capture and expand the tested domain.

## Checkpoint - 29 September 2026 21:16 UTC

- `rename-g-finish` proves a G rename commits at frame 2001, including two `FF` bytes and a stale tail in the eight-byte name record. A subsequent DEFINE PLAYER screen displays G and its confirmation clears it. `define-player-populated-confirm` proves that confirmation also resets MIKE's statistics, best, medal, holder and level to cold values. `define-league-first-rider` has not yet completed a group; its first Start shows `2 MIN` and leaves the picker active.
- The first native slice adds OPTIONS/RECORDS entry and navigation plus four exact ROM content entries (proposed v28, 446 total). The `app-debug` front-end runner builds. In ignored `local/evidence/options/compare.py`, all 13 captured OPTIONS entry pictures and three RECORDS category-entry pictures match the original; sampled arrow and slide fields match after correcting signed arrow-column bytes. New `records-back` and `records-main` PAL captures each pass their required gates; native matches their five and four captured pictures, including the distinct Y-to-OPTIONS and choice-four-to-main returns. Five hidden OAM tile bytes still differ, and deep views/editors deliberately raise pending-recovery errors. This slice is not accepted.
- The pack was extracted to `local/classic-pal-crawler-tracks-v28.pack` in the main checkout and linked read-only in the task checkout; tracked manifest and required-entry pins were updated. Weekly usage was 66% at 21:15 UTC, below the 80% task boundary. No reset or purchase. Next: resolve hidden OAM, freeze the editor/record presentation and persistence path, then continue the native implementation and comparison.
