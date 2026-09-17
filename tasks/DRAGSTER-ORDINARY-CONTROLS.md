# DRAGSTER-ORDINARY-CONTROLS - Playable DRAGSTER controls

## Assignment

- Status: in_progress
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
| 7 (16:31-17:10Z) | Silent divergences remain that aborts cannot show | Differential fuzz: 150 seeded ordinary timelines (pauses without Up/Down, opposing directions, every button) captured in the original and compared natively; ZOOM ZOO sweep over every M4-16 original against the `abc38e6` runner | Three races held B, X and Right through a landing: the drive skips velocity during a roll bounce (`$042B`, `$82:A9B3/AA10`). Seed 53: a direction flip left roll step 0, which the original completes (`$82:959B`). `explore` also learned to report originals that leave the result screen, late finishes and a finished player's unpublished Start | Fixed in `f2b2675` and `70bebc9`; final candidate gates |


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

- Branch head and gates: see the final entry below.
- Usage (account-wide, app telemetry): 10% 5-hour / 6% weekly at 15:00Z; 23% /
  7% at 16:00Z; the 5-hour window reset at 16:20Z; 3% / 8% at 16:31Z.
- Next: independent review of the final candidate; live play by the user with
  keyboard and gamepad through result and Race Again (parked); then integration.
  Not started: DRAGSTER 10:00 limit capture, ZOOM ZOO physical D-pad.
