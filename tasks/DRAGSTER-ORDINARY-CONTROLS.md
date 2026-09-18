# DRAGSTER-ORDINARY-CONTROLS - Playable DRAGSTER controls

## Assignment

- Status: reviewed and integrated (implementation approved at `16b8daf`, review `f427098`); acceptance conditional on final-tip CI, remote verification and the user's live playtest. Closeout: ignored `artifacts/dragster-ordinary-integration/closeout.json`
- Milestone: follow-up to accepted DRAGSTER gameplay (M2-M3, M4-01); not an M4 milestone gate
- Coordinator: Claude Opus 5 primary session (Claude Code desktop)
- Task provider: Anthropic (Claude Opus 5), per D-0004
- Worker/session/runtime/model: primary and subagent workers, all Claude Opus 5
- Provider quota: plan telemetry is available through the app usage tool. Session start 2026-09-17T14:57Z: 5-hour window 9% (resets 16:20Z), weekly 6% (resets 2026-09-24T08:00Z), extra usage disabled. **User budget for this overnight run: stop at 100% of the 5-hour window or 50% of the weekly limit, whichever comes first.** No reset, purchase or provider change.
- Reviewer: fresh Claude Opus 5 subagent in an isolated checkout at the exact candidate
- Dependencies: M4-16 (recovered ZOOM ZOO engine), DRAGSTER-PALETTE-CYCLE; both on main at `abc38e6`
- Base commit: `abc38e6`
- Branch and isolated worktree: `task/dragster-ordinary-controls`, `.worktrees/dragster-ordinary-controls`
- Owned paths: DRAGSTER native simulation and its frontend dispatch, DRAGSTER tests/manifests/research, this record, README/STATE DRAGSTER wording
- Checkpoint location: this record

## Outcome and boundaries

Native DRAGSTER must not abort on ordinary controls. Recover, from original
evidence, what the original does with jump while riding, brake, Left and
reversal, and the tricks and rewards they reach, so a player can race DRAGSTER
with keyboard or gamepad like ZOOM ZOO. Accepted DRAGSTER differential,
restore, result and presentation gates must keep passing unchanged. Out of
scope: DRAGSTER's 10:00 limit and window timing/shape (separate follow-ups,
unless the recovery naturally covers them), audio, menus.

## Inputs and prerequisites

Supported PAL ROM and audited bsnes core (identities in docs/STATE.md); the
two-track pack v7 (`b75539a0...`); accepted DRAGSTER replay manifests under
`tests/manifests/replay/` and native cases under `tests/manifests/native/`.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Current behaviour recorded | `artifacts/dragster-ordinary-controls/controls-probe.sh` | aborts listed below | probe summary (done) |
| Original evidence for each recovered control | original-only DRAGSTER timelines, captured twice with identical digests | frozen before native tuning | freezes and research note |
| Native matches the original on those timelines | differential comparison over the declared state projection, with fresh-process restores | every declared byte matches | reports |
| Accepted DRAGSTER gates unchanged | historical matrix (compare, restore, finish, opponent-first, presentation) | all pass with unchanged expectations | gate logs |
| No abort on ordinary controls | the probe, plus random and ordinary input sequences over complete races | no fail-closed abort; any remaining guard documented and unreachable in ordinary play | probe and fuzz logs |
| Live play | user plays DRAGSTER with gamepad and keyboard through result and restart | no abort | parked until the user returns |
| Independent review, CI | fresh reviewer; hosted CI on exact tip | approve; green | review, closeout |

## Evidence and attempts

