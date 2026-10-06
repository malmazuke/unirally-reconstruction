# TWO-HUMAN-RESTART - the menus after a two-human race's restart

## Assignment

- Status: **claimed** 6 October 2026 by the session that closed SPLIT-RIDERS-UNDER-INK, on main
  `ad74dbf` after that task's merge (main equal to `origin/main`, closeout written). Prepared
  5 October 2026 by the SPLIT-PAUSE-MENU session, from R-0079's "Not covered".
- Milestone: M4 breadth
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider: Anthropic
- Worker/session/runtime/model: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`); one
  session as coordinator, primary and integrator
- Actual model/reasoning effort, routing rationale: **tier 1** if the menus' flow or saved records
  change (D-0008), tier 2 if only NOW PLAYING's picture does; decide from the first capture.
- Provider quota window (D-0004): at claim weekly 58%, five-hour 11%; standing rule: continue
  until weekly 80%.
- Dependencies: SPLIT-PAUSE-MENU (R-0079), RACE-PAUSE-EXITS (R-0060), TWO-PLAYER-VS (R-0071).
- Base commit: main `ad74dbf`.
- Branch and isolated worktree: `task/two-human-restart` in `.worktrees/two-human-restart`.
- Owned paths: `src/core/race_result.cpp` (the restart's return), `src/core/now_playing.cpp`,
  the front end's local modes, native tests, a research record, this record, `docs/STATE.md`,
  `tasks/README.md`, `tasks/NEXT_SESSION.md`.

## Outcome and boundaries

QUIT during a two-human race's countdown restarts it. The original returns through `$80:88DD`
and re-enters NOW PLAYING at `$80:BC36-BC45`, inside the one-player handler: its NOW PLAYING then
shows no win counts (R-0079: `vs2` from 2289, up to 1,183 pixels), and what the menus do after
that restarted race is not captured. Native returns to NOW PLAYING with the counts and continues
in the race's own mode. Capture a 2P, a VS and a league restart through the restarted race's
result and continuation, recover the flow, and make native follow it.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Flow and records | Captures through the restarted race's result and the menus after it | Native front-end words, cartridge RAM and pictures equal | JSON |
| Nothing else moves | Gates, sweeps | Unchanged outside the restart | gate logs |
| Review | By tier | Approved | review on the pull request |

## Evidence and attempts

Captures and tools are in main `local/evidence/two-human-restart/`. Base binaries are main
`ad74dbf`'s (`base-ad74dbf/`). A fresh listing reader's report is `listing-report.md`.

| Attempt | Hypothesis | Experiment | Observation | Next decision |
| --- | --- | --- | --- | --- |
| 1 | The restart re-enters the one-player handler (R-0079) | `vs-restart`, `twop-restart` (8,400 frames each) and the listings | Each handler loops to its own NOW PLAYING (`$80:BFE0` VS, `$80:BD5D` 2P); mode, pairing and VS flags unchanged. R-0079 was wrong. | Account for what still differs |
| 2 | VS's NOW PLAYING counts differ because of the restart | `$80:B297-B2B9`; the first VS visit | Only 2P (`$77:10AD` = 2) prints the counts, from `$77:10A9`/`$10AB`; VS never does, restarted or not. 2P's second choice zeroes them. | Print in 2P only, from the session's wins |
| 3 | The restarted race's finish pictures differ because of the race counter | `vs-control` (7,800 frames, no restart); `$7E:11E9`/`$11EB` | The kinds are 4/3 after the restart and 2/1 in the control: `(counter & ~1)` + 1 or 2, and the aborted race counted. | The counter as race state, tier 1 |
| 4 | Native carries the counter | `race_counter` in the scenario and race state, one serialized byte, the reader's guards | 4530-4743 now equal in both captures; restored states equal | The league restart |
| 5 | A league restart behaves the same | `league-restart` (11,800 frames; R-0079's presses with Right released before NOW PLAYING) | Race words and look equal on 2,506 race frames. The NOW PLAYING animation, the league result's icons and one transition differ, the same on main. | Gates, review |

The pictures still differing, all equal on main's binary too: NOW PLAYING's arrows and icons
(R-0071's residual), the result icons, the result → VS CHAMPIONS, → PICK CHALLENGER and 2P
→ continuation slides (in the control as well), the league result's icons.
