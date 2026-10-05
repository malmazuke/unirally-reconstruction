# SPLIT-PAUSE-MENU - the pause in two-pad races

## Assignment

- Status: **claimed** 5 October 2026 by the session that closed CLASSIC-PAUSE-MENU, on main
  `98326d1` after that task's merge (main equal to `origin/main`, closeout written). Prepared the
  same day from R-0078's "Not covered".
- Milestone: M4 breadth
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`); one
  session is coordinator, primary and integrator
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  **tier 1** (D-0008): who may pause, and whose total a quit writes, are race state and ordering.
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): at claim (5 October 2026 about 11:20 Sydney) the weekly window was 36% and
  the five-hour window 28%; standing rule: continue until weekly 80%.
- Reviewer: the full D-0006 process for tier 1.
- Dependencies and evidence of acceptance: CLASSIC-PAUSE-MENU (R-0078), RACE-PAUSE-EXITS (R-0060),
  TWO-PLAYER-VS, LEAGUE (R-0073).
- Base commit: `98326d1` (main at claim).
- Branch and isolated worktree: `task/split-pause-menu` in `.worktrees/split-pause-menu`.
- Owned paths and shared interfaces: `run_pause_menu` (`src/core/race_update.cpp`), the pause
  state if the pauser must be kept, `update_race_for_menus`, the pause picture in
  `src/core/presentation.cpp` and `src/core/race_hud.*`, native tests, a research record, this
  record, `docs/STATE.md`, `tasks/README.md`, `tasks/NEXT_SESSION.md`.
- Claim/lease/heartbeat/checkpoint location: this record.

## Outcome and boundaries

In a two-pad race (2P, VS, a league pair) the original reads pad 2's Start as well as pad 1's
(`$83:CD05`; port 2 is a pad there, R-0060), puts the menu in the half of the pad that paused
(`$83:F6B9-F6F3`: VRAM 0x18A8/0x18ED for pad 1, 0x1A68/0x1AAD for pad 2), and, in the menu modes
whose `$77:10AD` has bit 2, shows one of eight pause messages by the pauser's rider instead once
the countdown is over and nobody has finished (`$83:F6FD-F791`, text at `$83:F516`). A quit writes
the pauser's total (`$83:F8DA-F912`). Native lets only a league pair's second pad pause and draws
M4-16's authored panel over every split race.

Recover and implement the pause in two-pad races from the menus: who may pause, the menu's
half, the messages, the picture (dimming included) and the quit's totals, against captures of
each mode. One-player play and its pictures (R-0078) do not change.

## Inputs and prerequisites

The two-pad races' manifests under `local/evidence/two-player-vs/` and `local/evidence/league/`,
the static listing for `$83:CD05-CD4B` and `$83:F63E-F979`, R-0078's `pictures.py` (the runner
already writes split race pictures).

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Behaviour | Captures of 2P, VS and a league pair paused by each pad, quitting and continuing | Native state equal frame for frame, both pads | JSON |
| Picture | Native split pause frames against the captures | Pixel-exact with the menu or message shown | JSON |
| Nothing else moves | Gates, race and front-end sweeps, R-0078's captures | Unchanged outside two-pad pauses | gate logs |
| Review | Tier 1 | Approved | review on the pull request |

## Evidence and attempts

All times 5 October 2026, Sydney. Captures and scripts in main `local/evidence/split-pause-menu/`;
[R-0079](../docs/research/R-0079-two-pad-pause.md) holds the observations and numbers.

| Attempt | Hypothesis | Experiment | Observation | Next decision |
| --- | --- | --- | --- | --- |
| 1 (11:25-11:35) | 2P and VS pause from either pad; league messages are the only difference | Captures `twop`, `vs` (pads 1 and 2 in and after the countdown), the menu routine's sites watched | Both pads open the menu; pad 2's in the lower view (`$83:F6D9`); no messages in 2P or VS; my Down + Start after the countdown was pad 1's QUIT | Native: pad 2's Start, release, view, quit |
| 2 (11:35-11:45) | Pad 2 in every two-pad race, both Starts for the release, the confirming pad's total | `run_pause_menu`, the release, `update_race_for_menus`; `split_compare.py` (pause words, countdown, camera, laps, finish flags, checkpoint counts) | Main diverges from `twop`'s 2200; the candidate matches every compared word on `twop` and `vs` | Capture pad 2's quit and restart |
| 3 (11:45-11:55) | Pad 2's totals and the menus' return | `twop2` (pad 2's quit), `vs2` (pad 2's countdown restart) | State equal; `vs2` went to a result with MARTIN `::00.02` where the original restarts: `$80:88DD` tests both totals, native only a league's | Test both totals in 2P and VS |
| 4 (11:55-12:00) | The split picture is R-0078's in the pauser's view | Menu cells and words 14 rows down for the lower view, its ink, under the riders, split HUD kept out of the cells; restore checks with pad 2's menu open | Paused pictures equal except where a caption, the lower arrow or the lower colour math (existing gaps) shows; `twop-plain`: the candidate's 530 pictures byte-identical to main's | League pairs |
| 5 (12:00-12:10) | League pairs pause the same way and show messages after the countdown | `league` (my countdown pauses froze the countdown, so the first post-countdown pause was a restart: kept as `league-restart`, recaptured later); pack v35 with `$83:F516`; `draw_classic_pause_message` | State equal on both; message cells exact for pad 1 (0 of 81,920 pixels) and both pads; pad 2's only the lower colour math (960 pixels) | Records, gates, review |

## Handoff

- Head: the records commit after `6302d37` on `task/split-pause-menu` (not yet pushed).
- Unresolved, queued: the split race's tutorial captions, lower arrow and lower colour math
  (pre-existing, R-0071), the split CONTINUE clear, and NOW PLAYING after a two-human restart
  (R-0079's "Not covered").
- Next: gates (`local/evidence/split-pause-menu/gates.sh` on a detached checkout), the tier-1
  review, the pull request.