| Attempt | Hypothesis | Experiment | Observation | Next decision |
| --- | --- | --- | --- | --- |
| 1 | Ordinary controls abort native DRAGSTER | User live playtest with an Xbox controller, then the probe holding each button with Right for 2,200 updates on `abc38e6` | User run: "moving brake is outside the recovered movement domain" after 1.3 s. Probe: Right alone, and Right with Up, Down, Y, A, X, L or R, reach the result (phase 3, player wins); Right+B aborts "unrecovered landing velocity transform"; Left aborts "leftward movement is outside the recovered primary domain"; no input stays at the start | Investigate the source mapping and the shared ZOOM ZOO engine |
| 2 (2026-09-17T15:00-15:27Z) | The original race engine is shared, so DRAGSTER's timelines are reproduced further by `update_zoom_zoo` with DRAGSTER content than by extending `update_movement` | One exploratory original (`explore/mixed1`: B jump, L rotation, Y brake, Left reversal, B+X, player loss), measured with (a) `update_movement` from its end-1533 seed over the 333-byte projection and (b) `update_zoom_zoo` from native initialization over the 742-byte projection | DRAGSTER initializes at end-1328 (48 frames before ZOOM ZOO). (a) matches 166 updates, aborts at 1700 (first B while moving). (b) matches 413 updates, first divergence 1741 `progress_adjustment` 71/72 (the `$1281` bound). With the bound (race mode `$77:074B`), mode-0 result publication and track geometry (`$81:A304-A51B`), (b) matches all 2,773 updates to 4100 | Choose (b); keep `update_movement` for the historical gates (`af72177`) |
| 3 (15:27-15:38Z) | The shared engine holds on untuned ordinary timelines | Five cases captured twice and frozen before native evaluation (`abab7be`): primary (win), reversal (loss, pause, wrong-way), random-1..3 (random-3 withheld) | primary, reversal, random-2 match every byte untuned; random-1 diverged at 2207 `input_high`: the case held Up and Down together and bsnes's gamepad reports neither | Drop opposing directions for DRAGSTER (`with_physical_dpad`, `7e8af40`); random-1 becomes a regression |
| 4 (15:38-15:46Z) | The app can play DRAGSTER on the shared path | App switched to the shared engine with the accepted renderer via a presentation adapter; launcher upgrades the DRAGSTER-only pack (`0a94a3b`) | Right+B plays to the stable winner result; gates on `0a94a3b`: four DRAGSTER cases pass (379/327/567/527 restores), M4-16 primary passes 757 restores, four-preset synthetic passes; the historical matrix failed only for environment reasons (symlinked `local/native`, fixtures outside the checkout) | Write an abort fuzz over complete races |
| 5 (15:46-16:23Z) | Ordinary random input over complete races never stops the app | `dragster_fuzz_runner` (app update, restart and render calls); each failing race captured in the original | Four stop classes, each settled against a fresh original: countdown also releases A and X (`$83:E7A2-E7BF`); a landing clears held rotations; result font lacked digits 1, 2, 4, 9 (verified on original result frames); a tie counts as won (tie picture matches the winner). The view also read before the BG1 map for riders behind x 894 | Fix, freeze the two fuzz races as regressions (`6ffd9ac`) |
| 6 (16:23-16:31Z) | The first fixes close the fuzz | 3,000-seed fuzz on `6ffd9ac` (stopped at 1,118 seeds after 8 identical stops); failing races captured | Seed 383: the original keeps a released hold above its rotations across a bounce, so the M4-16 hold/rotation restore bound is wrong. Seed 208 showed a silent divergence at 1536: the charge latch (`$82:9995-99EF`) is unchanged at zero throttle | Remove the bound (elapsed bounds kept), follow the latch routine exactly (`3e79228`); GCC conversion fix after hosted CI failed on `6ffd9ac` (`33767fd`) |
| 7 (16:31-17:10Z) | Silent divergences remain that aborts cannot show | Differential fuzz: 150 seeded ordinary timelines (pauses without Up/Down, opposing directions, every button) captured in the original and compared natively; ZOOM ZOO sweep over every M4-16 original against the `abc38e6` runner | Three races held B, X and Right through a landing: the drive skips velocity during a roll bounce (`$042B`, `$82:A9B3/AA10`). Seed 53: a direction flip left roll step 0, which the original completes (`$82:959B`). `explore` also learned to report originals that leave the result screen, late finishes and a finished player's unpublished Start | Fixed in `f2b2675` and `70bebc9` |
| 8 (16:40-16:56Z) | The first batch's other divergences are shared-routine gaps too | Every divergent seed of batch 1 re-explored with the latest runner; access captures around seed 135 | Batch 1 (seeds 1-150 on `f2b2675`): 123 matched, 19 diverged, 6 generator overlaps, 2 retires. All 19 now match after: idle wobble velocity below reference 32 is applied unchanged (`$82:A1F2`, seeds 66, 82; `ad2c7ec`); moving brake goes through the drive routines, so a bounce keeps velocity (seed 140); a supported rider at its target skips the orientation store (`$83:F02D-F034`, seed 135; both `65696be`). ZOOM ZOO sweep of 26 M4-16 originals: all rows match on both `abc38e6` and `65696be` | Second batch (seeds 151-300) with the repository tool on `65696be`, then final gates |
| 9 (16:57-17:25Z) | No divergence remains on fresh ordinary timelines | `dragster_diff_fuzz.py` seeds 151-300 on `65696be` (three parallel ranges) | 150/150 match every compared row (610,658 rows): 136 complete races, 5 end in a pause-menu retire and 15 leave the result screen for the track menu (both outside the product, compared up to that point) | Final candidate gates, abort fuzz, probes |


