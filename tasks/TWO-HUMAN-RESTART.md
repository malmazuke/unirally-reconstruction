# TWO-HUMAN-RESTART - the menus after a two-human race's restart

## Assignment

- Status: **accepted** 6 October 2026 (tier 1, [PR #60](https://github.com/malmazuke/unirally-reconstruction/pull/60));
  claimed 6 October 2026 by the session that closed SPLIT-RIDERS-UNDER-INK, on main
  `ad74dbf` after that task's merge (main equal to `origin/main`, closeout written). Prepared
  5 October 2026 by the SPLIT-PAUSE-MENU session, from R-0079's "Not covered".
- Milestone: M4 breadth
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider: Anthropic
- Worker/session/runtime/model: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`); one
  session as coordinator, primary and integrator
- Actual model/reasoning effort, routing rationale: **tier 1** (D-0008). The first captures showed
  the finish poses depend on the race counter `$77:10B1`, which became race state and a byte in
  two-view states and league wrappers.
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

## Gates

`local/evidence/two-human-restart/gates.sh` runs against main `ad74dbf`'s binaries. The final
head's run is `gates-551a777.out`.
- **68c3256** failed all eleven frozen gates: `restart_zoom_zoo` counted the race counter, so a
  restart no longer equalled a fresh race (found by the reviewer). 406223f keeps the counter in a
  restart on its own, and 551a777 regenerates the address index.
- **On 551a777:**
  - presets and ctest: 41 of 41 on each of the four;
  - synthetic suite, v1 contracts and hidden runs: pass;
  - the eleven frozen gates: pass;
  - race sweep: 432 runs, 0 differences;
  - every split capture's words, look and restores, and the four restart captures': 0
    differences;
  - cues identical to main; tooling OK; no function over 80 lines; the address index is current;
  - fuzz: 40 aborts, as on main.
- **Front-end sweep:** 121 of 179 manifests equal. The others differ in three ways:
  - the counter byte in two-view, league and demo state rows;
  - later league races' finish poses, which now match the original's work RAM (R-0084);
  - `goldwyn`'s fourth-race finish picture.

  Against the originals' frames 6 pictures are better and none worse (`fe-pictures-551a777.txt`,
  `rows-551a777.txt`).

## Review and integration

- Reviewer: a fresh Claude Opus 5.5 subagent in its own clone, tier 1, with preregistered cases.
- On 68c3256 it found that a restart on its own counted the counter, which failed every frozen
  gate's restart check. 406223f fixed that, and 551a777 fixed the stale address index it reported.
- Approved `551a777`. The withheld captures:
  - one-player, two DRAGSTER races: the second takes kinds 4/3 in the original; 988 of 988
    pictures are equal (main differs on 80);
  - 2P SAME TRACK with pad 1 QUIT in the second race's countdown: words equal on 4,425 race
    frames. The pictures differ from main only at the 47 finish-pose pictures, which now match
    the original, and NOW PLAYING's text is exact.
  - Six restores with counters 2 and 3: equal. Every layout's counter byte is where claimed.
- Findings, none blocking:
  - R-0084's Evidence was a placeholder; it is now filled in.
  - Four limits were missing from "Not covered" and are now recorded: tables 5/6 are listing-only;
    the attract demo keeps counter 1; one-player poses are not checked against the caller's
    counter; SAME TRACK starts one frame earlier in the original.
  - Tests for the loser kind, the pair refusal and the league byte: added to
    `dragster_race_tests` after the approval. The reviewer confirmed this tests-only delta.
- Integration: merged by merge commit after the final head's checks; closeout in main
  `artifacts/two-human-restart-integration/closeout.json`.
