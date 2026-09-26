# FRONT-END-ENDINGS - the gold medal's endings and the reveal of new tours

## Assignment

- Status: **accepted** (tier 2, pull request #35, merged in `16fe22c`; round 2 approved). Queued 26 September 2026 (UTC) by FRONT-END-TOUR-END; claimed
  26 September 2026 at 10:05Z by the Claude Code desktop session that ran RACE-RIDERS-OPPONENTS,
  on `be11fa8`.
- Milestone: M4 (original game coverage: menus)
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`)
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  **tier 2** (screens and their records).
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): at claim the 5-hour window was 30% used and the weekly window 48%. The
  user's allowance: continue until the weekly window reaches 80%.
- Reviewer (primary automatically spawns fresh model/effort, isolated checkout; no user
  trigger): a fresh Anthropic subagent.
- Dependencies and evidence of acceptance: FRONT-END-TOUR-END (R-0059, the award and the way back).
- Base commit: the `main` tip at claim.
- Branch and isolated worktree: `task/front-end-endings` in `.worktrees/front-end-endings`.
- Owned paths and shared interfaces: `src/core/award.cpp` and the front end's files, pack rules
  and content for the endings, native tests, a research record, this record, `docs/STATE.md`,
  `tasks/README.md`, `tasks/NEXT_SESSION.md`.
- Claim/lease/heartbeat/checkpoint location: this record.
- Session time limit, concurrency allocation and actual spend authorization if relevant: one
  worker plus the review subagent; no monetary spend.

## Outcome and boundaries

What R-0059 leaves:
- **The gold medal's endings.** A third completion of a tour plays that tour's ending
  (`$83:88FD`), one routine per tour: a gold award screen and a scripted animation, then the
  restore. HUNTER's (`$83:AB9A`) shows picture screens and ends in a soft reset (`$80:8858`).
  Native now leaves a gold completion at once through the award's way out.
- **The reveal.** Whenever the unlock rule's counts match, even with the level unchanged, PICK
  TOUR draws the tours of level - 1, slides, then shows the others four frames later
  (`$80:E588-E5A2`, `$77:10FD`). Native shows the level at once.
- **A completion by a fifth win**, captured from a preloaded cartridge RAM with four done tracks.
  The laboratory runner must then start from those records.

Make native do all three, frame for frame. `local/evidence/front-end-tour-end/decode/tour-end.md`
section 5 outlines every ending.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Pictures and state | Captures of the original reaching each tour's gold by forced completions (preloaded cartridge RAM with silver medals, R-0050's method), and a reveal (four bronze medals), against native | Pictures match to the pixel and the words agree frame by frame, or each residue is explained | pictures, logs, research record |
| Fifth win | A capture from a preloaded cartridge RAM with four done tracks, the fifth race won | The same completion as a forced one, frame for frame | pictures, logs |
| Records | `sram.py` after each | The medals, levels and reveal equal the original's cartridge RAM | log |
| Nothing moves | The gates of FRONT-END-TOUR-END | Unchanged | logs |

## Result

[R-0062](../docs/research/R-0062-gold-endings.md). The eight tours' gold endings and the reveal of
new tours are native and match the original frame for frame. Captures from power-on use forced
completions of races quit through the pause menu, so no cartridge RAM preload was needed. Two parts
moved to their own tasks: HUNTER's ending, which ends in a soft reset
([HUNTER-ENDING](HUNTER-ENDING.md)), and a completion by a fifth win
([FIFTH-WIN-COMPLETION](FIFTH-WIN-COMPLETION.md)), which needs the runner to start from given
records. The work was split: a research worker made the captures and the decode; the primary wrote
the scheme, CRAWLER's ending and the reveal; an implementation worker wrote the other seven tours'
scripts. Tier 2 stands.

Along the way the front-end runner stopped overwriting a race's frame label, which had made the
race engine refuse HOPPER's first track (a race starting before its scenario's frame).

## Evidence

Captures and scripts in `local/evidence/front-end-endings/`: `decode/` (the seven captures with
their manifests, pictures and work RAM, `endings.md`, the listings, `compare.py`, `sram.py`),
`base-be11fa8/` (main's binaries for the equivalence sweep), `gates.sh`.

| Criterion | Result |
| --- | --- |
| Pictures and state | crawler-gold, shuffler-gold, walker-gold, hopper-gold: no state difference and every picture equal (830, 1,030, 1,030, 1,030). locked-gold (JUMPER, BOUNDER; 16,300 frames) and all-gold to frame 39700 (24 completions, RUNNER, SPRINTER, the level-2 and level-3 reveals): no state difference, every compared picture equal. |
| Reveal | The reveal capture: no state difference, 600 of 600 pictures. |
| Fifth win | Moved to FIFTH-WIN-COMPLETION. |
| Records | `sram.py` (now with `$77:10FD`): the kept words equal through the reveal, and at the gates' frames. |
| Nothing moves | The gates on `58a635d` (`local/evidence/front-end-endings/gates-58a635d.out`, 12:29-14:15Z, 106 minutes). The three presets build, ctest 27 of 27, the synthetic suite, both v1 contracts and every hidden app run pass. The eleven differential gates pass (179 to 801 restores each). The equivalence sweep against main's binaries (base `be11fa8`, pack v21) compares 1,933,523 updates, 1,047 restarts and 2,052 pictures with no difference. The per-track recompare of both sweeps is identical on 20 and 25 tracks. Every earlier front-end comparison and RACE-RIDERS-OPPONENTS' shows no difference and all its pictures equal; the seven race captures' pictures are as that task left them (the off-screen arrow). This task's eleven windows (four golds, the reveal, JUMPER, BOUNDER, RUNNER, SPRINTER, the level-2 and level-3 reveals) show no difference and every picture equal. The records match at 34 frames. No function is over 80 lines; the address index passes. The race pictures step first ran on the wrong pack default and was re-run on the same head (`gates-58a635d-pictures.out`; `gates.sh` now passes the pack). |

## Review

Tier 2, a fresh Opus 5.5 subagent in an isolated worktree. Round 1 (`92c9fc2`): changes requested (`review-92c9fc2.md`): a function over 80 lines, HOPPER's and JUMPER's pose names, the NMI-hook comments, the registry, a test of every tour's ending; answered in `a5299e9`, `9e50dc5`, `552c554`, `58a635d`. Round 2 (`58a635d`): approved, provided these gate results are recorded (`review-58a635d.md`).

## Handoff

- Exact next experiment/command: after the review and the merge,
  [RACE-OFFSCREEN-ARROW](RACE-OFFSCREEN-ARROW.md) or [HUNTER-ENDING](HUNTER-ENDING.md).