Notes on attempt 1:
- **Native DRAGSTER feeds SNES B to both brake and jump.** `update_movement`
  passes B to both `update_horizontal`'s brake and `update_jump`. ZOOM ZOO's
  engine, from `$82:AAE2-AAFB` (R-0032), maps B to jump `$0331` and Y to brake
  `$0325`. So jump (keyboard Z, gamepad South, which is Xbox A) held while
  riding either aborts or reaches the unrecovered landing path. Y does nothing
  in native DRAGSTER, although it is the original's brake. That is a silent
  divergence, not an abort.
- **The accepted DRAGSTER scenarios never press B while moving.** They hold
  Right, sometimes release it, and hold Up, so no gate exercised this path.

Decisions (recorded without asking; the user is away):
- **Architecture: DRAGSTER on the shared race engine** (attempt 2 measurements;
  R-0038). `update_movement`, `classic_crawler_dragster_start` and the `URMV`
  formats stay for the accepted historical gates; the new state is `URDG0001`.
- **Opposing directions** are dropped for DRAGSTER as a SNES pad does. ZOOM
  ZOO's accepted input path is left as it is (a follow-up may adopt it).
- **Pack:** the DRAGSTER-only (v1) pack cannot carry the shared tables. The
  launcher uses a valid two-track pack beside it or extracts one with `--rom`;
  the app refuses the v1 pack before gameplay instead of aborting mid-race.
- **Tie** counts as won (player processed first; tie picture matches winner).
- **Presentation:** accepted DRAGSTER renderer through an adapter, approximate
  fade, authored pause menu, view held at the start area behind x 894, static
  result background; the original's exit from the result on other buttons is
  not emulated (Race Again on Start).
- **Restore bound:** the M4-16 hold/rotation bound is removed on original
  evidence; the `zoom_zoo_trial_tests` 7/6 case is now accepted.

Mistakes:
- Ran the first DRAGSTER gate while editing sources in the same worktree; the
  source-diff check would have voided it. Stopped it and moved all gates to a
  detached worktree (`.worktrees/dragster-ordinary-gates`).
- The first gate-worktree historical run failed on its own setup (symlinked
  `local/native` and fixtures outside the checkout escape the repository checks).
- Exploration policies released B once airborne, so jumps looked short.
- The first fuzz rendered every stable-result update (slow) and rarely finished
  races; it also only finds aborts, so two silent divergences (charge latch,
  roll bounce drive) needed the differential fuzz.
- Committed the fuzz runner without GCC conversion care; hosted Ubuntu CI failed
  on `6ffd9ac` (fixed in `33767fd`).
- Added `adjustment_limit` as a per-track constant before finding it follows
  race mode; replaced before any freeze.
- Printed a whole SRAM image while debugging (wasted context).

## Handoff

- **Candidate:** `task/dragster-ordinary-controls`, pushed. The gate candidate
  is `6534241` (later commits are records only). Hosted CI on `6534241` passed
  on macOS and Ubuntu GCC: run 35250987766.
- **Gates**, all passed on a clean detached worktree
  `.worktrees/dragster-ordinary-gates` at `6534241` with its own binaries;
  reports in that worktree under
  `artifacts/dragster-ordinary-controls/candidate-6534241/`:
  - four presets built, four synthetic suites passed;
  - DRAGSTER ordinary controls, every 742-byte row plus a second fresh run, a
    restart from the stable result and fresh-process restores: primary 379
    restores, reversal 327, random-1 567, random-2 527,
    regression-countdown-actions-tie 179, regression-landing-held-roll 181,
    random-3 493 (withheld until this run, untuned), primary again under the
    sanitizer;
  - ZOOM ZOO M4-16 final set: primary 757 restores (debug and sanitizer),
    idle late start 801 restores (debug and sanitizer), six complete cases,
    seven opponent reward probes, eight opponent trick probes;
  - M4-15 race matrix: 8 runs, no failure; DRAGSTER historical matrix
    (M4-12 compares, restores, finish, opponent-first, presentation): 20
    commands, no failure.
- **Fuzzing:** abort fuzz (`dragster_fuzz_runner`, the app's update, restart
  and render calls) 3,000 seeds on `65696be`: 36,682,702 updates, 8,999
  complete races, 94,740 pause restarts, 767,092 rendered pictures, 0 aborts
  (`artifacts/dragster-ordinary-controls/fuzz/fuzz-65696be-seeds-1-3000.log`).
  Differential fuzz (`dragster_diff_fuzz.py`): 150 seeds exposed the last
  divergences; 150 fresh seeds on `65696be` match every compared row (610,658
  rows), with 5 pause-menu retires and 15 result-screen exits reported as
  outside the product.
