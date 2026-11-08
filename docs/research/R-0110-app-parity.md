# R-0110 - The shipped app plays what the runner plays

Status: research result and app change ([APP-PARITY](../../tasks/APP-PARITY.md)), 8 November
2026, on main `68b1c9b4`. PAL ROM SHA-256
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`. No new ROM address is cited:
the faults were in the app's defaults and its replay of a laboratory input script, not in recovered
behaviour. Evidence in main `local/evidence/app-parity/`. Queue item 1 of
[R-0109](R-0109-coverage-census.md) ("The app against the runner").

Tags: **[C]** an experiment made for this task; **[S]** read from the source at main `68b1c9b4`.

## Question

R-0109 lists where the shipped app (`unirally`) differs from `front_end_runner`, which is checked
against the original frame for frame:

1. Audio and saves are opt-in (`--native-title-menu-audio`, `--save-file PATH`).
2. The idle demo's race takes the live pads, not the demo's inputs.
3. A one-player race after a notice is not run (R-0106).
4. The app reports no race state per frame, so a scripted app run is compared only on cues and
   totals (R-0106).

What makes each differ, and can the app match the runner on the same inputs?

## Causes [S]

- **Item 2.** `sdl_main.cpp` read a scripted race's pads from the script only for a two-human race,
  or a one-player race whose loading the app waits (native audio). Every other race update took
  `input.snapshot()`, the live keyboard and gamepads: the idle demo's race always, and a one-player
  race without audio. `front_end_runner` gives every race frame the script's row for that frame
  (`update_demo_race`, `update_local_race`). In the original the demo's riders are driven by the
  track's markers (`update_demo_controllers`, R-0087) and the pads are read only to end the demo
  (R-0096), so the only input a demo race takes is the press that exits it. A scripted app run
  never saw a press during a demo race: DEMO-AUDIO's three pad-exit schedules ran to the demo's
  timeout in the app.
- **Item 3.** The front end reads the script at `front_end_frame()`. A notice (a mode the app does
  not have, here the WIPE RAM code with a pack before v36) shows for 100 updates without running
  the front end, then puts back the main menu's state as it first appeared, its frame counter
  included (`FrontEndSession::update`). So after a notice the app read the script's rows from the
  main menu's first frame again: the same presses chose the same notice, and the app never left
  the main menu (main's app: 29 notices in 5,581 updates, no race). A frame-keyed script cannot
  drive past a notice, so a race after one was never run.
- **Item 4.** The app had no row or picture output; `front_end_runner`'s writers were private to
  the runner.
- **Item 1.** Defaults: audio and the save file were off unless asked for.

## What changed

- **Defaults (item 1)** (`launch_settings`, `src/app/frontend.cpp`; `default_save_file`,
  `src/app/save_file.cpp`). A player's launch plays native audio and keeps a save file; `--no-audio`
  and `--no-save-file` opt out; `--save-file PATH` keeps working; `--audio` asks for audio
  explicitly and `--native-title-menu-audio` is its old name. **Decision:** a laboratory launch
  (`--hidden`, `--front-end-inputs` or `--fixed-controller-mask`) keeps the old defaults, silent and
  without a save, unless given `--audio` or `--save-file`. Every gate step, test and evidence script
  that runs the app passes `--hidden` (`catalogue.py`'s hidden runs and `app-demo-audio`,
  `podium-audio/*`, `app-league-race/*`, `demo-audio/gates.sh` and the other gate scripts), so each
  keeps its behaviour, and no scripted run reads or writes the player's save. A `--track` race has
  neither (both need the front end); `--audio` or `--save-file` with `--track` is refused as before.
  **Decision, the default save:** macOS `~/Library/Application Support/Unirally/unirally.sav`,
  Linux and other Unix `$XDG_DATA_HOME/unirally/unirally.sav` (`~/.local/share/unirally/` when it is
  unset or relative, as the XDG base directory specification says), Windows
  `%APPDATA%\Unirally\unirally.sav`, from the environment (not `SDL_GetPrefPath`, which creates its
  folder when asked; the app creates the folder on its first write). The app prints the file it
  uses. **Degrading:** a launch that did not ask for audio runs silent with a note when the pack has
  no native audio (before v31) or no audio device starts, as the app ran before; `--audio` makes
  either an error, as `--native-title-menu-audio` did.
- **The race's frames (items 2 and 4)** (`ScriptedRaceFrames`, `sdl_main.cpp`). Every update of a
  race from the menus is numbered with `front_end_runner`'s frame, its loading frames and the idle
  demo's race included (a demo initializes on the frame before the front end's, as the runner's
  `start_race` labels it), and a scripted run reads every race's pads there: the one-player race
  without audio and the idle demo's race too. A live run still takes the pads, so a press still ends
  the demo.
- **The script past a notice (item 3)** (`ScriptClock`, `FrontEndSession::script_frame`,
  `script_offset`). The
  script's frames now go on through a notice and after it: the notice's 100 frames count, and when
  the main menu is put back the script runs `script_offset` frames ahead of the front end's frame
  (the notice's frames plus the main-menu frames the front end went back over). The front end and
  every race read the script at their frame plus the offset. Before any notice the offset is 0 and
  nothing changes. The equivalent runner script is the app's with the rows after the notice moved
  back by the offset (`notice_route.py` makes the app's from a runner script).
- **The runner's outputs from the app (item 4)** (`src/core/lab_outputs.{hpp,cpp}`). The runner's
  row writer (`race_state_row`), PPM picture (`ppm_picture`) and digest line (`digest_line`, FNV-1a)
  moved to shared code; the runner calls them and writes the same bytes. The app takes
  `--race-timeline FILE` (two-human races and idle demos, with the demo's setup row as the runner
  writes it), `--one-player-timeline FILE` and `--frame-digests FILE`: each frame's picture digest,
  the runner's `P` lines, labelled with the runner's frames, a race's loading frames blank as the
  runner draws them, none on the frame a race is chosen or a notice shows (the runner has neither),
  and on a race's last frame both the race's and the menus' pictures, as the runner writes them.
  The app's VRAM (`V`) and cartridge RAM (`R`) digests are not written: the app has no VRAM of a
  race, and its saved image is already compared through the save file (R-0090).
- **The screen while a race loads** is blank in the app, as in the runner and the original (it
  drew the race's first picture over the loading frames before).

## Observations [C]

The candidate's app (app-debug, `--hidden --native-title-menu-audio`, real time, pack v41 unless
stated) against `front_end_runner` (lab-release) on the same script, every output compared byte
for byte (`app_vs_runner.sh`); main's app (`base-68b1c9b4/`) on the cue logs (it has no other
output):

| Route | Frames | Output | Candidate's app | Main's app |
| --- | ---: | --- | --- | --- |
| DEMO-AUDIO `demo-pad-early` (A on pad 1, 5000-5003) | 9,000 | cues / demo rows / pictures | equal: 13,662 / 4,280 / 8,997 lines | cues DIFFER |
| DEMO-AUDIO `demo-pad-late` (Left, 6400-6403, in the fade) | 10,000 | cues / demo rows / pictures | equal: 16,535 / 5,680 / 9,997 | cues DIFFER |
| DEMO-AUDIO `demo-pad-port2` (B on pad 2, 8000-8003, split demo) | 11,000 | cues / demo rows / pictures | equal: 17,948 / 6,075 / 10,996 | cues DIFFER |
| DEMO-AUDIO `demo-long` (no press, short titles 13, 16, 18) | 13,000 | cues / demo rows / pictures | equal: 21,879 / 7,604 / 12,996 | (the gate step `app-demo-audio`: cues equal) |
| AUDIO-ONE-PLAYER `two-races` | 5,400 | cues / one-player rows / pictures | equal: 8,223 / 2,550 / 5,399 | (R-0106: cues equal) |
| TWO-HUMAN-STUNTS-AUDIO `twop-bowl` (2P) | 6,100 | cues / race rows / pictures | equal: 9,019 / 2,727 / 6,100 | (R-0106: cues equal) |
| `two-races` after the WIPE RAM notice (pack v35) | 5,581 | see below | equal | no race |

- The demo exits: the candidate returns from the second demo at front-end frames 5001 (early), 6401
  (late) and from the third at 8001 (pad 2), the frames after the presses, as the runner does. Main's
  app ignores the presses: its second demo times out at 6424 and its third at 9479, and its cue logs
  first differ from the runner's at frames 5001, 6401 and 8001, where the runner's demo ends.
- The notice route (`notice_race.sh`): two-races' script with the WIPE RAM code (`$02B0` on pad 1)
  at frames 500-503. The app shows the notice from script frame 501; the main menu resumes at
  front-end frame 420 (the runner's first main-menu state) at script frame 601, an offset of 181.
  Against `front_end_runner` on two-races itself: the cues before the code equal (348 lines), the
  cues after the notice moved back 181 equal (7,955 lines; the 2 lines between are the app's own),
  the one-player rows equal (2,550 lines, labelled with the runner's frames as they are), the
  pictures before the code equal (500) and after the notice equal (4,979). The race ends on the
  runner's frame 3454 with its totals 3357/3358: the runner logs `race returned at 3454`, the frame
  of the race's last update; the app logs `race returned at front-end frame 3455`, the front end's
  frame after the return (`front_end_frame()`, the frame its next update reads), with or without a
  notice. The rows and pictures, labelled alike, are equal. Main's app read the code again on every pass:
  29 notices, no race; its cues equal before the code and differ from the notice's end on.
- A one-player race without audio (a laboratory run's default, `SILENT=1`): the app's one-player
  rows equal the runner's before frame 5400 (2,550 rows; the race returns at 3454 with 3357/3358).
  With main's condition (live pads unless two humans or audio) the rows part at frame 1500, the
  script's Right press, which the app's race never saw.
- Each frame's picture: the app's digests equal the runner's on every frame of every route above,
  the front end, the loading frames, one-player, two-human and demo races (one-view and split), the
  result screens and the way back.
- `front_end_runner` is byte-identical to main's on every output (stdout, stderr, cues, race,
  one-player and look timelines, frame digests) on `demo-pad-port2` (11,000 frames), `two-races`
  (5,400) and `twop-bowl` (6,100) (`runner_same.sh`).
- The defaults (`defaults.sh`, `HOME` pointed at a fresh folder; since review 1 with SDL's dummy
  video, software renderer and dummy audio, still a player's launch, and from an output path with a
  space, as the gates' is; a read-only home plays on without a save, `--save-file` there fails, and
  a damaged default save is refused and kept): a player's launch plays native
  audio and writes the default save (8,192 bytes), which the next launch loads; `--no-audio
  --no-save-file` is silent and writes nothing; `--hidden` is silent and writes nothing, as main's
  app is; `--hidden --native-title-menu-audio` plays; with no audio device (`SDL_AUDIO_DRIVER`
  naming none) a player's launch runs silent with exit 0 and `--audio` fails. No local pack that
  runs the front end lacks native audio (every pack from v32 has it; the v29 development packs stop
  the front end in main's app too), so the missing-pack path is checked by reading only.
- Producer drops: the runs above report 0-7 later drops each (`native_audio_later_drops`, 7 on
  `demo-long`'s 13,000 updates). With `--frame-digests` the debug app draws and digests a picture on
  every update besides the producer's work, and the machine ran the runner and builds alongside; the
  drops are the host's latency policy and change no cue, row or picture (all are written by the game
  thread before the producer).

## Each change reverted [C]

`mutants.sh` (and `m6.sh`) revert one change at a time, rebuild and run the check that covers it;
each fails:

| Mutant | Check | Result |
| --- | --- | --- |
| M1 `launch_settings`' audio default off | `frontend_contract_tests`; `defaults.sh` | ctest fails; 2 cases DIFFER |
| M2 the idle demo's race on the live pads | `app_vs_runner.sh` `demo-pad-early` | cues, rows and pictures DIFFER from frame 5000 |
| M5 the demo's race numbered from the front end's frame (no `- 1`) | the same | rows DIFFER from the first update (1450), cues at 5004 |
| M6 a one-player race without audio on the live pads (main's condition) | `SILENT=1` `two-races` | rows DIFFER from 1500 |
| M3 a race after a notice read without `script_offset` | `notice_race.sh` | cues, rows and pictures DIFFER from 1381 (the app pauses at the countdown) |
| M4 the front end read at its own frame after a notice (main's) | `notice_race.sh` | notices again, no race: cues, rows, pictures DIFFER |
| M7 the shared digest's seed changed | `runner_same.sh` | the runner's digests DIFFER on all three routes |
| M8 `ScriptClock::frame` frozen during a notice (review 1) | `frontend_contract_tests` | fails: "a notice's frames count" (`notice_race.sh` cannot see it: during a notice only the producer's pad words read the script) |
| M9 an unwritable default save ends the launch again (review 1) | `defaults.sh` | `read-only` DIFFERS (rc 1) |

## Reproduction

From the worktree (`local/` linked; app-debug and lab-release built), each through
`local/locks/heavy-run.sh`, with `TMPDIR` under the worktree's `artifacts/tmp`:

- `local/evidence/app-parity/demo_pads.sh OUT [APP]` (item 2), `notice_race.sh OUT [APP]` (item 3),
  `race_routes.sh OUT [APP]` and `app_vs_runner.sh OUT NAME INPUTS FRAMES` (item 4),
  `defaults.sh OUT [APP]` (item 1), `runner_same.sh MAIN_RUNNER OUT`. With `CUES_ONLY=1` and main's
  app as APP the first three compare cues only; `SILENT=1` runs `app_vs_runner.sh` without audio.
  `batch1.sh` ran the candidate's and main's runs, `mutants.sh` and `m6.sh` the mutants; their
  outputs are `item*-*/`, `batch1.out`, `mutants.out` and `m6.out`.
- The task's checks: `local/evidence/app-parity/checks.list`.

## Limits

- **What item 3 reaches.** Every race track 0-44 has a scenario, so with pack v36 or later (v41
  here) no menu route shows a notice: only the WIPE RAM code on a pack before v36 does, which is why
  the item-3 check runs pack v35. Item 3 mainly keeps laboratory input scripts working past a notice.
  `front_end_runner` has no notices (it stops on a mode it does not have), and the original shows
  the WIPE RAM menu there, so there is no capture of the original through a notice: the app after a
  notice is tied to the original only through the runner on the route without it, which the gates
  check against the original (AUDIO-ONE-PLAYER's `two-races` cues, on v41).
- **The default save on Windows** is read with `std::getenv("APPDATA")` into a `path` from a
  narrow string, which decodes in the ANSI code page: a user folder with characters outside it
  would be mis-decoded, and MSVC warns (C4996). There is no Windows build or CI; `_wgetenv` would be
  the fix when there is.
- **An unusable save** (review 1): a default save that cannot be read or written leaves the
  launch without one, with a note (`Save file: none (cannot use ...)`, or `none from now on (cannot
  write it ...)` at the first write, a frame after power-on); a damaged one (not 8,192 bytes) is
  refused and kept as it is, its message naming `--no-save-file` and `--save-file PATH`.
  **Decision:** a `--save-file PATH` the player named that cannot be used stays an error.
- The app's outputs are compared with `front_end_runner`'s, which is compared with the original by
  the gates (DEMO-AUDIO's schedules, AUDIO-ONE-PLAYER, TWO-HUMAN-STUNTS-AUDIO). The app's PCM is the
  anchored producer's, measured, not exact (D-0010), and out of scope.
- The app's picture digests are its frames' `P` lines only; the runner's `V` and `R` lines have no
  app counterpart. A live (unscripted) run's pictures are drawn as before, after up to four
  catch-up updates, and are not digested unless asked. A rider pose outside the packed tables draws
  the held pose in the app (`used_pose_fallback`) where the runner's picture fails; no route here
  has one. NEON (track 42) draws the menus' leftover colours in the app (R-0068) and not in the
  runner's pictures; no route here races it.
- A notice is the app's, not the original's: after it the front end restarts from the main menu's
  first state (the rider and tour chosen last start over, the records kept), and the comparison is
  with the runner on the route without the notice. A laboratory run without audio has no loading
  frames, so its digests skip them; the checks run with audio.
- Short titles (R-0088, R-0096) are not predicted by the app: `demo-long`'s runner takes its
  `--short-demo-title` cycles (13, 16 and 18), all after frame 12999, where the comparison stops, as
  in the gate step.
