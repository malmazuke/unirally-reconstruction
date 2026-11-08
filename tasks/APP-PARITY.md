# APP-PARITY - the shipped app plays what the runner plays

## Assignment

- Status: **claimed** 8 November 2026 03:10 UTC on main `68b1c9b4`, on the user's instruction
  ("Start APP-PARITY"). Queued 8 November 2026 by [COVERAGE-CENSUS](COVERAGE-CENSUS.md) (R-0109,
  queue item 1). Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`): coordinator and
  integrator; a worker subagent implements, a fresh subagent reviews. Run under
  [D-0013](../docs/decisions/D-0013-task-time-and-disk-budget.md): claim to merge against the
  6-hour target, new evidence under 1 GB. Usage at the claim: 5-hour 11%, weekly 14%.
- Branch `task/app-parity` in `.worktrees/app-parity`; evidence in main
  `local/evidence/app-parity/`.
- Task provider: Anthropic.
- Milestone: M4. Review tier: **1** since the worker's change moved the runner's output writers to
  `src/core` (queued as 2: the app shell and its defaults; a change that reaches `src/core`
  race or front-end state raises it to tier 1).

The lab runner (`front_end_runner`) is checked against the original frame for frame, but the shipped
app (`unirally`) still differs where R-0109's app list says: audio and saves are opt-in
(`--native-title-menu-audio`, `--save-file`), the idle demo's race takes the live pads instead of the
demo's own inputs, a one-player race after a notice is not run, and the app reports no race state per
frame (R-0090, R-0106). Outcome: the app plays with native audio and a default save file unless told
otherwise, the idle demo's race runs as the runner's does, the race after a notice runs, and the app
can write the runner's per-frame race rows so a scripted app run is compared with the runner on rows,
cues and pictures (not only cues and totals).

## Checkpoint (worker, 8 November 2026)

[R-0110](../docs/research/R-0110-app-parity.md) holds the detail.

- **Causes.** (2) `sdl_main.cpp` read a scripted race's pads from the script only for a two-human
  race or a one-player race whose loading it waits (audio); the idle demo's race, and a one-player
  race without audio, took the live pads, so a scripted pad press never ended a demo. (3) After a
  notice the front end goes back to the main menu's first state, frame counter included, and read
  the script there again: the same presses chose the same notice (main's app: 29 notices in 5,581
  updates, no race). (4) The runner's row and digest writers were private to it. (1) Defaults.
- **Changes.** `launch_settings` and `default_save_file` (audio and a save by default, opt-outs,
  laboratory launches keep the old defaults); every race from the menus numbered with the runner's
  frames (`ScriptedRaceFrames`, the demo's race included) and read from the script there;
  `FrontEndSession::script_frame`/`script_offset` (the script goes on past a notice);
  `src/core/lab_outputs.{hpp,cpp}` (the runner's row, PPM and digest writers, shared) and the app's
  `--race-timeline`, `--one-player-timeline` and `--frame-digests`; the screen blank while a race
  loads.
- **Decisions.** A laboratory launch is `--hidden`, `--front-end-inputs` or
  `--fixed-controller-mask`: silent and saveless unless asked, so every existing gate and evidence
  run (all pass `--hidden`) behaves as before and never touches the player's save. The default save
  is `~/Library/Application Support/Unirally/unirally.sav` (macOS),
  `$XDG_DATA_HOME/unirally/unirally.sav` or `~/.local/share/unirally/unirally.sav` (Linux and other
  Unix), `%APPDATA%\Unirally\unirally.sav` (Windows), read from the environment. A launch that did
  not ask for audio runs silent when the pack has none or no device starts; `--audio` (old name
  `--native-title-menu-audio`, still accepted) makes either an error. The app writes only the
  runner's `P` (picture) digests, not `V` or `R`.

## Handoff (worker)

- Branch `task/app-parity` on claim `572839dd` (main `68b1c9b4`). Commits: `fb4821bf` (the
  change, tests and user docs), then the records commit (R-0110, this handoff, R-0109's pointer).
- **Review tier 1**, not 2: `src/core` changed (`lab_outputs.{hpp,cpp}`, the runner's row, PPM and
  digest writers moved there; `front_end_runner.cpp` calls them). The runner's outputs are
  byte-identical to main's; no race or front-end state code changed. The rest is `src/app`.
- Results (app-debug, `--hidden --native-title-menu-audio`, real time, against lab-release
  `front_end_runner`; pack v41 unless stated; main's app is `base-68b1c9b4/`, cues only):

| Item | Route | Frames | Candidate's app against the runner | Main's app |
| --- | --- | ---: | --- | --- |
| 2 | DEMO-AUDIO `demo-pad-early` | 9,000 | cues 13,662, demo rows 4,280, pictures 8,997: equal; exit at 5001 | ignores the press (timeout 6424); cues differ from 5001 |
| 2 | `demo-pad-late` | 10,000 | 16,535 / 5,680 / 9,997 equal; exit 6401 | differs from 6401 |
| 2 | `demo-pad-port2` (pad 2, split demo) | 11,000 | 17,948 / 6,075 / 10,996 equal; exit 8001 | differs from 8001 (timeout 9479) |
| 2, 4 | `demo-long` (gate step's route) | 13,000 | 21,879 / 7,604 / 12,996 equal | (gate: cues equal) |
| 3 | `two-races` after the WIPE RAM notice (v35) | 5,581 | cues before the code 348 and after the notice (moved back 181) 7,955 equal; one-player rows 2,550, pictures 500 + 4,979 equal; race returns at 3454, 3357/3358 | 29 notices, no race |
| 4 | AUDIO-ONE-PLAYER `two-races` | 5,400 | cues 8,223, one-player rows 2,550, pictures 5,399 equal | (cues equal, R-0106) |
| 4 | the same without audio (`SILENT=1`) | 5,400 | one-player rows 2,550 equal | (takes the live pads: rows part at 1500, M6) |
| 4 | TWO-HUMAN-STUNTS-AUDIO `twop-bowl` | 6,100 | cues 9,019, race rows 2,727, pictures 6,100 equal | (cues equal, R-0106) |
| 1 | `defaults.sh` (HOME a scratch folder) | 150 a case | audio and the default save (8,192 bytes) written, then loaded; `--no-audio --no-save-file` and `--hidden` silent, no file; `--native-title-menu-audio` plays; no audio device: silent, `--audio` fails | silent, no save (its hidden run as the candidate's) |

- Runner: `runner_same.sh` byte-identical to main's on stdout, stderr, cues, race, one-player and
  look timelines and frame digests (`demo-pad-port2` 17,948 cue / 6,075 row / 20,850 digest lines;
  `two-races` 8,223 / 2,550 / 13,359; `twop-bowl` 9,019 / 2,727 / 15,445).
- The old app checks on the candidate: APP-LEAGUE-RACE's `warm_route.sh` (equal through 3599, 6,201
  lines) and `twop_bowl.sh` (equal through 6099, 9,019 lines, return at 4713). The demo-long run is
  the `app-demo-audio` step's route and equals the runner.
- Mutants (R-0110 "Each change reverted"): M1 audio default off (ctest and `defaults.sh` fail), M2
  demo on live pads, M5 demo numbered without `- 1`, M6 silent 1P on live pads, M3 race without
  `script_offset`, M4 front end without `script_offset`, M7 the shared digest's seed: each check
  DIFFERS. Main's app is the all-reverted case for items 1-3 (table).
- Checks run: `project.py build` `status=passed` for app-debug, lab-debug and lab-release;
  `ctest` app-debug 41/41 and lab-debug 41/41; `clang-tidy` (`-p build/app-debug`) clean on
  `lab_outputs.cpp`, `front_end_runner.cpp`, `frontend.cpp`, `save_file.cpp`, `sdl_main.cpp`;
  `git-clang-format` applied to the changed `src/app` and `tests/app` lines (clean on `src/core`);
  `coverage native-symbols --check` `status=passed` (no new ROM address, so no static-map regen);
  `PYTHONPATH=tools python3 -m unittest tests.tooling.test_frontend` 15 OK. app-release not built
  (no live listening check was needed). Synthetic suite and gates not run by the worker. Platform
  paths are plain `std::getenv` and `#if` on `__APPLE__`/`_WIN32`; not built on Linux here (CI).
