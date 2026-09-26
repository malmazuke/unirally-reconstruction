# RACE-RIDERS-OPPONENTS - the one-player race's other riders and opponents

## Assignment

- Status: **in review**. Queued and claimed 26 September 2026 at 06:50Z by the Claude Code
  desktop session that ran RACE-PAUSE-EXITS, on `3f2c2f7`.
- Milestone: M4 (original game coverage)
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`)
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  **tier 1**: it changes the race's state and scenarios.
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): at claim the 5-hour window was 4% used and the weekly window 45%. The
  user's allowance: continue until the weekly window reaches 80%.
- Reviewer (primary automatically spawns fresh model/effort, isolated checkout; no user
  trigger): a fresh Anthropic subagent.
- Dependencies and evidence of acceptance: the race scenarios (R-0046, R-0050, R-0052), the
  one-player screens (R-0055, R-0056), the menus' side of a race (R-0057 to R-0060).
- Base commit: the `main` tip at claim.
- Branch and isolated worktree: `task/race-riders-opponents` in `.worktrees/race-riders-opponents`.
- Owned paths and shared interfaces: the race's setup and scenarios, the race's content and
  presentation where a rider or opponent changes them, the app's and the runner's race start,
  pack rules and content, native tests, a research record, this record, `docs/STATE.md`,
  `tasks/README.md`, `tasks/NEXT_SESSION.md`.
- Claim/lease/heartbeat/checkpoint location: this record.
- Session time limit, concurrency allocation and actual spend authorization if relevant: one
  worker plus the review subagent; no monetary spend.

## Outcome and boundaries

The race scenarios are MIKE's against BRONSEN only, so the app shows a notice for any other
rider chosen on PICK YOUR UNI, and for any other opponent: SILVIA (0x12) and GOLDWYN (0x13) after
a medal is won, ANTI-UNI (0x14) on HUNTER. Find what the race takes from the rider (`$77:0748`)
and the opponent (`$77:0749`), such as their sprites, palettes, names and the AI's level, and make
native race every pairing the one-player screens can choose, frame for frame.

Out of scope: the stunt events (STUNT-EVENTS); the second human (SPLIT-SCREEN-RACE).

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Race state | Captures of the original racing other riders and each opponent, against native (the race engine's differential tooling) | Equal race state every update, or each residue explained | logs, research record |
| Pictures | The race's pictures for those captures | Equal to the pixel, or explained | pictures |
| Playable | The app from power-on with another rider and against each opponent | The race runs and returns to the menus | report |
| Nothing moves | ctest, the synthetic suite, the v1 contracts, hidden runs, the differential gates, the equivalence sweep, the front end's comparisons | Unchanged for MIKE against BRONSEN | logs |

## Result

[R-0061](../docs/research/R-0061-riders-and-opponents.md). The rider changes no physics: it
chooses its voices, its tutorial hints (the records' `$77:1116`), its sprite palette and the ink's
colour math (`$82:D4DC`). The opponent sets the AI's tier (`$1275`, `$1283` from `$83:C8B3`,
`$1281`), including SILVIA's level-2 launch rule (`$83:E17D`), which native did not have, and its
voices and palette. Native races every pairing the one-player menus choose, in the app, the
front-end runner and the race runners. Pack profile v21 adds the two tables. While comparing
ANDREW's pictures, the caption's blank turned out to reach the screen a picture later than native
drew it, for every rider; that is fixed and recorded as a correction to R-0042.

Tier 1 stands: the race's state changes by opponent.

The differential gates' restart check changed with the rule it checks. A restart starts without
hints when they had ended, as the original does (R-0061: the M4-16 primary's frame 2742, DRAGSTER's
primary row 474), so `zoom_zoo_playable` and `dragster_playable` compare it with a fresh race
started with the last state's hints, not with the fresh race itself. Deserialization now refuses
a `$1277` the AI level cannot leave.

## Review

Round 1 (tier 1, fresh Opus 5.5 subagent, isolated worktree) of `46ea255`: changes requested,
`local/evidence/race-riders-opponents/review-46ea255.md`. Answered:
- M1, the gates' restart check: the expectation above.
- M2, the counter-7 skip untested: the track 25 capture, found by a native search.
- S1: the caption rule marked measured, the upload order named, and queued.
- S2: the equivalence sweep's picture differences are listed with the gates.
- S3: the `$1277` refusal. S4: tests of the restart's hints and of reading paired states back.
- S5: `$7E212C`'s writer and the subscreen named in R-0061. The nits: the ink out of
  `render_classic_race`, the test comment, the asset name.

## Evidence

Captures and scripts in `local/evidence/race-riders-opponents/`: `decode/` (the listing and the
first captures, work RAM `$0000-$1FFF`), `race/` (`track_reference` captures with full memory,
each taken twice and identical; `explore-*.json`; `pictures.py` and its logs) and `front-end/`
(`compare.py`, `sram.py`, the captures and the app's hidden runs in `app/`).

| Criterion | Result |
| --- | --- |
| Race state | Seven captures: ANDREW, SILVIA, GOLDWYN and a hints-off MIKE on DRAGSTER (2,473 updates each), SILVIA and ANDREW v GOLDWYN on ZOOM ZOO (4,825 each), and SILVIA on track 25 (2,307), which reaches the level-2 skip on a counter ending in 7. Every update is equal from the boundary; a build without that skip diverges at its frame. |
| Pictures | 307 frames of those races. ANDREW's and the hints-off race's are equal to the pixel. SILVIA's and GOLDWYN's differ only by the off-screen rider arrow, a declared omission since M4-16 (R-0043), except two picture residues on track 25 (a caption a picture late, MIKE's upper body two pictures early). All three are queued as [RACE-OFFSCREEN-ARROW](RACE-OFFSCREEN-ARROW.md). |
| The menus around the races | `andrew` (ANDREW's race), `forced-silver` (SILVIA's) and `goldwyn` (BRONSEN, SILVIA and GOLDWYN in turn), from power-on: no difference in the menus' words, OAM buffer or text map on any frame, and 1,054, 582 and 6,321 pictures all equal. `sram.py` (now with `$77:1116`): the kept words equal at `andrew` 3690 and `goldwyn` 5600, 10500 and 13390. |
| Playable | The app, hidden, with `andrew`'s pads: ANDREW's race runs and returns to the menus. With `goldwyn`'s: three races (BRONSEN, SILVIA, GOLDWYN) run and return, with the original's totals and no notice. |
| Nothing moves | The gates on `dbb16fc` (`local/evidence/race-riders-opponents/gates-dbb16fc.out`, 08:14-09:52Z, 98 minutes). The three presets build, ctest 27 of 27, the synthetic suite, both v1 contracts and every hidden app run pass. The eleven differential gates pass, with 179 to 801 restores each. The per-track recompare of both sweeps is identical on 20 and 25 tracks. Every earlier front-end comparison (main menu, the 1P screens, the one-run and lap results, the tour's completion, the pause exits) has no differences and all its pictures equal. The records match at 24 frames. No function is over 80 lines, and the address index passes. The equivalence sweep against main's binaries (base `3f2c2f7`, pack v20) compares 1,933,523 updates, 1,032 restarts and 2,022 pictures, and finds 7 differences, all intended. Five are the first row of a race restarted by a random schedule's pause after its hints ended (tracks 8, 10, 14, 39 and 41): only byte 624, `hints_active`, differs, 0 where main has 1. Two are pictures one frame after the player's queue ran dry (track 20 `right-jumps` frame 5360, track 10 `buttons-4` frame 5417): the last caption is still shown, in the caption band only. |

Round 2 of `dbb16fc`: approved, provided the gates pass (they do, above);
`review-dbb16fc.md`. Its remaining points are answered:
- `race/silvia-runner-25` is repeated, byte for byte (`race/repeat-logs`).
- The consecutive frames 1836-1864 and BRONSEN's twin frames 1849-1851 are kept beside it
  (`race/silvia-runner-25-frames-1836-1864`, `race/bronsen-runner-25-frames-1849-1851` and their
  pictures).
- The search is kept in `race/phase7-search/`.

## Handoff

- Exact next experiment/command: after the review and the merge,
  [FRONT-END-ENDINGS](FRONT-END-ENDINGS.md) or [RACE-OFFSCREEN-ARROW](RACE-OFFSCREEN-ARROW.md).
