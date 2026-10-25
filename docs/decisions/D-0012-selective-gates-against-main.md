# D-0012 - Gates that choose their checks and compare with main

- Status: adopted 25 October 2026 with [GATES-AGAINST-MAIN](../../tasks/GATES-AGAINST-MAIN.md)
  (tier 1, #84), at the user's request of 24 October 2026.
- Owner: Claude Opus 5.5 (`claude-opus-5-5`), Claude Code desktop app, a side session of the
  RECORD-SCREENS coordinator.
- Related: [D-0003](D-0003-human-readable-native-code.md) (frozen differential checks),
  [D-0008](D-0008-static-map-track-breadth-review-tiers.md) (review tiers),
  [D-0011](D-0011-evidence-retention.md) (evidence retention).
- Tool: `tools/unirally_lab/gates/` ([BUILD_AND_VALIDATION](../BUILD_AND_VALIDATION.md#the-gate-runner-d-0012)).

## Problem

On 24 October 2026 the user said that hour-long gate checks, and a process that uses hundreds of
GB of disk, are not acceptable.

**Time.** RECORD-SCREENS' pre-merge gates (23 October, `local/evidence/record-screens/gates.sh`,
copied from PODIUM-AUDIO's) ran serially for 5 h 18 min under `local/locks/heavy-run.sh`.

| Part | Minutes |
| --- | --- |
| Builds of four presets, ctest, the synthetic suite | 15 |
| Eight hidden app runs (real time, about 80 s each) | 12 |
| The eleven frozen differential gates | 52 |
| The race equivalence sweep against main | 25 |
| Pause, two-pad, race-end, look and restart comparisons, track sweeps, earlier tasks' captures | 70 |
| MENU-INPUT's `checks.sh` / LEAGUE-TOURS' / PODIUM-AUDIO's | 35 / 57 / 22 |
| The task's own `checks.sh` | 55 |

**Growth.** Each task appends the previous task's `checks.sh` to its `gates.sh`, so every task's
gates are longer than the last.

**Disk.** Comparison scripts render every native picture as a 172 KB PPM into a temp directory and
compare it with the original's PNG. One gate step held 2.2 GB of temp; `wipe-ram/compare.py`
never deletes its directory (about 2 GB until the gate exits); `cond.sh` writes 140-215 MB into
evidence folders. The user saw 3.8 GB free during those gates. `local/evidence` holds 109 GB.

**Where the time goes.** The steps are mostly not checking the candidate:
- the earlier tasks' scripts compare, picture by picture in Python, native output that the
  candidate did not change with the original's frames;
- everything runs one step at a time on a 12-core machine.

The frozen race gates are mostly restore runs: hundreds of fresh processes, each replaying the rest
of a race from a saved state. Re-projecting the captures turned out to be a small part. This
section's first draft guessed otherwise, and the full run below corrected it.

## Decision

### 1. One runner

`python3 -m tools.unirally_lab.gates run`, from the candidate's checkout, under
`local/locks/heavy-run.sh`. It builds main's binaries at the merge base (one persistent worktree,
`.worktrees/gates-base`, built incrementally) and the candidate's presets. It then decides which
steps the change calls for and runs them in parallel where they do not contend. It prints a line
per step with its reason, and writes `report.json` (status, time, peak temp per step, the run's
peak disk). Task `gates.sh` scripts and the appending of earlier tasks' `checks.sh` stop. The step
catalogue (`catalogue.py`) holds every section of the last `gates.sh`.

### 2. What decides "touches its system"

A step names the binaries it drives. A binary can only behave differently if one of these changed:
- **its sources:** one of the repository files ninja recorded as compiled into it. This is
  `gate_identity`'s method, now per target: `front_end_runner`, `zoom_zoo_runner`, the presentation
  runners, `audio_driver_runner`, the fuzz runner and the app;
- **the content pack** it reads;
- **a build-wide file:** the presets, `third_party/`, `cmake/` or the toolchain lock, which change
  builds in ways no command line shows, so every binary counts as changed;
- **a CMake file anywhere** (a definition added in `src/core/CMakeLists.txt` changes no source
  file): each target's build commands (`ninja -t commands`) are compared with main's, for whose
  tree every preset is configured, and a target whose commands differ is changed.

The checks' scripts also import the repository's Python: picture readers, projections, the
runner's own sweeps. No binary's inputs record that, so **any change under `tools/unirally_lab/`
or to `tools/project.py` selects every step**, and every check against the original runs. A
tooling change is rare and pays for a full selective run.

Every failed query counts as "changed". A step none of whose binaries changed would replay as it
did on main, so it does not run. Paths are not guessed from file names: a race-engine file is in
`front_end_runner`'s inputs too, because the front end runs races, so a race change also selects
the front-end checks.

### 3. What runs on every change, and what runs when its system changes

| Check | Runs | Against |
| --- | --- | --- |
| Builds of four presets with ctest; the synthetic suite (alone); tooling tests; the function-size and native-symbol rules | every change outside `docs/` and `tasks/` | - |
| The task's own checks | every run | the original (acceptance of new behaviour) |
| Front-end replay: every recorded check input through main's `front_end_runner` and the candidate's, every output (pictures on the checks' own picture frames) | `front_end_runner` changed | main |
| Race equivalence sweep (47 scenarios x 9 schedules, restarts, pictures, the legacy runner) | a race runner changed | main |
| Eleven frozen differential gates | `zoom_zoo_runner` changed, or their projection or guards changed | the original, by the contract's frozen digest |
| Track sweeps (track-breadth-2, locked tours) | `zoom_zoo_runner` changed | the original and main |
| Restore checks (two-pad, idle demo) | `front_end_runner` changed | the candidate itself |
| v1 presentation contracts; fuzz | their runner changed | the fixtures; none |
| Hidden app runs; the app's audio producer (real time) | the app changed | status, drops |
| Driver check on captured port writes | `audio_driver_runner` changed | the original |
| The checks against the original that drive `front_end_runner`: pause menu, two-pad, race end, look, restart, idle demo, league frozen, TWO-HUMAN-STUNTS, CREDITS-NAME, WIPE-RAM, DEMO-AUDIO, MODE-AUDIO, split demo, MENU-INPUT and LEAGUE-TOURS per capture, PODIUM-AUDIO | their own recorded runs differ from main (section 4), or they have no recorded runs, or a full run | the original |

### 4. Comparing with main in place of the original (GATES-AGAINST-MAIN)

**The argument.** A check against the original runs `front_end_runner` on its inputs and compares
some of the outputs with the original's capture. If the candidate's outputs equal main's on those
same inputs, the check reads exactly what it read on main. That is where it was accepted, so its
verdict cannot change.

**The inputs are the checks' own.** A full run puts a shim in place of the candidate's
`front_end_runner`. It logs each invocation the checks make (the pads, the pack, the race
initialization frames and record writes the scripts derive from the original, the warm-boot
image), then runs the real binary. These logs are the corpus (`local/gates/corpus/`, one log per
step, input files stored by content). Nothing is re-derived, so the corpus cannot drift from what
the checks run. A task's own checks are recorded on every run, so the next task inherits them.
- **The pack.** The checks get the current pack through an alias. A check that takes `$PACK` is
  then told apart from one that pins a pack, even the same version, and the replay gives a
  `$PACK` case each side's own pack and a pinned case the pinned pack.
- **Staleness.** Each check's recording carries a fingerprint covering three things:
  - its command;
  - the scripts and input recipes (schedules, record writes, runner arguments) of every evidence
    folder the command names, however it names them;
  - the scripts of every folder those scripts name.

  A check runs against the original when its fingerprint has changed, when its recording logged an
  error, or when the replay did not run all its recorded runs.
- **A recording is all or nothing.** A step records into a new log, which becomes its corpus only
  when the step finishes `ran` or `passed` with exit status 0. A timeout, a cap, a stall, a script
  stopped partway, or an interrupted run leaves the earlier recording as it was, and in a
  recording run the step fails.
  Capture manifests and scripts two folders away are outside the fingerprint, because gate steps
  rewrite some reports on every run; changing one calls for a new recording. Only full runs and a
  task's `--task-checks` record.

**The outputs are all of them.** The replay drops the outputs a check asked for and asks both
binaries for every output:
- the state rows;
- stderr;
- the race, one-player and look timelines;
- the sound cues;
- on every frame, a digest of the cartridge RAM;
- on every frame the checks compared a picture on, a digest of the picture and the VRAM.

Each digest is of the bytes the runner would write to the file (`front_end_runner --frame-digests
--picture-frames`, new here; one 64-bit digest per frame and kind). A bug in how a file is written
therefore shows up, not only one in the values behind it.

That is a superset of what any check compares, so "equal" is stronger than the check, not weaker.
An output a check reads that one of the runners cannot give (a newer option) counts as a
difference.

**Where it differs, the original decides.** A check whose recorded runs differ from main in an
output it reads runs against the original. Stdout and stderr count for every check; the timelines,
cues, pictures, cartridge RAM and VRAM count for the checks that asked for them. The candidate
changed something the check measures, and only the original can say whether that is the intended
improvement or a regression. MENU-INPUT's and LEAGUE-TOURS' scripts are split one step per capture,
so one changed screen re-runs only the captures that pass through it.

**Every check gets a verdict.** Most checks against the original print what they measured rather
than pass or fail; the serial `gates.sh` left those lines to a reader. Here each such step has a
twin, `<step>@main`, which runs the same command in main's checkout with main's binaries and pack.
- Lines equal to the twin's mean `same-as-main`: the change did not move what the check measures.
- Different lines mean `changed`, and the run fails. A task that means to change that result reads
  both, records why, and lists the step in `--expect-changed`; then it is `changed (expected)`.

- Equal lines can still mean `broken`, which fails the run: the check broke the same way on both
  sides (a deleted capture, a script error). It is `broken` when it prints fewer lines or more error
  lines than its recording did ("Traceback", "no capture", "no schedule" and the like), or, on the
  main it was recorded on, anything other than what it printed then.
- A task's own checks against the original (`--force`) always run and keep their twin. Their
  `changed` is shown and does not fail the run, and the status line names them to be read.
  `--no-twin` drops the twin for a forced check main's runner cannot run (an option the task
  adds), and that check is read without a verdict.

So a planted regression cannot end in `status=passed` because a measurement went unread (the
coordinating session's review of the first proofs).

**What stays against the original whatever main says:**
- **the task's own checks:** acceptance of new behaviour, and the first acceptance of a capability;
- **the eleven frozen race gates.** `--frozen-original` checks the contract's frozen digest of the
  original's rows instead of re-projecting both captures. That is the same comparison with the
  original, and it gives the same rows digests and restore counts. It saves little time; what it
  buys is that a gate no longer needs its 850 MB captures (section 9). They re-project when anything under `tools/unirally_lab/native/`, `reference/` or `content/`, or
  the native manifests, change (the projection and everything it imports), or with `--reproject`;
- **the track sweeps**, which are cheap;
- **the driver check on captured port writes**, whenever the driver changes;
- **everything, in the full run.**

**Known residual differences** need nothing special: main's output carries them, so equal to main
means unchanged.

**Parts not yet native** have no native output to compare, so this decision does not touch how
they are accepted.

### 5. The full run

`run --full --record` runs every step and every check against the original, and records the corpus
again. It is due:
- before each milestone tag;
- after every fifth merged task, or weekly, whichever comes first;
- whenever a check's script or a captured reference changes.

A full run's result is recorded in `docs/STATE.md`.

### 6. Disk

- **Pictures are never stored by the runner.** The replay reduces every stream to one 8-byte digest
  per line, read from pipes and named pipes as the runners write them.
- **Each step has its own `TMPDIR`.** It is measured every 3 seconds together with the step's output
  directory and anything new in the evidence folders the step is known to write into. A step past
  its cap is stopped and fails. Caps are 1 GB by default and set from measured peaks where a step
  keeps more: 1.5 GB for the MENU-INPUT and LEAGUE-TOURS captures, 2 GB for MENU-INPUT's review captures, 2.5 GB for PODIUM-AUDIO's PCM
  schedules, 3 GB for `two-pad` and `rv-warn`, which keep thousands of pictures. A finished step's
  files over 64 MB are removed from its output.
- **The caps of the running steps may not sum past 6 GB**, so the run's own temp stays under that.
- **No step starts while free disk is under a 4 GB floor plus its cap.**
- **Each step's temp is deleted when it ends**, which also fixes the leaked `wipe-ram` pictures.
- The report records each step's peak and the run's lowest free disk.

### 7. Parallel, and the synthetic suite

Steps run in three lanes:
- **exclusive:** builds, and the synthetic suite. The suite's timeout probe fails under load
  (18 September and 14 October 2026), so it stays alone rather than being loosened;
- **real time:** the app runs, together with each other and with nothing else, so their late-pair
  counts are not load artefacts;
- **pool:** everything else, up to the job count (cores minus four), with wide steps such as the
  sweeps counted at their worker count.

Steps that write into the same evidence folder do not overlap.

### 8. A task's own checks

A task gives its checks to the runner (`--task-checks`):
- as a script;
- better, as a `.list` of independent commands, one per line, which run in parallel.

New comparison scripts should hash pictures in memory rather than write files. When the task
merges, the next task adds its checks to the catalogue, split per capture as MENU-INPUT's and
LEAGUE-TOURS' are.

### 9. Regenerating an original capture

When an escalated check needs a capture that D-0011 removed, the task regenerates it from its
recipe and checks the recorded digests, as D-0011 says. Since the frozen gates now read only their
`reference.json`, their memory captures (`memory.wram`/`memory.sram`, 850 MB per ZOOM ZOO capture)
are needed only to locate a first mismatch or to re-project. That is 14.3 GB (48 files) under the
frozen gates' capture folders. They are candidates for D-0011 deletion with their recipes kept.
Deleting them is a separate step, not part of this decision. Every other capture a catalogue step
reads stays: a selective run may escalate to it, and the full run reads all of them.

## Measured

Runs on 24 and 25 October 2026, Apple M2 Max, 12 cores (8 performance), `--jobs 8`; logs in main
`local/evidence/gates-against-main/`. "Peak temp" is the most the running steps held at once
(their temp and output directories, measured); "peak disk" is the largest drop in free disk
during the run, which also counts the base worktree's build and anything else on the disk.

| Run (runner `649024b`) | Old serial `gates.sh` | Runner | Peak temp / disk | Result |
| --- | --- | --- | --- | --- |
| RECORD-SCREENS' change (main without it, against main with it) | 5 h 17 min (`b6fa277`) | **34.3 min** | 2.7 / 5.3 GB | it adds sources in `src/core/CMakeLists.txt`, and the command comparison leaves `zoom_zoo_runner` and the audio runners unchanged, so the race gates and PCM schedules do not run; its 36 checks run in parallel, as its own, without twins (main's runner has no `--rider-word`) and listed to read; `changed`: `fe-equivalence`, `fe-classify`, `mi-rv-records`, the records screens it rewrote |
| PODIUM-AUDIO's change (main without it or RECORD-SCREENS, against that with it) | 4 h 32 min (`b7efb93`) | **43.6 min** | 3.9 / 4.0 GB | it touched the race engine and the podium's sound: the eleven gates passed; its own checks keep their twins: `pa-cues` and the six PCM schedules `changed (own check)`, the podium now matching the original (`tour-cues`: 4 podium frames differ on main, 0 on the candidate); `changed`: `lt-cues`, LEAGUE-TOURS' tour schedule, which that fix moves |
| Full run with recording (main `17aee63` with the runner) | - | **83.4 min** | 3.1 / 4.0 GB | every step, `status=passed`, no recording errors |

The disk figure is the drop in free space, which counts anything else written meanwhile (the
base worktree's rebuild among it); the temp figure is what the run's steps held.

- **Same results as the serial gates.** The full run's lines were compared with PODIUM-AUDIO's
  last serial run (same source as main), section by section, and agree. The exceptions are the
  real-time app's late-pair counts, which vary from run to run (2,686 to 3,775 dropped pairs on
  the same code). The frozen gates give the same rows digests and restore counts with
  `--frozen-original` as with re-projection.
- **The replay.** 426 cases (9.0 M frames, 308 K pictures) through both binaries take 6 minutes.
- **Deliberate regressions** (one commit each, local branches `gam/reg-*`, never pushed), each
  selected through its binary, each ending `status=failed`:

  | Regression | Run | What fails |
  | --- | --- | --- |
  | Race state: `drag_step` 3 to 4 (`speed_limits.cpp`) | runner `9b1ce41`, the race steps | all eleven frozen gates ("native rows differ from the frozen original rows"), the race sweep (423 differing runs), both track sweeps; the replay finds 285 of 426 cases differing |
  | Front-end picture: one bit of one pixel on every 1000th frame | runner `86b9466`, the replay and three of the checks it escalates, with their twins | `credits-name` and `mi-kb-input` `changed`; the replay found it in 100 cases, only in the pictures. The old every-500th-frame sweep (`fe-equivalence`) is not among them: its frames miss the regression's |
  | Sound cues: one frame wait dropped on the main menu | runner `86b9466`, as the picture | `demo-audio-cues` and `mi-cues` `changed`; the replay found it in 416 cases, only in the cue log |
  | Audio driver: `counter_threshold` 4 to 5 (`audio_driver.cpp`) | runner `49604f4`, the whole selective run (39.1 min, 3.9 GB) | `pa-cond` and `demo-audio-cond` `changed` (first row difference 50122, PCM not equal); the replay is equal on all 426 cases, rightly, since the driver is not on the cue path |

  A picture or cue regression in a task's whole run would also run the other checks it escalates
  (83 and 95) and their twins; the three per run here show the escalated checks fail.
- **The review's planted cases** (round 1; runner `b1ec9bb` unless named), each now caught:

  | Case | Before the fixes | Now |
  | --- | --- | --- |
  | B: `front_end_runner` writes PPM files with red and blue swapped | the replay equal, every picture check skipped | the replay's digests are of the file bytes: `mi-kb-input` `changed` |
  | A: `parse_ppm` (the checks' picture reader) drops a byte | every corpus check skipped | `tools/unirally_lab/` changed selects every check; `mi-kb-input` and `split-demo` `changed` against their twins, which run main's reader |
  | C: a compile definition in `src/core/CMakeLists.txt` | every binary "same" | runner `5bff2f9`: every binary's build commands differ from main's, so all are selected (the definition is harmless and they pass) |
  | D (round 2): every output file loses its last byte in the one writer | the replay equal | runner `649024b`: files and digests share `write_bytes`; `mi-kb-input` `changed` (`rev-d.out`) |

- **Why RECORD-SCREENS' change once took 66.6 minutes** (runner `b1ec9bb`): the first CMake rule
  marked every binary changed because it adds sources, and its forced checks had twins on main,
  which cannot run its `--rider-word`. The command comparison and "a task's own checks have no
  twin" took it back to 34.1 minutes.
- **What the first versions got wrong**, each found by these runs:
  - Drawing a picture every frame costs 2.2 ms (44.5 s per 20,000 frames against 0.4 s without),
    so the picture digests follow the checks' own picture frames.
  - Running the app's audio producer beside the hidden runs doubled its late pairs, so it runs
    alone.
  - A run whose one step's dependency was left out waited four hours holding the lock. The
    scheduler now fails steps that can never start, and stops a step with no CPU and no output
    for 15 minutes.
  - A check that only reported let a planted driver regression end `status=passed`, so every such
    check now has its twin on main.
  - A step's output directory was not measured, so one run held 10.5 GB against its 6 GB budget.
    Output now counts, bulk over 64 MB is removed when a step ends, and the measured drop in free
    disk holds back new steps.

## Consequences

- **A change meant to move a check costs its twin as well.** A changed check runs twice, on the
  candidate and on main.
- **Stdout counts for every check.** The state rows are the one output every check can read
  without asking. A change that moves them escalates every check whose runs it reaches.
- A typical task's gates are a selective run; the long checks against the original run only where
  the change moved their output.
- Main's native output becomes the reference for "nothing else moved". A regression accepted into
  main would be inherited; the periodic full run, which compares everything with the original, is
  the guard against that.
- The corpus is local and ROM-derived in part (record writes, warm-boot images); it follows the
  evidence rules and is re-recorded by any full run.
- A check whose command or scripts changed runs against the original until a full run records it
  again.