- **Probes:** `probe-6534241` (the task's 2,200-update probe) and
  `probe-2600-6534241`: every mask exits 0 with no abort; at 2,600 updates
  every riding mask reaches the stable result screen (phase 3, player wins),
  Right with Y holds the brake at the line, and Left or neutral does not
  finish. The launcher upgrades the DRAGSTER-only pack to the two-track pack
  without opening the ROM; the app refuses the DRAGSTER-only pack before
  gameplay with the extraction command.
- **ZOOM ZOO sweep:** all 26 M4-16 originals match every projected row on both
  `abc38e6` and the candidate (`zoom-zoo-sweep-pose.txt`).
- Usage (account-wide, app telemetry): 10% 5-hour / 6% weekly at 15:00Z; 23% /
  7% at 16:00Z; the 5-hour window reset at 16:20Z; 3% / 8% at 16:31Z; 12% /
  10% at 16:56Z, with the final gates and fuzzing after that.
- **Next:** independent review of `6534241` (or this head, which adds only
  records); live play by the user with keyboard and gamepad through result and
  Race Again, parked while the user is away; then integration. Not started:
  the DRAGSTER 10:00 time limit (no case reaches it), the original's result
  screen exit on other buttons, and ZOOM ZOO's physical D-pad (raised as a
  separate suggestion).

## Independent review and integration

Fresh Claude Opus 5 reviewer, isolated checkout at `16b8daf`, report
`tasks/DRAGSTER-ORDINARY-CONTROLS-review.md` at `f427098` (pushed).
**Verdict: approve, no blocking defect.** It reproduced rather than read:
wrote its own 65816 disassembler and confirmed every cited address and all
seven fuzz corrections; re-captured primary, reversal and the withheld
random-3 with digests identical to both implementer captures; designed its own
withheld race (countdown X+Left, four riding jumps, rolls, held L/R through a
landing, moving brake, a 190-update reversal) which native matched for all
3,473 updates; re-validated both tracked freezes against its own captures with
379 and 493 restores; ran the DRAGSTER historical matrix, four presets, the
M4-16 primary and idle gates, 400 fresh fuzz seeds (4.9M updates, 0 aborts) and
a 12-mask probe; and checked that the removed M4-16 hold/rotation bound is only
reachable by one frozen original's states.

| Finding | Severity | Disposition |
| --- | --- | --- |
| 1. The shared follow rule's camera floor moved 0 to 14 with no test below x 13,696 | Low | Fixed in `1d8cfeb`: the floor and the first moving frame are pinned. |
| 2. The tie relaxation in `build_result_map` also relaxes the legacy path | Low | Accepted: the reviewer verified it against the tie original (both finish 3226, totals 3358/3358) and the legacy gates pass. |
| 3. R-0038 said seed 383 diverged at 3473 where the code and test say 3472 | Low | Fixed in `1d8cfeb`: the frame was not re-derived, so the note records the discrepancy instead of asserting either. |
| 4. `zoom_zoo_runner` with a v1 pack gives the low-level entry-absent message, not the app's remedy | Low | Recorded, not fixed: the lab runner is not the product launch path. |
| 5. Declared presentation limits are real and correctly stated | Note | No action. |

Integration gates on the merge candidate `b452170` (clean tree, unchanged
throughout; `artifacts/dragster-ordinary-integration/`): four presets built with
ctest, both synthetic suites, all seven DRAGSTER originals compared (plus the
primary under the sanitizer), the M4-16 primary and idle-late-start gates, the
DRAGSTER historical matrix, hidden DRAGSTER and ZOOM ZOO launches, and the
controls probe with no abort. **My mistake:** the first run reported the two
DRAGSTER presentation checks as failures, exit 3. That was my harness passing an
unquoted fixture path containing a space, not a candidate defect; rerun from
inside the checkout they pass with the accepted counts unchanged
(36/697/279/445/653/962/961 and 1,073).

Still open: the user's live playtest, and their confirmation of the v1-pack
product change.

### Live playtest, keyboard (user witness)

18 September 2026, main `9411e59`, two-track v7 pack, report
`artifacts/live-dragster/keyboard.json` (ignored). The user played a full
DRAGSTER race with the keyboard and **confirmed using jump, brake while moving
and Left**, the inputs that aborted before this task.

```
Presentation frames: 3676; rider-pose fallback frames: 3542
Live input: mapped key down/up 156/67; nonzero updates 2441; simultaneous updates 1599
DRAGSTER result updates 0; restarts 2; stable results reached 1; restarts from result/pause 1/1
Gamepad input: ... gamepad-only nonzero updates 0
```

No abort over 3,677 updates; one stable result reached, then Race Again from
the result and a restart from the pause menu; input was keyboard-only. The
3,542 held-art frames are the declared DRAGSTER rider-art limitation, which the
presentation unification task below owns. The gamepad run and the user's
decision on the v1-pack refusal are still open.
