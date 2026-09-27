# STUNT-RESULT - a stunt event's result, records and way back to the menus

## Assignment

- Status: **in review**. Queued 27 September 2026 (UTC) by STUNT-EVENT-RACE; claimed 27 September
  2026 at 06:35Z by the Claude Code desktop session that ran STUNT-EVENT-RACE, on `e131469`. The
  implementation worker started on STUNT-EVENT-RACE's branch while it was in review; its commits
  were moved onto this claim.
- Coordinator: the claiming session is coordinator, primary and integrator
- Worker/session/runtime/model: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`)
- Provider quota (D-0004): at claim the 5-hour window was 14% used and the weekly window 71%. The
  user's allowance: continue until the weekly window reaches 80%.
- Reviewer: a fresh Anthropic subagent, isolated checkout.
- Branch and isolated worktree: `task/stunt-result` in `.worktrees/stunt-result`.
- Milestone: M4 (original game coverage)
- Tier: 2 (screens and their records), as the earlier result tasks.
- Dependencies: STUNT-EVENT-RACE (R-0066), the one-run and lap results (R-0057, R-0058), the
  fifth-win route (R-0065).

## Outcome and boundaries

After a stunt event's race, the menus' stunt result (`$80:F0EE-F2E9`, the decode's section 2): the
screen and its tally animation, the records written at the result (the rider's best, the score
history, checksums), the shared exit wait, the statistics and top three (`$80:C948`, `$80:8CCB`),
the win test (`$83:88E1`) with the done or loss flag, and PICK TRACK. Also the pause QUIT's stunt
words, the removal of the app's notice for a stunt event, and native's cold-start `tries`, which
holds 3 where the original holds 0 until the rider's choice (R-0064).

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Result | Front-end comparisons from each race's return to PICK TRACK on bowl-lose and hill-win | 0 differing pixels, equal menu state, OAM buffer and text map | logs |
| Records | `sram.py` at the result, the tally's end, the exit and the end | Equal to the original's cartridge RAM | logs |
| Completion | A tour completed by a stunt win (R-0065's route) | The award frame for frame | logs |
| Nothing moves | The gates of STUNT-EVENT-RACE | Unchanged | logs |

## Result

[R-0067](../docs/research/R-0067-stunt-result.md). After a stunt event the menus show its result as
the original does: the table of the player's tricks counted up column by column (a press speeds
the count), the rider's score, best and the qualifying score; then the statistics, the track's top
three and the win test, which marks the track done or counts a loss, and PICK TRACK, or the award
when the win completes the tour. The pause QUIT in a stunt event scores 0. Native's cold start now
holds no tries until a rider is chosen, closing R-0064's older difference. The app plays stunt
events from the menus; `--track` still refuses them, since its result is the Classic result screen.
Native keeps neither the score history nor the checksums, which nothing in one-player play reads.
Pack profile v25. A research worker decoded it; an implementation worker wrote it and made the
corrections; the primary integrated.

## Evidence

`local/evidence/stunt-result/` (NOTES.md; the captures bowl-lose, hill-win, bowl-quit,
hill-complete, bowl-press; `acceptance.sh`, `nothing-moves.sh`, `decode/compare.py` and `sram.py`).

| Criterion | Result |
| --- | --- |
| Result | From power-on through each stunt result to PICK TRACK: bowl-lose (a loss) 2,565 frames, hill-win (a win) 2,698, bowl-quit (the pause QUIT) 2,689, bowl-press (presses during the tally) 2,565: no state difference; every picture from frame 400 equal (2,165, 2,298, 2,289 and 2,165). |
| Records | Equal to the original's cartridge RAM at 10 or 11 frames of each capture (the cold start's tries, the result, the tally's end, the best, the statistics, the scoring, the end). |
| Completion | hill-complete (HILL CLIMB won with WALKER's other four done tracks written during the race): 4,298 frames through the award and PICK TOUR, no difference, 3,898 pictures equal. |
| Nothing moves | The gates on `16d2789` (`local/evidence/stunt-result/gates-16d2789.out`, 06:31-08:41Z, 130 minutes): the three presets, ctest 28 of 28, the synthetic suite, both v1 contracts, every hidden app run (and two new ones playing BOWL and HILL CLIMB from the menus with no notice) and the eleven differential gates pass. The equivalence sweep against main `e131469`'s binaries (pack v24 against v25), now with the nine stunt scenarios on both sides: 432 runs, 2,387,105 updates, 1,290 restarts and 2,538 pictures, no difference. The per-track recompare is identical, stunt rows included. Every earlier front-end comparison and records check is unchanged, except HUNTER's code routes, whose `tries` now match. The five captures above; the tooling tests (504); no function over 80 lines; the address index passes. |

## Review

Tier 2, a fresh Opus 5.5 subagent in an isolated worktree, of `5c8389d`: request changes, for one must-fix (`review-5c8389d.md`): a test held the ROM's qualifying scores. The corrections: synthetic qualifying scores and tally cells; R-0067 marking its listing-only rules; a fifth capture, bowl-press, with presses during the tally; one shared press test; R-0064's pointer to the closed `tries` difference; the score history and checksums' omission explained. A focused re-review of `b38a8c7` approved with two should-fixes (`review-b38a8c7.md`), made in the last commit: a ROM-free test of the statistics' tie (ctest fails when a tie is made a loss) and a synthetic total stream in the tests.

## Handoff

- Exact next experiment/command: after the merge, [STUNT-HUD](STUNT-HUD.md), then the main menu's
  other modes (COVERAGE-ROADMAP).