- For the gates: `--task-checks local/evidence/app-parity/checks.list` (five checks, about 25 min:
  `runner_same.sh "$B/front_end_runner"`, `defaults.sh` (opens a visible window and plays sound,
  about 3 s a case, `HOME` pointed at `$G`), `demo_pads.sh` (10 min), `notice_race.sh` (2 min),
  `race_routes.sh` (6 min)). Expected changes against main: none in `front_end_runner` or any lab
  runner; the app's defaults (laboratory runs unchanged) and its scripted demo, silent one-player and
  after-notice races. The pack profile is unchanged.
- Limits (R-0110): the app writes only the runner's `P` digests (no `V`, `R`); with
  `--frame-digests` the debug app draws every update and its producer reports 0-7 later drops (no
  effect on cues, rows or pictures); NEON's leftover colours and the held-pose fallback are drawn by
  the app only (no route here has them); no local pack that runs the front end lacks native audio,
  so that degrade path is read, not run (the no-device path is run); a notice is the app's own
  screen and is compared with the runner's route without it; short titles are still not predicted.
  `docs/research/README.md` does not exist, so R-0110 is linked from this record, R-0109 and
  `docs/BUILD_AND_VALIDATION.md`.
- Evidence: main `local/evidence/app-parity/`, 26 MB after gzip: the scripts (`app_vs_runner.sh`,
  `demo_pads.sh`, `notice_race.sh`, `notice_route.py`, `notice_compare.py`, `race_routes.sh`,
  `defaults.sh`, `runner_same.sh`, `batch1.sh`, `mutants.sh`, `m6.sh`, `checks.list`), the runs
  (`item1-*` to `item4-*`, `mutants/`, `m6/`, `batch1.out`, `mutants.out`, `m6.out`) and
  `base-68b1c9b4/` (main's app and runner; the app loads SDL and the DSP library from this
  worktree's build). Private `TMPDIR` under the worktree's `artifacts/tmp`. A note: while
  `batch1.sh` ran, `app_vs_runner.sh` was edited once (a silent mode); the twop-bowl run in flight
  printed normal results, and `race_routes.sh` reruns it in the gates.
- Time: worker 03:11:30 to 04:26 UTC on 8 November 2026 (`date -u`), about 1 h 15 min: reading
  and design 10 min; the change and the first builds 10 min; then about 55 min of app runs under
  the heavy-run lock (item 2 03:23-03:34, batch 1 03:35-04:00, mutants 04:01-04:20, the M6 rerun
  04:21-04:24), with the tests, lints, the commit and the records done alongside.

## Review round 1 fixes (worker)

Review: `local/evidence/app-parity/review-1/review.md` (request changes, one blocking finding).

- **1 (blocking), `defaults.sh`.** Rewritten without `eval`: each case is a function, every path is
  quoted, and every exit-code test reads an rc file that must exist (`rc` prints `missing`, never a
  number, so a missing file fails both `== 0` and `!= 0`). Run from absolute paths with a space.
- **2, no window or sound.** `defaults.sh` runs every case with `SDL_VIDEO_DRIVER=dummy
  SDL_RENDER_DRIVER=software SDL_AUDIO_DRIVER=dummy` (still a player's launch); `checks.list`'s
  comment says so.
- **3, an unusable default save.** `open_save_file` (`sdl_main.cpp`): a default save that cannot be
  read leaves the launch without one (`Save file: none (cannot use PATH: why)`); a first write that
  fails (the read-only home's `create_directories`) drops it (`Save file: none from now on (cannot
  write it: why)`) and play goes on. A damaged one (`DamagedSaveFile`, not 8,192 bytes) is refused
  and kept, its message naming `--no-save-file` and `--save-file PATH`. **Decision:** a
  `--save-file PATH` the player named that cannot be used stays an error. New `defaults.sh` cases:
  `read-only`, `read-only-named`, `damaged` (10 cases now).
- **4.** `SaveFile`'s comment is back above its class.
- **5, notes.** The notice term is now in `ScriptClock` (`frontend.hpp`), which `FrontEndSession`
  uses, with a contract test of the clock through two notices (`script_clock_rule`); R-0110 states
  the 3454/3455 labelling (the runner's last race frame, the app's next front-end frame); README and
  `src/app/README.md` no longer call menus, multiplayer, LEAGUE or OPTIONS missing, give the
  relative-`XDG_DATA_HOME` rule and the save's degrade rules, and say no menu route reaches a notice
  from pack v36 (every track 0-44 has a scenario); `notice_compare.py` prints `pictures: DIFFERS`
  and exits 1 when there is no notice; `app_vs_runner.sh` clears its route's old outputs first;
  `frontend run` reports a `--track` run's audio as `off (--track)`; R-0110's Limits add what item 3
  reaches (mainly lab scripts past a notice, tied to the original only through the runner), the
  Windows `getenv` decoding and the save's degrade rules. `tasks/README.md`'s tier is left to the
  coordinator.
- Mutants: **M8** `ScriptClock::frame`'s notice term frozen: `frontend_contract_tests` fails ("a
  notice's frames count"; `review-1-fixes/mutant-clock.out`). **M9** the store's degrade removed:
  `defaults.sh` `read-only` DIFFERS, rc 1 (`review-1-fixes/mutant-save-degrade.out`). Main's app on
  the new `defaults.sh`: 8 of 10 cases DIFFER (`no-device-asked` and `read-only-named` pass: main's
  app refuses `--audio` as an unknown option, and fails on the read-only `--save-file` too).
- Checks: `project.py build` `status=passed` app-debug and lab-debug; `ctest` app-debug 41/41,
  lab-debug 41/41; `git-clang-format` applied on the changed `src/app` and `tests/app` lines;
  `clang-tidy` clean on `frontend.cpp`, `save_file.cpp`, `sdl_main.cpp`; `coverage native-symbols
  --check` `status=passed` (no `src/core` change this round); `tests.tooling.test_frontend` OK.
  The whole `checks.list` once, each line with its own absolute `$G` under
  `artifacts/round 1/checks/` (spaces in the path) and `$B` main's binaries
  (`run_checks.sh`, `review-1-fixes/checks.out`): all five rc 0, 05:21-05:42 UTC:
  `runner_same.sh` byte-identical on all three routes; `defaults.sh` 10/10; `demo_pads.sh` equal on
  all three routes (13,662 / 4,280 / 8,997, 16,535 / 5,680 / 9,997, 17,948 / 6,075 / 10,996);
  `notice_race.sh` equal (cues 348 + 7,955, rows 2,550, pictures 500 + 4,979); `race_routes.sh`
  equal (two-races 8,223 / 2,550 / 5,399, twop-bowl 9,019 / 2,727 / 6,100, silent two-races 2,550).
- Commits: `2f0a4fa7` (the code, tests and user docs), then this record.
- Time: 05:15:42 to 05:43 UTC (`date -u`), about 28 min, of which the checks run took 22 min.

