# HUNTER-ENDING - HUNTER's gold ending and the soft reset

## Assignment

- Status: **in review**. Queued 26 September 2026 (UTC) by FRONT-END-ENDINGS; claimed 26 September
  2026 at 20:40Z by the Claude Code desktop session that ran RACE-OFFSCREEN-ARROW, on `2ffcea0`.
- Milestone: M4 (original game coverage: menus)
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`)
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  **tier 2** (screens).
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): at claim the 5-hour window was 12% used and the weekly window 57%. The
  user's allowance: continue until the weekly window reaches 80%.
- Reviewer (primary automatically spawns fresh model/effort, isolated checkout; no user
  trigger): a fresh Anthropic subagent.
- Dependencies and evidence of acceptance: FRONT-END-ENDINGS (R-0062, the other tours' endings and
  the reveal).
- Base commit: the `main` tip at claim.
- Branch and isolated worktree: `task/hunter-ending` in `.worktrees/hunter-ending`.
- Owned paths and shared interfaces: the front end's files, pack rules and content for the ending,
  native tests, a research record, this record, `docs/STATE.md`, `tasks/README.md`,
  `tasks/NEXT_SESSION.md`.
- Claim/lease/heartbeat/checkpoint location: this record.
- Session time limit, concurrency allocation and actual spend authorization if relevant: one
  worker plus the review subagent; no monetary spend.

## Outcome and boundaries

HUNTER's gold ending (`$83:AB9A`) is not like the other eight: after the fade it shows a newspaper
picture (the "DAILY NEWS" set, or another selected by the title code's flag `$77:10D0`) until a
press, a second picture scrolled in by HDMA (`$80:E2CF`: BG1VOFS and INIDISP tables) until a press or
1,201 frames, then the credits, eleven faces on animated unis, until a press, and then a soft reset
(`JML $80:8858`) to the Nintendo screen and the title, with no unlock rule, no checksums and no
PICK TOUR. Native now leaves a HUNTER completion at once through the award's way out. Make native
play it frame for frame, including the reset back to the boot screens with the records kept.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Pictures and state | `local/evidence/front-end-endings/decode/all-gold` (all 25 completions from power-on; pictures every frame 39780-42786) through the front end's `compare.py` | Every picture and the menus' words equal, or each residue explained | logs, research record |
| Records | `sram.py` after the reset | Equal to the original's cartridge RAM | log |
| Nothing moves | The gates of FRONT-END-ENDINGS | Unchanged | logs |

## Result

[R-0064](../docs/research/R-0064-hunter-ending-and-soft-reset.md). HUNTER's ending, its newspaper
pages rolled down line by line, the credits and the soft reset back to the Nintendo screen with the
records kept are native, and so are the title code (Up, Left, Up, R, A: the CHEAT! page) and the
main menu's code (B, Down, L and R: straight into the ending). The renderer now applies per-line
INIDISP and BG1VOFS. Pack profile v23. A research worker decoded it; an implementation worker wrote
it; the primary integrated. Tier 2 stands.

## Evidence

`local/evidence/hunter-ending/`: `decode/` (the research), the captures `code-route` and
`cheat-route` with `captures.sh`, this task's `compare.py` and `sram.py`, `logs-e12f6b6/`.

| Criterion | Result |
| --- | --- |
| Pictures and state | all-gold's HUNTER ending and reset: no difference on 28,487 compared frames, 3,007 of 3,007 pictures. code-route: 1,850 frames, 1,410 of 1,410. cheat-route (two resets): 2,800 frames, 1,060 of 1,060. The 17 frames after each reset, where the original is still clearing work RAM, are not compared. |
| Records | all-gold: the kept words equal at 39786, 42302, 42709 and 42786. The code routes: equal but for `tries` (`$77:1073`), an older difference of native's cold start (R-0064). |
| Nothing moves | The gates (below). |

## Handoff

- Exact next experiment/command: after the review and the merge,
  [FIFTH-WIN-COMPLETION](FIFTH-WIN-COMPLETION.md).
