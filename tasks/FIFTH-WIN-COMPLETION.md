# FIFTH-WIN-COMPLETION - a tour completed by its fifth win

## Assignment

- Status: **in review**. Queued 26 September 2026 (UTC) by FRONT-END-ENDINGS; claimed 26 September
  2026 at 23:55Z by the Claude Code desktop session that ran HUNTER-ENDING, on `e01aaaa`.
- Milestone: M4 (original game coverage: menus)
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`)
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  **tier 2** (screens and their records), unless a race's scoring changes (tier 1).
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): at claim the 5-hour window was 4% used and the weekly window 60%. The
  user's allowance: continue until the weekly window reaches 80%.
- Reviewer (primary automatically spawns fresh model/effort, isolated checkout; no user
  trigger): a fresh Anthropic subagent.
- Dependencies and evidence of acceptance: FRONT-END-TOUR-END (R-0059), FRONT-END-ENDINGS (R-0062).
- Base commit: the `main` tip at claim.
- Branch and isolated worktree: `task/fifth-win-completion` in `.worktrees/fifth-win-completion`.
- Owned paths and shared interfaces: the front end's scoring, the laboratory runner's start from
  given records, native tests, a research record, this record, `docs/STATE.md`, `tasks/README.md`,
  `tasks/NEXT_SESSION.md`.
- Claim/lease/heartbeat/checkpoint location: this record.
- Session time limit, concurrency allocation and actual spend authorization if relevant: one
  worker plus the review subagent; no monetary spend.

## Outcome and boundaries

Every completion captured so far is forced (pad 1 holding Select + X + R as a result is left). The
ordinary way is a fifth won race on a tour, which R-0059 reads from the listing (`$83:881B` on is
the same path). Capture it and compare: from power-on that needs five won races on one tour, and
the laboratory has winning inputs only for DRAGSTER; the shorter way is a cartridge RAM preload with
the tour's other four done tracks (`$77:1075` + 5 x tour, and the checksums `$83:90F4` would write),
which needs the front-end runner and its comparison to start from given records.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Fifth win | A capture of a fifth won race on a tour (preloaded or from power-on) against native | The completion, the award and PICK TOUR frame for frame | pictures, logs |
| Records | `sram.py` after it | Equal to the original's cartridge RAM | log |
| Nothing moves | The gates of FRONT-END-ENDINGS | Unchanged | logs |

## Result

[R-0065](../docs/research/R-0065-fifth-win-completion.md). A tour completed by its fifth won race
is captured from power-on and native matches it frame for frame. A real fifth win is out of reach
(each tour's third track is a stunt event), and a power-on preload does not survive the rider's
choice, which clears the done tracks. So the route wins DRAGSTER with CRAWLER's other four done
tracks written into cartridge RAM during the race. Native's scoring needed no change for it; its
completion test is now the original's 8-bit sum of the five done bytes, the same for every byte the
game writes. Replay manifests and the reference worker take cartridge RAM writes, and the
front-end runner replays them (`--record-write`). A research worker decoded it; the primary wrote
the rest. Tier 2 stands: the scoring change moves nothing the game can reach.

## Evidence

`local/evidence/fifth-win-completion/`: `decode/` (the research, its own capture and this task's
`compare.py` and `sram.py`), `project/` (the same manifest through `coverage capture` and `access
capture`).

| Criterion | Result |
| --- | --- |
| Fifth win | fifth-win, power-on to 6300: no difference on 4,055 compared frames; 2,511 of 2,511 pictures equal (3790-6300). Without the writes (main's runner) native diverges at 3805. |
| Records | Equal to the original's cartridge RAM at 1000, 3803, 3804, 3900, 5600 and 6300. |
| Nothing moves | The gates on `fe26824` (`local/evidence/fifth-win-completion/gates-fe26824.out`, 00:11-02:03Z, 112 minutes). The three presets build, ctest 27 of 27, the synthetic suite, both v1 contracts and every hidden app run pass. The eleven differential gates pass. The equivalence sweep against main's binaries (base `e01aaaa`) compares 1,933,523 updates, 1,047 restarts and 2,052 pictures with no difference. The per-track recompare is identical. Every earlier front-end comparison and every record check is unchanged (HUNTER's code routes keep R-0064's older `tries` difference). The tooling tests pass (499). No function is over 80 lines; the address index passes. The gates' own fifth-win step did not run (a variable clash in the script, since fixed); it ran in the checks below. |
| Corrections | The checks on `e55931e` (`checks-e55931e/checks.out`): the presets and ctest 27 of 27; fifth-win no difference on 4,055 frames and 2,511 of 2,511 pictures, the records equal at the six frames; cont-win, cont-loss and forced-bronze unchanged; the tooling tests; function size and the address index. |

## Review

Tier 2, a fresh Opus 5.5 subagent in an isolated worktree, of `fe26824`: approved with no must-fix items (`review-fe26824.md`). It checked the worker's write ordering on the core (saves and resumes around a write), the runner against the worker, and the 8-bit sum against the listing. Its should-fixes are made in `e55931e`: R-0065 now says a lap race uses the one-run race's win test and lists exactly the scoring addresses the capture ran; `--record-write` refuses a signed, oversized or trailing-text value; a test covers the sum's 8-bit wrap; a manifest or script refuses two writes to one byte after one frame; the worker checks the write offsets when the core loads. Tier 2 stands, as the reviewer agreed: the new completion rule agrees with the old for every byte the game writes.

## Handoff

- Exact next experiment/command: after the review and the merge, the main menu's other modes
  (COVERAGE-ROADMAP: 2P, VS, LEAGUE, OPTIONS, the demo).
