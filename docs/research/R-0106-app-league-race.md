# R-0106 - The app's scripted two-human races read the script their loading late

Status: research result and app change ([APP-LEAGUE-RACE](../../tasks/APP-LEAGUE-RACE.md)),
7 November 2026, on main `6797457b`. PAL ROM SHA-256
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`. No new ROM address is cited:
this is a fault in the app's replay of a laboratory input script, not in recovered behaviour.
Evidence in main `local/evidence/app-league-race/`.

Tags: **[C]** an experiment made for this task; **[S]** read from the source at main `6797457b`.

## Question

On the same save and inputs the app (`unirally`, `--front-end-inputs`, `--native-title-menu-audio`)
and `front_end_runner` part ways in a league race, and `front_end_runner` is the one that matches
the original:

1. PODIUM-AUDIO's warm route (`podium-audio/app_run.sh`, the image `app/podium-pending.bin`, the
   inputs `app/warm-podium.frontend-inputs.txt`): since R-0105 it reaches NOW PLAYING at 1720 and
   a league race from 2700, and the app logs pause cues from frame 3180 (CARTRIDGE-OPTION-BITS'
   gates round 1).
2. LEAGUE's `organic-full-tour-turnaround` (R-0073; the same schedule is PODIUM-AUDIO's
   `full-tour`): the app's second race (track 1) diverges at frame 12,619 and never ends (R-0099).

Where do they first differ, and why?

## Cause

- **The app's script clock for a two-human race** [S] (`src/app/sdl_main.cpp`). When NOW PLAYING
  chose the race, the app set its counter to the race's initialization frame,
  `front_end_frame() - 1 + race_loading_frames()`, the frame `front_end_runner` initializes the
  race on (`start_race`). It then advanced the counter on every update and read the script's row
  for it. With native audio the app first waits the race's loading frames, as the original does
  (D-0010, R-0077), and those updates advanced the counter too. So the race's n-th update read
  the row for the initialization frame plus the loading plus n, where `front_end_runner` reads
  the initialization frame plus n: **the race's script was read its loading frames late.**
- Without native audio the app does not wait the loading, so the counter was right. That is why
  LEAGUE's hidden app smoke (no audio, `full-app.inputs`) finished all five events.
- A one-player race with native audio read the script by the app's update count instead, which
  equals `front_end_runner`'s frame while the app's updates keep step with the front end's frames
  (they do not after a notice, whose frames the front end does not count).

## Observations [C]

The first differences, and the same differences reproduced by `front_end_runner` with the race's
rows moved earlier by its loading (`explore/`):

| Route | Race | Chosen (front-end frame) | Initialized | Loading | Script around it | First cue difference |
| --- | --- | ---: | ---: | ---: | --- | --- |
| warm podium route | DRAGSTER (track 0), league pair | 2708 | 2827 | 120 | Start on pad 1 at 3300-3305 | 3180: the app opens the pause menu (`D pause`, `E 3 128`, `D pause-fade`) |
| `organic-full-tour-turnaround` | race 1, track 0 | 6908 | 7028 | 121 | Right on both pads 7100-10499 | none in this race |
| `organic-full-tour-turnaround` | race 2, track 1 | 11808 | 11975 | 168 | Left on both pads 11990-12402, then Right (pad 2 from 12403, pad 1 from 12405) | 12619: `E 11 20` and `E 2 14` missing from the app's log |

- The warm route: 3180 + 120 = 3300, the Start press. `front_end_runner` with the rows from 2900
  on moved 120 frames earlier (`explore/warm-podium.race-shifted-120.inputs`) writes the app's cue
  log byte for byte, all 6,921 lines through frame 3599.
- The turnaround: `front_end_runner` with race 1's update frames (7029-9122) reading the rows 121
  frames later and race 2's (11976-13000) 168 later
  (`explore/turnaround-before/race-shifted.inputs`) writes main's app's cue log byte for byte
  through frame 12999. Race 1's one long hold made the late read harmless there; race 2's switch
  from Left to Right came 168 frames early.

So both divergences have this one cause, and nothing else differs on these routes through those
frames.

## What changed

- `race_initialization_frame(const FrontEndState&)` (`src/core/front_end.hpp`,
  `race_result.cpp`): NOW PLAYING's last front-end frame plus the race's loading, 0 when the
  loading is not measured. `front_end_runner`'s `start_race` and the app now both take the race's
  initialization frame from it (the runner computed the same expression inline).
- `ScriptedRaceFrames` (`src/app/frontend.hpp`): the frames a scripted race reads, numbered as
  `front_end_runner` numbers them. `start(initialization_frame, loading_waited)` makes the first
  update after the choice read `initialization_frame + 1 - loading_waited`: with native audio the
  waited loading frames read the frames before the initialization frame, and the race's n-th
  update reads the initialization frame plus n either way.
- `sdl_main.cpp`: a scripted two-human race, and with native audio a one-player race, read the
  script through one `ScriptedRaceFrames`, started when NOW PLAYING chooses the race with the
  loading the app will wait. The one-player path's frames are unchanged whenever the app's updates
  keep step with the front end's, which they do on every route measured here.
- `front_end_runner`'s outputs are byte-identical to main's (cues, stdout and stderr) on the warm
  route, the turnaround (40,264 frames) and AUDIO-ONE-PLAYER's `two-races` and `bowl-quit`
  (`runner_same.sh`).

## After the change [C]

The candidate's app (`app-debug`, `--native-title-menu-audio`, real time) against
`front_end_runner` (lab-release), resolved cue logs compared byte for byte:

| Route | Frames | Main's app | Candidate's app |
| --- | ---: | --- | --- |
| warm podium route (`app/podium-pending.bin`) | 3,600 | differs from 3180 (6,921 lines against the runner's 6,201) | equal, 6,201 lines |
| `organic-full-tour-turnaround`, cold boot | 13,000 | differs from 12,619 | equal |
| `organic-full-tour-turnaround`, cold boot | 40,264 | (never ends race 2) | equal, 61,324 lines, 25 podium lines |
| AUDIO-ONE-PLAYER `two-races` | 5,400 | equal (8,223 lines) | equal (8,223 lines) |
| AUDIO-ONE-PLAYER `bowl-quit` | 3,601 | equal (5,070 lines) | equal (5,070 lines) |

- On the whole turnaround the candidate's app races all five league events and returns from
  each on `front_end_runner`'s frame with its totals: 3293/3290 (9123), 9690/9686 (17269),
  60000/60000 (21716), 8430/8429 (28268) and 10989/10994 (36005, R-0073's GOING UP totals of the
  original). Then the podium (its session at 37234), the way back (38068), KEEP SCORES and the
  tour's re-entry.
- The producer ran 39,644 cued frames and never stopped; its only drop is the boot's (2,366
  pairs, peak 0), no later drop, 4,165 underrun pairs (this run shared the machine with builds).
- `front_end_runner`'s cues against the original's derived list for the same schedule
  (`full-tour`, PODIUM-AUDIO): the podium's window 37233-38069 has 0 differing frames (35
  original lines); 8 frames differ elsewhere (10603, 17680, 22202, 28680, 36416, 38167, 38171,
  38172), on main's runner too: the runner's outputs are unchanged.

## `app-podium-audio`

Since R-0105 no warm boot reaches a podium (the league slots clear the podium owed), so the app's
podium check must race a league's last event. `podium-audio/app_run.sh` now plays the turnaround
schedule from a cold boot (`cues/full-tour.frontend-inputs.txt`, the same rows as LEAGUE's
`full-app.inputs`) for 38,400 updates: five league events, the podium (its session at 37234) and
the way back (the title session at 38068). It reports the producer's frames and drops, requires
the app's cue log to equal `front_end_runner`'s with a podium in it, and ties the runner to the
original: `front_end_runner`'s cues in the podium's window (37233-38069) against the original's
derived list for PODIUM-AUDIO's `full-tour` capture (`cues/full-tour.cues.txt`). Any mismatch
prints DIFFERS. The previous script is `app_run.sh.before-app-league-race`.

## Reproduction

From the worktree (local/ linked; app-debug and lab-release built), each through
`local/locks/heavy-run.sh`:

1. Before: main's app (`artifacts/unirally-before`, built at the claim) on both routes:
   `cartridge-option-bits/gates-round-1/app_pair.sh` for the warm route; the app with
   `--updates 13000 --front-end-inputs local/evidence/league/full-app.inputs` for the turnaround.
   The shifted-input runs of `front_end_runner` as in Observations.
2. After: `explore/after_runs.sh OUTDIR` (both routes, the turnaround's whole 40,264 frames),
   `explore/one_player_runs.sh OUTDIR` (the one-player path, main's app and the candidate's).
3. The task's checks: `local/evidence/app-league-race/checks.list` (`runner_same.sh`,
   `warm_route.sh`) and the catalogue step `app-podium-audio` (`podium-audio/app_run.sh`).

## Limits

- The app's cue log is compared with `front_end_runner`'s; only the podium's window of the
  runner's is compared with the original's here. Outside it the runner keeps its known
  differences from the original on `full-tour` (8 frames: R-0098's late way back and others,
  unchanged by this task).
- The app reports no race state per frame, so race state is compared through the cue log and the
  race totals the app prints at each return.
- A one-player race after a notice was not run: the change makes it read `front_end_runner`'s
  frames, where it read the app's update count before.
- The idle demo's race still takes the live pads, not the script (unchanged; DEMO-AUDIO's check
  does not press a pad during a demo race).
