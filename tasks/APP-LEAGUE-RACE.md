# APP-LEAGUE-RACE - the app's league races, and the podium's audio in the app

## Assignment

- Status: **in progress**. Claimed 6 November 2026 22:21 UTC on main `6797457b`, on the user's
  instruction ("Next task"; Claude Code desktop app, Claude Opus 5.5 `claude-opus-5-5`;
  coordinator and integrator; a worker subagent implements, a fresh subagent reviews). Queued
  29 October 2026 by [CARTRIDGE-OPTION-BITS](CARTRIDGE-OPTION-BITS.md) (R-0105, gates round 1).
  Run under [D-0013](../docs/decisions/D-0013-task-time-and-disk-budget.md): claim to merge against
  the 6-hour target, new evidence under 1 GB. Usage at the claim: 5-hour 0%, weekly 0%.
- Branch `task/app-league-race` in `.worktrees/app-league-race`; evidence in main
  `local/evidence/app-league-race/`.
- Task provider: Anthropic.
- Milestone: M4 (original game coverage).
- Review tier: **1** (the app's race path).
- Dependencies: PODIUM-AUDIO (R-0099), LEAGUE (R-0073), CARTRIDGE-OPTION-BITS (R-0105).

## Why

In a league race the app and `front_end_runner` part ways on the same save and inputs:
- PODIUM-AUDIO (R-0099) found LEAGUE's `organic-full-tour-turnaround` second race diverging at
  frame 12,619 in the app and never ending.
- CARTRIDGE-OPTION-BITS' gates found the app logging pause cues from frame 3180 of the league race
  on `podium-audio/app_run.sh`'s route (`local/evidence/cartridge-option-bits/gates-round-1/`),
  on main too.

`front_end_runner` matches the original there. Since R-0105 the original's podium cannot be
reached by a warm boot (the league slots clear the owed podium), so the app's only podium check,
`app-podium-audio`, now stops before the race and covers no podium.

## Outcome

- The app's league race equals `front_end_runner`'s (and so the original's) on both routes.
- `app-podium-audio` covers a podium again, reached by racing a league's last event in the app.

## Checkpoint (worker, 6-7 November 2026)

[R-0106](../docs/research/R-0106-app-league-race.md) holds the detail.

- **Cause (one, for both routes).** `sdl_main.cpp` started a scripted two-human race's clock at
  the race's initialization frame when NOW PLAYING chose it, then advanced it on every update,
  the loading frames the app waits with native audio included. The race's n-th update read the
  script's row for the initialization frame plus the loading plus n; `front_end_runner` reads the
  initialization frame plus n. Without native audio the app waits no loading, so the clock was
  right (LEAGUE's hidden smoke finished five events).
- **Evidence.** Warm route: the race (DRAGSTER) is chosen at front-end frame 2708, initializes on
  2827 (120 loading frames), and the script's Start at 3300 opened the app's pause at 3180.
  Turnaround: race 2 (track 1) is chosen at 11808, initializes on 11975 (168), and the script's
  switch from Left to Right at 12403 reached the app 168 frames early; first cue difference
  12619. `front_end_runner` fed the race rows moved by the loading writes main's app's cue log
  byte for byte on both routes (warm 6,921 lines through 3599; turnaround through 12999).
- **Change.** `race_initialization_frame(const FrontEndState&)` in `src/core` (the runner's
  inline expression, now shared); `ScriptedRaceFrames` in `src/app/frontend.hpp`, started with
  the initialization frame and the loading the app waits; `sdl_main.cpp` reads every scripted race
  (two-human, and one-player with native audio) through it.

| Route (app against `front_end_runner`, resolved cue logs) | Frames | Main's app | Candidate's app |
| --- | ---: | --- | --- |
| warm podium route | 3,600 | differs from 3180 | equal (6,201 lines) |
| `organic-full-tour-turnaround` | 13,000 | differs from 12,619 | equal |
| `organic-full-tour-turnaround` | 40,264 | race 2 never ends | equal (61,324 lines, 25 podium lines; five returns on the runner's frames and totals) |
| AUDIO-ONE-PLAYER `two-races` | 5,400 | equal (8,223 lines) | equal (8,223 lines) |
| AUDIO-ONE-PLAYER `bowl-quit` | 3,601 | equal (5,070 lines) | equal (5,070 lines) |

`front_end_runner`: main's and the candidate's outputs (cues, stdout, stderr) byte-identical on
all four routes (`runner_same.sh`). Against the original's derived cues on the turnaround
(PODIUM-AUDIO's `full-tour`), the podium's window 37233-38069 has 0 differing frames.

`app-podium-audio` now races: the catalogue step runs `app-league-race/app_podium_run.sh`, which plays the turnaround from a cold boot for
38,400 updates (the five events, the podium's session at 37234, the way back at 38068) and prints
DIFFERS unless the app's cue log equals the runner's with a podium in it and the runner's podium
window equals the original's. PODIUM-AUDIO's `app_run.sh` (the warm route, no podium) is kept
unchanged, so other branches' gates still pass it until this task merges.

## Handoff (worker)

- Branch `task/app-league-race` on claim `91ce4a40` (main `6797457b`). Commits: `0dd9aaf6` (the
  fix, tests, R-0106), `2cf022ee` (the catalogue step `app-podium-audio` runs
  `app-league-race/app_podium_run.sh`), then this record (handoff commit).
- Mutants: `ScriptedRaceFrames::start` without the waited loading fails
  `frontend_contract_tests` ("the first loading frame follows NOW PLAYING's last");
  `race_initialization_frame` without the `- 1` fails `front_end_tests` (line 550). The
  `sdl_main.cpp` change removed is main's app: `warm_route.sh` and `app_podium_run.sh` then print
  DIFFERS (3180 and 12619). The runner's change is the same expression moved into the shared
  function, so removing it changes nothing (`runner_same.sh`).
- Checks run: `project.py build` `status=passed` for app-debug, lab-debug and lab-release;
  `ctest` lab-debug 41/41 and app-debug 41/41; `clang-tidy` 19 (`-p build/app-debug`) clean on
  `race_result.cpp` and `front_end_runner.cpp` (19 warnings suppressed, all in non-user code);
  `git-clang-format` 19 clean on the changed `src` and `tests/app` lines
  (`tests/native/front_end_tests.cpp` keeps its own 100-column style); `regen.sh` then
  `coverage native-symbols --check` `status=passed` (0 cited addresses without a record; the
  index and the static map unchanged: no new ROM address); `tests.tooling.test_gates` OK.
  Final run on `2cf022ee` (`final_checks.sh`, output `final-2cf022ee/`): `runner_same.sh` rc 0 (warm route 6,201 cue
  lines, turnaround 61,324, `two-races` 8,223, `bowl-quit` 5,070, all byte-identical to main's
  runner); `warm_route.sh` rc 0 (equal through 3599, 6,201 lines); `app_podium_run.sh` rc 0 in
  769 s: the app's cue log equals the runner's through 38399 (59,456 lines, 25 podium lines, the
  session at 37234), five returns on the runner's frames and totals, the producer 37,780 cued
  frames, never stopped, only the boot's drop (2,815 pairs, peak 0), 3,526 underrun pairs; the
  runner's podium window 0 frames differ from the original's. After the run the script bounds
  the original's list at frame N-1 (the run's own "outside" count included the original's frames
  past 38399); recomputed on the run's outputs: 8 frames differ outside the podium, as on main.
  Synthetic suite and gates not run by the worker.
- For the gates: `--task-checks local/evidence/app-league-race/checks.list` (`runner_same.sh`
  with `$B/front_end_runner`; `warm_route.sh`), and the catalogue step `app-podium-audio` (about
  770 s of its 1200 s timeout). Expected changes against main: none in
  `front_end_runner` or any lab runner; the app's scripted races only. The pack profile is
  unchanged (the scripts keep PODIUM-AUDIO's v40 pack).
- **Note for parallel branches:** the new step script fails on a tree without R-0106 (its app
  never finishes the turnaround's second race), but each branch's own catalogue names its script,
  so branches cut before this merge keep running `podium-audio/app_run.sh`, which is unchanged.
- Evidence: main `local/evidence/app-league-race/`, about 45 MB: `explore/` (before and after
  runs on both routes, the shifted-input runner runs, the 1P runs, `after_runs.sh`,
  `one_player_runs.sh`), `base-6797457b/` (main's app and runner; the app loads SDL and the DSP
  library from this worktree's build), `runner_same.sh`, `warm_route.sh`, `app_podium_run.sh`,
  `final_checks.sh`, `checks.list`, `final-2cf022ee/`. The private `TMPDIR` is under the
  worktree's `artifacts/tmp`.
- Not covered (R-0106): only the podium's window of the runner is compared with the original
  here (its 8 other differing frames on `full-tour` are main's too); the app reports race state
  only as cues and totals; a one-player race after a notice; the idle demo's race takes the live
  pads (unchanged).
- Time: claim 22:21 UTC; worker 22:22:50 to 23:11 UTC on 6 November 2026 (`date -u`),
  about 48 min: reading 10 min; reproduction, the shifted-input runs and the cause 10 min; the
  change, tests and lints 10 min; the after runs and the final check run (real-time app runs,
  about 35 min of app time under the heavy-run lock) with the records written alongside.

## Review round 1 (approved with one should-fix finding, `c04980b4`)

A fresh reviewer subagent (Claude Opus 5.5, no inherited conversation, its own checkout
`.worktrees/app-league-race-review` at `c04980b4`) approved: report in
`local/evidence/app-league-race/review-1/review.md`, 23:13-23:50 UTC on 6 November 2026 (about
37 min). It rebuilt the candidate and reproduced every headline number (warm route 6,201 lines
equal, main's app differs at `3180 D pause`; the turnaround through 38399, 59,456 lines, five
returns on the runner's frames). It also ran routes the worker had not: 1P `cont-loss` (13,223
lines), `six-quits` (14,272) and `w-demo-race` (8,768), all equal. VS `vs-bowl` without audio
returned as the runner does on both apps. TWO-HUMAN-STUNTS-AUDIO's 2P `twop-bowl` with audio is
equal on the candidate, while main's app differs from line 2372 and returns 48 frames late (4760,
runner 4712), so the bug also hit plain 2P races with audio. It checked that the combined read
condition selects the same races as the old two branches, and that the worker's scripts print
DIFFERS on corrupted inputs. Each mutant fails a test except one (note 5).

- **1 (should-fix):** `sdl_main.cpp`'s local `race_initialization_frame` counted app updates
  and served as the loading-cue countdown and a flag, beside the new front-end-frame function
  of the same name. **Fixed:** renamed `race_start_update`, and the read condition names its flag
  `race_waits_loading`. No behaviour change.
- **2 (note):** `ScriptedRaceFrames`' comment said the loading frames are "just before" the
  initialization frame; `start(I, L)` reads I-L+1 through I. **Fixed** in the comment.
- **3 (note):** two loose test comments in `front_end_tests.cpp`. **Fixed** (a demo has no
  measured loading only on tracks 5 and up; the initialization frame's rule reworded).
- **4 (note):** an initialization frame of 0 would make a two-human race read from frame 1;
  unreachable (every track 0-44 has a loading offset, and the runner refuses races without one).
  Left as is.
- **5 (note):** the runner's call of the shared function has no unit test; a `+1U` mutant passes
  ctest and is caught by `runner_same.sh`, `warm_route.sh` and the gates. Left to those checks:
  the runner's frame numbering has no unit seam.
- **6 (note):** `app_podium_run.sh` ties the runner to the original only in the podium window
  (by design), ignored the app's exit status, keeps the v40 pack, and ran in 770 s of 1200 s.
  **Fixed:** an app exit status other than 0 now prints DIFFERS. The rest stays as recorded.
- **7 (note):** `twop-bowl` as a short non-league check. **Done:**
  `local/evidence/app-league-race/twop_bowl.sh`, added to `checks.list`.

Checks after the fixes: `project.py build` `status=passed` for app-debug, lab-debug and
lab-release; `ctest` app-debug and lab-debug 41/41; `git-clang-format` clean on the changed
`src` and `tests/app` lines. The task's checks (`fix-1/run.out`, 23:52-00:08 UTC):
`warm_route.sh` equal through 3599 (6,201 lines); `twop_bowl.sh` equal through 6099 (9,019 lines,
the race returns at 4713); `runner_same.sh` byte-identical on all four routes;
`app_podium_run.sh` app rc 0, equal through 38399 (59,456 lines, 25 podium lines), five returns on
the runner's frames and totals, the producer 37,780 cued frames and never stopped, the runner's
podium window 0 frames differing from the original's (8 outside it, as on main).
